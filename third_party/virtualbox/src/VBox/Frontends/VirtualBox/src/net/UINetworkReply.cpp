/* $Id: UINetworkReply.cpp $ */
/** @file
 * VBox Qt GUI - UINetworkReply stuff implementation.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QDir>
# include <QFile>
# include <QThread>
# include <QRegExp>
# include <QVector>
# include <QVariant>

/* GUI includes: */
# include "UINetworkReply.h"
# include "UINetworkManager.h"
# include "UIExtraDataManager.h"
# ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
#  include "VBoxGlobal.h"
#  include "VBoxUtils.h"
# else /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
#  include <VBox/log.h>
# endif /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */

/* Other VBox includes: */
# include <iprt/initterm.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Other VBox includes: */
#include <iprt/crypto/pem.h>
#include <iprt/crypto/store.h>
#include <iprt/err.h>
#include <iprt/http.h>
#include <iprt/path.h>
#include <iprt/sha.h>
#include <iprt/string.h>
#include <iprt/zip.h>


/** QThread extension
  * used as network-reply private thread interface. */
class UINetworkReplyPrivateThread : public QThread
{
#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
    Q_OBJECT;

signals:

    /** Notifies listeners about reply progress change.
      * @param  iBytesReceived  Holds the current amount of bytes received.
      * @param  iBytesTotal     Holds the total amount of bytes to be received. */
    void sigDownloadProgress(qint64 iBytesReceived, qint64 iBytesTotal);
#endif /* !VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */

public:

    /** Constructs network-reply thread of the passed @a type for the passed @a url and @a requestHeaders. */
    UINetworkReplyPrivateThread(UINetworkRequestType type, const QUrl &url, const UserDictionary &requestHeaders);

    /** @name APIs
     * @{ */
        /** Aborts reply. */
        void abort();

        /** Returns the URL of the reply which is the URL of the request for now. */
        const QUrl& url() const { return m_url; }

        /** Returns the last cached IPRT HTTP error of the reply. */
        int error() const { return m_iError; }

        /** Returns binary content of the reply. */
        const QByteArray& readAll() const { return m_reply; }
        /** Returns value for the cached reply header of the passed @a type. */
        QString header(UINetworkReply::KnownHeader type) const;

        /** Returns short descriptive context of thread's current operation. */
        const QString context() const { return m_strContext; }
    /** @} */

private:

    /** @name Helpers for HTTP and Certificates handling.
     * @{ */
        /** Applies configuration. */
        int applyConfiguration();
        /** Applies proxy rules. */
        int applyProxyRules();
        /** Applies security certificates. */
        int applyHttpsCertificates();
        /** Applies raw headers. */
        int applyRawHeaders();
        /** Performs main request. */
        int performMainRequest();

        /** Performs whole thread functionality. */
        void run();

        /** Handles download progress callback.
          * @param  cbDownloadTotal  Brings the total amount of bytes to be received.
          * @param  cbDownloaded     Brings the current amount of bytes received. */
        void handleProgressChange(uint64_t cbDownloadTotal, uint64_t cbDownloaded);
    /** @} */

    /** @name Static helpers for HTTP and Certificates handling.
     * @{ */
        /** Returns full certificate file-name. */
        static QString fullCertificateFileName();

        /** Applies proxy rules.
          * @remarks  Implementation doesn't exists, to be removed? */
        static int applyProxyRules(RTHTTP hHttp, const QString &strHostName, int iPort);

        /** Applies raw headers.
          * @param  hHttp    Brings the HTTP client instance.
          * @param  headers  Brings the map of headers to be applied. */
        static int applyRawHeaders(RTHTTP hHttp, const UserDictionary &headers);

        /** Returns the number of certificates found in a search result array.
          * @param  pafFoundCerts  Brings the array parallel to s_aCerts with the status of each wanted certificate. */
        static unsigned countCertsFound(bool const *pafFoundCerts);

        /** Returns whether we've found all the necessary certificates.
          * @param  pafFoundCerts  Brings the array parallel to s_aCerts with the status of each wanted certificate. */
        static bool areAllCertsFound(bool const *pafFoundCerts);

        /** Refreshes the certificates.
          * @param  hHttp          Brings the HTTP client instance. (Can be NIL when running the testcase.)
          * @param  phStore        On input, this holds the current store, so that we can fish out wanted
          *                        certificates from it. On successful return, this is replaced with a new
          *                        store reflecting the refrehsed content of @a pszCaCertFile.
          * @param  pafFoundCerts  On input, this holds the certificates found in the current store.
          *                        On return, this reflects what is current in the @a pszCaCertFile.
          *                        The array runs parallel to s_aCerts.
          * @param  pszCaCertFile  Where to write the refreshed certificates if we've managed to gather
          *                        a collection that is at least as good as the old one. */
        static int refreshCertificates(RTHTTP hHttp, PRTCRSTORE phStore, bool *pafFoundCerts, const char *pszCaCertFile);

        /** Downloads missing certificates.
          * @param  hNewStore         On successful return, this store will contain newly downloaded certificates.
          * @param  pafNewFoundCerts  On successful return, this array parallel to s_aCerts will contain the
          *                           status of each newly downloaded certificate.
          * @param  hHttp             Brings the HTTP client instance.
          * @param  pStaticErrInfo    Unused currently. */
        static void downloadMissingCertificates(RTCRSTORE hNewStore, bool *pafNewFoundCerts, RTHTTP hHttp,
                                                PRTERRINFOSTATIC pStaticErrInfo);

