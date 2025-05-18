
export declare function setMainIsolate(): void;

export type StackFrame = {
    function: string;
    filename: string;
    lineno: number;
    colno: number;
};

export declare function captureStackTrace(): StackFrame[];