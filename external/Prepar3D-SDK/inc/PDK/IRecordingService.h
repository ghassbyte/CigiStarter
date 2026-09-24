// Copyright (c) 2020 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IRecordingService.h

#pragma once

#include <ObjBase.h>

/** \defgroup    recordingservice      Recording Service
 This service is provided through the PDK interface and allows callers to manage
 the creation and playback of recordings.
 \{
 */
namespace P3D
{
    enum RecordingState
    {
        None,
        Recording,
        Playing
    };

    /**
     * Service used to enable creation and playback of recordings
     */
    DECLARE_INTERFACE_(IRecordingServiceV510, IUnknown)
    {
        /**
        *   Play back a specified recording
        *   @param    pszFilePath                       The full file path to the recording file
        *   @param    iBookmarkIndex                    The bookmark starting index, 0 if starting from the beginning
        *   @param    dStartTimeInSec                   The start time in seconds, 0 if starting from the beginning
        *   @param    dEndTimeInSec                     The end time in seconds, -1 if starting from the beginning
        *   @param    bDisplayPlaybackCompleteDialog    Set to TRUE to prompt the user after playback completes, FALSE otherwise
        *   @return   Returns S_OK if the playback was successfully started, E_FAIL otherwise
        *   @remarks  Fails if the user is currently recording or is in a multiplayer session.
        *   @remarks  Bookmark index takes precedence over start time. If the user wishes to specify a start time in seconds, bookmark index should be set to 0.
        **/
        virtual HRESULT PlaybackRecording(LPCWSTR pszFilePath, int iBookmarkIndex, double dStartTimeInSec, double dEndTimeInSec, BOOL bDisplayPlaybackCompleteDialog = TRUE) abstract;

        /**
        *   Stops current playback
        *   @return   Returns S_OK if the playback was successfully stopped, E_FAIL otherwise
        **/
        virtual HRESULT StopPlayback() abstract;

        /**
        *   Start recording
        *   @param    pszTitle          The desired title of the recording
        *   @param    pszDescription    The desired description of the recording
        *   @return   Returns S_OK if the recording was successfully started, E_FAIL otherwise
        *   @remarks  Fails if the user is currently recording or playing back a recording.
        *   @remarks  Arguments for title and description may be specified at recording start to ensure proper save in case recording is ended prematurely.
        **/
        virtual HRESULT StartRecording(LPCWSTR pszTitle = L"", LPCWSTR pszDescription = L"") abstract;

        /**
        *   Stop and save the recording
        *   @param    pszTitle          The title of the recording
        *   @param    pszDescription    The description of the recording
        *   @param    bPromptUser       Set to TRUE to prompt the user after recording completes, FALSE otherwise
        *   @return   Returns S_OK if the recording was successfully saved, E_FAIL otherwise
        *   @remarks  Fails if the user is not currently recording or if no title is specified.
        **/
        virtual HRESULT StopAndSaveRecording(LPCWSTR pszTitle, LPCWSTR pszDescription, BOOL bPromptUser) abstract;

        /**
        *   Add bookmark
        *   @param    pszTitle  The title of the bookmark
        *   @return   Returns S_OK if the bookmark was successfully created, E_FAIL otherwise
        *   @remarks  Fails if the user is not currently recording.
        **/
        virtual HRESULT AddBookmark(LPCWSTR pszTitle) abstract;

        /**
        *   Get recording/playback state
        *   @return   The current recording/playback state
        **/
        virtual RecordingState GetRecordingState() const abstract;

        /**
        *   Get the current timestamp in it's packed signed integer form
        *   @remarks    The timestamp is only valid during recording and playback.
        *   @return     The timestamp in it's packed signed integer form, -1 if not playing or recording
        **/
        virtual INT64 GetCurrentTimestamp() const abstract;

        /**
        *   Converts the given timestamp it's packed integer form to seconds
        *   @remarks    The timestamp conversion is only valid during recording and playback.
        *   @return     The timestamp in seconds, -1 if not playing or recording
        **/
        virtual double ConvertTimestampToSeconds(INT64 iTimestamp) const abstract;

        /**
        *   Converts the given timestamp in seconds to the packed signed integer form
        *   @param  dSeconds    The time stamp in seconds
        *   @return             The timestamp in it's packed signed integer form, -1 if not playing or recording
        *   @remarks            The timestamp conversion is only valid during recording and playback.
        **/
        virtual INT64 ConvertSecondsToTimestamp(double dSeconds) const abstract;

        /**
        * Creates an instance of recording data for the recording being played or saved
        * @param    riid            The IID of the interface instance to be created (i.e. IID_IRecordingDataV510)
        * @param    ppRecordingData Pointer to store the newly created instance
        * @return   Returns S_OK if the requested interface was successfully created, E_FAIL otherwise
        * @remarks  The instance is returned with a reference count of 1 and must be released when no longer needed
        */
        virtual HRESULT GetCurrentRecordingData(REFIID riid, void** ppRecordingData) const abstract;

        /**
        * Creates an instance of recording data for the given file path
        * @param    pszFilePath     The full file path of the recording
        * @param    riid            The IID of the interface instance to be created (i.e. IID_IRecordingDataV510)
        * @param    ppRecordingData Pointer to store the newly created recording data instance
        * @return   Returns S_OK if the requested interface was successfully created, E_FAIL otherwise
        * @remarks  The instance is returned with a reference count of 1 and must be released when no longer needed
        */
        virtual HRESULT GetRecordingData(LPCWSTR pszFilePath, REFIID riid, void** ppRecordingData) abstract;
    };

