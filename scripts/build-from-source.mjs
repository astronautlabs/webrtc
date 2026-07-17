#!/usr/bin/env node
"use strict";

import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync, execSync } from "node:child_process";

const __dirname = import.meta.dirname ?? path.dirname(fileURLToPath(import.meta.url));
const CMAKE_JS = path.resolve(path.resolve(__dirname), "..", "node_modules", "cmake-js", "bin", "cmake-js");

function main(args) {
    if (args[0] === '--help') {
        console.log(`@astronautlabs/webrtc: build script`);
        process.exit(0);
    }
    const isDevWorkspace = args[0] === 'workspace';
    const debugMode = ['1', 'true'].includes(process.env.DEBUG);
    const platform = os.platform();
    const arch = process.env.TARGET_ARCH ?? os.arch();
    const buildFolder = isDevWorkspace ? `build` : `build/${platform}-${arch}`;
    const cmakeJsArgs = [
        "-O", buildFolder,
        "-a", arch,
        "--CDCMAKE_EXPORT_COMPILE_COMMANDS=1"
    ];

    if (isDevWorkspace || debugMode)
        cmakeJsArgs.push("--debug");

    prependToPath(path.resolve(__dirname, 'ninja', os.platform()));

    if (platform === "win32") {
        injectVSEnvironment();

        // We really need VS 2026, because Clang 19 (from VS 2022) is too old to handle the object files that libwebrtc
        // will compile using Clang 23.
        cmakeJsArgs.push(...[
            '-G', 'Ninja',
            '--CDCMAKE_C_COMPILER=C:/Program Files/LLVM/bin/clang-cl.exe',
            '--CDCMAKE_CXX_COMPILER=C:/Program Files/LLVM/bin/clang-cl.exe',
            '--CDCMAKE_LINKER=C:/Program Files/LLVM/bin/lld-link.exe',
            '--CDCMAKE_SYSTEM_NAME=Windows'
        ]);

        // Explicitly find the real rc.exe from the Windows SDK and pass it to
        // CMake, since cmake-js may still find the npm "rc" package otherwise.
        const { stdout } = spawnSync("where", ["rc.exe"], { encoding: "utf-8" });
        if (stdout) {
            cmakeJsArgs.push(`--CDCMAKE_RC_COMPILER=${stdout.replace(/\\/g, "/").trim()}`);
        }
    }

    if (arch !== os.arch()) {
        console.log(`Setting up cross-compilation (building ${platform}-${arch} on ${os.platform()}-${os.arch()})`);
        cmakeJsArgs.push(
            `--CDCMAKE_TOOLCHAIN_FILE=toolchains/${platform}-${arch}.toolchain`,
        );
    }

    console.log();
    console.log(`------------------------------------------------`);
    console.log("Running: cmake-js configure " + cmakeJsArgs.join(" "));
    let { status } = spawnSync(process.execPath, [CMAKE_JS, "configure", ...cmakeJsArgs], {
        stdio: "inherit",
    });
    if (status)
        throw new Error("cmake-js configure failed for wrtc");

    console.log();
    console.log(`------------------------------------------------`);
    console.log("Running: cmake-js build " + cmakeJsArgs.join(" "));
    status = spawnSync(process.execPath, [CMAKE_JS, "build", ...cmakeJsArgs], {
        stdio: "inherit",
    }).status;
    if (status)
        throw new Error("cmake-js build failed for wrtc");
}

function prependToPath(dir) {
    process.env.PATH = [
        ...(process.env.PATH ? process.env.PATH.split(path.delimiter) : []),
        dir
    ].join(path.delimiter);
}

function injectVSEnvironment() {
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

main(process.argv.slice(2))

