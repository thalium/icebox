/* $Id: tstMvWnd.cpp $ */
/** @file
 * ???
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/win/windows.h>

#define Assert(_m) do {} while (0)
#define vboxVDbgPrint(_m) do {} while (0)

static LRESULT CALLBACK WindowProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    if(uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
//    switch(uMsg)
//    {
//        case WM_CLOSE:
//            vboxVDbgPrint((__FUNCTION__": got WM_CLOSE for hwnd(0x%x)", hwnd));
//            return 0;
//        case WM_DESTROY:
//            vboxVDbgPrint((__FUNCTION__": got WM_DESTROY for hwnd(0x%x)", hwnd));
//            return 0;
//        case WM_NCHITTEST:
//            vboxVDbgPrint((__FUNCTION__": got WM_NCHITTEST for hwnd(0x%x)\n", hwnd));
//            return HTNOWHERE;
//    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#define VBOXDISPWND_NAME L"tstMvWnd"

HRESULT tstMvWndCreate(DWORD w, DWORD h, HWND *phWnd)
{
    HRESULT hr = S_OK;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    /* Register the Window Class. */
    WNDCLASS wc;
    if (!GetClassInfo(hInstance, VBOXDISPWND_NAME, &wc))
    {
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = VBOXDISPWND_NAME;
        if (!RegisterClass(&wc))
        {
            DWORD winErr = GetLastError();
            vboxVDbgPrint((__FUNCTION__": RegisterClass failed, winErr(%d)\n", winErr));
            hr = E_FAIL;
        }
    }

    if (hr == S_OK)
    {
        HWND hWnd = CreateWindowEx (0 /*WS_EX_CLIENTEDGE*/,
                                        VBOXDISPWND_NAME, VBOXDISPWND_NAME,
                                        WS_OVERLAPPEDWINDOW,
                                        0, 0,
                                        w, h,
                                        GetDesktopWindow() /* hWndParent */,
                                        NULL /* hMenu */,
                                        hInstance,
                                        NULL /* lpParam */);
        Assert(hWnd);
        if (hWnd)
        {
            *phWnd = hWnd;
        }
        else
        {
            DWORD winErr = GetLastError();
            vboxVDbgPrint((__FUNCTION__": CreateWindowEx failed, winErr(%d)\n", winErr));
            hr = E_FAIL;
        }
    }

    return hr;
}
static int g_Width = 400;
static int g_Height = 300;
static DWORD WINAPI tstMvWndThread(void *pvUser)
{
    HWND hWnd = (HWND)pvUser;
    RECT Rect;
    BOOL bRc = GetWindowRect(hWnd, &Rect);
    Assert(bRc);
    if (bRc)
    {
        bRc = SetWindowPos(hWnd, HWND_TOPMOST,
          0, /* int X */
          0, /* int Y */
          g_Width, //Rect.left - Rect.right,
          g_Height, //Rect.bottom - Rect.top,
          SWP_SHOWWINDOW);
        Assert(bRc);
        if (bRc)
        {
            int dX = 10, dY = 10;
            int xMin = 5, xMax = 300;
            int yMin = 5, yMax = 300;
            int x = dX, y = dY;
            do
            {
                bRc = SetWindowPos(hWnd, HWND_TOPMOST,
                  x, /* int X */
                  y, /* int Y */
                  g_Width, //Rect.left - Rect.right,
                  g_Height, //Rect.bottom - Rect.top,
                  SWP_SHOWWINDOW);

                x += dX;
                if (x > xMax)
                    x = xMin;
                y += dY;
                if (y > yMax)
                    y = yMin;

                Sleep(5);
            } while(1);
        }
    }

    return 0;
}

int main(int argc, char **argv, char **envp)
{
    HWND hWnd;
    HRESULT hr = tstMvWndCreate(200, 200, &hWnd);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        HANDLE hThread = CreateThread(
                              NULL /* LPSECURITY_ATTRIBUTES lpThreadAttributes */,
                              0 /* SIZE_T dwStackSize */,
                              tstMvWndThread,
                              hWnd,
                              0 /* DWORD dwCreationFlags */,
                              NULL /* pThreadId */);
        Assert(hThread);
        if (hThread)
        {
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        DestroyWindow (hWnd);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
//    NOREF(hInstance); NOREF(hPrevInstance); NOREF(lpCmdLine); NOREF(nCmdShow);

    return main(__argc, __argv, environ);
}
