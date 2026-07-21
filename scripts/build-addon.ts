#!/usr/bin/env node

import os from "node:os";

import { CMAKE_JS, findProgram, injectVSEnvironment, main, parseBuildToolArgs, runJS } from "./utils";

main(async args => {
    let { buildType } = parseBuildToolArgs();
    const platform = os.platform();
    const arch = process.env.TARGET_ARCH ?? os.arch();
    const isCrossCompile = arch !== os.arch();
    const buildFolder = isCrossCompile ? `build/${platform}-${arch}` : `build`;
    const cmakeJsArgs = [
        "-O", buildFolder,
        "-a", arch,
        "--CDCMAKE_EXPORT_COMPILE_COMMANDS=1"
    ];

    if (buildType === 'Debug')
        cmakeJsArgs.push("--debug");

    if (platform === "win32") {
        injectVSEnvironment();
        cmakeJsArgs.push(...[
            '-G', 'Ninja',
            '--CDCMAKE_C_COMPILER=C:/Program Files/LLVM/bin/clang-cl.exe',
            '--CDCMAKE_CXX_COMPILER=C:/Program Files/LLVM/bin/clang-cl.exe',
            '--CDCMAKE_LINKER=C:/Program Files/LLVM/bin/lld-link.exe',
            '--CDCMAKE_SYSTEM_NAME=Windows'
        ]);

        // Find the real rc.exe from the Windows SDK since cmake-js will find the npm "rc" package
        let rcExe = findProgram("rc.exe");
        if (rcExe)
            cmakeJsArgs.push(`--CDCMAKE_RC_COMPILER=${rcExe.replace(/\\/g, "/").trim()}`);
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

    let status = runJS(CMAKE_JS, ["configure", ...cmakeJsArgs]);
    if (status)
        throw new Error("cmake-js configure failed for wrtc");

    console.log();
    console.log(`------------------------------------------------`);
    console.log("Running: cmake-js build " + cmakeJsArgs.join(" "));
    status = runJS(CMAKE_JS, ["build", ...cmakeJsArgs]);
    if (status)
        throw new Error("cmake-js build failed for wrtc");
});