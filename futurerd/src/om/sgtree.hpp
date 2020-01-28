// A scapegoat tree for the top-level of our OM data structure. We use
// it slightly differently than normal: each insert occurs after an
// existing node, rather than with some given item. The "items" here
// are labels that correspond to the path to each node.

// temporary
typedef unsigned int label_t;

class sgtree {
private:

  // The tree is alpha-weight-balanced, where alpha = 2/3
  static constexpr size_t numerator = 2;
  static constexpr size_t denominator = 3;
  struct node {
    node *left, *right;
    label_t lab;
    node *parent; // temporary, remove later!
  };
  node* m_root;

  // Why do we need this??
  size_t m_size; // number of nodes

  // Why do we need this??
  size_t m_depth;

  static inline label_t size(node* n) const {
    return (n == nullptr) ? 0 : size(n->left) + size(n->right) + 1;
  }

  static inline bool balanced(node* n) {
    return denominator * size(n) <= numerator * size(n->parent);
  }

  static inline label_t log2(label_t q) constexpr {
    return ((label_t) (8*sizeof(label_t) - __builtin_clzll(q) - 1));
  }

  static inline label_t ceil(double x, double y) constexpr {
    return 1 + ((x-1) / y);
  }

  // log_{1/alpha} q = log_2(q) / log_2(1/alpha)
  static inline label_t log_1alpha(label_t q) {
    static constexpr double log_alpha = log2((double)denominator / (double)numerator);
    return (label_t) ceil((double)log2(q), log_alpha);
  }

  static inline node* sibling(node* n) {
    return (n->parent->left == n) ? n->parent->right : n->parent->left;
  }

public:
  sgtree();
  ~sgtree() {} // for now, do nothing, since we'll use it the whole time
  void insert(node* prev); // insert after prev

  size_t size() const { return n; } // is this really necessary?
}; // class sgtree
