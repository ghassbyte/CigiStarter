// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// ISoundService.h
// Description: Interface to Prepar3D sound system.
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>

#include "Types.h"

namespace P3D
{

/// Sound group enumeration
enum SOUND_GROUP
{
    SOUND_GROUP_ENGINE,
    SOUND_GROUP_COCKPIT,
    SOUND_GROUP_ENVIRONMENT,
    SOUND_GROUP_VOICE
};

/// View point enumeration
enum VIEW_POINT
{
    VIEW_POINT_ALL,
    VIEW_POINT_INTERIOR,
    VIEW_POINT_EXTERIOR
};

/** \defgroup soundservice   Sound Service
 *
 * This PDK service allows callers to manipulate sounds at runtime.
 * \{
 */

 /**
  * This interface represents a single sound instance.
  */
DECLARE_INTERFACE_(ISoundInstanceV440, IUnknown)
{
    /**
    * Retruns S_OK if the sound is playing, S_FALSE if it is not, and E_FAIL upon failure.
    */
    STDMETHOD(IsPlaying)() abstract;

    /**
    * Starts to play the sound instance. Returns S_OK upon success, E_FAIL upon failure.
    */
    STDMETHOD(Play)() abstract;

    /**
    * Stops the sound instance. Returns S_OK upon success, E_FAIL upon failure.
    */
    STDMETHOD(Stop)() abstract;

    /**
    * Returns the P3D::SOUND_GROUP of the sound instance.
    */
    STDMETHOD_(P3D::SOUND_GROUP, GetSoundGroup)() const abstract;

    /**
    * Returns the P3D::VIEW_POINT of the sound instance.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(P3D::VIEW_POINT, GetViewPoint)() const abstract;

    /**
    * Sets the P3D::VIEW_POINT of the sound instance.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetViewPoint)(__in P3D::VIEW_POINT eViewPoint) abstract;

    /**
    * Returns true if this is a looping sound instance, false otherwise.
    */
    STDMETHOD_(bool, IsLooping)() const abstract;

    /**
    * Returns true if this is a 3D sound instance, false otherwise.
    */
    STDMETHOD_(bool, Is3D)() const abstract;

    /**
    * Returns true if this sound instances supports pitch control, false otherwise.
    */
    STDMETHOD_(bool, IsPitchControlled)() const abstract;    

    /**
    * Returns the current volume of the sound instance.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD_(double, GetVolume)() const abstract;

    /**
    * Sets the current volume of the sound instance.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD(SetVolume)(__in double dVolume) abstract;

    /**
    * Returns the minimum volume that this sound instance will be played.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD_(double, GetMinVolume)() const abstract;

    /**
    * Sets the maximum volume that this sound instance will be played.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD(SetMinVolume)(__in double dMinVolume) abstract;

    /**
    * Returns the maximum volume that this sound instance will be played.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD_(double, GetMaxVolume)() const abstract;

    /**
    * Sets the maximum volume that this sound instance will be played.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    */
    STDMETHOD(SetMaxVolume)(__in double dMaxVolume) abstract;

    /**
    * Returns the current pitch of the sound instance.
    * @remarks The default value is 1.0 and values must be greater than 0.0. Pitch control must be enabled when creating the sound instance.
    */
    STDMETHOD_(float, GetPitch)() const abstract;

    /**
    * Sets the current pitch of the sound instance.
    * @remarks The default value is 1.0 and values must be greater than 0.0. Pitch control must be enabled when creating the sound instance.
    */
    STDMETHOD(SetPitch)(__in float fPitch) abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzLonAltLat   The position in radians/feet.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(GetPosition)(__out P3D::DXYZ& dxyzLonAltLat) const abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzLonAltLat   The position in radians/feet.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetPosition)(__in const P3D::DXYZ& dxyzLonAltLat) abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzOrientation   The orientation in radians.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(GetOrientation)(__out P3D::DXYZ& dxyzOrientation) const abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzOrientation   The position in radians.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetOrientation)(__in const P3D::DXYZ& dxyzOrientation) abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzOffset   The body relative offset in feet.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(GetOffset)(__out P3D::DXYZ& dxyzOffset) const abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzOffset   The body relative offset in feet.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetOffset)(__in const P3D::DXYZ& dxyzOffset) abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzVelocity   The world velocity in feet per second.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(GetVelocity)(__out P3D::DXYZ& dxyzVelocity) const abstract;

    /**
    * Returns S_OK upon success, E_FAIL otherwise.
    * @param    dxyzVelocity   The world velocity in feet per second.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetVelocity)(__in const P3D::DXYZ& dxyzVelocity) abstract;

    /**
    * Returns the body relative pitch of the cone in degrees.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(float, GetConePitch)() const abstract;

    /**
    * Returns the body relative heading of the cone in degrees.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(float, GetConeHeading)() const abstract;

    /**
    * Sets the body relative pitch and heading of the cone in degrees.
    * @param    fConePitch      The body relative pitch of the cone in degrees.
    * @param    fConeHeading    The body relative heading of the cone in degrees.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetConePitchHeading)(__in float fConePitch, __in float fConeHeading) abstract;

    /**
    * Returns the inside angle of the cone in degrees.
    * @remarks The default value is 360 and values range from 0 to 360 inclusively.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(unsigned short, GetConeInsideAngle)() const abstract;

    /**
    * Returns the outside angle of the cone in degrees.
    * @remarks The default value is 360 and values range from 0 to 360 inclusively.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(unsigned short, GetConeOutsideAngle)() const abstract;

    /**
    * Sets the inside and outside angles of the cone in degrees.
    * @remarks The default value is 360 and values range from 0 to 360 inclusively.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetConeAngles)(__in unsigned short usInsideConeAngle, __in unsigned short usOutsideConeAngle) abstract;

    /**
    * Sets the volume of the sound instance when outside of the cone.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(double, GetOutsideConeVolume)() const abstract;

    /**
    * Returns the volume of the sound instance when outside of the cone.
    * @remarks Units are in 1/100 dBs and values range from 0 to 10000.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetOutsideConeVolume)(__in double dOutsideConeVolume) abstract;

    /**
    * Returns the distance in feet where the sound instance begins to attenuate.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(float, GetFullScaleDistance)() const abstract;

    /**
    * Sets the distance in feet where the sound instance begins to attenuate.
    * @param    fFullScaleDistance  The full scale distance in feet.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetFullScaleDistance)(__in float fFullScaleDistance) abstract;

    /**
    * Returns the cockpit attentuation value. This value is added to the current volume when inside the cockpit.
    * @remarks Negative values should be used to decrease volume when in the cockpit, positive values should be used to increase volume.
    * @remarks Units are in 1/100 dBs and values range from -10000 to 10000.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD_(double, GetCockpitAttenuation)() const abstract;

    /**
    * Sets the cockpit attentuation value. This value is added to the current volume when inside the cockpit.
    * @remarks Negative values should be used to decrease volume when in the cockpit, positive values should be used to increase volume.
    * @remarks Units are in 1/100 dBs and values range from -10000 to 10000.
    * @remarks Only applies to 3D sound instances.
    */
    STDMETHOD(SetCockpitAttenuation)(__in double dCockpitAttenuation) abstract;
};

