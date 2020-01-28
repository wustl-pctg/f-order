#ifndef SERIAL
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#else
#define cilk_spawn 
#define cilk_sync
#define __cilkrts_accum_timing()
#endif

#define __cilkrts_accum_timing()

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cilk/cilktool.h>
#include "getoptions.h"

#ifndef TIMING_COUNT
#define TIMING_COUNT 10
#endif

int timing_count = TIMING_COUNT;
//extern size_t number_of_future;
//extern volatile size_t g_relabel_id;
//extern volatile double total_time_merge;
//extern volatile double total_time_list;
//extern size_t total_readers;
//extern size_t total_writers;
//extern size_t total_length;
//extern size_t total_iter;
//extern size_t total_query;
//extern size_t total_nonsp_query;
//extern size_t total_search;
//extern size_t total_strands;
#if TIMING_COUNT
#include "ktiming.h"
#endif

#ifndef RAND_MAX
#define RAND_MAX 32767
#endif

#define ERR_THRESHOLD   (0.1)

#include "cilk/future.h"

#define REAL int
static int BASE_CASE; //the base case of the computation (2*POWER)
static int POWER; //the power of two the base case is based on
#define timing

//void noop() {}

static const unsigned int Q[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
static const unsigned int S[] = {1, 2, 4, 8};

unsigned long rand_nxt = 0;

int cilk_rand(void) {
    int result;
    rand_nxt = rand_nxt * 1103515245 + 12345;
    result = (rand_nxt >> 16) % ((unsigned int) RAND_MAX + 1);
    return result;
}


//provides a look up for the Morton Number of the z-order curve given the x and y coordinate
//every instance of an (x,y) lookup must use this function
unsigned int z_convert(int row, int col){

    unsigned int z; // z gets the resulting 32-bit Morton Number.  
    // x and y must initially be less than 65536.
    // The top and the left boundary 

    col = (col | (col << S[3])) & Q[3];
    col = (col | (col << S[2])) & Q[2];
    col = (col | (col << S[1])) & Q[1];
    col = (col | (col << S[0])) & Q[0];

    row = (row | (row << S[3])) & Q[3];
    row = (row | (row << S[2])) & Q[2];
    row = (row | (row << S[1])) & Q[1];
    row = (row | (row << S[0])) & Q[0];

    z = col | (row << 1);

    return z;
}

//converts (x,y) position in the array to the mixed z-order row major layout
int block_convert(int row, int col){
    int block_index = z_convert(row >> POWER, col >> POWER);
    return (block_index * BASE_CASE << POWER) 
        + ((row - ((row >> POWER) << POWER)) << POWER) 
        + (col - ((col >> POWER) << POWER));
}

//init the matric in order
void order(REAL *M, int n){
    int i,j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < n; j++){
            M[block_convert(i,j)] = i * n + j;   
        }
    }
}

//init the matric in order
void order_rm(REAL *M, int n){
    int i;
    for(i = 0; i < n * n; i++) {
        M[i] = i;
    }
}

//init the matrix to all ones 
void one(REAL *M, int n){
    int i;
    for(i = 0; i < n * n; i++) {
        M[i] = 1.0;
    }
}


//init the matrix to all zeros 
void zero(REAL *M, int n){
    int i;
    for(i = 0; i < n * n; i++) {
        M[i] = 0.0;
    }
}

//init the matrix to random numbers
void init(REAL *M, int n){
    int i,j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < n; j++){
            M[block_convert(i,j)] = (REAL) cilk_rand();
        }
    }
}

//init the matrix to random numbers
void init_rm(REAL *M, int n){
    int i;
    for(i = 0; i < n * n; i++) {
        M[i] = (REAL) cilk_rand();
    }
}

//prints the matrix
void print_matrix(REAL *M, int n){
    int i,j;
    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            printf("%6d ", M[block_convert(i,j)]);
        }
        printf("\n");
    }
}


