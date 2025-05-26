
export declare function registerThread(name?: string): void;

export type StackFrame = {
    function: string;
    filename: string;
    lineno: number;
    colno: number;
};

export type Trace = Record<string, StackFrame[]>;

export declare function captureStackTrace(excludeWorkers: boolean): Trace;

export declare function getThreadLastSeen(): Record<string, number>;