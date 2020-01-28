// Similar to test11 but with a race on a referencen parameter, and a
// reversed read/write pattern.
#include "common.h"

int foo(int& shared) {
  int y = shared;
  return y;
}

int bar(int& shared) {
  shared = 42;
  return shared;
}


int main(int argc, char* argv[])
{
  TEST_SETUP();

  int x = -1, s = 0;
  x = spawn foo(s);
  bar(s);
  sync;

  // We will definitely see 0 since we run sequentially.
  assert(x == 0 || x == 42);
  assert_detected(1);

  TEST_TEARDOWN();
  
  return 0;
}
