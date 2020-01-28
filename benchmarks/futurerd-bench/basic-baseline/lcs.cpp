#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <cilk/cilktool.h>

#include <cmath>

#if !SERIAL
//#ifndef NO_FUTURES
  #include <cilk/future.h>
//#endif
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

#else

#define CILK_FUNC_PREAMBLE
#define CILK_FUNC_EPILOGUE

#endif

#include "../util/getoptions.hpp"
#include "../util/util.hpp"
#include "../util/ktiming.h"

#define SIZE_OF_ALPHABETS 4
#define COMPUTE_LCS 0

#define DIAGONAL 0
#define LEFT 1
#define UP   2

#ifndef TIMES_TO_RUN
#define TIMES_TO_RUN 10
#endif


int times_to_run = TIMES_TO_RUN;

static int base_case_log;
#define MIN_BASE_CASE 32
#define NUM_BLOCKS(n) (n >> base_case_log)
#define BLOCK_ALIGN(n) (((n + (1 << base_case_log)-1) >> base_case_log) << base_case_log)
#define BLOCK_IND_TO_IND(i) (i << base_case_log)

static inline int nearpow2(int x) { return 1 << (32 - __builtin_clz (x - 1)); }
static inline int ilog2(int x) { return 32 - __builtin_clz(x) - 1; }
static inline int max(int a, int b) { return (a < b) ? b : a; }

#ifdef SERIAL
static int wave_lcs_with_futures(int *stor, char *a, char *b, int n) {
  // solutions to LCS when not considering input b
  for(int i = 0; i < n; i++) { // vertical strip when j == 0
    stor[i*n] = 0;
  }
  // solutions to LCS when not considering input a
  for(int j = 1; j < n; j++) { // horizontal strip when i == 0
    stor[j] = 0;
  }

  for(int i = 1; i < n; i++) {
    for(int j = 1; j < n; j++) {
      if(a[i-1] == b[j-1]) {
        stor[i*n + j] = stor[(i-1)*n + j-1] + 1;
      } else {
        stor[i*n + j] = max(stor[(i-1)*n + j], stor[i*n + j-1]);
      }
    }
  }

  int leng = stor[n*(n-1) + n-1];
  return leng;
}
#endif

/**
 * stor : the storage for solutions to subproblems, where stor[i,j] stores the
 *        longest common subsequence of a[1,i], and b[1,j] (assume string index is
 *        1-based), so stor[0,*] = 0 because we are not considering a at all and
 *        stor[*,0] = 0 because we are not considering b at all.
 * a, b : input strings of size n-1
 * n    : length of input strings + 1
 **/
static int
simple_seq_solve(int* stor, int *where, char *a, char *b, char *res, int n) {

  // solutions to LCS when not considering input b
  for(int i = 0; i < n; i++) { // vertical strip when j == 0
    stor[i*n] = 0;
  }
  // solutions to LCS when not considering input a
  for(int j = 1; j < n; j++) { // horizontal strip when i == 0
    stor[j] = 0;
  }
  for(int i = 1; i < n; i++) {
    for(int j = 1; j < n; j++) {
      if(a[i-1] == b[j-1]) {
        stor[i*n + j] = stor[(i-1)*n + j-1] + 1;
        where[i*n+j] = DIAGONAL;
      } else {
        stor[i*n + j] = max(stor[(i-1)*n + j], stor[i*n + j-1]);
        if(stor[(i-1)*n + j] > stor[i*n + j-1]) {
          where[i*n+j] = UP;
        } else {
          where[i*n+j] = LEFT;
        }
      }
    }
  }

  int leng = stor[n*(n-1) + n-1];
#if COMPUTE_LCS
  int i = n-1, j = n-1, k = leng;
  res[k--] = '\0';

  while(k >= 0) {
    switch(where[i*n + j]) {
    case DIAGONAL:
      assert(a[i-1] == b[j-1]);
      res[k--] = a[i-1];
      i--;
      j--;
      break;
    case LEFT:
      j--;
      break;
    case UP:
      i--;
      break;
    }
  }
  assert(k<0);
#endif

  return leng;
}

