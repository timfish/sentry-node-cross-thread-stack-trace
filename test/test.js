const { Worker } = require('node:worker_threads');
const crypto = require('node:crypto');

const { registerThread } = require('../index.js');

registerThread(true);

const watchdog = new Worker('./test/watchdog.js');
watchdog.on('exit', (code) => console.log(`watchdog stopped with exit code ${code}`));
watchdog.on('error', (error) => console.error('watchdog error:', error));


const worker = new Worker('./test/worker.js');
worker.on('exit', (code) => console.log(`Worker stopped with exit code ${code}`));
worker.on('error', (error) => console.error('Worker error:', error));

function longWork() {
    for (let i = 0; i < 100; i++) {
        const salt = crypto.randomBytes(128).toString('base64');
        const hash = crypto.pbkdf2Sync('myPassword', salt, 10000, 512, 'sha512');
    }
}

longWork();