/// SoundInstance interface ID
DEFINE_GUID(IID_ISoundInstanceV440, 0x47e57528, 0x6748, 0x415d, 0x87, 0xf, 0x55, 0xde, 0xda, 0xac, 0xcf, 0xb8);

 /**
  * This PDK service provides developers with the ability to create and manipulate sounds at runtime.
  */
DECLARE_INTERFACE_(ISoundServiceV440, IUnknown)
{
    /**
    * Creates a reference counted ISoundInstance object. Returns S_OK upon success, E_FAIL otherwise.
    * @param    pszFilename     The filename of the sound file (.wav or .ogg format).
    * @param    eSoundGroup     The P3D::SOUND_GROUP that the sound instance is associated with.
    * @param    eViewPoint      The P3D::VIEW_POINT that the sound instance is associated with.
    * @param    bLooping        True if the sound instance should loop, false otherwise.
    * @param    b3D             True if the sound instance should support 3D functionality such as position and velocity, false otherwise.
    * @param    bPitchControl   True if the sound instance should support pitch control, false otherwise. This is only necessary if using the SetPitch function.
    * @param    ppSoundInstance The newly created ISoundInstance with reference count 1.
    *                           QueryInterface should be called on this object to determine the current version of the returned interface.
    *                           This object should be released when it is no longer needed (either via smart pointer or explicit call to Release()).
    */
    STDMETHOD(CreateSoundInstance)(__in LPCWSTR pszFilename,
                                   __in P3D::SOUND_GROUP eSoundGroup,
                                   __in P3D::VIEW_POINT eViewPoint,
                                   __in bool bLooping,
                                   __in bool b3D,
                                   __in bool bPitchControl,
                                   __out void** ppSoundInstance) abstract;
};

/// SoundService interface ID
DEFINE_GUID(IID_ISoundServiceV440, 0x53693f90, 0xe8fc, 0x43ab, 0xbc, 0xf, 0x7a, 0x4f, 0x86, 0x6f, 0x21, 0x28);
/// SoundService service ID
DEFINE_GUID(SID_SoundService, 0xb134d215, 0x544e, 0x470b, 0xa0, 0xd, 0x8e, 0x65, 0xf9, 0x55, 0x2b, 0x26);

/** \} */

};
