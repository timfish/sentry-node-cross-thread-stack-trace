# `cross-thread-stack-trace`

Native Node module to capture stack traces from all registered threads.

This allows capturing main and worker thread stack traces from another watchdog
thread, even if the event loops are blocked.

In the main or worker threads:

```ts
const { registerThread } = require("cross-thread-stack-trace");

registerThread();
```

Watchdog thread:

```ts
const { captureStackTrace } = require("cross-thread-stack-trace");

const stack = captureStackTrace();
console.log(stack);
```

Build the module and run the test:

```
npm i && npm test
```

Results in:

```js
{
  main: [
    {
      function: 'from',
      filename: 'node:buffer',
      lineno: 298,
      colno: 28
    },
    {
      function: 'pbkdf2Sync',
      filename: 'node:internal/crypto/pbkdf2',
      lineno: 78,
      colno: 17
    },
    {
      function: 'longWork',
      filename: '/Users/tim/Documents/Repositories/cross-thread-stack-trace/test/test.js',
      lineno: 20,
      colno: 29
    },
    {
      function: '?',
      filename: '/Users/tim/Documents/Repositories/cross-thread-stack-trace/test/test.js',
      lineno: 24,
      colno: 1
    }
  ],
  'worker-2': [
    {
      function: 'from',
      filename: 'node:buffer',
      lineno: 298,
      colno: 28
    },
    {
      function: 'pbkdf2Sync',
      filename: 'node:internal/crypto/pbkdf2',
      lineno: 78,
      colno: 17
    },
    {
      function: 'longWork',
      filename: '/Users/tim/Documents/Repositories/cross-thread-stack-trace/test/worker.js',
      lineno: 10,
      colno: 29
    },
    {
      function: '?',
      filename: '/Users/tim/Documents/Repositories/cross-thread-stack-trace/test/worker.js',
      lineno: 14,
      colno: 1
    }
  ]
}
```
