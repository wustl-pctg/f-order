#include "list.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include "ktiming.h"

volatile double total_time_merge = 0;
volatile double total_time_list  = 0;
extern Memory g_memory;
extern __thread PrivateMemory p_memory;

List::List() {
    //nodes = NULL;
    //size_ = 0;
    //start = 0;
    //end   = -1;
    hash.table_length = 0;
    hash.buffer_length = 0;
    hash.total = 0;
    hash.table = NULL;
    hash.head  = NULL;
    //std::cout << "create empty hash" << std::endl;
}

List::List(const Node &node, uint32_t future_id) {
    assert(p_memory.current < p_memory.size);
    assert(p_memory.current_h < p_memory.size_h);
    //size_ = 1;
    //p_memory.mem[p_memory.current] = node;
    //start = p_memory.current;
    //end   = start;
    //p_memory.current++;
    //nodes = p_memory.mem;
    //std::cout << "current old: " << p_memory.current_h << std::endl;
    hash.table_length  = 1;
    hash.buffer_length = 0;
    hash.total = 1;
    hash.table = p_memory.hash + p_memory.current_h;
    hash.table[0].future_id = future_id;
    hash.table[0].next = NULL;
    //hash.table[0].start = 0;
    p_memory.current_h++;
    //std::cout << "single element hash: table_length: " << hash.table_length << " buffer_length: " << hash.buffer_length
    //    << " total: " << hash.total << " current new: " << p_memory.current_h << std::endl;

    hash.table[0].group = p_memory.mem + p_memory.current;
    hash.table[0].length = 1;

    hash.table[0].group[0] = node;

    p_memory.current++;
}

void List::lock() {
    pthread_spin_lock(&sync_lock);
}

void List::unlock() {
    pthread_spin_unlock(&sync_lock);
}

void List::init_lock() {
    pthread_spin_init(&sync_lock, PTHREAD_PROCESS_PRIVATE);
}

void List::destroy_lock() {
    pthread_spin_destroy(&sync_lock); 
}

List::~List() {
    nodes = NULL;
    size_ = 0;
    start = 0;
    end   = -1;
}

size_t total_length = 0;
size_t total_iter = 0;
size_t total_search = 0;
size_t small = 0;
size_t large = 0;

bool List::Search(const om_node_my node_e, const om_node_my node_h, uint32_t future_id) {
    //printf("list size: %d\n", end - start + 1);

    if (Empty()) return false; 
    
    Node *group = NULL;
    int length = 0;

    int addr = future_id % hash.table_length;
    
    if (hash.table[addr].group != NULL && hash.table[addr].future_id == future_id) {
        group = hash.table[addr].group;
        length = hash.table[addr].length;
    } else if (hash.table[addr].group != NULL) {
        int low = 0;
        int high = hash.buffer_length - 1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (hash.table[hash.table_length + mid].future_id < future_id) {
                low = mid + 1;
            } else if (hash.table[hash.table_length + mid].future_id == future_id) {
                group = hash.table[hash.table_length + mid].group;
                length = hash.table[hash.table_length + mid].length;
                //std::cout << "found" << std::endl;
                break;
            } else {
                high = mid - 1;
            }
        }
        //for (int i = 0; i < hash.buffer_length; i++) {
        //    if (hash.table[hash.table_length + i].future_id == future_id) {
        //        group = hash.table[hash.table_length + i].group;
        //        length = hash.table[hash.table_length + i].length;
        //        break;
        //    }
        //}
    }

    if (group == NULL) return false;
    
    //total_search++;
    //if (length == 1) small++;
    //if (length > 1) large++;
    //total_length += length;
    //return false;

    int iter = 0;
    //total_length += (end - start + 1);
    int low = 0;
    int high = length - 1;
    while (low <= high) {
        //iter++;
        int mid = low + (high - low) / 2;
        Node n = group[mid];
        bool e_prec = om_precedes(node_e, n.cnode_e);
        bool h_prec = om_precedes(node_h, n.cnode_h);
        if (e_prec && h_prec) {
            //printf("result: true: mid: %d\n", start + mid);
            //total_iter += iter;
            return true;
        } else if (!e_prec) {
            low = mid + 1;
        } else {
            if (om_precedes(node_e, n.desc_e) && om_precedes(node_h, n.desc_h)) {
                //printf("result: true: mid: %d\n", start + mid);
                //total_iter += iter;
                return true;
            }
            high = mid - 1;
        }
        //if (low <= high) {
        //    mid = low + (high - low) / 2;
        //    n = nodes[mid];
        //}
        
    }
    //printf("result: false: iter: %d\n", iter);
    //total_iter += iter;
    return false;
}

