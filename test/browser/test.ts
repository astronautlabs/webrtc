import { fork } from 'child_process';
import { spawn } from 'child_process';
import { startServer } from './server';
import path from 'path';

const TEST_DIR = path.join(__dirname);
const CLIENT_FILE = path.join(TEST_DIR, 'client.ts');
const WTR = path.join(__dirname, '..', 'node_modules', '.bin', 'web-test-runner');

const server = startServer();

const client = spawn("node", [WTR, CLIENT_FILE], { stdio: 'inherit' })
    .once('error', error => (exitWithError(`Failed to start WTR: ${error.stack}`)))
    .once('exit', code => code ? exitWithError(`WTR error (status ${code})`, code) : exit(0))
;

console.log(`Client PID: ${client.pid}`);

process.once('exit', exit);
process.once('SIGINT', exit);
process.once('SIGUSR1', exit);
process.once('SIGUSR2', exit);
process.once('uncaughtException', exit);

function exitWithError(message: string, code = 1) {
    console.error(message);
    exit(code);
}

function exit(code: number) {
    client?.kill();
    process.exit(code);
}
