#ifndef CRUTAPI_H
#define CRUTAPI_H

#ifdef WINDOWS
#define CRUT_APIENTRY __stdcall
#else
#define CRUT_APIENTRY
#endif

#include "cr_net.h"
#include "cr_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compile-time version tests */
#define CRUT_MAJOR_VERSION 1
#define CRUT_MINOR_VERSION 9
#define CRUT_PATCH_VERSION 0

/* Display mode bit masks. */
#define CRUT_RGB			0
#define CRUT_RGBA			GLUT_RGB
#define CRUT_INDEX			1
#define CRUT_SINGLE			0
#define CRUT_DOUBLE			2
#define CRUT_ACCUM			4
#define CRUT_ALPHA			8
#define CRUT_DEPTH			16
#define CRUT_STENCIL			32

/* Mouse buttons. */
/* CRUT_NO_BUTTON provided for menu definition.  Not every menu will
   have a button associated with it. */
#define CRUT_NO_BUTTON                  -1
#define CRUT_LEFT_BUTTON		0
#define CRUT_MIDDLE_BUTTON		1
#define CRUT_RIGHT_BUTTON		2

/* Mouse button state. */
#define CRUT_DOWN			0
#define CRUT_UP				1

/* Visibility  state. */
#define CRUT_NOT_VISIBLE		0
#define CRUT_VISIBLE			1

/* function keys */
#define CRUT_KEY_F1			1
#define CRUT_KEY_F2			2
#define CRUT_KEY_F3			3
#define CRUT_KEY_F4			4
#define CRUT_KEY_F5			5
#define CRUT_KEY_F6			6
#define CRUT_KEY_F7			7
#define CRUT_KEY_F8			8
#define CRUT_KEY_F9			9
#define CRUT_KEY_F10			10
#define CRUT_KEY_F11			11
#define CRUT_KEY_F12			12
/* directional keys */
#define CRUT_KEY_LEFT			100
#define CRUT_KEY_UP			101
#define CRUT_KEY_RIGHT			102
#define CRUT_KEY_DOWN			103
#define CRUT_KEY_PAGE_UP		104
#define CRUT_KEY_PAGE_DOWN		105
#define CRUT_KEY_HOME			106
#define CRUT_KEY_END			107
#define CRUT_KEY_INSERT			108

/* event types */
#define CRUT_NO_EVENT                   0
#define CRUT_MOUSE_EVENT                1
#define CRUT_RESHAPE_EVENT              2
#define CRUT_KEYBOARD_EVENT             3
#define CRUT_MOTION_EVENT               4
#define CRUT_PASSIVE_MOTION_EVENT       5
#define CRUT_MENU_EVENT                 6
#define CRUT_VISIBILITY_EVENT           7

#define DEFAULT_CRUT_PORT 9000

#define MENU_MAX_SIZE 1024
#define MENU_ITEM_REGULAR 0
#define MENU_ITEM_SUBMENU 1

/* CRUTMessage is just a placeholder to find the msg_type */
typedef struct
{
	CRMessageHeader header;
	int msg_type;

} CRUTMessage;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    int button;
    int state;
    int x;
    int y;

} CRUTMouseMsg;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    int width;
    int height;

} CRUTReshapeMsg;

typedef struct
{
	CRMessageHeader header;
	int msg_type;
	int state;
	
} CRUTVisibilityMsg;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    unsigned char key;
    int x;
    int y;

} CRUTKeyboardMsg;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    int x;
    int y;

} CRUTMotionMsg;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    int x;
    int y;

} CRUTPassiveMotionMsg;

typedef struct
{
    CRMessageHeader header;
    int msg_type;
    int menuID;
    int value;

} CRUTMenuMsg;

typedef struct
{
    unsigned short tcpip_port;
    int mtu;
    char protocol[1024];
    CRConnection *send_conn;

} CRUTClientPointer;

typedef struct
{
    CRUTClientPointer *crutclients;
    CRConnection *mothershipConn;
    int numclients;
    int winX;
    int winY;
    int winWidth;
    int winHeight;

    int compositeAlpha;
    int compositeDepth;

    char menuBuffer[MENU_MAX_SIZE];

} CRUTAPI;

#define MAX_MSG_SIZE sizeof(CRUTMouseMsg)


/* extern CRUTAPI crut_api;*/

void CRUT_APIENTRY crutInitAPI( CRUTAPI *crut_api, const char *mothership );
void CRUT_APIENTRY crutGetWindowParams( CRUTAPI *crut_api );
void CRUT_APIENTRY crutGetMenuXML( CRUTAPI *crut_api );
void CRUT_APIENTRY crutSetWindowID( CRUTAPI *crut_api, int windowID );
void CRUT_APIENTRY crutConnectToClients( CRUTAPI *crut_api );
void crutSendEvent( CRUTAPI *crut_api, void *msg, int size );
void CRUT_APIENTRY crutSendMouseEvent( CRUTAPI *crut_api, int button, int state, int x, int y );
void CRUT_APIENTRY crutSendKeyboardEvent( CRUTAPI *crut_api, int key, int x, int y );
void CRUT_APIENTRY crutSendReshapeEvent( CRUTAPI *crut_api, int width, int height );
void CRUT_APIENTRY crutSendVisibilityEvent( CRUTAPI *crut_api, int state );
void CRUT_APIENTRY crutSendMotionEvent( CRUTAPI *crut_api, int x, int y );
void CRUT_APIENTRY crutSendPassiveMotionEvent( CRUTAPI *crut_api, int x, int y );
void CRUT_APIENTRY crutSendMenuEvent( CRUTAPI *crut_api, int menuID, int value );

#ifdef __cplusplus
}
#endif

#endif /* CRUTAPI_H */
