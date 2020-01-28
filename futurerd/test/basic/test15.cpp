// Similar to test13, but with a single future task that is "touched"
// multiple times
#include "common.h"
#include <future.hpp>

int fut1_none() { return 42; }
int foo_none(cilk::future<int>* f) { return f->get(); }
int bar_none(cilk::future<int>* f) { return f->get(); }

int fut1_param(int& shared) { shared = 1; return 42; }
int foo_param(cilk::future<int>* f, int& shared) {
  int x = f->get();
  return shared + x;
}

int g_shared1 = 0;
int fut1_global() { g_shared1 = 1; return 42; }
int foo_global(cilk::future<int>* f) {
  int x = f->get();
  return g_shared1 + x;
}

int bar_global(cilk::future<int>* f) {
  int x = f->get();
  return g_shared1 + x;
}

int main() {
  TEST_SETUP();
  int res1, res2;
  
  // No shared data at all
  {
    res1 = res2 = -1;
    //cilk_async(int, f, fut1_none);
    auto f = async_helper<int>(fut1_none);
    res1 = spawn foo_none(f);
    res2 = bar_none(f);
    sync;
    assert(res1 == 42);
    assert(res2 == 42);
    assert_detected(0);
  }

  // Shared stack variable
  {
    res1 = res2 = -1;
    int s = -1;
    //cilk_async(int, f, fut1_param, s);
    auto f = async_helper<int,int&>(fut1_param, s);
    res1 = spawn foo_param(f, s);
    res2 = foo_param(f, s);
    sync;
    assert(res1 == 43);
    assert(res2 == 43);
    assert_detected(0);
  }

  // Shared global variable
  {
    res1 = res2 = -1;
    //cilk_async(int, f, fut1_global);
    auto f = async_helper<int>(fut1_global);
    res1 = spawn foo_global(f);
    res2 = bar_global(f);
    sync;
    assert(res1 == 43);
    assert(res2 == 43);
    assert(g_shared1 == 1);
    assert_detected(0);
  }

  TEST_TEARDOWN();
  return 0;
}
