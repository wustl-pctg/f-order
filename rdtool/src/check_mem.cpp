#include "check_mem.h"
#include "mem_access.h"
//#include "list.h"

void print_num_of_readers(MemAccess_t *mem) {
	uint32_t counter = 0;
	while (mem != NULL) {
		counter++;
		mem = mem->next;
	}
	fprintf(stderr, "# of readers: %u\n", counter);  
}

void report_race() {
  // to-do: report race here
}

void handle_write(MemAccessList_t* slot, addr_t rip, addr_t addr,
                                 size_t mem_size) {

  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
  assert(start >= 0);
  assert(start < NUM_SLOTS);

  // This may not be true, e.g. when start == 1...
  assert((start + grains) <= NUM_SLOTS);

  for (int i{start}; i < (start + grains); ++i) {
    //MemAccess_t *writer = slot->writers[i];
    MemAccess_t *writer = slot->writers[i];
    if(writer == NULL) {
      //writer = new MemAccess_t(current, rip);
      writer = new MemAccess_t(current, NULL);
	  pthread_spin_lock(&slot->writers_lock);
      if(slot->writers[i] == NULL) {
        slot->writers[i] = writer;
      }
      pthread_spin_unlock(&slot->writers_lock);
      if(writer == slot->writers[i]) continue;
      else { // was NULL but some other logically parallel strand updated it
          delete writer;
          writer = slot->writers[i];
      }
    }
    // At this point, someone else may come along and write into writers[i]...
    // om_assert(writer == writers[i]);
    om_assert(writer);

    // om_assert(writer->estrand != curr_estrand && 
    //           writer->hstrand != curr_hstrand);

    
    if (writer->accessor->english != current->english) {
        bool race = false;
        QUERY_START;
        race = !Precedes(writer->accessor, current); 
                    //&& !Precedes(current, writer->accessor);

        QUERY_END;
		if (race) report_race();
    }
    
    pthread_spin_lock(&slot->writers_lock);
    // replace the last writer regardless
    //writers[i]->update_acc_info(curr_estrand, curr_hstrand, inst_addr);
    
	slot->writers[i]->accessor = current;
    //slot->writers[i]->rip     = rip;
    pthread_spin_unlock(&slot->writers_lock);

  }

  // update readers
  //for (int i{start}; i < (start + grains); ++i) {
  //  MemAccess_t *reader = slot->readers[i];
  //  if (reader)
  //    new (reader) MemAccess_t{current, rip};
  //  else
  //    slot->readers[i] = new MemAccess_t{current, rip};
  //}

  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = slot->readers[i];
	while (reader != NULL) {
  	  bool race = false;
      QUERY_START;
      race = !Precedes(reader->accessor, current);
      QUERY_END;
	  if (race) report_race();
	  MemAccess_t *old = reader;
	  reader = reader->next;
	  delete old; // to-do: we could recycle this reader to thread-local pool
	}
	slot->readers[i] = NULL;
  }
  
}

void handle_read(MemAccessList_t* slot, addr_t rip, addr_t addr,
                                 size_t mem_size) {
  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
  assert(start >= 0);
  assert(start < NUM_SLOTS);

  // TODO: As far as I can tell, this may not be true, e.g. when start == 1...
  // Update: have not seen this in a while, I think something else was
  // wrong at the time.
  //if ((start + grains) > NUM_SLOTS) {
  //  fprintf(stderr, "start=%i, grains=%i, NUM_SLOTS=%i, size=%zu\n",
  //          start, grains, NUM_SLOTS, mem_size);
  //}
  assert((start + grains) <= NUM_SLOTS);

  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *writer = slot->writers[i]; // can be NULL
    if (writer == NULL) continue;
    bool race = false;
    QUERY_START;
    race = !Precedes(writer->accessor, current);
	QUERY_END;
    if (race) report_race();
  }

  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = slot->readers[i];
    if(reader == NULL) {
      //reader = new MemAccess_t(current, rip);
      reader = new MemAccess_t(current, NULL);
	  pthread_spin_lock(&slot->readers_lock);
      if (slot->readers[i] == NULL) {
        slot->readers[i] = reader;
      }
      pthread_spin_unlock(&slot->readers_lock);
      if (reader == slot->readers[i]) {
	  	continue;
	  } else {
        //delete reader;
        //reader = slot->readers[i];
		reader->next = slot->readers[i];
      	slot->readers[i] = reader;
	  	continue;
	  }
    }

    om_assert(reader != NULL);
    pthread_spin_lock(&slot->readers_lock);

	// deduplication
	MemAccess_t *tmp = slot->readers[i];
	bool allocated = false;
	while (tmp != NULL) {
	  if (current == tmp->accessor) {
		//assert(current->english == tmp->accessor->english);
		allocated = true;
		break;
	  }
	  tmp = tmp->next;
	}
	
	if (!allocated) {
	  slot->readers[i] = new MemAccess_t(current, reader);
	}
	
	//print_num_of_readers(slot->readers[i]);
	pthread_spin_unlock(&slot->readers_lock);
  }
}

void check_access(bool is_read, addr_t rip,
                                 addr_t addr, size_t mem_size) {
  auto slot = shadow_mem.find(ADDR_TO_KEY(addr));
  //smem_data* current = active();
  //assert(current);
  assert(current->english);

  if (slot == nullptr) {
    // not in shadow memory; create a new MemAccessList_t and insert
    MemAccessList_t *mem_list  = new MemAccessList_t(addr, is_read, current, rip, mem_size);
    slot = shadow_mem.insert(ADDR_TO_KEY(addr), mem_list);
    
    if (slot != mem_list) {
        delete mem_list;
    } else {
        return;
    }
  }
 
  if (is_read) handle_read(slot, rip, addr, mem_size);
  else handle_write(slot, rip, addr, mem_size);

}