    /**
    * Interface used to access data about a recording
    */
    DECLARE_INTERFACE_(IRecordingDataV510, IUnknown)
    {
        /** Gets the title of the recording **/
        virtual LPCWSTR GetTitle() const abstract;

        /** Gets the description of the recording **/
        virtual LPCWSTR GetDescription() const abstract;

        /** Gets the filename of the recording **/
        virtual LPCWSTR GetFilename() const abstract;

        /** Gets the duration of the recording in seconds **/
        virtual double GetDuration() const abstract;

        /** Gets the starting timestamp of the recording **/
        virtual INT64 GetStartTime() const abstract;

        /** Gets the ending timestamp of the recording **/
        virtual INT64 GetEndTime() const abstract;

        /** Gets the number of bookmarks in the recording **/
        virtual UINT32 GetBookmarkCount() const abstract;

        /**
        * Creates an instance of bookmark data for the given index
        * @param    uIndex          The bookmark index
        * @param    riid            The IID of the interface instance to be created (i.e. IID_IRecordingBookmarkDataV510)
        * @param    ppBookmarkData  Pointer to store the newly created bookmark data instance
        * @return   Returns S_OK if the requested interface was successfully created, E_FAIL otherwise
        * @remarks  The instance is returned with a reference count of 1 and must be released when no longer needed
        */
        virtual HRESULT GetBookmarkData(UINT32 uIndex, REFIID riid, void** ppBookmarkData) const abstract;
    };

    /**
    * Interface used to access data about a recording's bookmark
    */
    DECLARE_INTERFACE_(IRecordingBookmarkDataV510, IUnknown)
    {
        /** Gets the title of the bookmark **/
        virtual LPCWSTR GetTitle() const abstract;

        /** Gets the starting timestamp of the bookmark **/
        virtual INT64 GetTimestamp() const abstract;
    };

    interface __declspec(uuid("{dc7b120c-b47c-4086-bb5d-a9e394609d2c}")) IRecordingDataV510;
    extern __declspec(selectany) REFIID IID_IRecordingDataV510 = __uuidof(IRecordingDataV510);

    interface __declspec(uuid("{b53a24d5-c26c-478a-aa4d-81858e3e22d8}")) IRecordingBookmarkDataV510;
    extern __declspec(selectany) REFIID IID_IRecordingBookmarkDataV510 = __uuidof(IRecordingBookmarkDataV510);

    interface __declspec(uuid("{736b3b04-9f27-483c-83e0-4123153c7526}")) IRecordingServiceV510;
    extern __declspec(selectany) REFIID IID_IRecordingServiceV510 = __uuidof(IRecordingServiceV510);
    extern __declspec(selectany) REFIID SID_RecordingService = IID_IRecordingServiceV510;
}

/** \} */ // end doc group
