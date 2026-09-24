// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// INetworkServices.h
// Interface to Prepar3D Network Systems
//----------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include <atlcomcli.h>
#include "Legacy\INetworkServices.Legacy.h"

// Outside the P3D namespace.
// Must include NetInOutPublic.h to use.
interface NetInPublic;
interface NetOutPublic;

namespace P3D
{
    /** \defgroup networkservices   Network Services
    *
    * This PDK service allows callers to manipulate various network systems during runtime.
    * \{
    */

    /**
    * This interface can be implemented by a plugin to send and receive custom multiplayer packets.
    * An example on how to use this interface can be found in the CustomPackets PDK sample.
    */
    DECLARE_INTERFACE_(ICustomPacketV530, IUnknown)
    {
    public:

        /**
        * This function will be invoked when sending the custom packet.
        * @param    out         The NetOutPublic object used for serialization.
        * @param    pContext    Optional context pointer passed into the SendCustomPacket call.
        * @return   This function should return S_OK if the packet is to be sent.
        * @remarks  The OnSend and OnReceive functions must serialize and deserialize the packet in the same manner.
        * @remarks  This function must be invoked by a plugin using the multiplayer service SendCustomPacket function.
        */
        STDMETHOD(OnSend)(__in NetOutPublic& out, __in void* pContext) abstract;

        /**
        * This function will be invoked when receiving the custom packet.
        * @param    in          The NetInPublic object used for deserialization.
        * @remarks  The OnSend and OnReceive functions must serialize and deserialize the packet in the same manner.
        * @return   This function should return S_OK if the packet is successfully received.
        */
        STDMETHOD(OnReceive)(__in NetInPublic& in) abstract;
    };

