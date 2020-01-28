#include "common.hpp"
// #include "blist.hpp"
// #include "tlist.hpp"

// this file should *only* define the interface

namespace om {

class two_list {
private:
  using top_level_fake = void;
  using bot_level_fake = void;

  top_level_fake *m_tl;
  bot_level_fake *m_first_bl;

public:
  using node = void;

  two_list();
  ~two_list() {}  // not yet necessary
  node* insert(node* base);
  static bool precedes(const node* x, const node* y);

  node* first() const;
}; // class two_list


} // namespace om
