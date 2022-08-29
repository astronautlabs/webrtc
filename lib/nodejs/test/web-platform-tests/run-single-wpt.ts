/* eslint-disable no-console */
import path from 'path';
import { URL } from 'url';
import { specify } from 'mocha-sugar-free';
import { inBrowserContext } from './util';
import { JSDOM, VirtualConsole } from 'jsdom/lib/api.js';
import ResourceLoader from 'jsdom/lib/jsdom/browser/resources/resource-loader';
import * as wrtc from '../..';
import fetch from 'node-fetch';

const reporterPathname = '/resources/testharnessreport.js';

class CustomResourceLoader extends ResourceLoader {
  constructor() {
    super({ strictSSL: false });
  }

  fetch(urlString, options) {
    const url = new URL(urlString);

    if (url.pathname === reporterPathname) {
      return Promise.resolve(Buffer.from('window.shimTest();', 'utf-8'));
    } else if (url.pathname.startsWith('/resources/')) {
      // When running to-upstream tests, the server doesn't have a /resources/ directory.
      // So, always go to the one in ./tests.
      // The path replacement accounts for a rewrite performed by the WPT server:
      // https://github.com/w3c/web-platform-tests/blob/master/tools/serve/serve.py#L271
      
      const wptDir = path.resolve(__dirname, '..', '..', '..', '..');
      const filePath = path.resolve(wptDir, 'web-platform-tests' + url.pathname)
        .replace('/resources/WebIDLParser.js', '/resources/webidl2/lib/webidl2.js');

      return super.fetch(`file://${filePath}`, options);
    }

    return super.fetch(urlString, options);
  }
}

interface Result {
  status: number; // 0 is success
  name: string;
  message: string;
  stack: string;
}

export async function runSingleWPT(urlPrefix, testPath, expectFail, allowTimeoutSuccess = false) {
  const unhandledExceptions = [];
  let allowUnhandledExceptions = false;

  const virtualConsole = new VirtualConsole().sendTo(console, { omitJSDOMErrors: true });
  virtualConsole.on('jsdomError', e => {
    if (e.type === 'unhandled exception' && !allowUnhandledExceptions) {
      unhandledExceptions.push(e);

      // Some failing tests make a lot of noise.
      // There's no need to log these messages
      // for errors we're already aware of.
      //if (!expectFail) {
        console.error(`    (!!!) Uncaught exception: ${e.detail.message}:`);
        console.error(e.detail.stack);
      //}
    }
  });

  let dom = await JSDOM.fromURL(urlPrefix + testPath, {
    runScripts: 'dangerously',
    virtualConsole,
    resources: new CustomResourceLoader(),
    pretendToBeVisual: true,
    storageQuota: 100000 // Filling the default quota takes about a minute between two WPTs
  });
  const { window } = dom;

  // NOTE(mroberts): Here is where we inject @/webrtc.
  Object.assign(window, wrtc);
  window.TypeError = TypeError;

  window.navigator.mediaDevices = Object.assign({}, window.navigator.mediaDevices, {
    getUserMedia: wrtc.getUserMedia
  });

  window.fetch = function safeFetch() {
    const args = [].slice.call(arguments);
    const url = args[0];
    try {
      new window.URL(url);
    } catch (error) {
      args[0] = window.location.protocol + '//' + window.location.host + url;
    }
    return fetch.apply(null, args);
  };

  await new Promise<void>((resolve, reject) => {
    const results: Result[] = [];

    window.shimTest = () => {
      const oldSetup = window.setup;
      window.setup = options => {
        if (options.allow_uncaught_exception) {
          allowUnhandledExceptions = true;
        }
        oldSetup(options);
      };

      let completionCallback;
      let internalTimeout = setTimeout(() => {
        // It shouldn't be possible to hit this, but some tests are broken in a way that 
        // prevents the completion callback.
        console.log(`    (***) Test timed out without harness indicating timeout (bug in test)`);
        completionCallback([], { status: 2 });
      }, 70000);

      window.add_result_callback(test => {
        console.log(`    (***) ${summarizeResult(test)}`);
        results.push(test);
      });

      window.add_completion_callback(completionCallback = (_, harnessStatus) => {
        clearTimeout(internalTimeout);

        // This needs to be delayed since some tests do things even after calling done().
        process.nextTick(() => {
          window.close();
        });

        if (harnessStatus.status === 2) {
          console.log(`    (***) Test harness timed out`);
          results.push({
            name: `Boilerplate`,
            message: `Test harness should not timeout`,
            stack: ``,
            status: allowTimeoutSuccess ? 0 : 2
          });
        } else {
          console.log(`    (***) Test harness completed`);
        }

        for (let unhandledException of unhandledExceptions) {
          results.push({
            name: `Unhandled exception`,
            message: unhandledException.message,
            stack: unhandledException.stack,
            status: 1
          });
        }

        let errors = results.filter(x => x.status !== 0);

        if (errors.length === 0 && expectFail) {
          reject(new Error(`
            Hey, did you fix a bug? This test used to be failing, but during
            this run there were no errors. If you have fixed the issue covered
            by this test, you can edit the "to-run.yaml" file and remove the line
            containing this test. Thanks!
        `));
        } else if (errors.length > 0 && !expectFail) {
          reject(new Error(
            `Only ${results.length - errors.length}/${results.length} assertions succeeded:\n\n` 
            + `${results.map(r => `               ${summarizeResult(r)}`).join('\n')}`
          ));
        } else {
          resolve();
        }
      });
    };
  });
}

function summarizeResult(result: Result) {
  if (result.status === 0) {
    return `✅ ${result.name}`;
  } else if (result.status === 1) {
    return `❌ ${result.name}: ${result.message}`;
  } else if (result.status === 2) {
    return `⌛ ${result.name}: ${result.message}`;
  } else if (result.status === 3) {
    return `❔ ${result.name}: ${result.message}`;
  }
}