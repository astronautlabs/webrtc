# @/webrtc

> 🚧 **Work In Progress**  
> This library is in an alpha state. It is not yet ready for production use.

> 📺 Part of the [**Astronaut Labs Broadcast Suite**](https://github.com/astronautlabs/broadcast)

> Originally published as [`wrtc`](https://github.com/node-webrtc/node-webrtc), forked primarily for use in
> [Astronaut Labs Broadcast Stack](https://github.com/astronautlabs/broadcast). Feel free to use for your own purposes
> as well.

[![NPM](https://img.shields.io/npm/v/@astronautlabs/webrtc.svg)](https://www.npmjs.com/package/@astronautlabs/webrtc) [![Build Status](https://circleci.com/gh/astronautlabs/webrtc/tree/main.svg?style=shield)](https://circleci.com/gh/astronautlabs/webrtc)

Node.js bindings for `libwebrtc`, which implements [WebRTC M95](https://groups.google.com/g/discuss-webrtc/c/SfzpFc-dH-E/m/JHlMpLO1AAAJ). This project aims for spec-compliance and is tested using the W3C's [web-platform-tests](https://github.com/web-platform-tests/wpt) project. A number of [nonstandard APIs](docs/nonstandard-apis.md) for testing are also included.

# Install

```
npm install @astronautlabs/webrtc
```

Installing from NPM downloads a prebuilt binary for your operating system × architecture. Set the `TARGET_ARCH` environment variable to "arm" or "arm64" to download for armv7l or arm64, respectively. Linux and macOS users can also set the `DEBUG` environment variable to download debug builds.

You can also [build from source](docs/build-from-source.md).

# Supported Platforms

We intend to officially support
- the latest 3 stable versions of Node.js 
- the latest 3 stable releases of Electron 

For the following platforms:
- Linux
- macOS
- Windows

On the following architectures:
- x64
- arm64
- armv7l 

Build validation is not yet in place for all of these platforms. 

The following platforms are confirmed to work with `@astronautlabs/webrtc`. Some may have prebuilt binaries available. Since we target [N-API version 3](https://nodejs.org/api/n-api.html), there may be additional platforms supported that are not listed here. If your platform is not supported, you may still be able to [build from source](docs/build-from-source.md).

The table below maps our support intentions to which configurations have been validated.
<table>
  <thead>
    <tr>
      <td style="text-align: center;" colspan="2" rowspan="2"></td>
      <th style="text-align: center;" colspan="3">Linux</th>
      <th style="text-align: center;">macOS</th>
      <th style="text-align: center;">Windows</th>
    </tr>
    <tr>
      <th style="text-align: center;">armv7l</th>
      <th style="text-align: center;">arm64</th>
      <th style="text-align: center;">x64</th>
      <th style="text-align: center;">x64</th>
      <th style="text-align: center;">x64</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th rowspan="3">Node.js</th>
      <th>14</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center">✓</td>
    </tr>
    <tr>
      <th>16</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center">✓</td>
    </tr>
    <tr>
      <th>18</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center">✓</td>
    </tr>
    <tr>
      <th rowspan="3">Electron</th>
      <th>18</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
    </tr>
    <tr>
      <th>19</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
    </tr>
    <tr>
      <th>20</th>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
      <td align="center"></td>
    </tr>
  </tbody>
</table>

# Troubleshooting

## Node.js `>= 26.0.0 && < 26.3.0` on Windows

Node.js has started building with ClangCL, which triggers the bundled node-gyp to try to build normal `.gyp` addons like 
`libyuv` (which is a dependency of `@/webrtc`) using ClangCL, but simultaneously passes MSVC specific arguments such as 
`/ltcg:incremental` which causes the `libyuv` build to fail on `npm install`. 

Solution: Either upgrade Node.js to 26.3 or later, or install a `node-gyp@10.3` or later. 

It is not in our interest to produce a workaround on our end for this short series of Node.js builds which are not in 
the LTS branch, especially when there are easy workarounds by upgrading Node.js or `node-gyp`.

# Developers

This section has notes about how to be productive working with this codebase including building it from source.

## Build Tools & Prerequisites

Linux:
- `python` 3.10.12+
- `llvm`/`clang` 22+  
  If you use LLVM's Apt repository, ensure you have unprefixed versions of at least `clang`, `clang++` and `llvm-ar`:
  ```bash
  sudo ln -s `which clang-22` /usr/local/bin/clang
  sudo ln -s `which clang++-22` /usr/local/bin/clang++
  sudo ln -s `which llvm-ar-22` /usr/local/bin/llvm-ar
  ```
- `cmake` 3.22.1+
- `ninja` 1.10.1+
- `libnspr4`/`libnss3` (if you want to run the browser tests)

Windows:
- Python3
- Make sure long path support is enabled
    - Set `HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnable` to `1`
    - `git config --global core.longpaths true`
    - `git config core.longpaths true`
- Install VS 2026 [Community edition is fine]
- Install Windows 10 or 11 SDK
- Install LLVM 22 or newer (from https://llvm.org/)
- Install cmake 4.4 or later


## Build Commands

- `npm run build:tsc` - Compiles the Typescript sources into Javascript
- `npm run build:native` - Compiles libwebrtc and the wrtc.node Node.js addon in release mode
- `npm run build:dev` - Compiles libwebrtc/wrtc.node in debug mode and outputs compile_commands.json for clangd support.
  This is the one you want to use if you are working on the project, as opposed to building it for use in an application.

## IDEs

### Visual Studio Code + clangd

This project will generate `build/compile_commands.json` when you use a developer build (`npm run build:dev`). The 
`clangd` VS Code extension will pick this up out of the box, providing excellent intellisense for the project.

### CLion

This should work out of the box, except you need to make sure your node binary is available to CLion, which might not 
be the case if you are using NVM and have added its shell integration in .bashrc or .bash_profile instead of .profile. 
Also CLion has an option (default enabled in newer versions) to use the shell when executing commands, you would need 
that turned on. None of this applies if your Node is installed at the system level.

## Platforms

Make sure to check the platform-specific sections below for important information.

## Platform-specific details

### Windows

Initial build

```powershell
$env:SKIP_DOWNLOAD = 'true'   # Important to skip fetching a prebuilt version from CDN
$env:DEBUG = '1'
$env:PARALLELISM = '24'       # set to number of logical cores

# Initial install will build libwebrtc
# Get a coffee.

npm install
```

## Debugging CI Core Dumps

In the event of test failure, the `test` job will collect any core dumps and deposit them as CircleCI Artifacts. You can 
then download the core dumps and debug them using `gdb`.

You can download the Debug binary from the `build` job that spawned the corresponding `test` job. Place it in your 
workspace as `build/Debug/wrtc.node`.

You'll also need to be running the exact same version of Node.js in order for the symbols to line up. 

```bash
nvm install <node-version-that-crashed>
nvm use <node-version-that-crashed>
```

To load a dump and get its backtrace:
```bash
$ gdb $(which node) core-dump-filename
(gdb) set solib-search-path build/Debug/
(gdb) bt
```

If you have the wrong Node.js version (or if the build of that version is incompatible with the one from CI), or if you
forget to set the solib-search-path to look in build/Debug, all but the Node.js symbols will be unknown (`??` in GDB).

# Required Reading

References
- [RTP: A Transport Protocol for Real-Time Applications](https://datatracker.ietf.org/doc/html/rfc3550) (IETF RFC 3550)
- [Unified Plan](https://datatracker.ietf.org/doc/html/draft-roach-mmusic-unified-plan-00#section-2)
- [Plan B](https://datatracker.ietf.org/doc/html/draft-uberti-rtcweb-plan-00)
- [RTCRtpTransceiver](https://developer.mozilla.org/en-US/docs/Web/API/RTCRtpTransceiver) (MDN)
- [Negotiating Media Multiplexing Using the Session Description Protocol](https://datatracker.ietf.org/doc/html/draft-ietf-mmusic-sdp-bundle-negotiation-54#section-18) ("BUNDLE" IETF I-D)

Editorial
- [The evolution of WebRTC 1.0.](https://blog.mozilla.org/webrtc/the-evolution-of-webrtc/) (Mozilla)
- [Exploring RTCRtpTransceiver.](https://blog.mozilla.org/webrtc/rtcrtptransceiver-explored/) (Mozilla)
- [Webrtc with transceivers](https://niccoloterreri.com/webrtc-with-transceivers) (nterreri)