        /** Converts a PEM certificate, verifies it against @a pCertInfo and adds it to the given store.
          * @param  hStore       The store to add certificate to.
          * @param  pvResponse   The raw PEM certificate file bytes.
          * @param  cbResponse   The number of bytes.
          * @param  pWantedCert  The certificate info (we use hashes and encoded size). */
        static int convertVerifyAndAddPemCertificateToStore(RTCRSTORE hStore, void const *pvResponse,
                                                            size_t cbResponse, PCRTCRCERTWANTED pWantedCert);

        /** Redirects download progress callback to particular object which can handle it.
          * @param  hHttp            Brings the HTTP client instance.
          * @param  pvUser           Brings the convenience pointer for the
          *                          user-agent object which should handle that callback.
          * @param  cbDownloadTotal  Brings the total amount of bytes to be received.
          * @param  cbDownloaded     Brings the current amount of bytes received. */
        static DECLCALLBACK(void) handleProgressChange(RTHTTP hHttp, void *pvUser, uint64_t cbDownloadTotal, uint64_t cbDownloaded);
    /** @} */

    /** Additinoal download nfo about wanted certificate. */
    typedef struct CERTINFO
    {
        /** Holds the filename of the zip file we download (PEM). */
        const char *pszZipFile;
        /** Lists direct URLs to PEM formatted files. */
        const char *apszUrls[4];
    } CERTINFO;

    /** Holds the request type. */
    const UINetworkRequestType m_type;
    /** Holds the request url. */
    const QUrl m_url;
    /** Holds the request headers. */
    const UserDictionary m_requestHeaders;

    /** Holds the IPRT HTTP client instance handle. */
    RTHTTP m_hHttp;
    /** Holds the last cached IPRT HTTP error of the reply. */
    int m_iError;
    /** Holds short descriptive context of thread's current operation. */
    QString m_strContext;
    /** Holds the reply instance. */
    QByteArray m_reply;
    /** Holds the cached reply headers. */
    UserDictionary m_headers;

    /** Holds the URLs to root zip files containing certificates we want. */
    static const char * const s_apszRootsZipUrls[];
    /** Holds the download details. */
    static const CERTINFO s_CertInfoPcaCls3Gen5;
    /** Holds the details on the certificates we are after.
      * The pvUser member points to a UINetworkReplyPrivateThread::CERTINFO. */
    static const RTCRCERTWANTED s_aCerts[];
    /** Holds the certificate file name (no path). */
    static const QString s_strCertificateFileName;

#ifdef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
public:
    /** Starts the test routine. */
    static void testIt(RTTEST hTest);
#endif /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
};


#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
/** QObject extension
  * used as network-reply private data interface. */
class UINetworkReplyPrivate : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about reply progress change.
      * @param  iBytesReceived  Holds the current amount of bytes received.
      * @param  iBytesTotal     Holds the total amount of bytes to be received. */
    void downloadProgress(qint64 iBytesReceived, qint64 iBytesTotal);

    /** Notifies listeners about reply has finished processing. */
    void finished();

public:

    /** Constructs network-reply private data of the passed @a type for the passed @a url and @a requestHeaders. */
    UINetworkReplyPrivate(UINetworkRequestType type, const QUrl &url, const UserDictionary &requestHeaders);
    /** Destructs reply private data. */
    ~UINetworkReplyPrivate();

    /** Aborts reply. */
    void abort() { m_pThread->abort(); }

    /** Returns URL of the reply. */
    QUrl url() const { return m_pThread->url(); }

    /** Returns the last cached error of the reply. */
    UINetworkReply::NetworkError error() const { return m_error; }
    /** Returns the user-oriented string corresponding to the last cached error of the reply. */
    QString errorString() const;

    /** Returns binary content of the reply. */
    QByteArray readAll() const { return m_pThread->readAll(); }
    /** Returns value for the cached reply header of the passed @a type. */
    QString header(UINetworkReply::KnownHeader type) const { return m_pThread->header(type); }

private slots:

    /** Handles signal about reply has finished processing. */
    void sltFinished();

private:

    /** Holds full error template in "Context description: Error description" form. */
    QString m_strErrorTemplate;

    /** Holds the last cached error of the reply. */
    UINetworkReply::NetworkError m_error;

    /** Holds the reply thread instance. */
    UINetworkReplyPrivateThread *m_pThread;
};
#endif /* !VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */


/*********************************************************************************************************************************
*   Class UINetworkReplyPrivateThread implementation.                                                                            *
*********************************************************************************************************************************/

/* static */
const char * const UINetworkReplyPrivateThread::s_apszRootsZipUrls[] =
{
    "http://www.symantec.com/content/en/us/enterprise/verisign/roots/roots.zip"
};

/* static */
const UINetworkReplyPrivateThread::CERTINFO UINetworkReplyPrivateThread::s_CertInfoPcaCls3Gen5 =
{
    /*.pszZipFile     =*/
    "VeriSign Root Certificates/Generation 5 (G5) PCA/VeriSign Class 3 Public Primary Certification Authority - G5.pem",
    /*.apszUrls[]     =*/
    {
        "http://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem",
        "http://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem", /* (in case they correct above typo) */
        "http://www.verisign.com/repository/roots/root-certificates/PCA-3G5.pem", /* dead */
        NULL,
    }
};

