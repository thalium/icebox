/** @file
 * Settings file data structures.
 *
 * These structures are created by the settings file loader and filled with values
 * copied from the raw XML data. This was all new with VirtualBox 3.1 and allows us
 * to finally make the XML reader version-independent and read VirtualBox XML files
 * from earlier and even newer (future) versions without requiring complicated,
 * tedious and error-prone XSLT conversions.
 *
 * It is this file that defines all structures that map VirtualBox global and
 * machine settings to XML files. These structures are used by the rest of Main,
 * even though this header file does not require anything else in Main.
 *
 * Note: Headers in Main code have been tweaked to only declare the structures
 * defined here so that this header need only be included from code files that
 * actually use these structures.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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

#ifndef ___VBox_settings_h
#define ___VBox_settings_h

#include <iprt/time.h>

#include "VBox/com/VirtualBox.h"

#include <VBox/com/Guid.h>
#include <VBox/com/string.h>

#include <list>
#include <map>
#include <vector>

/**
 * Maximum depth of a medium tree, to prevent stack overflows.
 * XPCOM has a relatively low stack size for its workers, and we have
 * to avoid crashes due to exceeding the limit both on reading and
 * writing config files.
 */
#define SETTINGS_MEDIUM_DEPTH_MAX 300

/**
 * Maximum depth of the snapshot tree, to prevent stack overflows.
 * XPCOM has a relatively low stack size for its workers, and we have
 * to avoid crashes due to exceeding the limit both on reading and
 * writing config files. The bottleneck is reading config files with
 * deep snapshot nesting, as libxml2 needs quite some stack space,
 * so with the current stack size the margin isn't big.
 */
#define SETTINGS_SNAPSHOT_DEPTH_MAX 250

namespace xml
{
    class ElementNode;
}

namespace settings
{

class ConfigFileError;

////////////////////////////////////////////////////////////////////////////////
//
// Structures shared between Machine XML and VirtualBox.xml
//
////////////////////////////////////////////////////////////////////////////////

typedef std::map<com::Utf8Str, com::Utf8Str> StringsMap;
typedef std::list<com::Utf8Str> StringsList;

/**
 * USB device filter definition. This struct is used both in MainConfigFile
 * (for global USB filters) and MachineConfigFile (for machine filters).
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct USBDeviceFilter
{
    USBDeviceFilter();

    bool operator==(const USBDeviceFilter&u) const;

    com::Utf8Str            strName;
    bool                    fActive;
    com::Utf8Str            strVendorId,
                            strProductId,
                            strRevision,
                            strManufacturer,
                            strProduct,
                            strSerialNumber,
                            strPort;
    USBDeviceFilterAction_T action;                 // only used with host USB filters
    com::Utf8Str            strRemote;              // irrelevant for host USB objects
    uint32_t                ulMaskedInterfaces;     // irrelevant for host USB objects
};

typedef std::list<USBDeviceFilter> USBDeviceFiltersList;

struct Medium;
typedef std::list<Medium> MediaList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct Medium
{
    Medium();

    bool operator==(const Medium &m) const;

    com::Guid       uuid;
    com::Utf8Str    strLocation;
    com::Utf8Str    strDescription;

    // the following are for hard disks only:
    com::Utf8Str    strFormat;
    bool            fAutoReset;         // optional, only for diffs, default is false
    StringsMap      properties;
    MediumType_T    hdType;

    MediaList       llChildren;         // only used with hard disks

    static const struct Medium Empty;
};

/**
 * A media registry. Starting with VirtualBox 3.3, this can appear in both the
 * VirtualBox.xml file as well as machine XML files with settings version 1.11
 * or higher, so these lists are now in ConfigFileBase.
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct MediaRegistry
{
    bool operator==(const MediaRegistry &m) const;

    MediaList               llHardDisks,
                            llDvdImages,
                            llFloppyImages;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct NATRule
{
    NATRule();

    bool operator==(const NATRule &r) const;

    com::Utf8Str            strName;
    NATProtocol_T           proto;
    uint16_t                u16HostPort;
    com::Utf8Str            strHostIP;
    uint16_t                u16GuestPort;
    com::Utf8Str            strGuestIP;
};
typedef std::map<com::Utf8Str, NATRule> NATRulesMap;

struct NATHostLoopbackOffset
{
    NATHostLoopbackOffset();

    bool operator==(const NATHostLoopbackOffset &o) const;

    bool operator==(const com::Utf8Str& strAddr)
    {
        return strLoopbackHostAddress == strAddr;
    }

    bool operator==(uint32_t off)
    {
        return u32Offset == off;
    }

    /** Note: 128/8 is only acceptable */
    com::Utf8Str strLoopbackHostAddress;
    uint32_t u32Offset;
};