static int process_lcs_tile(int *stor, char *a, char *b, int n, int iB, int jB) {

  //printf("Processing block (%d, %d)\n", iB, jB);

  int bSize = 1 << base_case_log;

  for(int i = 0; i < bSize; i++) {
    for(int j = 0; j < bSize; j++) {

      int i_ind = BLOCK_IND_TO_IND(iB) + i;
      int j_ind = BLOCK_IND_TO_IND(jB) + j;

      if(i_ind == 0 || j_ind == 0) {
        stor[i_ind*n + j_ind] = 0;
      } else {
        if(a[i_ind-1] == b[j_ind-1]) {
          stor[i_ind*n + j_ind] = stor[(i_ind-1)*n + j_ind-1] + 1;
        } else {
          stor[i_ind*n + j_ind] = max(stor[(i_ind-1)*n + j_ind],
                                      stor[i_ind*n + j_ind-1]);
        }
      }
    }
  }

  return 0;
}

/* Unused
   static int iter_lcs(int *stor, char *a, char *b, int n) {

   for(int iB = 0; iB < nBlocks; iB++) {
   for(int jB = 0; jB < nBlocks; jB++) {
   process_lcs_tile(stor, a, b, n, iB, jB);
   }
   }

   return stor[n*(n-1) + n-1];
   }
*/


#ifdef NO_FUTURES

void recursive_process_lcs_tiles(int *stor, char *a, char *b, int n, int ibase, int start, int end) {
    
    int count = end - start;
    int mid;

    while (count > 1) {
      mid = start + count/2;
      cilk_spawn recursive_process_lcs_tiles(stor, a, b, n, ibase, start, mid);
      start = mid;
      count = end - start;
    }

    int iB = ibase - start;
    process_lcs_tile(stor, a, b, n, iB, start);
}

int __attribute__((noinline)) wave_lcs_with_futures(int *stor, char *a, char *b, int n) {
  int nBlocks = NUM_BLOCKS(n);
  
  // walk the upper half of triangle, including the diagonal (we assume square NxN LCS)
  for(int wave_front = 0; wave_front < nBlocks; wave_front++) {
    //#pragma cilk grainsize = 1
    //cilk_for(int jB = 0; jB <= wave_front; jB++) {
    //  int iB = wave_front - jB;
    //  process_lcs_tile(stor, a, b, n, iB, jB);
    //}
    recursive_process_lcs_tiles(stor, a, b, n, wave_front, 0, wave_front+1);
  }

  // walk the lower half of triangle
  for(int wave_front = 1; wave_front < nBlocks; wave_front++) {
    int iBase = nBlocks + wave_front - 1;
    recursive_process_lcs_tiles(stor, a, b, n, iBase, wave_front, nBlocks);
    //#pragma cilk grainsize = 1
    //cilk_for(int jB = wave_front; jB < nBlocks; jB++) {
    //  int iB = iBase - jB;
    //  process_lcs_tile(stor, a, b, n, iB, jB);
    //}
  }

  return stor[n*(n-1) + n-1];
}

#endif

#ifdef STRUCTURED_FUTURES

int __attribute__((noinline)) wave_lcs_with_futures(int *stor, char *a, char *b, int n) {
  FORCE_CILK_FUNC;

  int nBlocks = NUM_BLOCKS(n);

  // create an array of future objects
  //auto farray = (cilk::future<int>*)
  //  malloc(sizeof(cilk::future<int>) * nBlocks * nBlocks);
  cilk::future<int>* farray = new cilk::future<int>[nBlocks*nBlocks];

  // walk the upper half of triangle, including the diagonal (we assume square NxN LCS)
  for(int wave_front = 0; wave_front < nBlocks; wave_front++) {
    for(int jB = 0; jB <= wave_front; jB++) {
      int iB = wave_front - jB;

      if(iB > 0) { farray[(iB-1)*nBlocks + jB].get(); } // up dependency

      // since we are walking the wavefront serially, no need to get
      // left dependency --- already gotten by previous square.

      //START_FIRST_FUTURE_SPAWN;
      //process_lcs_tile_helper(&farray[iB*nBlocks+jB], stor, a, b, n, iB, jB);
      //END_FUTURE_SPAWN;
      cilk_future_create1(&farray[iB*nBlocks+jB], process_lcs_tile, stor, a, b, n, iB, jB);
      //reasync_helper<int,int*,char*,char*,int,int,int>
      //  (&farray[iB*nBlocks+jB], process_lcs_tile, stor, a, b, n, iB, jB);

      // process_lcs_tile(stor, a, b, n, iB, jB);

    }
  }

  // walk the lower half of triangle
  for(int wave_front = 1; wave_front < nBlocks; wave_front++) {
    int iBase = nBlocks + wave_front - 1;
    for(int jB = wave_front; jB < nBlocks; jB++) {
      int iB = iBase - jB;
      // need to get both up and left dependencies for the last row,
      // but otherwise just the up dependency.

      if(iB == (nBlocks - 1) && jB > 0) // left dependency
        farray[iB*nBlocks + jB - 1].get();
      if(iB > 0) // up dependency
        farray[(iB-1)*nBlocks + jB].get();

      //START_FIRST_FUTURE_SPAWN;
      //process_lcs_tile_helper(&farray[iB*nBlocks+jB], stor, a, b, n, iB, jB);
      //END_FUTURE_SPAWN;
      cilk_future_create1(&farray[iB*nBlocks+jB], process_lcs_tile, stor, a, b, n, iB, jB);

      //reasync_helper<int,int*,char*,char*,int,int,int>
      //  (&farray[iB*nBlocks+jB], process_lcs_tile, stor, a, b, n, iB, jB);

      // process_lcs_tile(stor, a, b, n, iB, jB);

    }
  }
  // make sure the last square finishes before we move onto returning
  farray[nBlocks * nBlocks - 1].get();

  //free(farray);
  delete [] farray;

  cilk_sync;

  return stor[n*(n-1) + n-1];
}
#endif

