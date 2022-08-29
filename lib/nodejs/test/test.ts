import 'segfault-handler';
import { describe, suite } from "razmin";

console.log(`****************`);
console.log(`@/webrtc Test Suite`);
console.log(`PID: ${process.pid}`);

let waitTime = 0;

if (process.argv.includes('--native-debug')) {
    console.log(`Attach native debugger now.`);
    console.log(`Starting in 20 seconds...`);
    waitTime = 20*1000;
}

setTimeout(() => {
    suite()
        .include([
            '**/*.test.js',
            //'**/rollback.test.js',
            //'**/latency.test.js',
            //'**/r*.test.js',
            //'**/connect.test.js'
        ])
        .withOptions({
            execution: {
                order: 'default',
                timeout: 10*1000,
                verbose: process.argv.includes('--verbose')
            }
        })
        .run()
    ;
}, waitTime);


// TODO(mroberts): async_hooks were introduced in Node 9. We use them to test
// that destructors fire at the appropriate time (and hence, no memory leaks
// occur). Once we drop support for Node < 9, remove this.
// TODO: this need to be fixed
// if (semver(process.version).major >= 9 && typeof gc === 'function') {
//   require('./destructor');
// }