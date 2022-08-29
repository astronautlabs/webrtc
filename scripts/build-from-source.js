#!/usr/bin/env node
/* eslint no-console:0, no-process-env:0 */
'use strict';

const { spawnSync } = require('child_process');

const args = ['configure'];
const DEBUG = !!process.env.DEBUG;

if (DEBUG) {
  console.log(`** Enable debug configuration`);
  args.push('--debug');
}

if (process.platform === 'win32') {
  args.push('-g');
  args.push('"Visual Studio 16 2019"'); // Important: libwebrtc determines the version that will work.
}

function main() {
  console.log('Running ncmake ' + args.join(' '));
  let { status } = spawnSync('ncmake', args, {
    shell: true,
    stdio: 'inherit'
  });
  if (status) {
    throw new Error('ncmake configure failed for wrtc');
  }

  let buildArgs = ['build'];

  if (DEBUG) {
    console.log(`** Enabling debug build`);
    buildArgs.push('--debug');
  }
  
  console.log(`Running ncmake build ${buildArgs.join(' ')}`);
  status = spawnSync('ncmake', buildArgs, {
    shell: true,
    stdio: 'inherit'
  }).status;
  if (status) {
    throw new Error('ncmake build failed for wrtc');
  }

  console.log('Built wrtc');
}

module.exports = main;

if (require.main === module) {
  main();
}