int group_merge(Node *group1, Node *group2, int length1, int length2, Node *new_group) {
    int i = 0;
    int j = 0;
    int k = 0;
    //std::cout << "group merge group 1" << std::endl;
    //for (int l = 0; l < length1; l++) {
    //    std::cout << static_cast<const void *>(group1[l].cnode_e) << std::endl; 
    //}
    //std::cout << "group merge group 2" << std::endl; 
    //for (int l = 0; l < length2; l++) {
    //    std::cout << static_cast<const void *>(group2[l].cnode_e) << std::endl; 
    //}
    while (i < length1 && j < length2) {
        Node *n1 = &group1[i];
        Node *n2 = &group2[j];
        if (om_precedes(n1->cnode_e, n2->cnode_e)) {
            new_group[k] = *n1;
            i++;
        } else if (n1->cnode_e == n2->cnode_e) {
            bool prec = om_precedes(n1->desc_e, n2->desc_e) && om_precedes(n1->desc_h, n2->desc_h);
            if (!prec) {
                new_group[k] = *n1;
            } else {
                new_group[k] = *n2;
            }
            i++;
            j++;
        } else {
            new_group[k] = *n2;
            j++;
        }
        k++;
    }

    while (i < length1) new_group[k++] = group1[i++]; //set_list(k++, ori1->list(i++));
    while (j < length2) new_group[k++] = group2[j++]; //set_list(k++, ori2->list(j++));

    //std::cout << "group merge group 3" << std::endl; 
    //for (int l = 0; l < k; l++) {
    //    std::cout << static_cast<const void *>(new_group[l].cnode_e) << std::endl; 
    //}

    return k;
}

void List::Merge(const List &ori) {
    Hash hash_old = hash; 
    assert(p_memory.current_h < p_memory.size_h);
    assert(p_memory.current < p_memory.size);

    //std::cout << "current old: " << p_memory.current_h << std::endl; 
    
    //std::cout << "merge: table_length: " << hash.table_length << " buffer_length: " << hash.buffer_length
    //  << " total: " << hash.total << std::endl;
    
    //std::cout << "merge: table_length: " << ori.hash.table_length << " buffer_length: " << ori.hash.buffer_length
    //  << " total: " << ori.hash.total << std::endl;


    hash.table_length = (hash_old.total + ori.hash.total) * 2;
    hash.buffer_length = 0;
    hash.total = 0;
    hash.table = p_memory.hash + p_memory.current_h;

    for (int i = 0; i < hash.table_length; i++) {
        hash.table[i].group = NULL;
    }

    HashElement *p_1 = hash_old.head;
    HashElement *p_2 = ori.hash.head;
    HashElement *pre = NULL;

    while (p_1 != NULL && p_2 != NULL) {
        uint32_t id = 0;
        Node *group = NULL;
        int length = 0;
        if (p_1->future_id < p_2->future_id) {
            id = p_1->future_id;
            group = p_1->group;
            length = p_1->length;
            p_1 = p_1->next;
        } else if (p_1->future_id == p_2->future_id) {
            id = p_1->future_id;
            group = p_memory.mem + p_memory.current;
            length = group_merge(p_1->group, p_2->group, p_1->length, p_2->length, group);
            p_memory.current += length;
            
            p_1 = p_1->next;
            p_2 = p_2->next;
        } else {
            id = p_2->future_id;
            group = p_2->group;
            length = p_2->length;
            p_2 = p_2->next;
        }
        
        int addr = id % hash.table_length;
        
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = group;
        hash.table[addr].length = length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
    }

    while (p_1 != NULL) {
        uint32_t id = p_1->future_id;
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = p_1->group;
        hash.table[addr].length = p_1->length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
        p_1 = p_1->next;
    }
    
    while (p_2 != NULL) {
        uint32_t id = p_2->future_id;
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = p_2->group;
        hash.table[addr].length = p_2->length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
        p_2 = p_2->next;
    }

    p_memory.current_h += hash.table_length;
    p_memory.current_h += hash.buffer_length;
   
    //std::cout << "merge: table_length: " << hash.table_length << " buffer_length: " << hash.buffer_length
    //  << " total: " << hash.total << " current new: " << p_memory.current_h << std::endl;

    if (hash.total == 114464) {
        std::cout << "print hash table" << std::endl;
        for (int i = 0; i < hash.table_length; i++) {
            if (hash.table[i].group != NULL) {
                std::cout << hash.table[i].future_id << std::endl;
                std::cout << "length: " << hash.table[i].length << std::endl;
                //if (hash.table[i].length == 30) {
                //    for (int j = 0; j < hash.table[i].length; j++)
                //        std::cout << static_cast<const void *>(hash.table[i].group[j].cnode_e) << std::endl;
                //}
            }
        }
        std::cout << std::endl;
    }
}


