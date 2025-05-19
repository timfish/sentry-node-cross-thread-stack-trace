#include <node.h>
#include <mutex>
#include <map>
#include <sstream>
#include <future>

using namespace v8;
using namespace node;

static const int kMaxStackFrames = 255;

static std::unordered_map<v8::Isolate *, int> threads = {};

static void ExecutionInterrupted(Isolate *isolate, void *data)
{
    auto promise = static_cast<std::promise<std::string> *>(data);

    Local<StackTrace> stack = StackTrace::CurrentStackTrace(isolate, kMaxStackFrames, StackTrace::kDetailed);

    if (stack.IsEmpty())
    {
        promise->set_value("[]");
        return;
    }

    std::ostringstream out;

    out << "[";
    auto count = stack->GetFrameCount();

    for (auto i = 0; i < count; i++)
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

        String::Utf8Value function_name(isolate, fn_name);
        String::Utf8Value script_name(isolate, frame->GetScriptName());
        auto line_number = frame->GetLineNumber();
        auto column = frame->GetColumn();

        out << "{\"function\":\"" << *function_name
            << "\",\"filename\":\"" << *script_name
            << "\",\"lineno\":" << line_number
            << ",\"colno\":" << column << "}";

        if (i < count - 1)
        {
            out << ",";
        }
    }

    out << "]";

    promise->set_value(out.str());
}

std::string CaptureStackTrace(Isolate *isolate)
{
    std::promise<std::string> promise;
    auto future = promise.get_future();

    isolate->RequestInterrupt(ExecutionInterrupted, &promise);
    return future.get();
}

void CaptureStackTraces(const FunctionCallbackInfo<Value> &args)
{
    bool exclude_workers = args.Length() == 1 && args[0]->IsBoolean() && args[0].As<Boolean>()->Value();
    auto capture_from_isolate = args.GetIsolate();

    std::vector<std::future<std::string>> futures;

    for (auto &thread : threads)
    {
        auto thread_isolate = thread.first;
        if (thread_isolate != capture_from_isolate)
        {
            int thread_id = thread.second;

            if (exclude_workers && thread_id != -1)
            {
                continue;
            }

            auto thread_name = thread_id == -1 ? "main" : "worker-" + std::to_string(thread_id);

            futures.emplace_back(std::async(std::launch::async, [thread_name](Isolate *isolate)
                                            { return "\"" + thread_name + "\":" + CaptureStackTrace(isolate); }, thread_isolate));
        }
    }

    std::ostringstream out;

    auto count = futures.size();
    out << "{";
    for (auto &future : futures)
    {
        out << future.get();
        if (--count > 0)
        {
            out << ",";
        }
    }
    out << "}";

    args.GetReturnValue().Set(String::NewFromUtf8(capture_from_isolate, out.str().c_str(), NewStringType::kNormal).ToLocalChecked());
}

void Cleanup(void *arg)
{
    auto isolate = static_cast<Isolate *>(arg);
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

    int thread_id = args[0].As<Number>()->Value();

    threads.emplace(isolate, thread_id);
    node::AddEnvironmentCleanupHook(isolate, Cleanup, isolate);
}

extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(Local<Object> exports,
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