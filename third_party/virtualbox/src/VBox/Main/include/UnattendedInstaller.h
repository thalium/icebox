/* $Id: UnattendedInstaller.h $ */
/** @file
 * UnattendedInstaller class header
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

#ifndef ____H_UNATTENDEDINSTALLER
#define ____H_UNATTENDEDINSTALLER

#include "UnattendedScript.h"

/* Forward declarations */
class Unattended;
class UnattendedInstaller;
class BaseTextScript;


/**
 * The abstract UnattendedInstaller class declaration
 *
 * The class is intended to service a new VM that this VM will be able to
 * execute an unattended installation
 */
class UnattendedInstaller : public RTCNonCopyable
{
/*data*/
protected:
    /** Main unattended installation script. */
    UnattendedScriptTemplate    mMainScript;
    /** Full path to the main template file (set by initInstaller). */
    Utf8Str                     mStrMainScriptTemplate;

    /** Post installation (shell) script. */
    UnattendedScriptTemplate    mPostScript;
    /** Full path to the post template file (set by initInstaller). */
    Utf8Str                     mStrPostScriptTemplate;

    /** Pointer to the parent object.
     * We use this for setting errors and querying attributes. */
    Unattended                 *mpParent;
    /** The path of the extra ISO image we create (set by initInstaller).
     * This is only valid when isAdditionsIsoNeeded() returns true. */
    Utf8Str                     mStrAuxiliaryIsoFilePath;
    /** The path of the extra floppy image we create (set by initInstaller)
     * This is only valid when isAdditionsFloppyNeeded() returns true. */
    Utf8Str                     mStrAuxiliaryFloppyFilePath;
    /** The boot device. */
    DeviceType_T const          meBootDevice;
    /** Default extra install kernel parameters (set by constructor).
     * This can be overridden by the extraInstallKernelParameters attribute of
     * IUnattended. */
    Utf8Str                     mStrDefaultExtraInstallKernelParameters;

private:
    UnattendedInstaller(); /* no default constructors */

public:
    /**
     * Regular constructor.
     *
     * @param   pParent                     The parent object.  Used for setting
     *                                      errors and querying attributes.
     * @param   pszMainScriptTemplateName   The name of the template file (no path)
     *                                      for the main unattended installer
     *                                      script.
     * @param   pszPostScriptTemplateName   The name of the template file (no path)
     *                                      for the post installation script.
     * @param   pszMainScriptFilename       The main unattended installer script
     *                                      filename (on aux media).
     * @param   pszPostScriptFilename       The post installation script filename
     *                                      (on aux media).
     * @param   enmBootDevice               The boot device type.
     */
    UnattendedInstaller(Unattended *pParent,
                        const char *pszMainScriptTemplateName, const char *pszPostScriptTemplateName,
                        const char *pszMainScriptFilename,     const char *pszPostScriptFilename,
                        DeviceType_T enmBootDevice = DeviceType_DVD);
    virtual ~UnattendedInstaller();

    /**
     * Instantiates the appropriate child class.
     *
     * @returns Pointer to the new instance, NULL if no appropriate installer.
     * @param   enmOsType               The guest OS type value.
     * @param   strGuestOsType          The guest OS type string
     * @param   strDetectedOSVersion    The detected guest OS version.
     * @param   strDetectedOSFlavor     The detected guest OS flavor.
     * @param   strDetectedOSHints      Hints about the detected guest OS.
     * @param   pParent                 The parent object.  Used for setting errors
     *                                  and querying attributes.
     * @throws  std::bad_alloc
     */
    static UnattendedInstaller *createInstance(VBOXOSTYPE enmOsType, const Utf8Str &strGuestOsType,
                                               const Utf8Str &strDetectedOSVersion, const Utf8Str &strDetectedOSFlavor,
                                               const Utf8Str &strDetectedOSHints, Unattended *pParent);