/* static */
const RTCRCERTWANTED UINetworkReplyPrivateThread::s_aCerts[] =
{
    /*[0] =*/
    {
        /*.pszSubject        =*/
        "C=US, O=VeriSign, Inc., OU=VeriSign Trust Network, OU=(c) 2006 VeriSign, Inc. - For authorized use only, "
        "CN=VeriSign Class 3 Public Primary Certification Authority - G5",
        /*.cbEncoded         =*/    0x4d7,
        /*.Sha1Fingerprint   =*/    true,
        /*.Sha512Fingerprint =*/    true,
        /*.abSha1            =*/
        {
            0x4e, 0xb6, 0xd5, 0x78, 0x49, 0x9b, 0x1c, 0xcf, 0x5f, 0x58,
            0x1e, 0xad, 0x56, 0xbe, 0x3d, 0x9b, 0x67, 0x44, 0xa5, 0xe5
        },
        /*.abSha512          =*/
        {
            0xd4, 0xf8, 0x10, 0x54, 0x72, 0x77, 0x0a, 0x2d,
            0xe3, 0x17, 0xb3, 0xcf, 0xed, 0x61, 0xae, 0x5c,
            0x5d, 0x3e, 0xde, 0xa1, 0x41, 0x35, 0xb2, 0xdf,
            0x60, 0xe2, 0x61, 0xfe, 0x3a, 0xc1, 0x66, 0xa3,
            0x3c, 0x88, 0x54, 0x04, 0x4f, 0x1d, 0x13, 0x46,
            0xe3, 0x8c, 0x06, 0x92, 0x9d, 0x70, 0x54, 0xc3,
            0x44, 0xeb, 0x2c, 0x74, 0x25, 0x9e, 0x5d, 0xfb,
            0xd2, 0x6b, 0xa8, 0x9a, 0xf0, 0xb3, 0x6a, 0x01
        },
        /*.pvUser */ &UINetworkReplyPrivateThread::s_CertInfoPcaCls3Gen5
    },
};

/* static */
const QString UINetworkReplyPrivateThread::s_strCertificateFileName = QString("vbox-ssl-cacertificate.crt");

UINetworkReplyPrivateThread::UINetworkReplyPrivateThread(UINetworkRequestType type, const QUrl &url, const UserDictionary &requestHeaders)
    : m_type(type)
    , m_url(url)
    , m_requestHeaders(requestHeaders)
    , m_hHttp(NIL_RTHTTP)
    , m_iError(VINF_SUCCESS)
{
}

void UINetworkReplyPrivateThread::abort()
{
    /* Call for abort: */
    if (m_hHttp != NIL_RTHTTP)
        RTHttpAbort(m_hHttp);
}

QString UINetworkReplyPrivateThread::header(UINetworkReply::KnownHeader type) const
{
    /* Look for known header type: */
    switch (type)
    {
        case UINetworkReply::ContentTypeHeader:   return m_headers.value("Content-Type");
        case UINetworkReply::ContentLengthHeader: return m_headers.value("Content-Length");
        case UINetworkReply::LastModifiedHeader:  return m_headers.value("Last-Modified");
        case UINetworkReply::LocationHeader:      return m_headers.value("Location");
        default: break;
    }
    /* Return null-string by default: */
    return QString();
}

int UINetworkReplyPrivateThread::applyConfiguration()
{
    /* Install downloading progress callback: */
    return RTHttpSetDownloadProgressCallback(m_hHttp, &UINetworkReplyPrivateThread::handleProgressChange, this);
}

int UINetworkReplyPrivateThread::applyProxyRules()
{
    /* Set thread context: */
    m_strContext = tr("During proxy configuration");

#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
    /* Get the proxy-manager: */
    UIProxyManager proxyManager(gEDataManager->proxySettings());

    /* If the specific proxy settings are enabled, we'll use them
     * unless user disabled that functionality manually. */
    switch (proxyManager.proxyState())
    {
        case UIProxyManager::ProxyState_Enabled:
            return RTHttpSetProxy(m_hHttp,
                                  proxyManager.proxyHost().toUtf8().constData(),
                                  proxyManager.proxyPort().toUInt(),
                                  NULL /* pszProxyUser */, NULL /* pszProxyPwd */);
        case UIProxyManager::ProxyState_Disabled:
            return VINF_SUCCESS;
        default:
            break;
    }
#endif /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */

    /* By default, use system proxy: */
    return RTHttpUseSystemProxySettings(m_hHttp);
}