    /**
    * This is the interface to the core Prepar3D multiplayer system. An example on how to query
    * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
    */
    DECLARE_INTERFACE_(IMultiplayerServiceV540, IMultiplayerServiceV530)
    {
    public:

        /**
        * Returns true if the client is currently connected to a multiplayer session.
        * @remarks The client may still be in the lobby at this point.
        */
        STDMETHOD_(bool, InSession)() const abstract;

        /**
        * Returns true if the client is currently connected to a multiplayer session and is the host.
        * @remarks The client may still be in the lobby at this point.
        */
        STDMETHOD_(bool, IsHosting)() const abstract;

        /**
        * Starts hosting a multiplayer session based on the current simulation state.
        * @return               Returns S_OK if the client successfully begins hosting a multiplayer session.
        */
        STDMETHOD(StartHosting)() const abstract;

        /**
        * Gets a value indicating whether or not anti-cheat is enabled.
        * @return               Returns a value indicating whether or not anti-cheat is enabled.
        */
        STDMETHOD_(bool, IsAntiCheatEnabled)() const abstract;

        /**
        * Gets a value indicating whether or not the local sims can slew.
        * @return               Returns a value indicating whether or not the local sims can slew.
        */
        STDMETHOD_(bool, IsSlewModeEnabled)() const abstract;

        /**
        * Gets a value indicating whether or not the local sims can pause the simulation.
        * @return               Returns a value indicating whether or not the local sims can pause the simulation.
        */
        STDMETHOD_(bool, IsPauseLocalSimulationEnabled)() const abstract;

        /**
        * Gets a value indicating the preferred audio panel
        * @return               Returns a value indicating the audio panel to use for transmit/receive
        */
        STDMETHOD_(UINT, GetPreferredAudioPanel)() const abstract;

        /**
        * Gets the maximum number of players in for session.
        * @return               Returns the maximum number of players for the current session.
        */
        STDMETHOD_(UINT, GetMaxPlayerCount)() const abstract;


        /**
        * Gets the time since the session started.
        * @return               Returns the time since the session started.
        */
        STDMETHOD_(double, GetTimeSinceStart)() const abstract;

        /**
        * Gets the number of players in the session.
        * @return               Returns the number of players in the current session.
        */
        STDMETHOD_(UINT, GetPlayerCount)() const abstract;

        /**
        * Gets the object ID of the player at the given index.
        * @param uIndex         The index of the player.
        * @return               Returns the player's object ID.
        * @remarks              A return value of 0 is an invalid object ID.
        *                       Indexes are 0 based and the max index will be GetPlayerCount() - 1.
        *                       Multiplayer role indexes may changed as users join and leave the session.
        */
        STDMETHOD_(UINT, GetPlayerObjectID)(__in UINT uIndex) const abstract;

        /**
        * Gets the name of the player at the given index.
        * @param uIndex         The index of the player.
        * @param pszName        The buffer to copy the player name into.
        * @param uLength        The length of the buffer to copy the player name into.
        * @return               Returns S_OK if the name was successfully retrieved and copied, E_FAIL otherwise.
        * @remarks              Indexes are 0 based and the max index will be GetPlayerCount() - 1.
        *                       Multiplayer role indexes may changed as users join and leave the session.
        */
        STDMETHOD(GetPlayerName)(__in UINT uIndex, __out LPWSTR pszName, __in UINT uLength) const abstract;

        /**
        * Gets the player role GUID of the player at the given index.
        * @param uIndex         The index of the player.
        * @param guidRole       The player role GUID.
        * @return               Returns S_OK if the multiplayer role GUID for the given player index was successfully found, E_FAIL otherwise.
        * @remarks              Indexes are 0 based and the max index will be GetPlayerCount() - 1.
        *                       Multiplayer role indexes may changed as users join and leave the session.
        *                       This function only succeeds when in a multiplayer structured scenario.
        */
        STDMETHOD(GetPlayerRoleGUID)(__in UINT uIndex, __out GUID& guidRole) const abstract;

        /**
        * Gets the IP of the player at the given index.
        * @param uIndex         The index of the player.
        * @param pszIPAddr      The buffer to copy the player IP into.
        * @param uLength        The length of the buffer to copy the player IP into.
        * @return               Returns S_OK if the IP was successfully retrieved and copied, E_FAIL otherwise.
        * @remarks              Indexes are 0 based and the max index will be GetPlayerCount() - 1.
        *                       Multiplayer role indexes may changed as users join and leave the session.
        */
        STDMETHOD(GetPlayerIP)(__in UINT uIndex, __out LPWSTR pszIPAddr, __in UINT uLength) const abstract;


        /**
        * Gets the aircraft title for the player.
        * @param pszTitle       The buffer to copy the player aircraft title into.
        * @param uLength        The length of the buffer to copy the player aircraft title into.
        * @return               Returns S_OK if the title was successfully retrieved and copied, E_FAIL otherwise.
        */
        STDMETHOD(GetAircraftTitle)(__in UINT uIndex, __out LPWSTR pszTitle, __in UINT uLength) const abstract;


        /**
        * Sets the voice frequencies for this client. These frequencies are used when transmitting voice in a multiplayer session.
        * This function can be useful when modeling radio systems that do not align with the native radio system's voice frequencies.
        * @param iTransmitReceiveHz The primary frequency used for transmit and receive.
        * @param iOptionalReceiveHz The secondary frequency used for receive only (optional).
        * @return                   Returns S_OK if the frequencies were successfully set, E_FAIL otherwise.
        * @remarks                  Radio frequencies should be entered in Hz (i.e. 128300000).
        *                           A value of zero should be used if transmission or receive should be disabled.
        *                           A value of -1 for both parameters will restore default frequency behavior.
        *                           This is a global function call that is associated with the current client and not an individual SimObject. Therefore, this function should only be called by the user object or a global plugin.
        */
        STDMETHOD(SetVoiceFrequencies)(__in INT32 iTransmitReceiveHz, __in INT32 iOptionalReceiveHz) abstract;

        /**
        * Gets the current voice frequencies for this client. These frequencies are used when transmitting voice in a multiplayer session.
        * This function may return the transmit and receive frequencies from the native radio system, or custom frequencies set by SetVoiceFrequencies.
        * @param iTransmitReceiveHz The primary frequency used for transmit and receive.
        * @param iOptionalReceiveHz The secondary frequency used for receive only (optional).
        * @return                   Returns S_OK if the frequencies were successfully returned, E_FAIL otherwise.
        * @remarks                  Radio frequencies are returned in Hz (i.e. 128300000).
        *                           A value of zero is returned if transmission or receive is disabled.
        *                           If the multiplayer host is configured to disregard radio frequency for communications, a value of 1 and 0 will be returned.
        */
        STDMETHOD(GetVoiceFrequencies)(__in INT32& iTransmitReceiveHz, __in INT32& iOptionalReceiveHz) const abstract;

        /**
        * Returns this client's Player ID. Player ID's are unique to each player in the multplayer session.
        * @remarks  Player ID should not be confused with object ID, which refers the player's SimObject ID.
        * @return   Returns this client's Player ID if successful, 0 otherwise.
        */
        STDMETHOD_(UINT, GetCurrentPlayerID)() const abstract;

        /**
        * Registers a plugin's custom packet implementation with the system.
        * @param    guidPacketID    The unique ID of the custom packet being registered
        * @param    pCustomPacket   The plugin's implementation of the custom packet
        * @param    bSend           True if the packet will be sent by the current client
        * @param    bReceive        True if this packet should be received by the current client
        * @return                   Returns S_OK if successfully registered, E_FAIL otherwise.
        * @remarks  The custom packet ID must be unique and should not be reused between RegisterCustomPacket calls.
        */
        STDMETHOD(RegisterCustomPacket)(__in const GUID& guidPacketID, __in ICustomPacketV530* pCustomPacket, __in bool bSend, __in bool bReceive) abstract;

        /**
        * Unregisters a plugin's custom packet implementation from the system.
        * @param    guidPacketID    The unique ID of the custom packet being unregistered
        * @return                   Returns S_OK if successfully unregistered, E_FAIL otherwise.
        */
        STDMETHOD(UnregisterCustomPacket)(__in const GUID& guidPacketID) abstract;

        /**
        * Invokes the custom packet's OnSend function and broadcasts the packet data to other clients.
        * @param    guidPacketID    The unique ID of the custom packet to be sent
        * @param    pContext        Optional context pointer to be passed into the custom packets OnSend call.
        * @return                   Returns S_OK if the packet was successfully broadcast
        * @remarks  Packets are only broadcast to clients that have registered to receive the custom packet.
        * @remarks  The host does not need to register custom packets inorder for other clients to receive the packet.
        */
        STDMETHOD(SendCustomPacket)(__in const GUID& guidPacketID, __in void* pContext = nullptr) abstract;

        /**
        * Sets whether or not the packet should be sent as "guaranteed". The default value is true.
        * @param    guidPacketID    The unique ID of the custom packet to be sent
        * @param    bGuaranteed     True if the packet should be sent guaranteed, false if not-guaranteed.
        * @return                   Returns S_OK upon success, E_FAIL otherwise
        * @remarks  Guaranteed packets will be confirmed by receiving clients, or rebroadcast if a confirmation is not received after a given period of time.
        *           This is useful for low-frequency packets that must be processed.
        *           Packets that are sent frequently and can afford to be dropped should be sent as not-guaranteed for performance reasons.
        */
        STDMETHOD(SetCustomPacketGuaranteed)(__in const GUID& guidPacketID, __in bool bGuaranteed) abstract;
    };


