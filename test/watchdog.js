const { captureStackTrace } = require('..');
const { inspect } = require('util');

setTimeout(() => {
    console.time('captureStackTrace');
    const result = captureStackTrace();
    console.timeEnd('captureStackTrace');
    console.log(inspect(result, false, null, true));
}, 2000);