bool List::Empty() const {
    return (hash.total == 0);
}

void List::PrintList() const {
}

List::List(const List &l1, const List &l2) {
    //clockmark_t t_begin, t_end;
    //t_begin = ktiming_getmark();
    
    assert(p_memory.current < p_memory.size);
    assert(p_memory.current_h < p_memory.size_h);

    //std::cout << "current old: " << p_memory.current_h << std::endl; 
    
    //std::cout << "list two: table_length: " << l1.hash.table_length << " buffer_length: " << l1.hash.buffer_length
    //    << " total: " << l1.hash.total << std::endl;

    //std::cout << "list two: table_length: " << l2.hash.table_length << " buffer_length: " << l2.hash.buffer_length
    //    << " total: " << l2.hash.total << std::endl;

    
    hash.table_length = (l1.hash.total + l2.hash.total) * 2;
    hash.buffer_length = 0;
    hash.total = 0;
    hash.table = p_memory.hash + p_memory.current_h;

    for (int i = 0; i < hash.table_length; i++) {
        hash.table[i].group = NULL;
    }

    HashElement *p_1 = l1.hash.head;
    HashElement *p_2 = l2.hash.head;
    HashElement *pre = NULL;

    while (p_1 != NULL && p_2 != NULL) {
        uint32_t id = 0;
        Node *group = NULL;
        int length = 0;
        if (p_1->future_id < p_2->future_id) {
            id = p_1->future_id;
            group = p_1->group;
            length = p_1->length;
            p_1 = p_1->next;
        } else if (p_1->future_id == p_2->future_id) {
            id = p_1->future_id;
            
            group = p_memory.mem + p_memory.current;
            length = group_merge(p_1->group, p_2->group, p_1->length, p_2->length, group);
            p_memory.current += length;

            p_1 = p_1->next;
            p_2 = p_2->next;
        } else {
            id = p_2->future_id;
            group = p_2->group;
            length = p_2->length;
            p_2 = p_2->next;
        }
        
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = group;
        hash.table[addr].length = length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
    }

    while (p_1 != NULL) {
        uint32_t id = p_1->future_id;
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = p_1->group;
        hash.table[addr].length = p_1->length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
        p_1 = p_1->next;
    }
    
    while (p_2 != NULL) {
        uint32_t id = p_2->future_id;
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = p_2->group;
        hash.table[addr].length = p_2->length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
        p_2 = p_2->next;
    }

    p_memory.current_h += hash.table_length;
    p_memory.current_h += hash.buffer_length;

    //std::cout << "list two: table_length: " << hash.table_length << " buffer_length: " << hash.buffer_length
    //    << " total: " << hash.total << " current new: " << p_memory.current_h << std::endl;

    
    /*int64_t i = l1.start;
    int64_t j = l2.start;
    start = p_memory.current;
    end   = start;
    while (i <= l1.end && j <= l2.end) {
        Node *n1 = &l1.nodes[i];
        Node *n2 = &l2.nodes[j];
        if (om_precedes(n1->dag, n2->dag)) {
            p_memory.mem[end] = *n1;
            i++;
        } else if (n1->dag == n2->dag) {
            if (om_precedes(n1->cnode_e, n2->cnode_e)) {
                p_memory.mem[end] = *n1;
                i++;
            } else if (n1->cnode_e == n2->cnode_e) {
                bool prec = om_precedes(n1->desc_e, n2->desc_e) && om_precedes(n1->desc_h, n2->desc_h);
                if (!prec) {
                    p_memory.mem[end] = *n1;
                } else {
                    p_memory.mem[end] = *n2;
                }
                i++;
                j++;
            } else {
                p_memory.mem[end] = *n2;
                j++;
            }
        } else {
            p_memory.mem[end] = *n2;
            j++;
        }
        end++;
    }

    while (i <= l1.end) p_memory.mem[end++] = l1.nodes[i++]; //set_list(k++, ori1->list(i++));
    while (j <= l2.end) p_memory.mem[end++] = l2.nodes[j++]; //set_list(k++, ori2->list(j++));
    
    p_memory.current = end;
    end--;
    nodes = p_memory.mem;
    t_end = ktiming_getmark();
    double t_list = ktiming_diff_sec(&t_begin, &t_end);
    total_time_list += t_list;*/
}