typedef std::list<NATHostLoopbackOffset> NATLoopbackOffsetList;

typedef std::vector<uint8_t> IconBlob;

/**
 * Common base class for both MainConfigFile and MachineConfigFile
 * which contains some common logic for both.
 */
class ConfigFileBase
{
public:
    bool fileExists();

    void copyBaseFrom(const ConfigFileBase &b);

protected:
    ConfigFileBase(const com::Utf8Str *pstrFilename);
    /* Note: this copy constructor doesn't create a full copy of other, cause
     * the file based stuff (xml doc) could not be copied. */
    ConfigFileBase(const ConfigFileBase &other);

    ~ConfigFileBase();

    typedef enum {Error, HardDisk, DVDImage, FloppyImage} MediaType;

    static const char *stringifyMediaType(MediaType t);
    SettingsVersion_T parseVersion(const com::Utf8Str &strVersion,
                                   const xml::ElementNode *pElm);
    void parseUUID(com::Guid &guid,
                   const com::Utf8Str &strUUID,
                   const xml::ElementNode *pElm) const;
    void parseTimestamp(RTTIMESPEC &timestamp,
                        const com::Utf8Str &str,
                        const xml::ElementNode *pElm) const;
    void parseBase64(IconBlob &binary,
                     const com::Utf8Str &str,
                     const xml::ElementNode *pElm) const;
    com::Utf8Str stringifyTimestamp(const RTTIMESPEC &tm) const;
    void toBase64(com::Utf8Str &str,
                  const IconBlob &binary) const;

    void readExtraData(const xml::ElementNode &elmExtraData,
                       StringsMap &map);
    void readUSBDeviceFilters(const xml::ElementNode &elmDeviceFilters,
                              USBDeviceFiltersList &ll);
    void readMediumOne(MediaType t, const xml::ElementNode &elmMedium, Medium &med);
    void readMedium(MediaType t, uint32_t depth, const xml::ElementNode &elmMedium, Medium &med);
    void readMediaRegistry(const xml::ElementNode &elmMediaRegistry, MediaRegistry &mr);
    void readNATForwardRulesMap(const xml::ElementNode  &elmParent, NATRulesMap &mapRules);
    void readNATLoopbacks(const xml::ElementNode &elmParent, NATLoopbackOffsetList &llLoopBacks);

    void setVersionAttribute(xml::ElementNode &elm);
    void specialBackupIfFirstBump();
    void createStubDocument();

    void buildExtraData(xml::ElementNode &elmParent, const StringsMap &me);
    void buildUSBDeviceFilters(xml::ElementNode &elmParent,
                               const USBDeviceFiltersList &ll,
                               bool fHostMode);
    void buildMedium(MediaType t,
                     uint32_t depth,
                     xml::ElementNode &elmMedium,
                     const Medium &mdm);
    void buildMediaRegistry(xml::ElementNode &elmParent,
                            const MediaRegistry &mr);
    void buildNATForwardRulesMap(xml::ElementNode &elmParent, const NATRulesMap &mapRules);
    void buildNATLoopbacks(xml::ElementNode &elmParent, const NATLoopbackOffsetList &natLoopbackList);
    void clearDocument();

    struct Data;
    Data *m;

    friend class ConfigFileError;
};

////////////////////////////////////////////////////////////////////////////////
//
// VirtualBox.xml structures
//
////////////////////////////////////////////////////////////////////////////////

struct USBDeviceSource
{
    com::Utf8Str            strName;
    com::Utf8Str            strBackend;
    com::Utf8Str            strAddress;
    StringsMap              properties;
};

typedef std::list<USBDeviceSource> USBDeviceSourcesList;

struct Host
{
    USBDeviceFiltersList    llUSBDeviceFilters;
    USBDeviceSourcesList    llUSBDeviceSources;
};

struct SystemProperties
{
    SystemProperties();

