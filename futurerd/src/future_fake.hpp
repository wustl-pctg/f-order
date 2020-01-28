/********** Launching futures **********/
#include <new>

#define FUTURE_PROLOG()
#define FUTURE_EPILOG()

/** The syntax I'd really like to have is:
 * (1) Normal future "spawning":
 *     future<int>* f = async foo(arg1, arg2);
 * (2) Reusing a future for a new task: (think placement new)
 *     async(f) foo(arg1, arg2);
 *
 * I don't see how to do these without compiler support.
 */

// Helper function versions
// I think there are better ways of doing this, but they have so far
// bested my C++11/14/17 knowledge. Probably some fancy template
// metaprogramming, plus almost surely some ways to use lambda
// functions. Unfortunately, the helper function must NOT be inlined,
// but it seems that our version of clang does not support lambda
// function attributes like noinline...
template<typename ReturnType, typename... Args>
[[gnu::noinline]] cilk::future<ReturnType>*
async_helper(ReturnType (*func)(Args...), Args... args)
{
  __DC;
  auto __fut = new cilk::future<ReturnType>();
  // __fut->start();
  __FS;
  __EC;
  __fut->finish(func(args...));
  cilk_future_helper_leave(&__fut->m_rd_data);
  return __fut;
}

// This c++14 version is a big better because you don't need to
// explicitly use the template arguments unless "func" is overloaded.
// #define _RT std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>
// template< class Function, class... Args>
// [[gnu::noinline]] cilk::future<_RT>*
// reasync_helper(cilk::future<_RT>* __fut, Function&& func, Args&&... args) {
//   __DC;
//   new(__fut) = cilk::future<_RT>();
//   __FS; // "start" the future
//   __EC;
//   __fut->finish(func(std::forward<Args>(args)...));
//   cilK_future_helper_leave(&__fut->m_rd_data);
//   return __fut;
// }

template<typename ReturnType, typename... Args>
[[gnu::noinline]] void
reasync_helper(cilk::future<ReturnType>* __fut,
               ReturnType (*func)(Args...), Args... args) {
  __DC;
  new(__fut) cilk::future<ReturnType>();
  // __fut->start();
  __FS; // "start" the future
  __EC;
  __fut->finish(func(args...));
  cilk_future_helper_leave(&__fut->m_rd_data);
  return;
}

/** Asynchronously start foo(x,y) -> int with
 *  cilk_async(int, f, foo, x, y)
 */
#define cilk_async(T,name,func,args...)         \
  __DC; auto (name) = new cilk::future<T>(); __EC;  \
  __FS; (name)->finish(func(args)); __FC;

// Allocate new future, but set it to a given variable
#define create_future(T,fut,func,args...)       \
  __DC; (fut) = new cilk::future<T>(); __EC;   \
  __FS; (fut)->finish(func(args)); __FC;

// Do the create_future as above, but in two steps
#define create_future_handle(T, fut)       \
  __DC; (fut) = new cilk::future<T>(); __EC;

#define spawn_proc_with_future_handle(fut,func,args...)       \
  __FS; (fut)->finish(func(args)); __FC;

// Use preallocated memory for the future itself
// Use a temporary variable in case "loc" is e.g. farray[i], which may
// cause a race on i
#define reuse_future(T,loc,func,args...)        \
  {__DC; auto __f = (loc);                      \
    new(__f) cilk::future<T>(); __EC;             \
    __FS; (__f)->finish(func(args)); __FC;}

// When we need to return a pair of futures...
// can't think of a better way to do this...
#define cilk_pg_async2(T,f1,f2,func,args...)          \
  __DC; auto (f1) = new cilk::future<T>();            \
  auto (f2) = new cilk::future<T>(); __EC;            \
  __FS; func(args);                                    \
  f1->finish(); f2->finish(); __FC;                    \

#define __end_cilk_for_body() __DC; race_detector::t_clear_stack = true; __EC

/** Versions that manually call put(), possibly before the end of the
    function.
 */
#define cilk_pg_async(T,name,func,args...)      \
  __DC; auto (name) = new cilk::future<T>(); __EC;  \
  __FS; func(args);                                  \
  (name)->finish(); __FC;
#define create_pg_future(T,fut,func,args...)    \
  __DC; (fut) = new cilk::future<T>(); __EC;    \
  __FS; func(args);                              \
  (fut)->finish(); __FC;
#define reuse_pg_future(T,loc,func,args...)     \
  __DC; new(loc) cilk::future<T>(); __EC;       \
  __FS; func(args);                              \
  loc->finish(); __FC;

/** Verisons that automatically deduce the future type. */
#define FTYPE(func,args...) decltype((func)(args))
#define cilk_auto_async(name,func,args...)      \
  cilk_async(FTYPE(func,args),name,func,args)
#define create_auto_future(fut,func,args...)    \
  create_future(FTYPE(func,args),fut,func,args)
#define reuse_auto_future(loc,func,args...)     \
  reuse_future(FTYPE(func,args),loc,func,args)
#define cilk_auto_pg_async(name,func,args...)     \
  cilk_pg_async(FTYPE(func,args),name,func,args)
#define create_auto_pg_future(fut,func,args...)     \
  create_pg_future(FTYPE(func,args),fut,func,args)
#define reuse_auto_pg_future(loc,func,args...)    \
  reuse_pg_future(FTYPE(func,args),loc,func,args)

// Some details on how the cilk fake macros implement spawn. Will be
// useful when implementing parallel futures.
/**
CILK_FAKE_SPAWN_R(a, fib(n-1)) -> a = spawn fib(n-1);
 CILK_FAKE_CALL_SPAWN_HELPER(CILK_FAKE_SPAWN_HELPER(expr, args), sf);
 
CILK_FAKE_CALL_SPAWN_HELPER:
  CILK_FAKE_DEFERRED_ENTER_FRAME(sf) --> if (!sf.worker) __cilk_fake_enter_frame(sf)
  CILK_FAKE_SAVE_FP(sf) --> assembly code (for non-Windows)
  if (__builtin_expect(! CILK_SETJMP(sf.ctx), 1)) {
    helper();
  }


CILK_FAKE_SPAWN_HELPER creates a lambda function which calls
CILK_FAKE_SPAWN_HELPER_BODY, which:
  CILK_FAKE_SPAWN_HELPER_PROLOG(sf);
  expand args
  fake_detach
  expression (function call)
  CILK_FAKE_SPAWN_HELPER_EPILOG

 */
