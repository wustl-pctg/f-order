#include <stdio.h>

#include <check.h>

#define TEST
//#include "blist.c"
//#include "blist_split.c"
#include "om.c"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

blist* g_list = NULL;
void setup_inserts(void) { g_list = bl_new(); }
void teardown_inserts(void) { bl_free(g_list); g_list = NULL; }

START_TEST(test_create)
{
  blist* list = bl_new();

  ck_assert_ptr_ne(list, NULL);
  ck_assert_uint_eq(bl_size(list), 0);
  ck_assert_ptr_eq(list->head, NULL);
  ck_assert_ptr_eq(list->tail, NULL);

  bl_free(list);
}
END_TEST

START_TEST(test_initial_insert)
{
  bl_node* x = bl_insert_initial(g_list);

  ck_assert_ptr_ne(x, NULL);
  ck_assert_ptr_eq(x->next, NULL);
  ck_assert_ptr_eq(x->prev, NULL);
  ck_assert(x->label == 0);
  
  ck_assert_ptr_eq(g_list->head, x);
  ck_assert_ptr_eq(g_list->tail, x);
  ck_assert_uint_eq(bl_size(g_list), 1);
}
END_TEST

START_TEST(test_insert_middle)
{
  bl_node* x = bl_insert_initial(g_list);
  bl_node* z = bl_insert(g_list, x);
  bl_node* y1 = bl_insert(g_list, x);
  bl_node* y0 = bl_insert(g_list, x);
  bl_node* y2 = bl_insert(g_list, y1);
  
  ck_assert_ptr_ne(y0, NULL);
  ck_assert_ptr_ne(y1, NULL);
  ck_assert_ptr_ne(y2, NULL);
  ck_assert_ptr_eq(x->prev, NULL); ck_assert_ptr_eq(x->next, y0);
  ck_assert_ptr_eq(y0->prev, x); ck_assert_ptr_eq(y0->next, y1); 
  ck_assert_ptr_eq(y1->prev, y0); ck_assert_ptr_eq(y1->next, y2); 
  ck_assert_ptr_eq(y2->prev, y1); ck_assert_ptr_eq(y2->next, z); 
  ck_assert_ptr_eq(z->prev, y2); ck_assert_ptr_eq(z->next, NULL);
  
  ck_assert(y0->label == (x->label + y1->label) / 2);
  ck_assert(y1->label == (y0->label + y2->label) / 2);
  ck_assert(y2->label == (y1->label + z->label) / 2);
  
  ck_assert_ptr_eq(g_list->head, x);
  ck_assert_ptr_eq(g_list->tail, z);
  ck_assert_uint_eq(bl_size(g_list), 5);
}
END_TEST

START_TEST(test_split_simple)
{
  NODE_INTERVAL = 2; SUBLIST_SIZE = 1;
  
  blist* list = bl_new();
  node* n0 = bl_insert_initial(list);
  node* n1 = bl_insert(list, n0);

  tl_node** array;
  size_t array_size = split(list, &array);

  ck_assert(array_size == 2);
  ck_assert(bl_size(array[0]->below) == 1); ck_assert(bl_size(array[1]->below) == 1);
  ck_assert(n0->label == 0); ck_assert(n1->label == 0);
  ck_assert(array[0]->below->head == n0); ck_assert(array[0]->below->tail == n0);
  ck_assert(array[1]->below->head == n1); ck_assert(array[1]->below->tail == n1);

  bl_free(array[0]->below); bl_free(array[1]->below);
  free(array);
}
END_TEST

START_TEST(test_split_many)
{
  NODE_INTERVAL = (label_t)6; // 32 / log 32
  SUBLIST_SIZE = (label_t)5;
  
  blist* list = bl_new();
  size_t num_nodes = 99;
  node** nodes = malloc(sizeof(node*) * num_nodes);
  nodes[0] = bl_insert_initial(list);
  for (int i = 1; i < num_nodes; ++i) {
    nodes[i] = node_new();
    nodes[i]->label = nodes[i-1]->label + NODE_INTERVAL;
    insert_internal(list, nodes[i-1], nodes[i]);
  }
  ck_assert(bl_size(list) == num_nodes);

  tl_node** lists;
  size_t array_size = split(list, &lists);
  ck_assert(array_size == 20); // ceil(num_nodes / SUBLIST_SIZE)

  size_t n = 0;
  for (int i = 0; i < array_size - 1; ++i) {
    blist* current_list = lists[i]->below;
    ck_assert(bl_size(current_list) == SUBLIST_SIZE);
    
    node* current_node = current_list->head;
    label_t label = 0;
    while (current_node) {
      ck_assert_ptr_eq(current_node, nodes[n]);
      ck_assert(current_node->label == label);

      if (current_list->head == current_node)
        ck_assert_ptr_eq(current_node->prev, NULL);
      else
        ck_assert_ptr_eq(current_node->prev, nodes[n-1]);

      if (current_list->tail == current_node)
        ck_assert_ptr_eq(current_node->next, NULL);
      else
        ck_assert_ptr_eq(current_node->next, nodes[n+1]);

      label += NODE_INTERVAL;
      current_node = current_node->next;
      n++;
    }
  }
  
  // Check final list, may not be full size
  ck_assert(bl_size(lists[array_size - 1]->below) == num_nodes % SUBLIST_SIZE);
  ck_assert(n + bl_size(lists[array_size - 1]->below) == num_nodes);

  for (int i = 0; i < array_size; ++i) bl_free(lists[i]->below);
  free(lists);
}
END_TEST


Suite* blist_suite(void)
{
  Suite* s = suite_create("Bottom List Tests");

  TCase* tc_basic = tcase_create("Bottom list creation and destruction");
  tcase_add_test(tc_basic, test_create);
  suite_add_tcase(s, tc_basic);

  TCase* tc_inserts = tcase_create("Basic bottom list inserts with no overflow");
  tcase_add_checked_fixture(tc_inserts, setup_inserts, teardown_inserts);
  tcase_add_test(tc_inserts, test_initial_insert);
   tcase_add_test(tc_inserts, test_insert_middle);
  suite_add_tcase(s, tc_inserts);

  TCase* tc_split = tcase_create("Handling overflow by splitting");
  tcase_add_test(tc_split, test_split_simple);
  tcase_add_test(tc_split, test_split_many);
  /* tcase_add_test(tc_split, test_split_end); */

   suite_add_tcase(s, tc_split);
  
  return s;
}

int main(void)
{
  int num_failed = 0;
  Suite* s = blist_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  num_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
