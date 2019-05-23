/* $Id: pulse_stubs.c $ */
/** @file
 * Stubs for libpulse.
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

#include <pulse/pulseaudio.h>

#include "pulse_stubs.h"

#define VBOX_PULSE_LIB "libpulse.so.0"

#define PROXY_STUB(function, rettype, signature, shortsig) \
    static rettype (*g_pfn_ ## function) signature; \
    \
    rettype VBox_##function signature; \
    rettype VBox_##function signature \
    { \
        return g_pfn_ ## function shortsig; \
    }

#define PROXY_STUB_VOID(function, signature, shortsig) \
    static void (*g_pfn_ ## function) signature; \
    \
    void VBox_##function signature; \
    void VBox_##function signature \
    { \
        g_pfn_ ## function shortsig; \
    }

PROXY_STUB     (pa_bytes_per_second, size_t,
                (const pa_sample_spec *spec),
                (spec))
PROXY_STUB     (pa_bytes_to_usec, pa_usec_t,
                (uint64_t l, const pa_sample_spec *spec),
                (l, spec))
PROXY_STUB     (pa_channel_map_init_auto, pa_channel_map*,
                (pa_channel_map *m, unsigned channels, pa_channel_map_def_t def),
                (m, channels, def))

PROXY_STUB     (pa_context_connect, int,
                (pa_context *c, const char *server, pa_context_flags_t flags,
                 const pa_spawn_api *api),
                (c, server, flags, api))
PROXY_STUB_VOID(pa_context_disconnect,
                (pa_context *c),
                (c))
PROXY_STUB     (pa_context_get_server_info, pa_operation*,
                (pa_context *c, pa_server_info_cb_t cb, void *userdata),
                (c, cb, userdata))
PROXY_STUB     (pa_context_get_sink_info_by_name, pa_operation*,
                (pa_context *c, const char *name, pa_sink_info_cb_t cb, void *userdata),
                (c, name, cb, userdata))
PROXY_STUB     (pa_context_get_source_info_by_name, pa_operation*,
                (pa_context *c, const char *name, pa_source_info_cb_t cb, void *userdata),
                (c, name, cb, userdata))
PROXY_STUB     (pa_context_get_state, pa_context_state_t,
                (pa_context *c),
                (c))
PROXY_STUB_VOID(pa_context_unref,
                (pa_context *c),
                (c))
PROXY_STUB     (pa_context_errno, int,
                (pa_context *c),
                (c))
PROXY_STUB     (pa_context_new, pa_context*,
                (pa_mainloop_api *mainloop, const char *name),
                (mainloop, name))
PROXY_STUB_VOID(pa_context_set_state_callback,
                (pa_context *c, pa_context_notify_cb_t cb, void *userdata),
                (c, cb, userdata))


PROXY_STUB     (pa_frame_size, size_t,
                (const pa_sample_spec *spec),
                (spec))
PROXY_STUB_VOID(pa_operation_unref,
                (pa_operation *o),
                (o))
PROXY_STUB     (pa_operation_get_state, pa_operation_state_t,
                (pa_operation *o),
                (o))
PROXY_STUB_VOID(pa_operation_cancel,
                (pa_operation *o),
                (o))

PROXY_STUB     (pa_rtclock_now, pa_usec_t,
                (void),
                ())
PROXY_STUB     (pa_sample_format_to_string, const char*,
                (pa_sample_format_t f),
                (f))
PROXY_STUB     (pa_sample_spec_valid, int,
                (const pa_sample_spec *spec),
                (spec))
PROXY_STUB     (pa_strerror, const char*,
                (int error),
                (error))

#if PA_PROTOCOL_VERSION >= 16
PROXY_STUB     (pa_stream_connect_playback, int,
                (pa_stream *s, const char *dev, const pa_buffer_attr *attr,
                 pa_stream_flags_t flags, const pa_cvolume *volume, pa_stream *sync_stream),
                (s, dev, attr, flags, volume, sync_stream))
#else
PROXY_STUB     (pa_stream_connect_playback, int,
                (pa_stream *s, const char *dev, const pa_buffer_attr *attr,
                 pa_stream_flags_t flags, pa_cvolume *volume, pa_stream *sync_stream),
                (s, dev, attr, flags, volume, sync_stream))
#endif
PROXY_STUB     (pa_stream_connect_record, int,
                (pa_stream *s, const char *dev, const pa_buffer_attr *attr,
                pa_stream_flags_t flags),
                (s, dev, attr, flags))
PROXY_STUB     (pa_stream_disconnect, int,
                (pa_stream *s),
                (s))
PROXY_STUB     (pa_stream_get_sample_spec, const pa_sample_spec*,
                (pa_stream *s),
                (s))
PROXY_STUB_VOID(pa_stream_set_latency_update_callback,
                (pa_stream *p, pa_stream_notify_cb_t cb, void *userdata),
                (p, cb, userdata))
PROXY_STUB     (pa_stream_write, int,
                (pa_stream *p, const void *data, size_t bytes, pa_free_cb_t free_cb,
                 int64_t offset, pa_seek_mode_t seek),
                (p, data, bytes, free_cb, offset, seek))
PROXY_STUB_VOID(pa_stream_unref,
                (pa_stream *s),
                (s))
PROXY_STUB     (pa_stream_get_state, pa_stream_state_t,
                (pa_stream *p),
                (p))
PROXY_STUB     (pa_stream_get_latency, int,
                (pa_stream *s, pa_usec_t *r_usec, int *negative),
                (s, r_usec, negative))
PROXY_STUB     (pa_stream_get_timing_info, pa_timing_info*,
                (pa_stream *s),
                (s))
PROXY_STUB     (pa_stream_readable_size, size_t,
                (pa_stream *p),
                (p))
PROXY_STUB      (pa_stream_set_buffer_attr, pa_operation *,
                (pa_stream *s, const pa_buffer_attr *attr, pa_stream_success_cb_t cb, void *userdata),
                (s, attr, cb, userdata))
PROXY_STUB_VOID(pa_stream_set_state_callback,
                (pa_stream *s, pa_stream_notify_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB_VOID(pa_stream_set_underflow_callback,
                (pa_stream *s, pa_stream_notify_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB_VOID(pa_stream_set_overflow_callback,
                (pa_stream *s, pa_stream_notify_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB_VOID(pa_stream_set_write_callback,
                (pa_stream *s, pa_stream_request_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB     (pa_stream_flush, pa_operation*,
                (pa_stream *s, pa_stream_success_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB     (pa_stream_drain, pa_operation*,
                (pa_stream *s, pa_stream_success_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB     (pa_stream_trigger, pa_operation*,
                (pa_stream *s, pa_stream_success_cb_t cb, void *userdata),
                (s, cb, userdata))
PROXY_STUB     (pa_stream_new, pa_stream*,
                (pa_context *c, const char *name, const pa_sample_spec *ss,
                 const pa_channel_map *map),
                (c, name, ss, map))
PROXY_STUB     (pa_stream_get_buffer_attr, const pa_buffer_attr*,
                (pa_stream *s),
                (s))
PROXY_STUB     (pa_stream_peek, int,
                (pa_stream *p, const void **data, size_t *bytes),
                (p, data, bytes))
PROXY_STUB     (pa_stream_cork, pa_operation*,
                (pa_stream *s, int b, pa_stream_success_cb_t cb, void *userdata),
                (s, b, cb, userdata))
PROXY_STUB     (pa_stream_drop, int,
                (pa_stream *p),
                (p))
PROXY_STUB     (pa_stream_writable_size, size_t,
                (pa_stream *p),
                (p))

PROXY_STUB_VOID(pa_threaded_mainloop_stop,
                (pa_threaded_mainloop *m),
                (m))
PROXY_STUB     (pa_threaded_mainloop_get_api, pa_mainloop_api*,
                (pa_threaded_mainloop *m),
                (m))
PROXY_STUB_VOID(pa_threaded_mainloop_free,
                (pa_threaded_mainloop* m),
                (m))
PROXY_STUB_VOID(pa_threaded_mainloop_signal,
                (pa_threaded_mainloop *m, int wait_for_accept),
                (m, wait_for_accept))
PROXY_STUB_VOID(pa_threaded_mainloop_unlock,
                (pa_threaded_mainloop *m),
                (m))
PROXY_STUB     (pa_threaded_mainloop_new, pa_threaded_mainloop *,
                (void),
                ())
PROXY_STUB_VOID(pa_threaded_mainloop_wait,
                (pa_threaded_mainloop *m),
                (m))
PROXY_STUB     (pa_threaded_mainloop_start, int,
                (pa_threaded_mainloop *m),
                (m))
PROXY_STUB_VOID(pa_threaded_mainloop_lock,
                (pa_threaded_mainloop *m),
                (m))

PROXY_STUB     (pa_usec_to_bytes, size_t,
                (pa_usec_t t, const pa_sample_spec *spec),
                (t, spec))

typedef struct
{
    const char *name;
    void (**fn)(void);
} SHARED_FUNC;

#define ELEMENT(function) { #function , (void (**)(void)) & g_pfn_ ## function }
static SHARED_FUNC SharedFuncs[] =
{
    ELEMENT(pa_bytes_per_second),
    ELEMENT(pa_bytes_to_usec),
    ELEMENT(pa_channel_map_init_auto),

    ELEMENT(pa_context_connect),
    ELEMENT(pa_context_disconnect),
    ELEMENT(pa_context_get_server_info),
    ELEMENT(pa_context_get_sink_info_by_name),
    ELEMENT(pa_context_get_source_info_by_name),
    ELEMENT(pa_context_get_state),
    ELEMENT(pa_context_unref),
    ELEMENT(pa_context_errno),
    ELEMENT(pa_context_new),
    ELEMENT(pa_context_set_state_callback),

    ELEMENT(pa_frame_size),
    ELEMENT(pa_operation_unref),
    ELEMENT(pa_operation_get_state),
    ELEMENT(pa_operation_cancel),
    ELEMENT(pa_rtclock_now),
    ELEMENT(pa_sample_format_to_string),
    ELEMENT(pa_sample_spec_valid),
    ELEMENT(pa_strerror),

    ELEMENT(pa_stream_connect_playback),
    ELEMENT(pa_stream_connect_record),
    ELEMENT(pa_stream_disconnect),
    ELEMENT(pa_stream_get_sample_spec),
    ELEMENT(pa_stream_set_latency_update_callback),
    ELEMENT(pa_stream_write),
    ELEMENT(pa_stream_unref),
    ELEMENT(pa_stream_get_state),
    ELEMENT(pa_stream_get_latency),
    ELEMENT(pa_stream_get_timing_info),
    ELEMENT(pa_stream_readable_size),
    ELEMENT(pa_stream_set_buffer_attr),
    ELEMENT(pa_stream_set_state_callback),
    ELEMENT(pa_stream_set_underflow_callback),
    ELEMENT(pa_stream_set_overflow_callback),
    ELEMENT(pa_stream_set_write_callback),
    ELEMENT(pa_stream_flush),
    ELEMENT(pa_stream_drain),
    ELEMENT(pa_stream_trigger),
    ELEMENT(pa_stream_new),
    ELEMENT(pa_stream_get_buffer_attr),
    ELEMENT(pa_stream_peek),
    ELEMENT(pa_stream_cork),
    ELEMENT(pa_stream_drop),
    ELEMENT(pa_stream_writable_size),

    ELEMENT(pa_threaded_mainloop_stop),
    ELEMENT(pa_threaded_mainloop_get_api),
    ELEMENT(pa_threaded_mainloop_free),
    ELEMENT(pa_threaded_mainloop_signal),
    ELEMENT(pa_threaded_mainloop_unlock),
    ELEMENT(pa_threaded_mainloop_new),
    ELEMENT(pa_threaded_mainloop_wait),
    ELEMENT(pa_threaded_mainloop_start),
    ELEMENT(pa_threaded_mainloop_lock),

    ELEMENT(pa_usec_to_bytes)
};
#undef ELEMENT

/**
 * Try to dynamically load the PulseAudio libraries.  This function is not
 * thread-safe, and should be called before attempting to use any of the
 * PulseAudio functions.
 *
 * @returns iprt status code
 */
int audioLoadPulseLib(void)
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
    rc = RTLdrLoad(VBOX_PULSE_LIB, &hLib);
    if (RT_FAILURE(rc))
    {
        LogRelFunc(("Failed to load library %s\n", VBOX_PULSE_LIB));
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

