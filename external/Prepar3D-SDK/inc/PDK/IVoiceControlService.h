// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IControlService.h
// Prepar3D SDK interface for voice control.
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>
#include "Legacy\IVoiceControlService.Legacy.h"

namespace P3D
{
    interface ICallbackV400;
    struct VoiceDebugInfo;
    /**  \defgroup voicecontrolservice   Voice Control Service
    * This service provides access to Prepar3D's voice control system.
    * It can be used to add custom voice controls to the application.
    * \{
    **/

    /**
    * This service provides access to Prepar3D's voice control system.
    **/
    DECLARE_INTERFACE_(IVoiceControlServiceV440, IVoiceControlServiceV420)
    {
        /**
        * Enables voice recognition.
        **/
        virtual void EnableVoiceRecognition() PURE;

        /**
        * Disables voice recognition.
        **/
        virtual void DisableVoiceRecognition() PURE;

        /**
        * Check if voice recognition is enabled.
        * @return       true if voice recognition is enabled, false otherwise.
        **/
        virtual bool IsVoiceRecognitionEnabled() PURE;

        /**
        * Adds a phrase from the language. ReloadPhrases will need to be called for changes to take effect. 
        * @param        phrase            The voice command to add.
        * @param        event             Optional string of the key event to bind the phrase to.
        * @param        certainty         Minimum certainty required for voice command to trigger (0.0 - 1.0). Defaults to 0.9
        * @return       ID generated unique to this phrase. If no event is provided, this ID will be signalled to Callbacks when triggered.
        **/
        virtual int AddPhrase(LPCWSTR phrase) PURE;
        virtual int AddPhrase(LPCWSTR phrase, double certainty) PURE;
        virtual int AddPhrase(LPCWSTR phrase, const wchar_t* event) PURE;
        virtual int AddPhrase(LPCWSTR phrase, const wchar_t* event, double certainty) PURE;

        /**
        * Removes a specific phrase from the language. ReloadPhrases will need to be called for changes to take effect. 
        * @param        phrase            The voice command to remove.
        **/
        virtual void RemovePhrase(LPCWSTR phrase) PURE;

        /**
        * Clears all phrases loaded into voice controls (including the default phrases).
        **/
        virtual void ClearPhrases() PURE;

        /**
        * Get Voice debug info. This is currently the heard phrase, confidence level
        **/
        virtual wchar_t* GetVoiceDebugInfo() PURE;

        /**
        * Get whether or not to draw debug text.
        **/
        virtual bool GetEnableVoiceDebug() PURE;

        /**
        * Set whether or not to draw debug text.
        **/
        virtual void SetEnableVoiceDebug(bool isEnabled) PURE;

        /**
        * Reloads the phrases with phrases added and removed since last load.
        **/
        virtual void ReloadGrammar() PURE;

        /**
        *   Register a phrase callback.
        *   @param    pCallback    Callback to register
        **/
        virtual void RegisterPhraseCallback(ICallbackV400 * pCallback) PURE;

        /**
        *   Unregister a phrase callback.
        *   @param    pCallback    Callback to unregister
        **/
        virtual void UnregisterPhraseCallback(ICallbackV400 * pCallback) PURE;
    };

    interface __declspec(uuid("{5424CEB3-0EDD-4F21-BE49-06E4747A06D4}")) IVoiceControlServiceV440;
    extern __declspec(selectany) REFIID  IID_IVoiceControlServiceV440 = __uuidof(IVoiceControlServiceV440);
    extern __declspec(selectany) REFIID  SID_VoiceControlService = IID_IVoiceControlServiceV440;

    /** \}*/ // end of doc group
};
