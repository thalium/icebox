/** @file
 * PDM - Pluggable Device Manager, audio interfaces.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

/**
 * == Audio architecture overview
 *
 * The audio architecture mainly consists of two PDM interfaces, PDMAUDIOCONNECTOR
 * and PDMIHOSTAUDIO.
 *
 * The PDMAUDIOCONNECTOR interface is responsible of connecting a device emulation, such
 * as SB16, AC'97 and HDA to one or multiple audio backend(s). Its API abstracts audio
 * stream handling and I/O functions, device enumeration and so on.
 *
 * The PDMIHOSTAUDIO interface must be implemented by all audio backends to provide an
 * abstract and common way of accessing needed functions, such as transferring output audio
 * data for playing audio or recording input from the host.
 *
 * A device emulation can have one or more LUNs attached to it, whereas these LUNs in turn
 * then all have their own PDMIAUDIOCONNECTOR, making it possible to connect multiple backends
 * to a certain device emulation stream (multiplexing).
 *
 * An audio backend's job is to record and/or play audio data (depending on its capabilities).
 * It highly depends on the host it's running on and needs very specific (host-OS-dependent) code.
 * The backend itself only has very limited ways of accessing and/or communicating with the
 * PDMIAUDIOCONNECTOR interface via callbacks, but never directly with the device emulation or
 * other parts of the audio sub system.
 *
 *
 * == Mixing
 *
 * The AUDIOMIXER API is optionally available to create and manage virtual audio mixers.
 * Such an audio mixer in turn then can be used by the device emulation code to manage all
 * the multiplexing to/from the connected LUN audio streams.
 *
 * Currently only input and output stream are supported. Duplex stream are not supported yet.
 *
 * This also is handy if certain LUN audio streams should be added or removed during runtime.
 *
 * To create a group of either input or output streams the AUDMIXSINK API can be used.
 *
 * For example: The device emulation has one hardware output stream (HW0), and that output
 *              stream shall be available to all connected LUN backends. For that to happen,
 *              an AUDMIXSINK sink has to be created and attached to the device's AUDIOMIXER object.
 *
 *              As every LUN has its own AUDMIXSTREAM object, adding all those objects to the
 *              just created audio mixer sink will do the job.
 *
 * Note: The AUDIOMIXER API is purely optional and is not used by all currently implemented
 *       device emulations (e.g. SB16).
 *
 *
 * == Data processing
 *
 * Audio input / output data gets handed-off to/from the device emulation in an unmodified
 * - that is, raw - way. The actual audio frame / sample conversion is done via the PDMAUDIOMIXBUF API.
 *
 * This concentrates the audio data processing in one place and makes it easier to test / benchmark
 * such code.
 *
 * A PDMAUDIOFRAME is the internal representation of a single audio frame, which consists of a single left
 * and right audio sample in time. Only mono (1) and stereo (2) channel(s) currently are supported.
 *
 *
 * == Timing
 *
 * Handling audio data in a virtual environment is hard, as the human perception is very sensitive
 * to the slightest cracks and stutters in the audible data. This can happen if the VM's timing is
 * lagging behind or not within the expected time frame.
 *
 * The two main components which unfortunately contradict each other is a) the audio device emulation
 * and b) the audio backend(s) on the host. Those need to be served in a timely manner to function correctly.
 * To make e.g. the device emulation rely on the pace the host backend(s) set - or vice versa - will not work,
 * as the guest's audio system / drivers then will not be able to compensate this accordingly.
 *
 * So each component, the device emulation, the audio connector(s) and the backend(s) must do its thing
 * *when* it needs to do it, independently of the others. For that we use various (small) ring buffers to
 * (hopefully) serve all components with the amount of data *when* they need it.
 *
 * Additionally, the device emulation can run with a different audio frame size, while the backends(s) may
 * require a different frame size (16 bit stereo -> 8 bit mono, for example).
 *
 * The device emulation can give the audio connector(s) a scheduling hint (optional), e.g. in which interval
 * it expects any data processing.
 *
 * A data transfer for playing audio data from the guest on the host looks like this:
 * (RB = Ring Buffer, MB = Mixing Buffer)
 *
 * (A) Device DMA -> (B) Device RB -> (C) Audio Connector Guest MB -> (D) Audio Connector Host MB -> \
 * (E) Backend RB (optional, up to the backend) > (F) Backend audio framework
 *
 * For capturing audio data the above chain is similar, just in a different direction, of course.
 *
 * The audio connector hereby plays a key role when it comes to (pre-) buffering data to minimize any audio stutters
 * and/or cracks. The following values, which also can be tweaked via CFGM / extra-data are available:
 *
 * - The pre-buffering time (in ms): Audio data which needs to be buffered before any playback (or capturing) can happen.
 * - The actual buffer size (in ms): How big the mixing buffer (for C and D) will be.
 * - The period size (in ms): How big a chunk of audio (often called period or fragment) for F must be to get handled correctly.
 *
 * The above values can be set on a per-driver level, whereas input and output streams for a driver also can be handled
 * set independently. The verbose audio (release) log will tell about the (final) state of each audio stream.
 *
 *
 * == Diagram
 *
 * +-------------------------+
 * +-------------------------+        +-------------------------+        +-------------------+
 * |PDMAUDIOSTREAM           |        |PDMAUDIOCONNECTOR        |    + ++|LUN                |
 * |-------------------------|        |-------------------------|    | |||-------------------|
 * |PDMAUDIOMIXBUF           |+------>|PDMAUDIOSTREAM Host      |+---|-|||PDMIAUDIOCONNECTOR |
 * |PDMAUDIOSTREAMCFG        |+------>|PDMAUDIOSTREAM Guest     |    | |||AUDMIXSTREAM       |
 * |                         |        |Device capabilities      |    | |||                   |
 * |                         |        |Device configuration     |    | |||                   |
 * |                         |        |                         |    | |||                   |
 * |                         |       +|PDMIHOSTAUDIO            |    | |||                   |
 * |                         |       ||+-----------------------+|    | ||+-------------------+
 * +-------------------------+       |||Backend storage space  ||    | ||
 *                                   ||+-----------------------+|    | ||
 *                                   |+-------------------------+    | ||
 *                                   |                               | ||
 * +---------------------+           |                               | ||
 * |PDMIHOSTAUDIO        |           |                               | ||
 * |+--------------+     |           |      +-------------------+    | ||      +-------------+
 * ||DirectSound   |     |           |      |AUDMIXSINK         |    | ||      |AUDIOMIXER   |
 * |+--------------+     |           |      |-------------------|    | ||      |-------------|
 * |                     |           |      |AUDMIXSTREAM0      |+---|-||----->|AUDMIXSINK0  |
 * |+--------------+     |           |      |AUDMIXSTREAM1      |+---|-||----->|AUDMIXSINK1  |
 * ||PulseAudio    |     |           |      |AUDMIXSTREAMn      |+---|-||----->|AUDMIXSINKn  |
 * |+--------------+     |+----------+      +-------------------+    | ||      +-------------+
 * |                     |                                           | ||
 * |+--------------+     |                                           | ||
 * ||Core Audio    |     |                                           | ||
 * |+--------------+     |                                           | ||
 * |                     |                                           | ||
 * |                     |                                           | ||+----------------------------------+
 * |                     |                                           | |||Device (SB16 / AC'97 / HDA)       |
 * |                     |                                           | |||----------------------------------|
 * +---------------------+                                           | |||AUDIOMIXER (Optional)             |
 *                                                                   | |||AUDMIXSINK0 (Optional)            |
 *                                                                   | |||AUDMIXSINK1 (Optional)            |
 *                                                                   | |||AUDMIXSINKn (Optional)            |
 *                                                                   | |||                                  |
 *                                                                   | |+|LUN0                              |
 *                                                                   | ++|LUN1                              |
 *                                                                   +--+|LUNn                              |
 *                                                                       |                                  |
 *                                                                       |                                  |
 *                                                                       |                                  |
 *                                                                       +----------------------------------+
 */

