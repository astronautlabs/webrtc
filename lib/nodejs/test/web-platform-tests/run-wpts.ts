import 'segfault-handler';
import path from 'path';
import fs from 'fs';
import jsYAML from 'js-yaml';
import { Minimatch } from 'minimatch';
import { readManifest, getPossibleTestFilePaths, stripPrefix } from './wpt-manifest-utils.js';
import startWPTServer from './start-wpt-server.js';
import { runSingleWPT } from './run-single-wpt';
import { ConsoleReporter, describe, it, suite, TestFunction } from 'razmin';

/**
 * If you wish to work on specific WPT tests, you can add filters for them here.
 * If any test description matches any of the fragments in this filter list, then 
 * only those tests will be run. If no filters are specified in this list, all tests
 * will run.
 */
const ONLY = [
  //`RTCPeerConnection-addTrack.https.html`
  //`RTCConfiguration-iceServers.html`
  //`RTCDataChannel-id.html`                  // flakey abort on linux due to weakptr destructor sequencing
];

// The WPT suite routinely ignores promise rejections.
process.on("unhandledRejection", () => {});

const validReasons = new Set([
  'fail',
  'fail-slow',
  'timeout',
  'flaky',
  'mutates-globals',
  'timeout-success',
  'needs-node10',
  'needs-node11'
]);

const hasNode10 = Number(process.versions.node.split('.')[0]) >= 10;
const hasNode11 = Number(process.versions.node.split('.')[0]) >= 11;

const manifestFilename = path.resolve(__dirname, '..', '..', '..', '..', 'lib', 'nodejs', 'test', 'web-platform-tests', 'wpt-manifest.json');
const manifest = readManifest(manifestFilename);
const possibleTestFilePaths = getPossibleTestFilePaths(manifest);

const toRunFilename = path.resolve(__dirname, '..', '..', '..', '..', 'lib', 'nodejs', 'test', 'web-platform-tests', 'to-run.yaml');
const toRunString = fs.readFileSync(toRunFilename, { encoding: 'utf-8' });
const toRunDocs = jsYAML.safeLoadAll(toRunString, { filename: toRunFilename });

const minimatchers = new Map();

checkToRun();

let wptServerURL;
//const runSingleWPT = wptRunner(() => wptServerURL);

let serverPromise;

async function startServer() {
  if (!serverPromise) {
    serverPromise = startWPTServer({ toUpstream: false }).then(url => {
      wptServerURL = url;
    });
  }

  return serverPromise;
}

if (process.argv.includes('--native-debug')) {
  console.log(`Attach native debugger now.`);
  console.log(`Starting in 20 seconds...`);
  setTimeout(() => defineSuite(), 20*1000);
} else {
  defineSuite();
}

function defineSuite() {
  suite(() => {
    describe('web-platform-tests', () => {
      for (const toRunDoc of toRunDocs) {
        describe(`: ${toRunDoc.DIR}`, it => {
          for (const testFilePath of possibleTestFilePaths) {
            if (testFilePath.startsWith(toRunDoc.DIR + '/')) {
              const matchingPattern = expectationsInDoc(toRunDoc).find(pattern => {
                const matcher = minimatchers.get(toRunDoc.DIR + '/' + pattern);
                return matcher.match(testFilePath);
              });

              const testFile = stripPrefix(testFilePath, toRunDoc.DIR + '/');
              const reason = matchingPattern && toRunDoc[matchingPattern][0];
              const shouldSkip = ['fail-slow', 'timeout', 'flaky', 'mutates-globals'].includes(reason);
              const expectFail = (reason === 'fail') ||
                                (reason === 'needs-node10' && !hasNode10) ||
                                (reason === 'needs-node11' && !hasNode11);
              const allowTimeoutSuccess = (reason === 'timeout-success');
              let title = `${testFile}`;
              if (expectFail)
                title = `[expected fail] ${title}`;
              
              let runTest: (testDescription: string, func: TestFunction) => void = it;
              
              if (matchingPattern && shouldSkip) {
                runTest = it.skip;
                title = `[${reason}] ${title}`;
              }

              if (ONLY.length > 0 && ONLY.some(fragment => title.includes(fragment))) {
                runTest = it.only;
              }

              title = `: ${title}`;

              runTest(title, async () => {
                await startServer();
                await runSingleWPT(
                  wptServerURL, testFilePath, expectFail,
                  allowTimeoutSuccess
                );
              });
            }
          }
        });
      }
    });
  }, { 
    execution: {
      order: 'default',
      timeout: 120000, // WPT times out at 60s, our own WPT harness times out at 75s
      verbose: true
    },
    reporting: {
      slowThreshold: 10000
    }
  });
}

function checkToRun() {
  let lastDir = '';
  for (const doc of toRunDocs) {
    if (doc.DIR.startsWith('/')) {
      throw new Error(`DIR entries must not start with a slash: saw "${doc.DIR}"`);
    }
    if (doc.DIR.endsWith('/')) {
      throw new Error(`DIR entries must not end with a slash: saw "${doc.DIR}"`);
    }

    if (!fs.existsSync(path.resolve(__dirname, '..', '..', '..', '..', 'web-platform-tests', doc.DIR))) {
      throw new Error(`The directory "${doc.DIR}" does not exist`);
    }

    if (doc.DIR < lastDir) {
      throw new Error(`Bad lexicographical directory sorting in to-run.yaml: ${doc.DIR} should come before ${lastDir}`);
    }
    lastDir = doc.DIR;

    let lastPattern = '';
    for (const pattern of expectationsInDoc(doc)) {
      if (pattern.startsWith('/')) {
        throw new Error(`Expectation patterns must not start with a slash: saw "${pattern}"`);
      }

      if (pattern < lastPattern) {
        throw new Error('Bad lexicographical expectation pattern sorting in to-run.yaml: ' + pattern +
                        ' should come before ' + lastPattern);
      }
      lastPattern = pattern;

      const reason = doc[pattern][0];
      if (!validReasons.has(reason)) {
        throw new Error(`Bad reason "${reason}" for expectation ${pattern}`);
      }

      const matcher = new Minimatch(doc.DIR + '/' + pattern);
      if (!possibleTestFilePaths.some(filename => matcher.match(filename))) {
        throw new Error(`Expectation pattern "${pattern}" does not match any test files`);
      }
      minimatchers.set(doc.DIR + '/' + pattern, matcher);
    }
  }
}

function expectationsInDoc(doc) {
  const keys = Object.keys(doc);
  keys.shift(); // get rid of the DIR key
  return keys;
}