    /**
     * Initialize the installer.
     *
     * @note This is called immediately after instantiation and the caller will
     *       always destroy the unattended installer instance on failure, so it
     *       is not necessary to keep track of whether this succeeded or not.
     */
    virtual HRESULT initInstaller();

#if 0 /* These are now in the AUX VISO. */
    /**
     * Whether the VBox guest additions ISO is needed or not.
     *
     * The default implementation always returns false when a VISO is used, see
     * UnattendedInstaller::addFilesToAuxVisoVectors.
     */
    virtual bool isAdditionsIsoNeeded() const;

    /**
     * Whether the VBox validation kit ISO is needed or not.
     *
     * The default implementation always returns false when a VISO is used, see
     * UnattendedInstaller::addFilesToAuxVisoVectors.
     */
    virtual bool isValidationKitIsoNeeded() const;
#endif

    /**
     * Indicates whether an original installation ISO is needed or not.
     */
    virtual bool isOriginalIsoNeeded() const        { return true; }

    /**
     * Indicates whether a floppy image is needed or not.
     */
    virtual bool isAuxiliaryFloppyNeeded() const    { return false; }

    /**
     * Indicates whether an additional or replacement ISO image is needed or not.
     */
    virtual bool isAuxiliaryIsoNeeded() const;

    /**
     * Indicates whether we should boot from the auxiliary ISO image.
     *
     * Will boot from installation ISO if false.
     */
    virtual bool bootFromAuxiliaryIso() const       { return isAuxiliaryIsoNeeded(); }

    /**
     * Indicates whether a the auxiliary ISO is a .viso-file rather than an
     * .iso-file.
     *
     * Different worker methods are used depending on the return value.  A
     * .viso-file is generally only used when the installation media needs to
     * be remastered with small changes and additions.
     */
    virtual bool isAuxiliaryIsoIsVISO() const       { return true; }

    /*
     * Getters
     */
    DeviceType_T   getBootableDeviceType() const        { return meBootDevice; }
    const Utf8Str &getTemplateFilePath() const          { return mStrMainScriptTemplate; }
    const Utf8Str &getPostTemplateFilePath() const      { return mStrPostScriptTemplate; }
    const Utf8Str &getAuxiliaryIsoFilePath() const      { return mStrAuxiliaryIsoFilePath; }
    const Utf8Str &getAuxiliaryFloppyFilePath() const   { return mStrAuxiliaryFloppyFilePath; }
    const Utf8Str &getDefaultExtraInstallKernelParameters() const { return mStrDefaultExtraInstallKernelParameters; }

    /*
     * Setters
     */
    void setTemplatePath(const Utf8Str& data); /**< @todo r=bird: This is confusing as heck. Dir for a while, then it's a file. Not a comment about it. Brilliant. */

    /**
     * Prepares the unattended scripts, does all but write them to the installation
     * media.
     */
    HRESULT prepareUnattendedScripts();

    /**
     * Prepares the media - floppy image, ISO image.
     *
     * This method calls prepareAuxFloppyImage() and prepareAuxIsoImage(), child
     * classes may override these methods or methods they call.
     *
     * @returns COM status code.
     * @param   fOverwrite      Whether to overwrite media files or fail if they
     *                          already exist.
     */
    HRESULT prepareMedia(bool fOverwrite = true);

protected:
    /**
     * Prepares (creates) the auxiliary floppy image.
     *
     * This is called by the base class prepareMedia() when
     * isAuxiliaryFloppyNeeded() is true. The base class implementation puts the
     * edited unattended script onto it.
     */
    HRESULT prepareAuxFloppyImage(bool fOverwrite);

    /**
     * Creates and formats (FAT12) a floppy image, then opens a VFS for it.
     *
     * @returns COM status code.
     * @param   pszFilename     The path to the image file.
     * @param   fOverwrite      Whether to overwrite the file.
     * @param   phVfs           Where to return a writable VFS handle to the newly
     *                          created image.
     */
    HRESULT newAuxFloppyImage(const char *pszFilename, bool fOverwrite, PRTVFS phVfs);

