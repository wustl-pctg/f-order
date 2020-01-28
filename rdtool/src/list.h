#ifndef LIST_H
#define LIST_H

//#include "rdtool/src/om/om.h"
#include "om/om.h"
#include <pthread.h>
#include <cstdint>
//typedef unsigned int om_node_my;
typedef om_node* om_node_my; 

extern "C" bool om_precedes_my(om_node_my node1, om_node_my node2); 

struct Node {
    om_node_my cnode_e;
    om_node_my cnode_h;
    om_node_my desc_e;
    om_node_my desc_h;
    //size_t id;
    om_node_my dag;
    //uint32_t future_id;
    Node(om_node_my c_e, om_node_my c_h, om_node_my d_e, om_node_my d_h, om_node_my dag): cnode_e(c_e), cnode_h(c_h), desc_e(d_e), desc_h(d_h), dag(dag) {}
    //Node(om_node_my c_e, om_node_my c_h, om_node_my d_e, om_node_my d_h, om_node_my dag): cnode_e(c_e), cnode_h(c_h), desc_e(d_e), desc_h(d_h) {}
    Node() {}
};

struct HashElement {
    Node *group;
    int length;
    uint32_t future_id;
    HashElement *next;
};

struct Hash {
    int table_length;
    int buffer_length;
    int total;
    //int64_t start;
    HashElement *table;
    HashElement *head;
};

class List {
public:
    Node *nodes;
    int size_;
    pthread_spinlock_t sync_lock;
    
    int64_t start;
    int64_t end;

    Hash hash;

    //unsigned int mod;

    List();
    List(const Node &node, uint32_t future_id);
    List(const List &l1, const List&l2);
    List(const List &l, const Node &n, uint32_t future_id);

    void lock();
    void unlock();
    void init_lock();
    void destroy_lock();

    bool Search(const om_node_my node_e, const om_node_my node_h, uint32_t future_id);
    void PrintList() const;
    bool Empty() const;
    void Merge(const List &ori); 
    ~List();
};

#define MAX_NUM_WORKERS 20

struct PrivateMemory {
    Node *mem;
    uint64_t current;
    uint64_t size;
    HashElement *hash;
    uint64_t current_h;
    uint64_t size_h;
};

class Memory {
private:
    Node *mems[MAX_NUM_WORKERS];
    HashElement *hash_mems[MAX_NUM_WORKERS];

    pthread_spinlock_t mem_lock;
    int index;
    uint64_t total_size;
    uint64_t size;
public:
    Memory();
    ~Memory();
    void init(int i, uint64_t size);
    PrivateMemory grab_mem();
    void destroy();
};



#endif
