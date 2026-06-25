# Build from Source

## Prerequisites

`@/webrtc` uses [cmake-js](https://github.com/cmake-js/cmake-js) to build from source. 

In addition to the prerequisites for cmake-js, you will need:

* Git
* CMake 3.12 or newer
* Clang 23 or newer
* Microsoft Visual Studio 2022 or newer (Windows)
    * Install the Clang toolkit and MSBuild Clang support from Individual Components

## Build

You can build a debug binary (for development) using `npm run build:dev`. 
If you want to build a release binary, use `npm run build:native`.
You can build the Typescript using `npm run build:tsc`.

```
git clone https://github.com/astronautlabs/webrtc.git
cd webrtc
npm run build:dev
npm run build:tsc
```

## Subsequent Builds

Subsequent builds can be triggered with `npm run build:dev` or `npm run build:native` depending on the binary you are 
building.

## Compiler

WebRTC itself is built with a clang that it compiles from scratch. However the wrtc.node binary is built using the 
system-installed Clang. It will use Clang even if the `cc`/`c++` binaries in the path point to a different compiler.

That being said, if you have set `CC`/`CXX` environment variables to pick your own compiler, that will be respected, 
but your mileage may vary using anything other than Clang.

## Other Notes

#### armv7l

In order to cross-compile for armv7l on Linux,

1. Set `TARGET_ARCH` to "arm".
2. Install the appropriate toolchain, and set `ARM_TOOLS_PATH`.
3. On Ubuntu, you may also need g++-arm-linux-gnueabihf.

```
wget https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/arm-linux-gnueabihf/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz
tar xf gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz
SKIP_DOWNLOAD=true TARGET_ARCH=arm ARM_TOOLS_PATH=$(pwd)/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf npm install
```

#### arm64

In order to cross-compile for arm64 on Linux,

1. Set `TARGET_ARCH` to "arm64".
2. Install the appropriate toolchain, and set `ARM_TOOLS_PATH`.
3. On Ubuntu, you may also need g++-aarch64-linux-gnu.

```
wget https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
SKIP_DOWNLOAD=true TARGET_ARCH=arm64 ARM_TOOLS_PATH=$(pwd)/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu npm install
```

### Windows

On Windows, we do not compile WebRTC sources with Clang. This is disabled by
passing `is_clang=false` to `gn gen`.

To fix error `Filename too long`, use (optionally with `--global` or `--system` switches to set for more than just this project):

```
git config core.longpaths true
```

Creating symbolic links with MKLINK is used by the build script but is disabled for non-Administrative users by default with a local security policy. On Windows 10, fix this with Run (Windows-R) then `gpedit.msc`. Edit key "Local Computer Policy -> Windows Settings -> Security Settings -> Local Policies -> User Rights Assignment -> Create Symbolic Links" and add your user name. Log out and in to change the policy. Note the [associated security vunerability](https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/create-symbolic-links#vulnerability).

The Windows SDK debugging tools should be installed. One way to achieve this is to [Download the Windows Driver Kit](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).

# Test

## Unit & Integration Tests

Once everything is built, run

```
npm test
```

## Web Platform Tests

[web-platform-tests/wpt](https://github.com/web-platform-tests/wpt) defines a suite of WebRTC tests. `@/webrtc`borrows 
a technique from [jsdom/jsdom](https://github.com/jsdom/jsdom) to run these tests in Node.js. 

Run the tests with:
```
npm run wpt:test
```

## Browser Tests

These tests are run by CircleCI to ensure `@/webrtc` remains compatible with the latest versions of Chrome and 
Firefox.

```
npm run test:browsers
```

## Electron Test

```
npm run test:electron
```
