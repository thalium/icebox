/* $Id: AudioAdapterImpl.cpp $ */
/** @file
 *
 * VirtualBox COM class implementation
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

#include "AudioAdapterImpl.h"
#include "MachineImpl.h"

#include <iprt/cpp/utils.h>

#include <VBox/settings.h>

#include "AutoStateDep.h"
#include "AutoCaller.h"
#include "Logging.h"


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

AudioAdapter::AudioAdapter()
    : mParent(NULL)
{
}

AudioAdapter::~AudioAdapter()
{
}

HRESULT AudioAdapter::FinalConstruct()
{
    return BaseFinalConstruct();
}

void AudioAdapter::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 *  Initializes the audio adapter object.
 *
 *  @param aParent  Handle of the parent object.
 */
HRESULT AudioAdapter::init(Machine *aParent)
{
    LogFlowThisFunc(("aParent=%p\n", aParent));

    ComAssertRet(aParent, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    /* Get the default audio driver out of the system properties */
    ComPtr<IVirtualBox> VBox;
    HRESULT rc = aParent->COMGETTER(Parent)(VBox.asOutParam());
    if (FAILED(rc)) return rc;
    ComPtr<ISystemProperties> sysProps;
    rc = VBox->COMGETTER(SystemProperties)(sysProps.asOutParam());
    if (FAILED(rc)) return rc;
    AudioDriverType_T defaultAudioDriver;
    rc = sysProps->COMGETTER(DefaultAudioDriver)(&defaultAudioDriver);
    if (FAILED(rc)) return rc;

    unconst(mParent) = aParent;
    /* mPeer is left null */

    mData.allocate();
    mData->driverType = defaultAudioDriver;
    mData->fEnabledIn = false;
    mData->fEnabledOut = false;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Initializes the audio adapter object given another audio adapter object
 *  (a kind of copy constructor). This object shares data with
 *  the object passed as an argument.
 *
 *  @note This object must be destroyed before the original object
 *  it shares data with is destroyed.
 *
 *  @note Locks @a aThat object for reading.
 */
HRESULT AudioAdapter::init(Machine *aParent, AudioAdapter *aThat)
{
    LogFlowThisFunc(("aParent=%p, aThat=%p\n", aParent, aThat));

    ComAssertRet(aParent && aThat, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;
    unconst(mPeer) = aThat;

    AutoCaller thatCaller(aThat);
    AssertComRCReturnRC(thatCaller.rc());

    AutoReadLock thatLock(aThat COMMA_LOCKVAL_SRC_POS);
    mData.share(aThat->mData);

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Initializes the guest object given another guest object
 *  (a kind of copy constructor). This object makes a private copy of data
 *  of the original object passed as an argument.
 *
 *  @note Locks @a aThat object for reading.
 */
HRESULT AudioAdapter::initCopy(Machine *aParent, AudioAdapter *aThat)
{
    LogFlowThisFunc(("aParent=%p, aThat=%p\n", aParent, aThat));

    ComAssertRet(aParent && aThat, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;
    /* mPeer is left null */

    AutoCaller thatCaller(aThat);
    AssertComRCReturnRC(thatCaller.rc());

    AutoReadLock thatLock(aThat COMMA_LOCKVAL_SRC_POS);
    mData.attachCopy(aThat->mData);

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void AudioAdapter::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    mData.free();

    unconst(mPeer) = NULL;
    unconst(mParent) = NULL;
}

// IAudioAdapter properties
/////////////////////////////////////////////////////////////////////////////

HRESULT AudioAdapter::getEnabled(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aEnabled = mData->fEnabled;

    return S_OK;
}

HRESULT AudioAdapter::setEnabled(BOOL aEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->fEnabled != RT_BOOL(aEnabled))
    {
        mData.backup();
        mData->fEnabled = RT_BOOL(aEnabled);

        alock.release();
        AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
        mParent->i_setModified(Machine::IsModified_AudioAdapter);
    }

    return S_OK;
}

HRESULT AudioAdapter::getEnabledIn(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aEnabled = mData->fEnabledIn;

    return S_OK;
}

HRESULT AudioAdapter::setEnabledIn(BOOL aEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableOrSavedOrRunningStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (RT_BOOL(aEnabled) != mData->fEnabledIn)
    {
        mData.backup();
        mData->fEnabledIn = RT_BOOL(aEnabled);

        // leave the lock before informing callbacks
        alock.release();

        AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);
        mParent->i_setModified(Machine::IsModified_AudioAdapter);
        mlock.release();

        mParent->i_onAudioAdapterChange(this);
    }

    return S_OK;
}

HRESULT AudioAdapter::getEnabledOut(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aEnabled = mData->fEnabledOut;

    return S_OK;
}

HRESULT AudioAdapter::setEnabledOut(BOOL aEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableOrSavedOrRunningStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (RT_BOOL(aEnabled) != mData->fEnabledOut)
    {
        mData.backup();
        mData->fEnabledOut = RT_BOOL(aEnabled);

        // leave the lock before informing callbacks
        alock.release();

        AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);
        mParent->i_setModified(Machine::IsModified_AudioAdapter);
        mlock.release();

        mParent->i_onAudioAdapterChange(this);
    }

    return S_OK;
}

HRESULT AudioAdapter::getAudioDriver(AudioDriverType_T *aAudioDriver)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAudioDriver = mData->driverType;

    return S_OK;
}

HRESULT AudioAdapter::setAudioDriver(AudioDriverType_T aAudioDriver)
{

    /* the machine needs to be mutable */
    AutoMutableOrSavedStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    if (mData->driverType != aAudioDriver)
    {
        if (settings::MachineConfigFile::isAudioDriverAllowedOnThisHost(aAudioDriver))
        {
            mData.backup();
            mData->driverType = aAudioDriver;
            alock.release();
            AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
            mParent->i_setModified(Machine::IsModified_AudioAdapter);
        }
        else
        {
            AssertMsgFailed(("Wrong audio driver type %d\n", aAudioDriver));
            rc = E_FAIL;
        }
    }

    return rc;
}

HRESULT AudioAdapter::getAudioController(AudioControllerType_T *aAudioController)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAudioController = mData->controllerType;

    return S_OK;
}

