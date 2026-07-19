#!/usr/bin/env node
"use strict";

import fs from "node:fs/promises";
import os from "node:os";
import child_process from "node:child_process";
import path from "node:path";

import { rimraf } from 'rimraf';
import { globSync } from 'glob';
import { mkdirp, mkdirpSync } from 'mkdirp';
import { acquireDepotTools, gclient, gn } from "./depot-tools";
import { ConcurrentWorkQueue, dirExists, fileExists, findProgram, run, withDir, withEnv } from "./utils";
import { randomUUID } from "node:crypto";

export interface WebRTCBuildOptions {
    /**
     * Whether to use ccache/sccache when available.
     */
    useCCache?: boolean;
    buildType?: 'Debug' | 'Release';
    targets?: string[];
    revision?: string;
}

function downloadWebRTC(webrtcRoot: string, revision: string) {
    mkdirpSync(webrtcRoot);
    withDir(webrtcRoot, () => {
        gclient.config({
            spec: `solutions=[{"name":"src","url":"https://webrtc.googlesource.com/src.git"}]`,
            unmanaged: true
        });
        gclient.sync({
            shallow: true,
            noHistory: true,
            noHooks: true,
            withBranchHeads: true,
            revision,
            reset: true
        });

        if (os.platform() === 'win32')
            run('python', ['src/build/vs_toolchain.py', 'update', '--force']);

        run('python', ['src/build/util/lastchange.py', '-o', 'src/build/util/LASTCHANGE']);
        run('python', ['src/tools/clang/scripts/update.py']);
    });
}

export async function buildWebRTC(outDir: string, options?: WebRTCBuildOptions) {
    outDir = path.resolve(outDir);
    options ??= {};
    options.buildType ??= 'Release';

    let webrtcRoot = path.resolve(__dirname, '..', `webrtc-buildroot`);
    mkdirpSync(webrtcRoot);

    console.log(`\n> building libwebrtc in ${webrtcRoot}`);

    const srcDir = path.resolve(webrtcRoot, 'src');
    const intermediateDir = path.resolve(webrtcRoot, 'intermediate');
    const ccachePath = findProgram(['sccache', 'ccache']);
    const targets = ['libjingle_peerconnection', ...options.targets ?? []];

    if (os.platform() === 'win32')
        process.env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0';

    await acquireDepotTools(path.join(webrtcRoot, 'depot_tools'));

    // Download

    if (!await dirExists(path.join(webrtcRoot, 'src')))
        downloadWebRTC(webrtcRoot, options?.revision ?? 'branch-heads/7871');

    let env: Record<string, string> = {};

    if (os.platform() === 'win32') {
        env.PATH = `${process.env.DEPOT_TOOLS};${process.env.__VSCMD_PREINIT_PATH}`;
        env.INCLUDE = '';
        env.LIB = '';
        env.LIBPATH = '';
        env.VSCMD_ARG_TGT_ARCH = '';
        env.VSCMD_ARG_HOST_ARCH = '';
        env.__VSCMD_PREINIT_VSINSTALLDIR = '';
    }

    // sysroot

    if (os.platform() !== 'win32') {
        let arch = process.env.TARGET_ARCH ?? os.arch();
        if (arch === 'x64')
            arch = 'amd64';
        run('python', [
            path.join(srcDir, 'build/linux/sysroot_scripts/install-sysroot.py'),
            `--arch=${process.env.TARGET_ARCH ?? os.arch()}`
        ]);
    }

    // configure

    if (!await dirExists(intermediateDir)) {
        withEnv(env, () => {
            withDir(srcDir, () => {
                gn.gen(intermediateDir, [
                    // NOTE: These are python expressions, and thus string values must be quoted
                    'rtc_build_examples=false',
                    'rtc_use_x11=false',
                    'rtc_enable_protobuf=false',
                    'rtc_include_pulse_audio=false',
                    'rtc_include_tests=false',
                    'use_lld=true',
                    'use_custom_libcxx=true',
                    `rtc_build_tools=${['arm64', 'arm'].includes(os.arch()) ? 'true' : 'false'}`,
                    `target_cpu="${process.env.TARGET_ARCH ?? os.arch()}"`,
                    `is_debug=${options.buildType === 'Debug' ? 'true' : 'false'}`,
                    ...(options?.useCCache && ccachePath ? [`cc_wrapper="${ccachePath}"`] : [])
                ]);
            });
        });
    }
    // Build

    withDir(intermediateDir, () => {
        run('ninja', [...targets, '-j', process.env.PARALLELISM ?? '24']);
    });

    // Collect libraries

    console.log(`\n> collecting libraries...`);
    let libPattern = os.platform() === 'win32' ? `*.lib` : `lib*.a`;
    let libraries = [
        ...globSync(`${intermediateDir}/**/${libPattern}`)
    ];
    let libDir = path.join(outDir, 'lib');
    let includeDir = path.join(outDir, 'include');
    mkdirpSync(libDir);
    mkdirpSync(includeDir);
    await Promise.all(libraries.map(async lib => {
        await flattenThinArchive(webrtcRoot, lib, path.join(libDir, path.basename(lib)));
        console.log(`- flattened ${path.basename(lib)}`);
    }));

    // Collect headers

    console.log(`\n> collecting headers...`);
    await collectHeaders(srcDir, path.join(outDir, 'include', 'webrtc'), ['*.h', '*.inc'], [/third_party/, /-sysroot/]);
    await collectHeaders(
        path.join(srcDir, 'third_party', 'libc++', 'src', 'include'),
        path.join(outDir, 'include', 'webrtc', 'third_party', 'libc++', 'src', 'include'),
        ['*'],
        []
    );
    await collectHeaders(
        path.join(srcDir, 'third_party', 'abseil-cpp', 'absl'),
        path.join(outDir, 'include', 'webrtc', 'third_party', 'abseil-cpp', 'absl'),
        ['*'],
        []
    );
    await collectHeaders(
        path.join(srcDir, 'third_party', 'libyuv', 'include'),
        path.join(outDir, 'include', 'webrtc', 'third_party', 'libyuv', 'include'),
        ['*'],
        []
    );
    await collectHeaders(
        path.join(srcDir, 'third_party', 'libc++', 'src', 'include'),
        path.join(outDir, 'include', 'webrtc', 'third_party', 'libc++', 'src', 'include'),
        ['*'],
        []
    );
    await collectHeaders(
        path.join(srcDir, 'buildtools', 'third_party', 'libc++'),
        path.join(outDir, 'include', 'webrtc', 'buildtools', 'third_party', 'libc++'),
        ['*'],
        []
    );
    await collectHeaders(
        path.join(srcDir, 'third_party', 'libc++abi', 'src', 'include'),
        path.join(outDir, 'include', 'webrtc', 'third_party', 'libc++abi', 'src', 'include'),
        ['*'],
        []
    );

    console.log(`\n> cleaning up build directory ${webrtcRoot}...`);
    await rimraf(webrtcRoot);

    console.log(`\n> done. output is at ${outDir}`);
}

