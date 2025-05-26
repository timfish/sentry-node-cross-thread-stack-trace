import { spawnSync } from 'node:child_process';
import { join } from 'node:path';
import { describe, expect, test } from 'vitest';

const __dirname = import.meta.dirname || new URL('.', import.meta.url).pathname;

describe('e2e Tests', () => {
  test('Capture stack trace from multiple threads', () => {
    const testFile = join(__dirname, 'stack-traces.js');
    const result = spawnSync('node', [testFile])

    expect(result.status).toBe(0);

    const stacks = JSON.parse(result.stdout.toString());

    expect(stacks['0']).toEqual(expect.arrayContaining([
      {
        function: 'pbkdf2Sync',
        filename: expect.any(String),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: 'longWork',
        filename: expect.stringMatching(/long-work.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: '?',
        filename: expect.stringMatching(/stack-traces.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
    ]));

    expect(stacks['2']).toEqual(expect.arrayContaining([
      {
        function: 'pbkdf2Sync',
        filename: expect.any(String),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: 'longWork',
        filename: expect.stringMatching(/long-work.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: '?',
        filename: expect.stringMatching(/worker.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
    ]));
  });

  test('detect stalled thread', { timeout: 10000 }, () => {
    const testFile = join(__dirname, 'stalled.js');
    const result = spawnSync('node', [testFile]);

    expect(result.status).toBe(0);

    const stacks = JSON.parse(result.stdout.toString());

    expect(stacks['0']).toEqual(expect.arrayContaining([
      {
        function: 'pbkdf2Sync',
        filename: expect.any(String),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: 'longWork',
        filename: expect.stringMatching(/long-work.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
      {
        function: '?',
        filename: expect.stringMatching(/stalled.js$/),
        lineno: expect.any(Number),
        colno: expect.any(Number),
      },
    ]));

    expect(stacks['2'].length).toEqual(1);
  });
});
