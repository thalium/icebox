/* $Id: ApplianceImplImport.cpp $ */
/** @file
 * IAppliance and IVirtualSystem COM class implementations.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/alloca.h>
#include <iprt/path.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/s3.h>
#include <iprt/sha.h>
#include <iprt/manifest.h>
#include <iprt/tar.h>
#include <iprt/zip.h>
#include <iprt/stream.h>
#include <iprt/crypto/digest.h>
#include <iprt/crypto/pkix.h>
#include <iprt/crypto/store.h>
#include <iprt/crypto/x509.h>

#include <VBox/vd.h>
#include <VBox/com/array.h>

#include "ApplianceImpl.h"
#include "VirtualBoxImpl.h"
#include "GuestOSTypeImpl.h"
#include "ProgressImpl.h"
#include "MachineImpl.h"
#include "MediumImpl.h"
#include "MediumFormatImpl.h"
#include "SystemPropertiesImpl.h"
#include "HostImpl.h"

#include "AutoCaller.h"
#include "Logging.h"

#include "ApplianceImplPrivate.h"
#include "CertificateImpl.h"

#include <VBox/param.h>
#include <VBox/version.h>
#include <VBox/settings.h>

#include <set>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// IAppliance public methods
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Public method implementation. This opens the OVF with ovfreader.cpp.
 * Thread implementation is in Appliance::readImpl().
 *
 * @param aFile     File to read the appliance from.
 * @param aProgress Progress object.
 * @return
 */
HRESULT Appliance::read(const com::Utf8Str &aFile,
                        ComPtr<IProgress> &aProgress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!i_isApplianceIdle())
        return E_ACCESSDENIED;

    if (m->pReader)
    {
        delete m->pReader;
        m->pReader = NULL;
    }

    // see if we can handle this file; for now we insist it has an ovf/ova extension
    if (   !aFile.endsWith(".ovf", Utf8Str::CaseInsensitive)
        && !aFile.endsWith(".ova", Utf8Str::CaseInsensitive))
        return setError(VBOX_E_FILE_ERROR, tr("Appliance file must have .ovf or .ova extension"));

    ComObjPtr<Progress> progress;
    try
    {
        /* Parse all necessary info out of the URI */
        i_parseURI(aFile, m->locInfo);
        i_readImpl(m->locInfo, progress);
    }
    catch (HRESULT aRC)
    {
        return aRC;
    }

    /* Return progress to the caller */
    progress.queryInterfaceTo(aProgress.asOutParam());
    return S_OK;
}

/**
 * Public method implementation. This looks at the output of ovfreader.cpp and creates
 * VirtualSystemDescription instances.
 * @return
 */
HRESULT Appliance::interpret()
{
    /// @todo
    //  - don't use COM methods but the methods directly (faster, but needs appropriate
    // locking of that objects itself (s. HardDisk))
    //  - Appropriate handle errors like not supported file formats
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!i_isApplianceIdle())
        return E_ACCESSDENIED;

    HRESULT rc = S_OK;

    /* Clear any previous virtual system descriptions */
    m->virtualSystemDescriptions.clear();

    if (!m->pReader)
        return setError(E_FAIL,
                        tr("Cannot interpret appliance without reading it first (call read() before interpret())"));

    // Change the appliance state so we can safely leave the lock while doing time-consuming
    // disk imports; also the below method calls do all kinds of locking which conflicts with
    // the appliance object lock
    m->state = Data::ApplianceImporting;
    alock.release();

    /* Try/catch so we can clean up on error */
    try
    {
        list<ovf::VirtualSystem>::const_iterator it;
        /* Iterate through all virtual systems */
        for (it = m->pReader->m_llVirtualSystems.begin();
             it != m->pReader->m_llVirtualSystems.end();
             ++it)
        {
            const ovf::VirtualSystem &vsysThis = *it;

            ComObjPtr<VirtualSystemDescription> pNewDesc;
            rc = pNewDesc.createObject();
            if (FAILED(rc)) throw rc;
            rc = pNewDesc->init();
            if (FAILED(rc)) throw rc;

            // if the virtual system in OVF had a <vbox:Machine> element, have the
            // VirtualBox settings code parse that XML now
            if (vsysThis.pelmVBoxMachine)
                pNewDesc->i_importVBoxMachineXML(*vsysThis.pelmVBoxMachine);

            // Guest OS type
            // This is taken from one of three places, in this order:
            Utf8Str strOsTypeVBox;
            Utf8StrFmt strCIMOSType("%RU32", (uint32_t)vsysThis.cimos);
            // 1) If there is a <vbox:Machine>, then use the type from there.
            if (   vsysThis.pelmVBoxMachine
                && pNewDesc->m->pConfig->machineUserData.strOsType.isNotEmpty()
               )
                strOsTypeVBox = pNewDesc->m->pConfig->machineUserData.strOsType;
            // 2) Otherwise, if there is OperatingSystemSection/vbox:OSType, use that one.
            else if (vsysThis.strTypeVBox.isNotEmpty())      // OVFReader has found vbox:OSType
                strOsTypeVBox = vsysThis.strTypeVBox;
            // 3) Otherwise, make a best guess what the vbox type is from the OVF (CIM) OS type.
            else
                convertCIMOSType2VBoxOSType(strOsTypeVBox, vsysThis.cimos, vsysThis.strCimosDesc);
            pNewDesc->i_addEntry(VirtualSystemDescriptionType_OS,
                                 "",
                                 strCIMOSType,
                                 strOsTypeVBox);

            /* VM name */
            Utf8Str nameVBox;
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (   vsysThis.pelmVBoxMachine
                && pNewDesc->m->pConfig->machineUserData.strName.isNotEmpty())
                nameVBox = pNewDesc->m->pConfig->machineUserData.strName;
            else
                nameVBox = vsysThis.strName;
            /* If there isn't any name specified create a default one out
             * of the OS type */
            if (nameVBox.isEmpty())
                nameVBox = strOsTypeVBox;
            i_searchUniqueVMName(nameVBox);
            pNewDesc->i_addEntry(VirtualSystemDescriptionType_Name,
                                 "",
                                 vsysThis.strName,
                                 nameVBox);

            /* Based on the VM name, create a target machine path. */
            Bstr bstrMachineFilename;
            rc = mVirtualBox->ComposeMachineFilename(Bstr(nameVBox).raw(),
                                                     NULL /* aGroup */,
                                                     NULL /* aCreateFlags */,
                                                     NULL /* aBaseFolder */,
                                                     bstrMachineFilename.asOutParam());
            if (FAILED(rc)) throw rc;
            /* Determine the machine folder from that */
            Utf8Str strMachineFolder = Utf8Str(bstrMachineFilename).stripFilename();

            /* VM Product */
            if (!vsysThis.strProduct.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_Product,
                                      "",
                                      vsysThis.strProduct,
                                      vsysThis.strProduct);

            /* VM Vendor */
            if (!vsysThis.strVendor.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_Vendor,
                                     "",
                                     vsysThis.strVendor,
                                     vsysThis.strVendor);

            /* VM Version */
            if (!vsysThis.strVersion.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_Version,
                                     "",
                                     vsysThis.strVersion,
                                     vsysThis.strVersion);

            /* VM ProductUrl */
            if (!vsysThis.strProductUrl.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_ProductUrl,
                                     "",
                                     vsysThis.strProductUrl,
                                     vsysThis.strProductUrl);

            /* VM VendorUrl */
            if (!vsysThis.strVendorUrl.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_VendorUrl,
                                     "",
                                     vsysThis.strVendorUrl,
                                     vsysThis.strVendorUrl);

            /* VM description */
            if (!vsysThis.strDescription.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_Description,
                                     "",
                                     vsysThis.strDescription,
                                     vsysThis.strDescription);

            /* VM license */
            if (!vsysThis.strLicenseText.isEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_License,
                                     "",
                                     vsysThis.strLicenseText,
                                     vsysThis.strLicenseText);

            /* Now that we know the OS type, get our internal defaults based on that. */
            ComPtr<IGuestOSType> pGuestOSType;
            rc = mVirtualBox->GetGuestOSType(Bstr(strOsTypeVBox).raw(), pGuestOSType.asOutParam());
            if (FAILED(rc)) throw rc;

            /* CPU count */
            ULONG cpuCountVBox;
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (   vsysThis.pelmVBoxMachine
                && pNewDesc->m->pConfig->hardwareMachine.cCPUs)
                cpuCountVBox = pNewDesc->m->pConfig->hardwareMachine.cCPUs;
            else
                cpuCountVBox = vsysThis.cCPUs;
            /* Check for the constraints */
            if (cpuCountVBox > SchemaDefs::MaxCPUCount)
            {
                i_addWarning(tr("The virtual system \"%s\" claims support for %u CPU's, but VirtualBox has support for "
                                "max %u CPU's only."),
                                vsysThis.strName.c_str(), cpuCountVBox, SchemaDefs::MaxCPUCount);
                cpuCountVBox = SchemaDefs::MaxCPUCount;
            }
            if (vsysThis.cCPUs == 0)
                cpuCountVBox = 1;
            pNewDesc->i_addEntry(VirtualSystemDescriptionType_CPU,
                                "",
                                Utf8StrFmt("%RU32", (uint32_t)vsysThis.cCPUs),
                                Utf8StrFmt("%RU32", (uint32_t)cpuCountVBox));

            /* RAM */
            uint64_t ullMemSizeVBox;
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (   vsysThis.pelmVBoxMachine
                && pNewDesc->m->pConfig->hardwareMachine.ulMemorySizeMB)
                ullMemSizeVBox = pNewDesc->m->pConfig->hardwareMachine.ulMemorySizeMB;
            else
                ullMemSizeVBox = vsysThis.ullMemorySize / _1M;
            /* Check for the constraints */
            if (    ullMemSizeVBox != 0
                 && (    ullMemSizeVBox < MM_RAM_MIN_IN_MB
                      || ullMemSizeVBox > MM_RAM_MAX_IN_MB
                    )
               )
            {
                i_addWarning(tr("The virtual system \"%s\" claims support for %llu MB RAM size, but VirtualBox has "
                                "support for min %u & max %u MB RAM size only."),
                                vsysThis.strName.c_str(), ullMemSizeVBox, MM_RAM_MIN_IN_MB, MM_RAM_MAX_IN_MB);
                ullMemSizeVBox = RT_MIN(RT_MAX(ullMemSizeVBox, MM_RAM_MIN_IN_MB), MM_RAM_MAX_IN_MB);
            }
            if (vsysThis.ullMemorySize == 0)
            {
                /* If the RAM of the OVF is zero, use our predefined values */
                ULONG memSizeVBox2;
                rc = pGuestOSType->COMGETTER(RecommendedRAM)(&memSizeVBox2);
                if (FAILED(rc)) throw rc;
                /* VBox stores that in MByte */
                ullMemSizeVBox = (uint64_t)memSizeVBox2;
            }
            pNewDesc->i_addEntry(VirtualSystemDescriptionType_Memory,
                                 "",
                                 Utf8StrFmt("%RU64", (uint64_t)vsysThis.ullMemorySize),
                                 Utf8StrFmt("%RU64", (uint64_t)ullMemSizeVBox));

            /* Audio */
            Utf8Str strSoundCard;
            Utf8Str strSoundCardOrig;
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (   vsysThis.pelmVBoxMachine
                && pNewDesc->m->pConfig->hardwareMachine.audioAdapter.fEnabled)
            {
                strSoundCard = Utf8StrFmt("%RU32",
                                          (uint32_t)pNewDesc->m->pConfig->hardwareMachine.audioAdapter.controllerType);
            }
            else if (vsysThis.strSoundCardType.isNotEmpty())
            {
                /* Set the AC97 always for the simple OVF case.
                 * @todo: figure out the hardware which could be possible */
                strSoundCard = Utf8StrFmt("%RU32", (uint32_t)AudioControllerType_AC97);
                strSoundCardOrig = vsysThis.strSoundCardType;
            }
            if (strSoundCard.isNotEmpty())
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_SoundCard,
                                     "",
                                     strSoundCardOrig,
                                     strSoundCard);

#ifdef VBOX_WITH_USB
            /* USB Controller */
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (   (   vsysThis.pelmVBoxMachine
                    && pNewDesc->m->pConfig->hardwareMachine.usbSettings.llUSBControllers.size() > 0)
                || vsysThis.fHasUsbController)
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_USBController, "", "", "");
#endif /* VBOX_WITH_USB */

            /* Network Controller */
            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            if (vsysThis.pelmVBoxMachine)
            {
                uint32_t maxNetworkAdapters = Global::getMaxNetworkAdapters(pNewDesc->m->pConfig->hardwareMachine.chipsetType);

                const settings::NetworkAdaptersList &llNetworkAdapters = pNewDesc->m->pConfig->hardwareMachine.llNetworkAdapters;
                /* Check for the constrains */
                if (llNetworkAdapters.size() > maxNetworkAdapters)
                    i_addWarning(tr("The virtual system \"%s\" claims support for %zu network adapters, but VirtualBox "
                                    "has support for max %u network adapter only."),
                                    vsysThis.strName.c_str(), llNetworkAdapters.size(), maxNetworkAdapters);
                /* Iterate through all network adapters. */
                settings::NetworkAdaptersList::const_iterator it1;
                size_t a = 0;
                for (it1 = llNetworkAdapters.begin();
                     it1 != llNetworkAdapters.end() && a < maxNetworkAdapters;
                     ++it1, ++a)
                {
                    if (it1->fEnabled)
                    {
                        Utf8Str strMode = convertNetworkAttachmentTypeToString(it1->mode);
                        pNewDesc->i_addEntry(VirtualSystemDescriptionType_NetworkAdapter,
                                             "", // ref
                                             strMode, // orig
                                             Utf8StrFmt("%RU32", (uint32_t)it1->type), // conf
                                             0,
                                             Utf8StrFmt("slot=%RU32;type=%s", it1->ulSlot, strMode.c_str())); // extra conf
                    }
                }
            }
            /* else we use the ovf configuration. */
            else if (vsysThis.llEthernetAdapters.size() >  0)
            {
                size_t cEthernetAdapters = vsysThis.llEthernetAdapters.size();
                uint32_t maxNetworkAdapters = Global::getMaxNetworkAdapters(ChipsetType_PIIX3);

                /* Check for the constrains */
                if (cEthernetAdapters > maxNetworkAdapters)
                    i_addWarning(tr("The virtual system \"%s\" claims support for %zu network adapters, but VirtualBox "
                                    "has support for max %u network adapter only."),
                                    vsysThis.strName.c_str(), cEthernetAdapters, maxNetworkAdapters);

                /* Get the default network adapter type for the selected guest OS */
                NetworkAdapterType_T defaultAdapterVBox = NetworkAdapterType_Am79C970A;
                rc = pGuestOSType->COMGETTER(AdapterType)(&defaultAdapterVBox);
                if (FAILED(rc)) throw rc;

                ovf::EthernetAdaptersList::const_iterator itEA;
                /* Iterate through all abstract networks. Ignore network cards
                 * which exceed the limit of VirtualBox. */
                size_t a = 0;
                for (itEA = vsysThis.llEthernetAdapters.begin();
                     itEA != vsysThis.llEthernetAdapters.end() && a < maxNetworkAdapters;
                     ++itEA, ++a)
                {
                    const ovf::EthernetAdapter &ea = *itEA; // logical network to connect to
                    Utf8Str strNetwork = ea.strNetworkName;
                    // make sure it's one of these two
                    if (    (strNetwork.compare("Null", Utf8Str::CaseInsensitive))
                         && (strNetwork.compare("NAT", Utf8Str::CaseInsensitive))
                         && (strNetwork.compare("Bridged", Utf8Str::CaseInsensitive))
                         && (strNetwork.compare("Internal", Utf8Str::CaseInsensitive))
                         && (strNetwork.compare("HostOnly", Utf8Str::CaseInsensitive))
                         && (strNetwork.compare("Generic", Utf8Str::CaseInsensitive))
                       )
                        strNetwork = "Bridged";     // VMware assumes this is the default apparently

                    /* Figure out the hardware type */
                    NetworkAdapterType_T nwAdapterVBox = defaultAdapterVBox;
                    if (!ea.strAdapterType.compare("PCNet32", Utf8Str::CaseInsensitive))
                    {
                        /* If the default adapter is already one of the two
                         * PCNet adapters use the default one. If not use the
                         * Am79C970A as fallback. */
                        if (!(defaultAdapterVBox == NetworkAdapterType_Am79C970A ||
                              defaultAdapterVBox == NetworkAdapterType_Am79C973))
                            nwAdapterVBox = NetworkAdapterType_Am79C970A;
                    }
#ifdef VBOX_WITH_E1000
                    /* VMWare accidentally write this with VirtualCenter 3.5,
                       so make sure in this case always to use the VMWare one */
                    else if (!ea.strAdapterType.compare("E10000", Utf8Str::CaseInsensitive))
                        nwAdapterVBox = NetworkAdapterType_I82545EM;
                    else if (!ea.strAdapterType.compare("E1000", Utf8Str::CaseInsensitive))
                    {
                        /* Check if this OVF was written by VirtualBox */
                        if (Utf8Str(vsysThis.strVirtualSystemType).contains("virtualbox", Utf8Str::CaseInsensitive))
                        {
                            /* If the default adapter is already one of the three
                             * E1000 adapters use the default one. If not use the
                             * I82545EM as fallback. */
                            if (!(defaultAdapterVBox == NetworkAdapterType_I82540EM ||
                                  defaultAdapterVBox == NetworkAdapterType_I82543GC ||
                                  defaultAdapterVBox == NetworkAdapterType_I82545EM))
                            nwAdapterVBox = NetworkAdapterType_I82540EM;
                        }
                        else
                            /* Always use this one since it's what VMware uses */
                            nwAdapterVBox = NetworkAdapterType_I82545EM;
                    }
#endif /* VBOX_WITH_E1000 */

                    pNewDesc->i_addEntry(VirtualSystemDescriptionType_NetworkAdapter,
                                         "",      // ref
                                         ea.strNetworkName,      // orig
                                         Utf8StrFmt("%RU32", (uint32_t)nwAdapterVBox),   // conf
                                         0,
                                         Utf8StrFmt("type=%s", strNetwork.c_str()));       // extra conf
                }
            }

            /* If there is a <vbox:Machine>, we always prefer the setting from there. */
            bool fFloppy = false;
            bool fDVD = false;
            if (vsysThis.pelmVBoxMachine)
            {
                settings::StorageControllersList &llControllers = pNewDesc->m->pConfig->hardwareMachine.storage.llStorageControllers;
                settings::StorageControllersList::iterator it3;
                for (it3 = llControllers.begin();
                     it3 != llControllers.end();
                     ++it3)
                {
                    settings::AttachedDevicesList &llAttachments = it3->llAttachedDevices;
                    settings::AttachedDevicesList::iterator it4;
                    for (it4 = llAttachments.begin();
                         it4 != llAttachments.end();
                         ++it4)
                    {
                        fDVD |= it4->deviceType == DeviceType_DVD;
                        fFloppy |= it4->deviceType == DeviceType_Floppy;
                        if (fFloppy && fDVD)
                            break;
                    }
                    if (fFloppy && fDVD)
                        break;
                }
            }
            else
            {
                fFloppy = vsysThis.fHasFloppyDrive;
                fDVD = vsysThis.fHasCdromDrive;
            }
            /* Floppy Drive */
            if (fFloppy)
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_Floppy, "", "", "");
            /* CD Drive */
            if (fDVD)
                pNewDesc->i_addEntry(VirtualSystemDescriptionType_CDROM, "", "", "");

            /* Hard disk Controller */
            uint16_t cIDEused = 0;
            uint16_t cSATAused = 0; NOREF(cSATAused);
            uint16_t cSCSIused = 0; NOREF(cSCSIused);
            ovf::ControllersMap::const_iterator hdcIt;
            /* Iterate through all hard disk controllers */
            for (hdcIt = vsysThis.mapControllers.begin();
                 hdcIt != vsysThis.mapControllers.end();
                 ++hdcIt)
            {
                const ovf::HardDiskController &hdc = hdcIt->second;
                Utf8Str strControllerID = Utf8StrFmt("%RI32", (uint32_t)hdc.idController);

                switch (hdc.system)
                {
                    case ovf::HardDiskController::IDE:
                        /* Check for the constrains */
                        if (cIDEused < 4)
                        {
                            /// @todo figure out the IDE types
                            /* Use PIIX4 as default */
                            Utf8Str strType = "PIIX4";
                            if (!hdc.strControllerType.compare("PIIX3", Utf8Str::CaseInsensitive))
                                strType = "PIIX3";
                            else if (!hdc.strControllerType.compare("ICH6", Utf8Str::CaseInsensitive))
                                strType = "ICH6";
                            pNewDesc->i_addEntry(VirtualSystemDescriptionType_HardDiskControllerIDE,
                                                 strControllerID,         // strRef
                                                 hdc.strControllerType,   // aOvfValue
                                                 strType);                // aVBoxValue
                        }
                        else
                            /* Warn only once */
                            if (cIDEused == 2)
                                i_addWarning(tr("The virtual \"%s\" system requests support for more than two "
                                              "IDE controller channels, but VirtualBox supports only two."),
                                              vsysThis.strName.c_str());

                        ++cIDEused;
                    break;

                    case ovf::HardDiskController::SATA:
                        /* Check for the constrains */
                        if (cSATAused < 1)
                        {
                            /// @todo figure out the SATA types
                            /* We only support a plain AHCI controller, so use them always */
                            pNewDesc->i_addEntry(VirtualSystemDescriptionType_HardDiskControllerSATA,
                                                 strControllerID,
                                                 hdc.strControllerType,
                                                 "AHCI");
                        }
                        else
                        {
                            /* Warn only once */
                            if (cSATAused == 1)
                                i_addWarning(tr("The virtual system \"%s\" requests support for more than one "
                                                "SATA controller, but VirtualBox has support for only one"),
                                                vsysThis.strName.c_str());

                        }
                        ++cSATAused;
                    break;

                    case ovf::HardDiskController::SCSI:
                        /* Check for the constrains */
                        if (cSCSIused < 1)
                        {
                            VirtualSystemDescriptionType_T vsdet = VirtualSystemDescriptionType_HardDiskControllerSCSI;
                            Utf8Str hdcController = "LsiLogic";
                            if (!hdc.strControllerType.compare("lsilogicsas", Utf8Str::CaseInsensitive))
                            {
                                // OVF considers SAS a variant of SCSI but VirtualBox considers it a class of its own
                                vsdet = VirtualSystemDescriptionType_HardDiskControllerSAS;
                                hdcController = "LsiLogicSas";
                            }
                            else if (!hdc.strControllerType.compare("BusLogic", Utf8Str::CaseInsensitive))
                                hdcController = "BusLogic";
                            pNewDesc->i_addEntry(vsdet,
                                                 strControllerID,
                                                 hdc.strControllerType,
                                                 hdcController);
                        }
                        else
                            i_addWarning(tr("The virtual system \"%s\" requests support for an additional "
                                            "SCSI controller of type \"%s\" with ID %s, but VirtualBox presently "
                                            "supports only one SCSI controller."),
                                            vsysThis.strName.c_str(),
                                            hdc.strControllerType.c_str(),
                                            strControllerID.c_str());
                        ++cSCSIused;
                    break;
                }
            }

            /* Hard disks */
            if (vsysThis.mapVirtualDisks.size() > 0)
            {
                ovf::VirtualDisksMap::const_iterator itVD;
                /* Iterate through all hard disks ()*/
                for (itVD = vsysThis.mapVirtualDisks.begin();
                     itVD != vsysThis.mapVirtualDisks.end();
                     ++itVD)
                {
                    const ovf::VirtualDisk &hd = itVD->second;
                    /* Get the associated disk image */
                    ovf::DiskImage di;
                    std::map<RTCString, ovf::DiskImage>::iterator foundDisk;

                    foundDisk = m->pReader->m_mapDisks.find(hd.strDiskId);
                    if (foundDisk == m->pReader->m_mapDisks.end())
                        continue;
                    else
                    {
                        di = foundDisk->second;
                    }

                    /*
                     * Figure out from URI which format the image of disk has.
                     * URI must have inside section <Disk>                   .
                     * But there aren't strong requirements about correspondence one URI for one disk virtual format.
                     * So possibly, we aren't able to recognize some URIs.
                     */

                    ComObjPtr<MediumFormat> mediumFormat;
                    rc = i_findMediumFormatFromDiskImage(di, mediumFormat);
                    if (FAILED(rc))
                        throw rc;

                    Bstr bstrFormatName;
                    rc = mediumFormat->COMGETTER(Name)(bstrFormatName.asOutParam());
                    if (FAILED(rc))
                        throw rc;
                    Utf8Str vdf = Utf8Str(bstrFormatName);

                    /// @todo
                    //  - figure out all possible vmdk formats we also support
                    //  - figure out if there is a url specifier for vhd already
                    //  - we need a url specifier for the vdi format

                    if (vdf.compare("VMDK", Utf8Str::CaseInsensitive) == 0)
                    {
                        /* If the href is empty use the VM name as filename */
                        Utf8Str strFilename = di.strHref;
                        if (!strFilename.length())
                            strFilename = Utf8StrFmt("%s.vmdk", hd.strDiskId.c_str());

                        Utf8Str strTargetPath = Utf8Str(strMachineFolder);
                        strTargetPath.append(RTPATH_DELIMITER).append(di.strHref);
                        /*
                         * Remove last extension from the file name if the file is compressed
                        */
                        if (di.strCompression.compare("gzip", Utf8Str::CaseInsensitive)==0)
                        {
                            strTargetPath.stripSuffix();
                        }

                        i_searchUniqueDiskImageFilePath(strTargetPath);

                        /* find the description for the hard disk controller
                         * that has the same ID as hd.idController */
                        const VirtualSystemDescriptionEntry *pController;
                        if (!(pController = pNewDesc->i_findControllerFromID(hd.idController)))
                            throw setError(E_FAIL,
                                           tr("Cannot find hard disk controller with OVF instance ID %RI32 "
                                              "to which disk \"%s\" should be attached"),
                                           hd.idController,
                                           di.strHref.c_str());

                        /* controller to attach to, and the bus within that controller */
                        Utf8StrFmt strExtraConfig("controller=%RI16;channel=%RI16",
                                                  pController->ulIndex,
                                                  hd.ulAddressOnParent);
                        pNewDesc->i_addEntry(VirtualSystemDescriptionType_HardDiskImage,
                                             hd.strDiskId,
                                             di.strHref,
                                             strTargetPath,
                                             di.ulSuggestedSizeMB,
                                             strExtraConfig);
                    }
                    else if (vdf.compare("RAW", Utf8Str::CaseInsensitive) == 0)
                    {
                        /* If the href is empty use the VM name as filename */
                        Utf8Str strFilename = di.strHref;
                        if (!strFilename.length())
                            strFilename = Utf8StrFmt("%s.iso", hd.strDiskId.c_str());

                        Utf8Str strTargetPath = Utf8Str(strMachineFolder)
                            .append(RTPATH_DELIMITER)
                            .append(di.strHref);
                        /*
                         * Remove last extension from the file name if the file is compressed
                        */
                        if (di.strCompression.compare("gzip", Utf8Str::CaseInsensitive)==0)
                        {
                            strTargetPath.stripSuffix();
                        }

                        i_searchUniqueDiskImageFilePath(strTargetPath);

                        /* find the description for the hard disk controller
                         * that has the same ID as hd.idController */
                        const VirtualSystemDescriptionEntry *pController;
                        if (!(pController = pNewDesc->i_findControllerFromID(hd.idController)))
                            throw setError(E_FAIL,
                                           tr("Cannot find disk controller with OVF instance ID %RI32 "
                                              "to which disk \"%s\" should be attached"),
                                           hd.idController,
                                           di.strHref.c_str());

                        /* controller to attach to, and the bus within that controller */
                        Utf8StrFmt strExtraConfig("controller=%RI16;channel=%RI16",
                                                  pController->ulIndex,
                                                  hd.ulAddressOnParent);
                        pNewDesc->i_addEntry(VirtualSystemDescriptionType_HardDiskImage,
                                             hd.strDiskId,
                                             di.strHref,
                                             strTargetPath,
                                             di.ulSuggestedSizeMB,
                                             strExtraConfig);
                    }
                    else
                        throw setError(VBOX_E_FILE_ERROR,
                                       tr("Unsupported format for virtual disk image %s in OVF: \"%s\""),
                                          di.strHref.c_str(),
                                          di.strFormat.c_str());
                }
            }

            m->virtualSystemDescriptions.push_back(pNewDesc);
        }
    }
    catch (HRESULT aRC)
    {
        /* On error we clear the list & return */
        m->virtualSystemDescriptions.clear();
        rc = aRC;
    }

    // reset the appliance state
    alock.acquire();
    m->state = Data::ApplianceIdle;

    return rc;
}

