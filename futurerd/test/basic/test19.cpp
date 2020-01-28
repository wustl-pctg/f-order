// One side of a spawn touches a future, the other gets. No race.
#include "common.h"
#include <future.hpp>

int fut(int& shared) {
  shared = 57;
  return 89;
}

int foo(int& shared, cilk::future<int>* f) {
  int x = f->get();
  shared = 42;
  return x;
}

cilk::future<int>* bar(int& shared) {
  shared = 42;
  //cilk_async(int, f, fut, shared);
  auto f = async_helper<int,int&>(fut,shared);
  return f;
}



int main() {
  TEST_SETUP();
  int s1, s2, x;

  { // left side touches, right side creates a future
    s1 = s2 = -1;

    //cilk_async(int, f, fut, s1);
    auto f = async_helper<int,int&>(fut,s1);
  
    // Clang (erroneously) thinks f could be uninitialized
    // I think this is fixed in modern versions of clang...
#pragma GCC diagnostic ignored "-Wuninitialized"
    x = spawn foo(s1, f);
    auto g = bar(s2);
    sync;

    assert(x == 89);
    assert(s1 == 42);
    s1 = 89; // want to make sure no race here

    //x = g->get();
    cilk_future_get_result(x, g);
    assert(x == 89);
    assert(s2 == 57);
    s2 = 42;
  
    assert_detected(0);
  }

  { // now opposite: left side creates, left side touches
    s1 = s2 = -1;

    //cilk_async(int, f, fut, s1);
    auto f = async_helper<int,int&>(fut,s1);
  
    // Clang (erroneously) thinks f could be uninitialized
    // I think this is fixed in modern versions of clang...
#pragma GCC diagnostic ignored "-Wuninitialized"
    auto g = spawn bar(s2);
    x = foo(s1, f);
    sync;

    assert(x == 89);
    assert(s1 == 42);
    s1 = 89; // want to make sure no race here

    //x = g->get();
    cilk_future_get_result(x, g);
    assert(x == 89);
    assert(s2 == 57);
    s2 = 42;
  
    assert_detected(0);

  }

  TEST_TEARDOWN();
  return 0;
}
