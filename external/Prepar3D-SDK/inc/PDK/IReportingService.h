// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IReportingService.h

#pragma once

#include <ObjBase.h>
/** \defgroup    reportingservice      Reporting Service
 This service is provided through the PDK interface and allows callers to report 
 information to the end user via content error logging and alerts.
 \{
 */
namespace P3D
{
    enum AlertWindowButton : int
    {
        Button_Clear = 0x00000000,
        Button_Close = 0x00000001,
        Button_Ok = 0x00000002,
        Button_Cancel = 0x00000004,
        Button_Abort = 0x00000008,
        Button_Retry = 0x00000010,
        Button_Ignore = 0x00000020,
        Button_Yes = 0x00000040,
        Button_No = 0x00000080,
        Button_Help = 0x00000100,
        Button_Try_Again = 0x00000200,
        Button_Continue = 0x00000400
    };

    enum AlertWindowType : int
    {
        Alert_Ok = Button_Ok,
        Alert_Close = Button_Close,
        Alert_Yes_No = Button_No | Button_Yes,
        Alert_Ok_Cancel = Button_Ok | Button_Cancel,
        Alert_Yes_No_Cancel = Button_Yes | Button_No | Button_Cancel,
        Alert_Abort_Retry_Ignore = Button_Abort | Button_Retry | Button_Ignore,
        Alert_Ok_Retry_Ignore = Button_Ok | Button_Retry | Button_Ignore,
        Alert_Ok_Help_Ignore = Button_Ok | Button_Help | Button_Ignore
    };

    /** Service used to report information to the user
     *
     */
    DECLARE_INTERFACE_(IReportingServiceV400, IUnknown)
    {
        /**
        *   Send message to content error reporting system
        *   @param    message      message to log
        **/
        virtual void ErrorReport(const char* message) abstract;
        /**
        *   Send message to content error reporting system
        *   @param    message      message to log
        **/
        virtual void ErrorReport(const WCHAR* message) abstract;
        /**
        *   Display an alert box to user with a message
        *   @param    message      message to log
        *   @param    message      window type that determines which buttons will display
        *   @return   buttom clicked
        **/
        virtual AlertWindowButton ShowAlert(const char* message, AlertWindowType type = Alert_Ok) abstract;
        /**
        *   Display an alert box to user with a message
        *   @param    message      message to log
        *   @param    message      window type that determines which buttons will display
        *   @return   buttom clicked
        **/
        virtual AlertWindowButton ShowAlert(const WCHAR* message, AlertWindowType type = Alert_Ok) abstract;
    };

    interface __declspec(uuid("{215BE739-7BE0-48C6-8877-6B226008F692}")) IReportingServiceV400;
    extern __declspec(selectany) REFIID IID_IReportingServiceV400 = __uuidof(IReportingServiceV400);
    extern __declspec(selectany) REFIID SID_ReportingService = IID_IReportingServiceV400;
}

/** \} */ // end doc group