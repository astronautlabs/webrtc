#!/usr/bin/env node
"use strict";

import os from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";

const __dirname = import.meta.dirname ? import.meta.dirname 
    : import.meta.filename ? path.dirname(import.meta.filename)
    : path.dirname(new URL(import.meta.url).pathname)
;

const CMAKE_JS = path.resolve(__dirname, "..", "node_modules", ".bin", "cmake-js");

function main(args) {
    if (args[0] === '--help') {
        console.log(`@astronautlabs/webrtc: build script`);
        process.exit(0);
    }
    const isDevWorkspace = args[0] === 'workspace';
    const debugMode = process.env.DEBUG === '1';
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
        cmakeJsArgs.push(...['--toolset=ClangCL']);

        // Explicitly find the real rc.exe from the Windows SDK and pass it to
        // CMake, since cmake-js may still find the npm "rc" package otherwise.
        const { stdout } = spawnSync("where", ["rc.exe"], { encoding: "utf-8" });
        if (stdout) {
            cmakeJsArgs.push(`--CDCMAKE_RC_COMPILER="${stdout.replace(/\\/g, "/")}"`);
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
    let { status } = spawnSync(CMAKE_JS, ["configure", ...cmakeJsArgs], {
        shell: true,
        stdio: "inherit",
    });
    if (status)
        throw new Error("cmake-js configure failed for wrtc");

    console.log();
    console.log(`------------------------------------------------`);
    console.log("Running: cmake-js build " + cmakeJsArgs.join(" "));
    status = spawnSync(CMAKE_JS, ["build", ...cmakeJsArgs], {
        shell: true,
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

main(process.argv.slice(2))