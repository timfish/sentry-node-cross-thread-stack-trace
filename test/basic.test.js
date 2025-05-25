import { describe, test, expect } from 'vitest';
import { spawnSync } from 'node:child_process';
import { join } from 'node:path';

const __dirname = import.meta.dirname || new URL('.', import.meta.url).pathname;

describe("Basic", () => {
    const testFile = join(__dirname, 'test.js');
    test("Should capture stack trace from multiple threads", () => {
        const result = spawnSync('node', [testFile])
        expect(result.status).toBe(0);

        const stacks = JSON.parse(result.stdout.toString());

        expect(stacks.main).toEqual(expect.arrayContaining([
            {
                function: 'pbkdf2Sync',
                filename: expect.any(String),
                lineno: expect.any(Number),
                colno: expect.any(Number),
            },
            {
                function: 'longWork',
                filename: expect.stringMatching(/test.js$/),
                lineno: 18,
                colno: 29
            },
            {
                function: '?',
                filename: expect.stringMatching(/test.js$/),
                lineno: 22,
                colno: 1
            },
        ]));

        expect(stacks['worker-2']).toEqual(expect.arrayContaining([
            {
                function: 'pbkdf2Sync',
                filename: expect.any(String),
                lineno: expect.any(Number),
                colno: expect.any(Number),
            },
            {
                function: 'longWork',
                filename: expect.stringMatching(/worker.js$/),
                lineno: 10,
                colno: 29
            },
            {
                function: '?',
                filename: expect.stringMatching(/worker.js$/),
                lineno: 14,
                colno: 1
            },
        ]));
    });
});