    /**
     * Copies files to the auxiliary floppy image.
     *
     * The base class implementation copies the main and post scripts to the root of
     * the floppy using the default script names.  Child classes may override this
     * to add additional or different files.
     *
     * @returns COM status code.
     * @param   hVfs            The floppy image VFS handle.
     */
    virtual HRESULT copyFilesToAuxFloppyImage(RTVFS hVfs);

    /**
     * Adds the given script to the root of the floppy image under the default
     * script filename.
     *
     * @returns COM status code.
     * @param   pEditor         The script to add.
     * @param   hVfs            The VFS to add it to.
     */
    HRESULT addScriptToFloppyImage(BaseTextScript *pEditor, RTVFS hVfs);

    /**
     * Prepares (creates) the auxiliary ISO image.
     *
     * This is called by the base class prepareMedia() when isAuxiliaryIsoNeeded()
     * is true.  The base class implementation puts the edited unattended script
     * onto it.
     */
    virtual HRESULT prepareAuxIsoImage(bool fOverwrite);

    /**
     * Opens the installation ISO image.
     *
     * @returns COM status code.
     * @param   phVfsIso        Where to return the VFS handle for the ISO.
     * @param   fFlags          RTFSISO9660_F_XXX flags to pass to the
     *                          RTFsIso9660VolOpen API.
     */
    virtual HRESULT openInstallIsoImage(PRTVFS phVfsIso, uint32_t fFlags = 0);

    /**
     * Creates and configures the ISO maker instance.
     *
     * This can be overridden to set configure options.
     *
     * @returns COM status code.
     * @param   phIsoMaker      Where to return the ISO maker.
     */
    virtual HRESULT newAuxIsoImageMaker(PRTFSISOMAKER phIsoMaker);

    /**
     * Adds files to the auxiliary ISO image maker.
     *
     * The base class implementation copies just the mMainScript and mPostScript
     * files to root directory using the default filenames.
     *
     * @returns COM status code.
     * @param   hIsoMaker       The ISO maker handle.
     * @param   hVfsOrgIso      The VFS handle to the original ISO in case files
     *                          needs to be added from it.
     */
    virtual HRESULT addFilesToAuxIsoImageMaker(RTFSISOMAKER hIsoMaker, RTVFS hVfsOrgIso);

    /**
     * Adds the given script to the ISO maker.
     *
     * @returns COM status code.
     * @param   pEditor         The script to add.
     * @param   hIsoMaker       The ISO maker to add it to.
     * @param   pszDstFilename  The file name (w/ path) to add it under.  If NULL,
     *                          the default script filename is used to add it to the
     *                          root.
     */
    HRESULT addScriptToIsoMaker(BaseTextScript *pEditor, RTFSISOMAKER hIsoMaker, const char *pszDstFilename = NULL);

    /**
     * Writes the ISO image to disk.
     *
     * @returns COM status code.
     * @param   hIsoMaker       The ISO maker handle.
     * @param   pszFilename     The filename.
     * @param   fOverwrite      Whether to overwrite the destination file or not.
     */
    HRESULT finalizeAuxIsoImage(RTFSISOMAKER hIsoMaker, const char *pszFilename, bool fOverwrite);

    /**
     * Adds files to the .viso-file vectors.
     *
     * The base class implementation adds the script from mAlg, additions ISO
     * content to '/vboxadditions', and validation kit ISO to '/vboxvalidationkit'.
     *
     * @returns COM status code.
     * @param   rVecArgs        The ISO maker argument list that will be turned into
     *                          a .viso-file.
     * @param   rVecFiles       The list of files we've created.  This is for
     *                          cleaning up at the end.
     * @param   hVfsOrgIso      The VFS handle to the original ISO in case files
     *                          needs to be added from it.
     * @param   fOverwrite      Whether to overwrite files or not.
     */
    virtual HRESULT addFilesToAuxVisoVectors(RTCList<RTCString> &rVecArgs, RTCList<RTCString> &rVecFiles,
                                             RTVFS hVfsOrgIso, bool fOverwrite);

