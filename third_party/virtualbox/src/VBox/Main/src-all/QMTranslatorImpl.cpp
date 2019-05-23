/* $Id: QMTranslatorImpl.cpp $ */
/** @file
 * VirtualBox API translation handling class
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include <iprt/file.h>
#include <iprt/asm.h>
#include <VBox/com/string.h>
#include <VBox/log.h>
#include <QMTranslator.h>

/* QM File Magic Number */
static const size_t MagicLength = 16;
static const uint8_t Magic[MagicLength] =
{
    0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95,
    0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd
};

/* Used internally */
class QMException : public std::exception
{
    const char *m_str;
public:
    QMException(const char *str) : m_str(str) {}
    virtual const char *what() const throw() { return m_str; }
};

/* Bytes stream. Used by the parser to iterate through the data */
class QMBytesStream
{
    size_t m_cbSize;
    const uint8_t * const m_dataStart;
    const uint8_t *m_iter;
    const uint8_t *m_end;

    /* Function stub for transform method */
    static uint16_t func_BE2H_U16(uint16_t value)
    {
        return RT_BE2H_U16(value);
    }

public:

    QMBytesStream(const uint8_t *const dataStart, size_t cbSize) :
        m_cbSize(dataStart ? cbSize : 0),
        m_dataStart(dataStart),
        m_iter(dataStart)
    {
        setEnd();
    }

    /* Sets end pointer
     * Used in message reader to detect the end of message block */
    inline void setEnd(size_t pos = 0)
    {
        m_end = m_dataStart + (pos && pos < m_cbSize ? pos : m_cbSize);
    }

    inline uint8_t read8()
    {
        checkSize(1);
        return *m_iter++;
    }

    inline uint32_t read32()
    {
        checkSize(4);
        uint32_t result = *reinterpret_cast<const uint32_t *>(m_iter);
        m_iter += 4;
        return RT_BE2H_U32(result);
    }

    /* Reads string in UTF16 and converts it into a UTF8 string */
    inline com::Utf8Str readUtf16String()
    {
        uint32_t size = read32();
        checkSize(size);
        if (size & 1) throw QMException("Incorrect string size");
        std::vector<uint16_t> wstr;
        wstr.reserve(size / 2);

        /* We cannot convert to host endianess without copying the data
         * since the file might be mapped to the memory and any memory
         * change will lead to the change of the file. */
        std::transform(reinterpret_cast<const uint16_t *>(m_iter),
                       reinterpret_cast<const uint16_t *>(m_iter + size),
                       std::back_inserter(wstr),
                       func_BE2H_U16);
        m_iter += size;
        return com::Utf8Str((CBSTR) &wstr.front(), wstr.size());
    }

    /* Reads string in one-byte encoding
     * The string is assumed to be in ISO-8859-1 encoding */
    inline com::Utf8Str readString()
    {
        uint32_t size = read32();
        checkSize(size);
        com::Utf8Str result(reinterpret_cast<const char *>(m_iter), size);
        m_iter += size;
        return result;
    }

    /* Checks the magic number
     * Should be called when in the beginning of the data */
    inline void checkMagic()
    {
        checkSize(MagicLength);
        if (memcmp(&(*m_iter), Magic, MagicLength)) throw QMException("Wrong magic number");
        m_iter += MagicLength;
    }

    /* Has we reached the end pointer? */
    inline bool hasFinished() { return m_iter == m_end; }

    /* Returns current stream position */
    inline size_t tellPos() { return m_iter - m_dataStart; }

    /* Moves current pointer to a desired position */
    inline void seek(int pos) { m_iter += pos; }

    /* Checks whether stream has enough data to read size bytes */
    inline void checkSize(int size)
    {
        if (m_end - m_iter < size) throw QMException("Incorrect item size");
    }
};