/**
 * Public method implementation. This creates one or more new machines according to the
 * VirtualSystemScription instances created by Appliance::Interpret().
 * Thread implementation is in Appliance::i_importImpl().
 * @param aOptions  Import options.
 * @param aProgress Progress object.
 * @return
 */
HRESULT Appliance::importMachines(const std::vector<ImportOptions_T> &aOptions,
                                  ComPtr<IProgress> &aProgress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (aOptions.size())
    {
        m->optListImport.setCapacity(aOptions.size());
        for (size_t i = 0; i < aOptions.size(); ++i)
        {
            m->optListImport.insert(i, aOptions[i]);
        }
    }

    AssertReturn(!(   m->optListImport.contains(ImportOptions_KeepAllMACs)
                   && m->optListImport.contains(ImportOptions_KeepNATMACs) )
                  , E_INVALIDARG);

    // do not allow entering this method if the appliance is busy reading or writing
    if (!i_isApplianceIdle())
        return E_ACCESSDENIED;

    if (!m->pReader)
        return setError(E_FAIL,
                        tr("Cannot import machines without reading it first (call read() before i_importMachines())"));

    ComObjPtr<Progress> progress;
    HRESULT rc = S_OK;
    try
    {
        rc = i_importImpl(m->locInfo, progress);
    }
    catch (HRESULT aRC)
    {
        rc = aRC;
    }

    if (SUCCEEDED(rc))
        /* Return progress to the caller */
        progress.queryInterfaceTo(aProgress.asOutParam());

    return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
// Appliance private methods
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Ensures that there is a look-ahead object ready.
 *
 * @returns true if there's an object handy, false if end-of-stream.
 * @throws HRESULT if the next object isn't a regular file. Sets error info
 *                 (which is why it's a method on Appliance and not the
 *                 ImportStack).
 */
bool Appliance::i_importEnsureOvaLookAhead(ImportStack &stack)
{
    Assert(stack.hVfsFssOva != NULL);
    if (stack.hVfsIosOvaLookAhead == NIL_RTVFSIOSTREAM)
    {
        RTStrFree(stack.pszOvaLookAheadName);
        stack.pszOvaLookAheadName = NULL;

        RTVFSOBJTYPE enmType;
        RTVFSOBJ hVfsObj;
        int vrc = RTVfsFsStrmNext(stack.hVfsFssOva, &stack.pszOvaLookAheadName, &enmType, &hVfsObj);
        if (RT_SUCCESS(vrc))
        {
            stack.hVfsIosOvaLookAhead = RTVfsObjToIoStream(hVfsObj);
            RTVfsObjRelease(hVfsObj);
            if (   (   enmType != RTVFSOBJTYPE_FILE
                    && enmType != RTVFSOBJTYPE_IO_STREAM)
                || stack.hVfsIosOvaLookAhead == NIL_RTVFSIOSTREAM)
                throw setError(VBOX_E_FILE_ERROR,
                               tr("Malformed OVA. '%s' is not a regular file (%d)."), stack.pszOvaLookAheadName, enmType);
        }
        else if (vrc == VERR_EOF)
            return false;
        else
            throw setErrorVrc(vrc, tr("RTVfsFsStrmNext failed (%Rrc)"), vrc);
    }
    return true;
}

HRESULT Appliance::i_preCheckImageAvailability(ImportStack &stack)
{
    if (i_importEnsureOvaLookAhead(stack))
        return S_OK;
    throw setError(VBOX_E_FILE_ERROR, tr("Unexpected end of OVA package"));
    /** @todo r=bird: dunno why this bother returning a value and the caller
     *        having a special 'continue' case for it. It always threw all non-OK
     *        status codes.  It's possibly to handle out of order stuff, so that
     *        needs adding to the testcase! */
}

/**
 * Opens a source file (for reading obviously).
 *
 * @param   stack
 * @param   rstrSrcPath         The source file to open.
 * @param   pszManifestEntry    The manifest entry of the source file.  This is
 *                              used when constructing our manifest using a pass
 *                              thru.
 * @returns I/O stream handle to the source file.
 * @throws  HRESULT error status, error info set.
 */
RTVFSIOSTREAM Appliance::i_importOpenSourceFile(ImportStack &stack, Utf8Str const &rstrSrcPath, const char *pszManifestEntry)
{
    /*
     * Open the source file.  Special considerations for OVAs.
     */
    RTVFSIOSTREAM hVfsIosSrc;
    if (stack.hVfsFssOva != NIL_RTVFSFSSTREAM)
    {
        for (uint32_t i = 0;; i++)
        {
            if (!i_importEnsureOvaLookAhead(stack))
                throw setErrorBoth(VBOX_E_FILE_ERROR, VERR_EOF,
                                   tr("Unexpected end of OVA / internal error - missing '%s' (skipped %u)"),
                                   rstrSrcPath.c_str(), i);
            if (RTStrICmp(stack.pszOvaLookAheadName, rstrSrcPath.c_str()) == 0)
                break;

            /* release the current object, loop to get the next. */
            RTVfsIoStrmRelease(stack.claimOvaLookAHead());
        }
        hVfsIosSrc = stack.claimOvaLookAHead();
    }
    else
    {
        int vrc = RTVfsIoStrmOpenNormal(rstrSrcPath.c_str(), RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE, &hVfsIosSrc);
        if (RT_FAILURE(vrc))
            throw setErrorVrc(vrc, tr("Error opening '%s' for reading (%Rrc)"), rstrSrcPath.c_str(), vrc);
    }

    /*
     * Digest calculation filtering.
     */
    hVfsIosSrc = i_manifestSetupDigestCalculationForGivenIoStream(hVfsIosSrc, pszManifestEntry);
    if (hVfsIosSrc == NIL_RTVFSIOSTREAM)
        throw E_FAIL;

    return hVfsIosSrc;
}

/**
 * Creates the destination file and fills it with bytes from the source stream.
 *
 * This assumes that we digest the source when fDigestTypes is non-zero, and
 * thus calls RTManifestPtIosAddEntryNow when done.
 *
 * @param   rstrDstPath     The path to the destination file.  Missing path
 *                          components will be created.
 * @param   hVfsIosSrc      The source I/O stream.
 * @param   rstrSrcLogNm    The name of the source for logging and error
 *                          messages.
 * @returns COM status code.
 * @throws Nothing (as the caller has VFS handles to release).
 */
HRESULT Appliance::i_importCreateAndWriteDestinationFile(Utf8Str const &rstrDstPath, RTVFSIOSTREAM hVfsIosSrc,
                                                         Utf8Str const &rstrSrcLogNm)
{
    int vrc;

    /*
     * Create the output file, including necessary paths.
     * Any existing file will be overwritten.
     */
    HRESULT hrc = VirtualBox::i_ensureFilePathExists(rstrDstPath, true /*fCreate*/);
    if (SUCCEEDED(hrc))
    {
        RTVFSIOSTREAM hVfsIosDst;
        vrc = RTVfsIoStrmOpenNormal(rstrDstPath.c_str(),
                                    RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_ALL,
                                    &hVfsIosDst);
        if (RT_SUCCESS(vrc))
        {
            /*
             * Pump the bytes thru. If we fail, delete the output file.
             */
            vrc = RTVfsUtilPumpIoStreams(hVfsIosSrc, hVfsIosDst, 0);
            if (RT_SUCCESS(vrc))
                hrc = S_OK;
            else
                hrc = setErrorVrc(vrc, tr("Error occured decompressing '%s' to '%s' (%Rrc)"),
                                  rstrSrcLogNm.c_str(), rstrDstPath.c_str(), vrc);
            uint32_t cRefs = RTVfsIoStrmRelease(hVfsIosDst);
            AssertMsg(cRefs == 0, ("cRefs=%u\n", cRefs)); NOREF(cRefs);
            if (RT_FAILURE(vrc))
                RTFileDelete(rstrDstPath.c_str());
        }
        else
            hrc = setErrorVrc(vrc, tr("Error opening destionation image '%s' for writing (%Rrc)"), rstrDstPath.c_str(), vrc);
    }
    return hrc;
}


/**
 *
 * @param   stack               Import stack.
 * @param   rstrSrcPath         Source path.
 * @param   rstrDstPath         Destination path.
 * @param   pszManifestEntry    The manifest entry of the source file.  This is
 *                              used when constructing our manifest using a pass
 *                              thru.
 * @throws HRESULT error status, error info set.
 */
void Appliance::i_importCopyFile(ImportStack &stack, Utf8Str const &rstrSrcPath, Utf8Str const &rstrDstPath,
                                 const char *pszManifestEntry)
{
    /*
     * Open the file (throws error) and add a read ahead thread so we can do
     * concurrent reads (+digest) and writes.
     */
    RTVFSIOSTREAM hVfsIosSrc = i_importOpenSourceFile(stack, rstrSrcPath, pszManifestEntry);
    RTVFSIOSTREAM hVfsIosReadAhead;
    int vrc = RTVfsCreateReadAheadForIoStream(hVfsIosSrc, 0 /*fFlags*/, 0 /*cBuffers=default*/, 0 /*cbBuffers=default*/,
                                              &hVfsIosReadAhead);
    if (RT_FAILURE(vrc))
    {
        RTVfsIoStrmRelease(hVfsIosSrc);
        throw setErrorVrc(vrc, tr("Error initializing read ahead thread for '%s' (%Rrc)"), rstrSrcPath.c_str(), vrc);
    }

    /*
     * Write the destination file (nothrow).
     */
    HRESULT hrc = i_importCreateAndWriteDestinationFile(rstrDstPath, hVfsIosReadAhead, rstrSrcPath);
    RTVfsIoStrmRelease(hVfsIosReadAhead);

    /*
     * Before releasing the source stream, make sure we've successfully added
     * the digest to our manifest.
     */
    if (SUCCEEDED(hrc) && m->fDigestTypes)
    {
        vrc = RTManifestPtIosAddEntryNow(hVfsIosSrc);
        if (RT_FAILURE(vrc))
            hrc = setErrorVrc(vrc, tr("RTManifestPtIosAddEntryNow failed with %Rrc"), vrc);
    }

    uint32_t cRefs = RTVfsIoStrmRelease(hVfsIosSrc);
    AssertMsg(cRefs == 0, ("cRefs=%u\n", cRefs)); NOREF(cRefs);
    if (SUCCEEDED(hrc))
        return;
    throw hrc;
}

/**
 *
 * @param   stack
 * @param   rstrSrcPath
 * @param   rstrDstPath
 * @param   pszManifestEntry    The manifest entry of the source file.  This is
 *                              used when constructing our manifest using a pass
 *                              thru.
 * @throws HRESULT error status, error info set.
 */
void Appliance::i_importDecompressFile(ImportStack &stack, Utf8Str const &rstrSrcPath, Utf8Str const &rstrDstPath,
                                       const char *pszManifestEntry)
{
    RTVFSIOSTREAM hVfsIosSrcCompressed = i_importOpenSourceFile(stack, rstrSrcPath, pszManifestEntry);

    /*
     * Add a read ahead thread here.  This means reading and digest calculation
     * is done on one thread, while unpacking and writing is one on this thread.
     */
    RTVFSIOSTREAM hVfsIosReadAhead;
    int vrc = RTVfsCreateReadAheadForIoStream(hVfsIosSrcCompressed, 0 /*fFlags*/, 0 /*cBuffers=default*/,
                                              0 /*cbBuffers=default*/, &hVfsIosReadAhead);
    if (RT_FAILURE(vrc))
    {
        RTVfsIoStrmRelease(hVfsIosSrcCompressed);
        throw setErrorVrc(vrc, tr("Error initializing read ahead thread for '%s' (%Rrc)"), rstrSrcPath.c_str(), vrc);
    }

    /*
     * Add decompression step.
     */
    RTVFSIOSTREAM hVfsIosSrc;
    vrc = RTZipGzipDecompressIoStream(hVfsIosReadAhead, 0, &hVfsIosSrc);
    RTVfsIoStrmRelease(hVfsIosReadAhead);
    if (RT_FAILURE(vrc))
    {
        RTVfsIoStrmRelease(hVfsIosSrcCompressed);
        throw setErrorVrc(vrc, tr("Error initializing gzip decompression for '%s' (%Rrc)"), rstrSrcPath.c_str(), vrc);
    }

    /*
     * Write the stream to the destination file (nothrow).
     */
    HRESULT hrc = i_importCreateAndWriteDestinationFile(rstrDstPath, hVfsIosSrc, rstrSrcPath);

    /*
     * Before releasing the source stream, make sure we've successfully added
     * the digest to our manifest.
     */
    if (SUCCEEDED(hrc) && m->fDigestTypes)
    {
        vrc = RTManifestPtIosAddEntryNow(hVfsIosSrcCompressed);
        if (RT_FAILURE(vrc))
            hrc = setErrorVrc(vrc, tr("RTManifestPtIosAddEntryNow failed with %Rrc"), vrc);
    }

    uint32_t cRefs = RTVfsIoStrmRelease(hVfsIosSrc);
    AssertMsg(cRefs == 0, ("cRefs=%u\n", cRefs)); NOREF(cRefs);

    cRefs = RTVfsIoStrmRelease(hVfsIosSrcCompressed);
    AssertMsg(cRefs == 0, ("cRefs=%u\n", cRefs)); NOREF(cRefs);

    if (SUCCEEDED(hrc))
        return;
    throw hrc;
}

/*******************************************************************************
 * Read stuff
 ******************************************************************************/

/**
 * Implementation for reading an OVF (via task).
 *
 * This starts a new thread which will call
 * Appliance::taskThreadImportOrExport() which will then call readFS(). This
 * will then open the OVF with ovfreader.cpp.
 *
 * This is in a separate private method because it is used from two locations:
 *
 * 1) from the public Appliance::Read().
 *
 * 2) in a second worker thread; in that case, Appliance::ImportMachines() called Appliance::i_importImpl(), which
 *    called Appliance::readFSOVA(), which called Appliance::i_importImpl(), which then called this again.
 *
 * @param   aLocInfo    The OVF location.
 * @param   aProgress   Where to return the progress object.
 * @throws  COM error codes will be thrown.
 */
void Appliance::i_readImpl(const LocationInfo &aLocInfo, ComObjPtr<Progress> &aProgress)
{
    BstrFmt bstrDesc = BstrFmt(tr("Reading appliance '%s'"),
                               aLocInfo.strPath.c_str());
    HRESULT rc;
    /* Create the progress object */
    aProgress.createObject();
    if (aLocInfo.storageType == VFSType_File)
        /* 1 operation only */
        rc = aProgress->init(mVirtualBox, static_cast<IAppliance*>(this),
                             bstrDesc.raw(),
                             TRUE /* aCancelable */);
    else
        /* 4/5 is downloading, 1/5 is reading */
        rc = aProgress->init(mVirtualBox, static_cast<IAppliance*>(this),
                             bstrDesc.raw(),
                             TRUE /* aCancelable */,
                             2, // ULONG cOperations,
                             5, // ULONG ulTotalOperationsWeight,
                             BstrFmt(tr("Download appliance '%s'"),
                                     aLocInfo.strPath.c_str()).raw(), // CBSTR bstrFirstOperationDescription,
                             4); // ULONG ulFirstOperationWeight,
    if (FAILED(rc)) throw rc;

    /* Initialize our worker task */
    TaskOVF *task = NULL;
    try
    {
        task = new TaskOVF(this, TaskOVF::Read, aLocInfo, aProgress);
    }
    catch (...)
    {
        throw setError(VBOX_E_OBJECT_NOT_FOUND,
                       tr("Could not create TaskOVF object for reading the OVF from disk"));
    }

    rc = task->createThread();
    if (FAILED(rc)) throw rc;
}

/**
 * Actual worker code for reading an OVF from disk. This is called from Appliance::taskThreadImportOrExport()
 * and therefore runs on the OVF read worker thread. This opens the OVF with ovfreader.cpp.
 *
 * This runs in one context:
 *
 * 1) in a first worker thread; in that case, Appliance::Read() called Appliance::readImpl();
 *
 * @param pTask
 * @return
 */
HRESULT Appliance::i_readFS(TaskOVF *pTask)
{
    LogFlowFuncEnter();
    LogFlowFunc(("Appliance %p\n", this));

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock appLock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc;
    if (pTask->locInfo.strPath.endsWith(".ovf", Utf8Str::CaseInsensitive))
        rc = i_readFSOVF(pTask);
    else
        rc = i_readFSOVA(pTask);

    LogFlowFunc(("rc=%Rhrc\n", rc));
    LogFlowFuncLeave();

    return rc;
}

HRESULT Appliance::i_readFSOVF(TaskOVF *pTask)
{
    LogFlowFunc(("'%s'\n", pTask->locInfo.strPath.c_str()));

    /*
     * Allocate a buffer for filenames and prep it for suffix appending.
     */
    char *pszNameBuf = (char *)alloca(pTask->locInfo.strPath.length() + 16);
    AssertReturn(pszNameBuf, VERR_NO_TMP_MEMORY);
    memcpy(pszNameBuf, pTask->locInfo.strPath.c_str(), pTask->locInfo.strPath.length() + 1);
    RTPathStripSuffix(pszNameBuf);
    size_t const cchBaseName = strlen(pszNameBuf);

    /*
     * Open the OVF file first since that is what this is all about.
     */
    RTVFSIOSTREAM hIosOvf;
    int vrc = RTVfsIoStrmOpenNormal(pTask->locInfo.strPath.c_str(),
                                    RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE, &hIosOvf);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Failed to open OVF file '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    HRESULT hrc = i_readOVFFile(pTask, hIosOvf, RTPathFilename(pTask->locInfo.strPath.c_str())); /* consumes hIosOvf */
    if (FAILED(hrc))
        return hrc;

    /*
     * Try open the manifest file (for signature purposes and to determine digest type(s)).
     */
    RTVFSIOSTREAM hIosMf;
    strcpy(&pszNameBuf[cchBaseName], ".mf");
    vrc = RTVfsIoStrmOpenNormal(pszNameBuf, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE, &hIosMf);
    if (RT_SUCCESS(vrc))
    {
        const char * const pszFilenamePart = RTPathFilename(pszNameBuf);
        hrc = i_readManifestFile(pTask, hIosMf /*consumed*/, pszFilenamePart);
        if (FAILED(hrc))
            return hrc;

        /*
         * Check for the signature file.
         */
        RTVFSIOSTREAM hIosCert;
        strcpy(&pszNameBuf[cchBaseName], ".cert");
        vrc = RTVfsIoStrmOpenNormal(pszNameBuf, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE, &hIosCert);
        if (RT_SUCCESS(vrc))
        {
            hrc = i_readSignatureFile(pTask, hIosCert /*consumed*/, pszFilenamePart);
            if (FAILED(hrc))
                return hrc;
        }
        else if (vrc != VERR_FILE_NOT_FOUND && vrc != VERR_PATH_NOT_FOUND)
            return setErrorVrc(vrc, tr("Failed to open the signature file '%s' (%Rrc)"), pszNameBuf, vrc);

    }
    else if (vrc == VERR_FILE_NOT_FOUND || vrc == VERR_PATH_NOT_FOUND)
    {
        m->fDeterminedDigestTypes = true;
        m->fDigestTypes           = 0;
    }
    else
        return setErrorVrc(vrc, tr("Failed to open the manifest file '%s' (%Rrc)"), pszNameBuf, vrc);

    /*
     * Do tail processing (check the signature).
     */
    hrc = i_readTailProcessing(pTask);

    LogFlowFunc(("returns %Rhrc\n", hrc));
    return hrc;
}

HRESULT Appliance::i_readFSOVA(TaskOVF *pTask)
{
    LogFlowFunc(("'%s'\n", pTask->locInfo.strPath.c_str()));

    /*
     * Open the tar file as file stream.
     */
    RTVFSIOSTREAM hVfsIosOva;
    int vrc = RTVfsIoStrmOpenNormal(pTask->locInfo.strPath.c_str(),
                                    RTFILE_O_READ | RTFILE_O_DENY_NONE | RTFILE_O_OPEN, &hVfsIosOva);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error opening the OVA file '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    RTVFSFSSTREAM hVfsFssOva;
    vrc = RTZipTarFsStreamFromIoStream(hVfsIosOva, 0 /*fFlags*/, &hVfsFssOva);
    RTVfsIoStrmRelease(hVfsIosOva);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error reading the OVA file '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    /*
     * Since jumping thru an OVA file with seekable disk backing is rather
     * efficient, we can process .ovf, .mf and .cert files here without any
     * strict ordering restrictions.
     *
     * (Technically, the .ovf-file comes first, while the manifest and its
     * optional signature file either follows immediately or at the very end of
     * the OVA. The manifest is optional.)
     */
    char    *pszOvfNameBase = NULL;
    size_t   cchOvfNameBase = 0; NOREF(cchOvfNameBase);
    unsigned cLeftToFind = 3;
    HRESULT  hrc = S_OK;
    do
    {
        char        *pszName = NULL;
        RTVFSOBJTYPE enmType;
        RTVFSOBJ     hVfsObj;
        vrc = RTVfsFsStrmNext(hVfsFssOva, &pszName, &enmType, &hVfsObj);
        if (RT_FAILURE(vrc))
        {
            if (vrc != VERR_EOF)
                hrc = setErrorVrc(vrc, tr("Error reading OVA '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);
            break;
        }

        /* We only care about entries that are files. Get the I/O stream handle for them. */
        if (   enmType  == RTVFSOBJTYPE_IO_STREAM
            || enmType  == RTVFSOBJTYPE_FILE)
        {
            /* Find the suffix and check if this is a possibly interesting file. */
            char *pszSuffix = strrchr(pszName, '.');
            if (   pszSuffix
                && (   RTStrICmp(pszSuffix + 1, "ovf") == 0
                    || RTStrICmp(pszSuffix + 1, "mf") == 0
                    || RTStrICmp(pszSuffix + 1, "cert") == 0) )
            {
                /* Match the OVF base name. */
                *pszSuffix = '\0';
                if (   pszOvfNameBase == NULL
                    || RTStrICmp(pszName, pszOvfNameBase) == 0)
                {
                    *pszSuffix = '.';

                    /* Since we're pretty sure we'll be processing this file, get the I/O stream. */
                    RTVFSIOSTREAM hVfsIos = RTVfsObjToIoStream(hVfsObj);
                    Assert(hVfsIos != NIL_RTVFSIOSTREAM);

                    /* Check for the OVF (should come first). */
                    if (RTStrICmp(pszSuffix + 1, "ovf") == 0)
                    {
                        if (pszOvfNameBase == NULL)
                        {
                            hrc = i_readOVFFile(pTask, hVfsIos, pszName);
                            hVfsIos = NIL_RTVFSIOSTREAM;

                            /* Set the base name. */
                            *pszSuffix = '\0';
                            pszOvfNameBase = pszName;
                            cchOvfNameBase = strlen(pszName);
                            pszName = NULL;
                            cLeftToFind--;
                        }
                        else
                            LogRel(("i_readFSOVA: '%s' contains more than one OVF file ('%s'), picking the first one\n",
                                    pTask->locInfo.strPath.c_str(), pszName));
                    }
                    /* Check for manifest. */
                    else if (RTStrICmp(pszSuffix + 1, "mf") == 0)
                    {
                        if (m->hMemFileTheirManifest == NIL_RTVFSFILE)
                        {
                            hrc = i_readManifestFile(pTask, hVfsIos, pszName);
                            hVfsIos = NIL_RTVFSIOSTREAM;  /*consumed*/
                            cLeftToFind--;
                        }
                        else
                            LogRel(("i_readFSOVA: '%s' contains more than one manifest file ('%s'), picking the first one\n",
                                    pTask->locInfo.strPath.c_str(), pszName));
                    }
                    /* Check for signature. */
                    else if (RTStrICmp(pszSuffix + 1, "cert") == 0)
                    {
                        if (!m->fSignerCertLoaded)
                        {
                            hrc = i_readSignatureFile(pTask, hVfsIos, pszName);
                            hVfsIos = NIL_RTVFSIOSTREAM;  /*consumed*/
                            cLeftToFind--;
                        }
                        else
                            LogRel(("i_readFSOVA: '%s' contains more than one signature file ('%s'), picking the first one\n",
                                    pTask->locInfo.strPath.c_str(), pszName));
                    }
                    else
                        AssertFailed();
                    if (hVfsIos != NIL_RTVFSIOSTREAM)
                        RTVfsIoStrmRelease(hVfsIos);
                }
            }
        }
        RTVfsObjRelease(hVfsObj);
        RTStrFree(pszName);
    } while (cLeftToFind > 0 && SUCCEEDED(hrc));

    RTVfsFsStrmRelease(hVfsFssOva);
    RTStrFree(pszOvfNameBase);

    /*
     * Check that we found and OVF file.
     */
    if (SUCCEEDED(hrc) && !pszOvfNameBase)
        hrc = setError(VBOX_E_FILE_ERROR, tr("OVA '%s' does not contain an .ovf-file"), pTask->locInfo.strPath.c_str());
    if (SUCCEEDED(hrc))
    {
        /*
         * Do tail processing (check the signature).
         */
        hrc = i_readTailProcessing(pTask);
    }
    LogFlowFunc(("returns %Rhrc\n", hrc));
    return hrc;
}

/**
 * Reads & parses the OVF file.
 *
 * @param   pTask               The read task.
 * @param   hVfsIosOvf          The I/O stream for the OVF.  The reference is
 *                              always consumed.
 * @param   pszManifestEntry    The manifest entry name.
 * @returns COM status code, error info set.
 * @throws  Nothing
 */
HRESULT Appliance::i_readOVFFile(TaskOVF *pTask, RTVFSIOSTREAM hVfsIosOvf, const char *pszManifestEntry)
{
    LogFlowFunc(("%s[%s]\n", pTask->locInfo.strPath.c_str(), pszManifestEntry));

    /*
     * Set the OVF manifest entry name (needed for tweaking the manifest
     * validation during import).
     */
    try         { m->strOvfManifestEntry = pszManifestEntry; }
    catch (...) { return E_OUTOFMEMORY; }

    /*
     * Set up digest calculation.
     */
    hVfsIosOvf = i_manifestSetupDigestCalculationForGivenIoStream(hVfsIosOvf, pszManifestEntry);
    if (hVfsIosOvf == NIL_RTVFSIOSTREAM)
        return VBOX_E_FILE_ERROR;

    /*
     * Read the OVF into a memory buffer and parse it.
     */
    void  *pvBufferedOvf;
    size_t cbBufferedOvf;
    int vrc = RTVfsIoStrmReadAll(hVfsIosOvf, &pvBufferedOvf, &cbBufferedOvf);
    uint32_t cRefs = RTVfsIoStrmRelease(hVfsIosOvf);     /* consumes stream handle.  */
    NOREF(cRefs);
    Assert(cRefs == 0);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Could not read the OVF file for '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    HRESULT hrc;
    try
    {
        m->pReader = new ovf::OVFReader(pvBufferedOvf, cbBufferedOvf, pTask->locInfo.strPath);
        hrc = S_OK;
    }
    catch (RTCError &rXcpt)      // includes all XML exceptions
    {
        hrc = setError(VBOX_E_FILE_ERROR, rXcpt.what());
    }
    catch (HRESULT aRC)
    {
        hrc = aRC;
    }
    catch (...)
    {
        hrc = E_FAIL;
    }
    LogFlowFunc(("OVFReader(%s) -> rc=%Rhrc\n", pTask->locInfo.strPath.c_str(), hrc));

    RTVfsIoStrmReadAllFree(pvBufferedOvf, cbBufferedOvf);
    if (SUCCEEDED(hrc))
    {
        /*
         * If we see an OVF v2.0 envelope, select only the SHA-256 digest.
         */
        if (   !m->fDeterminedDigestTypes
            && m->pReader->m_envelopeData.getOVFVersion() == ovf::OVFVersion_2_0)
            m->fDigestTypes &= ~RTMANIFEST_ATTR_SHA256;
    }

    return hrc;
}

/**
 * Reads & parses the manifest file.
 *
 * @param   pTask               The read task.
 * @param   hVfsIosMf           The I/O stream for the manifest file.  The
 *                              reference is always consumed.
 * @param   pszSubFileNm        The manifest filename (no path) for error
 *                              messages and logging.
 * @returns COM status code, error info set.
 * @throws  Nothing
 */
HRESULT Appliance::i_readManifestFile(TaskOVF *pTask, RTVFSIOSTREAM hVfsIosMf, const char *pszSubFileNm)
{
    LogFlowFunc(("%s[%s]\n", pTask->locInfo.strPath.c_str(), pszSubFileNm));

    /*
     * Copy the manifest into a memory backed file so we can later do signature
     * validation indepentend of the algorithms used by the signature.
     */
    int vrc = RTVfsMemorizeIoStreamAsFile(hVfsIosMf, RTFILE_O_READ, &m->hMemFileTheirManifest);
    RTVfsIoStrmRelease(hVfsIosMf);     /* consumes stream handle.  */
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error reading the manifest file '%s' for '%s' (%Rrc)"),
                           pszSubFileNm, pTask->locInfo.strPath.c_str(), vrc);

    /*
     * Parse the manifest.
     */
    Assert(m->hTheirManifest == NIL_RTMANIFEST);
    vrc = RTManifestCreate(0 /*fFlags*/, &m->hTheirManifest);
    AssertStmt(RT_SUCCESS(vrc), Global::vboxStatusCodeToCOM(vrc));

    char szErr[256];
    RTVFSIOSTREAM hVfsIos = RTVfsFileToIoStream(m->hMemFileTheirManifest);
    vrc = RTManifestReadStandardEx(m->hTheirManifest, hVfsIos, szErr, sizeof(szErr));
    RTVfsIoStrmRelease(hVfsIos);
    if (RT_FAILURE(vrc))
        throw setErrorVrc(vrc, tr("Failed to parse manifest file '%s' for '%s' (%Rrc): %s"),
                          pszSubFileNm, pTask->locInfo.strPath.c_str(), vrc, szErr);

    /*
     * Check which digest files are used.
     * Note! the file could be empty, in which case fDigestTypes is set to 0.
     */
    vrc = RTManifestQueryAllAttrTypes(m->hTheirManifest, true /*fEntriesOnly*/, &m->fDigestTypes);
    AssertRCReturn(vrc, Global::vboxStatusCodeToCOM(vrc));
    m->fDeterminedDigestTypes = true;

    return S_OK;
}

/**
 * Reads the signature & certificate file.
 *
 * @param   pTask               The read task.
 * @param   hVfsIosCert         The I/O stream for the signature file.  The
 *                              reference is always consumed.
 * @param   pszSubFileNm        The signature filename (no path) for error
 *                              messages and logging.  Used to construct
 *                              .mf-file name.
 * @returns COM status code, error info set.
 * @throws  Nothing
 */
HRESULT Appliance::i_readSignatureFile(TaskOVF *pTask, RTVFSIOSTREAM hVfsIosCert, const char *pszSubFileNm)
{
    LogFlowFunc(("%s[%s]\n", pTask->locInfo.strPath.c_str(), pszSubFileNm));

    /*
     * Construct the manifest filename from pszSubFileNm.
     */
    Utf8Str strManifestName;
    try
    {
        const char *pszSuffix = strrchr(pszSubFileNm, '.');
        AssertReturn(pszSuffix, E_FAIL);
        strManifestName = Utf8Str(pszSubFileNm, pszSuffix - pszSubFileNm);
        strManifestName.append(".mf");
    }
    catch (...)
    {
        return E_OUTOFMEMORY;
    }

    /*
     * Copy the manifest into a memory buffer.  We'll do the signature processing
     * later to not force any specific order in the OVAs or any other archive we
     * may be accessing later.
     */
    void  *pvSignature;
    size_t cbSignature;
    int vrc = RTVfsIoStrmReadAll(hVfsIosCert, &pvSignature, &cbSignature);
    RTVfsIoStrmRelease(hVfsIosCert);     /* consumes stream handle.  */
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error reading the signature file '%s' for '%s' (%Rrc)"),
                           pszSubFileNm, pTask->locInfo.strPath.c_str(), vrc);

    /*
     * Parse the signing certificate. Unlike the manifest parser we use below,
     * this API ignores parse of the file that aren't relevant.
     */
    RTERRINFOSTATIC StaticErrInfo;
    vrc = RTCrX509Certificate_ReadFromBuffer(&m->SignerCert, pvSignature, cbSignature,
                                             RTCRX509CERT_READ_F_PEM_ONLY,
                                             &g_RTAsn1DefaultAllocator, RTErrInfoInitStatic(&StaticErrInfo), pszSubFileNm);
    HRESULT hrc;
    if (RT_SUCCESS(vrc))
    {
        m->fSignerCertLoaded = true;
        m->fCertificateIsSelfSigned = RTCrX509Certificate_IsSelfSigned(&m->SignerCert);

        /*
         * Find the start of the certificate part of the file, so we can avoid
         * upsetting the manifest parser with it.
         */
        char *pszSplit = (char *)RTCrPemFindFirstSectionInContent(pvSignature, cbSignature,
                                                                  g_aRTCrX509CertificateMarkers, g_cRTCrX509CertificateMarkers);
        if (pszSplit)
            while (   pszSplit != (char *)pvSignature
                   && pszSplit[-1] != '\n'
                   && pszSplit[-1] != '\r')
                pszSplit--;
        else
        {
            AssertLogRelMsgFailed(("Failed to find BEGIN CERTIFICATE markers in '%s'::'%s' - impossible unless it's a DER encoded certificate!",
                                   pTask->locInfo.strPath.c_str(), pszSubFileNm));
            pszSplit = (char *)pvSignature + cbSignature;
        }
        *pszSplit = '\0';

        /*
         * Now, read the manifest part.  We use the IPRT manifest reader here
         * to avoid duplicating code and be somewhat flexible wrt the digest
         * type choosen by the signer.
         */
        RTMANIFEST hSignedDigestManifest;
        vrc = RTManifestCreate(0 /*fFlags*/, &hSignedDigestManifest);
        if (RT_SUCCESS(vrc))
        {
            RTVFSIOSTREAM hVfsIosTmp;
            vrc = RTVfsIoStrmFromBuffer(RTFILE_O_READ, pvSignature, pszSplit - (char *)pvSignature, &hVfsIosTmp);
            if (RT_SUCCESS(vrc))
            {
                vrc = RTManifestReadStandardEx(hSignedDigestManifest, hVfsIosTmp, StaticErrInfo.szMsg, sizeof(StaticErrInfo.szMsg));
                RTVfsIoStrmRelease(hVfsIosTmp);
                if (RT_SUCCESS(vrc))
                {
                    /*
                     * Get signed digest, we prefer SHA-2, so explicitly query those first.
                     */
                    uint32_t fDigestType;
                    char     szSignedDigest[_8K + 1];
                    vrc = RTManifestEntryQueryAttr(hSignedDigestManifest, strManifestName.c_str(), NULL,
                                                   RTMANIFEST_ATTR_SHA512 | RTMANIFEST_ATTR_SHA256,
                                                   szSignedDigest, sizeof(szSignedDigest), &fDigestType);
                    if (vrc == VERR_MANIFEST_ATTR_TYPE_NOT_FOUND)
                        vrc = RTManifestEntryQueryAttr(hSignedDigestManifest, strManifestName.c_str(), NULL,
                                                       RTMANIFEST_ATTR_ANY, szSignedDigest, sizeof(szSignedDigest), &fDigestType);
                    if (RT_SUCCESS(vrc))
                    {
                        const char *pszSignedDigest = RTStrStrip(szSignedDigest);
                        size_t      cbSignedDigest  = strlen(pszSignedDigest) / 2;
                        uint8_t     abSignedDigest[sizeof(szSignedDigest) / 2];
                        vrc = RTStrConvertHexBytes(szSignedDigest, abSignedDigest, cbSignedDigest, 0 /*fFlags*/);
                        if (RT_SUCCESS(vrc))
                        {
                            /*
                             * Convert it to RTDIGESTTYPE_XXX and save the binary value for later use.
                             */
                            switch (fDigestType)
                            {
                                case RTMANIFEST_ATTR_SHA1:      m->enmSignedDigestType = RTDIGESTTYPE_SHA1; break;
                                case RTMANIFEST_ATTR_SHA256:    m->enmSignedDigestType = RTDIGESTTYPE_SHA256; break;
                                case RTMANIFEST_ATTR_SHA512:    m->enmSignedDigestType = RTDIGESTTYPE_SHA512; break;
                                case RTMANIFEST_ATTR_MD5:       m->enmSignedDigestType = RTDIGESTTYPE_MD5; break;
                                default:    AssertFailed();     m->enmSignedDigestType = RTDIGESTTYPE_INVALID; break;
                            }
                            if (m->enmSignedDigestType != RTDIGESTTYPE_INVALID)
                            {
                                m->pbSignedDigest = (uint8_t *)RTMemDup(abSignedDigest, cbSignedDigest);
                                m->cbSignedDigest = cbSignedDigest;
                                hrc = S_OK;
                            }
                            else
                                hrc = setError(E_FAIL, tr("Unsupported signed digest type (%#x)"), fDigestType);
                        }
                        else
                            hrc = setErrorVrc(vrc, tr("Error reading signed manifest digest: %Rrc"), vrc);
                    }
                    else if (vrc == VERR_NOT_FOUND)
                        hrc = setErrorVrc(vrc, tr("Could not locate signed digest for '%s' in the cert-file for '%s'"),
                                          strManifestName.c_str(), pTask->locInfo.strPath.c_str());
                    else
                        hrc = setErrorVrc(vrc, tr("RTManifestEntryQueryAttr failed unexpectedly: %Rrc"), vrc);
                }
                else
                    hrc = setErrorVrc(vrc, tr("Error parsing the .cert-file for '%s': %s"),
                                      pTask->locInfo.strPath.c_str(), StaticErrInfo.szMsg);
            }
            else
                hrc = E_OUTOFMEMORY;
            RTManifestRelease(hSignedDigestManifest);
        }
        else
            hrc = E_OUTOFMEMORY;
    }
    else if (vrc == VERR_NOT_FOUND || vrc == VERR_EOF)
        hrc = setErrorBoth(E_FAIL, vrc, tr("Malformed .cert-file for '%s': Signer's certificate not found (%Rrc)"),
                           pTask->locInfo.strPath.c_str(), vrc);
    else
        hrc = setErrorVrc(vrc, tr("Error reading the signer's certificate from '%s' for '%s' (%Rrc): %s"),
                          pszSubFileNm, pTask->locInfo.strPath.c_str(), vrc, StaticErrInfo.Core.pszMsg);

    RTVfsIoStrmReadAllFree(pvSignature, cbSignature);
    LogFlowFunc(("returns %Rhrc (%Rrc)\n", hrc, vrc));
    return hrc;
}


/**
 * Does tail processing after the files have been read in.
 *
 * @param   pTask               The read task.
 * @returns COM status.
 * @throws  Nothing!
 */
HRESULT Appliance::i_readTailProcessing(TaskOVF *pTask)
{
    /*
     * Parse and validate the signature file.
     *
     * The signature file has two parts, manifest part and a PEM encoded
     * certificate.  The former contains an entry for the manifest file with a
     * digest that is encrypted with the certificate in the latter part.
     */
    if (m->pbSignedDigest)
    {
        /* Since we're validating the digest of the manifest, there have to be
           a manifest.  We cannot allow a the manifest to be missing.  */
        if (m->hMemFileTheirManifest == NIL_RTVFSFILE)
            return setError(VBOX_E_FILE_ERROR, tr("Found .cert-file but no .mf-file for '%s'"), pTask->locInfo.strPath.c_str());

        /*
         * Validate the signed digest.
         *
         * It's possible we should allow the user to ignore signature
         * mismatches, but for now it is a solid show stopper.
         */
        HRESULT hrc;
        RTERRINFOSTATIC StaticErrInfo;

        /* Calc the digest of the manifest using the algorithm found above. */
        RTCRDIGEST hDigest;
        int vrc = RTCrDigestCreateByType(&hDigest, m->enmSignedDigestType);
        if (RT_SUCCESS(vrc))
        {
            vrc = RTCrDigestUpdateFromVfsFile(hDigest, m->hMemFileTheirManifest, true /*fRewindFile*/);
            if (RT_SUCCESS(vrc))
            {
                /* Compare the signed digest with the one we just calculated.  (This
                   API will do the verification twice, once using IPRT's own crypto
                   and once using OpenSSL.  Both must OK it for success.) */
                vrc = RTCrPkixPubKeyVerifySignedDigest(&m->SignerCert.TbsCertificate.SubjectPublicKeyInfo.Algorithm.Algorithm,
                                                       &m->SignerCert.TbsCertificate.SubjectPublicKeyInfo.Algorithm.Parameters,
                                                       &m->SignerCert.TbsCertificate.SubjectPublicKeyInfo.SubjectPublicKey,
                                                       m->pbSignedDigest, m->cbSignedDigest, hDigest,
                                                       RTErrInfoInitStatic(&StaticErrInfo));
                if (RT_SUCCESS(vrc))
                {
                    m->fSignatureValid = true;
                    hrc = S_OK;
                }
                else if (vrc == VERR_CR_PKIX_SIGNATURE_MISMATCH)
                    hrc = setErrorVrc(vrc, tr("The manifest signature does not match"));
                else
                    hrc = setErrorVrc(vrc,
                                      tr("Error validating the manifest signature (%Rrc, %s)"), vrc, StaticErrInfo.Core.pszMsg);
            }
            else
                hrc = setErrorVrc(vrc, tr("RTCrDigestUpdateFromVfsFile failed: %Rrc"), vrc);
            RTCrDigestRelease(hDigest);
        }
        else
            hrc = setErrorVrc(vrc, tr("RTCrDigestCreateByType failed: %Rrc"), vrc);

        /*
         * Validate the certificate.
         *
         * We don't fail here on if we cannot validate the certificate, we postpone
         * that till the import stage, so that we can allow the user to ignore it.
         *
         * The certificate validity time is deliberately left as warnings as the
         * OVF specification does not provision for any timestamping of the
         * signature. This is course a security concern, but the whole signing
         * of OVFs is currently weirdly trusting (self signed * certs), so this
         * is the least of our current problems.
         *
         * While we try build and verify certificate paths properly, the
         * "neighbours" quietly ignores this and seems only to check the signature
         * and not whether the certificate is trusted.  Also, we don't currently
         * complain about self-signed certificates either (ditto "neighbours").
         * The OVF creator is also a bit restricted wrt to helping us build the
         * path as he cannot supply intermediate certificates.  Anyway, we issue
         * warnings (goes to /dev/null, am I right?) for self-signed certificates
         * and certificates we cannot build and verify a root path for.
         *
         * (The OVF sillibuggers should've used PKCS#7, CMS or something else
         * that's already been standardized instead of combining manifests with
         * certificate PEM files in some very restrictive manner!  I wonder if
         * we could add a PKCS#7 section to the .cert file in addition to the CERT
         * and manifest stuff dictated by the standard.  Would depend on how others
         * deal with it.)
         */
        Assert(!m->fCertificateValid);
        Assert(m->fCertificateMissingPath);
        Assert(!m->fCertificateValidTime);
        Assert(m->strCertError.isEmpty());
        Assert(m->fCertificateIsSelfSigned == RTCrX509Certificate_IsSelfSigned(&m->SignerCert));

        HRESULT hrc2 = S_OK;
        if (m->fCertificateIsSelfSigned)
        {
            /*
             * It's a self signed certificate.  We assume the frontend will
             * present this fact to the user and give a choice whether this
             * is acceptible.  But, first make sure it makes internal sense.
             */
            m->fCertificateMissingPath = true; /** @todo need to check if the certificate is trusted by the system! */
            vrc = RTCrX509Certificate_VerifySignatureSelfSigned(&m->SignerCert, RTErrInfoInitStatic(&StaticErrInfo));
            if (RT_SUCCESS(vrc))
            {
                m->fCertificateValid = true;

                /* Check whether the certificate is currently valid, just warn if not. */
                RTTIMESPEC Now;
                if (RTCrX509Validity_IsValidAtTimeSpec(&m->SignerCert.TbsCertificate.Validity, RTTimeNow(&Now)))
                {
                    m->fCertificateValidTime = true;
                    i_addWarning(tr("A self signed certificate was used to sign '%s'"), pTask->locInfo.strPath.c_str());
                }
                else
                    i_addWarning(tr("Self signed certificate used to sign '%s' is not currently valid"),
                                 pTask->locInfo.strPath.c_str());

                /* Just warn if it's not a CA. Self-signed certificates are
                   hardly trustworthy to start with without the user's consent. */
                if (   !m->SignerCert.TbsCertificate.T3.pBasicConstraints
                    || !m->SignerCert.TbsCertificate.T3.pBasicConstraints->CA.fValue)
                    i_addWarning(tr("Self signed certificate used to sign '%s' is not marked as certificate authority (CA)"),
                                 pTask->locInfo.strPath.c_str());
            }
            else
            {
                try { m->strCertError = Utf8StrFmt(tr("Verification of the self signed certificate failed (%Rrc, %s)"),
                                                   vrc, StaticErrInfo.Core.pszMsg); }
                catch (...) { AssertFailed(); }
                i_addWarning(tr("Verification of the self signed certificate used to sign '%s' failed (%Rrc): %s"),
                             pTask->locInfo.strPath.c_str(), vrc, StaticErrInfo.Core.pszMsg);
            }
        }
        else
        {
            /*
             * The certificate is not self-signed.  Use the system certificate
             * stores to try build a path that validates successfully.
             */
            RTCRX509CERTPATHS hCertPaths;
            vrc = RTCrX509CertPathsCreate(&hCertPaths, &m->SignerCert);
            if (RT_SUCCESS(vrc))
            {
                /* Get trusted certificates from the system and add them to the path finding mission. */
                RTCRSTORE hTrustedCerts;
                vrc = RTCrStoreCreateSnapshotOfUserAndSystemTrustedCAsAndCerts(&hTrustedCerts,
                                                                               RTErrInfoInitStatic(&StaticErrInfo));
                if (RT_SUCCESS(vrc))
                {
                    vrc = RTCrX509CertPathsSetTrustedStore(hCertPaths, hTrustedCerts);
                    if (RT_FAILURE(vrc))
                        hrc2 = setError(E_FAIL, tr("RTCrX509CertPathsSetTrustedStore failed (%Rrc)"), vrc);
                    RTCrStoreRelease(hTrustedCerts);
                }
                else
                    hrc2 = setError(E_FAIL,
                                    tr("Failed to query trusted CAs and Certificates from the system and for the current user (%Rrc, %s)"),
                                    vrc, StaticErrInfo.Core.pszMsg);

                /* Add untrusted intermediate certificates. */
                if (RT_SUCCESS(vrc))
                {
                    /// @todo RTCrX509CertPathsSetUntrustedStore(hCertPaths, hAdditionalCerts);
                    /// By scanning for additional certificates in the .cert file?  It would be
                    /// convenient to be able to supply intermediate certificates for the user,
                    /// right?  Or would that be unacceptable as it may weaken security?
                    ///
                    /// Anyway, we should look for intermediate certificates on the system, at
                    /// least.
                }
                if (RT_SUCCESS(vrc))
                {
                    /*
                     * Do the building and verification of certificate paths.
                     */
                    vrc = RTCrX509CertPathsBuild(hCertPaths, RTErrInfoInitStatic(&StaticErrInfo));
                    if (RT_SUCCESS(vrc))
                    {
                        vrc = RTCrX509CertPathsValidateAll(hCertPaths, NULL, RTErrInfoInitStatic(&StaticErrInfo));
                        if (RT_SUCCESS(vrc))
                        {
                            /*
                             * Mark the certificate as good.
                             */
                            /** @todo check the certificate purpose? If so, share with self-signed. */
                            m->fCertificateValid = true;
                            m->fCertificateMissingPath = false;

                            /*
                             * We add a warning if the certificate path isn't valid at the current
                             * time.  Since the time is only considered during path validation and we
                             * can repeat the validation process (but not building), it's easy to check.
                             */
                            RTTIMESPEC Now;
                            vrc = RTCrX509CertPathsSetValidTimeSpec(hCertPaths, RTTimeNow(&Now));
                            if (RT_SUCCESS(vrc))
                            {
                                vrc = RTCrX509CertPathsValidateAll(hCertPaths, NULL, RTErrInfoInitStatic(&StaticErrInfo));
                                if (RT_SUCCESS(vrc))
                                    m->fCertificateValidTime = true;
                                else
                                    i_addWarning(tr("The certificate used to sign '%s' (or a certificate in the path) is not currently valid (%Rrc)"),
                                                 pTask->locInfo.strPath.c_str(), vrc);
                            }
                            else
                                hrc2 = setErrorVrc(vrc, "RTCrX509CertPathsSetValidTimeSpec failed: %Rrc", vrc);
                        }
                        else if (vrc == VERR_CR_X509_CPV_NO_TRUSTED_PATHS)
                        {
                            m->fCertificateValid = true;
                            i_addWarning(tr("No trusted certificate paths"));

                            /* Add another warning if the pathless certificate is not valid at present. */
                            RTTIMESPEC Now;
                            if (RTCrX509Validity_IsValidAtTimeSpec(&m->SignerCert.TbsCertificate.Validity, RTTimeNow(&Now)))
                                m->fCertificateValidTime = true;
                            else
                                i_addWarning(tr("The certificate used to sign '%s' is not currently valid"),
                                             pTask->locInfo.strPath.c_str());
                        }
                        else
                            hrc2 = setError(E_FAIL, tr("Certificate path validation failed (%Rrc, %s)"),
                                            vrc, StaticErrInfo.Core.pszMsg);
                    }
                    else
                        hrc2 = setError(E_FAIL, tr("Certificate path building failed (%Rrc, %s)"),
                                        vrc, StaticErrInfo.Core.pszMsg);
                }
                RTCrX509CertPathsRelease(hCertPaths);
            }
            else
                hrc2 = setErrorVrc(vrc, tr("RTCrX509CertPathsCreate failed: %Rrc"), vrc);
        }

        /* Merge statuses from signature and certificate validation, prefering the signature one. */
        if (SUCCEEDED(hrc) && FAILED(hrc2))
            hrc = hrc2;
        if (FAILED(hrc))
            return hrc;
    }

    /** @todo provide details about the signatory, signature, etc.  */
    if (m->fSignerCertLoaded)
    {
        m->ptrCertificateInfo.createObject();
        m->ptrCertificateInfo->initCertificate(&m->SignerCert,
                                               m->fCertificateValid && !m->fCertificateMissingPath,
                                               !m->fCertificateValidTime);
    }

    /*
     * If there is a manifest, check that the OVF digest matches up (if present).
     */

    NOREF(pTask);
    return S_OK;
}



/*******************************************************************************
 * Import stuff
 ******************************************************************************/

/**
 * Implementation for importing OVF data into VirtualBox. This starts a new thread which will call
 * Appliance::taskThreadImportOrExport().
 *
 * This creates one or more new machines according to the VirtualSystemScription instances created by
 * Appliance::Interpret().
 *
 * This is in a separate private method because it is used from one location:
 *
 * 1) from the public Appliance::ImportMachines().
 *
 * @param locInfo
 * @param progress
 * @return
 */
HRESULT Appliance::i_importImpl(const LocationInfo &locInfo,
                                ComObjPtr<Progress> &progress)
{
    HRESULT rc = S_OK;

    SetUpProgressMode mode;
    if (locInfo.storageType == VFSType_File)
        mode = ImportFile;
    else
        mode = ImportS3;

    rc = i_setUpProgress(progress,
                         BstrFmt(tr("Importing appliance '%s'"), locInfo.strPath.c_str()),
                         mode);
    if (FAILED(rc)) throw rc;

    /* Initialize our worker task */
    TaskOVF* task = NULL;
    try
    {
        task = new TaskOVF(this, TaskOVF::Import, locInfo, progress);
    }
    catch(...)
    {
        delete task;
        throw rc = setError(VBOX_E_OBJECT_NOT_FOUND,
                            tr("Could not create TaskOVF object for importing OVF data into VirtualBox"));
    }

    rc = task->createThread();
    if (FAILED(rc)) throw rc;

    return rc;
}

/**
 * Actual worker code for importing OVF data into VirtualBox.
 *
 * This is called from Appliance::taskThreadImportOrExport() and therefore runs
 * on the OVF import worker thread. This creates one or more new machines
 * according to the VirtualSystemScription instances created by
 * Appliance::Interpret().
 *
 * This runs in two contexts:
 *
 * 1) in a first worker thread; in that case, Appliance::ImportMachines() called
 *    Appliance::i_importImpl();
 *
 * 2) in a second worker thread; in that case, Appliance::ImportMachines()
 *    called Appliance::i_importImpl(), which called Appliance::i_importFSOVA(),
 *    which called Appliance::i_importImpl(), which then called this again.
 *
 * @param   pTask       The OVF task data.
 * @return  COM status code.
 */
HRESULT Appliance::i_importFS(TaskOVF *pTask)
{
    LogFlowFuncEnter();
    LogFlowFunc(("Appliance %p\n", this));

    /* Change the appliance state so we can safely leave the lock while doing
     * time-consuming disk imports; also the below method calls do all kinds of
     * locking which conflicts with the appliance object lock. */
    AutoWriteLock writeLock(this COMMA_LOCKVAL_SRC_POS);
    /* Check if the appliance is currently busy. */
    if (!i_isApplianceIdle())
        return E_ACCESSDENIED;
    /* Set the internal state to importing. */
    m->state = Data::ApplianceImporting;

    HRESULT rc = S_OK;

    /* Clear the list of imported machines, if any */
    m->llGuidsMachinesCreated.clear();

    if (pTask->locInfo.strPath.endsWith(".ovf", Utf8Str::CaseInsensitive))
        rc = i_importFSOVF(pTask, writeLock);
    else
        rc = i_importFSOVA(pTask, writeLock);
    if (FAILED(rc))
    {
        /* With _whatever_ error we've had, do a complete roll-back of
         * machines and disks we've created */
        writeLock.release();
        ErrorInfoKeeper eik;
        for (list<Guid>::iterator itID = m->llGuidsMachinesCreated.begin();
             itID != m->llGuidsMachinesCreated.end();
             ++itID)
        {
            Guid guid = *itID;
            Bstr bstrGuid = guid.toUtf16();
            ComPtr<IMachine> failedMachine;
            HRESULT rc2 = mVirtualBox->FindMachine(bstrGuid.raw(), failedMachine.asOutParam());
            if (SUCCEEDED(rc2))
            {
                SafeIfaceArray<IMedium> aMedia;
                rc2 = failedMachine->Unregister(CleanupMode_DetachAllReturnHardDisksOnly, ComSafeArrayAsOutParam(aMedia));
                ComPtr<IProgress> pProgress2;
                rc2 = failedMachine->DeleteConfig(ComSafeArrayAsInParam(aMedia), pProgress2.asOutParam());
                pProgress2->WaitForCompletion(-1);
            }
        }
        writeLock.acquire();
    }

    /* Reset the state so others can call methods again */
    m->state = Data::ApplianceIdle;

    LogFlowFunc(("rc=%Rhrc\n", rc));
    LogFlowFuncLeave();
    return rc;
}

HRESULT Appliance::i_importFSOVF(TaskOVF *pTask, AutoWriteLockBase &rWriteLock)
{
    return i_importDoIt(pTask, rWriteLock);
}

HRESULT Appliance::i_importFSOVA(TaskOVF *pTask, AutoWriteLockBase &rWriteLock)
{
    LogFlowFuncEnter();

    /*
     * Open the tar file as file stream.
     */
    RTVFSIOSTREAM hVfsIosOva;
    int vrc = RTVfsIoStrmOpenNormal(pTask->locInfo.strPath.c_str(),
                                    RTFILE_O_READ | RTFILE_O_DENY_NONE | RTFILE_O_OPEN, &hVfsIosOva);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error opening the OVA file '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    RTVFSFSSTREAM hVfsFssOva;
    vrc = RTZipTarFsStreamFromIoStream(hVfsIosOva, 0 /*fFlags*/, &hVfsFssOva);
    RTVfsIoStrmRelease(hVfsIosOva);
    if (RT_FAILURE(vrc))
        return setErrorVrc(vrc, tr("Error reading the OVA file '%s' (%Rrc)"), pTask->locInfo.strPath.c_str(), vrc);

    /*
     * Join paths with the i_importFSOVF code.
     *
     * Note! We don't need to skip the OVF, manifest or signature files, as the
     *       i_importMachineGeneric, i_importVBoxMachine and i_importOpenSourceFile
     *       code will deal with this (as there could be other files in the OVA
     *       that we don't process, like 'de-DE-resources.xml' in EXAMPLE 1,
     *       Appendix D.1, OVF v2.1.0).
     */
    HRESULT hrc = i_importDoIt(pTask, rWriteLock, hVfsFssOva);

    RTVfsFsStrmRelease(hVfsFssOva);

    LogFlowFunc(("returns %Rhrc\n", hrc));
    return hrc;
}

/**
 * Does the actual importing after the caller has made the source accessible.
 *
 * @param   pTask               The import task.
 * @param   rWriteLock          The write lock the caller's caller is holding,
 *                              will be released for some reason.
 * @param   hVfsFssOva          The file system stream if OVA, NIL if not.
 * @returns COM status code.
 * @throws  Nothing.
 */
HRESULT Appliance::i_importDoIt(TaskOVF *pTask, AutoWriteLockBase &rWriteLock, RTVFSFSSTREAM hVfsFssOva /*= NIL_RTVFSFSSTREAM*/)
{
    rWriteLock.release();

    HRESULT hrc = E_FAIL;
    try
    {
        /*
         * Create the import stack for the rollback on errors.
         */
        ImportStack stack(pTask->locInfo, m->pReader->m_mapDisks, pTask->pProgress, hVfsFssOva);

        try
        {
            /* Do the importing. */
            i_importMachines(stack);

            /* We should've processed all the files now, so compare. */
            hrc = i_verifyManifestFile(stack);
        }
        catch (HRESULT hrcXcpt)
        {
            hrc = hrcXcpt;
        }
        catch (...)
        {
            AssertFailed();
            hrc = E_FAIL;
        }
        if (FAILED(hrc))
        {
            /*
             * Restoring original UUID from OVF description file.
             * During import VBox creates new UUIDs for imported images and
             * assigns them to the images. In case of failure we have to restore
             * the original UUIDs because those new UUIDs are obsolete now and
             * won't be used anymore.
             */
            ErrorInfoKeeper eik; /* paranoia */
            list< ComObjPtr<VirtualSystemDescription> >::const_iterator itvsd;
            /* Iterate through all virtual systems of that appliance */
            for (itvsd = m->virtualSystemDescriptions.begin();
                 itvsd != m->virtualSystemDescriptions.end();
                 ++itvsd)
            {
                ComObjPtr<VirtualSystemDescription> vsdescThis = (*itvsd);
                settings::MachineConfigFile *pConfig = vsdescThis->m->pConfig;
                if(vsdescThis->m->pConfig!=NULL)
                    stack.restoreOriginalUUIDOfAttachedDevice(pConfig);
            }
        }
    }
    catch (...)
    {
        hrc = E_FAIL;
        AssertFailed();
    }

    rWriteLock.acquire();
    return hrc;
}

/**
 * Undocumented, you figure it from the name.
 *
 * @returns Undocumented
 * @param   stack               Undocumented.
 */
HRESULT Appliance::i_verifyManifestFile(ImportStack &stack)
{
    LogFlowThisFuncEnter();
    HRESULT hrc;
    int vrc;

    /*
     * No manifest is fine, it always matches.
     */
    if (m->hTheirManifest == NIL_RTMANIFEST)
        hrc = S_OK;
    else
    {
        /*
         * Hack: If the manifest we just read doesn't have a digest for the OVF, copy
         *       it from the manifest we got from the caller.
         * @bugref{6022#c119}
         */
        if (   !RTManifestEntryExists(m->hTheirManifest, m->strOvfManifestEntry.c_str())
            && RTManifestEntryExists(m->hOurManifest, m->strOvfManifestEntry.c_str()) )
        {
            uint32_t fType = 0;
            char szDigest[512 + 1];
            vrc = RTManifestEntryQueryAttr(m->hOurManifest, m->strOvfManifestEntry.c_str(), NULL, RTMANIFEST_ATTR_ANY,
                                           szDigest, sizeof(szDigest), &fType);
            if (RT_SUCCESS(vrc))
                vrc = RTManifestEntrySetAttr(m->hTheirManifest, m->strOvfManifestEntry.c_str(),
                                             NULL /*pszAttr*/, szDigest, fType);
            if (RT_FAILURE(vrc))
                return setError(VBOX_E_IPRT_ERROR, tr("Error fudging missing OVF digest in manifest: %Rrc"), vrc);
        }

        /*
         * Compare with the digests we've created while read/processing the import.
         *
         * We specify the RTMANIFEST_EQUALS_IGN_MISSING_ATTRS to ignore attributes
         * (SHA1, SHA256, etc) that are only present in one of the manifests, as long
         * as each entry has at least one common attribute that we can check.  This
         * is important for the OVF in OVAs, for which we generates several digests
         * since we don't know which are actually used in the manifest (OVF comes
         * first in an OVA, then manifest).
         */
        char szErr[256];
        vrc = RTManifestEqualsEx(m->hTheirManifest, m->hOurManifest, NULL /*papszIgnoreEntries*/,
                                 NULL /*papszIgnoreAttrs*/,
                                 RTMANIFEST_EQUALS_IGN_MISSING_ATTRS | RTMANIFEST_EQUALS_IGN_MISSING_ENTRIES_2ND,
                                 szErr, sizeof(szErr));
        if (RT_SUCCESS(vrc))
            hrc = S_OK;
        else
            hrc = setErrorVrc(vrc, tr("Digest mismatch (%Rrc): %s"), vrc, szErr);
    }

    NOREF(stack);
    LogFlowThisFunc(("returns %Rhrc\n", hrc));
    return hrc;
}

/**
 * Helper that converts VirtualSystem attachment values into VirtualBox attachment values.
 * Throws HRESULT values on errors!
 *
 * @param hdc in: the HardDiskController structure to attach to.
 * @param ulAddressOnParent in: the AddressOnParent parameter from OVF.
 * @param controllerName out: the name of the hard disk controller to attach to (e.g. "IDE").
 * @param lControllerPort out: the channel (controller port) of the controller to attach to.
 * @param lDevice out: the device number to attach to.
 */
void Appliance::i_convertDiskAttachmentValues(const ovf::HardDiskController &hdc,
                                              uint32_t ulAddressOnParent,
                                              Utf8Str &controllerName,
                                              int32_t &lControllerPort,
                                              int32_t &lDevice)
{
    Log(("Appliance::i_convertDiskAttachmentValues: hdc.system=%d, hdc.fPrimary=%d, ulAddressOnParent=%d\n",
         hdc.system,
         hdc.fPrimary,
         ulAddressOnParent));

    switch (hdc.system)
    {
        case ovf::HardDiskController::IDE:
            // For the IDE bus, the port parameter can be either 0 or 1, to specify the primary
            // or secondary IDE controller, respectively. For the primary controller of the IDE bus,
            // the device number can be either 0 or 1, to specify the master or the slave device,
            // respectively. For the secondary IDE controller, the device number is always 1 because
            // the master device is reserved for the CD-ROM drive.
            controllerName = "IDE";
            switch (ulAddressOnParent)
            {
                case 0: // master
                    if (!hdc.fPrimary)
                    {
                        // secondary master
                        lControllerPort = (long)1;
                        lDevice = (long)0;
                    }
                    else // primary master
                    {
                        lControllerPort = (long)0;
                        lDevice = (long)0;
                    }
                break;

                case 1: // slave
                    if (!hdc.fPrimary)
                    {
                        // secondary slave
                        lControllerPort = (long)1;
                        lDevice = (long)1;
                    }
                    else // primary slave
                    {
                        lControllerPort = (long)0;
                        lDevice = (long)1;
                    }
                break;

                // used by older VBox exports
                case 2:     // interpret this as secondary master
                    lControllerPort = (long)1;
                    lDevice = (long)0;
                break;

                // used by older VBox exports
                case 3:     // interpret this as secondary slave
                    lControllerPort = (long)1;
                    lDevice = (long)1;
                break;

                default:
                    throw setError(VBOX_E_NOT_SUPPORTED,
                                   tr("Invalid channel %RI16 specified; IDE controllers support only 0, 1 or 2"),
                                   ulAddressOnParent);
                break;
            }
        break;

        case ovf::HardDiskController::SATA:
            controllerName = "SATA";
            lControllerPort = (long)ulAddressOnParent;
            lDevice = (long)0;
            break;

        case ovf::HardDiskController::SCSI:
        {
            if(hdc.strControllerType.compare("lsilogicsas")==0)
                controllerName = "SAS";
            else
                controllerName = "SCSI";
            lControllerPort = (long)ulAddressOnParent;
            lDevice = (long)0;
            break;
        }

        default: break;
    }

    Log(("=> lControllerPort=%d, lDevice=%d\n", lControllerPort, lDevice));
}

/**
 * Imports one disk image.
 *
 * This is common code shared between
 *  --  i_importMachineGeneric() for the OVF case; in that case the information comes from
 *      the OVF virtual systems;
 *  --  i_importVBoxMachine(); in that case, the information comes from the <vbox:Machine>
 *      tag.
 *
 * Both ways of describing machines use the OVF disk references section, so in both cases
 * the caller needs to pass in the ovf::DiskImage structure from ovfreader.cpp.
 *
 * As a result, in both cases, if di.strHref is empty, we create a new disk as per the OVF
 * spec, even though this cannot really happen in the vbox:Machine case since such data
 * would never have been exported.
 *
 * This advances stack.pProgress by one operation with the disk's weight.
 *
 * @param di ovfreader.cpp structure describing the disk image from the OVF that is to be imported
 * @param strTargetPath Where to create the target image.
 * @param pTargetHD out: The newly created target disk. This also gets pushed on stack.llHardDisksCreated for cleanup.
 * @param stack
 */
void Appliance::i_importOneDiskImage(const ovf::DiskImage &di,
                                     Utf8Str *pStrDstPath,
                                     ComObjPtr<Medium> &pTargetHD,
                                     ImportStack &stack)
{
    ComObjPtr<Progress> pProgress;
    pProgress.createObject();
    HRESULT rc = pProgress->init(mVirtualBox,
                                 static_cast<IAppliance*>(this),
                                 BstrFmt(tr("Creating medium '%s'"),
                                 pStrDstPath->c_str()).raw(),
                                 TRUE);
    if (FAILED(rc)) throw rc;

    /* Get the system properties. */
    SystemProperties *pSysProps = mVirtualBox->i_getSystemProperties();

    /* Keep the source file ref handy for later. */
    const Utf8Str &strSourceOVF = di.strHref;

    /* Construct source file path */
    Utf8Str strSrcFilePath;
    if (stack.hVfsFssOva != NIL_RTVFSFSSTREAM)
        strSrcFilePath = strSourceOVF;
    else
    {
        strSrcFilePath = stack.strSourceDir;
        strSrcFilePath.append(RTPATH_SLASH_STR);
        strSrcFilePath.append(strSourceOVF);
    }

    /* First of all check if the path is an UUID. If so, the user like to
     * import the disk into an existing path. This is useful for iSCSI for
     * example. */
    RTUUID uuid;
    int vrc = RTUuidFromStr(&uuid, pStrDstPath->c_str());
    if (vrc == VINF_SUCCESS)
    {
        rc = mVirtualBox->i_findHardDiskById(Guid(uuid), true, &pTargetHD);
        if (FAILED(rc)) throw rc;
    }
    else
    {
        RTVFSIOSTREAM hVfsIosSrc = NIL_RTVFSIOSTREAM;

        /* check read file to GZIP compression */
        bool const fGzipped = di.strCompression.compare("gzip",Utf8Str::CaseInsensitive) == 0;
        Utf8Str strDeleteTemp;
        try
        {
            Utf8Str strTrgFormat = "VMDK";
            ComObjPtr<MediumFormat> trgFormat;
            Bstr bstrFormatName;
            ULONG lCabs = 0;

            char *pszSuff = RTPathSuffix(pStrDstPath->c_str());
            if (pszSuff != NULL)
            {
                /*
                 * Figure out which format the user like to have. Default is VMDK
                 * or it can be VDI if according command-line option is set
                 */

                /*
                 * We need a proper target format
                 * if target format has been changed by user via GUI import wizard
                 * or via VBoxManage import command (option --importtovdi)
                 * then we need properly process such format like ISO
                 * Because there is no conversion ISO to VDI
                 */
                trgFormat = pSysProps->i_mediumFormatFromExtension(++pszSuff);
                if (trgFormat.isNull())
                    throw setError(E_FAIL, tr("Unsupported medium format for disk image '%s'"), di.strHref.c_str());

                rc = trgFormat->COMGETTER(Name)(bstrFormatName.asOutParam());
                if (FAILED(rc)) throw rc;

                strTrgFormat = Utf8Str(bstrFormatName);

                if (   m->optListImport.contains(ImportOptions_ImportToVDI)
                    && strTrgFormat.compare("RAW", Utf8Str::CaseInsensitive) != 0)
                {
                    /* change the target extension */
                    strTrgFormat = "vdi";
                    trgFormat = pSysProps->i_mediumFormatFromExtension(strTrgFormat);
                    *pStrDstPath = pStrDstPath->stripSuffix();
                    *pStrDstPath = pStrDstPath->append(".");
                    *pStrDstPath = pStrDstPath->append(strTrgFormat.c_str());
                }

                /* Check the capabilities. We need create capabilities. */
                lCabs = 0;
                com::SafeArray <MediumFormatCapabilities_T> mediumFormatCap;
                rc = trgFormat->COMGETTER(Capabilities)(ComSafeArrayAsOutParam(mediumFormatCap));

                if (FAILED(rc))
                    throw rc;

                for (ULONG j = 0; j < mediumFormatCap.size(); j++)
                    lCabs |= mediumFormatCap[j];

                if (   !(lCabs & MediumFormatCapabilities_CreateFixed)
                    && !(lCabs & MediumFormatCapabilities_CreateDynamic) )
                    throw setError(VBOX_E_NOT_SUPPORTED,
                                   tr("Could not find a valid medium format for the target disk '%s'"),
                                   pStrDstPath->c_str());
            }
            else
            {
                throw setError(VBOX_E_FILE_ERROR,
                               tr("The target disk '%s' has no extension "),
                               pStrDstPath->c_str(), VERR_INVALID_NAME);
            }

            /* Create an IMedium object. */
            pTargetHD.createObject();

            /*CD/DVD case*/
            if (strTrgFormat.compare("RAW", Utf8Str::CaseInsensitive) == 0)
            {
                try
                {
                    if (fGzipped)
                        i_importDecompressFile(stack, strSrcFilePath, *pStrDstPath, strSourceOVF.c_str());
                    else
                        i_importCopyFile(stack, strSrcFilePath, *pStrDstPath, strSourceOVF.c_str());
                }
                catch (HRESULT /*arc*/)
                {
                    throw;
                }

                /* Advance to the next operation. */
                /* operation's weight, as set up with the IProgress originally */
                stack.pProgress->SetNextOperation(BstrFmt(tr("Importing virtual disk image '%s'"),
                                                  RTPathFilename(strSourceOVF.c_str())).raw(),
                                                  di.ulSuggestedSizeMB);
            }
            else/* HDD case*/
            {
                rc = pTargetHD->init(mVirtualBox,
                                     strTrgFormat,
                                     *pStrDstPath,
                                     Guid::Empty /* media registry: none yet */,
                                     DeviceType_HardDisk);
                if (FAILED(rc)) throw rc;

                /* Now create an empty hard disk. */
                rc = mVirtualBox->CreateMedium(Bstr(strTrgFormat).raw(),
                                               Bstr(*pStrDstPath).raw(),
                                               AccessMode_ReadWrite, DeviceType_HardDisk,
                                               ComPtr<IMedium>(pTargetHD).asOutParam());
                if (FAILED(rc)) throw rc;

                /* If strHref is empty we have to create a new file. */
                if (strSourceOVF.isEmpty())
                {
                    com::SafeArray<MediumVariant_T>  mediumVariant;
                    mediumVariant.push_back(MediumVariant_Standard);

                    /* Kick of the creation of a dynamic growing disk image with the given capacity. */
                    rc = pTargetHD->CreateBaseStorage(di.iCapacity / _1M,
                                                      ComSafeArrayAsInParam(mediumVariant),
                                                      ComPtr<IProgress>(pProgress).asOutParam());
                    if (FAILED(rc)) throw rc;

                    /* Advance to the next operation. */
                    /* operation's weight, as set up with the IProgress originally */
                    stack.pProgress->SetNextOperation(BstrFmt(tr("Creating disk image '%s'"),
                                                      pStrDstPath->c_str()).raw(),
                                                      di.ulSuggestedSizeMB);
                }
                else
                {
                    /* We need a proper source format description */
                    /* Which format to use? */
                    ComObjPtr<MediumFormat> srcFormat;
                    rc = i_findMediumFormatFromDiskImage(di, srcFormat);
                    if (FAILED(rc))
                        throw setError(VBOX_E_NOT_SUPPORTED,
                                       tr("Could not find a valid medium format for the source disk '%s' "
                                          "Check correctness of the image format URL in the OVF description file "
                                          "or extension of the image"),
                                       RTPathFilename(strSourceOVF.c_str()));

                    /* If gzipped, decompress the GZIP file and save a new file in the target path */
                    if (fGzipped)
                    {
                        Utf8Str strTargetFilePath(*pStrDstPath);
                        strTargetFilePath.stripFilename();
                        strTargetFilePath.append(RTPATH_SLASH_STR);
                        strTargetFilePath.append("temp_");
                        strTargetFilePath.append(RTPathFilename(strSrcFilePath.c_str()));
                        strDeleteTemp = strTargetFilePath;

                        i_importDecompressFile(stack, strSrcFilePath, strTargetFilePath, strSourceOVF.c_str());

                        /* Correct the source and the target with the actual values */
                        strSrcFilePath = strTargetFilePath;

                        /* Open the new source file. */
                        vrc = RTVfsIoStrmOpenNormal(strSrcFilePath.c_str(), RTFILE_O_READ | RTFILE_O_DENY_NONE | RTFILE_O_OPEN,
                                                    &hVfsIosSrc);
                        if (RT_FAILURE(vrc))
                            throw setErrorVrc(vrc, tr("Error opening decompressed image file '%s' (%Rrc)"),
                                              strSrcFilePath.c_str(), vrc);
                    }
                    else
                        hVfsIosSrc = i_importOpenSourceFile(stack, strSrcFilePath, strSourceOVF.c_str());

                    /* Add a read ahead thread to try speed things up with concurrent reads and
                       writes going on in different threads. */
                    RTVFSIOSTREAM hVfsIosReadAhead;
                    vrc = RTVfsCreateReadAheadForIoStream(hVfsIosSrc, 0 /*fFlags*/, 0 /*cBuffers=default*/,
                                                          0 /*cbBuffers=default*/, &hVfsIosReadAhead);
                    RTVfsIoStrmRelease(hVfsIosSrc);
                    if (RT_FAILURE(vrc))
                        throw setErrorVrc(vrc, tr("Error initializing read ahead thread for '%s' (%Rrc)"),
                                          strSrcFilePath.c_str(), vrc);

                    /* Start the source image cloning operation. */
                    ComObjPtr<Medium> nullParent;
                    rc = pTargetHD->i_importFile(strSrcFilePath.c_str(),
                                                 srcFormat,
                                                 MediumVariant_Standard,
                                                 hVfsIosReadAhead,
                                                 nullParent,
                                                 pProgress);
                    RTVfsIoStrmRelease(hVfsIosReadAhead);
                    hVfsIosSrc = NIL_RTVFSIOSTREAM;
                    if (FAILED(rc))
                        throw rc;

                    /* Advance to the next operation. */
                    /* operation's weight, as set up with the IProgress originally */
                    stack.pProgress->SetNextOperation(BstrFmt(tr("Importing virtual disk image '%s'"),
                                                      RTPathFilename(strSourceOVF.c_str())).raw(),
                                                      di.ulSuggestedSizeMB);
                }

                /* Now wait for the background disk operation to complete; this throws
                 * HRESULTs on error. */
                ComPtr<IProgress> pp(pProgress);
                i_waitForAsyncProgress(stack.pProgress, pp);
            }
        }
        catch (...)
        {
            if (strDeleteTemp.isNotEmpty())
                RTFileDelete(strDeleteTemp.c_str());
            throw;
        }

        /* Make sure the source file is closed. */
        if (hVfsIosSrc != NIL_RTVFSIOSTREAM)
            RTVfsIoStrmRelease(hVfsIosSrc);

        /*
         * Delete the temp gunzip result, if any.
         */
        if (strDeleteTemp.isNotEmpty())
        {
            vrc = RTFileDelete(strSrcFilePath.c_str());
            if (RT_FAILURE(vrc))
                setWarning(VBOX_E_FILE_ERROR,
                           tr("Failed to delete the temporary file '%s' (%Rrc)"), strSrcFilePath.c_str(), vrc);
        }
    }
}

/**
 * Imports one OVF virtual system (described by the given ovf::VirtualSystem and VirtualSystemDescription)
 * into VirtualBox by creating an IMachine instance, which is returned.
 *
 * This throws HRESULT error codes for anything that goes wrong, in which case the caller must clean
 * up any leftovers from this function. For this, the given ImportStack instance has received information
 * about what needs cleaning up (to support rollback).
 *
 * @param vsysThis OVF virtual system (machine) to import.
 * @param vsdescThis  Matching virtual system description (machine) to import.
 * @param pNewMachine out: Newly created machine.
 * @param stack Cleanup stack for when this throws.
 */
void Appliance::i_importMachineGeneric(const ovf::VirtualSystem &vsysThis,
                                       ComObjPtr<VirtualSystemDescription> &vsdescThis,
                                       ComPtr<IMachine> &pNewMachine,
                                       ImportStack &stack)
{
    LogFlowFuncEnter();
    HRESULT rc;

    // Get the instance of IGuestOSType which matches our string guest OS type so we
    // can use recommended defaults for the new machine where OVF doesn't provide any
    ComPtr<IGuestOSType> osType;
    rc = mVirtualBox->GetGuestOSType(Bstr(stack.strOsTypeVBox).raw(), osType.asOutParam());
    if (FAILED(rc)) throw rc;

    /* Create the machine */
    SafeArray<BSTR> groups; /* no groups */
    rc = mVirtualBox->CreateMachine(NULL, /* machine name: use default */
                                    Bstr(stack.strNameVBox).raw(),
                                    ComSafeArrayAsInParam(groups),
                                    Bstr(stack.strOsTypeVBox).raw(),
                                    NULL, /* aCreateFlags */
                                    pNewMachine.asOutParam());
    if (FAILED(rc)) throw rc;

    // set the description
    if (!stack.strDescription.isEmpty())
    {
        rc = pNewMachine->COMSETTER(Description)(Bstr(stack.strDescription).raw());
        if (FAILED(rc)) throw rc;
    }

    // CPU count
    rc = pNewMachine->COMSETTER(CPUCount)(stack.cCPUs);
    if (FAILED(rc)) throw rc;

    if (stack.fForceHWVirt)
    {
        rc = pNewMachine->SetHWVirtExProperty(HWVirtExPropertyType_Enabled, TRUE);
        if (FAILED(rc)) throw rc;
    }

    // RAM
    rc = pNewMachine->COMSETTER(MemorySize)(stack.ulMemorySizeMB);
    if (FAILED(rc)) throw rc;

    /* VRAM */
    /* Get the recommended VRAM for this guest OS type */
    ULONG vramVBox;
    rc = osType->COMGETTER(RecommendedVRAM)(&vramVBox);
    if (FAILED(rc)) throw rc;

    /* Set the VRAM */
    rc = pNewMachine->COMSETTER(VRAMSize)(vramVBox);
    if (FAILED(rc)) throw rc;

    // I/O APIC: Generic OVF has no setting for this. Enable it if we
    // import a Windows VM because if if Windows was installed without IOAPIC,
    // it will not mind finding an one later on, but if Windows was installed
    // _with_ an IOAPIC, it will bluescreen if it's not found
    if (!stack.fForceIOAPIC)
    {
        Bstr bstrFamilyId;
        rc = osType->COMGETTER(FamilyId)(bstrFamilyId.asOutParam());
        if (FAILED(rc)) throw rc;
        if (bstrFamilyId == "Windows")
            stack.fForceIOAPIC = true;
    }

    if (stack.fForceIOAPIC)
    {
        ComPtr<IBIOSSettings> pBIOSSettings;
        rc = pNewMachine->COMGETTER(BIOSSettings)(pBIOSSettings.asOutParam());
        if (FAILED(rc)) throw rc;

        rc = pBIOSSettings->COMSETTER(IOAPICEnabled)(TRUE);
        if (FAILED(rc)) throw rc;
    }

    if (!stack.strAudioAdapter.isEmpty())
        if (stack.strAudioAdapter.compare("null", Utf8Str::CaseInsensitive) != 0)
        {
            uint32_t audio = RTStrToUInt32(stack.strAudioAdapter.c_str());       // should be 0 for AC97
            ComPtr<IAudioAdapter> audioAdapter;
            rc = pNewMachine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
            if (FAILED(rc)) throw rc;
            rc = audioAdapter->COMSETTER(Enabled)(true);
            if (FAILED(rc)) throw rc;
            rc = audioAdapter->COMSETTER(AudioController)(static_cast<AudioControllerType_T>(audio));
            if (FAILED(rc)) throw rc;
        }

#ifdef VBOX_WITH_USB
    /* USB Controller */
    if (stack.fUSBEnabled)
    {
        ComPtr<IUSBController> usbController;
        rc = pNewMachine->AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI, usbController.asOutParam());
        if (FAILED(rc)) throw rc;
    }
#endif /* VBOX_WITH_USB */

    /* Change the network adapters */
    uint32_t maxNetworkAdapters = Global::getMaxNetworkAdapters(ChipsetType_PIIX3);

    std::list<VirtualSystemDescriptionEntry*> vsdeNW = vsdescThis->i_findByType(VirtualSystemDescriptionType_NetworkAdapter);
    if (vsdeNW.empty())
    {
        /* No network adapters, so we have to disable our default one */
        ComPtr<INetworkAdapter> nwVBox;
        rc = pNewMachine->GetNetworkAdapter(0, nwVBox.asOutParam());
        if (FAILED(rc)) throw rc;
        rc = nwVBox->COMSETTER(Enabled)(false);
        if (FAILED(rc)) throw rc;
    }
    else if (vsdeNW.size() > maxNetworkAdapters)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many network adapters: OVF requests %d network adapters, "
                          "but VirtualBox only supports %d"),
                       vsdeNW.size(), maxNetworkAdapters);
    else
    {
        list<VirtualSystemDescriptionEntry*>::const_iterator nwIt;
        size_t a = 0;
        for (nwIt = vsdeNW.begin();
             nwIt != vsdeNW.end();
             ++nwIt, ++a)
        {
            const VirtualSystemDescriptionEntry* pvsys = *nwIt;

            const Utf8Str &nwTypeVBox = pvsys->strVBoxCurrent;
            uint32_t tt1 = RTStrToUInt32(nwTypeVBox.c_str());
            ComPtr<INetworkAdapter> pNetworkAdapter;
            rc = pNewMachine->GetNetworkAdapter((ULONG)a, pNetworkAdapter.asOutParam());
            if (FAILED(rc)) throw rc;
            /* Enable the network card & set the adapter type */
            rc = pNetworkAdapter->COMSETTER(Enabled)(true);
            if (FAILED(rc)) throw rc;
            rc = pNetworkAdapter->COMSETTER(AdapterType)(static_cast<NetworkAdapterType_T>(tt1));
            if (FAILED(rc)) throw rc;

            // default is NAT; change to "bridged" if extra conf says so
            if (pvsys->strExtraConfigCurrent.endsWith("type=Bridged", Utf8Str::CaseInsensitive))
            {
                /* Attach to the right interface */
                rc = pNetworkAdapter->COMSETTER(AttachmentType)(NetworkAttachmentType_Bridged);
                if (FAILED(rc)) throw rc;
                ComPtr<IHost> host;
                rc = mVirtualBox->COMGETTER(Host)(host.asOutParam());
                if (FAILED(rc)) throw rc;
                com::SafeIfaceArray<IHostNetworkInterface> nwInterfaces;
                rc = host->COMGETTER(NetworkInterfaces)(ComSafeArrayAsOutParam(nwInterfaces));
                if (FAILED(rc)) throw rc;
                // We search for the first host network interface which
                // is usable for bridged networking
                for (size_t j = 0;
                     j < nwInterfaces.size();
                     ++j)
                {
                    HostNetworkInterfaceType_T itype;
                    rc = nwInterfaces[j]->COMGETTER(InterfaceType)(&itype);
                    if (FAILED(rc)) throw rc;
                    if (itype == HostNetworkInterfaceType_Bridged)
                    {
                        Bstr name;
                        rc = nwInterfaces[j]->COMGETTER(Name)(name.asOutParam());
                        if (FAILED(rc)) throw rc;
                        /* Set the interface name to attach to */
                        rc = pNetworkAdapter->COMSETTER(BridgedInterface)(name.raw());
                        if (FAILED(rc)) throw rc;
                        break;
                    }
                }
            }
            /* Next test for host only interfaces */
            else if (pvsys->strExtraConfigCurrent.endsWith("type=HostOnly", Utf8Str::CaseInsensitive))
            {
                /* Attach to the right interface */
                rc = pNetworkAdapter->COMSETTER(AttachmentType)(NetworkAttachmentType_HostOnly);
                if (FAILED(rc)) throw rc;
                ComPtr<IHost> host;
                rc = mVirtualBox->COMGETTER(Host)(host.asOutParam());
                if (FAILED(rc)) throw rc;
                com::SafeIfaceArray<IHostNetworkInterface> nwInterfaces;
                rc = host->COMGETTER(NetworkInterfaces)(ComSafeArrayAsOutParam(nwInterfaces));
                if (FAILED(rc)) throw rc;
                // We search for the first host network interface which
                // is usable for host only networking
                for (size_t j = 0;
                     j < nwInterfaces.size();
                     ++j)
                {
                    HostNetworkInterfaceType_T itype;
                    rc = nwInterfaces[j]->COMGETTER(InterfaceType)(&itype);
                    if (FAILED(rc)) throw rc;
                    if (itype == HostNetworkInterfaceType_HostOnly)
                    {
                        Bstr name;
                        rc = nwInterfaces[j]->COMGETTER(Name)(name.asOutParam());
                        if (FAILED(rc)) throw rc;
                        /* Set the interface name to attach to */
                        rc = pNetworkAdapter->COMSETTER(HostOnlyInterface)(name.raw());
                        if (FAILED(rc)) throw rc;
                        break;
                    }
                }
            }
            /* Next test for internal interfaces */
            else if (pvsys->strExtraConfigCurrent.endsWith("type=Internal", Utf8Str::CaseInsensitive))
            {
                /* Attach to the right interface */
                rc = pNetworkAdapter->COMSETTER(AttachmentType)(NetworkAttachmentType_Internal);
                if (FAILED(rc)) throw rc;
            }
            /* Next test for Generic interfaces */
            else if (pvsys->strExtraConfigCurrent.endsWith("type=Generic", Utf8Str::CaseInsensitive))
            {
                /* Attach to the right interface */
                rc = pNetworkAdapter->COMSETTER(AttachmentType)(NetworkAttachmentType_Generic);
                if (FAILED(rc)) throw rc;
            }

            /* Next test for NAT network interfaces */
            else if (pvsys->strExtraConfigCurrent.endsWith("type=NATNetwork", Utf8Str::CaseInsensitive))
            {
                /* Attach to the right interface */
                rc = pNetworkAdapter->COMSETTER(AttachmentType)(NetworkAttachmentType_NATNetwork);
                if (FAILED(rc)) throw rc;
                com::SafeIfaceArray<INATNetwork> nwNATNetworks;
                rc = mVirtualBox->COMGETTER(NATNetworks)(ComSafeArrayAsOutParam(nwNATNetworks));
                if (FAILED(rc)) throw rc;
                // Pick the first NAT network (if there is any)
                if (nwNATNetworks.size())
                {
                    Bstr name;
                    rc = nwNATNetworks[0]->COMGETTER(NetworkName)(name.asOutParam());
                    if (FAILED(rc)) throw rc;
                    /* Set the NAT network name to attach to */
                    rc = pNetworkAdapter->COMSETTER(NATNetwork)(name.raw());
                    if (FAILED(rc)) throw rc;
                    break;
                }
            }
        }
    }

    // IDE Hard disk controller
    std::list<VirtualSystemDescriptionEntry*> vsdeHDCIDE =
        vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskControllerIDE);
    /*
     * In OVF (at least VMware's version of it), an IDE controller has two ports,
     * so VirtualBox's single IDE controller with two channels and two ports each counts as
     * two OVF IDE controllers -- so we accept one or two such IDE controllers
     */
    size_t cIDEControllers = vsdeHDCIDE.size();
    if (cIDEControllers > 2)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many IDE controllers in OVF; import facility only supports two"));
    if (!vsdeHDCIDE.empty())
    {
        // one or two IDE controllers present in OVF: add one VirtualBox controller
        ComPtr<IStorageController> pController;
        rc = pNewMachine->AddStorageController(Bstr("IDE").raw(), StorageBus_IDE, pController.asOutParam());
        if (FAILED(rc)) throw rc;

        const char *pcszIDEType = vsdeHDCIDE.front()->strVBoxCurrent.c_str();
        if (!strcmp(pcszIDEType, "PIIX3"))
            rc = pController->COMSETTER(ControllerType)(StorageControllerType_PIIX3);
        else if (!strcmp(pcszIDEType, "PIIX4"))
            rc = pController->COMSETTER(ControllerType)(StorageControllerType_PIIX4);
        else if (!strcmp(pcszIDEType, "ICH6"))
            rc = pController->COMSETTER(ControllerType)(StorageControllerType_ICH6);
        else
            throw setError(VBOX_E_FILE_ERROR,
                           tr("Invalid IDE controller type \"%s\""),
                           pcszIDEType);
        if (FAILED(rc)) throw rc;
    }

    /* Hard disk controller SATA */
    std::list<VirtualSystemDescriptionEntry*> vsdeHDCSATA =
        vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskControllerSATA);
    if (vsdeHDCSATA.size() > 1)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many SATA controllers in OVF; import facility only supports one"));
    if (!vsdeHDCSATA.empty())
    {
        ComPtr<IStorageController> pController;
        const Utf8Str &hdcVBox = vsdeHDCSATA.front()->strVBoxCurrent;
        if (hdcVBox == "AHCI")
        {
            rc = pNewMachine->AddStorageController(Bstr("SATA").raw(),
                                                   StorageBus_SATA,
                                                   pController.asOutParam());
            if (FAILED(rc)) throw rc;
        }
        else
            throw setError(VBOX_E_FILE_ERROR,
                           tr("Invalid SATA controller type \"%s\""),
                           hdcVBox.c_str());
    }

    /* Hard disk controller SCSI */
    std::list<VirtualSystemDescriptionEntry*> vsdeHDCSCSI =
        vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskControllerSCSI);
    if (vsdeHDCSCSI.size() > 1)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many SCSI controllers in OVF; import facility only supports one"));
    if (!vsdeHDCSCSI.empty())
    {
        ComPtr<IStorageController> pController;
        Utf8Str strName("SCSI");
        StorageBus_T busType = StorageBus_SCSI;
        StorageControllerType_T controllerType;
        const Utf8Str &hdcVBox = vsdeHDCSCSI.front()->strVBoxCurrent;
        if (hdcVBox == "LsiLogic")
            controllerType = StorageControllerType_LsiLogic;
        else if (hdcVBox == "LsiLogicSas")
        {
            // OVF treats LsiLogicSas as a SCSI controller but VBox considers it a class of its own
            strName = "SAS";
            busType = StorageBus_SAS;
            controllerType = StorageControllerType_LsiLogicSas;
        }
        else if (hdcVBox == "BusLogic")
            controllerType = StorageControllerType_BusLogic;
        else
            throw setError(VBOX_E_FILE_ERROR,
                           tr("Invalid SCSI controller type \"%s\""),
                           hdcVBox.c_str());

        rc = pNewMachine->AddStorageController(Bstr(strName).raw(), busType, pController.asOutParam());
        if (FAILED(rc)) throw rc;
        rc = pController->COMSETTER(ControllerType)(controllerType);
        if (FAILED(rc)) throw rc;
    }

    /* Hard disk controller SAS */
    std::list<VirtualSystemDescriptionEntry*> vsdeHDCSAS =
        vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskControllerSAS);
    if (vsdeHDCSAS.size() > 1)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many SAS controllers in OVF; import facility only supports one"));
    if (!vsdeHDCSAS.empty())
    {
        ComPtr<IStorageController> pController;
        rc = pNewMachine->AddStorageController(Bstr(L"SAS").raw(),
                                               StorageBus_SAS,
                                               pController.asOutParam());
        if (FAILED(rc)) throw rc;
        rc = pController->COMSETTER(ControllerType)(StorageControllerType_LsiLogicSas);
        if (FAILED(rc)) throw rc;
    }

    /* Now its time to register the machine before we add any hard disks */
    rc = mVirtualBox->RegisterMachine(pNewMachine);
    if (FAILED(rc)) throw rc;

    // store new machine for roll-back in case of errors
    Bstr bstrNewMachineId;
    rc = pNewMachine->COMGETTER(Id)(bstrNewMachineId.asOutParam());
    if (FAILED(rc)) throw rc;
    Guid uuidNewMachine(bstrNewMachineId);
    m->llGuidsMachinesCreated.push_back(uuidNewMachine);

    // Add floppies and CD-ROMs to the appropriate controllers.
    std::list<VirtualSystemDescriptionEntry*> vsdeFloppy = vsdescThis->i_findByType(VirtualSystemDescriptionType_Floppy);
    if (vsdeFloppy.size() > 1)
        throw setError(VBOX_E_FILE_ERROR,
                       tr("Too many floppy controllers in OVF; import facility only supports one"));
    std::list<VirtualSystemDescriptionEntry*> vsdeCDROM = vsdescThis->i_findByType(VirtualSystemDescriptionType_CDROM);
    if (    !vsdeFloppy.empty()
         || !vsdeCDROM.empty()
       )
    {
        // If there's an error here we need to close the session, so
        // we need another try/catch block.

        try
        {
            // to attach things we need to open a session for the new machine
            rc = pNewMachine->LockMachine(stack.pSession, LockType_Write);
            if (FAILED(rc)) throw rc;
            stack.fSessionOpen = true;

            ComPtr<IMachine> sMachine;
            rc = stack.pSession->COMGETTER(Machine)(sMachine.asOutParam());
            if (FAILED(rc)) throw rc;

            // floppy first
            if (vsdeFloppy.size() == 1)
            {
                ComPtr<IStorageController> pController;
                rc = sMachine->AddStorageController(Bstr("Floppy").raw(),
                                                    StorageBus_Floppy,
                                                    pController.asOutParam());
                if (FAILED(rc)) throw rc;

                Bstr bstrName;
                rc = pController->COMGETTER(Name)(bstrName.asOutParam());
                if (FAILED(rc)) throw rc;

                // this is for rollback later
                MyHardDiskAttachment mhda;
                mhda.pMachine = pNewMachine;
                mhda.controllerName = bstrName;
                mhda.lControllerPort = 0;
                mhda.lDevice = 0;

                Log(("Attaching floppy\n"));

                rc = sMachine->AttachDevice(Bstr(mhda.controllerName).raw(),
                                            mhda.lControllerPort,
                                            mhda.lDevice,
                                            DeviceType_Floppy,
                                            NULL);
                if (FAILED(rc)) throw rc;

                stack.llHardDiskAttachments.push_back(mhda);
            }

            rc = sMachine->SaveSettings();
            if (FAILED(rc)) throw rc;

            // only now that we're done with all disks, close the session
            rc = stack.pSession->UnlockMachine();
            if (FAILED(rc)) throw rc;
            stack.fSessionOpen = false;
        }
        catch(HRESULT aRC)
        {
            com::ErrorInfo info;

            if (stack.fSessionOpen)
                stack.pSession->UnlockMachine();

            if (info.isFullAvailable())
                throw setError(aRC, Utf8Str(info.getText()).c_str());
            else
                throw setError(aRC, "Unknown error during OVF import");
        }
    }

    // create the hard disks & connect them to the appropriate controllers
    std::list<VirtualSystemDescriptionEntry*> avsdeHDs = vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskImage);
    if (!avsdeHDs.empty())
    {
        // If there's an error here we need to close the session, so
        // we need another try/catch block.
        try
        {
#ifdef LOG_ENABLED
            if (LogIsEnabled())
            {
                size_t i = 0;
                for (list<VirtualSystemDescriptionEntry*>::const_iterator itHD = avsdeHDs.begin();
                     itHD != avsdeHDs.end(); ++itHD, i++)
                     Log(("avsdeHDs[%zu]: strRef=%s strOvf=%s\n", i, (*itHD)->strRef.c_str(), (*itHD)->strOvf.c_str()));
                i = 0;
                for (ovf::DiskImagesMap::const_iterator itDisk = stack.mapDisks.begin(); itDisk != stack.mapDisks.end(); ++itDisk)
                    Log(("mapDisks[%zu]: strDiskId=%s strHref=%s\n",
                         i, itDisk->second.strDiskId.c_str(), itDisk->second.strHref.c_str()));

            }
#endif

            // to attach things we need to open a session for the new machine
            rc = pNewMachine->LockMachine(stack.pSession, LockType_Write);
            if (FAILED(rc)) throw rc;
            stack.fSessionOpen = true;

            /* get VM name from virtual system description. Only one record is possible (size of list is equal 1). */
            std::list<VirtualSystemDescriptionEntry*> vmName = vsdescThis->i_findByType(VirtualSystemDescriptionType_Name);
            std::list<VirtualSystemDescriptionEntry*>::iterator vmNameIt = vmName.begin();
            VirtualSystemDescriptionEntry* vmNameEntry = *vmNameIt;


            ovf::DiskImagesMap::const_iterator oit = stack.mapDisks.begin();
            std::set<RTCString>  disksResolvedNames;

            uint32_t cImportedDisks = 0;

            while (oit != stack.mapDisks.end() && cImportedDisks != avsdeHDs.size())
            {
/** @todo r=bird: Most of the code here is duplicated in the other machine
 *        import method, factor out. */
                ovf::DiskImage diCurrent = oit->second;

                Log(("diCurrent.strDiskId=%s diCurrent.strHref=%s\n", diCurrent.strDiskId.c_str(), diCurrent.strHref.c_str()));
                /* Iterate over all given disk images of the virtual system
                 * disks description. We need to find the target disk path,
                 * which could be changed by the user. */
                VirtualSystemDescriptionEntry *vsdeTargetHD = NULL;
                for (list<VirtualSystemDescriptionEntry*>::const_iterator itHD = avsdeHDs.begin();
                     itHD != avsdeHDs.end();
                     ++itHD)
                {
                    VirtualSystemDescriptionEntry *vsdeHD = *itHD;
                    if (vsdeHD->strRef == diCurrent.strDiskId)
                    {
                        vsdeTargetHD = vsdeHD;
                        break;
                    }
                }
                if (!vsdeTargetHD)
                {
                    /* possible case if a disk image belongs to other virtual system (OVF package with multiple VMs inside) */
                    Log1Warning(("OVA/OVF import: Disk image %s was missed during import of VM %s\n",
                                 oit->first.c_str(), vmNameEntry->strOvf.c_str()));
                    NOREF(vmNameEntry);
                    ++oit;
                    continue;
                }

                //diCurrent.strDiskId contains the disk identifier (e.g. "vmdisk1"), which should exist
                //in the virtual system's disks map under that ID and also in the global images map
                ovf::VirtualDisksMap::const_iterator itVDisk = vsysThis.mapVirtualDisks.find(diCurrent.strDiskId);
                if (itVDisk == vsysThis.mapVirtualDisks.end())
                    throw setError(E_FAIL,
                                   tr("Internal inconsistency looking up disk image '%s'"),
                                   diCurrent.strHref.c_str());

                /*
                 * preliminary check availability of the image
                 * This step is useful if image is placed in the OVA (TAR) package
                 */
                if (stack.hVfsFssOva != NIL_RTVFSFSSTREAM)
                {
                    /* It means that we possibly have imported the storage earlier on the previous loop steps*/
                    std::set<RTCString>::const_iterator h = disksResolvedNames.find(diCurrent.strHref);
                    if (h != disksResolvedNames.end())
                    {
                        /* Yes, disk name was found, we can skip it*/
                        ++oit;
                        continue;
                    }
l_skipped:
                    rc = i_preCheckImageAvailability(stack);
                    if (SUCCEEDED(rc))
                    {
                        /* current opened file isn't the same as passed one */
                        if (RTStrICmp(diCurrent.strHref.c_str(), stack.pszOvaLookAheadName) != 0)
                        {
                            /* availableImage contains the disk file reference (e.g. "disk1.vmdk"), which should
                             * exist in the global images map.
                             * And find the disk from the OVF's disk list */
                            ovf::DiskImagesMap::const_iterator itDiskImage;
                            for (itDiskImage = stack.mapDisks.begin();
                                 itDiskImage != stack.mapDisks.end();
                                 itDiskImage++)
                                if (itDiskImage->second.strHref.compare(stack.pszOvaLookAheadName,
                                                                        Utf8Str::CaseInsensitive) == 0)
                                    break;
                            if (itDiskImage == stack.mapDisks.end())
                            {
                                LogFunc(("Skipping '%s'\n", stack.pszOvaLookAheadName));
                                RTVfsIoStrmRelease(stack.claimOvaLookAHead());
                                goto l_skipped;
                            }

                            /* replace with a new found disk image */
                            diCurrent = *(&itDiskImage->second);

                            /*
                             * Again iterate over all given disk images of the virtual system
                             * disks description using the found disk image
                             */
                            for (list<VirtualSystemDescriptionEntry*>::const_iterator itHD = avsdeHDs.begin();
                                 itHD != avsdeHDs.end();
                                 ++itHD)
                            {
                                VirtualSystemDescriptionEntry *vsdeHD = *itHD;
                                if (vsdeHD->strRef == diCurrent.strDiskId)
                                {
                                    vsdeTargetHD = vsdeHD;
                                    break;
                                }
                            }

                            /*
                             * in this case it's an error because something is wrong with the OVF description file.
                             * May be VBox imports OVA package with wrong file sequence inside the archive.
                             */
                            if (!vsdeTargetHD)
                                throw setError(E_FAIL,
                                               tr("Internal inconsistency looking up disk image '%s'"),
                                               diCurrent.strHref.c_str());

                            itVDisk = vsysThis.mapVirtualDisks.find(diCurrent.strDiskId);
                            if (itVDisk == vsysThis.mapVirtualDisks.end())
                                throw setError(E_FAIL,
                                               tr("Internal inconsistency looking up disk image '%s'"),
                                               diCurrent.strHref.c_str());
                        }
                        else
                        {
                            ++oit;
                        }
                    }
                    else
                    {
                        ++oit;
                        continue;
                    }
                }
                else
                {
                    /* just continue with normal files*/
                    ++oit;
                }

                /* very important to store disk name for the next checks */
                disksResolvedNames.insert(diCurrent.strHref);
////// end of duplicated code.
                const ovf::VirtualDisk &ovfVdisk = itVDisk->second;

                ComObjPtr<Medium> pTargetHD;

                Utf8Str savedVBoxCurrent = vsdeTargetHD->strVBoxCurrent;

                i_importOneDiskImage(diCurrent,
                                     &vsdeTargetHD->strVBoxCurrent,
                                     pTargetHD,
                                     stack);

                // now use the new uuid to attach the disk image to our new machine
                ComPtr<IMachine> sMachine;
                rc = stack.pSession->COMGETTER(Machine)(sMachine.asOutParam());
                if (FAILED(rc))
                    throw rc;

                // find the hard disk controller to which we should attach
                ovf::HardDiskController hdc = (*vsysThis.mapControllers.find(ovfVdisk.idController)).second;

                // this is for rollback later
                MyHardDiskAttachment mhda;
                mhda.pMachine = pNewMachine;

                i_convertDiskAttachmentValues(hdc,
                                              ovfVdisk.ulAddressOnParent,
                                              mhda.controllerName,
                                              mhda.lControllerPort,
                                              mhda.lDevice);

                Log(("Attaching disk %s to port %d on device %d\n",
                     vsdeTargetHD->strVBoxCurrent.c_str(), mhda.lControllerPort, mhda.lDevice));

                ComObjPtr<MediumFormat> mediumFormat;
                rc = i_findMediumFormatFromDiskImage(diCurrent, mediumFormat);
                if (FAILED(rc))
                    throw rc;

                Bstr bstrFormatName;
                rc = mediumFormat->COMGETTER(Name)(bstrFormatName.asOutParam());
                if (FAILED(rc))
                    throw rc;

                Utf8Str vdf = Utf8Str(bstrFormatName);

                if (vdf.compare("RAW", Utf8Str::CaseInsensitive) == 0)
                {
                    ComPtr<IMedium> dvdImage(pTargetHD);

                    rc = mVirtualBox->OpenMedium(Bstr(vsdeTargetHD->strVBoxCurrent).raw(),
                                                 DeviceType_DVD,
                                                 AccessMode_ReadWrite,
                                                 false,
                                                 dvdImage.asOutParam());

                    if (FAILED(rc))
                        throw rc;

                    rc = sMachine->AttachDevice(Bstr(mhda.controllerName).raw(),// name
                                                mhda.lControllerPort,     // long controllerPort
                                                mhda.lDevice,             // long device
                                                DeviceType_DVD,           // DeviceType_T type
                                                dvdImage);
                    if (FAILED(rc))
                        throw rc;
                }
                else
                {
                    rc = sMachine->AttachDevice(Bstr(mhda.controllerName).raw(),// name
                                                mhda.lControllerPort,     // long controllerPort
                                                mhda.lDevice,             // long device
                                                DeviceType_HardDisk,      // DeviceType_T type
                                                pTargetHD);

                    if (FAILED(rc))
                        throw rc;
                }

                stack.llHardDiskAttachments.push_back(mhda);

                rc = sMachine->SaveSettings();
                if (FAILED(rc))
                    throw rc;

                /* restore */
                vsdeTargetHD->strVBoxCurrent = savedVBoxCurrent;

                ++cImportedDisks;

            } // end while(oit != stack.mapDisks.end())

            /*
             * quantity of the imported disks isn't equal to the size of the avsdeHDs list.
             */
            if(cImportedDisks < avsdeHDs.size())
            {
                Log1Warning(("Not all disk images were imported for VM %s. Check OVF description file.",
                             vmNameEntry->strOvf.c_str()));
            }

            // only now that we're done with all disks, close the session
            rc = stack.pSession->UnlockMachine();
            if (FAILED(rc))
                throw rc;
            stack.fSessionOpen = false;
        }
        catch(HRESULT aRC)
        {
            com::ErrorInfo info;
            if (stack.fSessionOpen)
                stack.pSession->UnlockMachine();

            if (info.isFullAvailable())
                throw setError(aRC, Utf8Str(info.getText()).c_str());
            else
                throw setError(aRC, "Unknown error during OVF import");
        }
    }
    LogFlowFuncLeave();
}

