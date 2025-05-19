const { isMainThread, threadId } = require('node:worker_threads');
const native = require('./build/release/cross-thread-stack-trace.node');

exports.registerThread = function () {
    native.registerThread(isMainThread ? -1 : threadId );
};
exports.captureStackTrace = function (excludeWorkers) {
    return JSON.parse(native.captureStackTrace(excludeWorkers));
};