//prints the matrix
void print_matrix_rm(REAL *M, int n, int orig_n){
    int i,j;
    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            printf("%6d ", M[i * orig_n + j]);
        }
        printf("\n");
    }
}

//itterative solution for matrix multiplication
void iter_matmul(REAL *A, REAL *B, REAL *C, int n){
    int i, j, k;

    for(i = 0; i < n; i++){
        for(k = 0; k < n; k++){
            REAL c = 0.0;
            for(j = 0; j < n; j++){
                c += A[block_convert(i,j)] * B[block_convert(j,k)];
            }
            C[block_convert(i, k)] = c;
        }
    }
}

//itterative solution for matrix multiplication
void iter_matmul_rm(REAL *A, REAL *B, REAL *C, int n){
    int i, j, k;

    for(i = 0; i < n; i++){
        for(k = 0; k < n; k++){
            REAL c = 0.0;
            for(j = 0; j < n; j++){
                c += A[i * n + j] * B[j * n + k];
            }
            C[i * n + k] = c;
        }
    }
}

//calculates the max error between the itterative solution and other solution
double maxerror(REAL *M1, REAL *M2, int n) {
    int i,j;
    double err = 0.0;

    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            double diff = (M1[block_convert(i,j)] - M2[block_convert(i,j)]) / M1[block_convert(i,j)];
            if(diff < 0) {
                diff = -diff;
            }
            if(diff > err) {
                err = diff;
            }
        }
    }

    return err;
}

//calculates the max error between the itterative solution and other solution
double maxerror_rm(REAL *M1, REAL *M2, int n) {
    int i,j;
    double err = 0.0;

    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            double diff = (M1[i * n + j] - M2[i * n + j]) / M1[i * n + j];
            if(diff < 0) {
                diff = -diff;
            }
            if(diff > err) {
                err = diff;
            }
        }
    }

    return err;
}

void mat_mul_par(const REAL *const A, const REAL *const B, REAL *C, cilk::future<void> *CReady, int n);

