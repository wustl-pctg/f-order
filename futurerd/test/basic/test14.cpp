// Test 13 but with a race.
#include "common.h"
#include <future.hpp>

int g_shared = 0;
int fut_task() { return 42; }
int foo(cilk::future<int>* f) {
  int x = f->get();
  return g_shared += x;
}

int main() {
  TEST_SETUP();
  int res1, res2;
  
  res1 = res2 = -1;
  //cilk_async(int, f, fut_task);
  auto f = async_helper<int>(fut_task);
  //cilk_async(int, g, fut_task);
  auto g = async_helper<int>(fut_task);
  res1 = spawn foo(f);
  res2 = foo(g);
  sync;
  assert(g_shared == 42 || g_shared == 84);
  assert_detected(1);

  TEST_TEARDOWN();
  return 0;
}
