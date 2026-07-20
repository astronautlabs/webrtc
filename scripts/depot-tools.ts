import { mkdirpSync } from "mkdirp";
import path from 'node:path';
import { addToPath, dirExists, popDir, pushDir, run } from "./utils";

const DEFAULT_GIT_REPOSITORY = 'https://chromium.googlesource.com/chromium/tools/depot_tools.git';

export interface DepotToolsOptions {
    repository?: string;
}
export async function acquireDepotTools(dir: string, options?: DepotToolsOptions) {

    // The ninja wrapper (and possibly other parts of depot_tools) rely on it being in a directory called
    // `depot_tools` or else it will infinitely call itself. Ensure this is always the case.
    if (path.basename(dir) != 'depot_tools')
        throw new Error(`Final path segment must be 'depot_tools'`);

    if (!await dirExists(dir)) {
        mkdirpSync(path.dirname(dir));
        run("git", [
            "clone",
            "--filter=blob:none",
            options?.repository ?? DEFAULT_GIT_REPOSITORY,
            dir
        ]);
    }

    process.env.DEPOT_TOOLS = path.resolve(dir);
    addToPath(path.resolve(dir));
}


export interface GClientConfigOptions {
    unmanaged?: boolean;
    spec?: string;
}

export interface GClientSyncOptions {
    shallow?: boolean;
    noHistory?: boolean;
    noHooks?: boolean;
    withBranchHeads?: boolean;
    revision?: string;
    reset?: boolean;
}

export class gclient {
    static config(options?: GClientConfigOptions) {
        let args: string[] = [];
        if (options?.unmanaged)
            args.push('--unmanaged');
        if (options?.spec)
            args.push('--spec', options.spec);
        run("gclient.py", [ "config", ...args]);
    }

    static sync(options?: GClientSyncOptions) {
        let args: string[] = [];

        if (options?.shallow)
            args.push('--shallow');
        if (options?.noHistory)
            args.push('--no-history');
        if (options?.noHooks)
            args.push('--nohooks');
        if (options?.withBranchHeads)
            args.push('--with_branch_heads');
        if (options?.revision)
            args.push('-r', options.revision);
        if (options?.reset)
            args.push('-R');

        run("gclient.py", [ "sync", ...args]);
    }
}

export class gn {
    static gen(outDir: string, args: string[]) {
        run('gn.py', [
            'gen',
            outDir,
            `--args=${args.join(' ')}`
        ]);
    }
}