//recursive parallel solution to matrix multiplication
void mat_mul_par(const REAL *const A, const REAL *const B, REAL *C, cilk::future<void> *CReady, int n){

    if (CReady) {
      CReady->get();
      CReady = NULL;
    }
    //BASE CASE: here computation is switched to itterative matrix multiplication
    //At the base case A, B, and C point to row order matrices of n x n
    if(n == BASE_CASE) {
        int i, j, k;
        for(i = 0; i < n; i++){
            for(k = 0; k < n; k++){
                REAL c = 0.0;
                for(j = 0; j < n; j++){
                    c += A[i * n + j] * B[j* n + k];
                }
                C[i * n + k] += c;
            }
        }

        return;
    }

    cilk_spawn noop();

    //partition each matrix into 4 sub matrices
    //each sub-matrix points to the start of the z pattern
    const REAL *const A1 = &A[block_convert(0,0)];
    const REAL *const A2 = &A[block_convert(0, n >> 1)]; //bit shift to divide by 2
    const REAL *const A3 = &A[block_convert(n >> 1,0)];
    const REAL *const A4 = &A[block_convert(n >> 1, n >> 1)];

    const REAL *const B1 = &B[block_convert(0,0)];
    const REAL *const B2 = &B[block_convert(0, n >> 1)];
    const REAL *const B3 = &B[block_convert(n >> 1, 0)];
    const REAL *const B4 = &B[block_convert(n >> 1, n >> 1)];
    
    REAL *C1 = &C[block_convert(0,0)];
    REAL *C2 = &C[block_convert(0, n >> 1)];
    REAL *C3 = &C[block_convert(n >> 1,0)];
    REAL *C4 = &C[block_convert(n >> 1, n >> 1)];

    cilk::future<void> stages[4];// = new cilk::future<void>[4];
    //START_FIRST_FUTURE_SPAWN;
    //  mat_mul_par_fut_helper(&stages[0], A1, B1, C1, CReady, n>>1);
    //END_FUTURE_SPAWN;
    cilk_future_create1(&stages[0], mat_mul_par, A1, B1, C1, CReady, n>>1);

    //recursively call the sub-matrices for evaluation in parallel
    //cilk_spawn mat_mul_par(A1, B1, C1, n >> 1);

    //START_FUTURE_SPAWN;
    //  mat_mul_par_fut_helper(&stages[1], A1, B2, C2, CReady, n>>1);
    //END_FUTURE_SPAWN;
    cilk_future_create1(&stages[1], mat_mul_par, A1, B2, C2, CReady, n>>1);

    ///*cilk_spawn*/ mat_mul_par(A1, B2, C2, n >> 1);
    
    //START_FUTURE_SPAWN;
    //  mat_mul_par_fut_helper(&stages[2], A3, B1, C3, CReady, n>>1);
    //END_FUTURE_SPAWN;
    cilk_future_create1(&stages[2], mat_mul_par, A3, B1, C3, CReady, n>>1);
    

    ///*cilk_spawn*/ mat_mul_par(A3, B1, C3, n >> 1);
    
    //START_FUTURE_SPAWN;
    //  mat_mul_par_fut_helper(&stages[3], A3, B2, C4, CReady, n>>1);
    //END_FUTURE_SPAWN;
    cilk_future_create1(&stages[3], mat_mul_par, A3, B2, C4, CReady, n>>1);

    ///*cilk_spawn*/ mat_mul_par(A3, B2, C4, n >> 1);
    //cilk_sync; //wait here for first round to finish

    cilk_spawn mat_mul_par(A2, B3, C1, &stages[0], n>>1);
    ///*cilk_spawn*/ mat_mul_par(A2, B3, C1, n >> 1);
    
    cilk_spawn mat_mul_par(A2, B4, C2, &stages[1], n>>1);
    ///*cilk_spawn*/ mat_mul_par(A2, B4, C2, n >> 1);
    
    cilk_spawn mat_mul_par(A4, B3, C3, &stages[2], n>>1);
    ///*cilk_spawn*/ mat_mul_par(A4, B3, C3, n >> 1);
    
    //if (!CILK_SETJMP(sf.ctx)) {
    mat_mul_par(A4, B4, C4, &stages[3], n>>1);
    //}

    //delete [] stages;
}

//recursive parallel solution to matrix multiplication - row major order
void mat_mul_par_rm(REAL *A, REAL *B, REAL *C, int n, int orig_n){
    //BASE CASE: here computation is switched to itterative matrix multiplication
    //At the base case A, B, and C point to row order matrices of n x n
    if(n == BASE_CASE) {
        int i, j, k;
        for(i = 0; i < n; i++){
            for(k = 0; k < n; k++){
                REAL c = 0.0;
                for(j = 0; j < n; j++){
                    c += A[i * orig_n + j] * B[j * orig_n + k];
                }
                C[i * orig_n + k] += c;
            }
        }
        return;
    }


    //partition each matrix into 4 sub matrices
    //each sub-matrix points to the start of the z pattern
    REAL *A1 = &A[0];
    REAL *A2 = &A[n >> 1]; //bit shift to divide by 2
    REAL *A3 = &A[(n * orig_n) >> 1];
    REAL *A4 = &A[((n * orig_n) + n) >> 1];

    REAL *B1 = &B[0];
    REAL *B2 = &B[n >> 1];
    REAL *B3 = &B[(n * orig_n) >> 1];
    REAL *B4 = &B[((n * orig_n) + n) >> 1];
    
    REAL *C1 = &C[0];
    REAL *C2 = &C[n >> 1];
    REAL *C3 = &C[(n * orig_n) >> 1];
    REAL *C4 = &C[((n * orig_n) + n) >> 1];

    //recursively call the sub-matrices for evaluation in parallel
    /*cilk_spawn*/ mat_mul_par_rm(A1, B1, C1, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A1, B2, C2, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A3, B1, C3, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A3, B2, C4, n >> 1, orig_n);
    /*cilk_sync*/; //wait here for first round to finish

    /*cilk_spawn*/ mat_mul_par_rm(A2, B3, C1, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A2, B4, C2, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A4, B3, C3, n >> 1, orig_n);
    /*cilk_spawn*/ mat_mul_par_rm(A4, B4, C4, n >> 1, orig_n);
    /*cilk_sync*/; //wait here for all second round to finish

}