    com::Utf8Str            strDefaultMachineFolder;
    com::Utf8Str            strDefaultHardDiskFolder;
    com::Utf8Str            strDefaultHardDiskFormat;
    com::Utf8Str            strVRDEAuthLibrary;
    com::Utf8Str            strWebServiceAuthLibrary;
    com::Utf8Str            strDefaultVRDEExtPack;
    com::Utf8Str            strAutostartDatabasePath;
    com::Utf8Str            strDefaultAdditionsISO;
    com::Utf8Str            strDefaultFrontend;
    com::Utf8Str            strLoggingLevel;
    uint32_t                ulLogHistoryCount;
    bool                    fExclusiveHwVirt;
};

struct MachineRegistryEntry
{
    com::Guid       uuid;
    com::Utf8Str    strSettingsFile;
};

typedef std::list<MachineRegistryEntry> MachinesRegistry;

struct DhcpOptValue
{
    DhcpOptValue();
    DhcpOptValue(const com::Utf8Str &aText, DhcpOptEncoding_T aEncoding = DhcpOptEncoding_Legacy);

    com::Utf8Str text;
    DhcpOptEncoding_T encoding;
};

typedef std::map<DhcpOpt_T, DhcpOptValue> DhcpOptionMap;
typedef DhcpOptionMap::value_type DhcpOptValuePair;
typedef DhcpOptionMap::iterator DhcpOptIterator;
typedef DhcpOptionMap::const_iterator DhcpOptConstIterator;

typedef struct VmNameSlotKey
{
    VmNameSlotKey(const com::Utf8Str& aVmName, LONG aSlot);

    bool operator<(const VmNameSlotKey& that) const;

    const com::Utf8Str VmName;
    LONG      Slot;
} VmNameSlotKey;

typedef std::map<VmNameSlotKey, DhcpOptionMap> VmSlot2OptionsMap;
typedef VmSlot2OptionsMap::value_type VmSlot2OptionsPair;
typedef VmSlot2OptionsMap::iterator VmSlot2OptionsIterator;
typedef VmSlot2OptionsMap::const_iterator VmSlot2OptionsConstIterator;

struct DHCPServer
{
    DHCPServer();

    com::Utf8Str    strNetworkName,
                    strIPAddress,
                    strIPLower,
                    strIPUpper;
    bool            fEnabled;
    DhcpOptionMap   GlobalDhcpOptions;
    VmSlot2OptionsMap VmSlot2OptionsM;
};

typedef std::list<DHCPServer> DHCPServersList;


/**
 * NAT Networking settings (NAT service).
 */
struct NATNetwork
{
    NATNetwork();

    com::Utf8Str strNetworkName;
    com::Utf8Str strIPv4NetworkCidr;
    com::Utf8Str strIPv6Prefix;
    bool         fEnabled;
    bool         fIPv6Enabled;
    bool         fAdvertiseDefaultIPv6Route;
    bool         fNeedDhcpServer;
    uint32_t     u32HostLoopback6Offset;
    NATLoopbackOffsetList llHostLoopbackOffsetList;
    NATRulesMap  mapPortForwardRules4;
    NATRulesMap  mapPortForwardRules6;
};

typedef std::list<NATNetwork> NATNetworksList;


class MainConfigFile : public ConfigFileBase
{
public:
    MainConfigFile(const com::Utf8Str *pstrFilename);

    void readMachineRegistry(const xml::ElementNode &elmMachineRegistry);
    void readDHCPServers(const xml::ElementNode &elmDHCPServers);
    void readDhcpOptions(DhcpOptionMap& map, const xml::ElementNode& options);
    void readNATNetworks(const xml::ElementNode &elmNATNetworks);

    void write(const com::Utf8Str strFilename);

    Host                    host;
    SystemProperties        systemProperties;
    MediaRegistry           mediaRegistry;
    MachinesRegistry        llMachines;
    DHCPServersList         llDhcpServers;
    NATNetworksList         llNATNetworks;
    StringsMap              mapExtraDataItems;

private:
    void bumpSettingsVersionIfNeeded();
    void buildUSBDeviceSources(xml::ElementNode &elmParent, const USBDeviceSourcesList &ll);
    void readUSBDeviceSources(const xml::ElementNode &elmDeviceSources, USBDeviceSourcesList &ll);
};

