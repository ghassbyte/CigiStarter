// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// INetworkServices.h
// Interface to Prepar3D Network Systems
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>

namespace P3D
{
    interface ICustomPacketV530;

    /** \defgroup networkservices   Network Services
     *
     * This PDK service allows callers to manipulate various network systems during runtime.
     * \{
     */

     /**
      * This is the interface to the core Prepar3D multiplayer system. An example on how to query
      * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
      */
    DECLARE_INTERFACE_(IMultiplayerServiceV430, IUnknown)
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
        *                       
        */
        STDMETHOD(GetPlayerRoleGUID)(__in UINT uIndex, __out GUID& guidRole) const abstract;
    };

    /**
 * This is the interface to the core Prepar3D multiplayer system. An example on how to query
 * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
 */
    DECLARE_INTERFACE_(IMultiplayerServiceV453, IMultiplayerServiceV430)
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
        STDMETHOD(GetPlayerRoleGUID)(__in UINT uIndex, __out GUID & guidRole) const abstract;


        /**
        * Gets the aircraft title for the player.
        * @param pszTitle       The buffer to copy the player aircraft title into.
        * @param uLength        The length of the buffer to copy the player aircraft title into.
        * @return               Returns S_OK if the title was successfully retrieved and copied, E_FAIL otherwise.
        */
        STDMETHOD(GetAircraftTitle)(__in UINT uIndex, __out LPWSTR pszTitle, __in UINT uLength) const abstract;
    };

    /**
     * This is the interface to the core Prepar3D multiplayer system. An example on how to query
     * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
     */
    DECLARE_INTERFACE_(IMultiplayerServiceV510, IMultiplayerServiceV453)
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
        STDMETHOD(GetPlayerRoleGUID)(__in UINT uIndex, __out GUID & guidRole) const abstract;

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
    };

    /**
      * This is the interface to the core Prepar3D multiplayer system. An example on how to query
      * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
      */
    DECLARE_INTERFACE_(IMultiplayerServiceV520, IMultiplayerServiceV510)
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
        STDMETHOD(GetPlayerRoleGUID)(__in UINT uIndex, __out GUID & guidRole) const abstract;

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
        STDMETHOD(GetVoiceFrequencies)(__in INT32 & iTransmitReceiveHz, __in INT32 & iOptionalReceiveHz) const abstract;
    };

    /**
    * This is the interface to the core Prepar3D multiplayer system. An example on how to query
    * for a PDK service can be found in the DLLStart() function of the Camera PDK Sample.
    */
    DECLARE_INTERFACE_(IMultiplayerServiceV530, IMultiplayerServiceV520)
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
    };

    interface __declspec(uuid("{da7f0157-45b8-423a-b1fc-8c01d748b163}")) IMultiplayerServiceV430;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV430 = __uuidof(IMultiplayerServiceV430);

    interface __declspec(uuid("{BC0ABF23-ACF2-4AE6-9FDC-BC694AFF61BA}")) IMultiplayerServiceV453;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV453 = __uuidof(IMultiplayerServiceV453);

    interface __declspec(uuid("{e57a9ef3-7cee-43ab-9cb3-4e05d9ec5b36}")) IMultiplayerServiceV510;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV510 = __uuidof(IMultiplayerServiceV510);

    interface __declspec(uuid("{680aafdd-e40d-485a-9818-461ec99be42f}")) IMultiplayerServiceV520;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV520 = __uuidof(IMultiplayerServiceV520);

    interface __declspec(uuid("{12ba30d9-2b3d-45fa-b510-3b69b1dfcaf0}")) IMultiplayerServiceV530;
    extern __declspec(selectany) REFIID IID_IMultiplayerServiceV530 = __uuidof(IMultiplayerServiceV530);
};
