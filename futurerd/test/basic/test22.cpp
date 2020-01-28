// For loop test, with a race
#include "common.h"
#include <future.hpp>
#include <cstdio>

int main() {
  TEST_SETUP();

  int n = 4096;
  int shared = 42;
  int* array = (int*)malloc(sizeof(int) * n);

#pragma cilk grainsize = 1
  cilk_for(int i = 0; i < n; ++i) {
    array[i] = shared++;
  }
  free(array);

  assert_detected(n-1);

  TEST_TEARDOWN();
  return 0;
}
