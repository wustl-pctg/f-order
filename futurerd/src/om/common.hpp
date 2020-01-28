#pragma once
#include <climits>

namespace om {

typedef unsigned long label_t;
static constexpr label_t MAX_LABEL = ULONG_MAX;
static constexpr label_t MIN_LABEL = 0L;

class basic_node {
public:
  label_t label;
  basic_node* next = nullptr;
  basic_node(label_t lab) : label(lab) {}
}; // class basic_node

} // namespace om
