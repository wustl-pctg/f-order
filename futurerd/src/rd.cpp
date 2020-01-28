#include "rd.hpp"
#include "debug.hpp"

// XXX: Compiler doesn't define __cilk?
#define USE_CILK_API
#include <cilk/cilk_api.h>
// #include <internal/abi.h>

uint64_t race_detector::t_stack_low_watermark = (uint64_t)(-1);
bool race_detector::t_clear_stack = false;
enum rd_policy race_detector::g_policy = RD_EXIT;
size_t race_detector::g_num_races = 0;
int race_detector::check_disabled = 1;
reach_ds race_detector::g_reach;
shadow_mem race_detector::g_smem;
shadow_stack<sframe_data> race_detector::t_sstack;

// XXX: Can probably combine these two.
bool race_detector::tsan_init = false;
bool race_detector::shadow_enabled = true;

// create the race detector (intialize)
race_detector g_detector;

void race_detector::enable_checking() { check_disabled--; }
void race_detector::disable_checking() { check_disabled++; }

race_detector::race_detector() {
  // Ensure only one race detector
  static bool init = false;
  assert(init == false);
  init = true;

  enable_checking();

  t_sstack.push();
  // not really, but need to initialize
  //g_reach.at_spawn(t_sstack.head());
  g_reach.init(t_sstack.head());

  __cilkrts_set_param("nworkers", "1");
}

race_detector::~race_detector() {}

void race_detector::set_policy(enum rd_policy p) {
  if (p >= RD_LAST_POLICY)
    debug::die("Invalid Detection Policy\n");
  g_policy = p;
}

// @todo{report_race() should report what kind of race, i.e. RW, WR, or WW}
void race_detector::report_race(addr_t addr, addr_t last_rip, addr_t this_rip,
                                race_type rt) {
  return;
  if (! (g_policy == RD_SILENT) )
    fprintf(stderr, "Race detected at %lx, last rip %lx, this rip %lx\n", addr,
            last_rip, this_rip);
  g_num_races++;
  if (g_policy == RD_EXIT)
    std::exit(rt);
}

void race_detector::mark_stack_allocate(void* addr) {
  race_detector::t_stack_low_watermark = (uint64_t)addr;
}

smem_data* race_detector::active() { return g_reach.active(t_sstack.head()); }

// void race_detector::check_access(bool is_read, addr_t rip,
//                                  addr_t addr, size_t mem_size) {
//   shadow_mem::addr_info_t *slot = g_smem.find(addr);
//   // fprintf(stderr, "Checking %d, rip %p, addr %p, active %p.\n", is_read, rip, addr, active());

//   // no previous accesses
//   if (slot == nullptr) {
//     g_smem.insert(is_read, addr, active(), rip);
//     return;
//   }

//   // check race with last writer
//   if (slot->last_writer.access != nullptr // if last writer exists
//       && !g_reach.precedes_now(t_sstack.head(), slot->last_writer.access)) {
//     race_type rt = (is_read) ? race_type::WR : race_type::WW;
//     report_race(addr, slot->last_writer.rip, rip, rt);
//   }

//   // if write, check race with last read
//   if (!is_read // if a write
//       && slot->last_reader.access != nullptr // and last reader exists
//       && !g_reach.precedes_now(t_sstack.head(), slot->last_reader.access)) {
//     report_race(addr, slot->last_reader.rip, rip, race_type::RW);
//   }

//   // update shadow mem
//   g_smem.update(slot, is_read, addr, active(), rip);
// }

void race_detector::handle_read(access_t* slot, addr_t rip, addr_t addr,
                                 size_t mem_size, smem_data *current) {

  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
  assert(start >= 0);
  assert(start < NUM_SLOTS);

  // This may not be true, e.g. when start == 1...
  assert((start + grains) <= NUM_SLOTS);

  // Check race with last writers to this chunk
  for (int i{start}; i < (start + grains); ++i) {
    MemAccess_t *writer = slot->writers[i];
    if (writer && !g_reach.precedes_now(t_sstack.head(), writer->rd)) {
      report_race(addr, writer->rip, rip, race_type::WR);
    }
  }

  // update readers
  for (int i{start}; i < (start + grains); ++i) {
    MemAccess_t *reader = slot->readers[i];
    if (reader)
      new (reader) MemAccess_t{current, rip};
    else
      slot->readers[i] = new MemAccess_t{current, rip};
  }
}

void race_detector::handle_write(access_t* slot, addr_t rip, addr_t addr,
                                 size_t mem_size, smem_data *current) {
  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
  assert(start >= 0);
  assert(start < NUM_SLOTS);

  // TODO: As far as I can tell, this may not be true, e.g. when start == 1...
  // Update: have not seen this in a while, I think something else was
  // wrong at the time.
  if ((start + grains) > NUM_SLOTS) {
    fprintf(stderr, "start=%i, grains=%i, NUM_SLOTS=%i, size=%zu\n",
            start, grains, NUM_SLOTS, mem_size);
  }
  assert((start + grains) <= NUM_SLOTS);


  // Check race with last writers to this chunk, and update
  for (int i{start}; i < (start + grains); ++i) {
    MemAccess_t *writer = slot->writers[i];
    if (writer && !g_reach.precedes_now(t_sstack.head(), writer->rd)) {
      report_race(addr, writer->rip, (uint64_t)rip, race_type::WW);
    }
    if (writer)
      new (writer) MemAccess_t{current, rip};
    else
      slot->writers[i] = new MemAccess_t{current, rip};
  }

  for (int i{start}; i < (start + grains); ++i) {
    MemAccess_t *reader = slot->readers[i];
    if (reader && !g_reach.precedes_now(t_sstack.head(), reader->rd)) {
      report_race(addr, reader->rip, (uint64_t)rip, race_type::RW);
    }
  }
}


void race_detector::check_access(bool is_read, addr_t rip,
                                 addr_t addr, size_t mem_size) {
  auto slot = g_smem.find(ADDR_TO_KEY(addr));
  smem_data* current = active();
  assert(current);

  if (slot == nullptr) {
    // not in shadow memory; create a new MemAccessList_t and insert
    slot = new MemAccessList_t(addr, is_read, current, rip, mem_size);
    g_smem.insert(ADDR_TO_KEY(addr), slot);
    return;
  }

  if (is_read) handle_read(slot, rip, addr, mem_size, current);
  else handle_write(slot, rip, addr, mem_size, current);

}
