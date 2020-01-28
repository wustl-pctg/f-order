#include "two_list.hpp"
#include "blist.hpp"
#include "tlist.hpp"
#include <cassert>

namespace om {

using top_level = tlist;
using bot_level = blist<top_level>;
using bl_node = blist_node<bot_level>;
using node = two_list::node;

#define TL(x) (static_cast<top_level*>(x))
#define BL(x) (static_cast<bot_level*>(x))
#define BLNODE(x) (static_cast<bl_node*>(x))
#define BLNODEc(x) (static_cast<const bl_node*>(x))

two_list::two_list() {
  m_tl = static_cast<void*>(new top_level());
  m_first_bl = static_cast<void*>(new bot_level(TL(m_tl)->first()));
}

node* two_list::insert(node* base)
{
  bl_node* b = BLNODE(base);
  bl_node* n = nullptr;

  while (!(n = b->ds->insert(b)))
    TL(m_tl)->insert_many(b->ds->split());

  return static_cast<node*>(n);
}

bool two_list::precedes(const node* _x, const node* _y)
{
  const bl_node* x = BLNODEc(_x);
  const bl_node* y = BLNODEc(_y);
  // if (!x) return true;
  // assert(y);
  assert(x && y);
  if (x->ds == y->ds) return bot_level::precedes(x,y);
  return top_level::precedes(x->above(),y->above());
}

node* two_list::first() const
{
  return static_cast<void*>(BL(m_first_bl)->first());
}
} // namespace om
