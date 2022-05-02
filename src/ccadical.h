#ifndef _ccadical_h_INCLUDED
#define _ccadical_h_INCLUDED

/*------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------*/

#include <stdint.h>

#ifndef DllExport
    #define DllExport 
#endif

// C wrapper for CaDiCaL's C++ API following IPASIR.

typedef struct CCaDiCaL CCaDiCaL;

DllExport const char * ccadical_signature (void);
DllExport CCaDiCaL * ccadical_init (void);
DllExport void ccadical_release (CCaDiCaL *);

DllExport void ccadical_add (CCaDiCaL *, int lit);
DllExport void ccadical_assume (CCaDiCaL *, int lit);
DllExport int ccadical_solve (CCaDiCaL *);
DllExport int ccadical_val (CCaDiCaL *, int lit);
DllExport int ccadical_failed (CCaDiCaL *, int lit);

DllExport void ccadical_set_terminate (CCaDiCaL *,
  void * state, int (*terminate)(void * state));

void ccadical_set_learn (CCaDiCaL *,
  void * state, int max_length, void (*learn)(void * state, int * clause));

/*------------------------------------------------------------------------*/

// Non-IPASIR conformant 'C' functions.

DllExport void ccadical_constrain (CCaDiCaL *, int lit);
DllExport int ccadical_constraint_failed (CCaDiCaL *);
DllExport void ccadical_set_option (CCaDiCaL *, const char * name, int val);
DllExport void ccadical_limit (CCaDiCaL *, const char * name, int limit);
DllExport int ccadical_get_option (CCaDiCaL *, const char * name);
DllExport void ccadical_print_statistics (CCaDiCaL *);
DllExport int64_t ccadical_active (CCaDiCaL *);
DllExport int64_t ccadical_irredundant (CCaDiCaL *);
DllExport int ccadical_fixed (CCaDiCaL *, int lit);
DllExport void ccadical_terminate (CCaDiCaL *);
DllExport void ccadical_freeze (CCaDiCaL *, int lit);
DllExport int ccadical_frozen (CCaDiCaL *, int lit);
DllExport void ccadical_melt (CCaDiCaL *, int lit);
DllExport int ccadical_simplify (CCaDiCaL *);

/*------------------------------------------------------------------------*/

// Support legacy names used before moving to more IPASIR conforming names.

#define ccadical_reset ccadical_release
#define ccadical_sat ccadical_solve
#define ccadical_deref ccadical_val

/*------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------*/

#endif
