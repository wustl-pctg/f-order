#include "sgtree.hpp"

// see http://www.sanfoundry.com/java-program-implement-scape-goat-tree/
// and https://en.wikipedia.org/wiki/Scapegoat_tree

sgtree::sgtree() {
  // ??
}

size_t add(node* prev, node* n)
{
  assert(m_root);
  label_t label = prev->label + 1;
  label_t real_label = 0; // build this up as we go down
  node* current = m_root;
  size_t depth = 0;

  while(true) {
    assert(label != current->lab); // needs complete rebuild?
    node** child;
    if (label < current->lab) { // go left
      // do something to real_label here
      child = &current->left;
    } else { // go right
      // do something to real_label here
      child = &current->right;
    }
    
    if (*child) // keep going
      current = *child;
    else { // found an opening
      *child = n;
      n->parent = current;
      break;
    }
    depth++;
  }

  if (depth == m_depth) m_depth++;
  m_size++;
  n->lab = real_label;
  return m_depth;
}

int sgtree::insert(node* prev)
{
  node* current = m_root;
  node* n = new node();

  size_t d = add(prev, n);
  size_t size = 1;
  if (d > log_1alpha(q)) {
    current = current->parent;
    size += size(sibling(current));
    
    while (balanced(current)) {
      current = current->parent;
      size += size(sibling(current));
    }
    rebuild(current->parent, size);
  }
}

void sgtree::rebuild(node* n, size_t size)
{
  static node** scratch = (node**) malloc(sizeof(node*) * scratch_size);
  if (size > scratch_size) {
    scratch_size *= 2;
    realloc(scratch, scratch_size * sizeof(node*));
  }

  node* parent = n->parent;
  pack(n, scratch);
  node* subtree_root = build(scratch, size);

  if (parent == nullptr) { // rebuilt whole tree
    m_root = subtree_root;
  } else if (parent->right == n) { // was right child
    parent->right = subtree_root;
  } else { // was left child
    parent->left = subtree_root;
  }
  subtree_root->parent = parent;
  return;
}

node* sgtree::build(node** scratch, size_t size, size_t i)
{
  if (size == 0) return nullptr;
  size_t m = size / 2;
  
  node* left = build(scratch, i, m);
  scratch[i + m]->left = left;
  if (left) left->parent = scratch[i+m];

  node* right = build(scratch, i + m + 1, size - m - 1);
  scratch[i+m]->right = right;
  if (right) right->parent = scratch[i+m];

  return scratch[i+m];
}

// assume scratch is big enough
size_t sgtree::pack(node* n, node** scratch, size_t i = 0)
{
  if (n == nullptr) return index;
  i = pack(n->left, scratch, i);
  scratch[i++] = n;
  return pack(n->right, scratch, i);
}
