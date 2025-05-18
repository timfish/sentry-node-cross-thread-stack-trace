const { captureStackTrace } = require('.');

setTimeout(() => {
    console.time('captureStackTrace');
    const result = captureStackTrace();
    console.timeEnd('captureStackTrace');
    console.log(result);
}, 500);

