#include "pool.h"
#include "stdlib.h"

template <typename T> 
Pool<T>::Pool() {
    pthread_spin_init(&pool_lock, PTHREAD_PROCESS_PRIVATE);
    head = new Frame<T>();
    head->pre = head->next = head->deque = NULL;
    active_deque_ = new T();    
}

template <typename T> 
Pool<T>::~Pool() {
    while (head != NULL) {
        Frame<T> *next = head->next;
        delete head;
        head = next;
    }

    if (active_deque_ != NULL) delete active_deque_;
    pthread_spin_destroy(&pool_lock);
}

template <typename T> 
T* Pool<T>::RemoveFromPool(Frame<T> *frame) {
    pthread_spin_lock(&pool_lock);  
    T *ret = frame->deque;
    if (frame->pre != NULL) {
        frame->pre->next = frame->next;
    } else {
        head = frame->next;
    }

    frame->next->pre = frame->pre;
    pthread_spin_unlock(&pool_lock);
    delete frame;
    return ret;
}

template <typename T> 
Frame<T>* Pool<T>::SuspendSelf() {
    pthread_spin_lock(&pool_lock);
    Frame<T>* ret = Suspend(active_deque_);
    active_deque_ = NULL;
    pthread_spin_unlock(&pool_lock);
    return ret;
}

template <typename T> 
Frame<T>* Pool<T>::Suspend(T *deque) {
    Frame<T> *frame = new Frame<T>();
    pthread_spin_lock(&pool_lock);
    frame->deque = deque;
    frame->pre = NULL;
    frame->next = head;
    head->pre = frame;
    head = frame;
    pthread_spin_unlock(&pool_lock);
    return frame;
}