/**
 * Imports one OVF virtual system (described by a vbox:Machine tag represented by the given config
 * structure) into VirtualBox by creating an IMachine instance, which is returned.
 *
 * This throws HRESULT error codes for anything that goes wrong, in which case the caller must clean
 * up any leftovers from this function. For this, the given ImportStack instance has received information
 * about what needs cleaning up (to support rollback).
 *
 * The machine config stored in the settings::MachineConfigFile structure contains the UUIDs of
 * the disk attachments used by the machine when it was exported. We also add vbox:uuid attributes
 * to the OVF disks sections so we can look them up. While importing these UUIDs into a second host
 * will most probably work, reimporting them into the same host will cause conflicts, so we always
 * generate new ones on import. This involves the following:
 *
 *  1)  Scan the machine config for disk attachments.
 *
 *  2)  For each disk attachment found, look up the OVF disk image from the disk references section
 *      and import the disk into VirtualBox, which creates a new UUID for it. In the machine config,
 *      replace the old UUID with the new one.
 *
 *  3)  Change the machine config according to the OVF virtual system descriptions, in case the
 *      caller has modified them using setFinalValues().
 *
 *  4)  Create the VirtualBox machine with the modfified machine config.
 *
 * @param   vsdescThis
 * @param   pReturnNewMachine
 * @param   stack
 */
