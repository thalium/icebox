/* $Id: alsa_mangling.h $ */
/** @file
 * Mangle libasound symbols.
 *
 * This is necessary on hosts which don't support the -fvisibility gcc switch.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef AUDIO_ALSA_MANGLING_H
#define AUDIO_ALSA_MANGLING_H

#define ALSA_MANGLER(symbol) VBox_##symbol

#define snd_device_name_hint                    ALSA_MANGLER(snd_device_name_hint)
#define snd_device_name_get_hint                ALSA_MANGLER(snd_device_name_get_hint)
#define snd_device_name_free_hint               ALSA_MANGLER(snd_device_name_free_hint)

#define snd_pcm_hw_params_any                   ALSA_MANGLER(snd_pcm_hw_params_any)
#define snd_pcm_close                           ALSA_MANGLER(snd_pcm_close)
#define snd_pcm_avail_update                    ALSA_MANGLER(snd_pcm_avail_update)
#define snd_pcm_hw_params_set_channels_near     ALSA_MANGLER(snd_pcm_hw_params_set_channels_near)
#define snd_pcm_hw_params_set_period_time_near  ALSA_MANGLER(snd_pcm_hw_params_set_period_time_near)
#define snd_pcm_prepare                         ALSA_MANGLER(snd_pcm_prepare)
#define snd_pcm_sw_params_sizeof                ALSA_MANGLER(snd_pcm_sw_params_sizeof)
#define snd_pcm_hw_params_set_period_size_near  ALSA_MANGLER(snd_pcm_hw_params_set_period_size_near)
#define snd_pcm_hw_params_get_period_size       ALSA_MANGLER(snd_pcm_hw_params_get_period_size)
#define snd_pcm_hw_params                       ALSA_MANGLER(snd_pcm_hw_params)
#define snd_pcm_hw_params_sizeof                ALSA_MANGLER(snd_pcm_hw_params_sizeof)
#define snd_pcm_state                           ALSA_MANGLER(snd_pcm_state)
#define snd_pcm_open                            ALSA_MANGLER(snd_pcm_open)
#define snd_lib_error_set_handler               ALSA_MANGLER(snd_lib_error_set_handler)
#define snd_pcm_sw_params                       ALSA_MANGLER(snd_pcm_sw_params)
#define snd_pcm_hw_params_get_period_size_min   ALSA_MANGLER(snd_pcm_hw_params_get_period_size_min)
#define snd_pcm_writei                          ALSA_MANGLER(snd_pcm_writei)
#define snd_pcm_readi                           ALSA_MANGLER(snd_pcm_readi)
#define snd_strerror                            ALSA_MANGLER(snd_strerror)
#define snd_pcm_start                           ALSA_MANGLER(snd_pcm_start)
#define snd_pcm_drop                            ALSA_MANGLER(snd_pcm_drop)
#define snd_pcm_resume                          ALSA_MANGLER(snd_pcm_resume)
#define snd_pcm_hw_params_get_buffer_size       ALSA_MANGLER(snd_pcm_hw_params_get_buffer_size)
#define snd_pcm_hw_params_set_rate_near         ALSA_MANGLER(snd_pcm_hw_params_set_rate_near)
#define snd_pcm_hw_params_set_access            ALSA_MANGLER(snd_pcm_hw_params_set_access)
#define snd_pcm_hw_params_set_buffer_time_near  ALSA_MANGLER(snd_pcm_hw_params_set_buffer_time_near)
#define snd_pcm_hw_params_set_buffer_size_near  ALSA_MANGLER(snd_pcm_hw_params_set_buffer_size_near)
#define snd_pcm_hw_params_get_buffer_size_min   ALSA_MANGLER(snd_pcm_hw_params_get_buffer_size_min)
#define snd_pcm_hw_params_set_format            ALSA_MANGLER(snd_pcm_hw_params_set_format)
#define snd_pcm_sw_params_current               ALSA_MANGLER(snd_pcm_sw_params_current)
#define snd_pcm_sw_params_set_start_threshold   ALSA_MANGLER(snd_pcm_sw_params_set_start_threshold)
#define snd_pcm_sw_params_set_avail_min         ALSA_MANGLER(snd_pcm_sw_params_set_avail_min)

#endif /* !AUDIO_ALSA_MANGLING_H */
