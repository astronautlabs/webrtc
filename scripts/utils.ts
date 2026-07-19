import { spawnSync, execSync } from "node:child_process";
import os from 'os';
import path from 'path';
import fs from 'fs';

export function runJS(script: string, args: string[]) {
    let cmdLine = `${script} ${args.map(x => x.includes(' ') ? `"${x}"` : x).join(' ')}`;
    console.log(`\n> ${cmdLine}`);
    let code = spawnSync(process.execPath, [script, ...args], { stdio: "inherit" }).status;
    if (code !== 0)
        throw new Error(`Script failed (exit code ${code}): ${cmdLine}`);
    return code;
}

export function run(executable: string, args: string[]) {
    let executablePath = findProgram(executable);
    let cmdLine = `${executable} ${args.map(x => x.includes(' ') ? `"${x}"` : x).join(' ')}`;
    console.log(`\n> ${cmdLine}`);

    if (!executablePath)
        throw new Error(`Could not find '${executable}' on the PATH. PATH was: ${(process.env.PATH ?? '').split(path.delimiter).map(x => ` - ${x}`).join(`\n`)}`);

    let code = spawnSync(executablePath, [...args], { stdio: "inherit" }).status;
    if (code !== 0)
        throw new Error(`Command failed (exit code ${code}): ${cmdLine}`);
    return code;
}

export function addToPath(dir: string) {
    process.env.PATH = [ ...process.env.PATH?.split(path.delimiter) ?? [], dir ].join(path.delimiter);
}

const dirStack: string[] = [];
export function pushDir(dir: string) {
    dirStack.push(process.cwd());
    console.log(`> cd '${dir}'`);
    process.chdir(dir);
}

export function popDir() {
    if (dirStack.length === 0)
        throw new Error(`No directory to pop`);

    let dir = dirStack.pop()!;
    console.log(`< cd '${dir}'`);
    process.chdir(dir);
}

export function withDir<T>(dir: string, func: () => T): T {
    pushDir(dir);
    try {
        return func();
    } finally {
        popDir();
    }
}

export function withEnv<T>(env: Record<string, string>, func: () => T): T {
    let previousEnv = structuredClone(process.env);
    try {
        Object.assign(process.env, env);
        return func();
    } finally {
        Object.keys(process.env).forEach(key => delete process.env[key]);
        Object.assign(process.env, previousEnv);
    }
}

export function findProgram(name: string): string | undefined;
export function findProgram(names: string[]): string | undefined;
export function findProgram(names: string[] | string): string | undefined {
    if (typeof names === 'string')
        names = [ names ];

    for (let name of names) {
        let dirs = process.env.PATH?.split(path.delimiter) ?? [];
        for (let dir of dirs) {
            let pathExt = os.platform() === 'win32'
                ? (process.env.PATHEXT || '').split(path.delimiter)
                : [''];

            for (let ext of pathExt) {
                if (fs.existsSync(path.join(dir, `${name}${ext}`))) {
                    return path.join(dir, `${name}${ext}`);
                }
            }
        }
    }

    return undefined;
}

export function prependToPath(dir: string) {
    process.env.PATH = [
        ...(process.env.PATH ? process.env.PATH.split(path.delimiter) : []),
        dir
    ].join(path.delimiter);
}

export async function fileExists(filename: string) {
    try {
        let s = await stat(filename);
        return s.isFile() || s.isDirectory();
    } catch (e) {
        return false;
    }
}

export async function stat(filename: string) {
    return await new Promise<fs.Stats>((rs, rj) => fs.stat(filename, (e, s) => e ? rj(e) : rs(s)));
}

export async function dirExists(filename: string) {
    try {
        let s = await stat(filename);
        return s.isDirectory();
    } catch (e) {
        return false;
    }
}

export function injectVSEnvironment() {
    const vswhere = '"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe"';
    const vsQuery = `${vswhere} -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`;

    let vsPath;
    try {
        vsPath = execSync(vsQuery, { encoding: 'utf8' }).trim();
    } catch (e) {
        throw new Error("Failed to locate Visual Studio with C++ tools via vswhere.");
    }

    if (!vsPath)
        throw new Error("Visual Studio path resolved to empty.");

    const vcvarsPath = `${vsPath}\\VC\\Auxiliary\\Build\\vcvars64.bat`;

    // 2. Execute vcvars64.bat and chain the `set` command to dump the loaded environment
    // Note: We route stdout to NUL for vcvars to prevent noise, but let `set` print to stdout.
    const envCommand = `"${vcvarsPath}" >nul 2>&1 && set`;
    const envOutput = execSync(envCommand, { shell: 'cmd.exe', encoding: 'utf8' });

    // 3. Parse the output and inject it into the current Node process environment
    const lines = envOutput.split('\n');
    for (const line of lines) {
        const match = line.trim().match(/^([^=]+)=(.*)$/);
        if (match) {
            const key = match[1];
            const value = match[2];

            // We optionally merge PATH so we don't lose Node's existing paths
            if (key.toUpperCase() === 'PATH') {
                process.env[key] = `${value};${process.env[key]}`;
            } else {
                process.env[key] = value;
            }
        }
    }
}

/**
 * Runs a certain number of activities simultaneously
 */
export class ConcurrentWorkQueue {
    constructor(readonly concurrency: number) {
    }

    private pending: Promise<void>[] = [];

    /**
     * Enqueue some work to do concurrently. Upon resolution, its guaranteed that the work has
     * started. Will block while there are too many in-flight tasks. Work function should always
     * resolve (not reject), otherwise it will be considered an unhandled promise rejection.
     * @param work
     */
    async run(work: () => Promise<void>): Promise<void> {
        while (this.pending.length > this.concurrency)
            await Promise.any(this.pending);

        let promise = work();
        this.pending.push(promise);
        promise.finally(() => {
            let index = this.pending.indexOf(promise);
            if (index >= 0)
                this.pending.splice(index, 1);
        });
    }

    async join() {
        await Promise.all(this.pending);
    }
}

export async function writeTextFile(filename: string, content: string) {
    await new Promise<void>((resolve, reject) => {
        fs.writeFile(filename, content, err => err ? reject(err) : resolve());
    });
}