#ifdef NONBLOCKING_FUTURES
static int process_lcs_tile_with_get(cilk::future<int> *farray, int *stor,
                                     char *a, char *b, int n, int iB, int jB) {

  int nBlocks = NUM_BLOCKS(n);

  if(jB > 0) { farray[iB*nBlocks + jB - 1].get(); } // left depedency
  if(iB > 0) { farray[(iB-1)*nBlocks + jB].get(); } // up dependency

  process_lcs_tile(stor, a, b, n, iB, jB);

  return 0;
}

static void recursive_wave_lcs_with_futures(int *stor, char *a, char *b, int n, int nBlocks, cilk::future<int>* farray, int start, int end) {
  int count = end - start;
  int mid;

  while (count > 1) {
    mid = start + count/2;
    //cilk_spawn recursive_wave_sw_with_futures(stor, a, b, n, nBlocks, farray, start, mid);
    cilk_spawn recursive_wave_lcs_with_futures(stor, a, b, n, nBlocks, farray, start, mid);
    start = mid;
    count = end - start;
  }

  for (int i = start; count--; i++) {
    int iB = i / nBlocks;
    int jB = i % nBlocks;

    cilk_future_create1(&farray[i], process_lcs_tile_with_get, farray, stor, a, b, n, iB, jB);
    //launch_lcs_gf(stor, a, b, n, nBlocks, farray, i);
  }
}

int __attribute__((noinline)) wave_lcs_with_futures(int *stor, char *a, char *b, int n) {
  FORCE_CILK_FUNC;

  int nBlocks = NUM_BLOCKS(n);
  int blocks = nBlocks * nBlocks;

  // create an array of future handles
  //auto farray = (cilk::future<int>*)
  //  malloc(sizeof(cilk::future<int>) * blocks);
  cilk::future<int> *farray = new cilk::future<int>[blocks];

  recursive_wave_lcs_with_futures(stor, a, b, n, nBlocks, farray, 0, blocks);


//#pragma cilk grainsize = 1
//  cilk_for (int i = 0; i < blocks; i++) {
//    if (myFlag) cilk_spawn noop();
//    int iB = i / nBlocks;
//    int jB = i % nBlocks;
//    cilk_future_create1(&farray[i], process_lcs_tile_with_get, farray, stor, a, b, n, iB, jB);
//  }

  // make sure the last square finishes before we move onto returning
  farray[blocks-1].get();

  //free(farray);
  delete [] farray;

  return stor[n*(n-1) + n-1];
}
#endif

static void do_check(int *stor1, char *a1, char *b1, int n, int result) {
  char *a2 = (char *)malloc(n * sizeof(char));
  char *b2 = (char *)malloc(n * sizeof(char));
  char *res = (char *)malloc(n * sizeof(char)); // for storing LCS
  int *stor2 = (int *) malloc(sizeof(int) * n * n);
  int *where = (int *) malloc(sizeof(int) * n * n);

  memcpy(a2, a1, n * sizeof(char));
  memcpy(b2, b1, n * sizeof(char));
  res[n-1] = '\0';

  int result2 = simple_seq_solve(stor2, where, a2, b2, res, n);
  assert(result2 == result);

  for(int i=0; i < n; i++) {
    for(int j=0; j < n; j++) {
      assert(stor1[i*n + j] == stor2[i*n + j]);
    }
  }

  fprintf(stderr, "Check passed.\n");

  free(a2);
  free(b2);
  free(stor2);
  free(res);
  free(where);
}