const char *specifiers[] = {"-n", "-c", "-rc", "-h", "-nruns", 0};
int opt_types[] = {INTARG, BOOLARG, BOOLARG, BOOLARG, INTARG, 0};

void check_result(REAL* A, REAL* B, REAL* C, int n) {
  REAL *ans = (REAL *) malloc(n*n*sizeof(REAL));

  iter_matmul(A, B, ans, n);
  double err = maxerror(C, ans, n);
  if (err >= ERR_THRESHOLD) printf("INCORRECT RESULT\n");
}

int main(int argc, char *argv[]) {
    int n = 2048; //default n value
    POWER = 6; //default k value
    BASE_CASE = (int) pow(2.0, (double) POWER);

    int check = 0, rand_check = 0, help = 0; // default options
    get_options(argc, argv, specifiers, opt_types, 
                      &n, &check, &rand_check, &help, &timing_count);

  if(help) {
      fprintf(stderr, 
                  "Usage: matmul [-n size] [-c] [-rc] [-h] [<cilk options>]\n");
      fprintf(stderr, "if -c is set, "
                  "check result against iterative matrix multiply O(n^3).\n");
      fprintf(stderr, "if -rc is set, check "
                  "result against randomlized algo. due to Freivalds O(n^2).\n");
      exit(1);
    }


    REAL *A, *B, *C;

    A = (REAL *) malloc(n * n * sizeof(REAL)); //source matrix 
    B = (REAL *) malloc(n * n * sizeof(REAL)); //source matrix
    C = (REAL *) malloc(n * n * sizeof(REAL)); //result matrix
   
#if TIMING_COUNT
  clockmark_t begin, end;
  uint64_t elapsed[timing_count];
  for(int i=0; i < timing_count; i++) {
    cilk_tool_init();
    __cilkrts_set_param("local stacks", "256");
    init_rm(A, n);
    init_rm(B, n);
    zero(C, n);
    begin= ktiming_getmark();
    mat_mul_par(A, B, C, NULL, n); 
    end = ktiming_getmark();
    elapsed[i] = ktiming_diff_usec(&begin, &end);
    cilk_tool_destroy();
  }
  if (timing_count > 10)
    print_runtime_summary(elapsed, timing_count);
  else
    print_runtime(elapsed, timing_count);
#else
  init_rm(A, n);
  init_rm(B, n);
  zero(C, n);
  mat_mul_par(A, B, C, n); 
#endif
__cilkrts_accum_timing();

  if (check) check_result(A, B, C, n);

    //clean up memory
  delete [] A;
  delete [] B;
  delete [] C;
    //delete [] I;
  
  //printf("# of futures: %zu\n", number_of_future);
  //printf("done, trigger relabel: %zu\n", g_relabel_id); 
  //printf("total time merge: %f\n", total_time_merge);
  //printf("total time list: %f\n", total_time_list);
  //printf("writer: %zu\n", total_writers);
  //printf("reader: %zu\n", total_readers);
  //printf("total query: %zu\n", total_query);
  //printf("total nonsp query: %zu\n", total_nonsp_query);
  //printf("total length: %zu\n", total_length);
  //printf("total iter: %zu\n", total_iter);
  //printf("total search: %zu\n", total_search);
  //printf("total strand: %zu\n", total_strands);
  return 0;
}
