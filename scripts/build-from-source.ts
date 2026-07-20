#!/usr/bin/env node

import os from "node:os";
import path from "node:path";

import { spawnSync } from "node:child_process";
import { buildWebRTC } from "./build-webrtc";
import { dirExists, injectVSEnvironment, prependToPath, runJS } from "./utils";

/**
 * For available branches, see https://chromiumdash.appspot.com/branches
 * - M87:   branch-heads/4280
 * - M91:   branch-heads/4472
 * - M92:   branch-heads/4515
 * - M94:   branch-heads/4606
 * - M98:   branch-heads/4758
 * - M102:  branch-heads/5005
 * - M106:  branch-heads/5249
 * - M110:  branch-heads/5481
 * - M114:  branch-heads/5735
 * - M150:  branch-heads/7871
 */
const WEBRTC_REVISION = 'branch-heads/7871';

const CMAKE_JS = path.resolve(path.resolve(__dirname), "..", "node_modules", "cmake-js", "bin", "cmake-js");
const WEBRTC_DEPENDENCIES = [
    'webrtc',
    'builtin_video_encoder_factory',
    'builtin_video_decoder_factory',
    'rtc_software_fallback_wrappers',
    'rtc_internal_video_codecs',
    'media_engine',
    'rtc_simulcast_encoder_adapter'
];

async function main(args: string[]) {
    if (args[0] === '--help') {
        console.log(`@astronautlabs/webrtc: build script`);
        process.exit(0);
    }
    const isDevWorkspace = args[0] === 'workspace';
    const debugMode = isDevWorkspace || ['1', 'true'].includes(process.env.DEBUG ?? '');
    const platform = os.platform();
    const arch = process.env.TARGET_ARCH ?? os.arch();
    const buildFolder = isDevWorkspace ? `build` : `build/${platform}-${arch}`;
    const cmakeJsArgs = [
        "-O", buildFolder,
        "-a", arch,
        "--CDCMAKE_EXPORT_COMPILE_COMMANDS=1"
    ];

    if (debugMode)
        cmakeJsArgs.push("--debug");

    prependToPath(path.resolve(__dirname, 'ninja', os.platform()));

    const libwebrtcDir = path.resolve(__dirname, '..', buildFolder, 'external', 'webrtc');

    // Build libwebrtc

    await buildWebRTC(libwebrtcDir, {
        buildType: debugMode ? 'Debug' : 'Release',
        targets: WEBRTC_DEPENDENCIES,
        revision: WEBRTC_REVISION
    });

    // Prepare to build the addon

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

    let status = runJS(CMAKE_JS, ["configure", ...cmakeJsArgs]);
    if (status)
        throw new Error("cmake-js configure failed for wrtc");

    console.log();
    console.log(`------------------------------------------------`);
    console.log("Running: cmake-js build " + cmakeJsArgs.join(" "));
    status = runJS(CMAKE_JS, ["build", ...cmakeJsArgs]);
    if (status)
        throw new Error("cmake-js build failed for wrtc");
}

main(process.argv.slice(2))