#ifndef ___VBox_vmm_pdmaudioifs_h
#define ___VBox_vmm_pdmaudioifs_h

#include <iprt/assertcompile.h>
#include <iprt/circbuf.h>
#include <iprt/list.h>
#include <iprt/path.h>

#include <VBox/types.h>
#ifdef VBOX_WITH_STATISTICS
# include <VBox/vmm/stam.h>
#endif

/** @defgroup grp_pdm_ifs_audio     PDM Audio Interfaces
 * @ingroup grp_pdm_interfaces
 * @{
 */

#ifndef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH
# ifdef RT_OS_WINDOWS
#  define VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "c:\\temp\\"
# else
#  define VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "/tmp/"
# endif
#endif

/** PDM audio driver instance flags. */
typedef uint32_t PDMAUDIODRVFLAGS;

/** No flags set. */
#define PDMAUDIODRVFLAGS_NONE       0
/** Marks a primary audio driver which is critical
 *  when running the VM. */
#define PDMAUDIODRVFLAGS_PRIMARY    RT_BIT(0)

/**
 * Audio format in signed or unsigned variants.
 */
typedef enum PDMAUDIOFMT
{
    /** Invalid format, do not use. */
    PDMAUDIOFMT_INVALID,
    /** 8-bit, unsigned. */
    PDMAUDIOFMT_U8,
    /** 8-bit, signed. */
    PDMAUDIOFMT_S8,
    /** 16-bit, unsigned. */
    PDMAUDIOFMT_U16,
    /** 16-bit, signed. */
    PDMAUDIOFMT_S16,
    /** 32-bit, unsigned. */
    PDMAUDIOFMT_U32,
    /** 32-bit, signed. */
    PDMAUDIOFMT_S32,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOFMT_32BIT_HACK = 0x7fffffff
} PDMAUDIOFMT;

/**
 * Audio direction.
 */
typedef enum PDMAUDIODIR
{
    /** Unknown direction. */
    PDMAUDIODIR_UNKNOWN = 0,
    /** Input. */
    PDMAUDIODIR_IN      = 1,
    /** Output. */
    PDMAUDIODIR_OUT     = 2,
    /** Duplex handling. */
    PDMAUDIODIR_ANY     = 3,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIODIR_32BIT_HACK = 0x7fffffff
} PDMAUDIODIR;

/** Device latency spec in milliseconds (ms). */
typedef uint32_t PDMAUDIODEVLATSPECMS;

/** Device latency spec in seconds (s). */
typedef uint32_t PDMAUDIODEVLATSPECSEC;

/** Audio device flags. Use with PDMAUDIODEV_FLAG_ flags. */
typedef uint32_t PDMAUDIODEVFLAG;

/** No flags set. */
#define PDMAUDIODEV_FLAGS_NONE            0
/** The device marks the default device within the host OS. */
#define PDMAUDIODEV_FLAGS_DEFAULT         RT_BIT(0)
/** The device can be removed at any time and we have to deal with it. */
#define PDMAUDIODEV_FLAGS_HOTPLUG         RT_BIT(1)
/** The device is known to be buggy and needs special treatment. */
#define PDMAUDIODEV_FLAGS_BUGGY           RT_BIT(2)
/** Ignore the device, no matter what. */
#define PDMAUDIODEV_FLAGS_IGNORE          RT_BIT(3)
/** The device is present but marked as locked by some other application. */
#define PDMAUDIODEV_FLAGS_LOCKED          RT_BIT(4)
/** The device is present but not in an alive state (dead). */
#define PDMAUDIODEV_FLAGS_DEAD            RT_BIT(5)

/**
 * Audio device type.
 */
typedef enum PDMAUDIODEVICETYPE
{
    /** Unknown device type. This is the default. */
    PDMAUDIODEVICETYPE_UNKNOWN    = 0,
    /** Dummy device; for backends which are not able to report
     *  actual device information (yet). */
    PDMAUDIODEVICETYPE_DUMMY,
    /** The device is built into the host (non-removable). */
    PDMAUDIODEVICETYPE_BUILTIN,
    /** The device is an (external) USB device. */
    PDMAUDIODEVICETYPE_USB,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIODEVICETYPE_32BIT_HACK = 0x7fffffff
} PDMAUDIODEVICETYPE;

/**
 * Audio device instance data.
 */
typedef struct PDMAUDIODEVICE
{
    /** List node. */
    RTLISTNODE         Node;
    /** Friendly name of the device, if any. */
    char               szName[64];
    /** The device type. */
    PDMAUDIODEVICETYPE enmType;
    /** Reference count indicating how many audio streams currently are relying on this device. */
    uint8_t            cRefCount;
    /** Usage of the device. */
    PDMAUDIODIR        enmUsage;
    /** Device flags. */
    PDMAUDIODEVFLAG    fFlags;
    /** Maximum number of input audio channels the device supports. */
    uint8_t            cMaxInputChannels;
    /** Maximum number of output audio channels the device supports. */
    uint8_t            cMaxOutputChannels;
    /** Additional data which might be relevant for the current context. */
    void              *pvData;
    /** Size of the additional data. */
    size_t             cbData;
    /** Device type union, based on enmType. */
    union
    {
        /** USB type specifics. */
        struct
        {
            /** Vendor ID. */
            int16_t    VID;
            /** Product ID. */
            int16_t    PID;
        } USB;
    } Type;
} PDMAUDIODEVICE, *PPDMAUDIODEVICE;

/**
 * Structure for keeping an audio device enumeration.
 */
typedef struct PDMAUDIODEVICEENUM
{
    /** Number of audio devices in the list. */
    uint16_t        cDevices;
    /** List of audio devices. */
    RTLISTANCHOR    lstDevices;
} PDMAUDIODEVICEENUM, *PPDMAUDIODEVICEENUM;

/**
 * Audio (static) configuration of an audio host backend.
 */
typedef struct PDMAUDIOBACKENDCFG
{
    /** The backend's friendly name. */
    char     szName[32];
    /** Size (in bytes) of the host backend's audio output stream structure. */
    size_t   cbStreamOut;
    /** Size (in bytes) of the host backend's audio input stream structure. */
    size_t   cbStreamIn;
    /** Number of concurrent output (playback) streams supported on the host.
     *  UINT32_MAX for unlimited concurrent streams, 0 if no concurrent input streams are supported. */
    uint32_t cMaxStreamsOut;
    /** Number of concurrent input (recording) streams supported on the host.
     *  UINT32_MAX for unlimited concurrent streams, 0 if no concurrent input streams are supported. */
    uint32_t cMaxStreamsIn;
} PDMAUDIOBACKENDCFG, *PPDMAUDIOBACKENDCFG;

/**
 * A single audio frame.
 *
 * Currently only two (2) channels, left and right, are supported.
 *
 * Note: When changing this structure, make sure to also handle
 *       VRDP's input / output processing in DrvAudioVRDE, as VRDP
 *       expects audio data in st_sample_t format (historical reasons)
 *       which happens to be the same as PDMAUDIOFRAME for now.
 */
