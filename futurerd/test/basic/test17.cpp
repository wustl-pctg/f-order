// Spawning tasks that then spawn futures. No race.
#include "common.h"
#include <future.hpp>

int fut1(int& shared) {
  shared = 57;
  return 89;
}

int fut2() { return 89; }

cilk::future<int>* foo(int& shared) {
  shared = 42;
  //cilk_async(int, f, fut1, shared);
  auto f = async_helper<int,int&>(fut1, shared);
  return f;
}

cilk::future<int>* bar(int& shared) {
  //cilk_async(int, f, fut2);
  auto f = async_helper<int>(fut2);
  shared = 42;
  return f;
}



int main() {
  TEST_SETUP();
  int s1, s2;

  s1 = s2 = -1;
  // Clang (erroneously) thinks f could be uninitialized
  // I think this is fixed in modern versions of clang...
  #pragma GCC diagnostic ignored "-Wuninitialized"
  auto f = spawn foo(s1);
  auto g = bar(s2);
  sync;

  int x = f->get();
  assert(x == 89);
  assert(s1 == 57);
  
  x = g->get();
  assert(x == 89);
  assert(s2 == 42);

  assert_detected(0);

  TEST_TEARDOWN();
  return 0;
}
