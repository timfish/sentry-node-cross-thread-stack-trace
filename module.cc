#include <node.h>
#include <mutex>

using namespace v8;
using namespace node;

static v8::Isolate *main_thread_isolate;

static std::mutex interrupt_mutex;
static std::condition_variable interrupt_cv;
static bool interrupt_done = false;

static const int kMaxStackFrames = 255;
static const int kMaxStackJsonSize = 10240;

static void ExecutionInterrupted(Isolate *isolate, void *data)
{
    char *buffer = static_cast<char *>(data);

    v8::RegisterState state;
    v8::SampleInfo info;
    void *samples[kMaxStackFrames];

    uint32_t pos = 0;

    // Initialise the register state
    state.pc = nullptr;
    state.fp = &state;
    state.sp = &state;

    isolate->GetStackSample(state, samples, kMaxStackFrames, &info);

    Local<StackTrace> stack = StackTrace::CurrentStackTrace(isolate, 255, StackTrace::kDetailed);
    if (stack.IsEmpty())
    {
        snprintf(buffer, kMaxStackJsonSize, "[]");
        return;
    }

    pos += snprintf(&buffer[pos], kMaxStackJsonSize, "[");
    int count = stack->GetFrameCount();

    for (int i = 0; i < count; i++)
    {
        Local<StackFrame> frame = stack->GetFrame(isolate, i);
        Local<String> fn_name = frame->GetFunctionName();

        if (frame->IsEval())
        {
            fn_name = String::NewFromUtf8(isolate, "[eval]", NewStringType::kInternalized).ToLocalChecked();
        }
        else if (fn_name.IsEmpty() || fn_name->Length() == 0)
        {
            fn_name = String::NewFromUtf8(isolate, "?", NewStringType::kInternalized).ToLocalChecked();
        }
        else if (frame->IsConstructor())
        {
            fn_name = String::NewFromUtf8(isolate, "[constructor]", NewStringType::kInternalized).ToLocalChecked();
        }

        String::Utf8Value function_name(isolate, fn_name);
        String::Utf8Value script_name(isolate, frame->GetScriptName());
        const int line_number = frame->GetLineNumber();
        const int column = frame->GetColumn();

        pos += snprintf(&buffer[pos], kMaxStackJsonSize,
                        "{\"function\":\"%s\",\"filename\":\"%s\",\"lineno\":%d,\"colno\":%d}",
                        *function_name,
                        *script_name,
                        line_number,
                        column);

        if (i < count - 1)
        {
            pos += snprintf(&buffer[pos], kMaxStackJsonSize, ",");
        }
    }

    pos += snprintf(&buffer[pos], kMaxStackJsonSize, "]");

    {
        std::lock_guard<std::mutex> lock(interrupt_mutex);
        interrupt_done = true;
    }
    interrupt_cv.notify_one();
}

void CaptureStackTrace(const FunctionCallbackInfo<Value> &args)
{
    char buffer[kMaxStackJsonSize] = {0};

    if (auto isolate = main_thread_isolate)
    {
        // Reset the interrupt_done flag
        {
            std::lock_guard<std::mutex> lock(interrupt_mutex);
            interrupt_done = false;
        }

        isolate->RequestInterrupt(ExecutionInterrupted, buffer);

        // Wait for the interrupt to complete
        std::unique_lock<std::mutex> lock(interrupt_mutex);
        interrupt_cv.wait(lock, []
                          { return interrupt_done; });
    }

    Local<String> result = String::NewFromUtf8(args.GetIsolate(), buffer, NewStringType::kNormal).ToLocalChecked();
    args.GetReturnValue().Set(result);
}

void SetMainIsolate(const FunctionCallbackInfo<Value> &args)
{
    main_thread_isolate = args.GetIsolate();
}

extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(Local<Object> exports,
                        Local<Value> module,
                        Local<Context> context)
{
    Isolate *isolate = context->GetIsolate();

    exports->Set(context,
                 String::NewFromUtf8(isolate, "captureStackTrace", NewStringType::kInternalized).ToLocalChecked(),
                 FunctionTemplate::New(isolate, CaptureStackTrace)->GetFunction(context).ToLocalChecked())
        .Check();

    exports->Set(context,
                 String::NewFromUtf8(isolate, "setMainIsolate", NewStringType::kInternalized).ToLocalChecked(),
                 FunctionTemplate::New(isolate, SetMainIsolate)->GetFunction(context).ToLocalChecked())
        .Check();
}