    /**
     * Saves the given script to disk and adds it to the .viso-file vectors.
     *
     * @returns COM status code.
     * @param   pEditor         The script to add.
     * @param   rVecArgs        The ISO maker argument list that will be turned into
     *                          a .viso-file.
     * @param   rVecFiles       The list of files we've created.  This is for
     *                          cleaning up at the end.
     * @param   fOverwrite      Whether to overwrite files or not.
     */
    HRESULT addScriptToVisoVectors(BaseTextScript *pEditor, RTCList<RTCString> &rVecArgs,
                                   RTCList<RTCString> &rVecFiles, bool fOverwrite);

    /**
     * Writes out the .viso-file to disk.
     *
     * @returns COM status code.
     * @param   rVecArgs        The ISO maker argument list to write out.
     * @param   pszFilename     The filename.
     * @param   fOverwrite      Whether to overwrite the destination file or not.
     */
    HRESULT finalizeAuxVisoFile(RTCList<RTCString> const &rVecArgs, const char *pszFilename, bool fOverwrite);

    /**
     * Loads @a pszFilename from @a hVfsOrgIso into @a pEditor and parses it.
     *
     * @returns COM status code.
     * @param   hVfsOrgIso      The handle to the original installation ISO.
     * @param   pszFilename     The filename to open and load from the ISO.
     * @param   pEditor         The editor instance to load the file into and
     *                          do the parseing with.
     */
    HRESULT loadAndParseFileFromIso(RTVFS hVfsOrgIso, const char *pszFilename, AbstractScript *pEditor);
};


/**
 * Windows installer, for versions up to xp 64 / w2k3.
 */
class UnattendedWindowsSifInstaller : public UnattendedInstaller
{
public:
    UnattendedWindowsSifInstaller(Unattended *pParent)
        : UnattendedInstaller(pParent,
                              "win_nt5_unattended.sif", "win_postinstall.cmd",
                              "WINNT.SIF",              "VBOXPOST.CMD")
    { Assert(isOriginalIsoNeeded()); Assert(isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO()); Assert(!bootFromAuxiliaryIso()); }
    ~UnattendedWindowsSifInstaller()        {}

    bool isAuxiliaryFloppyNeeded() const    { return true; }
    bool bootFromAuxiliaryIso() const       { return false; }

};

/**
 * Windows installer, for versions starting with Vista.
 */
class UnattendedWindowsXmlInstaller : public UnattendedInstaller
{
public:
    UnattendedWindowsXmlInstaller(Unattended *pParent)
        : UnattendedInstaller(pParent,
                              "win_nt6_unattended.xml", "win_postinstall.cmd",
                              "autounattend.xml",       "VBOXPOST.CMD")
    { Assert(isOriginalIsoNeeded()); Assert(isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO()); Assert(!bootFromAuxiliaryIso()); }
    ~UnattendedWindowsXmlInstaller()      {}

    bool isAuxiliaryFloppyNeeded() const    { return true; }
    bool bootFromAuxiliaryIso() const       { return false; }
};


/**
 * Base class for the unattended linux installers.
 */
class UnattendedLinuxInstaller : public UnattendedInstaller
{
protected:
    /** Array of linux parameter patterns that should be removed by editIsoLinuxCfg.
     * The patterns are proceed by RTStrSimplePatternNMatch. */
    RTCList<RTCString, RTCString *> mArrStrRemoveInstallKernelParameters;

public:
    UnattendedLinuxInstaller(Unattended *pParent,
                             const char *pszMainScriptTemplateName, const char *pszPostScriptTemplateName,
                             const char *pszMainScriptFilename,     const char *pszPostScriptFilename = "vboxpostinstall.sh")
        : UnattendedInstaller(pParent,
                              pszMainScriptTemplateName, pszPostScriptTemplateName,
                              pszMainScriptFilename,     pszPostScriptFilename) {}
    ~UnattendedLinuxInstaller() {}