int merge_group_single(Node *group, int length, Node *new_group, const Node &n) {
    int i = 0;
    int j = 0;
    while (i < length) {
        Node *n_l = &group[i];
        if (om_precedes(n_l->cnode_e, n.cnode_e)) {
            new_group[j] = *n_l;
            if (om_precedes(n_l->desc_e, n.cnode_e), om_precedes(n_l->desc_h, n.cnode_h)) {
                new_group[j].desc_e = n.cnode_e;
                new_group[j].desc_h = n.cnode_h;
            }
            i++;
            j++;
        } else {
            new_group[j] = n;
            j++;
            break;
        }
    }
    if (i == length) new_group[j++] = n;
    while (i < length) new_group[j++] = group[i++]; //set_list(k++, ori2->list(j++));
    return j;
}

List::List(const List &l, const Node &n, uint32_t future_id) { 
    //clockmark_t t_begin, t_end;
    //t_begin = ktiming_getmark();
    //size_ = l1.size_ + l2.size_;
    //nodes = new Node[size_];
    //std::cout << "current old: " << p_memory.current_h << std::endl; 
  
    //std::cout << "single + list: table_length: " << l.hash.table_length << " buffer_length: " << l.hash.buffer_length
    //    << " total: " << l.hash.total << std::endl;

    assert(p_memory.current_h < p_memory.size_h);
    hash.table_length = (l.hash.total + 1) * 2;
    hash.buffer_length = 0;
    hash.total = 0;
    hash.table = p_memory.hash + p_memory.current_h;

    for (int i = 0; i < hash.table_length; i++) {
        hash.table[i].group = NULL;
    }

    HashElement *p = l.hash.head;
    HashElement *pre = NULL;
    bool inserted = false;
    while (p != NULL) {
        uint32_t id = 0;
        Node *group = NULL;
        int length = 0;
        if (p->future_id < future_id) {
            id = p->future_id;
            group = p->group;
            length = p->length;
            p = p->next;
        } else if (p->future_id == future_id) {
            id = p->future_id;

            group = p_memory.mem + p_memory.current;
            length = merge_group_single(p->group, p->length, group, n);
            p_memory.current += length;

            p = p->next;
            inserted = true;
        } else {
            if (inserted) {
                id = p->future_id;

                group = p->group;
                length = p->length;

                p = p->next;
            } else {
                id = future_id;
                inserted = true;

                group = p_memory.mem + p_memory.current;
                length = 1;
                p_memory.current += length;
                group[0] = n;
            }
        }
       
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].group = group;
        hash.table[addr].length = length;
        hash.table[addr].next  = NULL;
        
        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
    }

    if (!inserted) {
        uint32_t id = future_id;
        int addr = id % hash.table_length;
        if (hash.table[addr].group != NULL) {
            addr = hash.table_length + hash.buffer_length;
            hash.buffer_length++;
        }

        hash.table[addr].future_id = id;
        //hash.table[addr].start = 0;
        hash.table[addr].next  = NULL;
        hash.table[addr].group = p_memory.mem + p_memory.current;
        hash.table[addr].length = 1;
        p_memory.current += 1;
        hash.table[addr].group[0] = n;

        if (pre != NULL) pre->next = &(hash.table[addr]);
        else hash.head = &(hash.table[addr]);
        
        pre = &(hash.table[addr]);

        hash.total++;
    }
                
    p_memory.current_h += hash.table_length;
    p_memory.current_h += hash.buffer_length;

    //std::cout << "single + list: table_length: " << hash.table_length << " buffer_length: " << hash.buffer_length
    //    << " total: " << hash.total << " current new: " << p_memory.current_h << std::endl;

