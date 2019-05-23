/* $Id: QMTranslator.h $ */
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

#ifndef ____H_QMTRANSLATOR
#define ____H_QMTRANSLATOR

class QMTranslator_Impl;

class QMTranslator
{
public:
    QMTranslator();
    virtual ~QMTranslator();

    /* Gets translation from loaded QM file
     *
     * @param   context   QM context to look for translation
     * @param   source    Source string in one-byte encoding
     * @param   disamb    Disambiguationg comment, empty by default
     *
     * @returns Pointer to a translation in UTF-8 encoding, empty string on failure */

    const char *translate(const char *pszContext, const char *pszSource, const char *pszDisamb = "") const throw();

    /* Loads and parses QM file
     *
     * @param       filename    The name of the file to load
     *
     * @returns VINF_SUCCESS if successful */
    int load(const char *pszFilename) throw();

private:
        /* QMTranslator implementation.
         * To separate all the code from the interface */
    QMTranslator_Impl *_impl;

    /* If copying is required, please define the following operators */
    void operator=(QMTranslator &);
    QMTranslator(const QMTranslator &);
};

#endif /* !____H_QMTRANSLATOR */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