typedef struct PDMAUDIOFRAME
{
    /** Left channel. */
    int64_t i64LSample;
    /** Right channel. */
    int64_t i64RSample;
} PDMAUDIOFRAME;
/** Pointer to a single (stereo) audio frame. */
typedef PDMAUDIOFRAME *PPDMAUDIOFRAME;
/** Pointer to a const single (stereo) audio frame. */
typedef PDMAUDIOFRAME const *PCPDMAUDIOFRAME;

typedef enum PDMAUDIOENDIANNESS
{
    /** The usual invalid endian. */
    PDMAUDIOENDIANNESS_INVALID,
    /** Little endian. */
    PDMAUDIOENDIANNESS_LITTLE,
    /** Bit endian. */
    PDMAUDIOENDIANNESS_BIG,
    /** Endianness doesn't have a meaning in the context. */
    PDMAUDIOENDIANNESS_NA,
    /** The end of the valid endian values (exclusive). */
    PDMAUDIOENDIANNESS_END,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOENDIANNESS_32BIT_HACK = 0x7fffffff
} PDMAUDIOENDIANNESS;

/**
 * Audio playback destinations.
 */
typedef enum PDMAUDIOPLAYBACKDEST
{
    /** Unknown destination. */
    PDMAUDIOPLAYBACKDEST_UNKNOWN = 0,
    /** Front channel. */
    PDMAUDIOPLAYBACKDEST_FRONT,
    /** Center / LFE (Subwoofer) channel. */
    PDMAUDIOPLAYBACKDEST_CENTER_LFE,
    /** Rear channel. */
    PDMAUDIOPLAYBACKDEST_REAR,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOPLAYBACKDEST_32BIT_HACK = 0x7fffffff
} PDMAUDIOPLAYBACKDEST;

/**
 * Audio recording sources.
 */
typedef enum PDMAUDIORECSOURCE
{
    /** Unknown recording source. */
    PDMAUDIORECSOURCE_UNKNOWN = 0,
    /** Microphone-In. */
    PDMAUDIORECSOURCE_MIC,
    /** CD. */
    PDMAUDIORECSOURCE_CD,
    /** Video-In. */
    PDMAUDIORECSOURCE_VIDEO,
    /** AUX. */
    PDMAUDIORECSOURCE_AUX,
    /** Line-In. */
    PDMAUDIORECSOURCE_LINE,
    /** Phone-In. */
    PDMAUDIORECSOURCE_PHONE,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIORECSOURCE_32BIT_HACK = 0x7fffffff
} PDMAUDIORECSOURCE;

/**
 * Audio stream (data) layout.
 */
typedef enum PDMAUDIOSTREAMLAYOUT
{
    /** Unknown access type; do not use. */
    PDMAUDIOSTREAMLAYOUT_UNKNOWN = 0,
    /** Non-interleaved access, that is, consecutive
     *  access to the data. */
    PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED,
    /** Interleaved access, where the data can be
     *  mixed together with data of other audio streams. */
    PDMAUDIOSTREAMLAYOUT_INTERLEAVED,
    /** Complex layout, which does not fit into the
     *  interleaved / non-interleaved layouts. */
    PDMAUDIOSTREAMLAYOUT_COMPLEX,
    /** Raw (pass through) data, with no data layout processing done.
     *
     *  This means that this stream will operate on PDMAUDIOFRAME data
     *  directly. Don't use this if you don't have to. */
    PDMAUDIOSTREAMLAYOUT_RAW,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOSTREAMLAYOUT_32BIT_HACK = 0x7fffffff
} PDMAUDIOSTREAMLAYOUT, *PPDMAUDIOSTREAMLAYOUT;

/** No stream channel data flags defined. */
#define PDMAUDIOSTREAMCHANNELDATA_FLAG_NONE      0

/**
 * Structure for keeping a stream channel data block around.
 */
typedef struct PDMAUDIOSTREAMCHANNELDATA
{
    /** Circular buffer for the channel data. */
    PRTCIRCBUF pCircBuf;
    size_t     cbAcq;
    /** Channel data flags. */
    uint32_t   fFlags;
} PDMAUDIOSTREAMCHANNELDATA, *PPDMAUDIOSTREAMCHANNELDATA;

/**
 * Structure for a single channel of an audio stream.
 * An audio stream consists of one or multiple channels,
 * depending on the configuration.
 */
typedef struct PDMAUDIOSTREAMCHANNEL
{
    /** Channel ID. */
    uint8_t                   uChannel;
    /** Step size (in bytes) to the channel's next frame. */
    size_t                    cbStep;
    /** Frame size (in bytes) of this channel. */
    size_t                    cbFrame;
    /** Offset (in bytes) to first frame in the data block. */
    size_t                    cbFirst;
    /** Currente offset (in bytes) in the data stream. */
    size_t                    cbOff;
    /** Associated data buffer. */
    PDMAUDIOSTREAMCHANNELDATA Data;
} PDMAUDIOSTREAMCHANNEL, *PPDMAUDIOSTREAMCHANNEL;

/**
 * Union for keeping an audio stream destination or source.
 */
typedef union PDMAUDIODESTSOURCE
{
    /** Desired playback destination (for an output stream). */
    PDMAUDIOPLAYBACKDEST Dest;
    /** Desired recording source (for an input stream). */
    PDMAUDIORECSOURCE    Source;
} PDMAUDIODESTSOURCE, *PPDMAUDIODESTSOURCE;

/**
 * Properties of audio streams for host/guest for in or out directions.
 */
typedef struct PDMAUDIOPCMPROPS
{
    /** Sample width (in bytes). */
    uint8_t     cBytes;
    /** Number of audio channels. */
    uint8_t     cChannels;
    /** Shift count used for faster calculation of various
     *  values, such as the alignment, bytes to frames and so on.
     *  Depends on number of stream channels and the stream format
     *  being used.
     *
     ** @todo Use some RTAsmXXX functions instead?
     */
    uint8_t     cShift;
    /** Signed or unsigned sample. */
    bool        fSigned : 1;
    /** Whether the endianness is swapped or not. */
    bool        fSwapEndian : 1;
    /** Sample frequency in Hertz (Hz). */
    uint32_t    uHz;
} PDMAUDIOPCMPROPS;
AssertCompileSizeAlignment(PDMAUDIOPCMPROPS, 8);
/** Pointer to audio stream properties. */
typedef PDMAUDIOPCMPROPS *PPDMAUDIOPCMPROPS;

/** Initializor for PDMAUDIOPCMPROPS. */
#define PDMAUDIOPCMPROPS_INITIALIZOR(a_cBytes, a_fSigned, a_cCannels, a_uHz, a_cShift, a_fSwapEndian) \
    { a_cBytes, a_cCannels, a_cShift, a_fSigned, a_fSwapEndian, a_uHz }
/** Calculates the cShift value of given sample bits and audio channels.
 *  Note: Does only support mono/stereo channels for now. */
#define PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(cBytes, cChannels)    ((cChannels == 2) + (cBytes / 2))
/** Calculates the cShift value of a PDMAUDIOPCMPROPS structure. */
#define PDMAUDIOPCMPROPS_MAKE_SHIFT(pProps)                     PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS((pProps)->cBytes, (pProps)->cChannels)
/** Converts (audio) frames to bytes.
 *  Needs the cShift value set correctly, using PDMAUDIOPCMPROPS_MAKE_SHIFT. */
#define PDMAUDIOPCMPROPS_F2B(pProps, frames)                    ((frames) << (pProps)->cShift)
/** Converts bytes to (audio) frames.
 *  Needs the cShift value set correctly, using PDMAUDIOPCMPROPS_MAKE_SHIFT. */
