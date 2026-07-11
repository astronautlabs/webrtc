import { fork } from 'child_process';
import { spawn } from 'child_process';
import { startServer } from './server';
import path from 'path';

const TEST_DIR = path.join(__dirname);
const CLIENT_FILE = path.join(TEST_DIR, 'client.ts');
const WTR = path.join(__dirname, '..', '..', 'node_modules', '.bin', 'web-test-runner');

const server = startServer();

const client = spawn("node", [WTR, CLIENT_FILE, '--puppeteer', '--debug'], { stdio: 'inherit' })
    .once('error', error => (exitWithError(`Failed to start WTR: ${error.stack}`)))
    .once('exit', code => exit(`WTR`, code))
;

process.once('exit', exit);
process.once('SIGINT', exit);
process.once('SIGUSR1', exit);
process.once('SIGUSR2', exit);
process.once('uncaughtException', exit);

function exitWithError(message: string, code?: number | null) {
    code ??= 1;
    console.error(message);
    exit(`test`, code);
}

function exit(label: string, code?: number | null) {
    code ??= 0;
    if (code)
        console.error(`Error: ${label}: Exited with code ${code}`);
    client?.kill();
    process.exit(code);
}
