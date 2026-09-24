// Copyright (c) 2010-2020 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IParameterList.h

#pragma once

#include <ObjBase.h>

namespace P3D
{
    /**
    *  Parameter values used for IParameterListV400
    **/
    struct P3DParameter
    {
        UINT64 Value;
    };

    /**
    *   Parameter list interface used by ICallbackV400
    **/
    DECLARE_INTERFACE_(IParameterListV400, IUnknown)
    {
        /**
        *   Get service provider.  Use QueryService to request services from it.
        *   @return Currently all callbacks pass
        *   an IPdk pointer as the service provider, but some callbacks may
        *   provide callback-specific providers in the future.
        **/
        virtual IServiceProvider* GetServiceProvider() abstract;
        /**
        *   Get P3DParameter by index.
        *   @param    index   parameter index;
        *   @return   The parameter requested.
        **/
        virtual P3DParameter GetParameter(UINT32 index) abstract;
        /**
        *   Get parameter count.
        *   @param    index   parameter index;
        *   @return   number of parameters in the params list.
        **/
        virtual UINT32 GetCount() abstract;
    };

    interface __declspec(uuid("{075551e8-0885-4711-9f7f-a6eb6a61da6c}")) IParameterListV400;
    extern __declspec(selectany) REFIID IID_IParameterListV400 = __uuidof(IParameterListV400);

    /* Parameter value type */
    enum class ParameterType
    {
        Unknown,
        Boolean,
        Integer,
        Unsigned,
        Float,
        Double,
        Unsigned64,
        Float3,
        Double3,
        GUID,
        Pointer,
        String
    };

    /* Custom parameter interface */
    DECLARE_INTERFACE_(ICustomParameterV600, IUnknown)
    {
        virtual const wchar_t* GetName() const abstract;
        virtual bool GetValue(bool& value) const abstract;
        virtual bool GetValue(int& value) const abstract;
        virtual bool GetValue(unsigned& value) const abstract;
        virtual bool GetValue(float& value) const abstract;
        virtual bool GetValue(double& value) const abstract;
        virtual bool GetValue(UINT64& value) const abstract;
        virtual bool GetValue(P3D::P3DFXYZ& value) const abstract;
        virtual bool GetValue(P3D::P3DDXYZ& value) const abstract;
        virtual bool GetValue(GUID& value) const abstract;
        virtual bool GetValue(void*& value) const abstract;
        virtual bool GetValue(wchar_t* value, unsigned uLength) const abstract;
        virtual ParameterType GetType() const abstract;
    };

    interface __declspec(uuid("{ea647c84-e60d-4f6f-9a24-d3f268d2aae8}")) ICustomParameterV600;
    extern __declspec(selectany) REFIID IID_ICustomParameterV600 = __uuidof(ICustomParameterV600);

    // Custom parameter list interface
    DECLARE_INTERFACE_(ICustomParameterListV600, IParameterListV400)
    {
        /*IParameterListV400*/
        virtual IServiceProvider* GetServiceProvider() abstract;
        virtual P3DParameter GetParameter(UINT32 index) abstract;
        virtual UINT32 GetCount() abstract;

        /*ICustomParameterListV600*/
        virtual const ICustomParameterV600* GetParam(UINT32 index) const abstract;
        virtual const ICustomParameterV600* GetParam(const wchar_t* pszName) const abstract;
    };

    interface __declspec(uuid("{1acb97e0-a988-490e-9e05-d6336b5d3403}")) ICustomParameterListV600;
    extern __declspec(selectany) REFIID IID_ICustomParameterListV600 = __uuidof(ICustomParameterListV600);
}