HRESULT AudioAdapter::setAudioController(AudioControllerType_T aAudioController)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    if (mData->controllerType != aAudioController)
    {
        AudioCodecType_T defaultCodec;

        /*
         * which audio hardware type are we supposed to use?
         */
        switch (aAudioController)
        {
            /* codec type needs to match the controller. */
            case AudioControllerType_AC97:
                defaultCodec = AudioCodecType_STAC9700;
                break;
            case AudioControllerType_SB16:
                defaultCodec = AudioCodecType_SB16;
                break;
            case AudioControllerType_HDA:
                defaultCodec = AudioCodecType_STAC9221;
                break;

            default:
                AssertMsgFailed(("Wrong audio controller type %d\n", aAudioController));
                defaultCodec = AudioCodecType_Null; /* Shut up MSC */
                rc = E_FAIL;
        }
        if (rc == S_OK)
        {
            mData.backup();
            mData->controllerType = aAudioController;
            mData->codecType = defaultCodec;
            alock.release();
            AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
            mParent->i_setModified(Machine::IsModified_AudioAdapter);
        }
    }

    return rc;
}

HRESULT AudioAdapter::getAudioCodec(AudioCodecType_T *aAudioCodec)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAudioCodec = mData->codecType;

    return S_OK;
}

HRESULT AudioAdapter::setAudioCodec(AudioCodecType_T aAudioCodec)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.rc())) return adep.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /*
     * ensure that the codec type matches the audio controller
     */
    switch (mData->controllerType)
    {
        case AudioControllerType_AC97:
        {
            if (   (aAudioCodec != AudioCodecType_STAC9700)
                && (aAudioCodec != AudioCodecType_AD1980))
                rc = E_INVALIDARG;
            break;
        }

        case AudioControllerType_SB16:
        {
            if (aAudioCodec != AudioCodecType_SB16)
                rc = E_INVALIDARG;
            break;
        }

        case AudioControllerType_HDA:
        {
            if (aAudioCodec != AudioCodecType_STAC9221)
                rc = E_INVALIDARG;
            break;
        }

        default:
            AssertMsgFailed(("Wrong audio controller type %d\n",
                             mData->controllerType));
            rc = E_FAIL;
    }

    if (!SUCCEEDED(rc))
        return setError(rc,
                        tr("Invalid audio codec type %d"),
                        aAudioCodec);

    if (mData->codecType != aAudioCodec)
    {
        mData.backup();
        mData->codecType = aAudioCodec;
        alock.release();
        AutoWriteLock mlock(mParent COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
        mParent->i_setModified(Machine::IsModified_AudioAdapter);
    }

    return rc;
}