#define PDMAUDIOPCMPROPS_B2F(pProps, cb)                        (cb >> (pProps)->cShift)

/**
 * Structure for keeping an audio stream configuration.
 */
typedef struct PDMAUDIOSTREAMCFG
{
    /** Friendly name of the stream. */
    char                     szName[64];
    /** Direction of the stream. */
    PDMAUDIODIR              enmDir;
    /** Destination / source indicator, depending on enmDir. */
    PDMAUDIODESTSOURCE       DestSource;
    /** The stream's PCM properties. */
    PDMAUDIOPCMPROPS         Props;
    /** The stream's audio data layout.
     *  This indicates how the audio data buffers to/from the backend is being layouted.
     *
     *  Currently, the following layouts are supported by the audio connector:
     *
     *  PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED:
     *      One stream at once. The consecutive audio data is exactly in the format and frame width
     *      like defined in the PCM properties. This is the default.
     *
     *  PDMAUDIOSTREAMLAYOUT_RAW:
     *      Can be one or many streams at once, depending on the stream's mixing buffer setup.
     *      The audio data will get handled as PDMAUDIOFRAME frames without any modification done. */
    PDMAUDIOSTREAMLAYOUT     enmLayout;
    /** Device emulation-specific data needed for the audio connector. */
    struct
    {
        /** Scheduling hint set by the device emulation about when this stream is being served on average (in ms).
         *  Can be 0 if not hint given or some other mechanism (e.g. callbacks) is being used. */
        uint32_t             uSchedulingHintMs;
    } Device;
    /**
     * Backend-specific data for the stream.
     * On input (requested configuration) those values are set by the audio connector to let the backend know what we expect.
     * On output (acquired configuration) those values reflect the values set and used by the backend.
     * Set by the backend on return. Not all backends support all values / features.
     */
    struct
    {
        /** Period size of the stream (in audio frames).
         *  This value reflects the number of audio frames in between each hardware interrupt on the
         *  backend (host) side. 0 if not set / available by the backend. */
        uint32_t             cfPeriod;
        /** (Ring) buffer size (in audio frames). Often is a multiple of cfPeriod.
         *  0 if not set / available by the backend. */
        uint32_t             cfBufferSize;
        /** Pre-buffering size (in audio frames). Frames needed in buffer before the stream becomes active (pre buffering).
         *  The bigger this value is, the more latency for the stream will occur.
         *  0 if not set / available by the backend. */
        uint32_t             cfPreBuf;
    } Backend;
} PDMAUDIOSTREAMCFG;
AssertCompileSizeAlignment(PDMAUDIOPCMPROPS, 8);
/** Pointer to audio stream configuration keeper. */
typedef PDMAUDIOSTREAMCFG *PPDMAUDIOSTREAMCFG;


/** Converts (audio) frames to bytes. */
#define PDMAUDIOSTREAMCFG_F2B(pCfg, frames) ((frames) << (pCfg->Props).cShift)
/** Converts bytes to (audio) frames. */
#define PDMAUDIOSTREAMCFG_B2F(pCfg, cb)  (cb >> (pCfg->Props).cShift)

#if defined(RT_LITTLE_ENDIAN)
# define PDMAUDIOHOSTENDIANNESS PDMAUDIOENDIANNESS_LITTLE
#elif defined(RT_BIG_ENDIAN)
# define PDMAUDIOHOSTENDIANNESS PDMAUDIOENDIANNESS_BIG
#else
# error "Port me!"
#endif

/**
 * Audio mixer controls.
 */
typedef enum PDMAUDIOMIXERCTL
{
    /** Unknown mixer control. */
    PDMAUDIOMIXERCTL_UNKNOWN = 0,
    /** Master volume. */
    PDMAUDIOMIXERCTL_VOLUME_MASTER,
    /** Front. */
    PDMAUDIOMIXERCTL_FRONT,
    /** Center / LFE (Subwoofer). */
    PDMAUDIOMIXERCTL_CENTER_LFE,
    /** Rear. */
    PDMAUDIOMIXERCTL_REAR,
    /** Line-In. */
    PDMAUDIOMIXERCTL_LINE_IN,
    /** Microphone-In. */
    PDMAUDIOMIXERCTL_MIC_IN,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOMIXERCTL_32BIT_HACK = 0x7fffffff
} PDMAUDIOMIXERCTL;

/**
 * Audio stream commands. Used in the audio connector
 * as well as in the actual host backends.
 */
typedef enum PDMAUDIOSTREAMCMD
{
    /** Unknown command, do not use. */
    PDMAUDIOSTREAMCMD_UNKNOWN = 0,
    /** Enables the stream. */
    PDMAUDIOSTREAMCMD_ENABLE,
    /** Disables the stream.
     *  For output streams this stops the stream after playing the remaining (buffered) audio data.
     *  For input streams this will deliver the remaining (captured) audio data and not accepting
     *  any new audio input data afterwards. */
    PDMAUDIOSTREAMCMD_DISABLE,
    /** Pauses the stream. */
    PDMAUDIOSTREAMCMD_PAUSE,
    /** Resumes the stream. */
    PDMAUDIOSTREAMCMD_RESUME,
    /** Tells the stream to drain itself.
     *  For output streams this plays all remaining (buffered) audio frames,
     *  for input streams this permits receiving any new audio frames.
     *  No supported by all backends. */
    PDMAUDIOSTREAMCMD_DRAIN,
    /** Tells the stream to drop all (buffered) audio data immediately.
     *  No supported by all backends. */
    PDMAUDIOSTREAMCMD_DROP,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOSTREAMCMD_32BIT_HACK = 0x7fffffff
} PDMAUDIOSTREAMCMD;

/**
 * Audio volume parameters.
 */
typedef struct PDMAUDIOVOLUME
{
    /** Set to @c true if this stream is muted, @c false if not. */
    bool    fMuted;
    /** Left channel volume.
     *  Range is from [0 ... 255], whereas 0 specifies
     *  the most silent and 255 the loudest value. */
    uint8_t uLeft;
    /** Right channel volume.
     *  Range is from [0 ... 255], whereas 0 specifies
     *  the most silent and 255 the loudest value. */
    uint8_t uRight;
} PDMAUDIOVOLUME, *PPDMAUDIOVOLUME;

/** Defines the minimum volume allowed. */
#define PDMAUDIO_VOLUME_MIN     (0)
/** Defines the maximum volume allowed. */
#define PDMAUDIO_VOLUME_MAX     (255)

/**
 * Structure for holding rate processing information
 * of a source + destination audio stream. This is needed
 * because both streams can differ regarding their rates
 * and therefore need to be treated accordingly.
 */
typedef struct PDMAUDIOSTREAMRATE
{
    /** Current (absolute) offset in the output
     *  (destination) stream. */
    uint64_t       dstOffset;
    /** Increment for moving dstOffset for the
     *  destination stream. This is needed because the
     *  source <-> destination rate might be different. */
    uint64_t       dstInc;
    /** Current (absolute) offset in the input
     *  stream. */
    uint32_t       srcOffset;
    /** Last processed frame of the input stream.
     *  Needed for interpolation. */
    PDMAUDIOFRAME  srcFrameLast;
} PDMAUDIOSTREAMRATE, *PPDMAUDIOSTREAMRATE;

/**
 * Structure for holding mixing buffer volume parameters.
 * The volume values are in fixed point style and must
 * be converted to/from before using with e.g. PDMAUDIOVOLUME.
 */