int UINetworkReplyPrivateThread::applyHttpsCertificates()
{
    /* Check if we really need SSL: */
    if (!url().toString().startsWith("https:", Qt::CaseInsensitive))
        return VINF_SUCCESS;

    /* Set thread context: */
    m_strContext = tr("During certificate downloading");

    /*
     * Calc the filename of the CA certificate file.
     */
    const QString strFullCertificateFileName(fullCertificateFileName());
    QByteArray utf8FullCertificateFileName = strFullCertificateFileName.toUtf8();
    const char *pszCaCertFile = utf8FullCertificateFileName.constData();

    /*
     * Check the state of our CA certificate file, it's one of the following:
     *      - Missing, recreate from scratch (= refresh).
     *      - Everything is there and it is less than 28 days old, do nothing.
     *      - Everything is there but it's older than 28 days, refresh.
     *      - Missing certificates and is older than 1 min, refresh.
     *
     * Start by creating a store for loading the current state into, as we'll
     * be need that for the refresh.
     */
    RTCRSTORE hCurStore = NIL_RTCRSTORE;
    int rc = RTCrStoreCreateInMem(&hCurStore, 256);
    if (RT_SUCCESS(rc))
    {
        bool fRefresh    = true;
        bool afCertsFound[RT_ELEMENTS(s_aCerts)];
        RT_ZERO(afCertsFound);

        /*
         * Load the file if it exists.
         *
         * To effect regular updates, we need the modification date of the file,
         * so we use RTPathQueryInfoEx here and not RTFileExists.
         */
        RTFSOBJINFO Info;
        int rc = RTPathQueryInfoEx(pszCaCertFile, &Info, RTFSOBJATTRADD_NOTHING, RTPATH_F_FOLLOW_LINK);
        if (   RT_SUCCESS(rc)
            && RTFS_IS_FILE(Info.Attr.fMode))
        {
            RTERRINFOSTATIC StaticErrInfo;
            rc = RTCrStoreCertAddFromFile(hCurStore, RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR, pszCaCertFile,
                                          RTErrInfoInitStatic(&StaticErrInfo));
            if (RTErrInfoIsSet(&StaticErrInfo.Core))
                LogRel(("checkCertificates: %s\n", StaticErrInfo.Core.pszMsg));
            else
                AssertRC(rc);

            /*
             * Scan the store the for certificates we need, then see what we
             * need to do wrt file age.
             */
            rc = RTCrStoreCertCheckWanted(hCurStore, s_aCerts, RT_ELEMENTS(s_aCerts), afCertsFound);
            AssertRC(rc);
            RTTIMESPEC RefreshAge;
            uint32_t   cSecRefresh = rc == VINF_SUCCESS  ? 28 * RT_SEC_1DAY /* all found */ : 60 /* stuff missing */;
            fRefresh = RTTimeSpecCompare(&Info.ModificationTime, RTTimeSpecSubSeconds(RTTimeNow(&RefreshAge), cSecRefresh)) <= 0;
        }

        /*
         * Refresh the file if necessary.
         */
        if (fRefresh)
            refreshCertificates(m_hHttp, &hCurStore, afCertsFound, pszCaCertFile);

        RTCrStoreRelease(hCurStore);

        /*
         * Final verdict.
         */
        if (areAllCertsFound(afCertsFound))
            rc = VINF_SUCCESS;
        else
            rc = VERR_NOT_FOUND; /** @todo r=bird: Why not try and let RTHttpGet* bitch if the necessary certs are missing? */

        /*
         * Set our custom CA file.
         */
        if (RT_SUCCESS(rc))
            rc = RTHttpSetCAFile(m_hHttp, pszCaCertFile);
    }
    return rc;
}

int UINetworkReplyPrivateThread::applyRawHeaders()
{
    /* Set thread context: */
    m_strContext = tr("During network request");

    /* Make sure we have a raw headers at all: */
    if (m_requestHeaders.isEmpty())
        return VINF_SUCCESS;

    /* Apply raw headers: */
    return applyRawHeaders(m_hHttp, m_requestHeaders);
}

int UINetworkReplyPrivateThread::performMainRequest()
{
    /* Set thread context: */
    m_strContext = tr("During network request");

    /* Paranoia: */
    m_reply.clear();

    /* Prepare result: */
    int rc = 0;

    /* Depending on request type: */
    switch (m_type)
    {
        case UINetworkRequestType_HEAD:
        {
            /* Perform blocking HTTP HEAD request: */
            void   *pvResponse = 0;
            size_t  cbResponse = 0;
            rc = RTHttpGetHeaderBinary(m_hHttp, m_url.toString().toUtf8().constData(), &pvResponse, &cbResponse);
            if (RT_SUCCESS(rc))
            {
                m_reply = QByteArray((char*)pvResponse, (int)cbResponse);
                RTHttpFreeResponse(pvResponse);
            }

            /* Paranoia: */
            m_headers.clear();

            /* Parse header contents: */
            const QString strHeaders = QString(m_reply);
            const QStringList headers = strHeaders.split("\n", QString::SkipEmptyParts);
            foreach (const QString &strHeader, headers)
            {
                const QStringList values = strHeader.split(": ", QString::SkipEmptyParts);
                if (values.size() > 1)
                    m_headers[values.at(0)] = values.at(1);
            }

            /* Special handling of redirection header: */
            if (rc == VERR_HTTP_REDIRECTED)
            {
                char *pszBuf = 0;
                const int rrc = RTHttpGetRedirLocation(m_hHttp, &pszBuf);
                if (RT_SUCCESS(rrc))
                    m_headers["Location"] = QString(pszBuf);
                if (pszBuf)
                    RTMemFree(pszBuf);
            }

            break;
        }
        case UINetworkRequestType_GET:
        {
            /* Perform blocking HTTP GET request: */
            void   *pvResponse = 0;
            size_t  cbResponse = 0;
            rc = RTHttpGetBinary(m_hHttp, m_url.toString().toUtf8().constData(), &pvResponse, &cbResponse);
            if (RT_SUCCESS(rc))
            {
                m_reply = QByteArray((char*)pvResponse, (int)cbResponse);
                RTHttpFreeResponse(pvResponse);
            }

            break;
        }
        default:
            break;
    }

    /* Return result: */
    return rc;
}

