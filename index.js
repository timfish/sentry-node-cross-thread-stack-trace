const native = require('./build/release/cross-thread-stack-trace.node');

exports.setMainIsolate = native.setMainIsolate;
exports.captureStackTrace = function () {
    return JSON.parse(native.captureStackTrace());
};