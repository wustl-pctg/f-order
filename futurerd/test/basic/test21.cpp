// For loop test, no races
#include "common.h"
#include <future.hpp>

int main() {
  TEST_SETUP();

  int n = 4096*8;
  int shared = 42;
  int* array = (int*)malloc(sizeof(int) * n);
  cilk_for(int i = 0; i < n; ++i)
    array[i] = shared + 1;
  free(array);
  assert_detected(0);

  TEST_TEARDOWN();
  return 0;
}
