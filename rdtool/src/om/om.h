/** An efficient order-maintenance data structure meant to be used
    with Batcher.
*/
#ifndef _OM_H
#define _OM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stdio.h>

#include "om_common.h"
#include "blist.h"

typedef unsigned char flag_t;
struct tl_node_s {

  /** Even internal nodes have a label. For internal nodes this
     corresponds to the highest possible leaf label it could possibly
     contain. */
  label_t label;
  size_t level;
  size_t num_leaves; // Note we only keep this, not the actual node count
  
  struct tl_node_s* parent;

  /// Leaves don't need left/right pointers
  /// Internal nodes don't need prev/next pointers
  union {
    struct tl_node_s* left;
    struct tl_node_s* prev;
  };
  union {
    struct tl_node_s* right;
    struct tl_node_s* next;
  };
  /** Note:
   *  Left/right pointers are only used for relabeling, and ONLY valid
   *  after a split operation. It would be extra work to splice the
   *  split nodes into the list of leaves while rebuilding (without a
   *  race condition on freeing the old nodes), so I'm not going to
   *  implement it unless we have a reason to always have a list of leaves.
   */

  union {
    blist* below;
    struct tl_node_s* split_nodes;
  };
  flag_t needs_rebalance;
};

struct om_s {
  struct tl_node_s* root;
  struct tl_node_s* head;
  struct tl_node_s* tail;
  size_t height;
};

typedef struct tl_node_s tl_node;
typedef struct om_s om;
typedef bl_node om_node;

/** Allocate a tree from the heap and initialize.
  * @return An empty tree.
*/
om* om_new();

/** Initialize a pre-allocated list. */
void om_create(om* self);

/** Destroy and deallocate list. */
void om_free(om* self);

/** Destroy, but do not de-allocate, list. */
void om_destroy(om* self);

om_node* om_insert(om* self, om_node* base);
om_node* om_insert_initial(om* self);

/** Returns true if x precedes y in the order, false otherwise. */
bool om_precedes(const om_node* x, const om_node* y);

tl_node* om_get_tl(om_node* n);

/** Verify all labels are in the correct order and struct is valid. */
int om_verify(const om* self);

/** Print list to a file. */
void om_fprint(const om* self, FILE* out);

/** Print list to standard output. */
void om_print(const om* self);

void om_relabel(om* self, tl_node** heavy_lists, size_t num_heavy_lists);

#ifdef __cplusplus
} // extern C
#endif

#endif // ifndef _OM_H
