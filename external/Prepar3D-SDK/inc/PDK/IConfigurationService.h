// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
//  IConfigurationServiceV430.h
//  Prepar3D SDK interface for configuration sets.
//----------------------------------------------------------------------------

#pragma once

#include "Legacy\IConfigurationService.Legacy.h"

namespace P3D
{
    /**  \defgroup cfgservice    Configuration Service
    * This service provides access to configuration data defined in SimProp XML files.
    * \{
    **/

    /**
    * This service provides access configuration data stored in SimProp XML files
    **/
    DECLARE_INTERFACE_(IConfigurationServiceV600, IConfigurationServiceV440)
    {
        /**
         * Get configuration set by ID
         * @parm cfgid   GUID id of configuration set
         * @return IConfigurationSetV440*, configuration set
         **/
        virtual IConfigurationSetV440* GetConfigurationSet(const GUID& cfgid) = 0;

        /**
         * Get list of all configuration sets.
         * @param cfgList   Empty list that will be filled with the requested data. 
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetConfigurationList(IConfigurationSetList& cfgList) = 0;
        /**
         * Get list of all configuration sets.
         * @param desc   Description struct that defines the new set to be added.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddConfiguration(SimConfigurationDescriptionV440& desc) = 0;
        /**
         * Get list of all configuration sets.
         * @param cfgid   ID of set to remove.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT RemoveConfiguration(const GUID& cfgid) = 0;
        /**
         * Clears the list of sets to be configured via the UI
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT ClearUISettingsList() = 0;
        /**
         * Add a configuration set to the UI settings window.  This set will be editable via the UI
         * after making a call to OpenUISettingsWindow().
         * @param cfgid   ID of set to add.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddConfigurationToUISettings(const GUID& cfgid) = 0;
        /**
         * DEPRECATED - Use const string version
         * Set the window title for the configuration UI.
         * after making a call to OpenUISettingsWindow().
         * @param windowTitle   Title text to show when in the settings UI window
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT SetUISettingsWindowTitle(const LPWSTR& windowTitle) = 0;
        /**
         * Get list of all configuration sets currently editable via the UI.
         * @param cfgList   Empty list that will be filled with the requested data.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetUIConfigurationList(IConfigurationSetList& cfgList) = 0;
        /**
         * Open UI settings window.
         * @param bModal   Set this to true to make the window modal
         * @param width    Width of UI window in pixels (Set 0 for default)
         * @param height   Height of UI window in pixels (Set 0 for default)
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT OpenUISettingsWindow(bool bModal = true, UINT width = 0, UINT height = 0) = 0;
        /**
         * Close UI settings window.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT CloseUISettingsWindow() = 0;
        /**
         * Check to see if UI window is open.
         * @param bOpen   Will be set to true if window is open, false if not
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT IsUISettingsWindowOpen(bool& bOpen) = 0;
        /**
         * DEPRECATED - Use const string version
         * Add button to settings window. Current max is 4.  Button data is cleared when UI closes.
         * When a button is pressed each configuration set in the UI will invoke its callbacks passing
         * Button1Pressed, Button2Pressed, Button3Pressed, or Button4Pressed as the first callback parameter
         *
         * @param buttonName   Text to show on the button
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddUISettingsWindowButton(const LPWSTR& buttonName) = 0;
        /**
         * Get name of currently selected property in the settings UI
         * @param[out] propertyName   Name of selected property
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetSelectedPropertyName(std::wstring& propertyName) = 0;
        /**
         * Set the window title for the configuration UI.
         * after making a call to OpenUISettingsWindow().
         * @param windowTitle   Title text to show when in the settings UI window
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT SetUISettingsWindowTitle(const LPCWSTR & windowTitle) = 0;
        /**
         * Add button to settings window. Current max is 4.  Button data is cleared when UI closes.
         * When a button is pressed each configuration set in the UI will invoke its callbacks passing
         * Button1Pressed, Button2Pressed, Button3Pressed, or Button4Pressed as the first callback parameter
         *
         * @param buttonName   Text to show on the button
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddUISettingsWindowButton(const LPCWSTR & buttonName) = 0;
    };

    enum ConfigurationCallbackEvent : UINT64
    {
        FileLoaded = 0,
        SimPropSetChanged = 1,
        FileSaved = 2,
        Button1Pressed = 3,
        Button2Pressed = 4,
        Button3Pressed = 5,
        Button4Pressed = 6,
        ExitDialog = 7,
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs

    interface __declspec(uuid("{3987304C-DF06-4A71-A7A2-334C213D0C78}")) IConfigurationServiceV600;
    extern __declspec(selectany) REFIID  IID_IConfigurationServiceV600 = __uuidof(IConfigurationServiceV600);

    extern __declspec(selectany) REFIID  SID_ConfigurationService = IID_IConfigurationServiceV440;

    /** \}*/ // end of doc group
};