    bool isAuxiliaryIsoNeeded() const       { return true; }

protected:
    /**
     * Performs basic edits on a isolinux.cfg file.
     *
     * @returns COM status code
     * @param   pEditor         Editor with the isolinux.cfg file loaded and parsed.
     */
    virtual HRESULT editIsoLinuxCfg(GeneralTextScript *pEditor);
};


/**
 * Debian installer.
 *
 * This will remaster the orignal ISO and therefore be producing a .viso-file.
 */
class UnattendedDebianInstaller : public UnattendedLinuxInstaller
{
public:
    UnattendedDebianInstaller(Unattended *pParent,
                              const char *pszMainScriptTemplateName = "debian_preseed.cfg",
                              const char *pszPostScriptTemplateName = "debian_postinstall.sh",
                              const char *pszMainScriptFilename     = "preseed.cfg")
        : UnattendedLinuxInstaller(pParent, pszMainScriptTemplateName, pszPostScriptTemplateName, pszMainScriptFilename)
    {
        Assert(!isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded());
        Assert(!isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO());
        mStrDefaultExtraInstallKernelParameters.setNull();
        mStrDefaultExtraInstallKernelParameters += " auto=true";
        mStrDefaultExtraInstallKernelParameters.append(" preseed/file=/cdrom/").append(pszMainScriptFilename);
        mStrDefaultExtraInstallKernelParameters += " priority=critical";
        mStrDefaultExtraInstallKernelParameters += " quiet";
        mStrDefaultExtraInstallKernelParameters += " splash";
        mStrDefaultExtraInstallKernelParameters += " noprompt";  /* no questions about things like CD/DVD ejections */
        mStrDefaultExtraInstallKernelParameters += " noshell";   /* No shells on VT1-3 (debian, not ubuntu). */
        mStrDefaultExtraInstallKernelParameters += " automatic-ubiquity";   // ubiquity
        // the following can probably go into the preseed.cfg:
        mStrDefaultExtraInstallKernelParameters.append(" debian-installer/locale=").append(pParent->i_getLocale());
        mStrDefaultExtraInstallKernelParameters += " keyboard-configuration/layoutcode=us";
        mStrDefaultExtraInstallKernelParameters += " languagechooser/language-name=English"; /** @todo fixme */
        mStrDefaultExtraInstallKernelParameters.append(" localechooser/supported-locales=").append(pParent->i_getLocale()).append(".UTF-8");
        mStrDefaultExtraInstallKernelParameters.append(" countrychooser/shortlist=").append(pParent->i_getCountry()); // ubiquity?
        mStrDefaultExtraInstallKernelParameters += " --";
    }
    ~UnattendedDebianInstaller()            {}

    bool isOriginalIsoNeeded() const        { return false; }

protected:
    HRESULT addFilesToAuxVisoVectors(RTCList<RTCString> &rVecArgs, RTCList<RTCString> &rVecFiles,
                                     RTVFS hVfsOrgIso, bool fOverwrite);
    HRESULT editDebianTxtCfg(GeneralTextScript *pEditor);

};


/**
 * Ubuntu installer (same as debian, except for the template).
 */
class UnattendedUbuntuInstaller : public UnattendedDebianInstaller
{
public:
    UnattendedUbuntuInstaller(Unattended *pParent)
        : UnattendedDebianInstaller(pParent, "ubuntu_preseed.cfg")
    { Assert(!isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded()); Assert(!isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO()); }
    ~UnattendedUbuntuInstaller() {}
};


/**
 * RHEL 6 and 7 installer.
 *
 * This serves as a base for the kickstart based installers.
 */
