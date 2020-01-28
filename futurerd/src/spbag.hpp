#pragma once
#include "utils/union_find.hpp"

// This is written a little differently than uf::node because the
// merge is special. See below.
struct spbag : utils::uf::node {

  static void* operator new(std::size_t sz);
  static void* operator new[](std::size_t sz) = delete;

  enum class bag_kind { S, P };
  bag_kind kind;
  
  inline bool is_S() const { return kind == bag_kind::S; }
  inline bool is_P() const { return kind == bag_kind::P; }
  inline void set_kind(bag_kind k) { kind = k; }
  spbag(bag_kind k) : kind(k) {}
  
  static inline spbag* find(spbag *x) {
    return static_cast<spbag*>(utils::uf::find(x));
  }

  // We can use a more restricted function than "precedes" since we
  // only call it with the active strand, i.e. precedes(u, active)
  inline bool precedes_now() { return find(this)->is_S(); }

  // This special merge makes sure that the type of the merged bag is
  // the same as that of "this". That's why I write this as a member
  // function rather than a static function that takes two spbags.
  void merge(spbag *that);
  
private:
  //static void data_swap(spbag *x, spbag *y);

}; // struct bag