HRESULT AudioAdapter::getPropertiesList(std::vector<com::Utf8Str>& aProperties)
{
    using namespace settings;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aProperties.resize(0);
    StringsMap::const_iterator cit = mData->properties.begin();
    while(cit != mData->properties.end())
    {
        Utf8Str key = cit->first;
        aProperties.push_back(cit->first);
        ++cit;
    }

    return S_OK;
}

HRESULT AudioAdapter::getProperty(const com::Utf8Str &aKey, com::Utf8Str &aValue)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    settings::StringsMap::const_iterator cit = mData->properties.find(aKey);
    if (cit != mData->properties.end())
        aValue = cit->second;

    return S_OK;
}

HRESULT AudioAdapter::setProperty(const com::Utf8Str &aKey, const com::Utf8Str &aValue)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Generic properties processing.
     * Look up the old value first; if nothing's changed then do nothing.
     */
    Utf8Str strOldValue;

    settings::StringsMap::const_iterator cit = mData->properties.find(aKey);
    if (cit != mData->properties.end())
        strOldValue = cit->second;

    if (strOldValue != aValue)
    {
        if (aValue.isEmpty())
            mData->properties.erase(aKey);
        else
            mData->properties[aKey] = aValue;
    }

    alock.release();

    return S_OK;
}

// IAudioAdapter methods
/////////////////////////////////////////////////////////////////////////////

// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

/**
 *  Loads settings from the given machine node.
 *  May be called once right after this object creation.
 *
 *  @param data Configuration settings.
 *
 *  @note Locks this object for writing.
 */
HRESULT AudioAdapter::i_loadSettings(const settings::AudioAdapter &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Note: we assume that the default values for attributes of optional
     * nodes are assigned in the Data::Data() constructor and don't do it
     * here. It implies that this method may only be called after constructing
     * a new AudioAdapter object while all its data fields are in the default
     * values. Exceptions are fields whose creation time defaults don't match
     * values that should be applied when these fields are not explicitly set
     * in the settings file (for backwards compatibility reasons). This takes
     * place when a setting of a newly created object must default to A while
     * the same setting of an object loaded from the old settings file must
     * default to B. */
    mData.assignCopy(&data);

    return S_OK;
}

/**
 *  Saves settings to the given machine node.
 *
 *  @param data Configuration settings.
 *
 *  @note Locks this object for reading.
 */
HRESULT AudioAdapter::i_saveSettings(settings::AudioAdapter &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data = *mData.data();

    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
void AudioAdapter::i_rollback()
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.rollback();
}

/**
 *  @note Locks this object for writing, together with the peer object (also
 *  for writing) if there is one.
 */
void AudioAdapter::i_commit()
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    /* sanity too */
    AutoCaller peerCaller(mPeer);
    AssertComRCReturnVoid(peerCaller.rc());

    /* lock both for writing since we modify both (mPeer is "master" so locked
     * first) */
    AutoMultiWriteLock2 alock(mPeer, this COMMA_LOCKVAL_SRC_POS);

    if (mData.isBackedUp())
    {
        mData.commit();
        if (mPeer)
        {
            /* attach new data to the peer and reshare it */
            mPeer->mData.attach(mData);
        }
    }
}

/**
 *  @note Locks this object for writing, together with the peer object
 *  represented by @a aThat (locked for reading).
 */
void AudioAdapter::i_copyFrom(AudioAdapter *aThat)
{
    AssertReturnVoid(aThat != NULL);

    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    /* sanity too */
    AutoCaller thatCaller(aThat);
    AssertComRCReturnVoid(thatCaller.rc());

    /* peer is not modified, lock it for reading (aThat is "master" so locked
     * first) */
    AutoReadLock rl(aThat COMMA_LOCKVAL_SRC_POS);
    AutoWriteLock wl(this COMMA_LOCKVAL_SRC_POS);

    /* this will back up current data */
    mData.assignCopy(aThat->mData);
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
