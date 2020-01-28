// Two attached spawn branches, both are gets, no race
// Version without any shared data, one with param, one with global
#include "common.h"
#include <future.hpp>

int fut1_none() { return 42; }
int fut2_none() { return 57; }
int foo_none(cilk::future<int>* f) { return f->get(); }
int bar_none(cilk::future<int>* f) { return f->get(); }

int fut1_param(int& shared) { shared = 1; return 42; }
int fut2_param(int& shared) { shared = 2; return 57; }
int foo_param(cilk::future<int>* f, int& shared) {
  int x = f->get();
  return shared + x;
}

int g_shared1 = 0;
int g_shared2 = 0;
int fut1_global() { g_shared1 = 1; return 42; }
int fut2_global() { g_shared2 = 2; return 57; }
int foo_global(cilk::future<int>* f) {
  int x = f->get();
  return g_shared1 + x;
}

int bar_global(cilk::future<int>* f) {
  int x = f->get();
  return g_shared2 + x;
}

int main() {
  TEST_SETUP();
  int res1, res2;
  
  // No shared data at all
  {
    res1 = res2 = -1;
    //cilk_async(int, f, fut1_none);
    auto f = async_helper<int>(fut1_none);
    //cilk_async(int, g, fut2_none);
    auto g = async_helper<int>(fut2_none);
    res1 = spawn foo_none(f);
    res2 = bar_none(g);
    sync;
    assert(res1 == 42);
    assert(res2 == 57);
    assert_detected(0);
  }

  // Shared stack variable
  {
    res1 = res2 = -1;
    int s1, s2;
    s1 = s2 = -1;
    //cilk_async(int, f, fut1_param, s1);
    auto f = async_helper<int,int&>(fut1_param, s1);
    //cilk_async(int, g, fut2_param, s2);
    auto g = async_helper<int,int&>(fut2_param, s2);
    res1 = spawn foo_param(f, s1);
    res2 = foo_param(g, s2);
    sync;
    assert(res1 == 43);
    assert(res2 == 59);
    assert_detected(0);
  }

  // Shared global variable
  {
    res1 = res2 = -1;
    //cilk_async(int, f, fut1_global);
    auto f = async_helper<int>(fut1_global);
    //cilk_async(int, g, fut2_global);
    auto g = async_helper<int>(fut2_global);
    res1 = spawn foo_global(f);
    res2 = bar_global(g);
    sync;
    assert(res1 == 43);
    assert(res2 == 59);
    assert(g_shared1 == 1);
    assert(g_shared2 == 2);
    assert_detected(0);
  }

  TEST_TEARDOWN();
  return 0;
}
