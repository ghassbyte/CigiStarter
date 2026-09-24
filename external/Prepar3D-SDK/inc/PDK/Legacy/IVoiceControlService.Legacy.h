// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IControlService.h
// Prepar3D SDK interface for voice control.
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>
#include <Unknwn.h>

namespace P3D
{
    interface ICallbackV400;
    /**  \defgroup voicecontrolservice   Voice Control Service
    * This service provides access to Prepar3D's voice control system.
    * It can be used to add custom voice controls to the application.
    * \{
    **/

    /**
    * This service provides access to Prepar3D's voice control system.
    **/
    DECLARE_INTERFACE_(IVoiceControlServiceV420, IUnknown)
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

    interface __declspec(uuid("{9E20D480-B0D1-400D-B94B-A210F3D0289A}")) IVoiceControlServiceV420;
    extern __declspec(selectany) REFIID  IID_IVoiceControlServiceV420 = __uuidof(IVoiceControlServiceV420);

    /** \}*/ // end of doc group
};
