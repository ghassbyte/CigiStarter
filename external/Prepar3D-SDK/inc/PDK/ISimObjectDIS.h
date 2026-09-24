// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ISimObjectDIS.h
// Provides public interfaces for dll implementations
//-----------------------------------------------------------------------------------

#pragma once

#include "Legacy\ISimObjectDIS.Legacy.h"

namespace P3D
{

/** @addtogroup isim */ /** @{ */

class ArticulatedPart
{
public:

    unsigned char   m_yRecordType;
    unsigned char   m_yChangeIndicator;
    unsigned short  m_usAttachedToId;
    unsigned int    m_uiParameterType;
    float           m_fParameterValue;
    float           m_fPadding;
};

class ArticulatedParameter
{
public:
    union
    {
        ArticulatedPart m_ArticulatedPart;
    };
};

/**
* This interface allows developers to build PDU's on a per-field basis.
* @remarks It is the developer's responsibility to fill in the PDU header as well as any necessary data.
*/
DECLARE_INTERFACE_(IPduBuilderV440, IUnknown)
{
    STDMETHOD_(void, WriteChar)     (__in char c)               PURE; /**<Adds an 8-bit signed byte to the packet. */
    STDMETHOD_(void, WriteUChar)    (__in unsigned char uc)     PURE; /**<Adds an 8-bit unsigned short to the packet. */
    STDMETHOD_(void, WriteFloat)    (__in float f)              PURE; /**<Adds a 32-bit floating point to the packet. */
    STDMETHOD_(void, WriteDouble)   (__in double d)             PURE; /**<Adds a 64-bit floating point to the packet. */
    STDMETHOD_(void, WriteInt)      (__in int i)                PURE; /**<Adds a 32-bit signed integer to the packet. */
    STDMETHOD_(void, WriteUInt)     (__in unsigned int u)       PURE; /**<Adds a 32-bit unsigned integer to the packet. */
    STDMETHOD_(void, WriteLong)     (__in long l)               PURE; /**<Adds a 32-bit signed long to the packet. */
    STDMETHOD_(void, WriteULong)    (__in unsigned long ul)     PURE; /**<Adds a 32-bit unsigned long to the packet. */
    STDMETHOD_(void, WriteLongLong) (__in long long ll)         PURE; /**<Adds a 64-bit signed long to the packet. */
    STDMETHOD_(void, WriteUShort)   (__in unsigned short us)    PURE; /**<Adds a 16-bit unsigned short to the packet. */
    STDMETHOD_(void, WriteShort)    (__in short s)              PURE; /**<Adds a 16-bit signed short to the packet. */
    STDMETHOD_(int,  GetSize)       ()                    const PURE; /**<Returns the current size of the PDU in bytes. */
};

DEFINE_GUID(IID_IPduBuilderV440, 0xff332f2c, 0xb0c7, 0x4661, 0xa1, 0x27, 0xed, 0x38, 0x10, 0xc7, 0x81, 0x1f);

/**
* This interface allows developers to read PDU's on a per-field basis.
*/
DECLARE_INTERFACE_(IPduReaderV440, IUnknown)
{
    STDMETHOD_(char, ReadChar)()                    PURE; /**<Reads an 8-bit signed byte to the packet. */
    STDMETHOD_(unsigned char, ReadUChar)()          PURE; /**<Reads an 8-bit unsigned short to the packet. */
    STDMETHOD_(float, ReadFloat)()                  PURE; /**<Reads a 32-bit floating point to the packet. */
    STDMETHOD_(double, ReadDouble)()                PURE; /**<Reads a 64-bit floating point to the packet. */
    STDMETHOD_(int, ReadInt)()                      PURE; /**<Reads a 32-bit signed integer to the packet. */
    STDMETHOD_(unsigned int, ReadUInt)()            PURE; /**<Reads a 32-bit unsigned integer to the packet. */
    STDMETHOD_(long, ReadLong)()                    PURE; /**<Reads a 32-bit signed long to the packet. */
    STDMETHOD_(unsigned long, ReadULong)()          PURE; /**<Reads a 32-bit unsigned long to the packet. */
    STDMETHOD_(long long, ReadLongLong)()           PURE; /**<Reads a 64-bit signed long to the packet. */
    STDMETHOD_(unsigned short, ReadUShort)()        PURE; /**<Reads a 16-bit unsigned short to the packet. */
    STDMETHOD_(short, ReadShort)()                  PURE; /**<Reads a 16-bit signed short to the packet. */
    STDMETHOD_(const char*, GetRawData)()   const   PURE; /**<Returns the current data inside of the PDU. */
    STDMETHOD_(UINT, GetSize)()             const   PURE; /**<Returns the current size of the PDU in bytes. */
};

DEFINE_GUID(IID_IPduReaderV440, 0xc1bbc504, 0xf113, 0x4ef0, 0x98, 0x46, 0x1a, 0xcf, 0xa4, 0x49, 0x16, 0x2a);

/**
* This interface allows developers to create PDU's to be sent or received.
**/
DECLARE_INTERFACE_(IPduCallbackV440, IUnknown)
{
    /**
    * Plugins should implement this function to receive callbacks when Prepar3D is about to send a packet.
    * @remarks Returning anything other than S_OK will prevent the packet from being sent over the network.
    **/
    STDMETHOD (OnSend)(__in IPduReaderV440* pReader, __in BYTE uPduType) PURE;

    /**
    * Plugins should implement this function to receive callbacks when Prepar3D has received a packet.
    * @remarks Returning anything other than S_OK will prevent the packet from being processed within Prepar3D.
    **/
    STDMETHOD (OnReceive)(__in IPduReaderV440* pReader, __in BYTE uPduType) PURE;
};

DEFINE_GUID(IID_IPduCallbackV440, 0x53979fa9, 0xc69b, 0x436e, 0xb3, 0x72, 0xde, 0xe7, 0x15, 0x34, 0xb3, 0x7a);

/** ## *Professional Plus Only*
* This service allows the developer to interact and retrieve information with a distributed interactive simulation (DIS) session. 
* Developers integrating with this interface should be familiar with and are expected to follow DIS IEEE standards. This service is 
* provided by the IPdk interface.*/
DECLARE_INTERFACE_(IDISManagerV540, IDISManagerV520)
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

