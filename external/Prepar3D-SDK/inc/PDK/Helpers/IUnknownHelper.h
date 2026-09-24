// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IUnknownHelper.h

#ifndef _IUNKNOWNHELPER_H
#define _IUNKNOWNHELPER_H

#pragma once
/** @addtogroup types */ /** @{ */
/***********************************************************************************************
***********************************************************************************************/
#define DEFAULT_REFCOUNT_INLINE_IMPL()\
public:\
    volatile LONG   m_RefCount = 1;\
    virtual ULONG STDMETHODCALLTYPE AddRef() {return InterlockedIncrement(&m_RefCount) ;}\
    virtual ULONG STDMETHODCALLTYPE Release()\
    {\
        ULONG   RetVal = InterlockedDecrement(&m_RefCount);\
        if      (RetVal == 0)         {delete this;}\
        else if (RetVal & 0x80000000) {__debugbreak();}\
        return RetVal;\
    }\

/***********************************************************************************************
***********************************************************************************************/
#define DECLARE_IUNKNOWN_WITH_INLINE_REFCOUNT_IMPL()\
public:\
    DEFAULT_REFCOUNT_INLINE_IMPL()\
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(__in REFIID riid, __out void **ppvObject);\

/***********************************************************************************************
***********************************************************************************************/

#define NO_COPY_IUNKNOWN_IMPL(IClassName) \
    private:  \
        IClassName(const IClassName&); \
        IClassName& operator=(const IClassName&);

/** @} */

#endif // _IUNKNOWNHELPER_H
