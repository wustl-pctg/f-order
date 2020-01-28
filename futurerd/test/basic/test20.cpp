// Like test19, but with some races
// One side of a spawn touches a future, the other gets.
#include "common.h"
#include <future.hpp>

int fut(int& shared) {
  shared = 57;
  return 89;
}

int foo(int& shared) {
  shared = 42;
  return 1;
}

cilk::future<int>* bar(int& s1, int& s2) {
  //cilk_async(int, f, fut, s2);
  auto f = async_helper<int,int&>(fut, s2);
  s1 = 42;
  return f;
}

int main() {
  TEST_SETUP();
  int s1, s2, x;

  {
    s1 = s2 = -1;

    //cilk_async(int, f, fut, s1);
    auto f = async_helper<int,int&>(fut, s1);
  
    // Clang (erroneously) thinks f could be uninitialized
    // I think this is fixed in modern versions of clang...
#pragma GCC diagnostic ignored "-Wuninitialized"
    x = spawn foo(s2);
    auto g = bar(s1, s2);
    sync;

    assert(x == 1);

    x = g->get();
    assert(x == 89);
    assert(s1 == 57 || s1 == 42);
  
    x = f->get();
    assert(x == 89);
    assert(s2 == 42 || s2 == 57);

    assert_detected(2);
  }

  // Now opposite
  {
    s1 = s2 = -1;

    //cilk_async(int, f, fut, s1);
    auto f = async_helper<int,int&>(fut, s1);
  
    // Clang (erroneously) thinks f could be uninitialized
    // I think this is fixed in modern versions of clang...
#pragma GCC diagnostic ignored "-Wuninitialized"
    auto g = spawn bar(s1, s2);
    x = foo(s2);
    sync;

    assert(x == 1);

    x = g->get();
    assert(x == 89);
    assert(s1 == 57 || s1 == 42);
  
    x = f->get();
    assert(x == 89);
    assert(s2 == 42 || s2 == 57);

    assert_detected(4);

  }

  TEST_TEARDOWN();
  return 0;
}
