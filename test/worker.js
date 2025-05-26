const { longWork } = require('./long-work');
const { registerThread } = require('..');

registerThread();

longWork();
