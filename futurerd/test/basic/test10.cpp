// Regression test: hopefully we didn't mess anything up.
#include <cassert>
#include <iostream>

#include "common.h"
#include <future.hpp>

int g_shared = 0;

int fib(int n)
{
  if (n < 2) return n;

  // Clang (erroneously) thinks x could be uninitialized
  #pragma GCC diagnostic ignored "-Wuninitialized"
  int x = spawn fib(n - 1);
  int y = fib(n - 2);
  sync;

  g_shared++;

  return x + y;
}

int main(int argc, char* argv[])
{
  FUTURE_PROLOG();
  TEST_SETUP();

  int result = fib(4);
  assert(result == 3);
  assert_detected(1);

  TEST_TEARDOWN();
  FUTURE_EPILOG();
  return 0;
}