typedef struct PDMAUDMIXBUFVOL
{
    /** Set to @c true if this stream is muted, @c false if not. */
    bool    fMuted;
    /** Left volume to apply during conversion. Pass 0
     *  to convert the original values. May not apply to
     *  all conversion functions. */
    uint32_t uLeft;
    /** Right volume to apply during conversion. Pass 0
     *  to convert the original values. May not apply to
     *  all conversion functions. */
    uint32_t uRight;
} PDMAUDMIXBUFVOL, *PPDMAUDMIXBUFVOL;

/**
 * Structure for holding frame conversion parameters for
 * the audioMixBufConvFromXXX / audioMixBufConvToXXX macros.
 */
typedef struct PDMAUDMIXBUFCONVOPTS
{
    /** Number of audio frames to convert. */
    uint32_t                cFrames;
    union
    {
        struct
        {
            /** Volume to use for conversion. */
            PDMAUDMIXBUFVOL Volume;
        } From;
    } RT_UNION_NM(u);
} PDMAUDMIXBUFCONVOPTS;
/** Pointer to conversion parameters for the audio mixer.   */
typedef PDMAUDMIXBUFCONVOPTS *PPDMAUDMIXBUFCONVOPTS;
/** Pointer to const conversion parameters for the audio mixer.   */
typedef PDMAUDMIXBUFCONVOPTS const *PCPDMAUDMIXBUFCONVOPTS;

/**
 * Note: All internal handling is done in audio frames,
 *       not in bytes!
 */
typedef uint32_t PDMAUDIOMIXBUFFMT;
typedef PDMAUDIOMIXBUFFMT *PPDMAUDIOMIXBUFFMT;

/**
 * Convertion-from function used by the PDM audio buffer mixer.
 *
 * @returns Number of audio frames returned.
 * @param   paDst           Where to return the converted frames.
 * @param   pvSrc           The source frame bytes.
 * @param   cbSrc           Number of bytes to convert.
 * @param   pOpts           Conversion options.
 */
typedef DECLCALLBACK(uint32_t) FNPDMAUDIOMIXBUFCONVFROM(PPDMAUDIOFRAME paDst, const void *pvSrc, uint32_t cbSrc,
                                                        PCPDMAUDMIXBUFCONVOPTS pOpts);
/** Pointer to a convertion-from function used by the PDM audio buffer mixer. */
typedef FNPDMAUDIOMIXBUFCONVFROM *PFNPDMAUDIOMIXBUFCONVFROM;

/**
 * Convertion-to function used by the PDM audio buffer mixer.
 *
 * @param   pvDst           Output buffer.
 * @param   paSrc           The input frames.
 * @param   pOpts           Conversion options.
 */
typedef DECLCALLBACK(void) FNPDMAUDIOMIXBUFCONVTO(void *pvDst, PCPDMAUDIOFRAME paSrc, PCPDMAUDMIXBUFCONVOPTS pOpts);
/** Pointer to a convertion-to function used by the PDM audio buffer mixer. */
typedef FNPDMAUDIOMIXBUFCONVTO *PFNPDMAUDIOMIXBUFCONVTO;

typedef struct PDMAUDIOMIXBUF *PPDMAUDIOMIXBUF;
typedef struct PDMAUDIOMIXBUF
{
    RTLISTNODE                Node;
    /** Name of the buffer. */
    char                     *pszName;
    /** Frame buffer. */
    PPDMAUDIOFRAME            pFrames;
    /** Size of the frame buffer (in audio frames). */
    uint32_t                  cFrames;
    /** The current read position (in frames). */
    uint32_t                  offRead;
    /** The current write position (in frames). */
    uint32_t                  offWrite;
    /**
     * Total frames already mixed down to the parent buffer (if any). Always starting at
     * the parent's offRead position.
     *
     * Note: Count always is specified in parent frames, as the sample count can differ between parent
     *       and child.
     */
    uint32_t                  cMixed;
    /** How much audio frames are currently being used
     *  in this buffer.
     *  Note: This also is known as the distance in ring buffer terms. */
    uint32_t                  cUsed;
    /** Pointer to parent buffer (if any). */
    PPDMAUDIOMIXBUF           pParent;
    /** List of children mix buffers to keep in sync with (if being a parent buffer). */
    RTLISTANCHOR              lstChildren;
    /** Number of children mix buffers kept in lstChildren. */
    uint32_t                  cChildren;
    /** Intermediate structure for buffer conversion tasks. */
    PPDMAUDIOSTREAMRATE       pRate;
    /** Internal representation of current volume used for mixing. */
    PDMAUDMIXBUFVOL           Volume;
    /** This buffer's audio format. */
    PDMAUDIOMIXBUFFMT         AudioFmt;
    /** Standard conversion-to function for set AudioFmt. */
    PFNPDMAUDIOMIXBUFCONVTO   pfnConvTo;
    /** Standard conversion-from function for set AudioFmt. */
    PFNPDMAUDIOMIXBUFCONVFROM pfnConvFrom;
    /**
     * Ratio of the associated parent stream's frequency by this stream's
     * frequency (1<<32), represented as a signed 64 bit integer.
     *
     * For example, if the parent stream has a frequency of 44 khZ, and this
     * stream has a frequency of 11 kHz, the ration then would be
     * (44/11 * (1 << 32)).
     *
     * Currently this does not get changed once assigned.
     */
    int64_t                   iFreqRatio;
    /** For quickly converting frames <-> bytes and vice versa. */
    uint8_t                   cShift;
} PDMAUDIOMIXBUF;

typedef uint32_t PDMAUDIOFILEFLAGS;

/** No flags defined. */
#define PDMAUDIOFILE_FLAG_NONE          0
/** Keep the audio file even if it contains no audio data. */
#define PDMAUDIOFILE_FLAG_KEEP_IF_EMPTY RT_BIT(0)
/** Audio file flag validation mask. */
#define PDMAUDIOFILE_FLAG_VALID_MASK    0x1

/** Audio file default open flags. */
#define PDMAUDIOFILE_DEFAULT_OPEN_FLAGS (RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_WRITE)

/**
 * Audio file types.
 */
typedef enum PDMAUDIOFILETYPE
{
    /** Unknown type, do not use. */
    PDMAUDIOFILETYPE_UNKNOWN = 0,
    /** Raw (PCM) file. */
    PDMAUDIOFILETYPE_RAW,
    /** Wave (.WAV) file. */
    PDMAUDIOFILETYPE_WAV,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOFILETYPE_32BIT_HACK = 0x7fffffff
} PDMAUDIOFILETYPE;

typedef uint32_t PDMAUDIOFILENAMEFLAGS;

/** No flags defined. */
#define PDMAUDIOFILENAME_FLAG_NONE          0
/** Adds an ISO timestamp to the file name. */
#define PDMAUDIOFILENAME_FLAG_TS            RT_BIT(0)

/**
 * Structure for an audio file handle.
 */
typedef struct PDMAUDIOFILE
{
    /** Type of the audio file. */
    PDMAUDIOFILETYPE    enmType;
    /** Audio file flags. */
    PDMAUDIOFILEFLAGS   fFlags;
    /** File name and path. */
    char                szName[RTPATH_MAX + 1];
    /** Actual file handle. */
    RTFILE              hFile;
    /** Data needed for the specific audio file type implemented.
     *  Optional, can be NULL. */
    void               *pvData;
    /** Data size (in bytes). */
    size_t              cbData;
} PDMAUDIOFILE, *PPDMAUDIOFILE;

/** Stream status flag. To be used with PDMAUDIOSTRMSTS_FLAG_ flags. */
typedef uint32_t PDMAUDIOSTREAMSTS;

