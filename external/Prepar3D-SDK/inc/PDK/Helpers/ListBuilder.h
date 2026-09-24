// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// ListBuilder.h

#pragma once
#include <atlcomcli.h>
#include <vector>
#include <string>
namespace P3D
{
    /** @addtogroup types */ /** @{ */

    /** 
     *  Templated List Building Interface used by Prepar3D
     *  to add requested items to a list. 
     **/
    template <class T>
    class IListBuilder
    {
    public:
        /** 
         *  Called for each item until all items have been called or
         *  until AddItem returns false.
         *  @return true to continue adding items or false to stop
         **/
        virtual bool AddItem(T* item) abstract;
        /** 
         *  Called before any items are added.  
         **/
        virtual void BeginBuilding() abstract;
        /**
        *  Called after all items have been added.
        **/
        virtual void EndBuilding() {}
    };

    /**
    *  List builder implementation that creates a standard vector of 
    *  CComPtrs of a templated type.
    **/
    template <class T>
    class CComPtrVecBuilder : public  IListBuilder<T>
    {
    public:
        std::vector<CComPtr<T>> Items;
        
        virtual bool AddItem(T* item) override
        {
            Items.push_back(item);
            return true;
        }

        virtual void BeginBuilding() override
        {
            Items.clear();
        }
    };

    /**
    *  List builder implementation that creates a standard vector of
    *  objects of a templated type.
    **/
    template <class T>
    class VecListBuilder : public  IListBuilder<T>
    {
    public:
        std::vector<T> Items;

        virtual bool AddItem(T* item) override
        {
            Items.push_back(*item);
            return true;
        }

        virtual void BeginBuilding() override
        {
            Items.clear();
        }
    };

    using INameList = IListBuilder<const WCHAR>;

    /** 
     *    IListBuilder that stores names in a string vector. 
     *    The contents of each name are copied into the string so it is
     *    safe to store this list for later use.
     **/
    class NameListCopy : public  IListBuilder<const WCHAR>
    {
    public:
        /**
         *  Vector of strings that can be used to access the name list
         *  once it has been built.
         **/
        std::vector<std::basic_string<WCHAR>> Items;
        virtual bool AddItem(const WCHAR* item) override
        {
            Items.push_back(item);
            return true;
        }
        virtual void BeginBuilding() override
        {
            Items.clear();
        }
    };

    /**  
     * IListBuilder that stores names in a temporary const char* vector. 
     * The contents of each string are not copied, so this type is only safe to use within 
     * the scope of the current function. 
     **/
    class NameList : public  IListBuilder<const WCHAR>
    {
    public:
        /**
        *  const char* vector that can be used to access the name list
        *  once it has been built.
        **/
        std::vector<const WCHAR*> Items;
        virtual bool AddItem(const WCHAR* item) override
        {
            Items.push_back(item);
            return true;
        }

        virtual void BeginBuilding() override
        {
            Items.clear();
        }
    };

    /**
    * IListBuilder that stores names in a preallocated fixed size array.
    **/
    class NameListC : public  IListBuilder<const WCHAR>
    {
    public:
        /**
        * Constructor which takes preallocated fixed size array to hold this lists data.
        * @param    names   pointer to preallocated const char* array
        * @param    count   size of preallocated array.  Reference value that will be changed
        * to hold the count of items added to the list.
        **/
        NameListC(const WCHAR** names, int& count, bool bMakeCopy = false)
            : m_aNames(names), m_iMaxCount(count), m_iCount(count), m_bMakeCopy(bMakeCopy)
        {
            m_iCount = 0;
        }

        virtual bool AddItem(const WCHAR* item) override
        {
            if (m_iCount < m_iMaxCount)
            {
                if (m_bMakeCopy)
                {
                    WCHAR* newStr = new WCHAR[32];
                    wcscpy_s(newStr, 32, item);
                    m_aNames[m_iCount++] = newStr;
                }
                else
                {
                    m_aNames[m_iCount++] = item;
                }
                return true;
            }
            return false;
        }

        virtual void BeginBuilding() override
        {
            m_iCount = 0;
        }

    private:
        const WCHAR** m_aNames;
        int m_iMaxCount;
        int& m_iCount;
        bool m_bMakeCopy;
    };
    /** @} */
}