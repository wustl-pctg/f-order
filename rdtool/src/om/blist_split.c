static size_t split(blist* self)
{
  size_t num_lists = 0;
  assert(self->above->below == self);
  assert(self->head);

  node* current_node = self->head;
  tl_node* prev_tl_node = NULL;
  while (current_node) {

    tl_node* n = tl_node_new();
    n->level = MAX_LEVEL;
    n->next = NULL;
    n->prev = prev_tl_node;
    n->num_leaves = 1;
    if (prev_tl_node) prev_tl_node->next = n;
    else self->above->split_nodes = n;
    
    blist* list = bl_new();
    n->below = list;
    list->above = n;
    
    current_node->prev = NULL;
    label_t current_label = 0;
    size_t list_size = 0;
    while (current_node && list_size < SUBLIST_SIZE) {
      node* next = current_node->next;
      current_node->label = current_label;
      insert_internal(list, current_node->prev, current_node);

      current_node = next;
      current_label += NODE_INTERVAL;
      list_size++;
    }
    num_lists++;
    prev_tl_node = n;
  }

  // Free the sublist itself, but not its nodes!
  self->head = self->tail = NULL;
  free(self);

  return num_lists;
}
