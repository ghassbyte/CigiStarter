// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// TSimPropSetVector.h

#pragma once
#include <vector>
#include "SimPropItems.h"

template <class T>
class TSimPropSetVector : public std::vector<T>, public ISimPropSetWrapper
{
public:
    virtual void SetChildCount(LONG count) override
    {
        this->resize(count);
    }

    virtual LONG GetChildCount() override
    {
        return (LONG)this->size();
    }

    virtual PVOID GetChildAddress(LONG index) override
    {
        if (index > -1 && index < (LONG)this->size())
        {
            return &this->at(index);
        }
        return nullptr;
    }
};