#include "common.h"
#include <future.hpp>


int fib(int n)
{
  if (n < 2) return n;

  // Clang (erroneously) thinks x could be uninitialized
  #pragma GCC diagnostic ignored "-Wuninitialized"
  int x = spawn fib(n - 1);
  int y = fib(n - 2);
  sync;

  return x + y;
}

int main(int argc, char* argv[])
{
  FUTURE_PROLOG();
  TEST_SETUP();

  //cilk_async(int, f, fib, 10);
  auto f = async_helper<int,int>(fib, 10);
  assert(f->get() == 55);

  assert_detected(0);
  TEST_TEARDOWN();
  FUTURE_EPILOG();
  return 0;
}