    /**
    * This is the interface to the core Prepar3D multiplayer system. An example on how to query
    * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
    */
    DECLARE_INTERFACE_(IMultichannelServiceV440, IUnknown)
    {
    public:

        /**
        * Returns true if the client is currently connected to a multichannel session.
        * @remarks The client may still be in the lobby at this point.
        */
        STDMETHOD_(bool, InSession)() const abstract;

        /**
        * Returns true if the client is the multichannel master
        */
        STDMETHOD_(bool, IsMaster)() const abstract;

        /**
        * Returns true if the client is a multichannel slave
        */
        STDMETHOD_(bool, IsSlave)() const abstract;
    };

    
    /**
    * The byte position in a component data word where BYTE_POS_0 is the least significant byte
    * and BYTE_POS_3 is the most significat byte.
    */
    enum BYTE_POS
    {
        /// Most Significant Byte
        BYTE_POS_3 = 3,
        BYTE_POS_2 = 2,
        BYTE_POS_1 = 1,
        /// Least Significant Byte
        BYTE_POS_0 = 0,
    };

    /**
    * The short position in a component data word where SHORT_POS_1 is the most significant short
    * and SHORT_POS_0 is the least significant short.
    */
    enum SHORT_POS
    {
        /// Most Significant Short
        SHORT_POS_1 = 1,
        /// Least Significant Short
        SHORT_POS_0 = 0,
    };

