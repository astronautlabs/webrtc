'use strict';
/* eslint-disable no-console, global-require */
import path from 'path';
import dns from 'dns';
import childProcess from 'child_process';
import q from 'q';
import { inBrowserContext } from './util.js';
import { head as requestHead } from 'request-promise-native';
const dnsLookup = q.denodeify(dns.lookup);
import which from 'which';

const wptDir = path.resolve(__dirname, 'tests');

const configPaths = {
  default: path.resolve(__dirname, '..', '..', '..', '..', 'lib', 'nodejs', 'test', 'web-platform-tests', 'wpt-config.json'),
  toUpstream: path.resolve(__dirname, '..', '..', '..', '..', 'lib', 'nodejs', 'test', 'web-platform-tests', 'tuwpt-config.json')
};

const configs = {
  default: require(configPaths.default),
  toUpstream: require(configPaths.toUpstream)
};

export default async ({ toUpstream = false } = {}) => {
  if (inBrowserContext()) {
    return Promise.resolve();
  }

  const configType = toUpstream ? 'toUpstream' : 'default';
  const configPath = configPaths[configType];
  const config = configs[configType];

  const urlPrefix = `http://${config.browser_host}:${config.ports.http[0]}/`;

  try {
    await dnsLookup('web-platform.test');
  } catch (e) {
    throw new Error('Host entries not present for web platform tests. See ' +
      'https://github.com/w3c/web-platform-tests#running-the-tests');
  }

  const configArg = path.relative(path.resolve(wptDir), configPath);
  const args = ['./wpt.py', 'serve', '--config', configArg];

  let pythonPath = await which('python');

  console.log(`Starting WPT server...`);
  const python = childProcess.spawn(pythonPath, args, {
    cwd: wptDir,
    stdio: 'inherit'
  });

  return new Promise((resolve, reject) => {
    python.on('error', e => {
      console.error(`Error starting python server process: ${e.message}`);
      reject(new Error(`Error starting python server process: ${e.message}`));
    });

    resolve(pollForServer(urlPrefix));

    process.on('exit', () => {
      // Python doesn't register a default handler for SIGTERM and it doesn't run __exit__() methods of context
      // managers when it gets that signal. Using SIGINT avoids this problem.
      python.kill('SIGINT');
    });
  });
};

async function pollForServer(url) {
  try {
    await requestHead(url);
  } catch (err) {
    console.log(`WPT server at ${url} is not up yet (${err.message}); trying again`);
    await q.delay(500);
    return pollForServer(url);
  }

  console.log(`WPT server at ${url} is up!`);
  return url;
}
