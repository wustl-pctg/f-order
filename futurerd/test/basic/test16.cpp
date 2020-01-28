// Like test15 but with a race
#include "common.h"
#include <future.hpp>

int fut1_param(int& shared) { shared = 1; return 42; }
int foo_param(cilk::future<int>* f, int& shared) {
  int x = f->get();
  return shared + x;
}

int bar_param(cilk::future<int>* f, int& shared) {
  int x = ++shared;
  return f->get() + x;
}

int main() {
  TEST_SETUP();
  int res1, res2;
  
  // Shared stack variable
  {
    res1 = res2 = -1;
    int s = -1;
    //cilk_async(int, f, fut1_param, s);
    auto f = async_helper<int,int&>(fut1_param, s);
    res1 = spawn foo_param(f, s);
    res2 = bar_param(f, s);
    sync;
    assert_detected(2);
  }

  TEST_TEARDOWN();
  return 0;
}
