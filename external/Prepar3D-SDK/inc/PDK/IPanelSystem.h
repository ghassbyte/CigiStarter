// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// IPanelSystem.h
// Prepar3D SDK interface for accessing functionality of the panel system.
//----------------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>

#include "Helpers\GaugeTypes.h"
#include "Legacy\IPanelSystem.Legacy.h"

namespace P3D
{
     /** \defgroup   panelsystem  Panel System Service
     * The IPanelSystem interface provides functionality and services previously
     * only provided through the ImportTable in gauges.h.  This service alleviates the need
     * to declare the ImportTable and associated linkage.  
     *
     * The initial version mimics the functions specified in the gauges.h ImportTable.
     * However, it changes the naming convention to a Camel Case Notation to differentiate the names.
     * For example:  function_sample() would be changed to FunctionSample().
     * Functionality remains the same.
     *
     * An IPdk pointer can be obtained from the IPdk QueryService() method.  
     * \{ 
     */

/**
*   Parameter list interface used by IPanelSystemV520
**/
DECLARE_INTERFACE_(IPanelSystemV520, IPanelSystemV400)
{
    /**
    *   Returns if the specified panel window is visible. 
    *   @param      panel_id  Specifies the identification number of the window to query. The identification number is specified in the Panel.cfg file in the [WindowXX] section by the variable ident.  
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    **/
    STDMETHOD_(BOOL,    IsPanelWindowVisibleIdent) (UINT32 panel_id)                                                PURE;
    /**
    *   Specifies or retrieves the tooltip units (metric or US). 
    *   @param[in]      action   Specifies the action. If this value is less than zero, the units are toggled (between metric and US imperial). If this value equals zero, the enum value for the units is returned. If this value is greater than zero, the units are set to the value of the type parameter. 
    *   @param[in]      type   The enum value to set the tooltip units to, one of the below TOOLTIP_UNITS_TYPE enumeration types.
    *   @return     The function returns one member of the TOOLTIP_UNITS_TYPE enumeration below.
    *   @remarks    
    *
    *           enum TOOLTIP_UNITS_TYPE 
    *           {
    *               TOOLTIP_UNITS_TYPE_DEFAULT,
    *               TOOLTIP_UNITS_TYPE_METRIC,
    *               TOOLTIP_UNITS_TYPE_US,
    *           } 
    **/
    STDMETHOD_(ENUM,    TooltipUnitsGetSet) (int action, ENUM type)                                                 PURE;
    /**
    *   Function used to register the failure conditions for a gauge. 
    *   @param[in]      element  A pointer to the element header. 
    *   @remarks    Call this function once, after ElementListInstall.
    **/
    STDMETHOD_(void,    ElementListQuery) (PELEMENT_HEADER element)                                                 PURE;
    /**
    *   Function used to install a gauge during initialization of a panel.
    *   @param[in]      element   A pointer to the element header. 
    *   @param[in]      resource_file_handle   Specifies the resource file handle.
    *   @remarks    Use this function before calling any of the other element_list functions. A gauge is defined by an element list, however the macros used to create gauges described in the tutorial mean that in many cases these functions do not need to be called directly. 
    **/
    STDMETHOD_(void,    ElementListInstall) (PELEMENT_HEADER element, PVOID resource_file_handle)                   PURE;
    /**
    *   Function to prepare a gauge before being redrawn.
    *   @param[in]      element   A pointer to the element header. 
    *   @remarks    See the remarks for ElementListPlot.   
    **/
    STDMETHOD_(void,    ElementListInitialize) (PELEMENT_HEADER element)                                            PURE;
    /**
    *   Function to calculate new values for each gauge element. 
    *   @param[in]      element   A pointer to the element header. 
    *   @remarks    See the remarks for ElementListPlot.   
    **/
    STDMETHOD_(void,    ElementListUpdate) (PELEMENT_HEADER element)                                                PURE;
    /**
    *   Function to regenerate the effects mask and images for a gauge. 
    *   @param[in]      element   A pointer to the element header. 
    *   @param[in]      phase   Unused, enter zero. 
    *   @remarks    See the remarks for ElementListPlot.   
    **/
    STDMETHOD_(void,    ElementListGenerate) (PELEMENT_HEADER element, GENERATE_PHASE phase)                        PURE;
    /**
    *   Function to redraw a gauge. 
    *   @param[in]      element   A pointer to the element header. 
    *   @remarks    Before calling this function, each and every time, call the following functions in this order:
    *
    *                   1.ElementListInitialize
    *                   2.ElementListUpdate
    *                   3.ElementListGenerate
    *                   4.ElementListPlot
    *
    **/
    STDMETHOD_(void,    ElementListPlot) (PELEMENT_HEADER element)                                                  PURE;   
    /**
    *   Function used before redrawing a gauge.
    *   @param[in]      element   A pointer to the element header. 
    *   @remarks    It is not necessary to call this function if the redrawing of the gauge is handled by ElementListPlot.  
    **/
    STDMETHOD_(void,    ElementListErase) (PELEMENT_HEADER element)                                                 PURE;   
    /**
    *   Function used to remove a gauge completely, for example when changing user aircraft. 
    *   @param[in]      element   A pointer to the element header. 
    **/
    STDMETHOD_(void,    ElementListKill) (PELEMENT_HEADER element)                                                  PURE;
    /**
    *   Creates the mouse rectangles for a gauge.
    *   @param[in, out]    rect   Specifies a pointer to a list of MOUSERECT structures. 
    *             The first rectangle in the list is the main box. The rectangles are scaled 
    *             according to the settings in the gauge_header. The last rectangle structure in the 
    *             list should have the type MOUSE_RECT_EOL.  
    *   @param[in]    gauge_header   Specifies a pointer to a gauge_header structure (defined in gauges.h). 
    *   @param[in]    size   Specifies a pointer to a PIXPOINT structure, which contains an x and y value, defining the size of the rectangle.
    *   @remarks  Call MouseListRegister after setting up the mouse rectangles with this function. Note that the  mouse rectangle creation macros can be used instead of these low level function calls. 
    **/
    STDMETHOD_(void,    MouseListInstall) (PMOUSERECT rect, PGAUGEHDR gauge_header, PPIXPOINT size)                 PURE;
    /**
    *   Registers the mouse windows. 
    *   @param[in]      rect   Specifies a pointer to a MOUSERECT structure.   
    *   @param[in]      gauge_header   Specifies a pointer to a gauge_header structure (defined in gauges.h). 
    *   @return     
    **/
    STDMETHOD_(void,    MouseListRegister) (PMOUSERECT rect, PGAUGEHDR gauge_header)                                PURE;
    /**
    *   Unregisters the mouse windows.
    *   @param[in]      rect   Specifies a pointer to a MOUSERECT structure.   
    *   @param[in]      gauge_header   Specifies a pointer to a gauge_header structure (defined in gauges.h). 
    *   @return     
    **/
    STDMETHOD_(void,    MouseListUnregister) (PMOUSERECT rect, PGAUGEHDR gauge_header)                              PURE;
    /**
    *   Toggles the visible state of a panel window. 
    *   @param[in]  panel_id   Specifies the identification number of the window to toggle. The identification number is specified in the Panel.cfg file in the [WindowXX] section by the variable ident.     
    *   @return If the function succeeds, it returns a non-zero value. If it fails, it returns zero.     
    **/
    STDMETHOD_(BOOL,    PanelWindowToggle) (UINT32 panel_id)                                                        PURE;
    /**
    *   Initiates the action of a key event. 
    *   @param[in]  event_id   Specifies the event ID. Refer to the list of key events in the EventIDs document, and the #define KEY_events in gauges.h.       
    *   @param[in]  value   Specifies an additional integer value. Set this to zero if it is not required. 
    *   @return The function returns an ERR, which is usually ignored. If the event requested is not appropriate, it simply will not happen.      
    **/
    STDMETHOD_(ERR ,    TriggerKeyEvent) (ID32 event_id, UINT32 value)                                              PURE;
    /**
    *   Registers a variable from another gauge, for use by this gauge. 
    *   @param[in]      var   Specifies the address of the variable.
    *   @param[in]    var_type  Specifies the type of the variable, one of the following enum: 
    *
    *       typedef enum VAR_TYPE {
    *           VAR_TYPE_NONE,
    *           TYPE_BOOL8,
    *           TYPE_UINT8,
    *           TYPE_SINT8,
    *           TYPE_FLAGS8,
    *           TYPE_ENUM8,
    *           TYPE_BOOL16,
    *           TYPE_ANGL16,
    *           TYPE_UINT16,
    *           TYPE_SINT16,
    *           TYPE_UIF16,
    *           TYPE_SIF16,
    *           TYPE_FLAGS16,
    *           TYPE_ENUM16,
    *           TYPE_BCD16,
    *           TYPE_BCO16,
    *           TYPE_VAR16,
    *           TYPE_BOOL32,
    *           TYPE_ANGL32,
    *           TYPE_UINT32,
    *           TYPE_SINT32,
    *           TYPE_UIF32,
    *           TYPE_SIF32,
    *           TYPE_FLAGS32,
    *           TYPE_ENUM32,
    *           TYPE_VAR32,
    *           TYPE_ANGL48,
    *           TYPE_SINT48,
    *           TYPE_UIF48,
    *           TYPE_SIF48,
    *           TYPE_UINT64,
    *           TYPE_SINT64,
    *           TYPE_SIF64,
    *           TYPE_FLOAT64,
    *           TYPE_BOOL,
    *           TYPE_FLAGS,
    *           TYPE_ENUM,
    *           TYPE_VOID,
    *           TYPE_PVOID,
    *           TYPE_PUINT32,
    *           TYPE_PSINT32,
    *           TYPE_PFLOAT64,
    *           VAR_TYPE_MAX
    *       } VAR_TYPE;
    *
    *    @remarks  
    *    You can use named variables to enable communication between two or more gauges. To establish communication between gauges, a "server" and "client" gauge needs to be defined. 
    *    The terms "server" and "client" just distinguish between variable ownership and variable usage:\n
    *    *The server gauges provides one or more named variables for other gauges to access.\n
    *    *The client gauges accesses one or more named variables from the server gauges.\n
    *    A single gauge can be both a server (by providing one or more variables) and a client (by accessing another gauge's variables) at the same time. Use the RegisterVarByName, 
    *    UnregisterVarByName, and InitializeVarByName functions with named variables. The server gauge uses the RegisterVarByName function to register a named variable with 
    *    the panel system at startup, so create a callback for your gauge as part of the gauge_header structure. You can set this so it performs at startup, on shutdown, etc.
    *    When using named variables, don't call the LookupVar function (as you would with the standard panel system variables). After InitializeVarByName is called, the var_ptr 
    *    field of the  MODULE_VAR structure contains a pointer to the named variable. The panel system doesn't recognize named variables, per se, but the system does maintain the name 
    *    to the pointer link for gauges to query. As a result, you can't use a named variable as a controlling variable for an ELEMENT structure directly. Instead, use a MODULE_VAR_NONE 
    *    structure and provide a callback function that can query the variable's value using the var_ptr field of the  MODULE_VAR structure.
    *    Because named variables work via direct pointers between gauges, make sure that the server gauge is loaded before, or at the same time as, the client gauge. You can make sure 
    *    this happens by either putting both gauges on the same panel window or by putting the server gauge on the main panel window. This ensures that the server gauge is loaded and 
    *    the named variable is registered before the client gauge tries to connect to it. Alternatively, you can check the returned var_ptr for NULL and the returned var_type (both 
    *    in the MODULE_VAR structure) for VAR_TYPE_NONE and execute (in the ELEMENT callback function) the InitializeVarByName function until it returns valid information. (You can 
    *    also call the InitializeVarByName function every time you want to access the variable, but this approach is a little slower than caching the information once it's received.). 
    *    The server gauge must keep checking the current value of the variable(s) it has made available, if the current state/value of that variable has any effect.
    *    You can use named variables at any point in the simulation when you want to pass information between two or more gauges. Because named variables are shared by direct pointer 
    *    access, you can also share out an entire data structure using one named variable, as long as the server and client gauges interpret the same data format.
    *    You can place these gauges anywhere on a panel, as long as the server gauge is guaranteed to load before or at the same time as the client gauge.
    **/
    STDMETHOD_(void,    RegisterVarByName) (PVOID var, VAR_TYPE var_type, LPSTR name)                               PURE;
    /**
    *   Initializes a token variable.
    *   @param[in]    module_var   Specifies a pointer to a MODULE_VAR structure, containing the  token variable to initialize.  
    *   @remarks   Before a token variable can be used, you must initialize it with a call to this function.
    **/
    STDMETHOD_(void,    InitializeVar) (PMODULE_VAR module_var)                                                     PURE;
    /**
    *   Initializes a MODULE_VAR structure, given the name of a token variable.
    *   @param[in]      module_var   Specifies the address of the MODULE_VAR structure that will receive information about the variable.
    *   @param[in]      name  Specifies the name of the variable (the same name used in register_var_by_name).
    **/
    STDMETHOD_(void,    InitializeVarByName) (PMODULE_VAR module_var, LPSTR name)                                   PURE;
    /**
    *   Updates the contents of a token variable.  
    *   @param[in]     module_var   Pointer to a MODULE_VAR structure, containing the token variable to update. 
    *   @return    Before using the contents of a module variable, you must call the LookupVar function. 
    **/
    STDMETHOD_(void,    LookupVar) (PMODULE_VAR module_var)                                                         PURE;
    /**
    *   Unregisters a named variable from another gauge, and frees up the memory used. 
    *   @param[in]      name   Specifies the name of the variable.
    **/
    STDMETHOD_(void,    UnregisterVarByName) (LPSTR name)                                                           PURE;
    /**
    *   Unregisters all token variables, and frees up the memory used. 
    *   @param      none
    **/
    STDMETHOD_(void,    UnregisterAllNamedVars) (void)                                                              PURE;
    /**
    *   Closes the specified panel window. 
    *   @param[in]     panel_id  Specifies the identification number of the window to close. The identification number is specified in the Panel.cfg file in the [WindowXX] section by the variable ident. 
    *   @return        If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    **/
    STDMETHOD_(BOOL,    PanelWindowCloseIdent) (UINT32 panel_id)                                                    PURE;
    /**
    *   Displays the specified panel window. 
    *   @param[in]      panel_id  Specifies the identification number of the window to open. The identification number is specified in the Panel.cfg file in the [WindowXX] section by the variable ident.
    *   @return         If the function succeeds, it returns a non-zero value. If it fails, it returns zero.   
    **/
    STDMETHOD_(BOOL,    PanelWindowOpenIdent) (UINT32 panel_id)                                                     PURE;
    /**
    *   Changes the global HUD color to the next in the list. 
    *   @param      none
    *   @remarks    This function cycles through the range of HUD colors (green, dark green, blue, dark blue, red, dark red, black, white), setting the global HUD color to the next in the list..
    **/
    STDMETHOD_(void,    PanelWindowToggleHudColor) (void)                                                           PURE;
    /**
    *   Toggles the HUD units between metric and imperial. 
    *   @param      none
    **/
    STDMETHOD_(void,    PanelWindowToggleHudUnits) (void)                                                           PURE;
    /**
    *   Displays the radio stack 
    *   @param      none
    *   @remarks     This function has no effect if the radio stack is already displayed.
    **/
    STDMETHOD_(void,    RadioStackPopup) (void)                                                                     PURE;
    /**
    *   Closes the radio stack if it was opened with RadioStackPopup. 
    *   @param      none
    **/
    STDMETHOD_(void,    RadioStackAutoclose) (void)                                                                 PURE;
    /**
    *   Retrieves the ID number of a named local variable, if it exists. 
    *   @param[in]      name  Specifies the variable name.
    *   @return     The function returns an ID number if the variables exists, or -1 if it does not. 
    *   @remarks    Local variable names are case-insensitive.
    **/
    STDMETHOD_(ID,      CheckNamedVariable) (LPCSTR name)                                                           PURE;
    /**
    *   Registers a local variable name. 
    *   @param[in]      name  Specifies the variable name. 
    *   @return         The function returns an ID. If the named variable already exists, its existing ID will be returned. If it does not exist, a new registered variable is created. 
    *   @remarks        Local variable names are case-insensitive. The value of the variable is set to zero, and the units to UNITS_UNKNOWN, on creation. 
    **/
    STDMETHOD_(ID,      RegisterNamedVariable) (LPCSTR name)                                                        PURE;
    /**
    *   Retrieves the value of a local variable, given an ID.
    *   @param[in]     id  Specifies the ID of the variable. 
    *   @return     The function returns the value in a FLOAT64. Zero is returned if the variable ID is not found. 
    **/
    STDMETHOD_(FLOAT64, GetNamedVariableValue) (ID id)                                                              PURE;
    /**
    *   Retrieves the value of a named local variable, in the specified units. 
    *   @param[in]      id  Specifies the ID of the variable.
    *   @param[in]      units  Specifies the enum value of the units required. Use get_units_enum to retrieve the enum value from a string.
    *   @return     The function returns the value in a FLOAT64. Zero is returned if the variable ID is not found. 
    **/
    STDMETHOD_(FLOAT64, GetNamedVariableTypedValue) (ID id, ENUM units)                                             PURE;
    /**
    *   Sets a local variable to a given value. 
    *   @param[in]      id  Specifies the ID of the variable.
    *   @param[in]      value  Specifies the value the variable should be set to.
    *   @return     
    **/
    STDMETHOD_(void,    SetNamedVariableValue) (ID id, FLOAT64 value)                                               PURE;
    /**
    *   Specifies a local variable should be set to the given value with the given units.
    *   @param[in]      id  Specifies the ID of the variable.
    *   @param[in]      value  Specifies the value the variable should be set to. 
    *   @param[in]      units  Specifies the units of the value. 
    **/
    STDMETHOD_(void,    SetNamedVariableTypedValue) (ID id, FLOAT64 value, ENUM units)                              PURE;
    /**
    *   Retrieves the name of a local variable, given an ID number.
    *   @param[in]      id  Specifies the ID of the variable. 
    *   @return     The function returns the name in a PCSTRINGZ, or NULL if the name is not found.
    **/
    STDMETHOD_(LPCSTR,  GetNameOfNamedVariable) (ID id)                                                             PURE;
    /**
    *   Retrieves the resource string, given an ID.
    *   @param[in]      id  Specifies the resource ID.
    *   @return         The function returns the resource string in a PCSTRINGZ , or NULL if the ID is not found. 
    **/
    STDMETHOD_(LPCTSTR, PanelResourceStringGet) (ID32 id)                                                           PURE;
    /**
    *   Selects a menu item given an ID. 
    *   @param[in]      menu_id  Specifies the menu ID 
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    *   @remarks    This function does not "toggle", but rather "selects", but the name is left unchanged for backwards compatibility.
    **/
    STDMETHOD_(BOOL,    PanelWindowToggleMenuId) (ID32 menu_id)                                                     PURE;
    /**
    *   Selects the text color for string elements. 
    *   @param[in]  element   A pointer to the element header.
    *   @param[in]  override  Specifies if a color selected by the user should be overridden.
    *   @param[in]  color     RGB value for the color.
    **/
    STDMETHOD_(void,    ElementUseColor) (PELEMENT_HEADER element, BOOL override, UINT32 color)                     PURE;
    /**
    *   Used to specify the flags for a gauge.
    *   @param[in]  name      Specifies the name of the gauge.
    *   @param[in]  newflags  One or more of the following flags:
    *   @remarks  Flag Values:
    *
    *             GAUGE_FLAG_NORMAL         0
    *             GAUGE_FLAG_HIDDEN         0x1
    *             GAUGE_FLAG_BLINKING       0x2
    *             GAUGE_FLAG_GRAYED         0x4
    *             GAUGE_FLAG_HILIGHTED      0x8
    **/
    STDMETHOD_(void,    SetGaugeFlags) (LPCSTR name, FLAGS32 newflags)                                              PURE;
    /**
    *   Used to retrieve the flags set on a gauge.
    *   @param[in] name  Specifies the name of the gauge. 
    *   @return     The function returns a FLAGS32 value containing the flags that are set. 
    *   @remarks  Flag Values:
    *
    *             GAUGE_FLAG_NORMAL         0
    *             GAUGE_FLAG_HIDDEN         0x1
    *             GAUGE_FLAG_BLINKING       0x2
    *             GAUGE_FLAG_GRAYED         0x4
    *             GAUGE_FLAG_HILIGHTED      0x8
    **/
    STDMETHOD_(FLAGS32, GetGaugeFlags) (LPCSTR name)                                                                PURE;
    /**
    *   Compresses a calculator string into a more efficient internal format. 
    *   @param[in]      pCompiled      Pointer to a string, which will contain the compiled string if the function call is successful.
    *   @param[out]     pCompiledSize  Pointer to an integer, which will contain the length of the compiled string if the function call is successful. 
    *   @param[out]     source         Specifies the source calculator string. Refer to the  Creating XML Gauges document for details on format strings.   
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    **/
    STDMETHOD_(BOOL,    GaugeCalculatorCodePrecompile) (LPCSTR* pCompiled, UINT32* pCompiledSize, LPCSTR source)    PURE;
    /**
    *   Evaluates a coded calculator string.
    *   @param[in]  code  Specifies the calculator code.
    *   @param[out] fvalue  Pointer to a float. Returns the result of the calculation, if it is a floating point value.
    *   @param[out] ivalue  Pointer to an integer. Returns the result of the calculation, if it is an integer value.
    *   @param[out] svalue  Pointer to a string. Returns the result of the calculation, if it is a string. 
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    *   @remarks    Example:
    *
    *       FLOAT64 att_pitch = 0;
    *       FLOAT64 att_bank = 0;
    *       ExecuteCalculatorCode ("(A:ATTITUDE INDICATOR PITCH DEGREES:2, degrees)", &att_pitch, NULL, NULL);
    *       ExecuteCalculatorCode ("(A:ATTITUDE INDICATOR BANK DEGREES:2, degrees)",  &att_bank,  NULL, NULL);
    **/
    STDMETHOD_(BOOL,    ExecuteCalculatorCode) (LPCSTR code, FLOAT64* fvalue, SINT32* ivalue, LPCSTR* svalue)       PURE;
    /**
    *   Evaluates a formatted calculator string. 
    *   @param[out]     result      Returns the formatted string.
    *   @param[out]     resultsize  Returns the length of the formatted string.
    *   @param[in]      format      Specifies the calculator string to format.
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    **/
    STDMETHOD_(BOOL,    FormatCalculatorString) (LPSTR result, UINT32 resultsize, LPCSTR format)                    PURE;
    /**
    *   Retrieves the enum value for units, given the string name of the units. 
    *   @param[in]      unitname  Specifies the string name of the units. 
    *   @return         The function returns the ENUM. value for the units, or UNITS_UNKNOWN (-1) if the string name is not found. 
    *   @remarks        Use this function before calling AircraftVarget.
    **/
    STDMETHOD_(ENUM,    GetUnitsEnum) (LPCSTR unitname)                                                             PURE;
    /**
    *   Retrieves the enum value for a simulation variable, given the string name of that variable. 
    *   @param[in]      simvar  Specifies the  simulation variable name. 
    *   @return         The function returns the ENUM value of the simulation variable, or SIMVAR_NONE (-1) if the variable name is not found. 
    *   @remarks        Use this function before calling AircraftVarget.
    **/
    STDMETHOD_(ENUM,    GetAircraftVarEnum) (LPCSTR simvar)                                                         PURE;
    /**
    *   Retrieves the value of an aircraft simulation variable.
    *   @param[in]   simvar  Specifies a  simulation variable enum value. Use get_aircraft_var_enum to retrieve the enum value from a string.
    *   @param[in]   units   Specifies the units enum value the returned value should have. Use get_units_enum to retrieve the enum value from a string. 
    *   @param[in]   index   Specifies an index number, which is required for some engine and communication data. Refer to the  Simulation Variables document for details. 
    *   @return     The function returns the value in a FLOAT64. If the simulation variable is not found, zero will be returned. 
    **/
    STDMETHOD_(FLOAT64, AircraftVarget) (ENUM simvar, ENUM units, SINT32 index)                                     PURE;
    /**
    *   Specifies the registered callback function.
    *   @param[in]      name       Specifies the name of the module, "CABIN" in the Cabin_Comfort.cpp example.
    *   @param[in]      pcallback  Specifies a pointer to the IPanelCCallback function.
    *   @return         If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    **/
    STDMETHOD_(BOOL,    PanelRegisterCCallback) (LPCSTR name, IPanelCCallback* pcallback)                           PURE;
    /**
    *   Retrieves a pointer to the registered callback function.
    *   @param[in]      name  Specifies the name of the module, "CABIN" in the Cabin_Comfort.cpp example.
    *   @return         The function returns a pointer to an IPanelCCallback function. 
    **/
    STDMETHOD_(IPanelCCallback*,    PanelGetRegisteredCCallback) (LPCSTR name)                                      PURE;
    /**
    *   Retrieves a pointer to the aircraft callback function. 
    *   @param[in]      name Specifies the name of the callback function. 
    *   @return         The function returns a pointer to the IAircraftCCallback function, or NULL if the name is not found. 
    **/
    STDMETHOD_(IAircraftCCallback*, PanelGetAircraftCCallback) (LPCSTR name)                                        PURE;
    /**
    *   Transmits a WM_COMMAND application event. 
    *   @param[in]      event_id  Specifies a WM_COMMAND event ID. 
    *   @param[in]      value     Specifies a value to be transmitted along with the event ID. Can be set to zero.
    **/
    STDMETHOD_(void,    SendKeyEvent) (ID32 event_id, UINT32 value)                                                 PURE;
    /**
    *   Registers a key event callback function.
    *   @param[in]      handler  Specifies the handler function, which should match the following definition:
    *
    *                   typedef void (*GAUGE_KEY_EVENT_HANDLER) (ID32 event, UINT32 evdata, PVOID userdata);
    *   @param[in]      userdata Specifies an optional value for use by the gauge developer. This value will be returned to the key event handler function, whenever it is called. 
    **/
    STDMETHOD_(void,    RegisterKeyEventHandler) (GAUGE_KEY_EVENT_HANDLER handler, PVOID userdata)                  PURE;
    /**
    *   Unregisters the key event handler. 
    *   @param[in]      handler  Specifies the handler function. 
    *   @param[in]      userdata Specifies the user data value specified when creating the handler function. 
    **/
    STDMETHOD_(void,    UnregisterKeyEventHandler) (GAUGE_KEY_EVENT_HANDLER handler, PVOID userdata)                PURE;
    /**
    *   Sends data to the other aircraft, in a multiplayer shared cockpit scenario. 
    *   @param[in]  gauge_header  Specifies the gauge header, which is used to identify the gauge that the event applies to.
    *   @param[in]  pBuf          A pointer to an array of data.
    *   @param[in]  nSize         The length of the data array, in bytes.
    *   @return     If the function succeeds, it returns a non-zero value. If it fails, it returns zero.
    *   @remarks    This function is used to send data from one aircraft to another in the shared cockpit scenario of multiplayer operations. 
    *               Only two aircraft can share a cockpit, one is the master and the other is not. This function can be used to send data from either one to the other. 
    **/
    STDMETHOD_(BOOL,    ProcessSharedEventOut)(PGAUGEHDR gauge_header, BYTE* pBuf, UINT32 nSize)                    PURE;
    /**
    *   Returns true if the aircraft is the master aircraft of a shared cockpit. 
    *   @return     If the aircraft is the master aircraft, it returns a non-zero value. If it is not, it returns zero.
    *   @remarks    This function is used in the multiplayer scenario of a shared cockpit. One aircraft is the master and one is not. There is a maximum of two users in this scenario.
    **/
    STDMETHOD_(BOOL,    IsMaster)()                                                                                 PURE;
    /**
    *   Syncs a specified named variable from a shared cockpit.
    *   @param[in]      id  Specifies the ID of the variable.
    *   @param[in]      value Value of variable to sync.     
    **/
    STDMETHOD_(void,    SetNamedVariableValueSync) (ID id, FLOAT64 value)                                           PURE;
    /**
    *   Enables a specified named variable to sync from a shared cockpit.
    *   @param[in]      id  Specifies the ID of the variable.
    *   @param[in]      bSync Sets whether to sync variable or not.
    **/
    STDMETHOD_(void,    SetNamedVariableSyncEnabled) (ID id, BOOL bSync)                                            PURE;
    /**
    *   Returns the description for the given key event id as seen in the Control Settings user interface. 
    *   @param[in]     event_id  The id of the key event. 
    *   @return        The function returns the description of the given key event id. The function returns NULL if the description or key event id is not found.
    **/
    STDMETHOD_(LPCTSTR, GetEventDescription) (ID32 event_id)                                                        PURE;
    /**
    *   Returns the token string for the given key event id. 
    *   @param[in]      event_id  The id of the key event.
    *   @return         The function returns the token string of the given key event id. The function returns NULL if the key event id is not found.
    **/
    STDMETHOD_(LPCTSTR, GetEventTokenString) (ID32 event_id)                                                        PURE;
    /**
    *   Gets an IPdk pointer which can be used to query additional SDK services.
    *   @param[in]      riid  Specifies IPdk interface ID requested. IID_IPdkV01 is the latest version.
    *   @param[out]     ppPdk Specifies the address of the IPdk pointer to be set.
    *   @return         S_OK (0) if no errors and a valid IPdk pointer is set.
    **/
    STDMETHOD_(HRESULT, QueryPdk)(REFIID riid, void** ppPdk)                                                        PURE;
    /**
    *   Returns the number of event tokens. The event count can be used to iteratively call GetEventIdByIndex() to retrieve event id's.
    *   @return        The number of event tokens.
    **/
    STDMETHOD_(UINT32, GetEventCount) ()                                                                            PURE;
    /**
    *   Returns the event id for the associated event index. The event count can be obtained from the GetEventCount() function.
    *   @param[in]     uIndex  The index of the event id being requested.
    *   @return        The event id for the associated index. Valid event id's are greater than zero.
    **/
    STDMETHOD_(ID32, GetEventIdByIndex) (UINT32 uIndex)                                                             PURE;
    
    /**
    *   Opens the specified UI Panel (HTML5 or Scaleform). A panel ID is generated and returned when this is opened. 
    *   The PanelSystem can be used to modify this panel using this ID. Additionally the WindowPluginSystem can be used to modify the top level window. 
    *   The name of this window is generated as the file name without extension.
    *   @param[in]      file_name  The file name of the UI Panel.
    *   @param[in]      x          The x position of the panel.
    *   @param[in]      y          The y position of the panel.
    *   @param[in]      width      The width of the panel.
    *   @param[in]      height     The height of the panel.
    *   @return         Returns the generated panel window ID. This can be used to modify the panel window containing the HTML5 or Scaleform content.
    **/
    STDMETHOD_(UINT32, OpenUIPanel) (LPCTSTR file_name, int x, int y, int width, int height)                        PURE;
};

/// Current version of PanelSystem interface
// {9DB5D29A-92CA-4383-9F3F-BDF502342406}
DEFINE_GUID(IID_IPanelSystemV520,    0x9db5d29a, 0x92ca, 0x4383, 0x9f, 0x3f, 0xbd, 0xf5, 0x2, 0x34, 0x24, 0x6);

/// PanelSystem service ID
DEFINE_GUID(SID_PanelSystem,         0x68254e0a, 0x260e, 0x48ed, 0x88, 0x68, 0xce, 0xef, 0x63, 0xd8, 0x62, 0xc7);

/** \} */
}