void Appliance::i_importVBoxMachine(ComObjPtr<VirtualSystemDescription> &vsdescThis,
                                    ComPtr<IMachine> &pReturnNewMachine,
                                    ImportStack &stack)
{
    LogFlowFuncEnter();
    Assert(vsdescThis->m->pConfig);

    HRESULT rc = S_OK;

    settings::MachineConfigFile &config = *vsdescThis->m->pConfig;

    /*
     * step 1): modify machine config according to OVF config, in case the user
     * has modified them using setFinalValues()
     */

    /* OS Type */
    config.machineUserData.strOsType = stack.strOsTypeVBox;
    /* Description */
    config.machineUserData.strDescription = stack.strDescription;
    /* CPU count & extented attributes */
    config.hardwareMachine.cCPUs = stack.cCPUs;
    if (stack.fForceIOAPIC)
        config.hardwareMachine.fHardwareVirt = true;
    if (stack.fForceIOAPIC)
        config.hardwareMachine.biosSettings.fIOAPICEnabled = true;
    /* RAM size */
    config.hardwareMachine.ulMemorySizeMB = stack.ulMemorySizeMB;

/*
    <const name="HardDiskControllerIDE" value="14" />
    <const name="HardDiskControllerSATA" value="15" />
    <const name="HardDiskControllerSCSI" value="16" />
    <const name="HardDiskControllerSAS" value="17" />
*/

#ifdef VBOX_WITH_USB
    /* USB controller */
    if (stack.fUSBEnabled)
    {
        /** @todo r=klaus add support for arbitrary USB controller types, this can't handle
         *  multiple controllers due to its design anyway */
        /* Usually the OHCI controller is enabled already, need to check. But
         * do this only if there is no xHCI controller. */
        bool fOHCIEnabled = false;
        bool fXHCIEnabled = false;
        settings::USBControllerList &llUSBControllers = config.hardwareMachine.usbSettings.llUSBControllers;
        settings::USBControllerList::iterator it;
        for (it = llUSBControllers.begin(); it != llUSBControllers.end(); ++it)
        {
            if (it->enmType == USBControllerType_OHCI)
                fOHCIEnabled = true;
            if (it->enmType == USBControllerType_XHCI)
                fXHCIEnabled = true;
        }

        if (!fXHCIEnabled && !fOHCIEnabled)
        {
            settings::USBController ctrl;
            ctrl.strName = "OHCI";
            ctrl.enmType = USBControllerType_OHCI;

            llUSBControllers.push_back(ctrl);
        }
    }
    else
        config.hardwareMachine.usbSettings.llUSBControllers.clear();
#endif
    /* Audio adapter */
    if (stack.strAudioAdapter.isNotEmpty())
    {
        config.hardwareMachine.audioAdapter.fEnabled = true;
        config.hardwareMachine.audioAdapter.controllerType = (AudioControllerType_T)stack.strAudioAdapter.toUInt32();
    }
    else
        config.hardwareMachine.audioAdapter.fEnabled = false;
    /* Network adapter */
    settings::NetworkAdaptersList &llNetworkAdapters = config.hardwareMachine.llNetworkAdapters;
    /* First disable all network cards, they will be enabled below again. */
    settings::NetworkAdaptersList::iterator it1;
    bool fKeepAllMACs = m->optListImport.contains(ImportOptions_KeepAllMACs);
    bool fKeepNATMACs = m->optListImport.contains(ImportOptions_KeepNATMACs);
    for (it1 = llNetworkAdapters.begin(); it1 != llNetworkAdapters.end(); ++it1)
    {
        it1->fEnabled = false;
        if (!(   fKeepAllMACs
              || (fKeepNATMACs && it1->mode == NetworkAttachmentType_NAT)
              || (fKeepNATMACs && it1->mode == NetworkAttachmentType_NATNetwork)))
            /* Force generation of new MAC address below. */
            it1->strMACAddress.setNull();
    }
    /* Now iterate over all network entries. */
    std::list<VirtualSystemDescriptionEntry*> avsdeNWs = vsdescThis->i_findByType(VirtualSystemDescriptionType_NetworkAdapter);
    if (!avsdeNWs.empty())
    {
        /* Iterate through all network adapter entries and search for the
         * corresponding one in the machine config. If one is found, configure
         * it based on the user settings. */
        list<VirtualSystemDescriptionEntry*>::const_iterator itNW;
        for (itNW = avsdeNWs.begin();
             itNW != avsdeNWs.end();
             ++itNW)
        {
            VirtualSystemDescriptionEntry *vsdeNW = *itNW;
            if (   vsdeNW->strExtraConfigCurrent.startsWith("slot=", Utf8Str::CaseInsensitive)
                && vsdeNW->strExtraConfigCurrent.length() > 6)
            {
                uint32_t iSlot = vsdeNW->strExtraConfigCurrent.substr(5).toUInt32();
                /* Iterate through all network adapters in the machine config. */
                for (it1 = llNetworkAdapters.begin();
                     it1 != llNetworkAdapters.end();
                     ++it1)
                {
                    /* Compare the slots. */
                    if (it1->ulSlot == iSlot)
                    {
                        it1->fEnabled = true;
                        if (it1->strMACAddress.isEmpty())
                            Host::i_generateMACAddress(it1->strMACAddress);
                        it1->type = (NetworkAdapterType_T)vsdeNW->strVBoxCurrent.toUInt32();
                        break;
                    }
                }
            }
        }
    }

    /* Floppy controller */
    bool fFloppy = vsdescThis->i_findByType(VirtualSystemDescriptionType_Floppy).size() > 0;
    /* DVD controller */
    bool fDVD = vsdescThis->i_findByType(VirtualSystemDescriptionType_CDROM).size() > 0;
    /* Iterate over all storage controller check the attachments and remove
     * them when necessary. Also detect broken configs with more than one
     * attachment. Old VirtualBox versions (prior to 3.2.10) had all disk
     * attachments pointing to the last hard disk image, which causes import
     * failures. A long fixed bug, however the OVF files are long lived. */
    settings::StorageControllersList &llControllers = config.hardwareMachine.storage.llStorageControllers;
    Guid hdUuid;
    uint32_t cDisks = 0;
    bool fInconsistent = false;
    bool fRepairDuplicate = false;
    settings::StorageControllersList::iterator it3;
    for (it3 = llControllers.begin();
         it3 != llControllers.end();
         ++it3)
    {
        settings::AttachedDevicesList &llAttachments = it3->llAttachedDevices;
        settings::AttachedDevicesList::iterator it4 = llAttachments.begin();
        while (it4 != llAttachments.end())
        {
            if (  (   !fDVD
                   && it4->deviceType == DeviceType_DVD)
                ||
                  (   !fFloppy
                   && it4->deviceType == DeviceType_Floppy))
            {
                it4 = llAttachments.erase(it4);
                continue;
            }
            else if (it4->deviceType == DeviceType_HardDisk)
            {
                const Guid &thisUuid = it4->uuid;
                cDisks++;
                if (cDisks == 1)
                {
                    if (hdUuid.isZero())
                        hdUuid = thisUuid;
                    else
                        fInconsistent = true;
                }
                else
                {
                   if (thisUuid.isZero())
                        fInconsistent = true;
                    else if (thisUuid == hdUuid)
                        fRepairDuplicate = true;
                }
            }
            ++it4;
        }
    }
    /* paranoia... */
    if (fInconsistent || cDisks == 1)
        fRepairDuplicate = false;

    /*
     * step 2: scan the machine config for media attachments
     */
    /* get VM name from virtual system description. Only one record is possible (size of list is equal 1). */
    std::list<VirtualSystemDescriptionEntry*> vmName = vsdescThis->i_findByType(VirtualSystemDescriptionType_Name);
    std::list<VirtualSystemDescriptionEntry*>::iterator vmNameIt = vmName.begin();
    VirtualSystemDescriptionEntry* vmNameEntry = *vmNameIt;

    /* Get all hard disk descriptions. */
    std::list<VirtualSystemDescriptionEntry*> avsdeHDs = vsdescThis->i_findByType(VirtualSystemDescriptionType_HardDiskImage);
    std::list<VirtualSystemDescriptionEntry*>::iterator avsdeHDsIt = avsdeHDs.begin();
    /* paranoia - if there is no 1:1 match do not try to repair. */
    if (cDisks != avsdeHDs.size())
        fRepairDuplicate = false;

    // there must be an image in the OVF disk structs with the same UUID

    ovf::DiskImagesMap::const_iterator oit = stack.mapDisks.begin();
    std::set<RTCString>  disksResolvedNames;

    uint32_t cImportedDisks = 0;

    while (oit != stack.mapDisks.end() && cImportedDisks != avsdeHDs.size())
    {
/** @todo r=bird: Most of the code here is duplicated in the other machine
 *        import method, factor out. */
        ovf::DiskImage diCurrent = oit->second;

        Log(("diCurrent.strDiskId=%s diCurrent.strHref=%s\n", diCurrent.strDiskId.c_str(), diCurrent.strHref.c_str()));

        /* Iterate over all given disk images of the virtual system
         * disks description. We need to find the target disk path,
         * which could be changed by the user. */
        VirtualSystemDescriptionEntry *vsdeTargetHD = NULL;
        for (list<VirtualSystemDescriptionEntry*>::const_iterator itHD = avsdeHDs.begin();
             itHD != avsdeHDs.end();
             ++itHD)
        {
            VirtualSystemDescriptionEntry *vsdeHD = *itHD;
            if (vsdeHD->strRef == oit->first)
            {
                vsdeTargetHD = vsdeHD;
                break;
            }
        }
        if (!vsdeTargetHD)
        {
            /* possible case if a disk image belongs to other virtual system (OVF package with multiple VMs inside) */
            Log1Warning(("OVA/OVF import: Disk image %s was missed during import of VM %s\n",
                         oit->first.c_str(), vmNameEntry->strOvf.c_str()));
            NOREF(vmNameEntry);
            ++oit;
            continue;
        }









        /*
         * preliminary check availability of the image
         * This step is useful if image is placed in the OVA (TAR) package
         */
        if (stack.hVfsFssOva != NIL_RTVFSFSSTREAM)
        {
            /* It means that we possibly have imported the storage earlier on a previous loop step. */
            std::set<RTCString>::const_iterator h = disksResolvedNames.find(diCurrent.strHref);
            if (h != disksResolvedNames.end())
            {
                /* Yes, disk name was found, we can skip it*/
                ++oit;
                continue;
            }
l_skipped:
            rc = i_preCheckImageAvailability(stack);
            if (SUCCEEDED(rc))
            {
                /* current opened file isn't the same as passed one */
                if (RTStrICmp(diCurrent.strHref.c_str(), stack.pszOvaLookAheadName) != 0)
                {
                    // availableImage contains the disk identifier (e.g. "vmdisk1"), which should exist
                    // in the virtual system's disks map under that ID and also in the global images map
                    // and find the disk from the OVF's disk list
                    ovf::DiskImagesMap::const_iterator itDiskImage;
                    for (itDiskImage = stack.mapDisks.begin();
                         itDiskImage != stack.mapDisks.end();
                         itDiskImage++)
                        if (itDiskImage->second.strHref.compare(stack.pszOvaLookAheadName,
                                                                Utf8Str::CaseInsensitive) == 0)
                            break;
                    if (itDiskImage == stack.mapDisks.end())
                    {
                        LogFunc(("Skipping '%s'\n", stack.pszOvaLookAheadName));
                        RTVfsIoStrmRelease(stack.claimOvaLookAHead());
                        goto l_skipped;
                    }
                        //throw setError(E_FAIL,
                        //               tr("Internal inconsistency looking up disk image '%s'. "
                        //                  "Check compliance OVA package structure and file names "
                        //                  "references in the section <References> in the OVF file."),
                        //               stack.pszOvaLookAheadName);

                    /* replace with a new found disk image */
                    diCurrent = *(&itDiskImage->second);

                    /*
                     * Again iterate over all given disk images of the virtual system
                     * disks description using the found disk image
                     */
                    vsdeTargetHD = NULL;
                    for (list<VirtualSystemDescriptionEntry*>::const_iterator itHD = avsdeHDs.begin();
                         itHD != avsdeHDs.end();
                         ++itHD)
                    {
                        VirtualSystemDescriptionEntry *vsdeHD = *itHD;
                        if (vsdeHD->strRef == diCurrent.strDiskId)
                        {
                            vsdeTargetHD = vsdeHD;
                            break;
                        }
                    }

                    /*
                     * in this case it's an error because something is wrong with the OVF description file.
                     * May be VBox imports OVA package with wrong file sequence inside the archive.
                     */
                    if (!vsdeTargetHD)
                        throw setError(E_FAIL,
                                       tr("Internal inconsistency looking up disk image '%s'"),
                                       diCurrent.strHref.c_str());





                }
                else
                {
                    ++oit;
                }
            }
            else
            {
                ++oit;
                continue;
            }
        }
        else
        {
            /* just continue with normal files*/
            ++oit;
        }

        /* Important! to store disk name for the next checks */
        disksResolvedNames.insert(diCurrent.strHref);
////// end of duplicated code.
        // there must be an image in the OVF disk structs with the same UUID
        bool fFound = false;
        Utf8Str strUuid;

        // for each storage controller...
        for (settings::StorageControllersList::iterator sit = config.hardwareMachine.storage.llStorageControllers.begin();
             sit != config.hardwareMachine.storage.llStorageControllers.end();
             ++sit)
        {
            settings::StorageController &sc = *sit;

            // find the OVF virtual system description entry for this storage controller
/** @todo
 * r=bird: What on earh this is switch supposed to do?  (I've added the default:break;, so don't
 * get confused by it.)  Kind of looks like it's supposed to do something error handling related
 * in the default case...
 */
            switch (sc.storageBus)
            {
                case StorageBus_SATA:
                    break;
                case StorageBus_SCSI:
                    break;
                case StorageBus_IDE:
                    break;
                case StorageBus_SAS:
                    break;
                default: break; /* Shut up MSC. */
            }

            // for each medium attachment to this controller...
            for (settings::AttachedDevicesList::iterator dit = sc.llAttachedDevices.begin();
                 dit != sc.llAttachedDevices.end();
                 ++dit)
            {
                settings::AttachedDevice &d = *dit;

                if (d.uuid.isZero())
                    // empty DVD and floppy media
                    continue;

                // When repairing a broken VirtualBox xml config section (written
                // by VirtualBox versions earlier than 3.2.10) assume the disks
                // show up in the same order as in the OVF description.
                if (fRepairDuplicate)
                {
                    VirtualSystemDescriptionEntry *vsdeHD = *avsdeHDsIt;
                    ovf::DiskImagesMap::const_iterator itDiskImage = stack.mapDisks.find(vsdeHD->strRef);
                    if (itDiskImage != stack.mapDisks.end())
                    {
                        const ovf::DiskImage &di = itDiskImage->second;
                        d.uuid = Guid(di.uuidVBox);
                    }
                    ++avsdeHDsIt;
                }

                // convert the Guid to string
                strUuid = d.uuid.toString();

                if (diCurrent.uuidVBox != strUuid)
                {
                    continue;
                }

                /*
                 * step 3: import disk
                 */
                Utf8Str savedVBoxCurrent = vsdeTargetHD->strVBoxCurrent;
                ComObjPtr<Medium> pTargetHD;

                i_importOneDiskImage(diCurrent,
                                     &vsdeTargetHD->strVBoxCurrent,
                                     pTargetHD,
                                     stack);

                Bstr hdId;

                ComObjPtr<MediumFormat> mediumFormat;
                rc = i_findMediumFormatFromDiskImage(diCurrent, mediumFormat);
                if (FAILED(rc))
                    throw rc;

                Bstr bstrFormatName;
                rc = mediumFormat->COMGETTER(Name)(bstrFormatName.asOutParam());
                if (FAILED(rc))
                    throw rc;

                Utf8Str vdf = Utf8Str(bstrFormatName);

                if (vdf.compare("RAW", Utf8Str::CaseInsensitive) == 0)
                {
                    ComPtr<IMedium> dvdImage(pTargetHD);

                    rc = mVirtualBox->OpenMedium(Bstr(vsdeTargetHD->strVBoxCurrent).raw(),
                                                 DeviceType_DVD,
                                                 AccessMode_ReadWrite,
                                                 false,
                                                 dvdImage.asOutParam());

                    if (FAILED(rc)) throw rc;

                    // ... and replace the old UUID in the machine config with the one of
                    // the imported disk that was just created
                    rc = dvdImage->COMGETTER(Id)(hdId.asOutParam());
                    if (FAILED(rc)) throw rc;
                }
                else
                {
                    // ... and replace the old UUID in the machine config with the one of
                    // the imported disk that was just created
                    rc = pTargetHD->COMGETTER(Id)(hdId.asOutParam());
                    if (FAILED(rc)) throw rc;
                }

                /* restore */
                vsdeTargetHD->strVBoxCurrent = savedVBoxCurrent;

                /*
                 * 1. saving original UUID for restoring in case of failure.
                 * 2. replacement of original UUID by new UUID in the current VM config (settings::MachineConfigFile).
                 */
                {
                    rc = stack.saveOriginalUUIDOfAttachedDevice(d, Utf8Str(hdId));
                    d.uuid = hdId;
                }

                fFound = true;
                break;
            } // for (settings::AttachedDevicesList::const_iterator dit = sc.llAttachedDevices.begin();
        } // for (settings::StorageControllersList::const_iterator sit = config.hardwareMachine.storage.llStorageControllers.begin();

            // no disk with such a UUID found:
        if (!fFound)
            throw setError(E_FAIL,
                           tr("<vbox:Machine> element in OVF contains a medium attachment for the disk image %s "
                              "but the OVF describes no such image"),
                           strUuid.c_str());

        ++cImportedDisks;

    }// while(oit != stack.mapDisks.end())


    /*
     * quantity of the imported disks isn't equal to the size of the avsdeHDs list.
     */
    if(cImportedDisks < avsdeHDs.size())
    {
        Log1Warning(("Not all disk images were imported for VM %s. Check OVF description file.",
                     vmNameEntry->strOvf.c_str()));
    }

    /*
     * step 4): create the machine and have it import the config
     */

    ComObjPtr<Machine> pNewMachine;
    rc = pNewMachine.createObject();
    if (FAILED(rc)) throw rc;

    // this magic constructor fills the new machine object with the MachineConfig
    // instance that we created from the vbox:Machine
    rc = pNewMachine->init(mVirtualBox,
                           stack.strNameVBox,// name from OVF preparations; can be suffixed to avoid duplicates
                           config);          // the whole machine config
    if (FAILED(rc)) throw rc;

    pReturnNewMachine = ComPtr<IMachine>(pNewMachine);

    // and register it
    rc = mVirtualBox->RegisterMachine(pNewMachine);
    if (FAILED(rc)) throw rc;

    // store new machine for roll-back in case of errors
    Bstr bstrNewMachineId;
    rc = pNewMachine->COMGETTER(Id)(bstrNewMachineId.asOutParam());
    if (FAILED(rc)) throw rc;
    m->llGuidsMachinesCreated.push_back(Guid(bstrNewMachineId));

    LogFlowFuncLeave();
}