////////////////////////////////////////////////////////////////////////////////
//
// Machine XML structures
//
////////////////////////////////////////////////////////////////////////////////

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct VRDESettings
{
    VRDESettings();

    bool areDefaultSettings(SettingsVersion_T sv) const;

    bool operator==(const VRDESettings& v) const;

    bool            fEnabled;
    AuthType_T      authType;
    uint32_t        ulAuthTimeout;
    com::Utf8Str    strAuthLibrary;
    bool            fAllowMultiConnection,
                    fReuseSingleConnection;
    com::Utf8Str    strVrdeExtPack;
    StringsMap      mapProperties;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct BIOSSettings
{
    BIOSSettings();

    bool areDefaultSettings() const;

    bool operator==(const BIOSSettings &d) const;

    bool            fACPIEnabled,
                    fIOAPICEnabled,
                    fLogoFadeIn,
                    fLogoFadeOut,
                    fPXEDebugEnabled;
    uint32_t        ulLogoDisplayTime;
    BIOSBootMenuMode_T biosBootMenuMode;
    APICMode_T      apicMode;           // requires settings version 1.16 (VirtualBox 5.1)
    int64_t         llTimeOffset;
    com::Utf8Str    strLogoImagePath;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct USBController
{
    USBController();

    bool operator==(const USBController &u) const;

    com::Utf8Str            strName;
    USBControllerType_T     enmType;
};

typedef std::list<USBController> USBControllerList;

struct USB
{
    USB();

    bool operator==(const USB &u) const;

    /** List of USB controllers present. */
    USBControllerList       llUSBControllers;
    /** List of USB device filters. */
    USBDeviceFiltersList    llDeviceFilters;
};

struct NAT
{
    NAT();

    bool areDNSDefaultSettings() const;
    bool areAliasDefaultSettings() const;
    bool areTFTPDefaultSettings() const;
    bool areDefaultSettings() const;

    bool operator==(const NAT &n) const;

    com::Utf8Str            strNetwork;
    com::Utf8Str            strBindIP;
    uint32_t                u32Mtu;
    uint32_t                u32SockRcv;
    uint32_t                u32SockSnd;
    uint32_t                u32TcpRcv;
    uint32_t                u32TcpSnd;
    com::Utf8Str            strTFTPPrefix;
    com::Utf8Str            strTFTPBootFile;
    com::Utf8Str            strTFTPNextServer;
    bool                    fDNSPassDomain;
    bool                    fDNSProxy;
    bool                    fDNSUseHostResolver;
    bool                    fAliasLog;
    bool                    fAliasProxyOnly;
    bool                    fAliasUseSamePorts;
    NATRulesMap             mapRules;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct NetworkAdapter
{
    NetworkAdapter();

    bool areGenericDriverDefaultSettings() const;
    bool areDefaultSettings(SettingsVersion_T sv) const;
    bool areDisabledDefaultSettings() const;

    bool operator==(const NetworkAdapter &n) const;

    uint32_t                ulSlot;

    NetworkAdapterType_T                type;
    bool                                fEnabled;
    com::Utf8Str                        strMACAddress;
    bool                                fCableConnected;
    uint32_t                            ulLineSpeed;
    NetworkAdapterPromiscModePolicy_T   enmPromiscModePolicy;
    bool                                fTraceEnabled;
    com::Utf8Str                        strTraceFile;

    NetworkAttachmentType_T             mode;
    NAT                                 nat;
    com::Utf8Str                        strBridgedName;
    com::Utf8Str                        strHostOnlyName;
    com::Utf8Str                        strInternalNetworkName;
    com::Utf8Str                        strGenericDriver;
    StringsMap                          genericProperties;
    com::Utf8Str                        strNATNetworkName;
    uint32_t                            ulBootPriority;
    com::Utf8Str                        strBandwidthGroup; // requires settings version 1.13 (VirtualBox 4.2)
};

typedef std::list<NetworkAdapter> NetworkAdaptersList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct SerialPort
{
    SerialPort();

    bool operator==(const SerialPort &n) const;

    uint32_t        ulSlot;

    bool            fEnabled;
    uint32_t        ulIOBase;
    uint32_t        ulIRQ;
    PortMode_T      portMode;
    com::Utf8Str    strPath;
    bool            fServer;
};

typedef std::list<SerialPort> SerialPortsList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct ParallelPort
{
    ParallelPort();

    bool operator==(const ParallelPort &d) const;

    uint32_t        ulSlot;

    bool            fEnabled;
    uint32_t        ulIOBase;
    uint32_t        ulIRQ;
    com::Utf8Str    strPath;
};

typedef std::list<ParallelPort> ParallelPortsList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct AudioAdapter
{
    AudioAdapter();

    bool areDefaultSettings(SettingsVersion_T sv) const;

    bool operator==(const AudioAdapter &a) const;

    bool                    fEnabled;
    bool                    fEnabledIn;
    bool                    fEnabledOut;
    AudioControllerType_T   controllerType;
    AudioCodecType_T        codecType;
    AudioDriverType_T       driverType;
    settings::StringsMap properties;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct SharedFolder
{
    SharedFolder();

    bool operator==(const SharedFolder &a) const;

    com::Utf8Str    strName,
                    strHostPath;
    bool            fWritable;
    bool            fAutoMount;
};

typedef std::list<SharedFolder> SharedFoldersList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct GuestProperty
{
    GuestProperty();

    bool operator==(const GuestProperty &g) const;

    com::Utf8Str    strName,
                    strValue;
    uint64_t        timestamp;
    com::Utf8Str    strFlags;
};

typedef std::list<GuestProperty> GuestPropertiesList;

typedef std::map<uint32_t, DeviceType_T> BootOrderMap;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct CpuIdLeaf
{
    CpuIdLeaf();

    bool operator==(const CpuIdLeaf &c) const;

    uint32_t                idx;
    uint32_t                idxSub;
    uint32_t                uEax;
    uint32_t                uEbx;
    uint32_t                uEcx;
    uint32_t                uEdx;
};

typedef std::list<CpuIdLeaf> CpuIdLeafsList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct Cpu
{
    Cpu();

    bool operator==(const Cpu &c) const;

    uint32_t                ulId;
};

typedef std::list<Cpu> CpuList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct BandwidthGroup
{
    BandwidthGroup();

    bool operator==(const BandwidthGroup &i) const;

    com::Utf8Str         strName;
    uint64_t             cMaxBytesPerSec;
    BandwidthGroupType_T enmType;
};

typedef std::list<BandwidthGroup> BandwidthGroupList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct IOSettings
{
    IOSettings();

    bool areIOCacheDefaultSettings() const;
    bool areDefaultSettings() const;

    bool operator==(const IOSettings &i) const;

    bool               fIOCacheEnabled;
    uint32_t           ulIOCacheSize;
    BandwidthGroupList llBandwidthGroups;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct HostPCIDeviceAttachment
{
    HostPCIDeviceAttachment();

    bool operator==(const HostPCIDeviceAttachment &a) const;

    com::Utf8Str    strDeviceName;
    uint32_t        uHostAddress;
    uint32_t        uGuestAddress;
};

typedef std::list<HostPCIDeviceAttachment> HostPCIDeviceAttachmentList;

/**
 * A device attached to a storage controller. This can either be a
 * hard disk or a DVD drive or a floppy drive and also specifies
 * which medium is "in" the drive; as a result, this is a combination
 * of the Main IMedium and IMediumAttachment interfaces.
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct AttachedDevice
{
    AttachedDevice();

    bool operator==(const AttachedDevice &a) const;

    DeviceType_T        deviceType;         // only HardDisk, DVD or Floppy are allowed

    // DVDs can be in pass-through mode:
    bool                fPassThrough;

    // Whether guest-triggered eject of DVDs will keep the medium in the
    // VM config or not:
    bool                fTempEject;

    // Whether the medium is non-rotational:
    bool                fNonRotational;

    // Whether the medium supports discarding unused blocks:
    bool                fDiscard;

    // Whether the medium is hot-pluggable:
    bool                fHotPluggable;

    int32_t             lPort;
    int32_t             lDevice;

    // if an image file is attached to the device (ISO, RAW, or hard disk image such as VDI),
    // this is its UUID; it depends on deviceType which media registry this then needs to
    // be looked up in. If no image file (only permitted for DVDs and floppies), then the UUID is NULL
    com::Guid           uuid;

    // for DVDs and floppies, the attachment can also be a host device:
    com::Utf8Str        strHostDriveSrc;        // if != NULL, value of <HostDrive>/@src

    // Bandwidth group the device is attached to.
    com::Utf8Str        strBwGroup;
};

typedef std::list<AttachedDevice> AttachedDevicesList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct StorageController
{
    StorageController();

    bool operator==(const StorageController &s) const;

    com::Utf8Str            strName;
    StorageBus_T            storageBus;             // _SATA, _SCSI, _IDE, _SAS
    StorageControllerType_T controllerType;
    uint32_t                ulPortCount;
    uint32_t                ulInstance;
    bool                    fUseHostIOCache;
    bool                    fBootable;

    // only for when controllerType == StorageControllerType_IntelAhci:
    int32_t                 lIDE0MasterEmulationPort,
                            lIDE0SlaveEmulationPort,
                            lIDE1MasterEmulationPort,
                            lIDE1SlaveEmulationPort;

    AttachedDevicesList     llAttachedDevices;
};

typedef std::list<StorageController> StorageControllersList;

/**
 * We wrap the storage controllers list into an extra struct so we can
 * use an undefined struct without needing std::list<> in all the headers.
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct Storage
{
    bool operator==(const Storage &s) const;

    StorageControllersList  llStorageControllers;
};

/**
 * Representation of Machine hardware; this is used in the MachineConfigFile.hardwareMachine
 * field.
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct Hardware
{
    Hardware();

    bool areParavirtDefaultSettings(SettingsVersion_T sv) const;
    bool areBootOrderDefaultSettings() const;
    bool areDisplayDefaultSettings() const;
    bool areVideoCaptureDefaultSettings() const;
    bool areAllNetworkAdaptersDefaultSettings(SettingsVersion_T sv) const;

    bool operator==(const Hardware&) const;

    com::Utf8Str        strVersion;             // hardware version, optional
    com::Guid           uuid;                   // hardware uuid, optional (null).

    bool                fHardwareVirt,
                        fNestedPaging,
                        fLargePages,
                        fVPID,
                        fUnrestrictedExecution,
                        fHardwareVirtForce,
                        fSyntheticCpu,
                        fTripleFaultReset,
                        fPAE,
                        fAPIC,                  // requires settings version 1.16 (VirtualBox 5.1)
                        fX2APIC;                // requires settings version 1.16 (VirtualBox 5.1)
    bool                fIBPBOnVMExit;          //< added out of cycle, after 1.16 was out.
    bool                fIBPBOnVMEntry;         //< added out of cycle, after 1.16 was out.
    bool                fSpecCtrl;              //< added out of cycle, after 1.16 was out.
    bool                fSpecCtrlByHost;        //< added out of cycle, after 1.16 was out.
    typedef enum LongModeType { LongMode_Enabled, LongMode_Disabled, LongMode_Legacy } LongModeType;
    LongModeType        enmLongMode;
    uint32_t            cCPUs;
    bool                fCpuHotPlug;            // requires settings version 1.10 (VirtualBox 3.2)
    CpuList             llCpus;                 // requires settings version 1.10 (VirtualBox 3.2)
    bool                fHPETEnabled;           // requires settings version 1.10 (VirtualBox 3.2)
    uint32_t            ulCpuExecutionCap;      // requires settings version 1.11 (VirtualBox 3.3)
    uint32_t            uCpuIdPortabilityLevel; // requires settings version 1.15 (VirtualBox 5.0)
    com::Utf8Str        strCpuProfile;          // requires settings version 1.16 (VirtualBox 5.1)

    CpuIdLeafsList      llCpuIdLeafs;

    uint32_t            ulMemorySizeMB;

    BootOrderMap        mapBootOrder;           // item 0 has highest priority

    GraphicsControllerType_T graphicsControllerType;
    uint32_t            ulVRAMSizeMB;
    uint32_t            cMonitors;
    bool                fAccelerate3D,
                        fAccelerate2DVideo;     // requires settings version 1.8 (VirtualBox 3.1)

    uint32_t            ulVideoCaptureHorzRes;  // requires settings version 1.14 (VirtualBox 4.3)
    uint32_t            ulVideoCaptureVertRes;  // requires settings version 1.14 (VirtualBox 4.3)
    uint32_t            ulVideoCaptureRate;     // requires settings version 1.14 (VirtualBox 4.3)
    uint32_t            ulVideoCaptureFPS;      // requires settings version 1.14 (VirtualBox 4.3)
    uint32_t            ulVideoCaptureMaxTime;  // requires settings version 1.14 (VirtualBox 4.3)
    uint32_t            ulVideoCaptureMaxSize;  // requires settings version 1.14 (VirtualBox 4.3)
    bool                fVideoCaptureEnabled;   // requires settings version 1.14 (VirtualBox 4.3)
    uint64_t            u64VideoCaptureScreens; // requires settings version 1.14 (VirtualBox 4.3)
    com::Utf8Str        strVideoCaptureFile;    // requires settings version 1.14 (VirtualBox 4.3)
    com::Utf8Str        strVideoCaptureOptions; // new since VirtualBox 5.2.

    FirmwareType_T      firmwareType;           // requires settings version 1.9 (VirtualBox 3.1)

    PointingHIDType_T   pointingHIDType;        // requires settings version 1.10 (VirtualBox 3.2)
    KeyboardHIDType_T   keyboardHIDType;        // requires settings version 1.10 (VirtualBox 3.2)

    ChipsetType_T       chipsetType;            // requires settings version 1.11 (VirtualBox 4.0)
    ParavirtProvider_T  paravirtProvider;       // requires settings version 1.15 (VirtualBox 4.4)
    com::Utf8Str        strParavirtDebug;       // requires settings version 1.16 (VirtualBox 5.1)

    bool                fEmulatedUSBCardReader; // 1.12 (VirtualBox 4.1)

    VRDESettings        vrdeSettings;

    BIOSSettings        biosSettings;
    USB                 usbSettings;
    NetworkAdaptersList llNetworkAdapters;
    SerialPortsList     llSerialPorts;
    ParallelPortsList   llParallelPorts;
    AudioAdapter        audioAdapter;
    Storage             storage;

    // technically these two have no business in the hardware section, but for some
    // clever reason <Hardware> is where they are in the XML....
    SharedFoldersList   llSharedFolders;
    ClipboardMode_T     clipboardMode;
    DnDMode_T           dndMode;

    uint32_t            ulMemoryBalloonSize;
    bool                fPageFusionEnabled;

    GuestPropertiesList llGuestProperties;

    IOSettings          ioSettings;             // requires settings version 1.10 (VirtualBox 3.2)
    HostPCIDeviceAttachmentList pciAttachments; // requires settings version 1.12 (VirtualBox 4.1)

    com::Utf8Str        strDefaultFrontend;     // requires settings version 1.14 (VirtualBox 4.3)
};

/**
 * Settings that has to do with debugging.
 */
struct Debugging
{
    Debugging();

    bool areDefaultSettings() const;

    bool operator==(const Debugging &rOther) const;

    bool                    fTracingEnabled;
    bool                    fAllowTracingToAccessVM;
    com::Utf8Str            strTracingConfig;
};

/**
 * Settings that has to do with autostart.
 */
struct Autostart
{
    Autostart();

    bool areDefaultSettings() const;

    bool operator==(const Autostart &rOther) const;

    bool                    fAutostartEnabled;
    uint32_t                uAutostartDelay;
    AutostopType_T          enmAutostopType;
};

struct Snapshot;
typedef std::list<Snapshot> SnapshotsList;

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct Snapshot
{
    Snapshot();

    bool operator==(const Snapshot &s) const;

    com::Guid       uuid;
    com::Utf8Str    strName,
                    strDescription;             // optional
    RTTIMESPEC      timestamp;

    com::Utf8Str    strStateFile;               // for online snapshots only

    Hardware        hardware;

    Debugging       debugging;
    Autostart       autostart;

    SnapshotsList   llChildSnapshots;

    static const struct Snapshot Empty;
};

/**
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by MachineConfigFile::operator==(), or otherwise
 * your settings might never get saved.
 */
struct MachineUserData
{
    MachineUserData();

    bool operator==(const MachineUserData &c) const;

    com::Utf8Str            strName;
    bool                    fDirectoryIncludesUUID;
    bool                    fNameSync;
    com::Utf8Str            strDescription;
    StringsList             llGroups;
    com::Utf8Str            strOsType;
    com::Utf8Str            strSnapshotFolder;
    bool                    fTeleporterEnabled;
    uint32_t                uTeleporterPort;
    com::Utf8Str            strTeleporterAddress;
    com::Utf8Str            strTeleporterPassword;
    FaultToleranceState_T   enmFaultToleranceState;
    uint32_t                uFaultTolerancePort;
    com::Utf8Str            strFaultToleranceAddress;
    com::Utf8Str            strFaultTolerancePassword;
    uint32_t                uFaultToleranceInterval;
    bool                    fRTCUseUTC;
    IconBlob                ovIcon;
    com::Utf8Str            strVMPriority;
};


/**
 * MachineConfigFile represents an XML machine configuration. All the machine settings
 * that go out to the XML (or are read from it) are in here.
 *
 * NOTE: If you add any fields in here, you must update a) the constructor and b)
 * the operator== which is used by Machine::saveSettings(), or otherwise your settings
 * might never get saved.
 */
class MachineConfigFile : public ConfigFileBase
{
public:
    com::Guid               uuid;

    MachineUserData         machineUserData;

    com::Utf8Str            strStateFile;
    bool                    fCurrentStateModified;      // optional, default is true
    RTTIMESPEC              timeLastStateChange;        // optional, defaults to now
    bool                    fAborted;                   // optional, default is false

    com::Guid               uuidCurrentSnapshot;

    Hardware                hardwareMachine;
    MediaRegistry           mediaRegistry;
    Debugging               debugging;
    Autostart               autostart;

    StringsMap              mapExtraDataItems;

    SnapshotsList           llFirstSnapshot;            // first snapshot or empty list if there's none

    MachineConfigFile(const com::Utf8Str *pstrFilename);

    bool operator==(const MachineConfigFile &m) const;

    bool canHaveOwnMediaRegistry() const;

    void importMachineXML(const xml::ElementNode &elmMachine);

    void write(const com::Utf8Str &strFilename);

    enum
    {
        BuildMachineXML_IncludeSnapshots = 0x01,
        BuildMachineXML_WriteVBoxVersionAttribute = 0x02,
        BuildMachineXML_SkipRemovableMedia = 0x04,
        BuildMachineXML_MediaRegistry = 0x08,
        BuildMachineXML_SuppressSavedState = 0x10
    };
    void buildMachineXML(xml::ElementNode &elmMachine,
                         uint32_t fl,
                         std::list<xml::ElementNode*> *pllElementsWithUuidAttributes);

    static bool isAudioDriverAllowedOnThisHost(AudioDriverType_T drv);
    static AudioDriverType_T getHostDefaultAudioDriver();

private:
    void readNetworkAdapters(const xml::ElementNode &elmHardware, NetworkAdaptersList &ll);
    void readAttachedNetworkMode(const xml::ElementNode &pelmMode, bool fEnabled, NetworkAdapter &nic);
    void readCpuIdTree(const xml::ElementNode &elmCpuid, CpuIdLeafsList &ll);
    void readCpuTree(const xml::ElementNode &elmCpu, CpuList &ll);
    void readSerialPorts(const xml::ElementNode &elmUART, SerialPortsList &ll);
    void readParallelPorts(const xml::ElementNode &elmLPT, ParallelPortsList &ll);
    void readAudioAdapter(const xml::ElementNode &elmAudioAdapter, AudioAdapter &aa);
    void readGuestProperties(const xml::ElementNode &elmGuestProperties, Hardware &hw);
    void readStorageControllerAttributes(const xml::ElementNode &elmStorageController, StorageController &sctl);
    void readHardware(const xml::ElementNode &elmHardware, Hardware &hw);
    void readHardDiskAttachments_pre1_7(const xml::ElementNode &elmHardDiskAttachments, Storage &strg);
    void readStorageControllers(const xml::ElementNode &elmStorageControllers, Storage &strg);
    void readDVDAndFloppies_pre1_9(const xml::ElementNode &elmHardware, Storage &strg);
    void readTeleporter(const xml::ElementNode *pElmTeleporter, MachineUserData *pUserData);
    void readDebugging(const xml::ElementNode *pElmDbg, Debugging *pDbg);
    void readAutostart(const xml::ElementNode *pElmAutostart, Autostart *pAutostart);
    void readGroups(const xml::ElementNode *elmGroups, StringsList *pllGroups);
    bool readSnapshot(const com::Guid &curSnapshotUuid, uint32_t depth, const xml::ElementNode &elmSnapshot, Snapshot &snap);
    void convertOldOSType_pre1_5(com::Utf8Str &str);
    void readMachine(const xml::ElementNode &elmMachine);

    void buildHardwareXML(xml::ElementNode &elmParent, const Hardware &hw, uint32_t fl, std::list<xml::ElementNode*> *pllElementsWithUuidAttributes);
    void buildNetworkXML(NetworkAttachmentType_T mode, bool fEnabled, xml::ElementNode &elmParent, const NetworkAdapter &nic);
    void buildStorageControllersXML(xml::ElementNode &elmParent,
                                    const Storage &st,
                                    bool fSkipRemovableMedia,
                                    std::list<xml::ElementNode*> *pllElementsWithUuidAttributes);
    void buildDebuggingXML(xml::ElementNode *pElmParent, const Debugging *pDbg);
    void buildAutostartXML(xml::ElementNode *pElmParent, const Autostart *pAutostart);
    void buildGroupsXML(xml::ElementNode *pElmParent, const StringsList *pllGroups);
    void buildSnapshotXML(uint32_t depth, xml::ElementNode &elmParent, const Snapshot &snap);

    void bumpSettingsVersionIfNeeded();
};

} // namespace settings


#endif /* ___VBox_settings_h */