/* Internal QMTranslator implementation */
class QMTranslator_Impl
{
    struct QMMessage
    {
        /* Everything is in UTF-8 */
        com::Utf8Str strContext;
        com::Utf8Str strTranslation;
        com::Utf8Str strComment;
        com::Utf8Str strSource;
        uint32_t     hash;
        QMMessage() : hash(0) {}
    };

    struct HashOffset
    {
        uint32_t hash;
        uint32_t offset;

        HashOffset(uint32_t _hash = 0, uint32_t _offs = 0) : hash(_hash), offset(_offs) {}

        bool operator<(const HashOffset &obj) const
        {
            return (hash != obj.hash ? hash < obj.hash : offset < obj.offset);
        }

    };

    typedef std::set<HashOffset> QMHashSet;
    typedef QMHashSet::const_iterator QMHashSetConstIter;
    typedef std::vector<QMMessage> QMMessageArray;

    QMHashSet m_hashSet;
    QMMessageArray m_messageArray;

public:

    QMTranslator_Impl() {}

    const char *translate(const char *pszContext,
                          const char *pszSource,
                          const char *pszDisamb) const
    {
        QMHashSetConstIter iter;
        QMHashSetConstIter lowerIter, upperIter;

        do {
            uint32_t hash = calculateHash(pszSource, pszDisamb);
            lowerIter = m_hashSet.lower_bound(HashOffset(hash, 0));
            upperIter = m_hashSet.upper_bound(HashOffset(hash, UINT32_MAX));

            for (iter = lowerIter; iter != upperIter; ++iter)
            {
                const QMMessage &message = m_messageArray[iter->offset];
                if ((!pszContext || !*pszContext || message.strContext == pszContext) &&
                    message.strSource == pszSource &&
                    ((pszDisamb && !*pszDisamb) || message.strComment == pszDisamb))
                    break;
            }

            /* Try without disambiguating comment if it isn't empty */
            if (pszDisamb)
            {
                if (!*pszDisamb) pszDisamb = 0;
                else pszDisamb = "";
            }

        } while (iter == upperIter && pszDisamb);

        return (iter != upperIter ? m_messageArray[iter->offset].strTranslation.c_str() : "");
    }

    void load(QMBytesStream &stream)
    {
        /* Load into local variables. If we failed during the load,
         * it would allow us to keep the object in a valid (previous) state. */
        QMHashSet hashSet;
        QMMessageArray messageArray;

        stream.checkMagic();

        while (!stream.hasFinished())
        {
            uint32_t sectionCode = stream.read8();
            uint32_t sLen = stream.read32();

            /* Hashes and Context sections are ignored. They contain hash tables
             * to speed-up search which is not useful since we recalculate all hashes
             * and don't perform context search by hash */
            switch (sectionCode)
            {
                case Messages:
                    parseMessages(stream, &hashSet, &messageArray, sLen);
                    break;
                case Hashes:
                    /* Only get size information to speed-up vector filling
                     * if Hashes section goes in the file before Message section */
                    m_messageArray.reserve(sLen >> 3);
                    RT_FALL_THRU();
                case Context:
                    stream.seek(sLen);
                    break;
                default:
                    throw QMException("Unkown section");
            }
        }
        /* Store the data into member variables.
         * The following functions never generate exceptions */
        m_hashSet.swap(hashSet);
        m_messageArray.swap(messageArray);
    }

private:

    /* Some QM stuff */
    enum SectionType
    {
        Hashes   = 0x42,
        Messages = 0x69,
        Contexts = 0x2f
    };

    enum MessageType
    {
        End          = 1,
        SourceText16 = 2,
        Translation  = 3,
        Context16    = 4,
        Hash         = 5,
        SourceText   = 6,
        Context      = 7,
        Comment      = 8
    };

