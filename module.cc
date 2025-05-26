#include <node.h>
#include <mutex>
#include <future>
#include <chrono>

using namespace v8;
using namespace node;
using namespace std::chrono;

static const int kMaxStackFrames = 255;

// Structure to hold information for each thread/isolate
struct ThreadInfo
{
    // Thread name
    std::string thread_name;
    // Last time this thread was seen in milliseconds since epoch
    milliseconds last_seen;
};

static std::mutex threads_mutex;
// Map to hold all registered threads and their information
static std::unordered_map<v8::Isolate *, ThreadInfo> threads = {};

// Function to be called when an isolate's execution is interrupted
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

// Function to capture the stack trace of a single isolate
Local<Array> CaptureStackTrace(Isolate *isolate)
{
    std::promise<Local<Array>> promise;
    auto future = promise.get_future();

    // The v8 isolate must be interrupted to capture the stack trace
    // Execution resumes automatically after ExecutionInterrupted returns
    isolate->RequestInterrupt(ExecutionInterrupted, &promise);
    return future.get();
}

// Function to capture stack traces from all registered threads
void CaptureStackTraces(const FunctionCallbackInfo<Value> &args)
{
    auto capture_from_isolate = args.GetIsolate();

    using ThreadResult = std::tuple<std::string, Local<Array>>;
    std::vector<std::future<ThreadResult>> futures;

    // We collect the futures into a vec so they can be processed in parallel
    std::lock_guard<std::mutex> lock(threads_mutex);
    for (auto [thread_isolate, thread_info] : threads)
    {
        if (thread_isolate == capture_from_isolate)
            continue;
        auto thread_name = thread_info.thread_name;
        futures.emplace_back(std::async(std::launch::async, [thread_name](Isolate *isolate) -> ThreadResult
                                        { return std::make_tuple(thread_name, CaptureStackTrace(isolate)); }, thread_isolate));
    }

    // We wait for all futures to complete and collect their results into a JavaScript object
    Local<Object> result = Object::New(capture_from_isolate);
    for (auto &future : futures)
    {
        auto [thread_name, frames] = future.get();
        auto key = String::NewFromUtf8(capture_from_isolate, thread_name.c_str(), NewStringType::kNormal).ToLocalChecked();
        result->Set(capture_from_isolate->GetCurrentContext(), key, frames).Check();
    }

    args.GetReturnValue().Set(result);
}

// Cleanup function to remove the thread from the map when the isolate is destroyed
void Cleanup(void *arg)
{
    auto isolate = static_cast<Isolate *>(arg);
    std::lock_guard<std::mutex> lock(threads_mutex);
    threads.erase(isolate);
}

// Function to register a thread and update it's last seen time
void RegisterThread(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();

    if (args.Length() != 1 || !args[0]->IsString())
    {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "registerThread(name) requires a single name argument", NewStringType::kInternalized).ToLocalChecked()));
        return;
    }

    v8::String::Utf8Value utf8(isolate, args[0]);
    std::string thread_name(*utf8 ? *utf8 : "");

    {
        std::lock_guard<std::mutex> lock(threads_mutex);
        auto found = threads.find(isolate);
        if (found == threads.end())
        {
            threads.emplace(isolate, ThreadInfo{thread_name, milliseconds::zero()});
            // Register a cleanup hook to remove this thread when the isolate is destroyed
            node::AddEnvironmentCleanupHook(isolate, Cleanup, isolate);
        }
        else
        {
            auto &thread_info = found->second;
            thread_info.thread_name = thread_name;
            thread_info.last_seen = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        }
    }
}

// Function to get the last seen time of all registered threads
void GetThreadLastSeen(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    Local<Object> result = Object::New(isolate);
    milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    {
        std::lock_guard<std::mutex> lock(threads_mutex);
        for (const auto &[thread_isolate, info] : threads)
        {
            if (info.last_seen == milliseconds::zero())
                continue; // Skip threads that have not registered more than once

            int64_t ms_since = (now - info.last_seen).count();
            result->Set(isolate->GetCurrentContext(),
                        String::NewFromUtf8(isolate, info.thread_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
                        Number::New(isolate, ms_since))
                .Check();
        }
    }
    args.GetReturnValue().Set(result);
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

    exports->Set(context,
                 String::NewFromUtf8(isolate, "getThreadLastSeen", NewStringType::kInternalized).ToLocalChecked(),
                 FunctionTemplate::New(isolate, GetThreadLastSeen)->GetFunction(context).ToLocalChecked())
        .Check();
}