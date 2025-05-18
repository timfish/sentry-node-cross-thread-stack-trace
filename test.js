const { Worker } = require('node:worker_threads');
const crypto = require('node:crypto');

const { setMainIsolate } = require('.');

setMainIsolate();

const worker = new Worker('./worker.js');
worker.on('exit', (code) => console.log(`Worker stopped with exit code ${code}`));
worker.on('error', (error) => console.error('Worker error:', error));

function longWork() {
    for (let i = 0; i < 100; i++) {
        const salt = crypto.randomBytes(128).toString('base64');
        const hash = crypto.pbkdf2Sync('myPassword', salt, 10000, 512, 'sha512');
    }
}

longWork();