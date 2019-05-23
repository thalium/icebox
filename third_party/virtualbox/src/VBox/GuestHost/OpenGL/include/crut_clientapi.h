#ifndef CRUT_CLIENTAPI_H
#define CRUT_CLEINTAPI_H

#ifdef WINDOWS
#define CRUT_CLIENT_APIENTRY __stdcall
#else
#define CRUT_CLIENT_APIENTRY
#endif

#include "chromium.h"
#include "crut_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void CRUT_CLIENT_APIENTRY crutInitClient(void);
void CRUT_CLIENT_APIENTRY crutReceiveEventType(int type);
void CRUT_CLIENT_APIENTRY crutMouseFunc( void (*func)(int button, int state, int x, int y) );
void CRUT_CLIENT_APIENTRY crutKeyboardFunc( void (*func) (unsigned char key, int x, int y) );
void CRUT_CLIENT_APIENTRY crutReshapeFunc( void (*func) (int width, int height) );
void CRUT_CLIENT_APIENTRY crutVisibilityFunc( void (*func) (int state) );
void CRUT_CLIENT_APIENTRY crutMotionFunc( void (*func) (int x, int y) );
void CRUT_CLIENT_APIENTRY crutPassiveMotionFunc( void (*func) (int x, int y) );
void CRUT_CLIENT_APIENTRY crutIdleFunc( void (*func)(void));
void CRUT_CLIENT_APIENTRY crutDisplayFunc(void (*func)(void));
void CRUT_CLIENT_APIENTRY crutPostRedisplay(void);
void CRUT_CLIENT_APIENTRY crutMainLoop(void);
int  CRUT_CLIENT_APIENTRY crutCreateContext ( unsigned int visual );
int  CRUT_CLIENT_APIENTRY crutCreateWindow ( unsigned int visual );
void CRUT_CLIENT_APIENTRY crutMakeCurrent( int window, int context );
void CRUT_CLIENT_APIENTRY crutSwapBuffers( int window, int flags );
void CRUT_CLIENT_APIENTRY crutReceiveEvent(CRUTMessage **msg);
int  CRUT_CLIENT_APIENTRY crutCheckEvent( void );
int  CRUT_CLIENT_APIENTRY crutPeekNextEvent( void );
int  CRUT_CLIENT_APIENTRY crutCreateMenu( void (*func) (int val) );
void CRUT_CLIENT_APIENTRY crutAddMenuEntry( char* name, int value );
void CRUT_CLIENT_APIENTRY crutAddSubMenu( char* name, int menuID );
void CRUT_CLIENT_APIENTRY crutAttachMenu( int button );

#ifdef __cplusplus
}
#endif

#endif /* CRUT_CLIENTAPI_H */
