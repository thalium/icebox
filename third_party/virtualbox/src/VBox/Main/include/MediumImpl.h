/* $Id: MediumImpl.h $ */
/** @file
 * VirtualBox COM class implementation
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


#ifndef ____H_MEDIUMIMPL
#define ____H_MEDIUMIMPL

#include <VBox/vd.h>
#include "MediumWrap.h"
#include "VirtualBoxBase.h"
#include "AutoCaller.h"
#include "SecretKeyStore.h"
class Progress;
class MediumFormat;
class MediumLockList;

namespace settings
{
    struct Medium;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Medium component class for all media types.
 */
class ATL_NO_VTABLE Medium :
    public MediumWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(Medium)

    HRESULT FinalConstruct();
    void FinalRelease();

    enum HDDOpenMode  { OpenReadWrite, OpenReadOnly };
                // have to use a special enum for the overloaded init() below;
                // can't use AccessMode_T from XIDL because that's mapped to an int
                // and would be ambiguous

    // public initializer/uninitializer for internal purposes only

    // initializer to create empty medium (VirtualBox::CreateMedium())
    HRESULT init(VirtualBox *aVirtualBox,
                 const Utf8Str &aFormat,
                 const Utf8Str &aLocation,
                 const Guid &uuidMachineRegistry,
                 const DeviceType_T aDeviceType);

    // initializer for opening existing media
    // (VirtualBox::OpenMedium(); Machine::AttachDevice())
    HRESULT init(VirtualBox *aVirtualBox,
                 const Utf8Str &aLocation,
                 HDDOpenMode enOpenMode,
                 bool fForceNewUuid,
                 DeviceType_T aDeviceType);

    // initializer used when loading settings
    HRESULT initOne(Medium *aParent,
                    DeviceType_T aDeviceType,
                    const Guid &uuidMachineRegistry,
                    const settings::Medium &data,
                    const Utf8Str &strMachineFolder);
    HRESULT init(VirtualBox *aVirtualBox,
                 Medium *aParent,
                 DeviceType_T aDeviceType,
                 const Guid &uuidMachineRegistry,
                 const settings::Medium &data,
                 const Utf8Str &strMachineFolder,
                 AutoWriteLock &mediaTreeLock);

    // initializer for host floppy/DVD
    HRESULT init(VirtualBox *aVirtualBox,
                 DeviceType_T aDeviceType,
                 const Utf8Str &aLocation,
                 const Utf8Str &aDescription = Utf8Str::Empty);

    void uninit();

    void i_deparent();
    void i_setParent(const ComObjPtr<Medium> &pParent);

    // unsafe methods for internal purposes only (ensure there is
    // a caller and a read lock before calling them!)
    const ComObjPtr<Medium>& i_getParent() const;
    const MediaList& i_getChildren() const;

    const Guid& i_getId() const;
    MediumState_T i_getState() const;
    MediumVariant_T i_getVariant() const;
    bool i_isHostDrive() const;
    const Utf8Str& i_getLocationFull() const;
    const Utf8Str& i_getFormat() const;
    const ComObjPtr<MediumFormat> & i_getMediumFormat() const;
    bool i_isMediumFormatFile() const;
    uint64_t i_getSize() const;
    uint64_t i_getLogicalSize() const;
    DeviceType_T i_getDeviceType() const;
    MediumType_T i_getType() const;
    Utf8Str i_getName();

    /* handles caller/locking itself */
    bool i_addRegistry(const Guid &id);
    /* handles caller/locking itself, caller is responsible for tree lock */
    bool i_addRegistryRecursive(const Guid &id);
    /* handles caller/locking itself */
    bool i_removeRegistry(const Guid& id);
    /* handles caller/locking itself, caller is responsible for tree lock */
    bool i_removeRegistryRecursive(const Guid& id);
    bool i_isInRegistry(const Guid& id);
    bool i_getFirstRegistryMachineId(Guid &uuid) const;
    void i_markRegistriesModified();

    HRESULT i_setPropertyDirect(const Utf8Str &aName, const Utf8Str &aValue);

    HRESULT i_addBackReference(const Guid &aMachineId,
                               const Guid &aSnapshotId = Guid::Empty);
    HRESULT i_removeBackReference(const Guid &aMachineId,
                                  const Guid &aSnapshotId = Guid::Empty);


    const Guid* i_getFirstMachineBackrefId() const;
    const Guid* i_getAnyMachineBackref() const;
    const Guid* i_getFirstMachineBackrefSnapshotId() const;
    size_t i_getMachineBackRefCount() const;