void UINetworkReplyPrivateThread::run()
{
    /* Init: */
    RTR3InitExeNoArguments(RTR3INIT_FLAGS_SUPLIB); /** @todo r=bird: WTF? */

    /* Create HTTP client: */
    m_iError = RTHttpCreate(&m_hHttp);
    if (RT_SUCCESS(m_iError))
    {
        /* Apply configuration: */
        if (RT_SUCCESS(m_iError))
            m_iError = applyConfiguration();

        /* Apply proxy-rules: */
        if (RT_SUCCESS(m_iError))
            m_iError = applyProxyRules();

        /* Apply https-certificates: */
        if (RT_SUCCESS(m_iError))
            m_iError = applyHttpsCertificates();

        /* Assign raw-headers: */
        if (RT_SUCCESS(m_iError))
            m_iError = applyRawHeaders();

        /* Perform main request: */
        if (RT_SUCCESS(m_iError))
            m_iError = performMainRequest();

        /* Destroy HTTP client: */
        RTHTTP hHttp = m_hHttp;
        if (hHttp != NIL_RTHTTP)
        {
            /** @todo r=bird: There is a race here between this and abort()! */
            m_hHttp = NIL_RTHTTP;
            RTHttpDestroy(hHttp);
        }
    }
}

void UINetworkReplyPrivateThread::handleProgressChange(uint64_t cbDownloadTotal, uint64_t cbDownloaded)
{
#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
    /* Notify listeners about progress change: */
    emit sigDownloadProgress(cbDownloaded, cbDownloadTotal);
#else /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
    Q_UNUSED(cbDownloaded);
    Q_UNUSED(cbDownloadTotal);
#endif /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
}

/* static */
QString UINetworkReplyPrivateThread::fullCertificateFileName()
{
#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS
    const QDir homeDir(QDir::toNativeSeparators(vboxGlobal().homeFolder()));
    return QDir::toNativeSeparators(homeDir.absoluteFilePath(s_strCertificateFileName));
#else /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
    return QString("/not/such/agency/non-existing-file.cer");
#endif /* VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */
}

/* static */
int UINetworkReplyPrivateThread::applyRawHeaders(RTHTTP hHttp, const UserDictionary &headers)
{
    /* Make sure HTTP is created: */
    if (hHttp == NIL_RTHTTP)
        return VERR_INVALID_HANDLE;

    /* We should format them first: */
    QVector<QByteArray> formattedHeaders;
    QVector<const char*> formattedHeaderPointers;
    foreach (const QString &header, headers.keys())
    {
        /* Prepare formatted representation: */
        QString strFormattedString = QString("%1: %2").arg(header, headers.value(header));
        formattedHeaders << strFormattedString.toUtf8();
        formattedHeaderPointers << formattedHeaders.last().constData();
    }
    const char **ppFormattedHeaders = formattedHeaderPointers.data();

    /* Apply HTTP headers: */
    return RTHttpSetHeaders(hHttp, formattedHeaderPointers.size(), ppFormattedHeaders);
}

/* static */
unsigned UINetworkReplyPrivateThread::countCertsFound(bool const *pafFoundCerts)
{
    unsigned cFound = 0;
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
        cFound += pafFoundCerts[i];
    return cFound;
}

/* static */
bool UINetworkReplyPrivateThread::areAllCertsFound(bool const *pafFoundCerts)
{
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
        if (!pafFoundCerts[i])
            return false;
    return true;
}