/*
    assert(p_memory.current < p_memory.size);
    //int i = 0, j = 0, k = 0;
    int64_t i = l.start;
    start = p_memory.current;
    end   = start;
    while (i <= l.end) {
        Node *n_l = &l.nodes[i];
        if (om_precedes(n_l->dag, n.dag)) {
            p_memory.mem[end] = *n_l;
            i++;
            end++;
        } else if (n_l->dag == n.dag) {
            if (om_precedes(n_l->cnode_e, n.cnode_e)) {
                p_memory.mem[end] = *n_l;
                if (om_precedes(n_l->desc_e, n.cnode_e), om_precedes(n_l->desc_h, n.cnode_h)) {
                    p_memory.mem[end].desc_e = n.cnode_e;
                    p_memory.mem[end].desc_h = n.cnode_h;
                }
                i++;
                end++;
            } else {
                p_memory.mem[end] = n;
                end++;
                break;
            }
        } else {
            p_memory.mem[end] = n;
            end++;
            break;
        }
    }
    if (i > l.end) p_memory.mem[end++] = n;
    while (i <= l.end) p_memory.mem[end++] = l.nodes[i++]; //set_list(k++, ori2->list(j++));
    //size_ = k;
    p_memory.current = end;
    end--;
    nodes = p_memory.mem;
    t_end = ktiming_getmark();
    double t_list = ktiming_diff_sec(&t_begin, &t_end);
    total_time_list += t_list;*/
}

Memory::Memory() {
    pthread_spin_init(&mem_lock, PTHREAD_PROCESS_PRIVATE);
}

Memory::~Memory() {
    pthread_spin_destroy(&mem_lock);
}

void Memory::destroy() {
    delete[] mems[0];
}

void Memory::init(int n, uint64_t s) {
    index = 0;
    size = s;
    total_size = n * s;
    mems[0] = new Node[total_size];
    hash_mems[0] = new HashElement[total_size * 2];
    
    for (uint64_t i = 0; i < total_size; i += 100) {
        mems[0][i].cnode_e = (om_node*)1024;
        mems[0][i].cnode_h = (om_node*)1024;
        mems[0][i].desc_e  = (om_node*)1024;
        mems[0][i].desc_h  = (om_node*)1024;
        //mems[0][i].dag     = (om_node*)1024;
        //hash_mems[0][i].length = 0;
        //hash_mems[0][i].future_id = 0;
    }

    for (uint64_t i = 0; i < total_size * 2; i += 100) {
        hash_mems[0][i].length = 0;
        hash_mems[0][i].future_id = 0;
    }

    for (int i = 1; i < n; i++) {
        mems[i] = mems[0] + i * size;
        hash_mems[i] = hash_mems[0] + i * size * 2;
    }
}

PrivateMemory Memory::grab_mem() {
    PrivateMemory memory;
    
    pthread_spin_lock(&mem_lock);
    memory.mem = mems[index];
    memory.hash = hash_mems[index++];
    pthread_spin_unlock(&mem_lock);
    
    memory.current   = 0;
    memory.size      = size;
    memory.current_h = 0;
    memory.size_h    = size * 2;
    
    return memory;
}


extern "C" bool om_precedes_my(om_node_my node1, om_node_my node2) {
    //return node1 < node2;
    return om_precedes(node1, node2);
}

