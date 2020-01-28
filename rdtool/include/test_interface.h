#ifndef _TEST_INTERFACE_H_
#define _TEST_INTERFACE_H_

#include "../src/om/om.h"
#include "../src/list.h"

struct Frame {
    om_node* english;
    om_node* hebrew;
    om_node* dag;
    List *list;
};


int ExternalTest();
void *ExternalRecordE();
void *ExternalRecordH();
Frame Record();
bool PrecedesOM(Frame a, Frame b);
bool PrecedesFuture(Frame a, Frame b);
bool ExternalPrecedes(void*, void*, void*, void*);
bool ExternalFuturePrecedes(void*, void*);

#endif
