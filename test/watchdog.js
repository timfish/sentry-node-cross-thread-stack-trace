const { captureStackTrace } = require('..');

setTimeout(() => {
    console.log(JSON.stringify(captureStackTrace()));
}, 2000);

