const { longWork } = require('./long-work');
const { registerThread } = require('@sentry-internal/node-native-stacktrace');

setInterval(() => {
  registerThread();
}, 200);

