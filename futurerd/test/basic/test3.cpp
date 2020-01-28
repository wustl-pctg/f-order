// Access to global variable -- no race
#include <iostream>
#include <cassert>

#include "common.h"
#include <future.hpp>

int g_shared = 0;

int foo() {
  g_shared = 57;
  return 42;
}

int bar(cilk::future<int>* f) {
  int x = f->get();
  return g_shared - x;
}

int main(int argc, char* argv[])
{
  FUTURE_PROLOG();
  TEST_SETUP();

  //cilk_async(int, f, foo, f);
  auto f = async_helper<int>(foo);
  int x = bar(f);

  assert(x == 15);
  assert_detected(0);

  TEST_TEARDOWN();
  FUTURE_EPILOG();
  
  return 0;
}
