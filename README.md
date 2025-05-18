# `cross-thread-stack-trace`

Native Node module to capture stack traces across threads. This allows capturing
main thread stack traces from a worker thread, even if the main thread event
loop is blocked.

Main thread:

```ts
const { setMainIsolate } = require("cross-thread-stack-trace");

setMainIsolate();
```

Worker thread:

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
[
    {
        function: "from",
        filename: "node:buffer",
        lineno: 298,
        colno: 28,
    },
    {
        function: "pbkdf2Sync",
        filename: "node:internal/crypto/pbkdf2",
        lineno: 78,
        colno: 17,
    },
    {
        function: "longWork",
        filename:
            "/Users/tim/Documents/Repositories/cross-thread-stack-trace/test.js",
        lineno: 15,
        colno: 29,
    },
    {
        function: "?",
        filename:
            "/Users/tim/Documents/Repositories/cross-thread-stack-trace/test.js",
        lineno: 19,
        colno: 1,
    },
    {
        function: "?",
        filename: "node:internal/modules/cjs/loader",
        lineno: 1730,
        colno: 14,
    },
    {
        function: "?",
        filename: "node:internal/modules/cjs/loader",
        lineno: 1895,
        colno: 10,
    },
    {
        function: "?",
        filename: "node:internal/modules/cjs/loader",
        lineno: 1465,
        colno: 32,
    },
    {
        function: "?",
        filename: "node:internal/modules/cjs/loader",
        lineno: 1282,
        colno: 12,
    },
    {
        function: "traceSync",
        filename: "node:diagnostics_channel",
        lineno: 322,
        colno: 14,
    },
    {
        function: "wrapModuleLoad",
        filename: "node:internal/modules/cjs/loader",
        lineno: 235,
        colno: 24,
    },
    {
        function: "executeUserEntryPoint",
        filename: "node:internal/modules/run_main",
        lineno: 170,
        colno: 5,
    },
    {
        function: "?",
        filename: "node:internal/main/run_main_module",
        lineno: 36,
        colno: 49,
    },
];
```
