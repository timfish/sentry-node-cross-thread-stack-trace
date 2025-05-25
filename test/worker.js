const crypto = require('node:crypto');

const { registerThread } = require('..');

registerThread();

function longWork() {
    for (let i = 0; i < 100; i++) {
        const salt = crypto.randomBytes(128).toString('base64');
        const hash = crypto.pbkdf2Sync('myPassword', salt, 10000, 512, 'sha512');
    }
}

longWork();