#ifdef DEBUG
    void i_dumpBackRefs();
#endif

    HRESULT i_updatePath(const Utf8Str &strOldPath, const Utf8Str &strNewPath);

    /* handles caller/locking itself */
    ComObjPtr<Medium> i_getBase(uint32_t *aLevel = NULL);
    /* handles caller/locking itself */
    uint32_t i_getDepth();

    bool i_isReadOnly();
    void i_updateId(const Guid &id);

    void i_saveSettingsOne(settings::Medium &data,
                           const Utf8Str &strHardDiskFolder);
    HRESULT i_saveSettings(settings::Medium &data,
                           const Utf8Str &strHardDiskFolder);

    HRESULT i_createMediumLockList(bool fFailIfInaccessible,
                                   Medium *pToLock,
                                   bool fMediumLockWriteAll,
                                   Medium *pToBeParent,
                                   MediumLockList &mediumLockList);

    HRESULT i_createDiffStorage(ComObjPtr<Medium> &aTarget,
                                MediumVariant_T aVariant,
                                MediumLockList *pMediumLockList,
                                ComObjPtr<Progress> *aProgress,
                                bool aWait);
    Utf8Str i_getPreferredDiffFormat();
    MediumVariant_T i_getPreferredDiffVariant();

    HRESULT i_close(AutoCaller &autoCaller);
    HRESULT i_unlockRead(MediumState_T *aState);
    HRESULT i_unlockWrite(MediumState_T *aState);
    HRESULT i_deleteStorage(ComObjPtr<Progress> *aProgress, bool aWait);
    HRESULT i_markForDeletion();
    HRESULT i_unmarkForDeletion();
    HRESULT i_markLockedForDeletion();
    HRESULT i_unmarkLockedForDeletion();

    HRESULT i_queryPreferredMergeDirection(const ComObjPtr<Medium> &pOther,
                                           bool &fMergeForward);

    HRESULT i_prepareMergeTo(const ComObjPtr<Medium> &pTarget,
                             const Guid *aMachineId,
                             const Guid *aSnapshotId,
                             bool fLockMedia,
                             bool &fMergeForward,
                             ComObjPtr<Medium> &pParentForTarget,
                             MediumLockList * &aChildrenToReparent,
                             MediumLockList * &aMediumLockList);
    HRESULT i_mergeTo(const ComObjPtr<Medium> &pTarget,
                      bool fMergeForward,
                      const ComObjPtr<Medium> &pParentForTarget,
                      MediumLockList *aChildrenToReparent,
                      MediumLockList *aMediumLockList,
                      ComObjPtr<Progress> *aProgress,
                      bool aWait);
    void i_cancelMergeTo(MediumLockList *aChildrenToReparent,
                       MediumLockList *aMediumLockList);

    HRESULT i_fixParentUuidOfChildren(MediumLockList *pChildrenToReparent);

    HRESULT i_addRawToFss(const char *aFilename, SecretKeyStore *pKeyStore, RTVFSFSSTREAM hVfsFssDst,
                          const ComObjPtr<Progress> &aProgress, bool fSparse);

    HRESULT i_exportFile(const char *aFilename,
                         const ComObjPtr<MediumFormat> &aFormat,
                         MediumVariant_T aVariant,
                         SecretKeyStore *pKeyStore,
                         RTVFSIOSTREAM hVfsIosDst,
                         const ComObjPtr<Progress> &aProgress);
    HRESULT i_importFile(const char *aFilename,
                        const ComObjPtr<MediumFormat> &aFormat,
                        MediumVariant_T aVariant,
                        RTVFSIOSTREAM hVfsIosSrc,
                        const ComObjPtr<Medium> &aParent,
                        const ComObjPtr<Progress> &aProgress);

    HRESULT i_cloneToEx(const ComObjPtr<Medium> &aTarget, ULONG aVariant,
                        const ComObjPtr<Medium> &aParent, IProgress **aProgress,
                        uint32_t idxSrcImageSame, uint32_t idxDstImageSame);

    const Utf8Str& i_getKeyId();

