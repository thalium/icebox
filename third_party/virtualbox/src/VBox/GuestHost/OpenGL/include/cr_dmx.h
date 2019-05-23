/**
 * DMX utility functions.
 */

#ifndef CR_DMX_H
#define CR_DMX_H

#include <X11/Xlib.h>
#include <X11/extensions/dmxext.h>

#include "cr_spu.h"
#include "state/cr_statetypes.h"


typedef struct {
    GLXDrawable xwin;     /**< backend server's X window */
    GLXDrawable xsubwin;  /**< child of xwin, clipped to screen bounds */
    Display *dpy;         /**< DMX back-end server display */
    CRrecti visrect;      /**< visible rect, in front-end screen coords */
} CRDMXBackendWindowInfo;


#ifdef __cplusplus
extern "C" {
#endif


extern int
crDMXSupported(Display *dpy);

extern CRDMXBackendWindowInfo *
crDMXAllocBackendWindowInfo(unsigned int numBackendWindows);

extern void
crDMXFreeBackendWindowInfo(unsigned int numBackendWindows,
													 CRDMXBackendWindowInfo *backendWindows);

/* Given the DMX front-end display "dpy" and window "xwin", update the
 * backend window information in "backendWindows".  If new subwindows are
 * needed, and an OpenGL interface pointer is provided, use that interface
 * and the subwindowVisBits to create new subwindows.
 */
extern GLboolean
crDMXGetBackendWindowInfo(Display *dpy, GLXDrawable xwin, 
													unsigned int numBackendWindows,
													CRDMXBackendWindowInfo *backendWindows,
													const crOpenGLInterface *openGlInterface,
													GLint subwindowVisBits);


#ifdef __cplusplus
}
#endif

#endif /* CR_DLM_H */
