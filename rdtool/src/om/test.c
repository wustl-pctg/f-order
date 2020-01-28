#include <stdio.h>
#include <assert.h>

#define TEST
#include "om.c"

#define ck_assert assert
#define ck_assert_ptr_eq(x,y) assert((x) == (y))
#define ck_assert_ptr_ne(x,y) assert((x) != (y))

size_t g_marked_array_size = 0;
tl_node** g_marked_array;

int fake = 0;

size_t verify(tl_node* n, label_t llab, label_t rlab)
{
  if (!n) return 0;
  if (n->level == MAX_LEVEL) {
    if (n->size != 1) {
      fake++;
      assert(0);
    }
    if (llab != n->label) {
      fake++;
      assert(0);
    }
    if (n->parent->level == MAX_LEVEL - 1) {
      if (rlab != llab) {
        fake++;
        assert(0);
      }
    }
    return 1;
  }
  if (n->label != rlab) {
    assert(n->label == rlab && rlab % 2 == 1);
  }
  if (n->left) {
    if (n->left->parent != n) {
      fake++;
      assert(0);
    }
  }
  if (n->right) {
    if (n->right->parent != n) {
      fake++;
      assert(0);
    }
  }
  
  label_t mlab = llab + ((rlab - llab) / 2);
  return verify(n->left, llab, mlab) + verify(n->right, mlab+1, rlab);
}

void verify_om(om* t, size_t size)
{
  if (verify(t->root, 0, MAX_LABEL) != size) {
    fake++;
    assert(0);
  }
}

int main(void)
{
  MAX_LABEL = (label_t) 31;
  MAX_LEVEL = (label_t) 5;
  NODE_INTERVAL = (label_t) 6; // 2 / log 1
  SUBLIST_SIZE = (label_t) 5;
  om* t = om_new();
  g_marked_array = malloc(sizeof(tl_node*) * 128);
  g_marked_array_size = 1;

  bl_node* n0 = om_insert_initial(t); verify_om(t, 1);
  bl_node* n1 = om_insert(t, n0); verify_om(t, 1);
  bl_node* n2 = om_insert(t, n0); verify_om(t, 1);
  bl_node* n3 = om_insert(t, n2); verify_om(t, 1);
  bl_node* n4 = om_insert(t, n2); verify_om(t, 1);
  bl_node* n5 = om_insert(t, n2); verify_om(t, 1);
  
  g_marked_array[0] = n5->list->above;
  printf("Begin relabel %i\n", 1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 2);
  printf("Done relabel %i\n", 1);
  
  assert(n0->list->above->parent->parent == t->root);
  assert(n1->list->above->parent->parent == t->root);
  assert(n0->list->above != n1->list->above);
  assert(n0->list->above->parent == n1->list->above->parent);
  
  bl_node* n6 = om_insert(t, n5); verify_om(t, 2);
  bl_node* n7 = om_insert(t, n5); verify_om(t, 2);
  
  g_marked_array[0] = n5->list->above;
  printf("Begin relabel %i\n", 2);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 3);
  printf("Done relabel %i\n", 2);

  
  bl_node* n8 = om_insert(t, n5); verify_om(t, 3);
  bl_node* n9 = om_insert(t, n8); verify_om(t, 3);
  
  g_marked_array[0] = n8->list->above;
  printf("Begin relabel %i\n", 3);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 4);
  printf("Done relabel %i\n", 3);

   
  bl_node* n10 = om_insert(t, n8); verify_om(t, 4);
  bl_node* n11 = om_insert(t, n8); verify_om(t, 4);
  bl_node* n12 = om_insert(t, n11); verify_om(t, 4);
  
  g_marked_array[0] = n12->list->above;
  printf("Begin relabel %i\n", 4);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 5);
  printf("Done relabel %i\n", 4);

   
  bl_node* n13 = om_insert(t, n12); verify_om(t, 5);
  bl_node* n14 = om_insert(t, n13); verify_om(t, 5);
  bl_node* n15 = om_insert(t, n9); verify_om(t, 5);
  bl_node* n16 = om_insert(t, n15); verify_om(t, 5);
  bl_node* n17 = om_insert(t, n6); verify_om(t, 5);
  bl_node* n18 = om_insert(t, n17); verify_om(t, 5);
  bl_node* n19 = om_insert(t, n17); verify_om(t, 5);
  bl_node* n20 = om_insert(t, n17); verify_om(t, 5);
  bl_node* n21 = om_insert(t, n20); verify_om(t, 5);
  
  g_marked_array[0] = n21->list->above;
  printf("Begin relabel %i\n", 6-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 6);
  printf("Done relabel %i\n", 6-1);

   
  bl_node* n22 = om_insert(t, n21); verify_om(t, 6);
  bl_node* n23 = om_insert(t, n22); verify_om(t, 6);
  bl_node* n24 = om_insert(t, n3); verify_om(t, 6);
  bl_node* n25 = om_insert(t, n24); verify_om(t, 6);
  bl_node* n26 = om_insert(t, n24); verify_om(t, 6);
  bl_node* n27 = om_insert(t, n24); verify_om(t, 6);
  bl_node* n28 = om_insert(t, n27); verify_om(t, 6);
  
  g_marked_array[0] = n27->list->above;
  printf("Begin relabel %i\n", 7-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 7);
  printf("Done relabel %i\n", 7-1);

   
  bl_node* n29 = om_insert(t, n27); verify_om(t, 7);
  bl_node* n30 = om_insert(t, n27); verify_om(t, 7);
  bl_node* n31 = om_insert(t, n30); verify_om(t, 7);
  
  g_marked_array[0] = n31->list->above;
  printf("Begin relabel %i\n", 8-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 8);
  printf("Done relabel %i\n", 8-1);

   
  bl_node* n32 = om_insert(t, n31); verify_om(t, 8);
  bl_node* n33 = om_insert(t, n32); verify_om(t, 8);
  bl_node* n34 = om_insert(t, n28); verify_om(t, 8);
  bl_node* n35 = om_insert(t, n34); verify_om(t, 8);
  bl_node* n36 = om_insert(t, n1); verify_om(t, 8);
  bl_node* n37 = om_insert(t, n1); verify_om(t, 8);
  bl_node* n38 = om_insert(t, n37); verify_om(t, 8);
  bl_node* n39 = om_insert(t, n37); verify_om(t, 8);
  bl_node* n40 = om_insert(t, n37); verify_om(t, 8);
  
  g_marked_array[0] = n40->list->above;
  printf("Begin relabel %i\n", 9-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 9);
  printf("Done relabel %i\n", 9-1);

   
  bl_node* n41 = om_insert(t, n40); verify_om(t, 9);
  bl_node* n42 = om_insert(t, n40); verify_om(t, 9);
  
  g_marked_array[0] = n40->list->above;
  printf("Begin relabel %i\n", 10-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 10);
  printf("Done relabel %i\n", 10-1);

  
  bl_node* n43 = om_insert(t, n40); verify_om(t, 10);
  bl_node* n44 = om_insert(t, n43); verify_om(t, 10);
  
  g_marked_array[0] = n43->list->above;
  printf("Begin relabel %i\n", 11-1);
  om_relabel(t, g_marked_array, g_marked_array_size);
  verify_om(t, 11);
  printf("Done relabel %i\n", 11-1);


  om_free(t);
  free(g_marked_array);
  return 0;
}
