// for loop with local variables and futures, no race
#include "common.h"
#include <future.hpp>

int foo(int& shared, int x) { return shared + x; }

int main() {
  TEST_SETUP();

  int n = 128;
  int shared = 42;

  // First let's just test a for loop that makes a function call
#pragma cilk grainsize = 1
  cilk_for(int i = 0; i < n; ++i) {
    int x = 57;
    foo(shared, x); 
  }

  // Now let's asyncronously launch that function
  auto farray = (cilk::future<int>*)
    malloc(sizeof(cilk::future<int>) * n);

#pragma cilk grainsize = 1
  cilk_for(int i = 0; i < n; ++i) {
    int x = 57;
    reasync_helper<int,int&,int>(&farray[i], foo, shared, x);
  }

  cilk_for(int i = 0; i < n; ++i)
    farray[i].get();
           
  free(farray);
  assert_detected(0);

  TEST_TEARDOWN();
  return 0;
}