/* static */
int UINetworkReplyPrivateThread::refreshCertificates(RTHTTP hHttp, PRTCRSTORE phStore, bool *pafFoundCerts,
                                                     const char *pszCaCertFile)
{
    /*
     * Collect the standard assortment of SSL certificates.
     */
    uint32_t  cHint = RTCrStoreCertCount(*phStore);
    RTCRSTORE hNewStore;
    int rc = RTCrStoreCreateInMem(&hNewStore, cHint > 32 && cHint < _32K ? cHint + 16 : 256);
    if (RT_SUCCESS(rc))
    {
        RTERRINFOSTATIC StaticErrInfo;
        rc = RTHttpGatherCaCertsInStore(hNewStore, 0 /*fFlags*/, RTErrInfoInitStatic(&StaticErrInfo));
        if (RTErrInfoIsSet(&StaticErrInfo.Core))
            LogRel(("refreshCertificates/#1: %s\n", StaticErrInfo.Core.pszMsg));
        else if (rc == VERR_NOT_FOUND)
            LogRel(("refreshCertificates/#1: No trusted SSL certs found on the system, will try download...\n"));
        else
            AssertLogRelRC(rc);
        if (RT_SUCCESS(rc) || rc == VERR_NOT_FOUND)
        {
            /*
             * Check and see what we've got.  If we haven't got all we desire,
             * try add it from the previous store.
             */
            bool afNewFoundCerts[RT_ELEMENTS(s_aCerts)];
            RT_ZERO(afNewFoundCerts); /* paranoia */

            rc = RTCrStoreCertCheckWanted(hNewStore, s_aCerts, RT_ELEMENTS(s_aCerts), afNewFoundCerts);
            AssertLogRelRC(rc);
            Assert(rc != VINF_SUCCESS || areAllCertsFound(afNewFoundCerts));
            if (rc != VINF_SUCCESS)
            {
                rc = RTCrStoreCertAddWantedFromStore(hNewStore,
                                                     RTCRCERTCTX_F_ADD_IF_NOT_FOUND | RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR,
                                                     *phStore, s_aCerts, RT_ELEMENTS(s_aCerts), afNewFoundCerts);
                AssertLogRelRC(rc);
                Assert(rc != VINF_SUCCESS || areAllCertsFound(afNewFoundCerts));
            }

            /*
             * If that didn't help, seek out certificates in more obscure places,
             * like java, mozilla and mutt.
             */
            if (rc != VINF_SUCCESS)
            {
                rc = RTCrStoreCertAddWantedFromFishingExpedition(hNewStore,
                                                                 RTCRCERTCTX_F_ADD_IF_NOT_FOUND
                                                                 | RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR,
                                                                 s_aCerts, RT_ELEMENTS(s_aCerts), afNewFoundCerts,
                                                                 RTErrInfoInitStatic(&StaticErrInfo));
                if (RTErrInfoIsSet(&StaticErrInfo.Core))
                    LogRel(("refreshCertificates/#2: %s\n", StaticErrInfo.Core.pszMsg));
                Assert(rc != VINF_SUCCESS || areAllCertsFound(afNewFoundCerts));
            }

            /*
             * If that didn't help, try download the certificates.
             */
            if (rc != VINF_SUCCESS && hHttp != NIL_RTHTTP)
                downloadMissingCertificates(hNewStore, afNewFoundCerts, hHttp, &StaticErrInfo);

            /*
             * If we've got the same or better hit rate than the old store,
             * replace the CA certs file.
             */
            if (   areAllCertsFound(afNewFoundCerts)
                || countCertsFound(afNewFoundCerts) >= countCertsFound(pafFoundCerts) )
            {
                rc = RTCrStoreCertExportAsPem(hNewStore, 0 /*fFlags*/, pszCaCertFile);
                if (RT_SUCCESS(rc))
                {
                    LogRel(("refreshCertificates/#3: Found %u/%u SSL certs we/you trust (previously %u/%u).\n",
                            countCertsFound(afNewFoundCerts), RTCrStoreCertCount(hNewStore),
                            countCertsFound(pafFoundCerts), RTCrStoreCertCount(*phStore) ));

                    memcpy(pafFoundCerts, afNewFoundCerts, sizeof(afNewFoundCerts));
                    RTCrStoreRelease(*phStore);
                    *phStore  = hNewStore;
                    hNewStore = NIL_RTCRSTORE;
                }
                else
                {
                    RT_ZERO(pafFoundCerts);
                    LogRel(("refreshCertificates/#3: RTCrStoreCertExportAsPem unexpectedly failed with %Rrc\n", rc));
                }
            }
            else
                LogRel(("refreshCertificates/#3: Sticking with the old file, missing essential certs.\n"));
        }
        RTCrStoreRelease(hNewStore);
    }
    return rc;
}