/** No flags being set. */
#define PDMAUDIOSTREAMSTS_FLAG_NONE            0
/** Whether this stream has been initialized by the
 *  backend or not. */
#define PDMAUDIOSTREAMSTS_FLAG_INITIALIZED     RT_BIT_32(0)
/** Whether this stream is enabled or disabled. */
#define PDMAUDIOSTREAMSTS_FLAG_ENABLED         RT_BIT_32(1)
/** Whether this stream has been paused or not. This also implies
 *  that this is an enabled stream! */
#define PDMAUDIOSTREAMSTS_FLAG_PAUSED          RT_BIT_32(2)
/** Whether this stream was marked as being disabled
 *  but there are still associated guest output streams
 *  which rely on its data. */
#define PDMAUDIOSTREAMSTS_FLAG_PENDING_DISABLE RT_BIT_32(3)
/** Whether this stream is in re-initialization phase.
 *  All other bits remain untouched to be able to restore
 *  the stream's state after the re-initialization bas been
 *  finished. */
#define PDMAUDIOSTREAMSTS_FLAG_PENDING_REINIT  RT_BIT_32(4)
/** Validation mask. */
#define PDMAUDIOSTREAMSTS_VALID_MASK           UINT32_C(0x0000001F)

/**
 * Enumeration presenting a backend's current status.
 */
typedef enum PDMAUDIOBACKENDSTS
{
    /** Unknown/invalid status. */
    PDMAUDIOBACKENDSTS_UNKNOWN = 0,
    /** No backend attached. */
    PDMAUDIOBACKENDSTS_NOT_ATTACHED,
    /** The backend is in its initialization phase.
     *  Not all backends support this status. */
    PDMAUDIOBACKENDSTS_INITIALIZING,
    /** The backend has stopped its operation. */
    PDMAUDIOBACKENDSTS_STOPPED,
    /** The backend is up and running. */
    PDMAUDIOBACKENDSTS_RUNNING,
    /** The backend ran into an error and is unable to recover.
     *  A manual re-initialization might help. */
    PDMAUDIOBACKENDSTS_ERROR,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOBACKENDSTS_32BIT_HACK = 0x7fffffff
} PDMAUDIOBACKENDSTS;

/**
 * Structure for keeping audio input stream specifics.
 * Do not use directly. Instead, use PDMAUDIOSTREAM.
 */
typedef struct PDMAUDIOSTREAMIN
{
#ifdef VBOX_WITH_STATISTICS
    struct
    {
        STAMCOUNTER BytesElapsed;
        STAMCOUNTER BytesTotalRead;
        STAMCOUNTER FramesCaptured;
    } Stats;
#endif
    struct
    {
        /** File for writing stream reads. */
        PPDMAUDIOFILE           pFileStreamRead;
        /** File for writing non-interleaved captures. */
        PPDMAUDIOFILE           pFileCaptureNonInterleaved;
    } Dbg;
} PDMAUDIOSTREAMIN, *PPDMAUDIOSTREAMIN;

/**
 * Structure for keeping audio output stream specifics.
 * Do not use directly. Instead, use PDMAUDIOSTREAM.
 */
typedef struct PDMAUDIOSTREAMOUT
{
#ifdef VBOX_WITH_STATISTICS
    struct
    {
        STAMCOUNTER BytesElapsed;
        STAMCOUNTER BytesTotalWritten;
        STAMCOUNTER FramesPlayed;
    } Stats;
#endif
    struct
    {
#ifdef DEBUG
        /** Number of audio frames written since the last playback (transfer)
         *  to the backend. */
        uint64_t                cfWrittenSinceLastPlay;
#endif
        /** File for writing stream writes. */
        PPDMAUDIOFILE           pFileStreamWrite;
        /** File for writing stream playback. */
        PPDMAUDIOFILE           pFilePlayNonInterleaved;
    } Dbg;
} PDMAUDIOSTREAMOUT, *PPDMAUDIOSTREAMOUT;

/** Pointer to an audio stream. */
typedef struct PDMAUDIOSTREAM *PPDMAUDIOSTREAM;

/**
 * Audio stream context.
 * Needed for separating data from the guest and host side (per stream).
 */
typedef struct PDMAUDIOSTREAMCTX
{
    /** The stream's audio configuration. */
    PDMAUDIOSTREAMCFG      Cfg;
    /** This stream's mixing buffer. */
    PDMAUDIOMIXBUF         MixBuf;
} PDMAUDIOSTREAMCTX;

/** Pointer to an audio stream context. */
typedef struct PDMAUDIOSTREAM *PPDMAUDIOSTREAMCTX;

/**
 * Structure for maintaining an input/output audio stream.
 */
typedef struct PDMAUDIOSTREAM
{
    /** List node. */
    RTLISTNODE             Node;
    /** Name of this stream. */
    char                   szName[64];
    /** Number of references to this stream. Only can be
     *  destroyed if the reference count is reaching 0. */
    uint32_t               cRefs;
    /** Stream status flag. */
    PDMAUDIOSTREAMSTS      fStatus;
    /** Audio direction of this stream. */
    PDMAUDIODIR            enmDir;
    /** The guest side of the stream. */
    PDMAUDIOSTREAMCTX      Guest;
    /** The host side of the stream. */
    PDMAUDIOSTREAMCTX      Host;
    /** Union for input/output specifics (based on enmDir). */
    union
    {
        PDMAUDIOSTREAMIN   In;
        PDMAUDIOSTREAMOUT  Out;
    } RT_UNION_NM(u);
    /** Timestamp (in ns) since last iteration. */
    uint64_t               tsLastIteratedNs;
    /** Timestamp (in ns) since last playback / capture. */
    uint64_t               tsLastPlayedCapturedNs;
    /** Timestamp (in ns) since last read (input streams) or
     *  write (output streams). */
    uint64_t               tsLastReadWrittenNs;
    /** For output streams this indicates whether the stream has reached
     *  its playback threshold, e.g. is playing audio.
     *  For input streams this  indicates whether the stream has enough input
     *  data to actually start reading audio. */
    bool                   fThresholdReached;
    /** Data to backend-specific stream data.
     *  This data block will be casted by the backend to access its backend-dependent data.
     *
     *  That way the backends do not have access to the audio connector's data. */
    void                  *pvBackend;
    /** Size (in bytes) of the backend-specific stream data. */
    size_t                 cbBackend;
} PDMAUDIOSTREAM;

/** Pointer to a audio connector interface. */
typedef struct PDMIAUDIOCONNECTOR *PPDMIAUDIOCONNECTOR;

/**
 * Enumeration for an audio callback source.
 */
typedef enum PDMAUDIOCBSOURCE
{
    /** Invalid, do not use. */
    PDMAUDIOCBSOURCE_INVALID    = 0,
    /** Device emulation. */
    PDMAUDIOCBSOURCE_DEVICE     = 1,
    /** Audio connector interface. */
    PDMAUDIOCBSOURCE_CONNECTOR  = 2,
    /** Backend (lower). */
    PDMAUDIOCBSOURCE_BACKEND    = 3,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOCBSOURCE_32BIT_HACK = 0x7fffffff
} PDMAUDIOCBSOURCE;

/**
 * Audio device callback types.
 * Those callbacks are being sent from the audio connector ->  device emulation.
 */
typedef enum PDMAUDIODEVICECBTYPE
{
    /** Invalid, do not use. */
    PDMAUDIODEVICECBTYPE_INVALID = 0,
    /** Data is availabe as input for passing to the device emulation. */
    PDMAUDIODEVICECBTYPE_DATA_INPUT,
    /** Free data for the device emulation to write to the backend. */
    PDMAUDIODEVICECBTYPE_DATA_OUTPUT,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIODEVICECBTYPE_32BIT_HACK = 0x7fffffff
} PDMAUDIODEVICECBTYPE;