async function collectHeaders(fromDir: string, toDir: string, patterns: string[] = ['*.h'], skipPatterns: RegExp[] = []) {
    let headers = globSync(patterns.map(pattern => `${fromDir}/**/${pattern}`)).filter(x => !skipPatterns.some(p => p.test(x)));
    let queue = new ConcurrentWorkQueue(5000);

    await Promise.all(headers.map(async headerFile => {
        if (await dirExists(headerFile))
            return;
        let relativePath = headerFile.slice(fromDir.length);
        let finalPath = path.join(toDir, relativePath);

        await queue.run(async () => {
            await mkdirp(path.dirname(finalPath));
            await fs.copyFile(headerFile, finalPath);
        });
    }));

    await queue.join();
}

export function flattenThinArchive(
    webrtcRoot: string,
    inputFile: string,
    outputFile: string
) {
    const llvmArExe = os.platform() === 'win32' ? 'llvm-ar.exe' : 'llvm-ar';
    const llvmArPath = path.join(webrtcRoot, 'src', 'third_party', 'llvm-build', 'Release+Asserts', 'bin', llvmArExe);

    return new Promise<void>((resolve, reject) => {
        let proc = child_process.spawn(
            llvmArPath, ['-M'], {
                stdio: ['pipe', 'inherit', 'inherit']
            }
        );
        proc.on('exit', code => code ? reject(new Error(`Failed to flatten thin archive '${inputFile}': exit code ${code}`)) : resolve());
        proc.stdin.write([
            `create ${outputFile}`,
            `addlib ${inputFile}`,
            `save`,
            `end`
        ].join(`\n`));
        proc.stdin.end();
    });
}
