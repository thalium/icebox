/** @file
 *
 * VBox extension to Wine D3D
 *
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___VBoxWineEx_h__
#define ___VBoxWineEx_h__

typedef enum
{
    VBOXWINEEX_SHRC_STATE_UNDEFINED = 0,
    /* the underlying GL resource can not be used because it can be removed concurrently by other SHRC client */
    VBOXWINEEX_SHRC_STATE_GL_DISABLE,
    /* the given client is requested to delete the underlying GL resource on SHRC termination */
    VBOXWINEEX_SHRC_STATE_GL_DELETE
} VBOXWINEEX_SHRC_STATE;


#ifndef IN_VBOXLIBWINE

#define VBOXWINEEX_VERSION 1

#ifndef IN_VBOXWINEEX
# define VBOXWINEEX_DECL(_type)   __declspec(dllimport) _type WINAPI
# else
# define VBOXWINEEX_DECL(_type)  __declspec(dllexport) _type WINAPI
#endif

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_CREATETEXTURE(IDirect3DDevice9Ex *iface,
            UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
            D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *shared_handle,
            void **pavClientMem);
typedef FNVBOXWINEEXD3DDEV9_CREATETEXTURE *PFNVBOXWINEEXD3DDEV9_CREATETEXTURE;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_CREATECUBETEXTURE(IDirect3DDevice9Ex *iface,
            UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, 
            D3DPOOL pool, IDirect3DCubeTexture9 **texture, HANDLE *shared_handle,
            void **pavClientMem);
typedef FNVBOXWINEEXD3DDEV9_CREATECUBETEXTURE *PFNVBOXWINEEXD3DDEV9_CREATECUBETEXTURE;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_CREATEVOLUMETEXTURE(IDirect3DDevice9Ex *iface,
            UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT Format, D3DPOOL Pool,
            IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle,
            void **pavClientMem);
typedef FNVBOXWINEEXD3DDEV9_CREATEVOLUMETEXTURE *PFNVBOXWINEEXD3DDEV9_CREATEVOLUMETEXTURE;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_VOLBLT(IDirect3DDevice9Ex *iface,
                                                    IDirect3DVolume9 *pSourceVolume, IDirect3DVolume9 *pDestinationVolume,
                                                    const struct VBOXBOX3D *pSrcBoxArg,
                                                    const struct VBOXPOINT3D *pDstPoin3D);
typedef FNVBOXWINEEXD3DDEV9_VOLBLT *PFNVBOXWINEEXD3DDEV9_VOLBLT;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_VOLTEXBLT(IDirect3DDevice9Ex *iface,
                                                    IDirect3DVolumeTexture9 *pSourceTexture, IDirect3DVolumeTexture9 *pDestinationTexture,
                                                    const struct VBOXBOX3D *pSrcBoxArg,
                                                    const struct VBOXPOINT3D *pDstPoin3D);
typedef FNVBOXWINEEXD3DDEV9_VOLTEXBLT *PFNVBOXWINEEXD3DDEV9_VOLTEXBLT;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_FLUSH(IDirect3DDevice9Ex *iface);
typedef FNVBOXWINEEXD3DDEV9_FLUSH *PFNVBOXWINEEXD3DDEV9_FLUSH;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_FLUSHTOHOST(IDirect3DDevice9Ex *iface);
typedef FNVBOXWINEEXD3DDEV9_FLUSHTOHOST *PFNVBOXWINEEXD3DDEV9_FLUSHTOHOST;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_FINISH(IDirect3DDevice9Ex *iface);
typedef FNVBOXWINEEXD3DDEV9_FINISH *PFNVBOXWINEEXD3DDEV9_FINISH;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_UPDATE(IDirect3DDevice9Ex *iface, D3DPRESENT_PARAMETERS * pParams, IDirect3DDevice9Ex **outIface);
typedef FNVBOXWINEEXD3DDEV9_UPDATE *PFNVBOXWINEEXD3DDEV9_UPDATE;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DDEV9_TERM(IDirect3DDevice9Ex *iface);
typedef FNVBOXWINEEXD3DDEV9_TERM *PFNVBOXWINEEXD3DDEV9_TERM;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DRC9_SETSHRCSTATE(IDirect3DResource9 *iface, VBOXWINEEX_SHRC_STATE enmState);
typedef FNVBOXWINEEXD3DRC9_SETSHRCSTATE *PFNVBOXWINEEXD3DRC9_SETSHRCSTATE;

typedef VBOXWINEEX_DECL(HRESULT) FNVBOXWINEEXD3DSWAPCHAIN9_PRESENT(IDirect3DSwapChain9 *iface, IDirect3DSurface9 *surf);
typedef FNVBOXWINEEXD3DSWAPCHAIN9_PRESENT *PFNVBOXWINEEXD3DSWAPCHAIN9_PRESENT;

#ifdef __cplusplus
extern "C"
{
#endif
VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9CreateTexture(IDirect3DDevice9Ex *iface,
            UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
            D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *shared_handle,
            void **pavClientMem); /* <- extension arg to pass in the client memory buffer,
                                 *    applicable ONLY for SYSMEM textures */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9CreateCubeTexture(IDirect3DDevice9Ex *iface,
            UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format,
            D3DPOOL pool, IDirect3DCubeTexture9 **texture, HANDLE *shared_handle,
            void **pavClientMem); /* <- extension arg to pass in the client memory buffer,
                                 *    applicable ONLY for SYSMEM textures */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9CreateVolumeTexture(IDirect3DDevice9Ex *iface,
            UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT Format, D3DPOOL Pool,
            IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle,
            void **pavClientMem);

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9VolBlt(IDirect3DDevice9Ex *iface,
                                                    IDirect3DVolume9 *pSourceVolume, IDirect3DVolume9 *pDestinationVolume,
                                                    const struct VBOXBOX3D *pSrcBoxArg,
                                                    const struct VBOXPOINT3D *pDstPoin3D);

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9VolTexBlt(IDirect3DDevice9Ex *iface,
                                                    IDirect3DVolumeTexture9 *pSourceTexture, IDirect3DVolumeTexture9 *pDestinationTexture,
                                                    const struct VBOXBOX3D *pSrcBoxArg,
                                                    const struct VBOXPOINT3D *pDstPoin3D);

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9Flush(IDirect3DDevice9Ex *iface); /* perform glFlush */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9Finish(IDirect3DDevice9Ex *iface); /* perform glFinish */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9FlushToHost(IDirect3DDevice9Ex *iface); /* flash data to host */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9Update(IDirect3DDevice9Ex *iface, D3DPRESENT_PARAMETERS * pParams,
                                                    IDirect3DDevice9Ex **outIface); /* update device parameters */

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DDev9Term(IDirect3DDevice9Ex *iface);

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DRc9SetShRcState(IDirect3DResource9 *iface, VBOXWINEEX_SHRC_STATE enmState);

VBOXWINEEX_DECL(HRESULT) VBoxWineExD3DSwapchain9Present(IDirect3DSwapChain9 *iface,
                                IDirect3DSurface9 *surf); /* use the given surface as a frontbuffer content source */

typedef struct VBOXWINEEX_D3DPRESENT_PARAMETERS
{
    D3DPRESENT_PARAMETERS Base;
    struct VBOXUHGSMI *pHgsmi;
} VBOXWINEEX_D3DPRESENT_PARAMETERS, *PVBOXWINEEX_D3DPRESENT_PARAMETERS;
#ifdef __cplusplus
}
#endif

#endif /* #ifndef IN_VBOXLIBWINE */

#endif
