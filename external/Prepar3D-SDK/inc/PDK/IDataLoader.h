// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------
// IDataLoader.h
// Prepar3D SDK interface for data  loading helper
//-----------------------------------------------------------------

#pragma once

namespace P3D
{
/** \defgroup   dataload     Data Load Helper
 *  This service is provided through the PDK interface and allows callers to load XML 
 *  data using Prepar3D's own parser. The interface is defined in the file IDataLoader.h. 
 *  Data compiled into a binary XML files, known as .SPB files, can also be loaded with 
 *  through this interface.  A sample implementation using this interface can be found in 
 *  the SimpleAirplane sample in the ISimObject Samples.
 *  @{
 */

    
/**
 *  Callback function type used for loading XML or SPB file data through the IDataLoadHelper interface.
 *  This is the callback type that will receive the float data. The pointer for the ID and string names passed 
 *  into LoadXmlFloatData are passed through to this callback for easy comparison. 
 *  The data load request will be made with a DataSet name and sub-name for an element within the DataSet.  These two names
 *  are passed back through the callback for reference, along with an array pointer and number of floats.  
 *  The void pointer can be used to identify an object or ID from the caller.
 *  The callback is synchronous and the array will be destroyed upon return from the callback.
 */  
typedef HRESULT (STDMETHODCALLTYPE *PFloatLoadCallback) (__in const void* pID, __in const WCHAR* pszDataSetName, __in const WCHAR* pszDataSetSubName, __in const float* rgFloat, __in UINT nFloats);

/**
 * This interface allows requests for specific data in XML (or binary SPB) files. Requests are processed 
 * and returned through a callback provided by the caller for float numeric data and directly to a provided 
 * string pointer for string data. Returned data should be copied, as core Prepar3D will delete its copy 
 * upon return, and pointers to the data will become invalid.
 */
DECLARE_INTERFACE_(IDataLoadHelperV400, IUnknown)
{
        
    /** 
     *  Reads a specified xml/spb, parses a list of floats into an array, and provides the list via callback 
     *  function. The callback is synchronous.  Return from this file occurs upon return from callback
     *  
     *   @param[in]   pvID                 ID passed in to identify the request in the callback. 
     *   @param[in]   pszDataFile          File to read.  If no extension, will try both SPB then XML
     *   @param[in]   pszDataSetName       DataSet name
     *   @param[in]   pszDataSetSubName    Subname of specific element within the DataSet. If NULL, returns all subsets.  
     *   @param[in]   pfCallback           Callback to receive data
     *   @return                           S_OK if succeeded
     *
     *   @remark      A callback is used here because the caller may not know the length of the return array, 
     *                and the caller must allocate and maintain the required memory for the data.
     */  
    STDMETHOD (LoadXmlFloatData)( __in const void* pvID, __in __notnull const WCHAR* pszDataFile, __in __notnull const WCHAR* pszDataSetName, __in const WCHAR* pszDataSetSubName, __in __notnull PFloatLoadCallback pfCallback) const PURE;
    
    /**
     *  Reads a specified xml/spb, and returns a string to a provided char pointer
     *  The callback is synchronous.
     *  
     *   @param[in]   pszDataFile           File to read.  If no extension, will try both SPB then XML
     *   @param[in]   pszDataSetName        DataSet name
     *   @param[in]   pszDataSetSubName     Subname of specific element within the DataSet. If NULL, returns all subsets.  
     *   @param[out]  pszData               Pointer to callers string
     *   @param[in]   uDataSize             Available string length
     *   @return                            S_OK if succeeded
     */
    STDMETHOD (LoadXmlStringData)(__in __notnull const WCHAR* pszDataFile, __in __notnull const WCHAR* pszDataSetName, __in const WCHAR* pszDataSetSubName, __out WCHAR* pszData, __in UINT uDataLength) const PURE;
};

DEFINE_GUID(IID_IDataLoadHelperV400,   0xad3115af, 0x8136, 0x48a9, 0xbd, 0xd1, 0xdc, 0xee, 0xad, 0xa3, 0x5d, 0x8d);
DEFINE_GUID(SID_DataLoadHelper,        0x3f6215c0, 0x65e6, 0x4d91, 0x97, 0x67, 0xb, 0xa7, 0x70, 0x38, 0x52, 0x9a);
/** @} */ // end doc group
}

/** \page  sampleXml        
*  \ingroup   dataload
*
* # Sample XML Data
* _______________________________________________
*
*   Note that if no extension is added to the file path passed into LoadXmlFloatData, Prepar3D
*   will first attempt to load data from a ".SPB", and then fall back to a ".XML".
*   The <FloatArray> data can be implemented as either an attribute or an element.
*
* \code{.xml}      
*       <SimBase.Document Type="AceXML" version="1,0">
*           <Simvar.DataContainer Name = "Engine">
*               <!-- Sample data 1 (single value as an attribute)-->
*               <DataSet Name = "MaxRpm" FloatArray = "101.0"/>
*               <!-- Sample data 2 (table data as an element)-->
*               <DataSet Name = "EngineData">
*                    <FloatArray>
*                        <!-- in, out -->
*                         0.0, 0.2
*                         0.5, 1.0
*                   </FloatArray>
*               </DataSet>
*               <!-- Sample string data -->
*               <DataSet Name = "Engine Name" String = "Left Engine"/>
*           </Simvar.DataContainer>
*       </SimBase.Document>
* \endcode
*/