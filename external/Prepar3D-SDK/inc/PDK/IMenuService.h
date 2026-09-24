// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IMenuServices.h
// Prepar3D SDK interface for menu items.
//----------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include "IEventService.h"
#include "Helpers\ListBuilder.h"
#include <ObjBase.h>
#include <atlcomcli.h>

#define NO_PARENT            0x0000       
#define SCENARIO_ITEM_ID     0x1001
#define VEHICLE_ITEM_ID      0x1002
#define WORLD_ITEM_ID        0x1003
#define OPTIONS_ITEM_ID      0x1004
#define VIEWS_ITEM_ID        0x1005
#define HELP_ITEM_ID         0x1006
#define WINGMEN_ITEM_ID      0x1008
#define TOOLS_ITEM_ID        0x100d
#define TEST_ITEM_ID         0x100e
#define ADDON_ITEM_ID        0x1010
#define ANALYSIS_ITEM_ID     0x1011
#define NETWORK_ITEM_ID      0x1012
#define NAVIGATION_ITEM_ID   0x1013
#define ALL_CHILDREN         0xFFFF

namespace P3D
{
    /**  \defgroup menuservice    Menu Service
    * This service provides access to Prepar3D's menu system.  It can be used to add custom menu items
    * to the application menus.
    * \{
    **/

    enum MenuTypePdk
    {
        MENU_SEPARATOR_ITEM = 0,
        MENU_ITEM,
        MENU_CHECK_ITEM,
        MENU_BAR_ITEM,
        MENU_ROOT
    };


    // Forward declarations
    interface IMenuItemV410;

    using IMenuItemList = IListBuilder<IMenuItemV410>;
    using MenuItemList = CComPtrVecBuilder<IMenuItemV410>;
    
    /**
    * This service provides access to Prepar3D's menu system.
    **/
    DECLARE_INTERFACE_(IMenuServiceV410, IUnknown)
    {

        /**
        * Creates an menu item instance.
        * @return        An menu item instance.
        **/
        virtual IMenuItemV410* CreateMenuItem() PURE;

        /**
        * Add menu item to menu.  Item will be removed current position if it has already been 
        * added to the menu.
        * @param        uMenuID           The menu ID to be added.
        * @param        uParentID         The parent ID of the menu ID to be added.
        * @return       S_OK if the icon instance was successfully added, E_FAIL otherwise.
        **/
        virtual HRESULT AddItem(USHORT uMenuID, USHORT uParentID, int nInsertIndex) PURE;

        /**
        * Removes a developer defined menu item.
        * @param        uItemID           The menu ID to be destroyed.
        * @return       S_OK if the icon instance was successfully removed, E_FAIL otherwise.
        **/
        virtual HRESULT RemoveItem(USHORT uItemID, USHORT uParentID) PURE;
    };
    // {5310E926-8B20-46F2-B82D-03279770DF91}
    DEFINE_GUID(IID_IMenuServiceV410, 0x5310e926, 0x8b20, 0x46f2, 0xb8, 0x2d, 0x3, 0x27, 0x97, 0x70, 0xdf, 0x91);
    // {7D7A6F58-5A30-4FA4-8CB7-DB7EC4E06DB7}
    DEFINE_GUID(SID_MenuService, 0x7d7a6f58, 0x5a30, 0x4fa4, 0x8c, 0xb7, 0xdb, 0x7e, 0xc4, 0xe0, 0x6d, 0xb7);
    // {19FEC0CB-8665-48D4-B03C-ACB680FC3708}
    DEFINE_GUID(P3D_EVENT_GUID, 0x19fec0cb, 0x8665, 0x48d4, 0xb0, 0x3c, 0xac, 0xb6, 0x80, 0xfc, 0x37, 0x8);

    /**
    * This interface may be used by developers to manipulate menu items.
    **/
    DECLARE_INTERFACE_(IMenuItemV410, IUnknown)
    {
        virtual USHORT  GetId() const PURE;
        virtual LPCWSTR GetText() const PURE;
        virtual void SetText(LPCWSTR text) PURE;
        virtual MenuTypePdk GetType() const PURE;
        virtual void SetType(MenuTypePdk type) PURE;
        virtual bool IsChecked() const PURE;
        virtual void SetChecked(bool checked)PURE;
        virtual void SetActivated(bool activated) PURE;

        virtual void RegisterCallback(ICallbackV400 * pCallback) PURE;
        virtual void UnregisterCallback(ICallbackV400 * pCallback) PURE;

    };
    // {AF4E5502-4860-430B-9D1B-A85707672673}
    DEFINE_GUID(IID_IMenuItemV410, 0xaf4e5502, 0x4860, 0x430b, 0x9d, 0x1b, 0xa8, 0x57, 0x7, 0x67, 0x26, 0x73);
    // {57607E73-EEE5-4FEA-B8F6-96650CC02968}
    DEFINE_GUID(SID_IMenuItemV410, 0x57607e73, 0xeee5, 0x4fea, 0xb8, 0xf6, 0x96, 0x65, 0xc, 0xc0, 0x29, 0x68);

    /** \}*/ // end of doc group
};
