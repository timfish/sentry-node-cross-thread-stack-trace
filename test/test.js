const { Worker } = require('node:worker_threads');
const crypto = require('node:crypto');

const { registerThread } = require('..');

registerThread();

const watchdog = new Worker('./test/watchdog.js');
watchdog.on('message', (message) => console.log(message));

const worker = new Worker('./test/worker.js');

function longWork() {
    for (let i = 0; i < 100; i++) {
        const salt = crypto.randomBytes(128).toString('base64');
        const hash = crypto.pbkdf2Sync('myPassword', salt, 10000, 512, 'sha512');
    }
}

longWork();