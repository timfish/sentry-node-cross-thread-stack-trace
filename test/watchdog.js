const { captureStackTrace } = require('@sentry-internal/node-native-stacktrace');

setTimeout(() => {
    console.log(JSON.stringify(captureStackTrace()));
}, 2000);

