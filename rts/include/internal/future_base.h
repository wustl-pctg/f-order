#pragma once

#include <internal/abi.h>
#include "../cilk/rd_future.h"
#include "../cilk/cilktool.h"

extern "C" {
__cilkrts_deque_link* __cilkrts_get_deque_link();
void __cilkrts_set_deque_FOM(void *deque, void *FOM);
void __cilkrts_set_deque_SD(void *deque, void *SD);
}

class future_base {
protected:
    __cilkrts_deque_link head = {
        .d = NULL, .next = NULL
    };

    __cilkrts_deque_link *volatile tail = &head;
    void *FOM;

    inline void suspend_deque() {
        __cilkrts_deque_link *l = __cilkrts_get_deque_link();
        __cilkrts_deque_link *t = tail;
        bool susp = false;

        while (t != NULL) {
            l->next = t;
            if (__a_compare_exchange_n(&tail, &t, l, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                susp = true;
                break;
            }
            t = tail;
        }

        if (susp) {
            void *SD = CilkFutureGet(true, this->FOM);
            __cilkrts_set_deque_SD(__cilkrts_get_deque(), SD);
            __cilkrts_suspend_deque();
        } else {
            CilkFutureGet(false, this->FOM);
        }
    }

    inline void resume_deques() {
        this->FOM = CilkFutureFinish();
        __cilkrts_deque_link *t = __a_exchange_n(&tail, NULL, __ATOMIC_SEQ_CST);

        while (t->next != NULL) {
            __cilkrts_set_deque_FOM(t->d, this->FOM);
            __cilkrts_make_resumable(t->d);
            t = t->next;
        }

        if(t->d) {
            // This is the oldest deque, so likely has
            // the most potential. If our deque is empty,
            // resume it and delete our deque; else mark
            // the deque as resumable and continue on our
            // own deque.
            __cilkrts_set_deque_FOM(t->d, this->FOM);
            __cilkrts_resume_suspended(t->d, 2);
        }
    }

public:
    inline bool ready() {
        // We set the tail to NULL only when the
        // future completes
        return tail == NULL;
    }

    inline void reset() {
        tail = &head;
        head.next = NULL;
    }
};
