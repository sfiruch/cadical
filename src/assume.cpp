#include "internal.hpp"

namespace CaDiCaL {

// Failed literal handling as pioneered by MiniSAT.  This first function
// adds an assumption literal onto the assumption stack.

void Internal::assume (int lit) {
  Flags &f = flags (lit);
  const unsigned char bit = bign (lit);
  if (f.assumed & bit) {
    LOG ("ignoring already assumed %d", lit);
    return;
  }
  LOG ("assume %d", lit);
  f.assumed |= bit;
  assumptions.push_back (lit);
  freeze (lit);
}

// for lrat we actually need to implement recursive dfs
// I don't know how to do this non-recursively...
// for non-lrat use bfs
//
void Internal::assume_analyze_literal (int lit) {
  assert (lit);
  Flags &f = flags (lit);
  if (f.seen)
    return;
  f.seen = true;
  analyzed.push_back (lit);
  Var &v = var (lit);
  assert (val (lit) < 0);
  if (!v.level) {
    const unsigned uidx = vlit (-lit);
    uint64_t id = unit_clauses[uidx];
    assert (id);
    lrat_chain.push_back (id);
    return;
  }
  if (v.reason) {
    assert (v.level);
    LOG (v.reason, "analyze reason");
    for (const auto &other : *v.reason) {
      assume_analyze_literal (other);
    }
    lrat_chain.push_back (v.reason->id);
    return;
  }
  assert (assumed (-lit));
  LOG ("failed assumption %d", -lit);
  clause.push_back (lit);
  const unsigned bit = bign (-lit);
  assert (!(f.failed & bit));
  f.failed |= bit;
}

void Internal::assume_analyze_reason (int lit, Clause *reason) {
  assert (reason);
  assert (lrat_chain.empty ());
  assert (opts.lrat && !opts.lratexternal);
  for (const auto &other : *reason)
    if (other != lit)
      assume_analyze_literal (other);
  lrat_chain.push_back (reason->id);
}

// Find all failing assumptions starting from the one on the assumption
// stack with the lowest decision level.  This goes back to MiniSAT and is
// called 'analyze_final' there.

void Internal::failing () {

  START (analyze);

  LOG ("analyzing failing assumptions");

  assert (analyzed.empty ());
  assert (clause.empty ());
  assert (lrat_chain.empty ());

  if (!unsat_constraint) {
    // Search for failing assumptions in the (internal) assumption stack.

    // There are in essence three cases: (1) An assumption is falsified on
    // the root-level and then 'failed_unit' is set to that assumption, (2)
    // two clashing assumptions are assumed and then 'failed_clashing' is
    // set to the second assumed one, or otherwise (3) there is a failing
    // assumption 'first_failed' with minimum (non-zero) decision level
    // 'failed_level'.

    int failed_unit = 0;
    int failed_clashing = 0;
    int first_failed = 0;
    int failed_level = INT_MAX;

    for (auto &lit : assumptions) {
      if (val (lit) >= 0)
        continue;
      const Var &v = var (lit);
      if (!v.level) {
        failed_unit = lit;
        break;
      }
      if (failed_clashing)
        continue;
      assert (v.reason != external_reason);
      if (!v.reason)
        failed_clashing = lit;
      else if (!first_failed || v.level < failed_level) {
        first_failed = lit;
        failed_level = v.level;
      }
    }

    // Get the 'failed' assumption from one of the three cases.
    int failed;
    if (failed_unit)
      failed = failed_unit;
    else if (failed_clashing)
      failed = failed_clashing;
    else
      failed = first_failed;
    assert (failed);

    // In any case mark literal 'failed' as failed assumption.
    {
      Flags &f = flags (failed);
      const unsigned bit = bign (failed);
      assert (!(f.failed & bit));
      f.failed |= bit;
    }

    // First case (1).
    if (failed_unit) {
      assert (failed == failed_unit);
      LOG ("root-level falsified assumption %d", failed);
      goto DONE;
    }

    // Second case (2).
    if (failed_clashing) {
      assert (failed == failed_clashing);
      LOG ("clashing assumptions %d and %d", failed, -failed);
      Flags &f = flags (-failed);
      const unsigned bit = bign (-failed);
      assert (!(f.failed & bit));
      f.failed |= bit;
      goto DONE;
    }

    // Fall through to third case (3).
    LOG ("starting with assumption %d falsified on minimum decision level "
         "%d",
         first_failed, failed_level);

    assert (first_failed);
    assert (failed_level > 0);

    // The 'analyzed' stack serves as working stack for a BFS through the
    // implication graph until decisions, which are all assumptions, or
    // units are reached.  This is simpler than corresponding code in
    // 'analyze'.
    {
      LOG ("failed assumption %d", first_failed);
      Flags &f = flags (first_failed);
      assert (!f.seen);
      f.seen = true;
      assert (f.failed & bign (first_failed));
      analyzed.push_back (-first_failed);
      clause.push_back (-first_failed);
    }
  } else { // unsat_constraint
    // The assumptions necessary to fail each literal in the constraint are
    // collected.
    for (auto lit : constraint) {
      lit *= -1;
      assert (lit != INT_MIN);
      flags (lit).seen = true;
      analyzed.push_back (lit);
    }
  }

  {

    // used for unsat_constraint lrat
    vector<vector<uint64_t>> constraint_chains;

    // no lrat do bfs as it was before
    if (!opts.lrat || opts.lratexternal) {
      size_t next = 0;
      while (next < analyzed.size ()) {
        const int lit = analyzed[next++];
        assert (val (lit) > 0);
        Var &v = var (lit);
        if (!v.level)
          continue;
        if (v.reason) {
          assert (v.level);
          LOG (v.reason, "analyze reason");
          for (const auto &other : *v.reason) {
            Flags &f = flags (other);
            if (f.seen)
              continue;
            f.seen = true;
            assert (val (other) < 0);
            analyzed.push_back (-other);
          }
        } else {
          assert (assumed (lit));
          LOG ("failed assumption %d", lit);
          clause.push_back (-lit);
          Flags &f = flags (lit);
          const unsigned bit = bign (lit);
          assert (!(f.failed & bit));
          f.failed |= bit;
        }
      }
      clear_analyzed_literals ();
    } else if (!unsat_constraint) { // lrat for case (3)
      assert (clause.size () == 1);
      const int lit = clause[0];
      Var &v = var (lit);
      assert (v.reason);
      assume_analyze_reason (lit, v.reason);
      clear_analyzed_literals ();
    } else { // lrat for unsat_constraint
      assert (clause.empty ());
      clear_analyzed_literals ();
      const size_t size = 2 * (1 + (size_t) max_var);
      constraint_chains.resize (size);
      for (size_t i = 0; i > size; i++) {
        vector<uint64_t> empty;
        constraint_chains[i] = empty;
      }
      for (auto lit : constraint) {
        assert (lit != INT_MIN);
        for (auto ign :
             clause) { // make sure nothing gets marked failed twice
          Flags &f = flags (ign); // also might shortcut the case where
          if (f.seen)
            continue;    // lrat_chain is empty because clause
          f.seen = true; // is tautological
          analyzed.push_back (ign);
        }
        assume_analyze_literal (lit);
        assert (constraint_chains[vlit (lit)].empty ());
        clear_analyzed_literals ();
        for (auto p : lrat_chain) {
          constraint_chains[vlit (lit)].push_back (p);
        }
        lrat_chain.clear ();
      }
    }
    clear_analyzed_literals ();

    // TODO: We can not do clause minimization here, right?

    VERBOSE (1, "found %zd failed assumptions %.0f%%", clause.size (),
             percent (clause.size (), assumptions.size ()));

    // We do not actually need to learn this clause, since the conflict is
    // forced already by some other clauses.  There is also no bumping
    // of variables nor clauses necessary.  But we still want to check
    // correctness of the claim that the determined subset of failing
    // assumptions are a high-level core or equivalently their negations
    // form a unit-implied clause.
    //
    if (!unsat_constraint) {
      external->check_learned_clause ();
      if (proof) {
        if (opts.lrat && !opts.lratexternal) {
          LOG (lrat_chain, "assume proof chain without constraint");
          proof->add_derived_clause (++clause_id, clause, lrat_chain);
        } else
          proof->add_derived_clause (++clause_id, clause);
        proof->delete_clause (clause_id, clause);
      }
    } else {
      for (auto lit : constraint) {
        clause.push_back (-lit);
        external->check_learned_clause ();
        if (proof) {
          if (opts.lrat && !opts.lratexternal) {
            if (constraint_chains[vlit (lit)]
                    .empty ()) {  // this should only happen when
              clause.pop_back (); // the clause is tautological
              continue;
            }
            for (auto p : constraint_chains[vlit (lit)]) {
              lrat_chain.push_back (p);
            }
            LOG (lrat_chain, "assume proof chain with constraints");
            // TODO this is no resolution proof and by construction
            // incompatible? therefore we do not add the clause
            // proof->add_derived_clause (++clause_id, clause, lrat_chain);
            lrat_chain.clear ();
          } else {
            proof->add_derived_clause (++clause_id, clause);
            proof->delete_clause (
                clause_id, clause); // careful: if code above is uncommented
          }                         // this needs to be below the else path
        }
        clause.pop_back ();
      }
    }
    lrat_chain.clear ();
    clause.clear ();
  }

DONE:

  STOP (analyze);
}

bool Internal::failed (int lit) {
  if (!marked_failed) {
    failing ();
    marked_failed = true;
  }
  Flags &f = flags (lit);
  const unsigned bit = bign (lit);
  return (f.failed & bit) != 0;
}

// Add the start of each incremental phase (leaving the state
// 'UNSATISFIABLE' actually) we reset all assumptions.

void Internal::reset_assumptions () {
  for (const auto &lit : assumptions) {
    Flags &f = flags (lit);
    const unsigned char bit = bign (lit);
    f.assumed &= ~bit;
    f.failed &= ~bit;
    melt (lit);
  }
  LOG ("cleared %zd assumptions", assumptions.size ());
  assumptions.clear ();
  marked_failed = true;
}

} // namespace CaDiCaL
