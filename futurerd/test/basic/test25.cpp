// Multiple for loops, one without a race, the second with a race
// For loop test, with a race
#include "common.h"
#include <future.hpp>

int main() {
  TEST_SETUP();

  int n = 4096;
  int shared = 42;
  int* array = (int*)malloc(sizeof(int) * n);

  // Just read, no race
#pragma cilk grainsize = 1
  cilk_for(int i = 0; i < n; ++i) {
    array[i] = shared;
  }

  // Now write
#pragma cilk grainsize = 1
  cilk_for(int i = 0; i < n; ++i) {
    shared += array[i];
  }

  free(array);

  assert_detected(n-1);

  TEST_TEARDOWN();
  return 0;
}