    /**
    * This interface is used by a plugin to populate Component Control data when sending and receiving packets.
    * This interface allows for setting/getting of bytes, shorts, integers, floats, doubles, and 64-bit integers.
    * @remarks  When setting a byte or short, the position of the byte and short must also be given.
    * @remarks  When setting a 64-bit value, valid word indexes are [0,2].
    * See also the CigiComponentControl PDK sample.
    **/
    DECLARE_INTERFACE_(IComponentControlV430, IUnknown)
    {
        /// Signals Prepar3D to queue the current Component Control packet data.
        /// This function should be called once per Component Control packet,
        /// but may also be called more than once per IComponentControlCallbackV430::OnSend()
        /// callback to send multiple packets.
        STDMETHOD_(void, SendData)() abstract;
        /// Resets the IComponentControl data to the default values.
        /// Useful for resetting the structure between SendData() calls, if necessary.
        STDMETHOD_(void, ClearData)() abstract;
        /// Sets the Short Component Control flag. If true, Prepar3D will queue a
        /// Short Component Control packet with a call to SendData().
        STDMETHOD_(void, SetShortComponent)(bool bIsShort) abstract;
        /// Gets the short component flag.
        STDMETHOD_(bool, GetShortComponent)() const abstract;
        /// Sets the Component ID.
        STDMETHOD_(void, SetComponentID)(UINT16 uID) abstract;
        /// Gets the Component ID.
        STDMETHOD_(UINT16, GetComponentID)() const abstract;
        /// Sets the Component Class.
        STDMETHOD_(void, SetComponentClass)(UINT8 uClass) abstract;
        /// Gets the Component Class.
        STDMETHOD_(UINT8, GetComponentClass)() const abstract;
        /// Sets the Component State.
        STDMETHOD_(void, SetComponentState)(UINT8 uState) abstract;
        /// Gets the Component State.
        STDMETHOD_(UINT8, GetComponentState)() const abstract;
        /// Sets the Instance ID.
        STDMETHOD_(void, SetInstanceID)(UINT16 uInstanceID) abstract;
        /// Gets the Instance ID.
        STDMETHOD_(UINT16, GetInstanceID)() const abstract;
        /// Sets an unsigned byte for the given word in the given byte position.
        STDMETHOD_(void, SetUCharData)(UINT8 uData, UINT uWord, P3D::BYTE_POS ePos) abstract;
        /// Gets an unsigned byte from the given word in the given byte position.
        STDMETHOD_(UINT8, GetUCharData)(UINT uWord, P3D::BYTE_POS ePos) const abstract;
        /// Sets a byte for the given word in the given byte position.
        STDMETHOD_(void, SetCharData)(INT8 iData, UINT uWord, P3D::BYTE_POS ePos) abstract;
        /// Gets a byte from the given word in the given byte position.
        STDMETHOD_(INT8, GetCharData)(UINT uWord, P3D::BYTE_POS ePos) const abstract;
        /// Sets a 16-bit unsigned short for the given word in the given short position.
        STDMETHOD_(void, SetUShortData)(UINT16 uData, UINT uWord, P3D::SHORT_POS ePos) abstract;
        /// Gets a 16-bit unsigned short from the given word in the given short position.
        STDMETHOD_(UINT16, GetUShortData)(UINT uWord, P3D::SHORT_POS ePos) const abstract;
        /// Sets a 16-bit short for the given word in the given short position.
        STDMETHOD_(void, SetShortData)(INT16 iData, UINT uWord, P3D::SHORT_POS ePos) abstract;
        /// Gets a 16-bit short from the given word in the given short position.
        STDMETHOD_(INT16, GetShortData)(UINT uWord, P3D::SHORT_POS ePos) const abstract;
        /// Sets a 32-bit unsigned integer for the given word.
        STDMETHOD_(void, SetUIntData)(UINT32 uData, UINT uWord) abstract;
        /// Gets a 32-bit unsigned integer from the given word.
        STDMETHOD_(UINT32, GetUIntData)(UINT uWord) const abstract;
        /// Sets a 32-bit integer for the given word.
        STDMETHOD_(void, SetIntData)(INT32 iData, UINT uWord) abstract;
        /// Gets a 32-bit integer from the given word.
        STDMETHOD_(INT32, GetIntData)(UINT uWord) const abstract;
        /// Sets a 32-bit floating point value for the given word.
        STDMETHOD_(void, SetFloatData)(float fData, UINT uWord) abstract;
        /// Gets a 32-bit floating point value from the given word.
        STDMETHOD_(float, GetFloatData)(UINT uWord) const abstract;
        /// Sets a 64-bit unsigned integer value for the given word.
        /// Valid word values for 64-bit data fields are [0,2].
        STDMETHOD_(void, SetUInt64Data)(UINT64 uData, UINT uWord) abstract;
        /// Gets a 64-bit unsigned integer value from the given word.
        /// Valid word values for 64-bit data fields are [0,2].
        STDMETHOD_(UINT64, GetUInt64Data)(UINT uWord) const abstract;
        /// Sets a 64-bit double floating point value for the given word.
        /// Valid word values for 64-bit data fields are [0,2].
        STDMETHOD_(void, SetDoubleData)(double dData, UINT uWord) abstract;
        /// Gets a 64-bit double floating point value from the given word.
        /// Valid word values for 64-bit data fields are [0,2].
        STDMETHOD_(double, GetDoubleData)(UINT uWord) const abstract;
    };


