// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ISimObjectDIS.Legacy.h
// Provides public interfaces for dll implementations
//-----------------------------------------------------------------------------------

#pragma once

#include "..\Types.h"

namespace P3D
{
interface IPduReaderV440;
interface IPduBuilderV440;
interface IPduCallbackV440;

/** ## *Professional Plus Only*
* This service allows the developer to interact and retrieve information with a distributed interactive simulation (DIS) session. 
* Developers integrating with this interface should be familiar with and are expected to follow DIS IEEE standards. This service is 
* provided by the IPdk interface.*/
DECLARE_INTERFACE_(IDISManagerV400, IUnknown)
{
    /** Returns TRUE if a DIS connection is active, FALSE otherwise. */
    STDMETHOD_(BOOL, IsConnected)() const PURE;

    /** Provides the EntityType for the given object ID if successful. */
    STDMETHOD (GetEntityTypeById)(__in UINT32 uID, __out P3D::EntityType& EntityType) const PURE;

    /** Used to issue a Fire PDU. */
    STDMETHOD (NotifyMunitionFired)(__in UINT32 uAttackerID,                        /**< The object ID of the firing entity */
                                    __in UINT32 uTargetID,                          /**< The object ID of the target entity if available, 0 otherwise */
                                    __in UINT32 uMunitionID,                        /**< The object ID of the munition entity if available, 0 otherwise */
                                    __in const P3D::EntityType& EntityType,         /**< The EntityType of the munition */
                                    __in const P3D::DXYZ& xyzLonAltLat,             /**< World location in radians and feet */
                                    __in const P3D::DXYZ& xyzLinearVelocity,        /**< World velocity in feet per second */
                                    __in unsigned short usWarheadType,              /**< The warhead type */
                                    __in unsigned short usFuseType,                 /**< The fuse type */
                                    __in unsigned short usQuantity,                 /**< The quantity of munitions represented */
                                    __in unsigned short usRate,                     /**< Rounds per minute */
                                    __in float fRange,                              /**< Meters */
                                    __inout unsigned short& usEventID               /**< Set to 0 for new Fire PDU or previously returned value to signify continuous firing */
                                    ) PURE;

    /** Used to issue a Detonation PDU. */
    STDMETHOD (NotifyMunitionDetonated)(__in UINT32 uAttackerID,                    /**< The object ID of the firing entity */
                                        __in UINT32 uTargetID,                      /**< The object ID of the target entity if available, 0 otherwise */
                                        __in UINT32 uMunitionID,                    /**< The object ID of the munition entity if available, 0 otherwise */
                                        __in const P3D::EntityType& EntityType,     /**< The EntityType of the munition */
                                        __in unsigned short usEventID,              /**< The event ID from an associated Fire PDU if available, 0 otherwise */
                                        __in const P3D::DXYZ& xyzLonAltLat,         /**< Radians/feet */
                                        __in const P3D::DXYZ& xyzLinearVelocity,    /**< World/FPS */
                                        __in unsigned short usWarheadType,          /**< The warhead type */
                                        __in unsigned short usFuseType,             /**< The fuse type */
                                        __in unsigned short usQuantity,             /**< The quantity of munitions represented */
                                        __in unsigned short usRate,                 /**< The rate of fire in rounds per minute */
                                        __in unsigned char yDetonationResult        /**< The result of the detonation */
                                        ) PURE;
};
DEFINE_GUID(IID_IDISManagerV400,  0xcfd6eb04, 0xf23a, 0x4f00, 0x85, 0xe7, 0x6c, 0x55, 0xba, 0xcd, 0x27, 0xf7);


DECLARE_INTERFACE_(IDISManagerV420, IDISManagerV400)
{
    /** Returns the entity identifier for the given object id if successful. */
    STDMETHOD(GetEntityIdByObjectId)(__in UINT32 uObjectId,                         /**< The object id of the request */
                                     __out unsigned short& usSiteId,                /**< The site id of the entity identifier */
                                     __out unsigned short& usApplicationId,         /**< The application id of the entity identifier */
                                     __out unsigned short& usEntityId               /**< The entity/object id of the entity identifier */
                                     ) PURE;

    /** Returns the object id for the given entity identifier if successful. */
    STDMETHOD(GetObjectIdByEntityId)(__in unsigned short usSiteId,                  /**< The site id of the entity identifier */
                                     __in unsigned short usApplicationId,           /**< The application id of the entity identifier */
                                     __in unsigned short usEntityId,                /**< The entity/object id of the entity identifier */
                                     __out UINT32& uObjectId                        /**< The object id of the request */
                                     ) PURE;
};
DEFINE_GUID(IID_IDISManagerV420, 0x3cf01842, 0x3127, 0x4259, 0x83, 0xee, 0x71, 0x75, 0xfa, 0x2e, 0x26, 0xf5);

DECLARE_INTERFACE_(IDISManagerV440, IDISManagerV420)
{
    /** Returns an IPduBuilderV440 interface with a reference count of 1. This interface can be used to build PDU's to be used with the
    IssuePdu function. Developers should release this object after it has been issued using the IssuePdu function.

    Sample implementation:
    *```
    P3D::IPduBuilderV440* pPdu = spDIS->CreatePdu();

    // Write PDU header
    pPdu->WriteUChar(6);
    pPdu->WriteUChar(spDIS->GetExerciseId());
    pPdu->WriteUChar(1);
    pPdu->WriteUChar(1);
    pPdu->WriteUInt(spDIS->GetSimTimestamp());
    // ...

    // Write remaining PDU specific data
    // ...

    spDIS->IssuePdu(pPdu);

    pPdu->Release();
    pPdu = nullptr;
    *```
    @remarks It is the developer's responsibility to fill in the PDU header as well as any necessary data.
    */
    STDMETHOD_(P3D::IPduBuilderV440*, CreatePdu)() PURE;

    /** Informs core P3D to queue the given IPduBuilderV440 interface data to be broadcast across the network.
    @remarks The IPduBuilderV440 object can be created with a call to CreatePdu.
    @remarks This function does not add a ref to the given IPduBuilderV440 interface. */
    STDMETHOD(IssuePdu)(P3D::IPduBuilderV440* pPduBuilder) PURE;

    /** Registers a PDU callback. */
    STDMETHOD(RegisterPduCallback)(__in BYTE yPduType, __in __notnull IPduCallbackV440* pCallback) PURE;

    /** Unregisters a PDU callback. */
    STDMETHOD(UnregisterPduCallback)(__in BYTE yPduType, __in __notnull IPduCallbackV440* pCallback) PURE;

    /** Returns the session's current site id. */
    STDMETHOD_(USHORT, GetSiteId)() const PURE;

    /** Returns the session's current application id. */
    STDMETHOD_(USHORT, GetApplicationId)() const PURE;

    /** Returns the session's current exercise id. */
    STDMETHOD_(BYTE, GetExerciseId)() const PURE;

    /** Creates and returns a unique event id for the session. This value should be used when creating
    PDU's with the IPduBuilderV440 interface that require an event ID. */
    STDMETHOD_(USHORT, GetEventId)() PURE;

    /** Returns the session's current wall clock hour since 0000 hours January 1, 1970 UTC. */
    STDMETHOD_(int, GetWallClockHour)() const PURE;

    /** Returns the session's current wall timestamp in DIS timestamp format.
    This value should be used when filling out the PDU header using the IPduBuilderV440 interface. */
    STDMETHOD_(UINT, GetWallTimestamp)() const PURE;

    /** Returns the session's current simulation clock hour since 0000 hours January 1, 1970 UTC. */
    STDMETHOD_(int, GetSimClockHour)() const PURE;

    /** Returns the session's current simulation timestamp in DIS timestamp format. */
    STDMETHOD_(UINT, GetSimTimestamp)() const PURE;
};
DEFINE_GUID(IID_IDISManagerV440, 0x6b4b829c, 0xf2a0, 0x45e6, 0x94, 0x58, 0xa5, 0xbd, 0x54, 0xa3, 0xd8, 0x6d);

/** ## *Professional Plus Only*
* This service allows the developer to interact and retrieve information with a distributed interactive simulation (DIS) session.
* Developers integrating with this interface should be familiar with and are expected to follow DIS IEEE standards. This service is
* provided by the IPdk interface.*/
DECLARE_INTERFACE_(IDISManagerV450, IDISManagerV440)
{
    /**
    * Plugins can toggle whether Prepar3D receives any packets.
    * @param        bDisableReceive     True to disable receiving, False to enable receiving.
    * @remarks      By default receiving is enabled.
    **/
    STDMETHOD(SetDisableReceive)(__in BOOL bDisableReceive) PURE;

    /**
    * Plugins can toggle whether Prepar3D sends any packets.
    * @param        bDisableSend        True to disable sending, False to enable sending.
    * @remarks      By default sending is enabled.
    **/
    STDMETHOD(SetDisableSend)(__in BOOL bDisableSend) PURE;
};

DEFINE_GUID(IID_IDISManagerV450, 0xd7767ca7, 0xa6f6, 0x4091, 0x9e, 0x89, 0x38, 0x57, 0x3f, 0x3e, 0xb5, 0xf2);

/** ## *Professional Plus Only*
* This service allows the developer to interact and retrieve information with a distributed interactive simulation (DIS) session.
* Developers integrating with this interface should be familiar with and are expected to follow DIS IEEE standards. This service is
* provided by the IPdk interface.*/
DECLARE_INTERFACE_(IDISManagerV520, IDISManagerV450)
{
    /** Converts a DIS timestamp to seconds past the hour. */
    STDMETHOD_(double, TimestampToSeconds)(__in UINT uTimestamp) const PURE;

    /** Converts seconds past the hour to a DIS timestamp. */
    STDMETHOD_(UINT, SecondsToTimestamp)(__in double dSeconds) const PURE;

    /** Converts DIS epoch clock time, referenced from 0000 hours January 1, 1970 UTC., to P3D absolute time in seconds, referenced from 12:00 AM January 1, 0000. */
    STDMETHOD_(double, ClockTimeToAbsoluteSeconds)(__in int iHour, __in UINT uTimestamp) const PURE;

    /** Converts P3D absolute time in seconds, referenced from 12:00 AM January 1, 0000, to DIS epoch clock time, referenced from 0000 hours January 1, 1970 UTC. */
    STDMETHOD_(void, AbsoluteSecondsToClockTime)(__in double dSeconds, __out int& iHour, __out UINT & uTimestamp) const PURE;
};

DEFINE_GUID(IID_IDISManagerV520, 0x9a7cf2b4, 0xf3b7, 0x40e7, 0xad, 0xa4, 0xc1, 0x98, 0x53, 0x61, 0xa, 0xbb);

/**************************************************************************************************************
**************************************************************************************************************/

/** @} */

} //End namespace P3D
