#include <node.h>
#include <mutex>
#include <future>

using namespace v8;
using namespace node;

static const int kMaxStackFrames = 255;

static std::mutex threads_mutex;
static std::unordered_map<v8::Isolate *, int> threads = {};

static void ExecutionInterrupted(Isolate *isolate, void *data)
{
    auto promise = static_cast<std::promise<Local<Array>> *>(data);
    auto stack = StackTrace::CurrentStackTrace(isolate, kMaxStackFrames, StackTrace::kDetailed);

    if (stack.IsEmpty())
    {
        promise->set_value(Array::New(isolate, 0));
        return;
    }

    auto frames = Array::New(isolate, stack->GetFrameCount());

    for (int i = 0; i < stack->GetFrameCount(); i++)
    {
        auto frame = stack->GetFrame(isolate, i);
        auto fn_name = frame->GetFunctionName();

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

        auto frame_obj = Object::New(isolate);
        frame_obj->Set(isolate->GetCurrentContext(),
                       String::NewFromUtf8(isolate, "function", NewStringType::kInternalized).ToLocalChecked(),
                       fn_name)
            .Check();

        frame_obj->Set(isolate->GetCurrentContext(),
                       String::NewFromUtf8(isolate, "filename", NewStringType::kInternalized).ToLocalChecked(),
                       frame->GetScriptName())
            .Check();

        frame_obj->Set(isolate->GetCurrentContext(),
                       String::NewFromUtf8(isolate, "lineno", NewStringType::kInternalized).ToLocalChecked(),
                       Integer::New(isolate, frame->GetLineNumber()))
            .Check();

        frame_obj->Set(isolate->GetCurrentContext(),
                       String::NewFromUtf8(isolate, "colno", NewStringType::kInternalized).ToLocalChecked(),
                       Integer::New(isolate, frame->GetColumn()))
            .Check();

        frames->Set(isolate->GetCurrentContext(), i, frame_obj).Check();
    }

    promise->set_value(frames);
}

Local<Array> CaptureStackTrace(Isolate *isolate)
{
    std::promise<Local<Array>> promise;
    auto future = promise.get_future();

    isolate->RequestInterrupt(ExecutionInterrupted, &promise);
    return future.get();
}

void CaptureStackTraces(const FunctionCallbackInfo<Value> &args)
{
    auto capture_from_isolate = args.GetIsolate();

    using ThreadResult = std::tuple<std::string, Local<Array>>;
    std::vector<std::future<ThreadResult>> futures;

    std::lock_guard<std::mutex> lock(threads_mutex);
    for (auto [thread_isolate, thread_id] : threads)
    {
        if (thread_isolate == capture_from_isolate)
            continue;
        auto thread_name = thread_id == -1 ? "main" : "worker-" + std::to_string(thread_id);
        futures.emplace_back(std::async(std::launch::async, [thread_name](Isolate *isolate) -> ThreadResult
                                        { return std::make_tuple(thread_name, CaptureStackTrace(isolate)); }, thread_isolate));
    }

    Local<Object> result = Object::New(capture_from_isolate);
    for (auto &future : futures)
    {
        auto [thread_name, frames] = future.get();
        auto key = String::NewFromUtf8(capture_from_isolate, thread_name.c_str(), NewStringType::kNormal).ToLocalChecked();
        result->Set(capture_from_isolate->GetCurrentContext(), key, frames).Check();
    }

    args.GetReturnValue().Set(result);
}

void Cleanup(void *arg)
{
    auto isolate = static_cast<Isolate *>(arg);
    std::lock_guard<std::mutex> lock(threads_mutex);
    threads.erase(isolate);
}

void RegisterThread(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();

    if (args.Length() != 1 || !args[0]->IsNumber())
    {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "registerThread() requires a single threadId argument", NewStringType::kInternalized).ToLocalChecked()));
        return;
    }

    auto thread_id = args[0].As<Number>()->Value();

    {
        std::lock_guard<std::mutex> lock(threads_mutex);
        threads.emplace(isolate, thread_id);
    }
    node::AddEnvironmentCleanupHook(isolate, Cleanup, isolate);
}

extern "C" NODE_MODULE_EXPORT void NODE_MODULE_INITIALIZER(Local<Object> exports,
                                                           Local<Value> module,
                                                           Local<Context> context)
{
    auto isolate = context->GetIsolate();

    exports->Set(context,
                 String::NewFromUtf8(isolate, "captureStackTrace", NewStringType::kInternalized).ToLocalChecked(),
                 FunctionTemplate::New(isolate, CaptureStackTraces)->GetFunction(context).ToLocalChecked())
        .Check();

    exports->Set(context,
                 String::NewFromUtf8(isolate, "registerThread", NewStringType::kInternalized).ToLocalChecked(),
                 FunctionTemplate::New(isolate, RegisterThread)->GetFunction(context).ToLocalChecked())
        .Check();
}