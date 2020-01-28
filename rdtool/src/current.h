#ifndef CURRENT_H
#define CURRENT_H

#include "list.h"
#include "om/blist.h"

class Current {
public:
  om_node *english;
  om_node *hebrew;
  List *list;
  //om_node *dag;
  uint32_t future_id;
  Current(): english(NULL), hebrew(NULL), list(NULL), future_id(0) {}
  Current(om_node *e, om_node *h, List *l, uint32_t id): english(e), hebrew(h), list(l), future_id(id) {}
};

#endif
