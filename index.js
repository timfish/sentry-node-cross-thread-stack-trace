const { threadId } = require('node:worker_threads');
const native = require('./build/release/cross-thread-stack-trace.node');

exports.registerThread = function (name = String(threadId)) {
    native.registerThread(name);
};
exports.captureStackTrace = function () {
    return native.captureStackTrace();
};
exports.getThreadLastSeen = function () {
    return native.getThreadLastSeen();
};