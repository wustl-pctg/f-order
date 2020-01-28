// Should be a reachability data structure ONLY

// We don't actually need the structured part, just the fork/join part
// of SP Bags.
#include "utils/stack.hpp"
#include "reach_structured.hpp"
#include "reach_gen.hpp"

namespace reach {
class nonblock {
protected:
  static constexpr uint32_t DEFAULT_SYNC_BLOCK_SIZE = 16;

public:
  struct node : utils::uf::node { // inherit from union-find node type
    node *att_pred = nullptr;
    node *att_succ = nullptr;
    reach::general::node id; // id of node in R graph, or 0 if not in R (size_t)
    structured::smem_data *sbag; // ANGE: a bit hacky, but good enough for now
    bool attached() { return id > 0; }
    node* find() { return static_cast<node*>(utils::uf::find(this)); }
    node(reach::general::node _id = 0) : id(_id) {}
    // always merge that into this.
    void merge(node *n) { utils::uf::merge(this, n); }
  }; // struct node

  struct fork_node_data {
    node *fork = nullptr; // the fork node
    node *lfc = nullptr;  // the left-fork child
    node *rfc = nullptr;  // the right-fork child
    node *ljp = nullptr;  // the last strand of a spawned child
  };

  typedef utils::stack<fork_node_data> fork_stack_t;
  struct sframe_data {
    structured::sframe_data sp;
    // Future RD stuff.
    node *future_fork = nullptr;
    // for handling joins with in-degree > 2; last-in first-out
    fork_stack_t fork_stack;
    sframe_data() : fork_stack(DEFAULT_SYNC_BLOCK_SIZE) {}
  };

  //struct smem_data {};
  using smem_data = node;

  struct sfut_data {
    node* put_strand;
  };

  // Member data
public:
  reach::general m_R;
  reach::structured m_sp; // series-parallel reachability
  static node* t_current; // XXX: necessary?

public:
  nonblock() {}
  nonblock(sframe_data *initial);
  void init(sframe_data *initial);

  smem_data* active(sframe_data *f);

  // Parallelism creation
  void at_future_create(sframe_data *f);
  void at_spawn(sframe_data *f, sframe_data *helper);

  // Parallelism deletion
  // void at_future_put(sfut_data*);
  void at_future_get(sframe_data *f, sfut_data* fut);
  void at_sync(sframe_data *f);

  // Continuations
  void at_spawn_continuation(sframe_data *f, sframe_data *p);
  void at_future_continuation(sframe_data *f, sframe_data *p);

  // Cilk/future task functions
  void at_future_finish(sframe_data *f, sframe_data *p, sfut_data* fut);

  void begin_strand(sframe_data *f, sframe_data *p);
  bool precedes_now(sframe_data *f, smem_data *last_access);

private:
  // make the set containing u attached if not already
  void attachify(node *n);

  // Helper function for syncs; assume binary join
  node* binary_join(node *f, // fork node
                    node *lfc, node *rfc, // left, right fork children
                    node *ljp, node *rjp); // left, right join children

  // Helper function for syncs; if join is not binary
  node* perform_join(sframe_data *f);
  void handle_binary_fork_at_sync(node *f, // fork node
                                  node *lfc, node *rfc,  // left, right fork children
                                  node *ljp, node *rjp); // left, (fake) right join parent

  static inline void
  copy_fork_stack_data(fork_node_data *dst, fork_node_data *src) {
    dst->fork = src->fork;
    dst->lfc = src->lfc;
    dst->rfc = src->rfc;
    dst->ljp = src->ljp;
  }

}; // class nonblock
} // namespace reach
