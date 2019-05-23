/* $Id: EbmlWriter.h $ */
/** @file
 * EbmlWriter.h - EBML writer + WebM container.
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

#ifndef ____EBMLWRITER
#define ____EBMLWRITER

#ifdef VBOX_WITH_LIBVPX
# ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4668) /* vpx_codec.h(64) : warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#  include <vpx/vpx_encoder.h>
#  pragma warning(pop)
# else
#  include <vpx/vpx_encoder.h>
# endif
#endif /* VBOX_WITH_LIBVPX */

#include <iprt/file.h>

#include <VBox/com/string.h> /* For Utf8Str. */

using namespace com;

class WebMWriter_Impl;

class WebMWriter
{

public:

    /**
     * Supported audio codecs.
     */
    enum AudioCodec
    {
        /** No audio codec specified. */
        AudioCodec_None = 0,
        /** Opus. */
        AudioCodec_Opus = 1
    };

    /**
     * Supported video codecs.
     */
    enum VideoCodec
    {
        /** No video codec specified. */
        VideoCodec_None = 0,
        /** VP8. */
        VideoCodec_VP8  = 1
    };

#ifdef VBOX_WITH_LIBVPX
    /**
     * Block data for VP8-encoded video data.
     */
    struct BlockData_VP8
    {
        const vpx_codec_enc_cfg_t *pCfg;
        const vpx_codec_cx_pkt_t  *pPkt;
    };
#endif /* VBOX_WITH_LIBVPX */

#ifdef VBOX_WITH_LIBOPUS
    /**
     * Block data for Opus-encoded audio data.
     */
    struct BlockData_Opus
    {
        /** Pointer to encoded Opus audio data. */
        const void *pvData;
        /** Size (in bytes) of encoded Opus audio data. */
        size_t      cbData;
        /** Duration (in ms) of encoded Opus audio data. */
        uint64_t    uDurationMs;
    };
#endif /* VBOX_WITH_LIBOPUS */

public:

    WebMWriter();
    virtual ~WebMWriter();

    /**
     * Creates an output file using an already open file handle.
     *
     * @param   a_pszFilename   Name of the file the file handle points at.
     * @param   a_phFile        Pointer to open file handle to use.
     * @param   a_enmAudioCodec Audio codec to use.
     * @param   a_enmVideoCodec Video codec to use.
     *
     * @returns VBox status code. */
    int CreateEx(const char *a_pszFilename, PRTFILE a_phFile,
                 WebMWriter::AudioCodec a_enmAudioCodec, WebMWriter::VideoCodec a_enmVideoCodec);

    /**
     * Creates an output file.
     *
     * @param   a_pszFilename   Name of the file to create.
     * @param   a_fOpen         File open mode of type RTFILE_O_.
     * @param   a_enmAudioCodec Audio codec to use.
     * @param   a_enmVideoCodec Video codec to use.
     *
     * @returns VBox status code. */
    int Create(const char *a_pszFilename, uint64_t a_fOpen,
               WebMWriter::AudioCodec a_enmAudioCodec, WebMWriter::VideoCodec a_enmVideoCodec);

    /** Closes output file. */
    int Close(void);

    int AddAudioTrack(uint16_t uHz, uint8_t cChannels, uint8_t cBits, uint8_t *puTrack);

    int AddVideoTrack(uint16_t uWidth, uint16_t uHeight, double dbFPS, uint8_t *puTrack);

    /**
     * Writes a block of compressed data.
     *
     * @param uTrack            Track number to write data to.
     * @param pvData            Pointer to block data to write.
     * @param cbData            Size (in bytes) of block data to write.
     *
     * @returns VBox status code.
     */
    int WriteBlock(uint8_t uTrack, const void *pvData, size_t cbData);

    /**
     * Gets file name.
     *
     * @returns File name as UTF-8 string.
     */
    const Utf8Str& GetFileName(void);

    /**
     * Gets current output file size.
     *
     * @returns File size in bytes.
     */
    uint64_t GetFileSize(void);

    /**
     * Gets current free storage space available for the file.
     *
     * @returns Available storage free space.
     */
    uint64_t GetAvailableSpace(void);

private:

    /** WebMWriter implementation.
     *  To isolate some include files. */
    WebMWriter_Impl *m_pImpl;

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(WebMWriter);
};

#endif /* !____EBMLWRITER */