    /** Converts a DIS timestamp to seconds past the hour. */
    STDMETHOD_(double, TimestampToSeconds)(__in UINT uTimestamp) const PURE;

    /** Converts seconds past the hour to a DIS timestamp. Relative or absolute will be chosen based on current user setting. */
    STDMETHOD_(UINT, SecondsToTimestamp)(__in double dSeconds) const PURE;

    /** Converts seconds past the hour to a DIS timestamp. Relative or absolute is specified by the caller. */
    STDMETHOD_(UINT, SecondsToTimestamp)(__in double dSeconds, __in bool bRelative) const PURE;

    /** Converts DIS epoch clock time, referenced from 0000 hours January 1, 1970 UTC., to P3D absolute time in seconds, referenced from 12:00 AM January 1, 0000. */
    STDMETHOD_(double, ClockTimeToAbsoluteSeconds)(__in int iHour, __in UINT uTimestamp) const PURE;

    /** Converts P3D absolute time in seconds, referenced from 12:00 AM January 1, 0000, to DIS epoch clock time, referenced from 0000 hours January 1, 1970 UTC. */
    STDMETHOD_(void, AbsoluteSecondsToClockTime)(__in double dSeconds, __out int& iHour, __out UINT& uTimestamp) const PURE;

    /** Returns the session's relative timestamp in DIS timestamp format. */
    STDMETHOD_(UINT, GetRelativeTimestamp)() const PURE;

    /** Returns TRUE if the session prefers relative timestamps vs. absolute timestamps based on user setting. */
    STDMETHOD_(BOOL, IsRelativeTimestampEnabled)() const PURE;
};

DEFINE_GUID(IID_IDISManagerV540,    0x10d44188, 0xb383, 0x4599, 0xa1, 0xdf, 0x2c, 0x2b, 0x57, 0xa6, 0x55, 0x1c);
DEFINE_GUID(SID_DISManager,         0x84573c4c, 0x6422, 0x43f7, 0xb0, 0xe3, 0x26, 0x87, 0x68, 0x9e, 0x40, 0x9);

/** ## *Professional Plus Only*
* This service allows developers to provide Distributed Interactive Simulation (DIS) information to the core simulation. 
* Developers should implement this service and provide the requested information following DIS IEEE standards. */
DECLARE_INTERFACE_(IDISServiceV400, IUnknown)
{
    /** This function will be called when the application requires the object's entity appearance. The entity appearance is a 32-bit unsigned integer. 
    * The application expects the data to be packed according to DIS standards. The application expects the appearance type to match that of the entity 
    * type and domain (platform, air, land, munition, expendable, etc.). The function should return S_OK if the entity appearance is being provided by 
    * the ISimObject. */
    STDMETHOD (SerializeEntityAppearance)(__inout UINT& iAppearance) PURE;
    /** This function will be called on remote entities when the appearance needs to be updated. The data should be deserialized in the same manner as 
    * described above. The function should return S_OK if the entity appearance was correctly deserialized by the ISimObject. */
    STDMETHOD (DeserializeEntityAppearance)(__in UINT iAppearance) PURE;
    
    /** This function is called when requesting the articulated parameter count. The function should return S_OK and provide the number of articulated 
    * parameters if the ISimObject is providing articulated parameter support. */
    STDMETHOD (GetArticulatedParameterCount)(__inout UINT& iCount) PURE;

    /** This function is called on a given articulated parameter when the application is requesting an update. The ArticulatedParameter union class should be 
    * filled out in accordance to DIS standards. This function maybe called by the application when an entity state PDU is required due to heartbeat duration 
    * or articulated parameter position or rotation threshold values being exceeded. The function should return S_OK if the articulated parameter was correctly serialized. */
    STDMETHOD (SerializeArticulatedParameter)(__in UINT iIndex, __inout ArticulatedParameter& ArticulatedParam) PURE;
    /** This function is called on remote objects when the given articulated parameter needs to be updated. This function maybe called by the application 
    * when an entity state PDU is received or due to the dead reckoning of the parameter. The function should return S_OK if the articulated parameter was 
    * correctly deserialized. */
    STDMETHOD (DeserializeArticulatedParameter)(__in UINT iIndex, __in const ArticulatedParameter& ArticulatedParam) PURE;
};

DEFINE_GUID(IID_IDISServiceV400,    0xcd623b32, 0x5f8b, 0x409f, 0x84, 0x2a, 0x6, 0xd5, 0x20, 0xb7, 0x53, 0x17);
DEFINE_GUID(SID_DISService,         0xa2202ec4, 0xb81c, 0x4c0b, 0xbd, 0xb7, 0x9f, 0xc6, 0x82, 0x40, 0xc8, 0x43);

/**************************************************************************************************************
**************************************************************************************************************/

/** @} */

} //End namespace P3D