/* static */
void UINetworkReplyPrivateThread::downloadMissingCertificates(RTCRSTORE hNewStore, bool *pafNewFoundCerts, RTHTTP hHttp,
                                                              PRTERRINFOSTATIC pStaticErrInfo)
{
    NOREF(pStaticErrInfo);
    int rc;

    /*
     * Try get the roots.zip from symantec (or virtualbox.org) first.
     */
    for (uint32_t iUrl = 0; iUrl < RT_ELEMENTS(s_apszRootsZipUrls); iUrl++)
    {
        void   *pvRootsZip;
        size_t  cbRootsZip;
        rc = RTHttpGetBinary(hHttp, s_apszRootsZipUrls[iUrl], &pvRootsZip, &cbRootsZip);
        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
                if (!pafNewFoundCerts[i])
                {
                    CERTINFO const *pInfo = (CERTINFO const *)s_aCerts[i].pvUser;
                    if (pInfo->pszZipFile)
                    {
                        void  *pvFile;
                        size_t cbFile;
                        rc = RTZipPkzipMemDecompress(&pvFile, &cbFile, pvRootsZip, cbRootsZip, pInfo->pszZipFile);
                        if (RT_SUCCESS(rc))
                        {
                            rc = convertVerifyAndAddPemCertificateToStore(hNewStore, pvFile, cbFile, &s_aCerts[i]);
                            RTMemFree(pvFile);
                            if (RT_SUCCESS(rc))
                            {
                                /*
                                 * Successfully added. Mark it as found and return if we've got them all.
                                 */
                                pafNewFoundCerts[i] = true;
                                if (areAllCertsFound(pafNewFoundCerts))
                                {
                                    RTHttpFreeResponse(pvRootsZip);
                                    return;
                                }
                            }
                        }
                    }
                }
            RTHttpFreeResponse(pvRootsZip);
        }
    }

    /*
     * Try download certificates separately.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aCerts); i++)
        if (!pafNewFoundCerts[i])
        {
            CERTINFO const *pInfo = (CERTINFO const *)s_aCerts[i].pvUser;
            for (uint32_t iUrl = 0; iUrl < RT_ELEMENTS(pInfo->apszUrls); iUrl++)
                if (pInfo->apszUrls[iUrl])
                {
                    void  *pvResponse;
                    size_t cbResponse;
                    rc = RTHttpGetBinary(hHttp, pInfo->apszUrls[iUrl], &pvResponse, &cbResponse);
                    if (RT_SUCCESS(rc))
                    {
                        rc = convertVerifyAndAddPemCertificateToStore(hNewStore, pvResponse, cbResponse, &s_aCerts[i]);
                        RTHttpFreeResponse(pvResponse);
                        if (RT_SUCCESS(rc))
                        {
                            pafNewFoundCerts[i] = true;
                            break;
                        }
                    }
                }
        }
}

/* static */
int UINetworkReplyPrivateThread::convertVerifyAndAddPemCertificateToStore(RTCRSTORE hStore,
                                                                          void const *pvResponse, size_t cbResponse,
                                                                          PCRTCRCERTWANTED pWantedCert)
{
    /*
     * Convert the PEM certificate to its binary form so we can hash it.
     */
    static RTCRPEMMARKERWORD const s_aWords_Certificate[]  = { { RT_STR_TUPLE("CERTIFICATE") } };
    static RTCRPEMMARKER     const s_aCertificateMarkers[] = { { s_aWords_Certificate, RT_ELEMENTS(s_aWords_Certificate) }, };
    RTERRINFOSTATIC StaticErrInfo;
    PCRTCRPEMSECTION pSectionHead;
    int rc = RTCrPemParseContent(pvResponse, cbResponse, 0 /*fFlags*/,
                                 &s_aCertificateMarkers[0], RT_ELEMENTS(s_aCertificateMarkers),
                                 &pSectionHead, RTErrInfoInitStatic(&StaticErrInfo));
    if (RTErrInfoIsSet(&StaticErrInfo.Core))
        LogRel(("RTCrPemParseContent: %s\n", StaticErrInfo.Core.pszMsg));
    if (RT_SUCCESS(rc))
    {
        /*
         * Look at what we got back and hash it.
         */
        rc = VERR_NOT_FOUND;
        for (PCRTCRPEMSECTION pCur = pSectionHead; pCur; pCur = pCur->pNext)
            if (pCur->cbData == pWantedCert->cbEncoded)
            {
                if (   RTSha1Check(pCur->pbData, pCur->cbData, pWantedCert->abSha1)
                    && RTSha512Check(pCur->pbData, pCur->cbData, pWantedCert->abSha512))
                {
                    /*
                     * Matching, add it to the store.
                     */
                    rc = RTCrStoreCertAddEncoded(hStore,
                                                 RTCRCERTCTX_F_ENC_X509_DER | RTCRCERTCTX_F_ADD_IF_NOT_FOUND,
                                                 pCur->pbData, pCur->cbData,
                                                 RTErrInfoInitStatic(&StaticErrInfo));
                    if (RTErrInfoIsSet(&StaticErrInfo.Core))
                        LogRel(("RTCrStoreCertAddEncoded: %s\n", StaticErrInfo.Core.pszMsg));
                    else if (RT_FAILURE(rc))
                        LogRel(("RTCrStoreCertAddEncoded: %Rrc\n", rc));
                    if (RT_SUCCESS(rc))
                        break;
                }
                else
                    LogRel(("convertVerifyAndAddPemCertificateToStore: hash mismatch (cbData=%#zx)\n", pCur->cbData));
            }
            else
                LogRel(("convertVerifyAndAddPemCertificateToStore: cbData=%#zx expected %#zx\n",
                        pCur->cbData, pWantedCert->cbEncoded));

        RTCrPemFreeSections(pSectionHead);
    }
    return rc;
}

/* static */
DECLCALLBACK(void) UINetworkReplyPrivateThread::handleProgressChange(RTHTTP hHttp, void *pvUser, uint64_t cbDownloadTotal, uint64_t cbDownloaded)
{
    /* Redirect callback to particular object: */
    Q_UNUSED(hHttp);
    AssertPtrReturnVoid(pvUser);
    static_cast<UINetworkReplyPrivateThread*>(pvUser)->handleProgressChange(cbDownloadTotal, cbDownloaded);
}


#ifndef VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS


/*********************************************************************************************************************************
*   Class UINetworkReplyPrivate implementation.                                                                                  *
*********************************************************************************************************************************/

UINetworkReplyPrivate::UINetworkReplyPrivate(UINetworkRequestType type, const QUrl &url, const UserDictionary &requestHeaders)
    : m_error(UINetworkReply::NoError)
    , m_pThread(0)
{
    /* Prepare full error template: */
    m_strErrorTemplate = tr("%1: %2", "Context description: Error description");

    /* Create and run reply thread: */
    m_pThread = new UINetworkReplyPrivateThread(type, url, requestHeaders);
    connect(m_pThread, &UINetworkReplyPrivateThread::sigDownloadProgress,
            this, &UINetworkReplyPrivate::downloadProgress, Qt::QueuedConnection);
    connect(m_pThread, &UINetworkReplyPrivateThread::finished,
            this, &UINetworkReplyPrivate::sltFinished);
    m_pThread->start();
}

