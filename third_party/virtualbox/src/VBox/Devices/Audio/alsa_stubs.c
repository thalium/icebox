/* $Id: alsa_stubs.c $ */
/** @file
 * Stubs for libasound.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <iprt/assert.h>
#include <iprt/ldr.h>
#include <VBox/log.h>
#include <VBox/err.h>

#include <alsa/asoundlib.h>

#include "alsa_stubs.h"

#define VBOX_ALSA_LIB "libasound.so.2"

#define PROXY_STUB(function, rettype, signature, shortsig) \
    static rettype (*pfn_ ## function) signature; \
    \
    rettype VBox_##function signature; \
    rettype VBox_##function signature \
    { \
        return pfn_ ## function shortsig; \
    }

PROXY_STUB(snd_device_name_hint, int,
           (int card, const char *iface, void ***hints),
           (card, iface, hints))
PROXY_STUB(snd_device_name_free_hint, int,
           (void **hints),
           (hints))
PROXY_STUB(snd_device_name_get_hint, char *,
           (const void *hint, const char *id),
           (hint, id))

PROXY_STUB(snd_pcm_hw_params_any, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params),
           (pcm, params))
PROXY_STUB(snd_pcm_close, int, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_avail_update, snd_pcm_sframes_t, (snd_pcm_t *pcm),
           (pcm))
PROXY_STUB(snd_pcm_hw_params_set_channels_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val),
           (pcm, params, val))
PROXY_STUB(snd_pcm_hw_params_set_period_time_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir),
           (pcm, params, val, dir))
PROXY_STUB(snd_pcm_prepare, int, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_sw_params_sizeof, size_t, (void), ())
PROXY_STUB(snd_pcm_hw_params_set_period_size_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir),
           (pcm, params, val, dir))
PROXY_STUB(snd_pcm_hw_params_get_period_size, int,
           (const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir),
           (params, frames, dir))
PROXY_STUB(snd_pcm_hw_params, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params),
           (pcm, params))
PROXY_STUB(snd_pcm_hw_params_sizeof, size_t, (void), ())
PROXY_STUB(snd_pcm_state, snd_pcm_state_t, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_open, int,
           (snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode),
           (pcm, name, stream, mode))
PROXY_STUB(snd_lib_error_set_handler, int, (snd_lib_error_handler_t handler),
           (handler))
PROXY_STUB(snd_pcm_sw_params, int,
           (snd_pcm_t *pcm, snd_pcm_sw_params_t *params),
           (pcm, params))
PROXY_STUB(snd_pcm_hw_params_get_period_size_min, int,
           (const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir),
           (params, frames, dir))
PROXY_STUB(snd_pcm_writei, snd_pcm_sframes_t,
           (snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size),
           (pcm, buffer, size))
PROXY_STUB(snd_pcm_readi, snd_pcm_sframes_t,
           (snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size),
           (pcm, buffer, size))
PROXY_STUB(snd_strerror, const char *, (int errnum), (errnum))
PROXY_STUB(snd_pcm_start, int, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_drop, int, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_resume, int, (snd_pcm_t *pcm), (pcm))
PROXY_STUB(snd_pcm_hw_params_get_buffer_size, int,
           (const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val),
           (params, val))
PROXY_STUB(snd_pcm_hw_params_set_rate_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir),
           (pcm, params, val, dir))
PROXY_STUB(snd_pcm_hw_params_set_access, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access),
           (pcm, params, _access))
PROXY_STUB(snd_pcm_hw_params_set_buffer_time_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir),
           (pcm, params, val, dir))
PROXY_STUB(snd_pcm_hw_params_set_buffer_size_near, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val),
           (pcm, params, val))
PROXY_STUB(snd_pcm_hw_params_get_buffer_size_min, int,
           (const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val),
           (params, val))
PROXY_STUB(snd_pcm_hw_params_set_format, int,
           (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val),
           (pcm, params, val))
PROXY_STUB(snd_pcm_sw_params_current, int,
           (snd_pcm_t *pcm, snd_pcm_sw_params_t *params),
           (pcm, params))
PROXY_STUB(snd_pcm_sw_params_set_start_threshold, int,
           (snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val),
           (pcm, params, val))
PROXY_STUB(snd_pcm_sw_params_set_avail_min, int,
           (snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val),
           (pcm, params, val))

typedef struct
{
    const char *name;
    void (**fn)(void);
} SHARED_FUNC;

#define ELEMENT(function) { #function , (void (**)(void)) & pfn_ ## function }
static SHARED_FUNC SharedFuncs[] =
{
    ELEMENT(snd_device_name_hint),
    ELEMENT(snd_device_name_get_hint),
    ELEMENT(snd_device_name_free_hint),

    ELEMENT(snd_pcm_hw_params_any),
    ELEMENT(snd_pcm_close),
    ELEMENT(snd_pcm_avail_update),
    ELEMENT(snd_pcm_hw_params_set_channels_near),
    ELEMENT(snd_pcm_hw_params_set_period_time_near),
    ELEMENT(snd_pcm_prepare),
    ELEMENT(snd_pcm_sw_params_sizeof),
    ELEMENT(snd_pcm_hw_params_set_period_size_near),
    ELEMENT(snd_pcm_hw_params_get_period_size),
    ELEMENT(snd_pcm_hw_params),
    ELEMENT(snd_pcm_hw_params_sizeof),
    ELEMENT(snd_pcm_state),
    ELEMENT(snd_pcm_open),
    ELEMENT(snd_lib_error_set_handler),
    ELEMENT(snd_pcm_sw_params),
    ELEMENT(snd_pcm_hw_params_get_period_size_min),
    ELEMENT(snd_pcm_writei),
    ELEMENT(snd_pcm_readi),
    ELEMENT(snd_strerror),
    ELEMENT(snd_pcm_start),
    ELEMENT(snd_pcm_drop),
    ELEMENT(snd_pcm_resume),
    ELEMENT(snd_pcm_hw_params_get_buffer_size),
    ELEMENT(snd_pcm_hw_params_set_rate_near),
    ELEMENT(snd_pcm_hw_params_set_access),
    ELEMENT(snd_pcm_hw_params_set_buffer_time_near),
    ELEMENT(snd_pcm_hw_params_set_buffer_size_near),
    ELEMENT(snd_pcm_hw_params_get_buffer_size_min),
    ELEMENT(snd_pcm_hw_params_set_format),
    ELEMENT(snd_pcm_sw_params_current),
    ELEMENT(snd_pcm_sw_params_set_start_threshold),
    ELEMENT(snd_pcm_sw_params_set_avail_min)
};
#undef ELEMENT

/**
 * Try to dynamically load the ALSA libraries.  This function is not
 * thread-safe, and should be called before attempting to use any of the
 * ALSA functions.
 *
 * @returns iprt status code
 */
int audioLoadAlsaLib(void)
{
    int rc = VINF_SUCCESS;
    unsigned i;
    static enum { NO = 0, YES, FAIL } isLibLoaded = NO;
    RTLDRMOD hLib;

    LogFlowFunc(("\n"));
    /* If this is not NO then the function has obviously been called twice,
       which is likely to be a bug. */
    if (NO != isLibLoaded)
    {
        AssertMsgFailed(("isLibLoaded == %s\n", YES == isLibLoaded ? "YES" : "NO"));
        return YES == isLibLoaded ? VINF_SUCCESS : VERR_NOT_SUPPORTED;
    }
    isLibLoaded = FAIL;
    rc = RTLdrLoad(VBOX_ALSA_LIB, &hLib);
    if (RT_FAILURE(rc))
    {
        LogRelFunc(("Failed to load library %s\n", VBOX_ALSA_LIB));
        return rc;
    }
    for (i=0; i<RT_ELEMENTS(SharedFuncs); i++)
    {
        rc = RTLdrGetSymbol(hLib, SharedFuncs[i].name, (void**)SharedFuncs[i].fn);
        if (RT_FAILURE(rc))
            return rc;
    }
    isLibLoaded = YES;
    return rc;
}

