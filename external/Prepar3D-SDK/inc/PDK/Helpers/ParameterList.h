// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// ParameterList.h

#pragma once

#include "InterfaceServiceWrapper.h"
#include "IEventService.h"
#include "IUnknownHelper.h"
#include "Types.h"

#include <vector>
#include <string>

namespace P3D
{
    /* Basic callback implementation */
    class P3dCallback : public ICallbackV400
    {
        DEFAULT_REFCOUNT_INLINE_IMPL();

    public:

        P3dCallback() noexcept : m_RefCount(1)  { }
        
        virtual void Invoke(IParameterListV400* pParams) abstract;

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == nullptr)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_ICallbackV400))
            {
                *ppv = static_cast<ICallbackV400*>(this);
            }
            else if (IsEqualIID(riid, IID_IUnknown))
            {
                *ppv = static_cast<IUnknown*>(this);
            }
            if (*ppv)
            {
                hr = S_OK;
                AddRef();
            }

            return hr;
        };
    };
    

    // Custom parameter implementation
    class CustomParameter : public ICustomParameterV600
    {
        DEFAULT_REFCOUNT_INLINE_IMPL();

    public:

        CustomParameter() :
            m_RefCount(1)
        {
            Clear();
        }

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == nullptr)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_ICustomParameterV600))
            {
                *ppv = static_cast<ICustomParameterV600*>(this);
            }
            else if (IsEqualIID(riid, IID_IUnknown))
            {
                *ppv = static_cast<IUnknown*>(this);
            }

            if (*ppv)
            {
                hr = S_OK;
                AddRef();
            }

            return hr;
        };

        virtual const wchar_t* GetName()            const override { return Name.c_str(); }
        virtual bool GetValue(bool& value)          const override { return GetValueT<bool>(value, Value.bValue, ParameterType::Boolean); }
        virtual bool GetValue(int& value)           const override { return GetValueT<int>(value, Value.iValue, ParameterType::Integer); }
        virtual bool GetValue(unsigned& value)      const override { return GetValueT<unsigned>(value, Value.uValue, ParameterType::Unsigned); }
        virtual bool GetValue(float& value)         const override { return GetValueT<float>(value, Value.fValue, ParameterType::Float); }
        virtual bool GetValue(double& value)        const override { return GetValueT<double>(value, Value.dValue, ParameterType::Double); }
        virtual bool GetValue(UINT64& value)        const override { return GetValueT<UINT64>(value, Value.uValue64, ParameterType::Unsigned64); }
        virtual bool GetValue(P3D::P3DFXYZ& value)  const override { return GetValueT<P3D::P3DFXYZ>(value, Value.fxyzValue, ParameterType::Float3); }
        virtual bool GetValue(P3D::P3DDXYZ& value)  const override { return GetValueT<P3D::P3DDXYZ>(value, Value.dxyzValue, ParameterType::Double3); }
        virtual bool GetValue(GUID& value)          const override { return GetValueT<GUID>(value, Value.guidValue, ParameterType::GUID); }
        virtual bool GetValue(void*& value)         const override { return GetValueT<void*>(value, Value.pValue, ParameterType::Pointer); }

        virtual bool GetValue(wchar_t* value, unsigned uLength) const override
        {
            bool bResult = false;

            if (Type == ParameterType::String)
            {
                if (value && uLength > 0)
                {
                    if (0 == wcscpy_s(value, uLength, String.c_str()))
                    {
                        bResult = true;
                    }
                }
            }

            return bResult;
        }

        virtual ParameterType GetType() const override
        {
            return Type;
        }

        void SetValue(bool value)                   { SetValueT<bool>(value, Value.bValue, ParameterType::Boolean); }
        void SetValue(int value)                    { SetValueT<int>(value, Value.iValue, ParameterType::Integer); }
        void SetValue(unsigned value)               { SetValueT<unsigned>(value, Value.uValue, ParameterType::Unsigned); }
        void SetValue(float value)                  { SetValueT<float>(value, Value.fValue, ParameterType::Float); }
        void SetValue(double value)                 { SetValueT<double>(value, Value.dValue, ParameterType::Double); }
        void SetValue(UINT64 value)                 { SetValueT<UINT64>(value, Value.uValue64, ParameterType::Unsigned64); }
        void SetValue(const P3D::P3DFXYZ& value)    { SetValueT<P3D::P3DFXYZ>(value, Value.fxyzValue, ParameterType::Float3); }
        void SetValue(const P3D::P3DDXYZ& value)    { SetValueT<P3D::P3DDXYZ>(value, Value.dxyzValue, ParameterType::Double3); }
        void SetValue(const GUID& value)            { SetValueT<GUID>(value, Value.guidValue, ParameterType::GUID); }
        void SetValue(void* value)                  { SetValueT<void*>(value, Value.pValue, ParameterType::Pointer); }

        void SetName(const wchar_t* name)
        {
            if (name)
            {
                Name = name;
            }
            else
            {
                Name.clear();
            }
        }

        void SetValue(const wchar_t* value)
        {
            Clear();
            if (value)
            {
                String = value;
            }
            Type = ParameterType::String;
        }

        void Clear()
        {
            Type = ParameterType::Unknown;
            String.clear();
            Value.bValue = false;
        }

    private:

        template<typename T>
        bool GetValueT(T& value, const T& member, ParameterType type) const
        {
            if (Type == type)
            {
                value = member;
                return true;
            }
            else
            {
                return false;
            }
        }

        template<typename T>
        void SetValueT(const T& value, T& member, ParameterType type)
        {
            Clear();
            member = value;
            Type = type;
        }

        union
        {
            bool bValue;
            int iValue;
            unsigned uValue;
            float fValue;
            double dValue;
            UINT64 uValue64;
            P3D::P3DFXYZ fxyzValue;
            P3D::P3DDXYZ dxyzValue;
            GUID guidValue;
            void* pValue;
        } Value;                    // the value of the param
        std::wstring    String;     // string value type

        std::wstring    Name;       // the name of the param
        ParameterType   Type;       // the type of the param
    };

    class __declspec(uuid("{a36f2ddf-f07b-425B-8954-28ce5b8242e0}")) CustomParameterList;
    extern __declspec(selectany) REFIID CLSID_CustomParameterList = __uuidof(CustomParameterList);
    
    /**
    *  Dynmic parameter list with key value pairs
    */
    class CustomParameterList : public ICustomParameterListV600
    {
        DEFAULT_REFCOUNT_INLINE_IMPL();

    public:

        CustomParameterList(IServiceProvider* pService = nullptr) :
            m_RefCount(1),
            m_pServiceProvider(pService)
        {
            if (pService == nullptr)
            {
                m_pServiceProvider.Attach(new InterfaceServiceWrapper(nullptr));
            }
        }

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == nullptr)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, CLSID_CustomParameterList))
            {
                *ppv = static_cast<CustomParameterList*>(this);
            }
            else if (IsEqualIID(riid, IID_ICustomParameterListV600))
            {
                *ppv = static_cast<ICustomParameterListV600*>(this);
            }
            else if (IsEqualIID(riid, IID_IParameterListV400))
            {
                *ppv = static_cast<IParameterListV400*>(this);
            }
            else if (IsEqualIID(riid, IID_IUnknown))
            {
                *ppv = static_cast<IUnknown*>(this);
            }
            if (*ppv)
            {
                hr = S_OK;
                AddRef();
            }

            return hr;
        };

        virtual IServiceProvider* GetServiceProvider()
        {
            return m_pServiceProvider;
        }

        virtual P3DParameter GetParameter(UINT32 index)
        {
            P3DParameter param = { 0 };

            if (index < m_Params.size())
            {
                param.Value = GetAsUInt64(m_Params[index]);
            }

            return param;
        }

        virtual UINT32 GetCount()
        {
            return static_cast<UINT32>(m_Params.size());
        }

        virtual const ICustomParameterV600* GetParam(UINT32 index) const override
        {
            if (index < m_Params.size())
            {
                return m_Params[index];
            }

            return nullptr;
        }

        virtual const ICustomParameterV600* GetParam(const wchar_t* pszName) const override
        {
            return FindParam(pszName);
        }

        CustomParameter* GetOrCreateParam(UINT32 index)
        {
            if (index < m_Params.size())
            {
                return m_Params[index];
            }

            UINT32 uCount = static_cast<UINT32>((index + 1) - m_Params.size());
            for (UINT32 i = 0; i < uCount; ++i)
            {
                CComPtr<CustomParameter> spParam;
                spParam.Attach(new CustomParameter());
                m_Params.push_back(spParam);
            }

            return m_Params[index];
        }

        CustomParameter* GetOrCreateParam(const wchar_t* pszName)
        {
            if (!pszName)
            {
                return nullptr;
            }

            CustomParameter* pParam = FindParam(pszName);

            if (!pParam)
            {
                CComPtr<CustomParameter> spParam;
                spParam.Attach(new CustomParameter());
                spParam->SetName(pszName);
                m_Params.push_back(spParam);
                pParam = m_Params.back();
            }

            return pParam;
        }

        CustomParameter* FindParam(const wchar_t* pszName)
        {
            CustomParameter* pParam = nullptr;

            if (pszName)
            {
                for (UINT32 i = 0; i < m_Params.size(); ++i)
                {
                    if (wcscmp(pszName, m_Params[i]->GetName()) == 0)
                    {
                        pParam = m_Params[i];
                        break;
                    }
                }
            }

            return pParam;
        }

        const CustomParameter* FindParam(const wchar_t* pszName) const
        {
            CustomParameter* pParam = nullptr;

            if (pszName)
            {
                for (UINT32 i = 0; i < m_Params.size(); ++i)
                {
                    if (wcscmp(pszName, m_Params[i]->GetName()) == 0)
                    {
                        pParam = m_Params[i];
                        break;
                    }
                }
            }

            return pParam;
        }

        void Clear() { m_Params.clear(); }

    protected:

        UINT64 GetAsUInt64(ICustomParameterV600* pParam) const
        {
            if (!pParam)
            {
                return 0;
            }
            else if (pParam->GetType() == ParameterType::Boolean)
            {
                bool bVal = false;
                pParam->GetValue(bVal);
                return bVal;
            }
            else if (pParam->GetType() == ParameterType::Integer)
            {
                int iVal = 0;
                pParam->GetValue(iVal);
                return iVal;
            }
            else if (pParam->GetType() == ParameterType::Unsigned)
            {
                unsigned uVal = 0;
                pParam->GetValue(uVal);
                return uVal;
            }
            else if (pParam->GetType() == ParameterType::Float)
            {
                float fVal = 0.0f;
                pParam->GetValue(fVal);
                return static_cast<UINT64>(fVal);
            }
            else if (pParam->GetType() == ParameterType::Double)
            {
                double dVal = 0.0;
                pParam->GetValue(dVal);
                return static_cast<UINT64>(dVal);
            }
            else if (pParam->GetType() == ParameterType::Unsigned64)
            {
                UINT64 uVal = 0;
                pParam->GetValue(uVal);
                return uVal;
            }
            else if (pParam->GetType() == ParameterType::Pointer)
            {
                void* pVal = nullptr;
                pParam->GetValue(pVal);
                return *static_cast<UINT64*>(pVal);
            }
            else
            {
                return 0;
            }
        }

        CComPtr<IServiceProvider> m_pServiceProvider;
        std::vector<CComPtr<CustomParameter>> m_Params;
    };
    
    /**
    *  Basic parameter list with two parameters
    */
    class ParameterList : public IParameterListV400
    {
        DEFAULT_REFCOUNT_INLINE_IMPL();

    public:

        ParameterList(IServiceProvider* pService, UINT64 uParam0, UINT64 uParam1) :
            m_RefCount(1),
            m_pServiceProvider(pService)
        {
            if (pService == nullptr)
            {
                m_pServiceProvider.Attach(new InterfaceServiceWrapper(nullptr));
            }
            m_Param0.Value = uParam0;
            m_Param1.Value = uParam1;
        }

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == nullptr)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_IParameterListV400))
            {
                *ppv = static_cast<IParameterListV400*>(this);
            }
            else if (IsEqualIID(riid, IID_IUnknown))
            {
                *ppv = static_cast<IUnknown*>(this);
            }
            if (*ppv)
            {
                hr = S_OK;
                AddRef();
            }

            return hr;
        };

        virtual IServiceProvider* GetServiceProvider()
        {
            return m_pServiceProvider;
        }

        virtual P3DParameter GetParameter(UINT32 index)
        {     
            switch (index)
            {
            case 0:
                return m_Param0;
                break;
            case 1:
                return m_Param1;
                break;
            }
            P3DParameter zeroParam = { 0 };
            return zeroParam;
        }

        virtual UINT32 GetCount()
        {
            return 2;
        }
    protected:
        CComPtr<IServiceProvider> m_pServiceProvider;
        P3DParameter m_Param0;
        P3DParameter m_Param1;
        static const P3DParameter PARAM_ZERO;
    };
}