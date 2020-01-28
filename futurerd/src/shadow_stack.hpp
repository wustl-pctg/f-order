#include "utils/stack.hpp"

#include <cassert>

template <typename T>
struct shadow_frame {
  enum frame_type : unsigned char { SPAWNER = 0, HELPER};
  frame_type type;
  
  bool synced = true; // is this frame "synced"? Need this because of the
               // implicit sync at the end of every Cilk function
  
  T data;
}; // struct shadow_frame

template <typename T>
class shadow_stack : public utils::stack< shadow_frame<T> > {
  using frame = shadow_frame<T>;
  using base_type = utils::stack<frame>;
  using index_t = typename utils::stack< shadow_frame<T> >::index_t;

public:

  T* push() {
    frame* f = base_type::push();
    f->synced = true;
    return this->head();
  }
  T* head() const { return ancestor(0); }
  T* parent() const { return ancestor(1); }
  T* ancestor(int i) const {
    frame *f = base_type::ancestor(i);
    return &f->data;
  }

  T* push_spawner() {
    frame* f = base_type::push();
    f->synced = true;
    f->type = frame::frame_type::SPAWNER;
    return this->head();
  }

  T* push_helper() {
    frame* f = base_type::push();
    f->synced = true;
    f->type = frame::frame_type::HELPER;
    return this->head();
  }
  
  // XXX: ???
   T* do_spawn() {
     // Current frame is no longer synced, if it was
     base_type::head()->synced = false;
     frame* f = base_type::push();
     f->synced = true;
     f->type = frame::frame_type::SPAWNER;
     return this->head();
   }
  // Returns whether or not we were already synced
  bool do_sync() {
    frame* f = base_type::head();
    bool old = f->synced;
    f->synced = true;
    return old;
  }
}; // class shadow_stack