/**
 * @throws HRESULT errors.
 */
void Appliance::i_importMachines(ImportStack &stack)
{
    // this is safe to access because this thread only gets started
    const ovf::OVFReader &reader = *m->pReader;

    // create a session for the machine + disks we manipulate below
    HRESULT rc = stack.pSession.createInprocObject(CLSID_Session);
    ComAssertComRCThrowRC(rc);

    list<ovf::VirtualSystem>::const_iterator it;
    list< ComObjPtr<VirtualSystemDescription> >::const_iterator it1;
    /* Iterate through all virtual systems of that appliance */
    size_t i = 0;
    for (it  = reader.m_llVirtualSystems.begin(), it1  = m->virtualSystemDescriptions.begin();
         it != reader.m_llVirtualSystems.end() && it1 != m->virtualSystemDescriptions.end();
         ++it, ++it1, ++i)
    {
        const ovf::VirtualSystem &vsysThis = *it;
        ComObjPtr<VirtualSystemDescription> vsdescThis = (*it1);

        ComPtr<IMachine> pNewMachine;

        // there are two ways in which we can create a vbox machine from OVF:
        // -- either this OVF was written by vbox 3.2 or later, in which case there is a <vbox:Machine> element
        //    in the <VirtualSystem>; then the VirtualSystemDescription::Data has a settings::MachineConfigFile
        //    with all the machine config pretty-parsed;
        // -- or this is an OVF from an older vbox or an external source, and then we need to translate the
        //    VirtualSystemDescriptionEntry and do import work

        // Even for the vbox:Machine case, there are a number of configuration items that will be taken from
        // the OVF because otherwise the "override import parameters" mechanism in the GUI won't work.

        // VM name
        std::list<VirtualSystemDescriptionEntry*> vsdeName = vsdescThis->i_findByType(VirtualSystemDescriptionType_Name);
        if (vsdeName.size() < 1)
            throw setError(VBOX_E_FILE_ERROR,
                           tr("Missing VM name"));
        stack.strNameVBox = vsdeName.front()->strVBoxCurrent;

        // have VirtualBox suggest where the filename would be placed so we can
        // put the disk images in the same directory
        Bstr bstrMachineFilename;
        rc = mVirtualBox->ComposeMachineFilename(Bstr(stack.strNameVBox).raw(),
                                                 NULL /* aGroup */,
                                                 NULL /* aCreateFlags */,
                                                 NULL /* aBaseFolder */,
                                                 bstrMachineFilename.asOutParam());
        if (FAILED(rc)) throw rc;
        // and determine the machine folder from that
        stack.strMachineFolder = bstrMachineFilename;
        stack.strMachineFolder.stripFilename();
        LogFunc(("i=%zu strName=%s bstrMachineFilename=%ls\n", i, stack.strNameVBox.c_str(), bstrMachineFilename.raw()));

        // guest OS type
        std::list<VirtualSystemDescriptionEntry*> vsdeOS;
        vsdeOS = vsdescThis->i_findByType(VirtualSystemDescriptionType_OS);
        if (vsdeOS.size() < 1)
            throw setError(VBOX_E_FILE_ERROR,
                           tr("Missing guest OS type"));
        stack.strOsTypeVBox = vsdeOS.front()->strVBoxCurrent;

        // CPU count
        std::list<VirtualSystemDescriptionEntry*> vsdeCPU = vsdescThis->i_findByType(VirtualSystemDescriptionType_CPU);
        if (vsdeCPU.size() != 1)
            throw setError(VBOX_E_FILE_ERROR, tr("CPU count missing"));

        stack.cCPUs = vsdeCPU.front()->strVBoxCurrent.toUInt32();
        // We need HWVirt & IO-APIC if more than one CPU is requested
        if (stack.cCPUs > 1)
        {
            stack.fForceHWVirt = true;
            stack.fForceIOAPIC = true;
        }

        // RAM
        std::list<VirtualSystemDescriptionEntry*> vsdeRAM = vsdescThis->i_findByType(VirtualSystemDescriptionType_Memory);
        if (vsdeRAM.size() != 1)
            throw setError(VBOX_E_FILE_ERROR, tr("RAM size missing"));
        stack.ulMemorySizeMB = (ULONG)vsdeRAM.front()->strVBoxCurrent.toUInt64();

#ifdef VBOX_WITH_USB
        // USB controller
        std::list<VirtualSystemDescriptionEntry*> vsdeUSBController =
            vsdescThis->i_findByType(VirtualSystemDescriptionType_USBController);
        // USB support is enabled if there's at least one such entry; to disable USB support,
        // the type of the USB item would have been changed to "ignore"
        stack.fUSBEnabled = !vsdeUSBController.empty();
#endif
        // audio adapter
        std::list<VirtualSystemDescriptionEntry*> vsdeAudioAdapter =
            vsdescThis->i_findByType(VirtualSystemDescriptionType_SoundCard);
        /** @todo we support one audio adapter only */
        if (!vsdeAudioAdapter.empty())
            stack.strAudioAdapter = vsdeAudioAdapter.front()->strVBoxCurrent;

        // for the description of the new machine, always use the OVF entry, the user may have changed it in the import config
        std::list<VirtualSystemDescriptionEntry*> vsdeDescription =
            vsdescThis->i_findByType(VirtualSystemDescriptionType_Description);
        if (!vsdeDescription.empty())
            stack.strDescription = vsdeDescription.front()->strVBoxCurrent;

        // import vbox:machine or OVF now
        if (vsdescThis->m->pConfig)
            // vbox:Machine config
            i_importVBoxMachine(vsdescThis, pNewMachine, stack);
        else
            // generic OVF config
            i_importMachineGeneric(vsysThis, vsdescThis, pNewMachine, stack);

    } // for (it = pAppliance->m->llVirtualSystems.begin() ...
}