private:

    // wrapped IMedium properties
    HRESULT getId(com::Guid &aId);
    HRESULT getDescription(com::Utf8Str &aDescription);
    HRESULT setDescription(const com::Utf8Str &aDescription);
    HRESULT getState(MediumState_T *aState);
    HRESULT getVariant(std::vector<MediumVariant_T> &aVariant);
    HRESULT getLocation(com::Utf8Str &aLocation);
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getDeviceType(DeviceType_T *aDeviceType);
    HRESULT getHostDrive(BOOL *aHostDrive);
    HRESULT getSize(LONG64 *aSize);
    HRESULT getFormat(com::Utf8Str &aFormat);
    HRESULT getMediumFormat(ComPtr<IMediumFormat> &aMediumFormat);
    HRESULT getType(AutoCaller &autoCaller, MediumType_T *aType);
    HRESULT setType(AutoCaller &autoCaller, MediumType_T aType);
    HRESULT getAllowedTypes(std::vector<MediumType_T> &aAllowedTypes);
    HRESULT getParent(AutoCaller &autoCaller, ComPtr<IMedium> &aParent);
    HRESULT getChildren(AutoCaller &autoCaller, std::vector<ComPtr<IMedium> > &aChildren);
    HRESULT getBase(AutoCaller &autoCaller, ComPtr<IMedium> &aBase);
    HRESULT getReadOnly(AutoCaller &autoCaller, BOOL *aReadOnly);
    HRESULT getLogicalSize(LONG64 *aLogicalSize);
    HRESULT getAutoReset(BOOL *aAutoReset);
    HRESULT setAutoReset(BOOL aAutoReset);
    HRESULT getLastAccessError(com::Utf8Str &aLastAccessError);
    HRESULT getMachineIds(std::vector<com::Guid> &aMachineIds);

    // wrapped IMedium methods
    HRESULT setIds(AutoCaller &aAutoCaller,
                   BOOL aSetImageId,
                   const com::Guid &aImageId,
                   BOOL aSetParentId,
                   const com::Guid &aParentId);
    HRESULT refreshState(AutoCaller &aAutoCaller,
                         MediumState_T *aState);
    HRESULT getSnapshotIds(const com::Guid &aMachineId,
                           std::vector<com::Guid> &aSnapshotIds);
    HRESULT lockRead(ComPtr<IToken> &aToken);
    HRESULT lockWrite(ComPtr<IToken> &aToken);
    HRESULT close(AutoCaller &aAutoCaller);
    HRESULT getProperty(const com::Utf8Str &aName,
                        com::Utf8Str &aValue);
    HRESULT setProperty(const com::Utf8Str &aName,
                        const com::Utf8Str &aValue);
    HRESULT getProperties(const com::Utf8Str &aNames,
                          std::vector<com::Utf8Str> &aReturnNames,
                          std::vector<com::Utf8Str> &aReturnValues);
    HRESULT setProperties(const std::vector<com::Utf8Str> &aNames,
                          const std::vector<com::Utf8Str> &aValues);
    HRESULT createBaseStorage(LONG64 aLogicalSize,
                              const std::vector<MediumVariant_T> &aVariant,
                              ComPtr<IProgress> &aProgress);
    HRESULT deleteStorage(ComPtr<IProgress> &aProgress);
    HRESULT createDiffStorage(AutoCaller &autoCaller,
                              const ComPtr<IMedium> &aTarget,
                              const std::vector<MediumVariant_T> &aVariant,
                              ComPtr<IProgress> &aProgress);
    HRESULT mergeTo(const ComPtr<IMedium> &aTarget,
                    ComPtr<IProgress> &aProgress);
    HRESULT cloneTo(const ComPtr<IMedium> &aTarget,
                    const std::vector<MediumVariant_T> &aVariant,
                    const ComPtr<IMedium> &aParent,
                    ComPtr<IProgress> &aProgress);
    HRESULT cloneToBase(const ComPtr<IMedium> &aTarget,
                        const std::vector<MediumVariant_T> &aVariant,
                        ComPtr<IProgress> &aProgress);
    HRESULT setLocation(const com::Utf8Str &aLocation,
                        ComPtr<IProgress> &aProgress);
    HRESULT compact(ComPtr<IProgress> &aProgress);
    HRESULT resize(LONG64 aLogicalSize,
                   ComPtr<IProgress> &aProgress);
    HRESULT reset(AutoCaller &autoCaller, ComPtr<IProgress> &aProgress);
    HRESULT changeEncryption(const com::Utf8Str &aCurrentPassword, const com::Utf8Str &aCipher,
                             const com::Utf8Str &aNewPassword, const com::Utf8Str &aNewPasswordId,
                             ComPtr<IProgress> &aProgress);
    HRESULT getEncryptionSettings(com::Utf8Str &aCipher, com::Utf8Str &aPasswordId);
    HRESULT checkEncryptionPassword(const com::Utf8Str &aPassword);

    // Private internal nmethods
    HRESULT i_queryInfo(bool fSetImageId, bool fSetParentId, AutoCaller &autoCaller);
    HRESULT i_canClose();
    HRESULT i_unregisterWithVirtualBox();
    HRESULT i_setStateError();
    HRESULT i_setLocation(const Utf8Str &aLocation, const Utf8Str &aFormat = Utf8Str::Empty);
    HRESULT i_setFormat(const Utf8Str &aFormat);
    VDTYPE i_convertDeviceType();
    DeviceType_T i_convertToDeviceType(VDTYPE enmType);
    Utf8Str i_vdError(int aVRC);

    bool    i_isPropertyForFilter(const com::Utf8Str &aName);

    HRESULT i_getFilterProperties(std::vector<com::Utf8Str> &aReturnNames,
                                  std::vector<com::Utf8Str> &aReturnValues);

    HRESULT i_preparationForMoving(const Utf8Str &aLocation);
    bool    i_isMoveOperation(const ComObjPtr<Medium> &pTarget) const;
    bool    i_resetMoveOperationData();
    Utf8Str i_getNewLocationForMoving() const;

    static DECLCALLBACK(void) i_vdErrorCall(void *pvUser, int rc, RT_SRC_POS_DECL,
                                            const char *pszFormat, va_list va);
    static DECLCALLBACK(bool) i_vdConfigAreKeysValid(void *pvUser,
                                                     const char *pszzValid);
    static DECLCALLBACK(int) i_vdConfigQuerySize(void *pvUser, const char *pszName,
                                                 size_t *pcbValue);
    static DECLCALLBACK(int) i_vdConfigQuery(void *pvUser, const char *pszName,
                                             char *pszValue, size_t cchValue);
    static DECLCALLBACK(int) i_vdTcpSocketCreate(uint32_t fFlags, PVDSOCKET pSock);
    static DECLCALLBACK(int) i_vdTcpSocketDestroy(VDSOCKET Sock);
    static DECLCALLBACK(int) i_vdTcpClientConnect(VDSOCKET Sock, const char *pszAddress, uint32_t uPort,
                                                  RTMSINTERVAL cMillies);
    static DECLCALLBACK(int) i_vdTcpClientClose(VDSOCKET Sock);
    static DECLCALLBACK(bool) i_vdTcpIsClientConnected(VDSOCKET Sock);
    static DECLCALLBACK(int) i_vdTcpSelectOne(VDSOCKET Sock, RTMSINTERVAL cMillies);
    static DECLCALLBACK(int) i_vdTcpRead(VDSOCKET Sock, void *pvBuffer, size_t cbBuffer, size_t *pcbRead);
    static DECLCALLBACK(int) i_vdTcpWrite(VDSOCKET Sock, const void *pvBuffer, size_t cbBuffer);
    static DECLCALLBACK(int) i_vdTcpSgWrite(VDSOCKET Sock, PCRTSGBUF pSgBuf);
    static DECLCALLBACK(int) i_vdTcpFlush(VDSOCKET Sock);
    static DECLCALLBACK(int) i_vdTcpSetSendCoalescing(VDSOCKET Sock, bool fEnable);
    static DECLCALLBACK(int) i_vdTcpGetLocalAddress(VDSOCKET Sock, PRTNETADDR pAddr);
    static DECLCALLBACK(int) i_vdTcpGetPeerAddress(VDSOCKET Sock, PRTNETADDR pAddr);

    static DECLCALLBACK(bool) i_vdCryptoConfigAreKeysValid(void *pvUser,
                                                           const char *pszzValid);
    static DECLCALLBACK(int) i_vdCryptoConfigQuerySize(void *pvUser, const char *pszName,
                                                       size_t *pcbValue);
    static DECLCALLBACK(int) i_vdCryptoConfigQuery(void *pvUser, const char *pszName,
                                                   char *pszValue, size_t cchValue);

    static DECLCALLBACK(int) i_vdCryptoKeyRetain(void *pvUser, const char *pszId,
                                                 const uint8_t **ppbKey, size_t *pcbKey);
    static DECLCALLBACK(int) i_vdCryptoKeyRelease(void *pvUser, const char *pszId);
    static DECLCALLBACK(int) i_vdCryptoKeyStorePasswordRetain(void *pvUser, const char *pszId, const char **ppszPassword);
    static DECLCALLBACK(int) i_vdCryptoKeyStorePasswordRelease(void *pvUser, const char *pszId);
    static DECLCALLBACK(int) i_vdCryptoKeyStoreSave(void *pvUser, const void *pvKeyStore, size_t cbKeyStore);
    static DECLCALLBACK(int) i_vdCryptoKeyStoreReturnParameters(void *pvUser, const char *pszCipher,
                                                                const uint8_t *pbDek, size_t cbDek);

    struct CryptoFilterSettings;
    HRESULT i_openHddForReading(SecretKeyStore *pKeyStore, PVDISK *ppHdd, MediumLockList *pMediumLockList,
                                struct CryptoFilterSettings *pCryptoSettingsRead);

    class Task;
    class CreateBaseTask;
    class CreateDiffTask;
    class CloneTask;
    class MoveTask;
    class CompactTask;
    class ResizeTask;
    class ResetTask;
    class DeleteTask;
    class MergeTask;
    class ImportTask;
    class EncryptTask;
    friend class Task;
    friend class CreateBaseTask;
    friend class CreateDiffTask;
    friend class CloneTask;
    friend class MoveTask;
    friend class CompactTask;
    friend class ResizeTask;
    friend class ResetTask;
    friend class DeleteTask;
    friend class MergeTask;
    friend class ImportTask;
    friend class EncryptTask;

    HRESULT i_taskCreateBaseHandler(Medium::CreateBaseTask &task);
    HRESULT i_taskCreateDiffHandler(Medium::CreateDiffTask &task);
    HRESULT i_taskMergeHandler(Medium::MergeTask &task);
    HRESULT i_taskCloneHandler(Medium::CloneTask &task);
    HRESULT i_taskMoveHandler(Medium::MoveTask &task);
    HRESULT i_taskDeleteHandler(Medium::DeleteTask &task);
    HRESULT i_taskResetHandler(Medium::ResetTask &task);
    HRESULT i_taskCompactHandler(Medium::CompactTask &task);
    HRESULT i_taskResizeHandler(Medium::ResizeTask &task);
    HRESULT i_taskImportHandler(Medium::ImportTask &task);
    HRESULT i_taskEncryptHandler(Medium::EncryptTask &task);

    void i_taskEncryptSettingsSetup(CryptoFilterSettings *pSettings, const char *pszCipher,
                                    const char *pszKeyStore,  const char *pszPassword,
                                    bool fCreateKeyStore);

    struct Data;            // opaque data struct, defined in MediumImpl.cpp
    Data *m;
};

#endif /* !____H_MEDIUMIMPL */

