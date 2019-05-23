

/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define NO_SDL_H
#import "VBoxSDL.h"
#import <Cocoa/Cocoa.h>

void *VBoxSDLGetDarwinWindowId(void)
{
    NSView            *pView = nil;
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    {
        NSApplication *pApp = NSApp;
        NSWindow *pMainWnd;
        pMainWnd = [pApp mainWindow];
        if (!pMainWnd)
            pMainWnd = pApp->_mainWindow; /* UGLY!! but mApp->_AppFlags._active = 0, so mainWindow() fails. */
        pView = [pMainWnd contentView];
    }
    [pPool release];
    return pView;
}