    /**
    * This interface is implemeted by a plugin to receive Component Control callbacks.
    * Callbacks can be registered/unregistered using the CIGI PDK Service
    * ICigiServiceV430::RegisterComponentControlCallback() and
    * ICigiServiceV430::UnregisterComponentControlCallback() functions.
    **/
    DECLARE_INTERFACE_(IComponentControlCallbackV430, IUnknown)
    {
        /**
        * This function is called once per frame while Prepar3D is an active CIGI host.
        * Plugins should fill the IComponentControl interface with the necessary Compontent Control packet data.
        * The plugin should then call IComponentControlV430::SendData() to signal to Prepar3D that the current
        * data should be queued as an outgoing message. Plugins are able to call SendData() multiple times during
        * this callback to send more than one Component Control packet per frame. The IComponentControlV430::ClearData()
        * function may be called to reset the packet data between calls to SendData(). The ClearData() function is
        * there mainly to ease development and is not required to be called.
        **/
        virtual HRESULT OnSend(IComponentControlV430& CompCtrl) abstract;

        /**
        * This function is called once per frame while Prepar3D is an active CIGI IG.
        * Plugins should implement this function and determine if they are to process the received packet.
        **/
        virtual HRESULT OnReceive(const IComponentControlV430& CompCtrl) abstract;
    };


    // Interface to a CIGI packet that allows plugins to read, write, and modify packet data.
    DECLARE_INTERFACE_(ICigiPacketV440, IUnknown)
    {
        STDMETHOD_(USHORT, GetPacketID)()                           const   abstract;

        STDMETHOD_(USHORT, GetPacketSize)()                         const   abstract;

        STDMETHOD_(BYTE, GetMajorVersion)()                         const   abstract;

        STDMETHOD_(BYTE, GetMinorVersion)()                         const   abstract;

        STDMETHOD_(void, SetPacketID)(USHORT PacketID)                      abstract;

        STDMETHOD(GetData)(void* pData, UINT cbSize)                const   abstract;

        STDMETHOD(SetData)(void* pData, UINT cbSize)                        abstract;

        STDMETHOD(SendData)()                                               abstract;
    };


