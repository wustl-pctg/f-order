// Similar to test17 but with a race
#include "common.h"
#include <future.hpp>

int fut(int& shared) {
  shared = 57;
  return 89;
}

cilk::future<int>* foo(int& shared) {
  //cilk_async(int, f, fut, shared);
  auto f = async_helper<int,int&>(fut,shared);
  shared = 42;
  return f;
}

cilk::future<int>* bar(int& shared) {
  //cilk_async(int, f, fut, shared);
  auto f = async_helper<int,int&>(fut,shared);
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
  assert(s1 == 57 || s1 == 42);
  s2 = 42;

  x = g->get();
  assert(s2 == 42 || s2 == 57);

  assert_detected(2);

  TEST_TEARDOWN();
  return 0;
}