    /* Read messages from the stream. */
    static void parseMessages(QMBytesStream &stream, QMHashSet * const hashSet, QMMessageArray * const messageArray, size_t cbSize)
    {
        stream.setEnd(stream.tellPos() + cbSize);
        uint32_t cMessage = 0;
        while (!stream.hasFinished())
        {
            QMMessage message;
            HashOffset hashOffs;

            parseMessageRecord(stream, &message);
            if (!message.hash)
                message.hash = calculateHash(message.strSource.c_str(), message.strComment.c_str());

            hashOffs.hash = message.hash;
            hashOffs.offset = cMessage++;

            hashSet->insert(hashOffs);
            messageArray->push_back(message);
        }
        stream.setEnd();
    }

    /* Parse one message from the stream */
    static void parseMessageRecord(QMBytesStream &stream, QMMessage * const message)
    {
        while (!stream.hasFinished())
        {
            uint8_t type = stream.read8();
            switch (type)
            {
                case End:
                    return;
                /* Ignored as obsolete */
                case Context16:
                case SourceText16:
                    stream.seek(stream.read32());
                    break;
                case Translation:
                {
                    com::Utf8Str str = stream.readUtf16String();
                    message->strTranslation.swap(str);
                    break;
                }
                case Hash:
                    message->hash = stream.read32();
                    break;

                case SourceText:
                {
                    com::Utf8Str str = stream.readString();
                    message->strSource.swap(str);
                    break;
                }

                case Context:
                {
                    com::Utf8Str str = stream.readString();
                    message->strContext.swap(str);
                    break;
                }

                case Comment:
                {
                    com::Utf8Str str = stream.readString();
                    message->strComment.swap(str);
                    break;
                }

                default:
                    /* Ignore unknown block */
                    LogRel(("QMTranslator::parseMessageRecord(): Unkown message block %x\n", type));
                    break;
            }
        }
    }

    /* Defines the so called `hashpjw' function by P.J. Weinberger
       [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
       1986, 1987 Bell Telephone Laboratories, Inc.]   */
    static uint32_t calculateHash(const char *pszStr1, const char *pszStr2 = 0)
    {
        uint32_t hash = 0, g;

        for (const char *pszStr = pszStr1; pszStr != pszStr2; pszStr = pszStr2)
            for (; pszStr && *pszStr; pszStr++)
            {
                hash = (hash << 4) + static_cast<uint8_t>(*pszStr);

                if ((g = hash & 0xf0000000ul) != 0)
                {
                    hash ^= g >> 24;
                    hash ^= g;
                }
            }

        return (hash != 0 ? hash : 1);
    }
};

/* Inteface functions implementation */
QMTranslator::QMTranslator() : _impl(new QMTranslator_Impl) {}

QMTranslator::~QMTranslator() { delete _impl; }

const char *QMTranslator::translate(const char *pszContext, const char *pszSource, const char *pszDisamb) const throw()
{
    return _impl->translate(pszContext, pszSource, pszDisamb);
}

/* The function is noexcept for now but it may be changed
 * to throw exceptions if required to catch them in another
 * place. */
int QMTranslator::load(const char *pszFilename) throw()
{
    /* To free safely the file in case of exception */
    struct FileLoader
    {
        uint8_t *data;
        size_t cbSize;
        int rc;
        FileLoader(const char *pszFname)
        {
            rc = RTFileReadAll(pszFname, (void**) &data, &cbSize);
        }

        ~FileLoader()
        {
            if (isSuccess())
                RTFileReadAllFree(data, cbSize);
        }
        bool isSuccess() { return RT_SUCCESS(rc); }
    };

    try
    {
        FileLoader loader(pszFilename);
        if (loader.isSuccess())
        {
            QMBytesStream stream(loader.data, loader.cbSize);
            _impl->load(stream);
        }
        return loader.rc;
    }
    catch(std::exception &e)
    {
        LogRel(("QMTranslator::load() failed to load file '%s', reason: %s\n", pszFilename, e.what()));
        return VERR_INTERNAL_ERROR;
    }
    catch(...)
    {
        LogRel(("QMTranslator::load() failed to load file '%s'\n", pszFilename));
        return VERR_GENERAL_FAILURE;
    }
}
