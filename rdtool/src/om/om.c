#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#include "om.h"
#include "om_common.h"

unsigned int g_num_malloc_calls;
double g_malloc_begin;
#include "blist.c"

#ifdef __cplusplus
extern "C" {
#endif

static inline int is_leaf(tl_node* n)
{
  return n->level == MAX_LEVEL || (n->prev == NULL && n->right == NULL);
}

tl_node* tl_node_new()
{
  tl_node* x = (tl_node*)malloc(1 * sizeof(tl_node));
  return x;
}

void tl_node_free_recursive(tl_node* n)
{
  if (n->level < MAX_LEVEL) {
    if (n->left) tl_node_free_recursive(n->left);
    if (n->right) tl_node_free_recursive(n->right);
  }
  free(n);
}

void tl_node_free(tl_node* n)
{
  free(n);
}

// The relabel functions are separated to make the code easier to read.
#include "om_relabel.c"

void om_create(om* self)
{
  tl_node* root = tl_node_new();
  root->below = bl_new();
  root->below->above = root;

  root->level = MAX_LEVEL;
  root->num_leaves = 1;
  root->label = 0;

  root->needs_rebalance = 0;
  root->parent = root->left = root->right = NULL;

  self->root = root;
  self->head = self->tail = root;
  self->height = 0;
}

om* om_new()
{
  om* self = (om*) malloc(sizeof(*self));
  om_create(self);
  return self;
}

void om_free(om* self)
{
  om_destroy(self);
  free(self);
}

/// Internal, recursive destroy
void destroy(tl_node* n)
{
  if (!n) return;
  if (n->level < MAX_LEVEL) {
    destroy(n->left);
    destroy(n->right);
  } else {
    bl_free(n->below);
  }
  free(n);
}

tl_node* om_get_tl(node* n)
{
  assert(n->list->above->level == MAX_LEVEL);
  return n->list->above;
}

void om_destroy(om* self) { destroy(self->root); }

node* om_insert_initial(om* self)
{
  assert(self->root->num_leaves == 1);
  node* n = bl_insert_initial(self->root->below);
  assert(n->list == self->root->below);
  return n;
}

node* om_insert(om* self, node* base)
{
  assert(base->list->above->level == MAX_LEVEL);
  assert(base->list->above->below == base->list);
  node* n = bl_insert(base->list, base);
  if (n) assert(n->label > base->label);
  return n;
}

bool om_precedes(const node* x, const node* y)
{
  if (x == NULL) return true;
  if (y == NULL) return false;
  if (x->list == y->list) return x->label < y->label;
  return x->list->above->label < y->list->above->label;
}

int verify(tl_node* n)
{
  if (!n) return 0;
  assert(n->level <= MAX_LEVEL);
  int left = verify(n->left);
  int right = verify(n->right);
  if (n->left) assert(n->left->parent == n);
  if (n->right) assert(n->right->parent == n);

  if (n->level == MAX_LEVEL) { // leaf
    assert(n->num_leaves == 1);
    assert(n->left == NULL && n->right == NULL);
    bl_verify(n->below);
    assert(n->below->above == n);
  } else { // not leaf
    assert(n->num_leaves == left + right);
  }
  return n->num_leaves;
}

int om_verify(const om* self)
{
  assert(self->root->parent == NULL);
  verify(self->root);
  return 0;
}

void om_fprint(const om* self, FILE* out)
{
  if (!om_verify(self)) fprintf(out, "Warning: OM is not valid!\n");

  size_t num_leaves = 1 << MAX_LEVEL;

  tl_node** future = (tl_node**) calloc(num_leaves, sizeof(tl_node*));
  tl_node** current = (tl_node**) calloc(num_leaves, sizeof(tl_node*));
  future[0] = self->root;

  for (int level = 0; level < MAX_LEVEL; ++level) {

    tl_node** tmp = current;
    current = future;
    future = tmp;

    size_t num_nodes = 1 << level;
    size_t spacing = 1 << (MAX_LEVEL - level);
    for (int i = 0; i < num_nodes; ++i) {
      tl_node* n = current[i];


      for (int j = 0; j < spacing; ++j) fprintf(out, "\t");
      if (!n || n->level != level) fprintf(out, "-");
      else fprintf(out, "%lu", n->label);
      for (int j = 0; j < spacing; ++j) fprintf(out, "\t");

      tl_node *left, *right;
      if (!n) {
        left = right = NULL;
      } else if (n->level == MAX_LEVEL) {
        left = n;
        right = NULL;
      } else {
        left = n->left;
        right = n->right;
      }
      future[i * 2] = left;
      future[i * 2 + 1] = right;
    }
    fprintf(out, "\n");
  }
  for (int i = 0; i < num_leaves; ++i) { // leaves
    tl_node* leaf = future[i];
    fprintf(out, "\t");
    if (leaf) fprintf(out, "%lu", leaf->label);
    else fprintf(out, "-");
    fprintf(out, "\t");
  }
  fprintf(out, "\n");
  free(future);
  free(current);
}

void om_print(const om* self) { om_fprint(self, stdout); }

size_t node_memsize(tl_node* n)
{
  if (!n) return 0;
  size_t size = sizeof(*n);
  if (is_leaf(n))
    size += bl_memsize(n->below);
  else
    size += node_memsize(n->left) + node_memsize(n->right);
  return size;
}

size_t om_memsize(const om* self)
{
  return node_memsize(self->root) + sizeof(om);
}


#ifdef __cplusplus
} // extern C
#endif
