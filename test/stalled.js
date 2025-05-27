const { Worker } = require('node:worker_threads');
const { longWork } = require('./long-work.js');
const { registerThread } = require('@sentry-internal/node-native-stacktrace');

setInterval(() => {
  registerThread();
}, 200).unref();

const watchdog = new Worker('./test/stalled-watchdog.js');
watchdog.on('exit', () => process.exit(0));

const worker = new Worker('./test/worker-do-nothing.js');

setTimeout(() => {
  longWork();
}, 2000);

