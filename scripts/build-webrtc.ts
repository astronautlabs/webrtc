#!/usr/bin/env node
"use strict";

import child_process from "node:child_process";
import fs from "node:fs/promises";
import os from "node:os";
import path from "node:path";

import { globSync } from 'glob';
import { rimraf } from 'rimraf';
import { mkdirp, mkdirpSync } from 'mkdirp';
import { randomUUID } from "node:crypto";
import { acquireDepotTools, gclient, gn } from "./depot-tools";
import { ConcurrentWorkQueue, dirExists, fileExists, findProgram, run, runWithInput, withDir, withEnv, writeTextFile } from "./utils";

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

    const buildCompleteMarker = path.join(outDir, 'build-complete');
    const srcDir = path.resolve(webrtcRoot, 'src');
    const intermediateDir = path.resolve(webrtcRoot, 'intermediate');
    const ccachePath = findProgram(['sccache', 'ccache']);
    const targets = ['libjingle_peerconnection', ...options.targets ?? []];

    if (await fileExists(buildCompleteMarker))
        return;

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

    const libDir = path.join(outDir, 'lib');
    let outCppLib = path.resolve(libDir, 'c++.lib');

    if (os.platform() === 'win32' && !await fileExists(outCppLib)) {
        console.log(`\n> linking libc++`);
        let objectFiles = globSync(`${path.join(intermediateDir, 'obj', 'buildtools', 'third_party', 'libc++', 'libc++')}/*.obj`);
        await mkdirp(libDir);
        run('lld-link', [
            '/lib',
            `/out:${libDir}/c++.lib`,
            ...objectFiles
        ]);
    }

    await linkMonolithicLibrary(intermediateDir, libDir);

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

    await writeTextFile(buildCompleteMarker, '');
    console.log(`\n> done. output is at ${outDir}`);
}

async function collectHeaders(fromDir: string, toDir: string, patterns: string[] = ['*.h'], skipPatterns: RegExp[] = []) {
    fromDir = path.resolve(fromDir);
    toDir = path.resolve(toDir);

    let headers = globSync(patterns.map(pattern => `${fromDir}/**/${pattern}`)).filter(x => !skipPatterns.some(p => p.test(x)));
    let queue = new ConcurrentWorkQueue(5000);

    await Promise.all(headers.map(async headerFile => {
        headerFile = path.resolve(headerFile);

        if (await dirExists(headerFile))
            return;
        let relativePath = headerFile.slice(fromDir.length);
        let finalPath = path.join(toDir, relativePath);

        await queue.run(async () => {
            try {
                await mkdirp(path.dirname(finalPath));
                await fs.copyFile(headerFile, finalPath);
            } catch (e) {
                throw new Error(
                    `Failed to copy header '${headerFile}' to '${finalPath}' (relative path '${relativePath}') while collecting headers from '${fromDir}' -> '${toDir}' `
                        + `[patterns: ${patterns.join(', ')}]`,
                    { cause: e }
                );
            }
        });
    }));

    await queue.join();
}

export function flattenThinArchive(
    inputFile: string,
    outputFile: string
) {
    const llvmAr = findProgram('llvm-ar');
    if (!llvmAr)
        throw new Error(`Cannot find 'llvm-ar', please make sure Clang 22 or later is installed and that llvm-ar is available on the PATH`);

    return new Promise<void>((resolve, reject) => {
        let proc = child_process.spawn(
            llvmAr, ['-M'], {
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

async function linkMonolithicLibrary(intermediateDir: string, outDir: string) {
    console.log(`\n> linking libwebrtc...`);
    let libPattern = os.platform() === 'win32' ? `*.lib` : `lib*.a`;
    let libraries = [
        ...globSync(`${intermediateDir}/**/${libPattern}`)
    ].map(x => path.resolve(x));
    mkdirpSync(outDir);

    if (os.platform() === 'win32') {
        // RSP
        let rspFile = path.join(os.tmpdir(), `webrtc-link-${randomUUID()}.rsp`);
        try {
            await writeTextFile(rspFile, libraries.join(`\r\n`));
            run('llvm-lib', [`/OUT:${outDir}\webrtc.lib`, `@${rspFile}`]);
        } finally {
            await fs.unlink(rspFile);
        }
    } else {
        // MRI script
        await runWithInput(
            'llvm-ar', ['-M'],
            [
                `create ${path.resolve(outDir)}/libwebrtc.a`,
                ...libraries.map(lib => `addlib ${path.resolve(lib)}`),
                `save`,
                `end`
            ].join(`\n`)
        );
    }

    // await Promise.all(libraries.map(async lib => {
    //     await flattenThinArchive(lib, path.join(outDir, path.basename(lib)));
    //     console.log(`- flattened .../lib/${path.basename(lib)} [from ${lib}]`);
    // }));
}
