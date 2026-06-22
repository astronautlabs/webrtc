#!/usr/bin/env node
/* eslint no-console:0, no-process-env:0 */
"use strict";

const os = require("os");
const path = require("path");
const { spawnSync } = require("child_process");
const { platform, arch, buildFolder } = require("./build-vars.js");

const args = ["-O", buildFolder, "-a", arch];

// if (!['win32', 'linux'].includes(os.platform())) {
//     console.error(`Currently only Win32 and Linux are supported. If you need other platforms, you must provide a ninja executable`);
//     console.error(`under scripts/ninja/${os.platform()}`);
//     process.exit(1);
// }

process.env.PATH = [
    ...(process.env.PATH ? process.env.PATH.split(path.delimiter) : []),
    path.resolve(__dirname, 'ninja', os.platform())
].join(path.delimiter)

//process.env.RC = "C:/Program Files/LLVM/bin/llvm-rc.exe";

if (process.env.DEBUG) {
  args.push(...["--debug", "--CDCMAKE_EXPORT_COMPILE_COMMANDS=1"]);
}

if (platform === "win32") {
  //args.push(...["-G", "Ninja"]);
  args.push(...['--toolset=ClangCL']);
}

if (arch !== os.arch()) {
  args.push(
    `--CDCMAKE_TOOLCHAIN_FILE=toolchains/${platform}-${arch}.toolchain`,
  );
}

function main() {
  // Resolve cmake-js path before modifying PATH, since it lives in node_modules/.bin
  const cmakeJs = path.resolve(
    __dirname,
    "..",
    "node_modules",
    ".bin",
    "cmake-js",
  );

  if (platform === "win32") {
    // Explicitly find the real rc.exe from the Windows SDK and pass it to
    // CMake, since cmake-js may still find the npm "rc" package otherwise.
    const { stdout } = spawnSync("where", ["rc.exe"], { encoding: "utf-8" });
    if (stdout) {
      args.push(`--CDCMAKE_RC_COMPILER="${stdout.replace(/\\/g, "/")}"`);
    }
  }

  console.log("Running cmake-js " + args.join(" "));
  let { status } = spawnSync(cmakeJs, ["configure", ...args], {
    shell: true,
    stdio: "inherit",
  });
  if (status) {
    throw new Error("cmake-js configure failed for wrtc");
  }

  console.log("Running cmake-js build");
  status = spawnSync(cmakeJs, ["build", ...args], {
    shell: true,
    stdio: "inherit",
  }).status;
  if (status) {
    throw new Error("cmake-js build failed for wrtc");
  }

  console.log("Built wrtc");
}

module.exports = main;

if (require.main === module) {
  main();
}