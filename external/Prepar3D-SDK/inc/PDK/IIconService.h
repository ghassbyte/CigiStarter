// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IIconServices.h
// Prepar3D SDK interface for icons
//----------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include "Helpers\ListBuilder.h"

namespace P3D
{
    /**  \defgroup iconservice    Icon Service
    * This PDK service allows callers to manipulate icons during runtime.
    * Icons registered and unregistered using this interface.
    * Icons utilize world-based positioning and may or may not be associated with an object.
    * \{
    **/

    // Forward declarations
    interface IIconInstanceV410;

    using IIconInstanceList = IListBuilder<IIconInstanceV410>;
    using IconInstanceList = CComPtrVecBuilder<IIconInstanceV410>;

    /**
    * This service is responsible for the management of Icons.
    **/
    DECLARE_INTERFACE_(IIconServiceV410, IUnknown)
    {
        /**
        * Returns the number of icon instances.
        * @return       The number of icon instances.
        **/
        virtual UINT32  GetIconInstanceCount() const PURE;

        /**
        * Returns a list of icon instances.
        * @param        pList                   The list of Icon Instances.
        **/
        virtual void    GetIconInstanceList(__out IIconInstanceList& pList) PURE;

        /**
        * Creates an icon instances.
        * @return        An Icon Instance.
        **/
        virtual IIconInstanceV410* CreateIconInstance() PURE;

        /**
        * Registers a developer defined icon instance.
        * @param        pIconInstance           The IIconInstanceV410 to be registered.
        * @return       S_OK if the icon instance was successfully registered, E_FAIL otherwise.
        **/
        virtual HRESULT Register(__in IIconInstanceV410* pIconInstance) PURE;

        /**
        * Unregisters a developer defined icon instance.
        * @param        pIconInstance           The icon instance to be destroyed.
        * @return       S_OK if the icon instance was successfully unregistered, E_FAIL otherwise.
        **/
        virtual HRESULT Unregister(__in IIconInstanceV410* pIconInstance) PURE;
    };

    DEFINE_GUID(IID_IIconServiceV410, 0xfad342c0, 0x47ca, 0x4725, 0x9e, 0x67, 0xe5, 0xd0, 0x19, 0x60, 0x8c, 0xab);
    DEFINE_GUID(SID_IconService, 0xa9ea0fed, 0x1ceb, 0x44a7, 0xac, 0xd4, 0x18, 0x49, 0x17, 0x28, 0x4d, 0xdb);

    /**
    * This interface may be used by developers to manipulate icon instances.
    **/
    DECLARE_INTERFACE_(IIconInstanceV410, IUnknown)
    {
        /**
        * Returns the position of a given icon instance.
        * @param        vWorldPosRadiansFeet    The world location in radians/feet.
        **/
        virtual void    GetPosition(__out DXYZ& vWorldPosRadiansFeet) const PURE;

        /**
        * Sets the position of a given icon instance.
        * @param        vWorldPosRadiansFeet    The world location in radians/feet.
        **/
        virtual void    SetPosition(__in const DXYZ& vWorldPosRadiansFeet) PURE;

        /**
        * Attaches the position of a given icon instance to an object.
        * @param        objectId                The object's id.
        * @return       Will return 0 if there is no object id, otherwise it will return object's id.
        **/
        virtual UINT    GetObjectId() const PURE;

        /**
        * Attaches the position of a given icon instance to an object.
        * @param        objectId                The object's id.
        * @return       S_OK if the icon instance was successfully set to an object, E_FAIL otherwise.
        * @remarks      An objectId of 0 will detach the icon instance from an object.
        *               SetObject will override SetPosition until SetObject(0) is called.
        **/
        virtual HRESULT SetObjectId(__in UINT objectId) PURE;
        
        /**
        * Sets the texture of a given icon instance.
        * @param        guid                    The icon/texture guid.
        * @return       Returns the guid texture reference.
        * @remarks      The GUID mapping is set in the IconConfiguration.xml file.
        **/
        virtual const GUID& GetTextureDefinition() const PURE;

        /**
        * Sets the texture of a given icon instance.
        * @param        guid                    The icon/texture guid.
        * @return       S_OK if the guid was successfully changed, E_FAIL otherwise.
        * @remarks      The GUID mapping is set in the IconConfiguration.xml file.
        **/
        virtual HRESULT SetTextureDefinition(__in const GUID& guid) PURE;

        /**
        * Returns visibility status of a given icon instance.
        * @return       true if visibility is set to true, false if visibility is set to false.
        * @remarks      visibility = true draws the icon, visibility = false doesn't draw the icon.
        **/
        virtual bool IsVisible() const PURE;

        /**
        * Sets visibility status of a given icon instance.
        * @param        visibility                 The visibility flag.
        * @remarks      visibility = true draws the icon, visibility = false doesn't draw the icon.
        **/
        virtual void SetVisibility(__in bool visibility) PURE;
    };

    DEFINE_GUID(IID_IIconInstanceV410, 0xd8f62591, 0x64ec, 0x4dc1, 0xb4, 0x78, 0xa2, 0x76, 0xe0, 0x7d, 0xa7, 0x7);
    DEFINE_GUID(SID_IconInstance, 0xd3f30e48, 0xdd93, 0x4828, 0xb8, 0xce, 0x9b, 0xab, 0x24, 0x26, 0xef, 0x2f);

    /** \}*/ // end of doc group
};