/**
 * Device callback data for audio input.
 */
typedef struct PDMAUDIODEVICECBDATA_DATA_INPUT
{
    /** Input: How many bytes are availabe as input for passing
     *         to the device emulation. */
    uint32_t cbInAvail;
    /** Output: How many bytes have been read. */
    uint32_t cbOutRead;
} PDMAUDIODEVICECBDATA_DATA_INPUT, *PPDMAUDIODEVICECBDATA_DATA_INPUT;

/**
 * Device callback data for audio output.
 */
typedef struct PDMAUDIODEVICECBDATA_DATA_OUTPUT
{
    /** Input:  How many bytes are free for the device emulation to write. */
    uint32_t cbInFree;
    /** Output: How many bytes were written by the device emulation. */
    uint32_t cbOutWritten;
} PDMAUDIODEVICECBDATA_DATA_OUTPUT, *PPDMAUDIODEVICECBDATA_DATA_OUTPUT;

/**
 * Audio backend callback types.
 * Those callbacks are being sent from the backend -> audio connector.
 */
typedef enum PDMAUDIOBACKENDCBTYPE
{
    /** Invalid, do not use. */
    PDMAUDIOBACKENDCBTYPE_INVALID = 0,
    /** The backend's status has changed. */
    PDMAUDIOBACKENDCBTYPE_STATUS,
    /** One or more host audio devices have changed. */
    PDMAUDIOBACKENDCBTYPE_DEVICES_CHANGED,
    /** Hack to blow the type up to 32-bit. */
    PDMAUDIOBACKENDCBTYPE_32BIT_HACK = 0x7fffffff
} PDMAUDIOBACKENDCBTYPE;

/** Pointer to a host audio interface. */
typedef struct PDMIHOSTAUDIO *PPDMIHOSTAUDIO;

/**
 * Host audio callback function.
 * This function will be called from a backend to communicate with the host audio interface.
 *
 * @returns IPRT status code.
 * @param   pDrvIns             Pointer to driver instance which called us.
 * @param   enmType             Callback type.
 * @param   pvUser              User argument.
 * @param   cbUser              Size (in bytes) of user argument.
 */
typedef DECLCALLBACK(int) FNPDMHOSTAUDIOCALLBACK(PPDMDRVINS pDrvIns, PDMAUDIOBACKENDCBTYPE enmType, void *pvUser, size_t cbUser);
/** Pointer to a FNPDMHOSTAUDIOCALLBACK(). */
typedef FNPDMHOSTAUDIOCALLBACK *PFNPDMHOSTAUDIOCALLBACK;

/**
 * Audio callback registration record.
 */
typedef struct PDMAUDIOCBRECORD
{
    /** List node. */
    RTLISTANCHOR        Node;
    /** Callback source. */
    PDMAUDIOCBSOURCE    enmSource;
    /** Callback type, based on the given source. */
    union
    {
        /** Device callback stuff. */
        struct
        {
            PDMAUDIODEVICECBTYPE enmType;
        } Device;
    } RT_UNION_NM(u);
    /** Pointer to context data. Optional. */
    void               *pvCtx;
    /** Size (in bytes) of context data.
     *  Must be 0 if pvCtx is NULL. */
    size_t              cbCtx;
} PDMAUDIOCBRECORD, *PPDMAUDIOCBRECORD;

#define PPDMAUDIOBACKENDSTREAM void *

/**
 * Audio connector interface (up).
 */