    DECLARE_INTERFACE_(ICigiPacketCallbackV440, IUnknown)
    {
        virtual HRESULT OnCustomSend(const USHORT& uPacketID, ICigiPacketV440& pPacket) abstract;

        virtual HRESULT OnP3DReceive(const USHORT& uPacketID, ICigiPacketV440& pPacket) abstract;

        virtual HRESULT OnP3DSend(const USHORT& uPacketID, ICigiPacketV440& pPacket) abstract;
    };


    /**
    * This is the interface to the core Prepar3D CIGI plugin. An example on how to query
    * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
    */
    DECLARE_INTERFACE_(ICigiServiceV430, IUnknown)
    {
    public:

        /**
        * Returns true if the application is currently connected to a CIGI session.
        */
        STDMETHOD_(bool, InSession)() const abstract;

        /**
        * Returns true if the application is currently connected to a CIGI session and is the host.
        */
        STDMETHOD_(bool, IsHosting)() const abstract;

        /**
        * Returns true if the application is currently connected to a CIGI session and is the IG.
        */
        STDMETHOD_(bool, IsIG)() const abstract;

        /**
        * Returns the CIGI major version for the current session.
        */
        STDMETHOD_(UINT, GetMajorVersion)() const abstract;

        /**
        * Returns the CIGI minor version for the current session.
        */
        STDMETHOD_(UINT, GetMinorVersion)() const abstract;

        /**
        * Gets the CIGI Entity ID for the given Prepar3D Object ID.
        * @param    uObjectID   The Prepar3D Object ID.
        * @param    usEntityID  The CIGI Entity ID for the given Prepar3D Object ID.
        * @return               Returns S_OK if the CIGI Entity ID for the given Prepar3D Object ID was found, E_FAIL otherwise.
        */
        STDMETHOD(GetEntityID)(__in UINT uObjectID, __out USHORT& usEntityID) const abstract;

        /**
        * Gets the Prepar3D Object ID for the given CIGI Entity ID.
        * @param    usEntityID  The CIGI Entity ID.
        * @param    uObjectID   The Prepar3D Object ID for the given CIGI Entity ID.
        * @return               Returns S_OK if the Prepar3D Object ID for the given CIGI Entity ID was found, E_FAIL otherwise.
        */
        STDMETHOD(GetObjectID)(__in USHORT usEntityID, __out UINT& uObjectID) const abstract;

        /**
        * Registers the given IComponentControlCallback.
        * @param    pCallback   The callback to be registered.
        * @return               Returns S_OK if the callback was successfully registered, E_FAIL otherwise.
        * @remarks              Plugins will only receive callbacks when in an active CIGI session.
        */
        STDMETHOD(RegisterComponentControlCallback)(__in __notnull P3D::IComponentControlCallbackV430* pCallback) abstract;

        /**
        * Unregisters the given IComponentControlCallback.
        * @param    pCallback   The callback to be unregistered.
        * @return               Returns S_OK if the callback was successfully unregistered, E_FAIL otherwise.
        */
        STDMETHOD(UnregisterComponentControlCallback)(__in __notnull P3D::IComponentControlCallbackV430* pCallback) abstract;

        /**
        * Registers the given ICigiPacketCallback.
        * @param    pCallback   The callback to be registered.
        * @return               Returns S_OK if the callback was successfully registered, E_FAIL otherwise.
        * @remarks              Plugins will only receive callbacks when in an active CIGI session.
        */
        STDMETHOD(RegisterCigiPacketCallback)(__in const USHORT& uPacketID, __in __notnull P3D::ICigiPacketCallbackV440* pCallback) abstract;

        /**
        * Unregisters the given ICigiPacketCallback.
        * @param    pCallback   The callback to be unregistered.
        * @return               Returns S_OK if the callback was successfully unregistered, E_FAIL otherwise.
        */
        STDMETHOD(UnregisterCigiPacketCallback)(__in const USHORT& uPacketID, __in __notnull P3D::ICigiPacketCallbackV440* pCallback) abstract;

        STDMETHOD(AddExcludedSendID)(__in const USHORT& uCustomPacketID) abstract;
        STDMETHOD(RemoveExcludedSendID)(__in const USHORT& uCustomPacketID) abstract;

        STDMETHOD(AddExcludedReceiveID)(__in const USHORT& uCustomPacketID) abstract;
        STDMETHOD(RemoveExcludedReceiveID)(__in const USHORT& uCustomPacketID) abstract;
    };


