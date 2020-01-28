// General reachability data structure

#include <cstddef>
#include <vector>

#include "utils/bitvector.hpp"
#include "utils/bitsums.hpp"

// Sometime I'd like to try basically a hash map but each entry is a
// "block" for a bitset. Using std::map was quite slow, but maybe some
// kind of multi-level table would be faster, like the shadow memory
// implementation.
// #include <map>
// using cbitset = std::map<size_t, bitset::block_t>;
// #define MASK(index) (1 << ((index) % sizeof(bitset::block_t)))


namespace reach {
class general {
public:
  using bitset = bitvector;
  // using bitset = std::vector<bool>;
  //using bitset = bitsums;

  using node = std::size_t;

  general();
  ~general();

  //void add_edges_from(node* from_nodes, int num_from, node to);
  void add_edge(node from, node to);
  node add_node();
  bool precedes(node x, node y);
  void merge_nodes(node this_node, node that_node);

private:

  // Reverse reachability: data[i][j] == true means we can reach i
  // from j. (i <- j) This is for better cache usage in add_edge.
  // std::vector< std::vector<bool> > data;
  std::vector<bitset> data;

#if STATS == 1
  uint64_t edges = 0;
#endif
}; // class reachability

} // namespace reach
