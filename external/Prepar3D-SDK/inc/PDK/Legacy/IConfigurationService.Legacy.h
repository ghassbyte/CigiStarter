// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IEventService.Legacy.h

#pragma once

#include "Types.h"
#include "Helpers\ListBuilder.h"
#include <ObjBase.h>
#include <atlcomcli.h>
#include <string>
#include "IEventService.h"

__interface ISimPropertySet;

namespace P3D
{
    struct SimConfigurationDescriptionV440
    {
        /** GUID ID or the configuration set **/
        GUID ID;
        /** Name of the set.  This will show up in the configuration UI **/
        LPCWSTR Name;
        /** Path to XML file that contains the SimProp configuration data **/
        LPCWSTR FilePath;
        /** Path to the propdef XML file that defines the SimPropertySet schema **/
        LPCWSTR SymbolsPath;
        /** Callback to handle configuration changes or related UI events **/
        ICallbackV400* Callback;
        /** Should this data by synchronized over the network.  This allows setting changes on a
         *  multichannle host to be applied to all connected clients
         **/
        bool  IsNetworkSynchronized;
    };

    /**
    * A set of configuration data defined in a SimProp XML file
    **/
    DECLARE_INTERFACE_(IConfigurationSetV440, IUnknown)
    {

        /**
        * Get the ISimPropertySet that holds the configuration data
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual ISimPropertySet* GetPropSet() abstract;
        /**
         * Get the GUID (Globally Unique Identifier) for this configuration set
         * @return GUID, GUID of the configuration set
         **/
        virtual const GUID& GetGUID() const abstract;
        /**
         * Get the name of this configuration set
         * @return LPCWSTR, same of the configuration set
         **/
        virtual LPCWSTR GetName() const abstract;
        /**
         * Get the ISimPropertySet that holds the configuration data
         * @return LPCWSTR, the file path to the SimProp xml file containing the configuration data
         **/
        virtual LPCWSTR GetFilePath() const;
        /**
         * Get the ISimPropertySet that holds the configuration data.  This should trigger
         * any callbacks listing for changes.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT SetPropSet(ISimPropertySet * pSet) abstract;

        /**
         * Load configuration data from file.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT LoadFromFile() abstract;

        /**
         * Save configuration data to file.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT SaveToFile() abstract;

        /**
         * Register callback.  Callbacks can be triggered when configuration data is
         * changed, or when buttons are pressed in the configuration UI.
         * @param pCallback   Callback to register
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT RegisterCallback(ICallbackV400 * pCallback) abstract;

        /**
         * Unregister callback.
         * @param pCallback   Callback to unregister
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT UnregisterCallback(ICallbackV400 * pCallback) abstract;
    };

    /** Configuration List Builder Interface **/
    typedef P3D::IListBuilder<IConfigurationSetV440> IConfigurationSetList;
    /** Configuration list builder implementation using a standard vector of CComPtrs **/
    typedef P3D::CComPtrVecBuilder<IConfigurationSetV440> ConfigurationSetList;

    /**
    * This service provides access configuration data stored in SimProp XML files
    **/
    DECLARE_INTERFACE_(IConfigurationServiceV440, IUnknown)
    {
        /**
         * Get configuration set by ID
         * @parm cfgid   GUID id of configuration set
         * @return IConfigurationSetV440*, configuration set
         **/
        virtual IConfigurationSetV440* GetConfigurationSet(const GUID & cfgid) = 0;

        /**
         * Get list of all configuration sets.
         * @param cfgList   Empty list that will be filled with the requested data.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetConfigurationList(IConfigurationSetList & cfgList) = 0;
        /**
         * Get list of all configuration sets.
         * @param desc   Description struct that defines the new set to be added.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddConfiguration(SimConfigurationDescriptionV440 & desc) = 0;
        /**
         * Get list of all configuration sets.
         * @param cfgid   ID of set to remove.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT RemoveConfiguration(const GUID & cfgid) = 0;
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
        virtual HRESULT AddConfigurationToUISettings(const GUID & cfgid) = 0;
        /**
         * Set the window title for the configuration UI.
         * after making a call to OpenUISettingsWindow().
         * @param windowTitle   Title text to show when in the settings UI window
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT SetUISettingsWindowTitle(const LPWSTR & windowTitle) = 0;
        /**
         * Get list of all configuration sets currently editable via the UI.
         * @param cfgList   Empty list that will be filled with the requested data.
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetUIConfigurationList(IConfigurationSetList & cfgList) = 0;
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
         * Add button to settings window. Current max is 4.  Button data is cleared when UI closes.
         * When a button is pressed each configuration set in the UI will invoke its callbacks passing
         * Button1Pressed, Button2Pressed, Button3Pressed, or Button4Pressed as the first callback parameter
         *
         * @param buttonName   Text to show on the button
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT AddUISettingsWindowButton(const LPWSTR & buttonName) = 0;
        /**
         * Get name of currently selected property in the settings UI
         * @param[out] propertyName   Name of selected property
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT GetSelectedPropertyName(std::wstring & propertyName) = 0;
    };

    interface __declspec(uuid("{9C729660-2E7D-4DD2-8FC6-33FD3EA9A264}")) IConfigurationServiceV440;
    extern __declspec(selectany) REFIID  IID_IConfigurationServiceV440 = __uuidof(IConfigurationServiceV440);

    interface __declspec(uuid("{697684C0-DA3E-4EF7-9392-7CF305FAC32E}")) IConfigurationSetV440;
    extern __declspec(selectany) REFIID  IID_IConfigurationSetV440 = __uuidof(IConfigurationSetV440);

/** \} */ // end doc group
}