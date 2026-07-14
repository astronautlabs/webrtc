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

## Platform-specific details

### Linux

Compiling this addon requires Clang 23+. Ubuntu is often behind on the Clang version it ships. You can install the 
newest Clang directly from the LLVM Project by following the instructions at https://apt.llvm.org/.

At the time of writing, Clang 22 is installed when using LLVM's automatic apt installation script. To ensure you get 
Clang 23, use:

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 23
```

You also need to install `libnspr4` and `libnss3` for the browser tests. 

```bash
sudo apt install libnspr4 libnss3
```

# Developers

This section has notes about how to be productive working with this codebase.

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

## Linux/Mac

Pre-steps
- Install `python3`, C/C++ toolchain (ie `build-essential`), `cmake`

```shell
export SKIP_DOWNLOAD=true   # Important to skip fetching a prebuilt version from CDN
export DEBUG=1
export PARALLELISM=24       # Set to number of logical cores on your machine

# Initial install will build libwebrtc
# Get a coffee.

npm install
```

## Windows

Pre-steps:
- Install python3
- Make sure long path support is enabled
    - Set `HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnable` to `1`
    - `git config --global core.longpaths true`
    - `git config core.longpaths true`
- Install VS 2026 [Community edition is fine]
    - Include the Clang toolset (version 22)
    - NOTE: `libwebrtc` is the limiting factor here, it builds with its own Clang, we need a new enough Clang to build 
      the addon sources with so that we can link libwebrtc to the addon objects, and VS 2022's Clang 19 is not new enough.
- Install cmake 4.4 or later
- Install Windows 10 SDK v10.0.19041.0
    * Use the Windows SDK installer, make sure to include the required Debugging Tools for Windows
    * (!!) Do not use the Visual Studio installer, if you have previously installed this SDK via Visual Studio Installer, 
      you must first remove it and install using the Windows SDK installer instead. If you use this, the build will fail
      on requirement of Windows SDK 10.0.19041.0

Initial build

```powershell
$env:SKIP_DOWNLOAD = 'true'   # Important to skip fetching a prebuilt version from CDN
$env:DEBUG = '1'
$env:PARALLELISM = '24'       # set to number of logical cores

# Initial install will build libwebrtc
# Get a coffee.

npm install
```

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
