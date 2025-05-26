const { longWork } = require('./long-work');
const { registerThread } = require('../lib/index.js');

setInterval(() => {
  registerThread();
}, 200);

