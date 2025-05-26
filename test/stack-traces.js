const { Worker } = require('node:worker_threads');
const { longWork } = require('./long-work.js');
const { registerThread } = require('../lib/index.js');

registerThread();

const watchdog = new Worker('./test/watchdog.js');

const worker = new Worker('./test/worker.js');

longWork();