    /** \} */

    //---------------------------------------------------------------------------
    // Predefined UUIDs

    interface __declspec(uuid("{58533E4E-7CEB-49F1-A912-E82E6BFF8D7F}")) IMultiplayerServiceV540;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV540 = __uuidof(IMultiplayerServiceV540);

    interface __declspec(uuid("{964a5908-3991-4dd8-a172-b3da78eebc5d}")) IMultichannelServiceV440;
    extern __declspec(selectany) REFIID IID_IMultichannelServiceV440 = __uuidof(IMultichannelServiceV440);
    
    interface __declspec(uuid("{ddb1f3e4-2c8e-44b5-9992-34cc644927e4}")) IComponentControlV430;
    extern __declspec(selectany) REFIID IID_IComponentControlV430 = __uuidof(IComponentControlV430);
    
    interface __declspec(uuid("{b77c047e-793c-4210-8172-13229a14333d}")) IComponentControlCallbackV430;
    extern __declspec(selectany) REFIID IID_IComponentControlCallbackV430 = __uuidof(IComponentControlCallbackV430);

    interface __declspec(uuid("{B93C6787-FC5E-4D74-92DC-6BB149E22DC8}")) ICigiPacketV440;
    extern __declspec(selectany) REFIID IID_ICigiPacketV440 = __uuidof(ICigiPacketV440);

    interface __declspec(uuid("{81808E7A-EC39-45FF-8111-4855C2491556}")) ICigiPacketCallbackV440;
    extern __declspec(selectany) REFIID IID_ICigiPacketCallbackV440 = __uuidof(ICigiPacketCallbackV440);

    interface __declspec(uuid("{1c500b49-35ea-49eb-aa90-fa904fd254b9}")) ICigiServiceV430;
    extern __declspec(selectany) REFIID IID_ICigiServiceV430 = __uuidof(ICigiServiceV430);

    interface __declspec(uuid("{191fedfe-9b48-4f79-bd72-42c204996588}")) ICustomPacketV530;
    extern __declspec(selectany) REFIID IID_ICustomPacketV530 = __uuidof(ICustomPacketV530);

    extern __declspec(selectany) REFGUID  SID_MultiplayerService = { 0x7db88f0f, 0x13f8, 0x4ee0, 0xa1, 0xf8, 0x23, 0x9a, 0x29, 0x23, 0x6b, 0xaf };
    extern __declspec(selectany) REFGUID  SID_MultichannelService = { 0x31195859, 0x206a, 0x43b9, 0xa9, 0x7c, 0xe, 0x42, 0xe0, 0xde, 0x95, 0x2 };
    extern __declspec(selectany) REFGUID  SID_CigiService = { 0x837698ae, 0x6cb7, 0x4ead, 0x9d, 0x34, 0x0, 0x17, 0x5, 0xdc, 0x8e, 0xf4 };

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID  IID_IMultiplayerService = IID_IMultiplayerServiceV530;
    extern __declspec(selectany) REFIID  IID_IMultichannelService = IID_IMultichannelServiceV440;
    extern __declspec(selectany) REFIID  IID_ICigiService = IID_ICigiServiceV430;

    typedef IMultiplayerServiceV530 IMultiplayerService;
    typedef IMultichannelServiceV440 IMultichannelService;
    typedef ICigiServiceV430 ICigiService;
};
