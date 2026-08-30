/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding Async Methods to Objects
 * -------------------------
 *
 * An object in an OPC UA information model may contain methods similar to
 * objects in a programming language. Methods are represented by a MethodNode.
 * Note that several objects may reference the same MethodNode. When an object
 * type is instantiated, a reference to the method is added instead of copying
 * the MethodNode. Therefore, the identifier of the context object is always
 * explicitly stated when a method is called.
 *
 * The method callback takes as input a custom data pointer attached to the
 * method node, the identifier of the object from which the method is called,
 * and two arrays for the input and output arguments. The input and output
 * arguments are all of type :ref:`variant`. Each variant may in turn contain a
 * (multi-dimensional) array or scalar of any data type.
 *
 * Constraints for the method arguments are defined in terms of data type, value
 * rank and array dimension (similar to variable definitions). The argument
 * definitions are stored in child VariableNodes of the MethodNode with the
 * respective BrowseNames ``(0, "InputArguments")`` and ``(0,
 * "OutputArguments")``.
 *
 * Example: Hello World Method
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The method takes a string scalar and returns a string scalar with "Hello "
 * prepended. The type and length of the input arguments is checked internally
 * by the SDK, so that we don't have to verify the arguments in the callback.
 *
 * ``UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY`` only makes sense if the actual
 * work happens off the server's own event-loop thread -- otherwise the method
 * could just compute the result and return it right away. So this example
 * spawns one worker thread per call that sleeps for a while to stand in for
 * real work (a database query, a request to another device, ...), and only
 * then reports the result. The worker hands off to the server via the public,
 * thread-safe ``UA_Server_setAsyncCallMethodResult`` API, which is safe to
 * call from any thread: it takes the server's internal lock itself.
 *
 * No lock of our own is needed for the handoff to the worker either: the job
 * data (the output pointer and a copy of the input) is passed as the new
 * thread's start argument, and thread creation itself is a synchronization
 * point -- everything written before ``pthread_create``/``CreateThread`` is
 * guaranteed visible to the new thread. After that, only that one thread
 * ever touches the job. The array of thread handles below (so we can join
 * everyone before shutdown) is only ever touched from the server's own
 * event-loop thread, both when a call comes in and after
 * ``UA_Server_runUntilInterrupt`` returns -- so nothing there needs a lock
 * either. There simply isn't any state left that two threads access at
 * the same time without already going through the server's own API.
 *
 * The server's memory-safety guarantee also means the application does not
 * need to track whether a call was canceled before touching the output
 * memory: the server keeps it alive until the worker's call to
 * ``UA_Server_setAsyncCallMethodResult``, however late that is. A report for
 * an operation that was already resolved elsewhere (canceled, timed out) just
 * returns ``UA_STATUSCODE_BADNOTFOUND`` -- not a crash.
 *
 * The one rule the application *does* have to follow itself: worker threads
 * that may still call ``UA_Server_setAsync*Result`` must be stopped and
 * joined before ``UA_Server_delete`` runs. The server can no longer field
 * that call once it is gone. See ``main`` below. */

#include <open62541/server.h>
#include <open62541/plugin/log.h>