typedef struct PDMIAUDIOCONNECTOR
{
    /**
     * Enables or disables the given audio direction for this driver.
     *
     * When disabled, assiociated output streams consume written audio without passing them further down to the backends.
     * Associated input streams then return silence when read from those.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   enmDir          Audio direction to enable or disable driver for.
     * @param   fEnable         Whether to enable or disable the specified audio direction.
     */
    DECLR3CALLBACKMEMBER(int, pfnEnable, (PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir, bool fEnable));

    /**
     * Returns whether the given audio direction for this driver is enabled or not.
     *
     * @returns True if audio is enabled for the given direction, false if not.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   enmDir          Audio direction to retrieve enabled status for.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsEnabled, (PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir));

    /**
     * Retrieves the current configuration of the host audio backend.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pCfg            Where to store the host audio backend configuration data.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetConfig, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOBACKENDCFG pCfg));

    /**
     * Retrieves the current status of the host audio backend.
     *
     * @returns Status of the host audio backend.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   enmDir          Audio direction to check host audio backend for. Specify PDMAUDIODIR_ANY for the overall
     *                          backend status.
     */
    DECLR3CALLBACKMEMBER(PDMAUDIOBACKENDSTS, pfnGetStatus, (PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir));

    /**
     * Creates an audio stream.
     *
     * @returns VBox status code.
     * @param   pInterface           Pointer to the interface structure containing the called function pointer.
     * @param   pCfgHost             Stream configuration for host side.
     * @param   pCfgGuest            Stream configuration for guest side.
     * @param   ppStream             Pointer where to return the created audio stream on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamCreate, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest, PPDMAUDIOSTREAM *ppStream));

    /**
     * Destroys an audio stream.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamDestroy, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Adds a reference to the specified audio stream.
     *
     * @returns New reference count. UINT32_MAX on error.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream adding the reference to.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamRetain, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Releases a reference from the specified stream.
     *
     * @returns New reference count. UINT32_MAX on error.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream releasing a reference from.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamRelease, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Reads PCM audio data from the host (input).
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream to write to.
     * @param   pvBuf           Where to store the read data.
     * @param   cbBuf           Number of bytes to read.
     * @param   pcbRead         Bytes of audio data read. Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamRead, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead));

    /**
     * Writes PCM audio data to the host (output).
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream to read from.
     * @param   pvBuf           Audio data to be written.
     * @param   cbBuf           Number of bytes to be written.
     * @param   pcbWritten      Bytes of audio data written. Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamWrite, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten));

    /**
     * Controls a specific audio stream.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     * @param   enmStreamCmd    The stream command to issue.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamControl, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd));

    /**
     * Processes stream data.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamIterate, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Returns the number of readable data (in bytes) of a specific audio input stream.
     *
     * @returns Number of readable data (in bytes).
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamGetReadable, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Returns the number of writable data (in bytes) of a specific audio output stream.
     *
     * @returns Number of writable data (in bytes).
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamGetWritable, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Returns the status of a specific audio stream.
     *
     * @returns Audio stream status
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(PDMAUDIOSTREAMSTS, pfnStreamGetStatus, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream));

    /**
     * Sets the audio volume of a specific audio stream.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pStream         Pointer to audio stream.
     * @param   pVol            Pointer to audio volume structure to set the stream's audio volume to.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamSetVolume, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, PPDMAUDIOVOLUME pVol));

    /**
     * Plays (transfers) available audio frames to the host backend. Only works with output streams.
     *
     * @returns VBox status code.
     * @param   pInterface           Pointer to the interface structure containing the called function pointer.
     * @param   pStream              Pointer to audio stream.
     * @param   pcFramesPlayed       Number of frames played. Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamPlay, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, uint32_t *pcFramesPlayed));

    /**
     * Captures (transfers) available audio frames from the host backend. Only works with input streams.
     *
     * @returns VBox status code.
     * @param   pInterface           Pointer to the interface structure containing the called function pointer.
     * @param   pStream              Pointer to audio stream.
     * @param   pcFramesCaptured     Number of frames captured. Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamCapture, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, uint32_t *pcFramesCaptured));

    /**
     * Registers (device) callbacks.
     * This is handy for letting the device emulation know of certain events, e.g. processing input / output data
     * or configuration changes.
     *
     * @returns VBox status code.
     * @param   pInterface           Pointer to the interface structure containing the called function pointer.
     * @param   paCallbacks          Pointer to array of callbacks to register.
     * @param   cCallbacks           Number of callbacks to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterCallbacks, (PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOCBRECORD paCallbacks, size_t cCallbacks));

} PDMIAUDIOCONNECTOR;

/** PDMIAUDIOCONNECTOR interface ID. */
#define PDMIAUDIOCONNECTOR_IID                  "A643B40C-733F-4307-9549-070AF0EE0ED6"

/**
 * Assigns all needed interface callbacks for an audio backend.
 *
 * @param   a_Prefix        The function name prefix.
 */
#define PDMAUDIO_IHOSTAUDIO_CALLBACKS(a_Prefix) \
    do { \
        pThis->IHostAudio.pfnInit              = RT_CONCAT(a_Prefix,Init); \
        pThis->IHostAudio.pfnShutdown          = RT_CONCAT(a_Prefix,Shutdown); \
        pThis->IHostAudio.pfnGetConfig         = RT_CONCAT(a_Prefix,GetConfig); \
        /** @todo Add pfnGetDevices here as soon as supported by all backends. */ \
        pThis->IHostAudio.pfnGetStatus         = RT_CONCAT(a_Prefix,GetStatus); \
        /** @todo Ditto for pfnSetCallback. */ \
        pThis->IHostAudio.pfnStreamCreate      = RT_CONCAT(a_Prefix,StreamCreate); \
        pThis->IHostAudio.pfnStreamDestroy     = RT_CONCAT(a_Prefix,StreamDestroy); \
        pThis->IHostAudio.pfnStreamControl     = RT_CONCAT(a_Prefix,StreamControl); \
        pThis->IHostAudio.pfnStreamGetReadable = RT_CONCAT(a_Prefix,StreamGetReadable); \
        pThis->IHostAudio.pfnStreamGetWritable = RT_CONCAT(a_Prefix,StreamGetWritable); \
        pThis->IHostAudio.pfnStreamGetStatus   = RT_CONCAT(a_Prefix,StreamGetStatus); \
        pThis->IHostAudio.pfnStreamIterate     = RT_CONCAT(a_Prefix,StreamIterate); \
        pThis->IHostAudio.pfnStreamPlay        = RT_CONCAT(a_Prefix,StreamPlay); \
        pThis->IHostAudio.pfnStreamCapture     = RT_CONCAT(a_Prefix,StreamCapture); \
    } while (0)

/**
 * PDM host audio interface.
 */
typedef struct PDMIHOSTAUDIO
{
    /**
     * Initializes the host backend (driver).
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(int, pfnInit, (PPDMIHOSTAUDIO pInterface));

    /**
     * Shuts down the host backend (driver).
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(void, pfnShutdown, (PPDMIHOSTAUDIO pInterface));

    /**
     * Returns the host backend's configuration (backend).
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pBackendCfg         Where to store the backend audio configuration to.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetConfig, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg));

    /**
     * Returns (enumerates) host audio device information.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pDeviceEnum         Where to return the enumerated audio devices.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetDevices, (PPDMIHOSTAUDIO pInterface, PPDMAUDIODEVICEENUM pDeviceEnum));

    /**
     * Returns the current status from the audio backend.
     *
     * @returns PDMAUDIOBACKENDSTS enum.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   enmDir              Audio direction to get status for. Pass PDMAUDIODIR_ANY for overall status.
     */
    DECLR3CALLBACKMEMBER(PDMAUDIOBACKENDSTS, pfnGetStatus, (PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir));

    /**
     * Sets a callback the audio backend can call. Optional.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pfnCallback         The callback function to use, or NULL when unregistering.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetCallback, (PPDMIHOSTAUDIO pInterface, PFNPDMHOSTAUDIOCALLBACK pfnCallback));

    /**
     * Creates an audio stream using the requested stream configuration.
     * If a backend is not able to create this configuration, it will return its best match in the acquired configuration
     * structure on success.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     * @param   pCfgReq             Pointer to requested stream configuration.
     * @param   pCfgAcq             Pointer to acquired stream configuration.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamCreate, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq));

    /**
     * Destroys an audio stream.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamDestroy, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Controls an audio stream.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     * @param   enmStreamCmd        The stream command to issue.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamControl, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd));

    /**
     * Returns the amount which is readable from the audio (input) stream.
     *
     * @returns For non-raw layout streams: Number of readable bytes.
     *          for raw layout streams    : Number of readable audio frames.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamGetReadable, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Returns the amount which is writable to the audio (output) stream.
     *
     * @returns For non-raw layout streams: Number of writable bytes.
     *          for raw layout streams    : Number of writable audio frames.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamGetWritable, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Returns the amount which is pending (in other words has not yet been processed) by/from the backend yet.
     * Optional.
     *
     * For input streams this is read audio data by the backend which has not been processed by the host yet.
     * For output streams this is written audio data to the backend which has not been processed by the backend yet.
     *
     * @returns For non-raw layout streams: Number of pending bytes.
     *          for raw layout streams    : Number of pending audio frames.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnStreamGetPending, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Returns the current status of the given backend stream.
     *
     * @returns PDMAUDIOSTREAMSTS
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(PDMAUDIOSTREAMSTS, pfnStreamGetStatus, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Gives the host backend the chance to do some (necessary) iteration work.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamIterate, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Signals the backend that the host wants to begin playing for this iteration. Optional.
     *
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(void, pfnStreamPlayBegin, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Plays (writes to) an audio (output) stream.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     * @param   pvBuf               Pointer to audio data buffer to play.
     * @param   cxBuf               For non-raw layout streams: Size (in bytes) of audio data buffer,
     *                              for raw layout streams    : Size (in audio frames) of audio data buffer.
     * @param   pcxWritten          For non-raw layout streams: Returns number of bytes written.   Optional.
     *                              for raw layout streams    : Returns number of frames written.  Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamPlay, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf, uint32_t *pcxWritten));

    /**
     * Signals the backend that the host finished playing for this iteration. Optional.
     *
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(void, pfnStreamPlayEnd, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Signals the backend that the host wants to begin capturing for this iteration. Optional.
     *
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(void, pfnStreamCaptureBegin, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

    /**
     * Captures (reads from) an audio (input) stream.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     * @param   pvBuf               Buffer where to store read audio data.
     * @param   cxBuf               For non-raw layout streams: Size (in bytes) of audio data buffer,
     *                              for raw layout streams    : Size (in audio frames) of audio data buffer.
     * @param   pcxRead             For non-raw layout streams: Returns number of bytes read.   Optional.
     *                              for raw layout streams    : Returns number of frames read.  Optional.
     */
    DECLR3CALLBACKMEMBER(int, pfnStreamCapture, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream, void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead));

    /**
     * Signals the backend that the host finished capturing for this iteration. Optional.
     *
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   pStream             Pointer to audio stream.
     */
    DECLR3CALLBACKMEMBER(void, pfnStreamCaptureEnd, (PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream));

} PDMIHOSTAUDIO;

/** PDMIHOSTAUDIO interface ID. */
#define PDMIHOSTAUDIO_IID                           "640F5A31-8245-491C-538F-29A0F9D08881"

/** @} */

#endif /* !___VBox_vmm_pdmaudioifs_h */