UINetworkReplyPrivate::~UINetworkReplyPrivate()
{
    /* Terminate network-reply thread: */
    m_pThread->abort();
    m_pThread->wait();
    delete m_pThread;
    m_pThread = 0;
}

QString UINetworkReplyPrivate::errorString() const
{
    /* Look for known error codes: */
    switch (m_error)
    {
        case UINetworkReply::NoError:                     break;
        case UINetworkReply::RemoteHostClosedError:       return m_strErrorTemplate.arg(m_pThread->context(), tr("Unable to initialize HTTP library"));
        case UINetworkReply::UrlNotFoundError:            return m_strErrorTemplate.arg(m_pThread->context(), tr("Url not found on the server"));
        case UINetworkReply::HostNotFoundError:           return m_strErrorTemplate.arg(m_pThread->context(), tr("Host not found"));
        case UINetworkReply::ContentAccessDenied:         return m_strErrorTemplate.arg(m_pThread->context(), tr("Content access denied"));
        case UINetworkReply::ProtocolFailure:             return m_strErrorTemplate.arg(m_pThread->context(), tr("Protocol failure"));
        case UINetworkReply::ConnectionRefusedError:      return m_strErrorTemplate.arg(m_pThread->context(), tr("Connection refused"));
        case UINetworkReply::SslHandshakeFailedError:     return m_strErrorTemplate.arg(m_pThread->context(), tr("SSL authentication failed"));
        case UINetworkReply::AuthenticationRequiredError: return m_strErrorTemplate.arg(m_pThread->context(), tr("Wrong SSL certificate format"));
        case UINetworkReply::ContentReSendError:          return m_strErrorTemplate.arg(m_pThread->context(), tr("Content moved"));
        case UINetworkReply::ProxyNotFoundError:          return m_strErrorTemplate.arg(m_pThread->context(), tr("Proxy not found"));
        default:                                          return m_strErrorTemplate.arg(m_pThread->context(), tr("Unknown reason"));
    }
    /* Return null-string by default: */
    return QString();
}

void UINetworkReplyPrivate::sltFinished()
{
    /* Look for known error codes: */
    switch (m_pThread->error())
    {
        case VINF_SUCCESS:                         m_error = UINetworkReply::NoError; break;
        case VERR_HTTP_INIT_FAILED:                m_error = UINetworkReply::RemoteHostClosedError; break;
        case VERR_HTTP_NOT_FOUND:                  m_error = UINetworkReply::UrlNotFoundError; break;
        case VERR_HTTP_HOST_NOT_FOUND:             m_error = UINetworkReply::HostNotFoundError; break;
        case VERR_HTTP_ACCESS_DENIED:              m_error = UINetworkReply::ContentAccessDenied; break;
        case VERR_HTTP_BAD_REQUEST:                m_error = UINetworkReply::ProtocolFailure; break;
        case VERR_HTTP_COULDNT_CONNECT:            m_error = UINetworkReply::ConnectionRefusedError; break;
        case VERR_HTTP_SSL_CONNECT_ERROR:          m_error = UINetworkReply::SslHandshakeFailedError; break;
        case VERR_HTTP_CACERT_WRONG_FORMAT:        m_error = UINetworkReply::AuthenticationRequiredError; break;
        case VERR_HTTP_CACERT_CANNOT_AUTHENTICATE: m_error = UINetworkReply::AuthenticationRequiredError; break;
        case VERR_HTTP_ABORTED:                    m_error = UINetworkReply::OperationCanceledError; break;
        case VERR_HTTP_REDIRECTED:                 m_error = UINetworkReply::ContentReSendError; break;
        case VERR_HTTP_PROXY_NOT_FOUND:            m_error = UINetworkReply::ProxyNotFoundError; break;
        default:                                   m_error = UINetworkReply::UnknownNetworkError; break;
    }
    /* Redirect signal to external listeners: */
    emit finished();
}


/*********************************************************************************************************************************
*   Class UINetworkReply implementation.                                                                                         *
*********************************************************************************************************************************/

UINetworkReply::UINetworkReply(UINetworkRequestType type, const QUrl &url, const UserDictionary &requestHeaders)
    : m_pReply(new UINetworkReplyPrivate(type, url, requestHeaders))
{
    /* Prepare network-reply object connections: */
    connect(m_pReply, &UINetworkReplyPrivate::downloadProgress, this, &UINetworkReply::downloadProgress);
    connect(m_pReply, &UINetworkReplyPrivate::finished,         this, &UINetworkReply::finished);
}

UINetworkReply::~UINetworkReply()
{
    /* Cleanup network-reply object: */
    if (m_pReply)
    {
        delete m_pReply;
        m_pReply = 0;
    }
}

void UINetworkReply::abort()
{
    return m_pReply->abort();
}

QUrl UINetworkReply::url() const
{
    return m_pReply->url();
}

UINetworkReply::NetworkError UINetworkReply::error() const
{
    return m_pReply->error();
}

QString UINetworkReply::errorString() const
{
    return m_pReply->errorString();
}

QByteArray UINetworkReply::readAll() const
{
    return m_pReply->readAll();
}

QVariant UINetworkReply::header(UINetworkReply::KnownHeader header) const
{
    return m_pReply->header(header);
}

#include "UINetworkReply.moc"

#endif /* !VBOX_GUI_IN_TST_SSL_CERT_DOWNLOADS */

