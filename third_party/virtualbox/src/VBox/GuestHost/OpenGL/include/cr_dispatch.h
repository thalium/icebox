#ifndef CR_DISPATCH_H
#define CR_DISPATCH_H

#include "spu_dispatch_table.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure offers a set of layers for controlling the
 * dispatch table.  The top layer is always the active layer,
 * and controls the dispatch table.  Utilities may push new
 * layers onto the stack, or pop layers off, to control how
 * dispatches are made.  A utility  may keep track of the
 * layer it created, and change it as needed, if dynamic
 * control of dispatch tables is needed.
 */

typedef struct crDispatchLayer {
    SPUDispatchTable *dispatchTable;
    void (*changedCallback)(SPUDispatchTable *changedTable, void *callbackParm);
    void *callbackParm;
    struct crDispatchLayer *next, *prev;
} crDispatchLayer;

/** This has to be saved and restored for each graphics context */
typedef struct {
    crDispatchLayer *topLayer;
    crDispatchLayer *bottomLayer;
} crCurrentDispatchInfo;

/**
 * These are already called from appropriate opengl_stub
 * routines, so SPUs or utilities shouldn't have to 
 * worry about them, as long as they let their
 * parent routines for NewContext and MakeCurrent
 * execute before they try to use any of the
 * dispatch utilities.
 */
DECLEXPORT(void) crDispatchInit(SPUDispatchTable *defaultTable);
DECLEXPORT(void) crInitDispatchInfo(crCurrentDispatchInfo *info);
DECLEXPORT(void) crSetCurrentDispatchInfo(crCurrentDispatchInfo *info);

/**
 * SPUs and utilities are expected to use these routines
 * to modify dispatch tables.  They should only be called
 * once the current dispatch info is set (i.e., after
 * the opengl_stub routine for MakeCurrent has a chance
 * to run).
 */
DECLEXPORT(crDispatchLayer) *crNewDispatchLayer(SPUDispatchTable *newTable, 
        void (*changedCallback)(SPUDispatchTable *changedTable, void *callbackParm),
	void *callbackParm);
DECLEXPORT(void) crChangeDispatchLayer(crDispatchLayer *layer,
        SPUDispatchTable *changedTable);
DECLEXPORT(void) crFreeDispatchLayer(crDispatchLayer *layer);


#ifdef __cplusplus
}
#endif

#endif /* CR_DISPATCH_H */
