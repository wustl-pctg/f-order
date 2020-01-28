// Two stage pipeline.
#include "common.h"
#include <future.hpp>
#include <algorithm>

int shared = 0;
cilk::future<int>* fhandles1;
cilk::future<int>* fhandles2;

int stage2(int i) {
  // Wait for previous iteration stage 2
  if (i != 0) fhandles2[i-1].get();

  // Do some work

  return 42;
}

int stage1(int i) {
  // Wait for previous iteration stage 1
  if (i != 0) fhandles1[i-1].get();
  else shared++;

  // Do some work

  // launch stage 2
  reasync_helper(&fhandles2[i], stage2, i);

  return 42;
}

int main() {
  TEST_SETUP();

  int n = 32;
  fhandles1 = (cilk::future<int>*) malloc(sizeof(cilk::future<int>) * n);
  fhandles2 = (cilk::future<int>*) malloc(sizeof(cilk::future<int>) * n);

  for (int i = 0; i < n; ++i) {
    reasync_helper(&fhandles1[i], stage1, i);
  }

  // Structured use of futures needs this
#ifdef STRUCTURED_FUTURES
  fhandles1[n-1].get();
#endif
  fhandles2[n-1].get();
  shared++;

  free(fhandles1);
  free(fhandles2);

  assert(shared == 2);
  assert_detected(0);

  TEST_TEARDOWN();
  return 0;
}
