
export declare function registerThread(): void;

export type StackFrame = {
    function: string;
    filename: string;
    lineno: number;
    colno: number;
};

export type Trace = {
    main: StackFrame[];
} & Record<string, StackFrame[]>;

export declare function captureStackTrace(excludeWorkers: boolean): Trace;