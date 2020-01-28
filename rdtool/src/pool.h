#ifndef POOL_H
#define POOL_H

#include <pthread.h>
#include <stdlib.h>

template <typename T>
class Pool;

template <typename T>
struct ShadowDeque {
    ShadowDeque<T> *next;
    ShadowDeque<T> *pre;
    T *deque;
    Pool<T> *pool;
    ShadowDeque(): next(NULL), pre(NULL), deque(NULL), pool(NULL) {}
};

template <typename T>
class Pool {
private:
    ShadowDeque<T> *head;

public:
    pthread_spinlock_t pool_lock;
    T *active_deque_;

    Pool() {
        //pthread_spin_init(&pool_lock, PTHREAD_PROCESS_PRIVATE);
        //head = new ShadowDeque<T>();
        active_deque_ = new T();    
    }

    void Create() {
        active_deque_ = new T();
    }
    
    ~Pool() {
        //while (head != NULL) {
        //    ShadowDeque<T> *next = head->next;
        //    if (head->deque != NULL) delete head->deque;
        //    delete head;
        //    head = next;
        //}

        if (active_deque_ != NULL) delete active_deque_;
        //pthread_spin_destroy(&pool_lock);
    }

    T* RemoveFromPool(ShadowDeque<T> *shadow_deque) {
        pthread_spin_lock(&shadow_deque->pool->pool_lock);  
        T *ret = shadow_deque->deque;
        
        if (shadow_deque->next != NULL) shadow_deque->next->pre = shadow_deque->pre;
        shadow_deque->pre->next = shadow_deque->next;
        pthread_spin_unlock(&shadow_deque->pool->pool_lock);
        
        delete shadow_deque;
        return ret;
    }

    //ShadowDeque<T>* SuspendSelf() {
    T* SuspendSelf() {
        //ShadowDeque<T>* ret = Suspend(active_deque_);
        T* ret = active_deque_;
        active_deque_ = NULL;
        return ret;
    }

    ShadowDeque<T>* Suspend(T *deque) {
        ShadowDeque<T> *shadow_deque = new ShadowDeque<T>();
        pthread_spin_lock(&pool_lock);
        shadow_deque->deque = deque;
        shadow_deque->pool = this;
        
        shadow_deque->next = head->next;
        if (head->next != NULL) head->next->pre = shadow_deque;
        shadow_deque->pre = head;
        head->next = shadow_deque;
        
        pthread_spin_unlock(&pool_lock);
        return shadow_deque;
    }

    void FreeCurrentDeque() {
        if (active_deque_ != NULL) {
            delete active_deque_;
            active_deque_ = NULL;
        }
    }

    void NewDeque() {
        active_deque_ = new T();        
    }
};

#endif