#ifdef UA_ARCHITECTURE_WIN32
#include <windows.h>
typedef HANDLE workerThread;
#define WORKER_THREAD_FUNC(name) static DWORD WINAPI name(LPVOID workerArg)
static void
workerThreadCreate(workerThread *t, LPTHREAD_START_ROUTINE fn, void *arg) {
    *t = CreateThread(NULL, 0, fn, arg, 0, NULL);
}
static void workerThreadJoin(workerThread t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
static void workerSleep(unsigned ms) { Sleep(ms); }
#else
#include <pthread.h>
#include <unistd.h>
typedef pthread_t workerThread;
#define WORKER_THREAD_FUNC(name) static void * name(void *workerArg)
static void
workerThreadCreate(workerThread *t, void *(*fn)(void*), void *arg) {
    pthread_create(t, NULL, fn, arg);
}
static void workerThreadJoin(workerThread t) { pthread_join(t, NULL); }
static void workerSleep(unsigned ms) { usleep(((useconds_t)ms) * 1000); }
#endif

/* Cap on the number of calls in flight at once -- just so the thread-handle
 * array below has a fixed size. Only the server's own event-loop thread ever
 * reads or writes it (see the comment above), so it needs no lock. */
#define MAX_CONCURRENT_CALLS 8
static workerThread activeThreads[MAX_CONCURRENT_CALLS];
static size_t activeThreadCount;

typedef struct {
    UA_Server *server;
    UA_Variant *output; /* Where to write the result / signal completion */
    UA_String input;    /* Owned copy -- the request may be gone by the time we run */
} AsyncJob;

static void
asyncOperationCancelCallback(UA_Server *s, const void *out) {
    /* Purely informational -- there is no shared state to touch here. The
     * server keeps *out alive until the worker's call to
     * UA_Server_setAsyncCallMethodResult below, however late that is. An
     * application could use this callback to cancel a blocking network call
     * the worker is waiting on; this example doesn't bother. */
    UA_LOG_INFO(UA_Server_getConfig(s)->logging, UA_LOGCATEGORY_APPLICATION,
                "Async method call %p was canceled", out);
}

WORKER_THREAD_FUNC(asyncCallWorker) {
    AsyncJob *job = (AsyncJob*)workerArg;

    /* Stand-in for real work that takes time and doesn't touch the server at
     * all -- a database query, a request to another system, a slow
     * computation. This runs entirely off the server's event loop, so the
     * server keeps serving other clients while we wait. */
    workerSleep(2000);

    UA_String out = UA_STRING_NULL;
    UA_String_format(&out, "Hello %S", job->input);
    UA_String_clear(&job->input);

    UA_Variant_setScalarCopy(job->output, &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);

    /* Report the result. If the call was already canceled or timed out in
     * the meantime, this returns UA_STATUSCODE_BADNOTFOUND -- the response
     * already went out and our result is simply discarded. Either way this
     * call is what lets the server free the output memory, so it must
     * always happen. It is safe to call from this thread: the server takes
     * its own internal lock for the duration of the call. */
    UA_StatusCode res =
        UA_Server_setAsyncCallMethodResult(job->server, job->output, UA_STATUSCODE_GOOD);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Server_getConfig(job->server)->logging, UA_LOGCATEGORY_APPLICATION,
                    "Async method call %p was already resolved (%s)",
                    (void*)job->output, UA_StatusCode_name(res));

    UA_free(job);
#ifdef UA_ARCHITECTURE_WIN32
    return 0;
#else
    return NULL;
#endif
}

static UA_StatusCode
helloWorldMethodCallback(UA_Server *s,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    if(activeThreadCount >= MAX_CONCURRENT_CALLS) {
        UA_LOG_WARNING(UA_Server_getConfig(s)->logging, UA_LOGCATEGORY_APPLICATION,
                       "Too many concurrent async calls");
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    }

    AsyncJob *job = (AsyncJob*)UA_malloc(sizeof(AsyncJob));
    if(!job)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    job->server = s;
    job->output = output;
    UA_String_copy((const UA_String*)input->data, &job->input);

    /* Everything written to *job above is guaranteed visible to the new
     * thread -- no lock needed for this handoff. */
    workerThreadCreate(&activeThreads[activeThreadCount], asyncCallWorker, job);
    activeThreadCount++;

    /* Signal async processing to the server. Will be completed later. */
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

int main(void) {
    UA_Server *server = UA_Server_new();

    /* Set the cancel callback */
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->asyncOperationCancelCallback = asyncOperationCancelCallback;

    /* Add method */
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","Say `Hello World` async");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","Hello World async");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);

    /* Run the server */
    UA_Server_runUntilInterrupt(server);

    /* Join every worker *before* deleting the server. Once UA_Server_delete
     * runs, a late UA_Server_setAsyncCallMethodResult call from a
     * still-running worker would touch freed memory -- the server can only
     * guarantee safety for calls that arrive before it is gone. */
    for(size_t i = 0; i < activeThreadCount; i++)
        workerThreadJoin(activeThreads[i]);

    /* Clean up */
    UA_Server_delete(server);

    return 0;
}
