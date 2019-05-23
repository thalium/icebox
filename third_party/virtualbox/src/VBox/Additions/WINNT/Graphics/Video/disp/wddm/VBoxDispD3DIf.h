/* $Id: VBoxDispD3DIf.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxDispD3DIf_h___
#define ___VBoxDispD3DIf_h___

/* D3D headers */
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/win/d3d9.h>
#include "../../../Wine_new/vbox/VBoxWineEx.h"

/* D3D functionality the VBOXDISPD3D provides */
typedef HRESULT WINAPI FNVBOXDISPD3DCREATE9EX(UINT SDKVersion, IDirect3D9Ex **ppD3D);
typedef FNVBOXDISPD3DCREATE9EX *PFNVBOXDISPD3DCREATE9EX;

typedef struct VBOXDISPD3D
{
    /* D3D functionality the VBOXDISPD3D provides */
    PFNVBOXDISPD3DCREATE9EX pfnDirect3DCreate9Ex;

    PFNVBOXWINEEXD3DDEV9_CREATETEXTURE pfnVBoxWineExD3DDev9CreateTexture;

    PFNVBOXWINEEXD3DDEV9_CREATECUBETEXTURE pfnVBoxWineExD3DDev9CreateCubeTexture;

    PFNVBOXWINEEXD3DDEV9_CREATEVOLUMETEXTURE pfnVBoxWineExD3DDev9CreateVolumeTexture;

    PFNVBOXWINEEXD3DDEV9_FLUSH pfnVBoxWineExD3DDev9Flush;

    PFNVBOXWINEEXD3DDEV9_VOLBLT pfnVBoxWineExD3DDev9VolBlt;

    PFNVBOXWINEEXD3DDEV9_VOLTEXBLT pfnVBoxWineExD3DDev9VolTexBlt;

    PFNVBOXWINEEXD3DDEV9_TERM pfnVBoxWineExD3DDev9Term;

    PFNVBOXWINEEXD3DSWAPCHAIN9_PRESENT pfnVBoxWineExD3DSwapchain9Present;

    PFNVBOXWINEEXD3DDEV9_FLUSHTOHOST pfnVBoxWineExD3DDev9FlushToHost;

    PFNVBOXWINEEXD3DDEV9_FINISH pfnVBoxWineExD3DDev9Finish;

    PFNVBOXWINEEXD3DSURF9_GETHOSTID pfnVBoxWineExD3DSurf9GetHostId;

    PFNVBOXWINEEXD3DSURF9_SYNCTOHOST pfnVBoxWineExD3DSurf9SyncToHost;

    PFNVBOXWINEEXD3DSWAPCHAIN9_GETHOSTWINID pfnVBoxWineExD3DSwapchain9GetHostWinID;

    PFNVBOXWINEEXD3DDEV9_GETHOSTID pfnVBoxWineExD3DDev9GetHostId;

    /* module handle */
    HMODULE hD3DLib;
} VBOXDISPD3D;

typedef struct VBOXWDDMDISP_FORMATS
{
    uint32_t cFormstOps;
    const struct _FORMATOP* paFormstOps;
    uint32_t cSurfDescs;
    struct _DDSURFACEDESC *paSurfDescs;
} VBOXWDDMDISP_FORMATS, *PVBOXWDDMDISP_FORMATS;

typedef struct VBOXWDDMDISP_D3D
{
    VBOXDISPD3D D3D;
    IDirect3D9Ex * pD3D9If;
    D3DCAPS9 Caps;
    UINT cMaxSimRTs;
} VBOXWDDMDISP_D3D, *PVBOXWDDMDISP_D3D;

void VBoxDispD3DGlobalInit(void);
void VBoxDispD3DGlobalTerm(void);
HRESULT VBoxDispD3DGlobalOpen(PVBOXWDDMDISP_D3D pD3D, PVBOXWDDMDISP_FORMATS pFormats);
void VBoxDispD3DGlobalClose(PVBOXWDDMDISP_D3D pD3D, PVBOXWDDMDISP_FORMATS pFormats);

HRESULT VBoxDispD3DOpen(VBOXDISPD3D *pD3D);
void VBoxDispD3DClose(VBOXDISPD3D *pD3D);

#ifdef VBOX_WITH_VIDEOHWACCEL
HRESULT VBoxDispD3DGlobal2DFormatsInit(struct VBOXWDDMDISP_ADAPTER *pAdapter);
void VBoxDispD3DGlobal2DFormatsTerm(struct VBOXWDDMDISP_ADAPTER *pAdapter);
#endif

#endif /* ifndef ___VBoxDispD3DIf_h___ */
