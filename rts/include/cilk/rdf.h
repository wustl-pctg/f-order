#ifndef RD_FUTURE_H
#define RD_FUTURE_H

#include <internal/abi.h>

/* Yifan: the runtime needs to interact with the tool via two types of pointers
 * 1: pointers point to shadow deques(SD), each deque in the runtime has a corresponding
 *    shadow deque maintained by the tool. Whenever a deque gets suspended, runtime needs to store the SD
 *    pointer with the deque.
 * 2: pointers point to FOM data structures, as described in the algorithm, each node has a FOM data 
 *    structure. 
*/

typedef enum StealType {
    kNormal = 0,//steal from an active deque
    kResume,    //resume a deque
    kSuspended, //steal from a suspended deque
    kResumable  //steal from a resumable deque
} StealType;

#ifdef __cplusplus
extern "C" {
#endif

void CilkFutureCreate(); //notify the tool that cilk_future_create has been entered
                                    //must be invoked before detaching the continuation

void* CilkFutureFinish();//invoke to notify the tool that current future has been finished
                                    //1. need to store the returned FOM pointer with corresponding 
                                    //future
                                    //2. need to store the returned FOM pointer with all the resumed deque

void* CilkFutureGet(bool suspend, void* FOM);//notify the tool that cilk_future_get has been called
                               //the first argument tells the tool whether the current deque gets suspended
                               //if not, pass the FOM pointer of that future via the second argument
                               //if the deque gets suspended, store the retured shadow deque pointer with suspended deque
  
void* CilkSteal(StealType type, void *shadow_deque, void *FOM, __cilkrts_worker* w, __cilkrts_worker* victim);
//notify the tool that the current worker successfully steal something(or resume some deque)
//the second parameter is a pointer points to the corresponding shadow deque structure;
//if resume a deque, pass the FOM pointer of that deque via the third parameter
//returns a SD pointer points to the new deque of the thief

void* CilkSuspend();//invoke to notify the tool that the current active deque is suspended
                               //store the returned SD pointer with the suspended deque

extern "C" bool __a_compare_exchange_n(__cilkrts_deque_link *volatile *volatile tail, __cilkrts_deque_link **t, __cilkrts_deque_link *l, bool b, int sm, int fm);


extern "C" __cilkrts_deque_link* __a_exchange_n(__cilkrts_deque_link *volatile *volatile tail, __cilkrts_deque_link *p, int m);


#ifdef __cplusplus
} // extern "C"
#endif


#endif
