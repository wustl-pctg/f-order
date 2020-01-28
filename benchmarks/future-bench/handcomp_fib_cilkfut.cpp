#include <cilk/cilk.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "ktiming.h"
#include "cilk/future.h"

#ifndef TIMES_TO_RUN
#define TIMES_TO_RUN 10 
#endif

int times_to_run = TIMES_TO_RUN;

//#define TEST_INTEROP_PRE_FUTURE_CREATE
//#define TEST_INTEROP_POST_FUTURE_CREATE
//#define TEST_INTEROP_MULTI_FUTURE

//#define FUTURE_AFTER_SYNC

/* 
 * fib 39: 63245986
 * fib 40: 102334155
 * fib 41: 165580141 
 * fib 42: 267914296
 */

int fib(int n);

//void noop() {}

int  __attribute__((noinline)) fib(int n) {
    int x;
    int y;

    if(n < 2) {
        return n;
    }

    cilk_spawn noop();

    cilk::future<int> x_fut = cilk::future<int>();

    cilk_future_create1(&x_fut, fib, n-1);
    y = fib(n-2);

    x = x_fut.get();
    cilk_sync;

    return x+y;
}

int __attribute__((noinline)) run(int n, uint64_t *running_time) {
    cilk_spawn noop();
    int res;
    clockmark_t begin, end; 

    for(int i = 0; i < times_to_run; i++) {
        begin = ktiming_getmark();

        res = fib(n);

        end = ktiming_getmark();
        running_time[i] = ktiming_diff_usec(&begin, &end);
    }

    cilk_sync;
    return res;
}

int main(int argc, char * args[]) {
    int n;

    if(argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: fib <n> [<times_to_run>]\n");
        exit(1);
    }
    
    n = atoi(args[1]);
    if (argc == 3) {
      times_to_run = atoi(args[2]);
    }

    uint64_t* running_time = (uint64_t*)malloc(times_to_run * sizeof(uint64_t));

    int res = cilk_spawn run(n, &running_time[0]);
//    cilkg_set_param("local stacks", "128");
//    cilkg_set_param("shared stacks", "128");

    cilk_sync;
    printf("Res: %d\n", res);

    if( times_to_run > 10 ) 
        print_runtime_summary(running_time, times_to_run); 
    else 
        print_runtime(running_time, times_to_run); 

    free(running_time);

    return 0;
}