const char* specifiers[] = {"-n", "-c", "-h", "-b", "-nruns", 0};
int opt_types[] = {INTARG, BOOLARG, BOOLARG, INTARG, INTARG, 0};

void run(int *&stor1, uint64_t* running_time, int i, char *a1, char *b1, int n, int &result) {
    FORCE_CILK_FUNC;
    printf("%d\n", i);
    stor1 = (int *) malloc(sizeof(int) * n * n);
    __asm__ volatile ("" ::: "memory");
    auto start = ktiming_getmark();
    result = wave_lcs_with_futures(stor1, a1, b1, n);
    auto end = ktiming_getmark();
    __asm__ volatile ("" ::: "memory");
    running_time[i] = ktiming_diff_usec(&start, &end);
    if (i < times_to_run-1) {
        free(stor1);
    }
}

int main(int argc, char *argv[]) {
//#if REACH_MAint && (!RACE_DETECT)
//  futurerd_disable_shadowing();
//#endif

  //cilk_spawn noop();

  int n = 1024;
  int bSize = 0;
  int check = 0, help = 0;

  get_options(argc, argv, specifiers, opt_types, &n, &check, &help, &bSize, &times_to_run);

  if(help) {
    fprintf(stderr, "Usage: lcs [-n size] [-b base_size] [-c] [-h]\n");
    fprintf(stderr, "\twhere -n specifies string length (default: 1024)\n");
    fprintf(stderr, "\t -b specifies base case size (default: sqrt(n))");
    fprintf(stderr, "\tcheck results if -c is set\n");
    fprintf(stderr, "\toutput this message if -h is set\n");
    exit(1);
  }


  //ensure_serial_execution();

  if (bSize == 0) {
    bSize = std::sqrt(n);

    // Minimum base case size, but only if not set explicitly
    if (bSize < MIN_BASE_CASE) bSize = MIN_BASE_CASE;
  }
  // Nearest power of 2
  bSize = nearpow2(bSize);
  base_case_log = ilog2(bSize);

  n = BLOCK_ALIGN(n); // make sure it's divisible by base case
  assert(n % bSize == 0);

  // Must be less than sqrt(2**31 - 1)
  if (n > 46340) {
    fprintf(stderr, "n must be less than 46340! (n was rounded to %d)\n", n);
    exit(1);
  }

  printf("Compute LCS with %d x %d table.\n", n, n);

  // str len is n-1, but allocated n to have the last char be \0
  char *a1 = (char *)malloc(n * sizeof(char));
  char *b1 = (char *)malloc(n * sizeof(char));
  int result = 0;

  /* Generate random inputs; a/b[n-1] not used */
  gen_rand_string(a1, n-1, SIZE_OF_ALPHABETS);
  gen_rand_string(b1, n-1, SIZE_OF_ALPHABETS);
  a1[n-1] = '\0';
  b1[n-1] = '\0';

#if SERIAL
  printf("Performing LCS serially.\n");
#elif defined(NONBLOCKING_FUTURES)
  printf("Performing LCS with non-structured futures and %d x %d base case.\n",
         bSize, bSize);
#elif defined(STRUCTURED_FUTURES) // STRUCTURED_FUTURES
  printf("Performing LCS with structured futures and %d x %d base case.\n",
         bSize, bSize);
#else
  printf("Performing LCS with cilk_for and %d x %d base case.\n",
         bSize, bSize);
#endif

  //__cilkrts_init();
  uint64_t running_time[times_to_run];

  int *stor1;// = (int *) malloc(sizeof(int) * n * n);
  for (int i = 0; i < times_to_run; i++) {
    //cilk_tool_init();
    run(stor1, running_time, i, a1, b1, n, result);
    //cilk_tool_destroy();
  }
  if(check) { do_check(stor1, a1, b1, n, result); }

  printf("Result: %d\n", result);
  //auto time = std::chrono::duration <double, std::milli> (end-start).count();
  //printf("Benchmark time: %f ms\n", time);
  if( times_to_run > 10 ) 
      print_runtime_summary(running_time, times_to_run); 
  else 
      print_runtime(running_time, times_to_run); 

  free(a1);
  free(b1);
  free(stor1);

  return 0;
}
