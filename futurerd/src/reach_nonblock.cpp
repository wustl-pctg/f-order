#include "reach_nonblock.hpp"
//#define NDEBUG 1
#include <cassert>
//#include <cstdio>
#include <cstdlib>

namespace reach {

using node = nonblock::node;

node* nonblock::t_current = nullptr;

nonblock::nonblock(sframe_data *initial) { init(initial); }

nonblock::smem_data* nonblock::active(sframe_data *f) {
  return t_current;
}

bool nonblock::precedes_now(sframe_data *f, smem_data *last_access) {
  if(last_access->sbag->precedes_now()) { return true; }

  node *u = last_access;
  node *v = t_current;

  node *su = u->find();
  if (!su->attached()) su = su->att_succ;

  node *sv = v->find();
  if (!sv->attached()) sv = sv->att_succ;

  if (su == nullptr || sv == nullptr)
    return false;

  return m_R.precedes(su->id, sv->id);
}

// Should be done in a constructor if possible
void nonblock::init(sframe_data *initial) {
  m_sp.init(&initial->sp);
  t_current = new node();
  t_current->att_pred = t_current;
  t_current->sbag = initial->sp.Sbag;
  attachify(t_current);
  //printf("Initializing %lu\n", t_current->id);
}

void nonblock::attachify(node* n) {
  node *su = n->find();
  if (su->attached()) return;

  // su is unattached
  assert(su->id == 0);
  su->id = m_R.add_node(); // make attached with node in R
  assert(su->att_pred);
  assert(su->att_pred->find()->id <= su->id);
  //printf("Attachify: %lu -> %lu\n", su->att_pred->find()->id, su->id);
  m_R.add_edge(su->att_pred->find()->id, su->id);
  assert(su->att_succ == nullptr);
  // ANGE XXX: do we care to update att_pred to self?  probly not because we'd never
  // query our own att_pred if we are already attached.
  su->att_succ = su;
}

/********** Parallelism Creation **********/

// only called in driver when cilk_enter_begin (enter_helper_begin doesn't
// call this). f is the sframe_data correponding to the cilk function, and p
// is whatever at the head of the stack at the time.
void nonblock::begin_strand(sframe_data *f, sframe_data *p) {
  m_sp.begin_strand(&f->sp,&p->sp);
  // Future stuff?
}

// called at detach; f is the spawning Cilk function and h is the helper
void nonblock::at_spawn(sframe_data *f, sframe_data *h) {
  m_sp.at_spawn(&f->sp, &h->sp);

  node *u = t_current; // fork node
  h->fork_stack.push();
  h->fork_stack.head()->fork = u;

  // Make new unattached set containing v
  node *v = new node(); // left child
  h->fork_stack.head()->lfc = v;

  // v->att_pred = (u->find()->attached()) ? u : u->find()->att_pred;
  v->att_pred = u->find()->att_pred;
  assert(v->att_pred != nullptr);
  assert(v->att_pred->attached());

  //h->nodes.push_fork(u, w, v);
  // v->att_pred = u->find()->att_pred;
  // w->att_pred = u->find()->att_pred;

  // XXX anything else for the other child? Set att_pred?
  // node *w = new node(); // right child
  // h->rfc = w;
  // h->cont = w;

  t_current = v;
  t_current->sbag = h->sp.Sbag;
}

// Outgoing non-SP edge
// We don't have helper frames for futures, so this is actually the
// new frame created for the new strand
void nonblock::at_future_create(sframe_data *f) {
  // Just create a new Sbag, since this is a new strand.
  m_sp.at_future_create(&f->sp);

  node *u = t_current;
  assert(f->future_fork == nullptr);
  f->future_fork = u;
  attachify(u);

  // Create a new attached set
  node *v = new node(m_R.add_node()); // left (executed next, since we use eager execution)
  //printf("Create: %lu --> %lu\n", u->find()->id, v->id);
  m_R.add_edge(u->find()->id, v->id);
  assert(v->find() == v);
  v->att_pred = v->att_succ = v; // self

  t_current = v;
  t_current->sbag = f->sp.Sbag;
}

/********** Parallelism Deletion **********/
// This won't be called until the future has actually finished
// Incoming non-SP edge
void nonblock::at_future_get(sframe_data *f, sfut_data *fut) {
  assert(fut->put_strand);

  node *u = t_current;
  attachify(u);

  // Make a new attached set
  node *v = new node(m_R.add_node());
  m_R.add_edge(u->find()->id, v->id);
  assert(v->find() == v);
  v->att_pred = v->att_succ = v; // self

  // XXX: Not in the pseudocode but I don't see how you couldn't have this.
  node *w = fut->put_strand;
  assert(w->find()->attached());
  m_R.add_edge(w->find()->id, v->id);
  //printf("Get: %lu -cont-> %lu, %lu -touch-> %lu\n",
  //u->find()->id, v->id, w->find()->id, v->id);

  t_current = v;
  t_current->sbag = f->sp.Sbag;
}

node* nonblock::perform_join(sframe_data *f) {

  int ind = 0;
  int num_attached = 0; // counting number of attached branch
  int num_jparents = f->fork_stack.size() + 1;
  node *fake_rjp = t_current;
  assert(t_current->find()->att_pred);

  // nodes going into join node in reverse serial order, i.e., t_current is 0th
  node *join_parents[num_jparents];
  join_parents[ind++] = t_current;

  node *j = new node(); // join node: the continuation after sync
  assert(j->find() == j);

  if(t_current->find()->attached()) { num_attached++; }

  // we first process all the binary fork in reversed sequential order
  while(f->fork_stack.empty() == false) {
    // we process the fork node in reversed sequential order
    fork_node_data *fndata = f->fork_stack.head();
    handle_binary_fork_at_sync(fndata->fork, fndata->lfc, fndata->rfc, fndata->ljp, fake_rjp);
    fake_rjp = fndata->fork;
    if(fake_rjp->find()->attached()) { num_attached++; }
    join_parents[ind++] = fndata->ljp; // needed for processing join node
    f->fork_stack.pop();
    // should have been merged into the same unattached set if nothing is attached
    assert(num_attached > 0 || t_current->find() == fake_rjp->find());
  }
  assert(ind == num_jparents);

  // then, based on the number of attached join parent, we process the join
  if(num_attached == 0) {
    t_current->merge(j); // merge j into it
    assert(!j->find()->attached() && j->find()->att_pred);
  } else if(num_attached == 1) {
    for(int i=0; i < num_jparents; i++) {
      if( join_parents[i]->find()->attached() ) {
        join_parents[i]->merge(j); // merge j into the single attached parent
      }
    }
  } else { // multiple attached join parent
    assert(j->id == 0);
    j->id = m_R.add_node(); // make j into its own attached set
    j->att_pred = j->att_succ = j;
  }

  if(num_attached > 0) {
    assert(j->find()->attached());

    // now that the attached set for the join is setup, we can add edges from
    // attached parent to it and make it the attached successor for
    // unattached parents

    //reach::general::node attached_parents[num_jparents];
    //int num_edges = 0;
    for(int i=0; i < num_jparents; i++) {
      node *pset = join_parents[i]->find();
      if(!pset->attached()) { pset->att_succ = j->find(); }
      else {
        m_R.add_edge(pset->find()->id, j->find()->id);
        //attached_parents[num_edges++] = pset->find()->id;
      }
    }
    //m_R.add_edges_from(attached_parents, num_edges, j->find()->id);
  }

  assert(j->find()->att_pred);
  return j;
}

// we don't actually have rjp for every fork. Instead, we pass in the
// most-recently processed fork node as a proxy (and we process them in
// reversed sequential order).
// For a given fork node, if both branches are unattached, the fork node would
// be unattached and merged into the same unattached set; if at least one branch
// is attached, the fork will be attached, and that corresponding right-branch for
// the next fork is also attached.  Since we don't do anything with the set of
// rjp unless both branches are attached, it's ok to use the last-processed fork node
// as a proxy for rjp for the next fork,
void nonblock::handle_binary_fork_at_sync(node *f, // fork node
                                          node *lfc, node *rfc, // left, right fork children
                                          node *ljp, node *rjp  // left, (fake) right join parent
                                          ) {
  assert(f && lfc && rfc && ljp && rjp);
  bool ljp_attached = ljp->find()->attached();
  bool rjp_attached = rjp->find()->attached();

  // No non-SP edges
  if (!ljp_attached && !rjp_attached) {
    assert(lfc->find() == ljp->find() && rfc->find() == rjp->find());
    assert(f->find()->att_pred == ljp->find()->att_pred);
    assert(f->find()->att_pred == rjp->find()->att_pred);
    f->merge(ljp);
    f->merge(rjp);

    // Both sides incident on non-SP edges
  } else if (ljp_attached && rjp_attached) {
    attachify(f);
    assert(f->find()->attached());
    assert(lfc->find()->attached());
    assert(rfc->find()->attached());
    m_R.add_edge(f->find()->id, lfc->find()->id);
    m_R.add_edge(f->find()->id, rfc->find()->id);

  } else {
    node *att_succ, *unatt_succ; // attached/unattached fork children
    if (ljp_attached) {
      att_succ = lfc->find(); unatt_succ = rfc->find();
    } else {
      att_succ = rfc->find(); unatt_succ = lfc->find();
    }
    assert(att_succ->attached());
    assert(!unatt_succ->attached());

    if (!f->find()->attached()) { att_succ->merge(f); }
    assert(f->find()->attached());
  }
}

// Currently we only support binary join
void nonblock::at_sync(sframe_data *f) {

  m_sp.at_sync(&f->sp);

  assert(f->fork_stack.size() != 0);
  t_current = perform_join(f); // perform_join returns the join node
  t_current->sbag = f->sp.Sbag;
}

/********** Continuations **********/

// f is the helper frame, p is parent
void nonblock::at_spawn_continuation(sframe_data *f, sframe_data *p) {
  // f is the spawn helper, so in principle it should have seen only one spawn
  assert(f->fork_stack.size() == 1);
  // with eager execution of future, we should have unset it already even if
  // we created a future within the spawned subcomputation
  assert(f->future_fork == nullptr);
  fork_node_data *fndata = f->fork_stack.head();
  assert(fndata->fork && fndata->lfc && !fndata->rfc);

  m_sp.at_spawn_continuation(&f->sp, &p->sp);

  fndata->ljp = t_current;
  t_current = new node();
  t_current->sbag = p->sp.Sbag;
  assert(t_current->find() == t_current);
  fndata->rfc = t_current;
  fndata->rfc->att_pred = fndata->fork->find()->att_pred;
  copy_fork_stack_data(p->fork_stack.push(), fndata); // p = dst, f = source
}

// f is the current head and p is the parent
void nonblock::at_future_continuation(sframe_data *f, sframe_data *p) {
  // Use p because the pop hasn't happened yet
  assert(f->future_fork);
  node *w = new node(m_R.add_node()); // right
  assert(w->find() == w);

  w->att_pred = w->att_succ = w; // self
  m_R.add_edge(f->future_fork->find()->id, w->id);
  //printf("Fork: %lu -> %lu\n", f->future_fork->find()->id, w->id);
  f->future_fork = nullptr;
  t_current = w;

  t_current->sbag = p->sp.Sbag; // w represents the continuation in the parent
  // t_current->sbag = f->sp.Sbag;
  // p->sp = f->sp; // move the bags over
}

// Called when a future task completes. We know we're going to have
// non-SP edges out from here, we just don't know where to yet.
// f is the current head (frame for future) and p is the parent
void nonblock::at_future_finish(sframe_data *f, sframe_data *p, sfut_data* fut) {
  // Save for later, i.e. at_get
  fut->put_strand = t_current;

  f->sp.Sbag->set_kind(spbag::bag_kind::P); // now that future is done, it's a P bag

  // Conceptually this happens at get(), but doing it here prevents it
  // from happening multiple times if get() is called multiple
  // times. However, calling it multiple times shouldn't affect
  // correctness.
  attachify(t_current);

  // The only way we know of a future continuation, currently.
  at_future_continuation(f, p);
}

} // namespace reach