HRESULT Appliance::ImportStack::saveOriginalUUIDOfAttachedDevice(settings::AttachedDevice &device,
                                                     const Utf8Str &newlyUuid)
{
    HRESULT rc = S_OK;

    /* save for restoring */
    mapNewUUIDsToOriginalUUIDs.insert(std::make_pair(newlyUuid, device.uuid.toString()));

    return rc;
}

HRESULT Appliance::ImportStack::restoreOriginalUUIDOfAttachedDevice(settings::MachineConfigFile *config)
{
    HRESULT rc = S_OK;

    settings::StorageControllersList &llControllers = config->hardwareMachine.storage.llStorageControllers;
    settings::StorageControllersList::iterator itscl;
    for (itscl = llControllers.begin();
         itscl != llControllers.end();
         ++itscl)
    {
        settings::AttachedDevicesList &llAttachments = itscl->llAttachedDevices;
        settings::AttachedDevicesList::iterator itadl = llAttachments.begin();
        while (itadl != llAttachments.end())
        {
            std::map<Utf8Str , Utf8Str>::iterator it =
                mapNewUUIDsToOriginalUUIDs.find(itadl->uuid.toString());
            if(it!=mapNewUUIDsToOriginalUUIDs.end())
            {
                Utf8Str uuidOriginal = it->second;
                itadl->uuid = Guid(uuidOriginal);
                mapNewUUIDsToOriginalUUIDs.erase(it->first);
            }
            ++itadl;
        }
    }

    return rc;
}

/**
 * @throws Nothing
 */
RTVFSIOSTREAM Appliance::ImportStack::claimOvaLookAHead(void)
{
    RTVFSIOSTREAM hVfsIos = this->hVfsIosOvaLookAhead;
    this->hVfsIosOvaLookAhead = NIL_RTVFSIOSTREAM;
    /* We don't free the name since it may be referenced in error messages and such. */
    return hVfsIos;
}