class UnattendedRhel6And7Installer : public UnattendedLinuxInstaller
{
public:
    UnattendedRhel6And7Installer(Unattended *pParent,
                                 const char *pszMainScriptTemplateName = "redhat67_ks.cfg",
                                 const char *pszPostScriptTemplateName = "redhat_postinstall.sh",
                                 const char *pszMainScriptFilename     = "ks.cfg")
          : UnattendedLinuxInstaller(pParent, pszMainScriptTemplateName, pszPostScriptTemplateName, pszMainScriptFilename)
    {
        Assert(!isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded());
        Assert(!isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO());
        mStrDefaultExtraInstallKernelParameters.assign(" ks=cdrom:/").append(pszMainScriptFilename).append(' ');
        mArrStrRemoveInstallKernelParameters.append("rd.live.check"); /* Disables the checkisomd5 step. Required for VISO. */
    }
    ~UnattendedRhel6And7Installer() {}

    bool isAuxiliaryIsoIsVISO()             { return true; }
    bool isOriginalIsoNeeded() const        { return false; }

protected:
    HRESULT addFilesToAuxVisoVectors(RTCList<RTCString> &rVecArgs, RTCList<RTCString> &rVecFiles,
                                     RTVFS hVfsOrgIso, bool fOverwrite);
};


/**
 * RHEL 5 installer (same as RHEL 6 & 7, except for the kickstart template).
 */
class UnattendedRhel5Installer : public UnattendedRhel6And7Installer
{
public:
    UnattendedRhel5Installer(Unattended *pParent) : UnattendedRhel6And7Installer(pParent, "rhel5_ks.cfg") {}
    ~UnattendedRhel5Installer() {}
};


/**
 * RHEL 4 installer (same as RHEL 6 & 7, except for the kickstart template).
 */
class UnattendedRhel4Installer : public UnattendedRhel6And7Installer
{
public:
    UnattendedRhel4Installer(Unattended *pParent) : UnattendedRhel6And7Installer(pParent, "rhel4_ks.cfg") {}
    ~UnattendedRhel4Installer() {}
};


/**
 * RHEL 3 installer (same as RHEL 6 & 7, except for the kickstart template).
 */
class UnattendedRhel3Installer : public UnattendedRhel6And7Installer
{
public:
    UnattendedRhel3Installer(Unattended *pParent) : UnattendedRhel6And7Installer(pParent, "rhel3_ks.cfg") {}
    ~UnattendedRhel3Installer() {}
};


/**
 * Fedora installer (same as RHEL 6 & 7, except for the template).
 */
class UnattendedFedoraInstaller : public UnattendedRhel6And7Installer
{
public:
    UnattendedFedoraInstaller(Unattended *pParent)
        : UnattendedRhel6And7Installer(pParent, "fedora_ks.cfg")
    { Assert(!isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded()); Assert(!isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO()); }
    ~UnattendedFedoraInstaller() {}
};


/**
 * Oracle Linux installer (same as RHEL 6 & 7, except for the template).
 */
class UnattendedOracleLinuxInstaller : public UnattendedRhel6And7Installer
{
public:
    UnattendedOracleLinuxInstaller(Unattended *pParent)
        : UnattendedRhel6And7Installer(pParent, "ol_ks.cfg")
    { Assert(!isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded()); Assert(!isAuxiliaryFloppyNeeded()); Assert(isAuxiliaryIsoIsVISO()); }
    ~UnattendedOracleLinuxInstaller() {}
};


#if 0 /* fixme */
/**
 * SUSE linux installer.
 *
 * @todo needs fixing.
 */
class UnattendedSuseInstaller : public UnattendedLinuxInstaller
{
public:
    UnattendedSuseInstaller(BaseTextScript *pAlg, Unattended *pParent)
        : UnattendedLinuxInstaller(pAlg, pParent, "suse_autoinstall.xml")
    { Assert(isOriginalIsoNeeded()); Assert(isAuxiliaryIsoNeeded()); Assert(!isAuxiliaryFloppyNeeded()); Assert(!isAuxiliaryIsoIsVISO()); }
    ~UnattendedSuseInstaller() {}

    HRESULT setupScriptOnAuxiliaryCD(const Utf8Str &path);
};
#endif

#endif // !____H_UNATTENDEDINSTALLER

