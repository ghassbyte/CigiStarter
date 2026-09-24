// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IRenderingService.h

#ifndef RENDERINGSERVICE_H
#define RENDERINGSERVICE_H

#include <ObjBase.h>
#include "RenderingTypes.h"
#include "Legacy\IRenderingService.Legacy.h"

/** @addtogroup rendering
* @{
*/

interface IMaterialV600;

/**
* Service for rendering into a view
*/
DECLARE_INTERFACE_(IObjectRendererV600, IObjectRendererV520)
{
    /**
    *  Adds a custom material to the top of the stack.
    *  @param   guidMaterial        The guid of the material
    */
    virtual HRESULT PushMaterial(const GUID & guidMaterial) abstract;

    /**
    *  Adds a custom material to the top of the stack.
    *  @param   pMaterial           The material pointer
    */
    virtual HRESULT PushMaterial(IMaterialV600* pMaterial) abstract;

    /**
    *  Removes a custom material to the top of the stack.
    */
    virtual HRESULT PopMaterial() abstract;

    /**
    * Creates a custom material
    * @param    riid            The IID of the interface instance to be created (i.e. IID_IMaterialV600)
    * @param    ppMaterial      Pointer to store the newly created instance
    * @return   Returns S_OK if the requested interface was successfully created, E_FAIL otherwise
    * @remarks  The instance is returned with a reference count of 1 and must be released when no longer needed
    */
    virtual HRESULT CreateMaterial(MATERIAL_TYPE eType, REFIID riid, void** ppMaterial) abstract;

    /**
    *  Draw a sphere
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawSphere(const ObjectWorldTransform& location,
        float radius,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a cylinder
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawCylinder(const ObjectWorldTransform& location,
        float radius,
        float height,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a line represented by a rectangular prism
    *  @param    startLocation      Start location the object will be drawn
    *  @param    endLocation        End location the object will be drawn
    *  @param    width              Width of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawLine(const LLADegreesMeters& startLocation,
        const LLADegreesMeters& endLocation,
        float width,
        float height,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a rectangular prism
    *  @param    location           Location the object will be drawn
    *  @param    width              Width of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    depth              Depth of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawRectangle(const ObjectWorldTransform& location,
        float width,
        float height,
        float depth,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a trianglular prism
    *  @param    location           Location the object will be drawn
    *  @param    width              Width of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    depth              Depth of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawTriangle(const ObjectWorldTransform& location,
        float width,
        float height,
        float depth,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a pyramid
    *  @param    location           Location the object will be drawn
    *  @param    width              Width of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    depth              Depth of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawPyramid(const ObjectWorldTransform& location,
        float width,
        float height,
        float depth,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draw a cone
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawCone(const ObjectWorldTransform& location,
        float radius,
        float height,
        ARGBColor color,
        RenderFlags renderFlags = 0) abstract;

    /**
    *  Draws text in screen space starting from the top-left with positive directions right and down.
    *  @param    x                      Text location in pixels in the x direction
    *  @param    y                      Text location in pixels in the y direction
    *  @param    szText                 Unicode text string to display
    *  @param    textColor              Text color
    *  @param    textDescription        Text description (i.e. font, alignment, additional flags)
    *  @param    renderFlags            Render flags to control drawing
    */
    virtual HRESULT DrawText2D(int x,
                               int y,
                               LPCWSTR szText,
                               ARGBColor textColor,
                               TextDescription& textDescription,
                               RenderFlags renderFlags) abstract;

    /**
    *  Draws text in 3D world space or screen space
    *  @param    location               Text location (lla and pbh).  The text bounding box will be placed at this point.
    *  @param    szText                 Unicode text string to display
    *  @param    textColor              Text color
    *  @param    textDescription        Text description (i.e. font, alignment, additional flags)
    *  @param    renderFlags            Render flags to control drawing
    */
    virtual HRESULT DrawText3D(const ObjectWorldTransform& location,
                               LPCWSTR szText,
                               ARGBColor textColor,
                               TextDescription& textDescription,
                               RenderFlags renderFlags) abstract;

    /**
    *  Draw a 2D quad
    *  @param    location           Location the object will be drawn
    *  @param    width              Width of the object in meters
    *  @param    height             Height of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    *  @param    szTextureName      Unicode text string for name of texture. Pass a null pointer to disable texturing.
    */
    virtual HRESULT DrawQuad(const ObjectWorldTransform& location,
                             float width,
                             float height,
                             ARGBColor color,
                             RenderFlags renderFlags,
                             LPCWSTR szTextureName) abstract;

    /**
        *  Add a light to the group
        *  @param    x                      x offset in meters from light group origin
        *  @param    y                      y offset in meters from light group origin
        *  @param    z                      z offset in meters from light group origin
        *  @param    lightType              Light type
        *  @param    color                  Light color
        *  @param    size                   Size of light
        *  @param    range                  Distance at which light should be visible
        *  @param    bAttenuateByAmbient    Attenuate light based on ambient light in the scene
        */
    virtual HRESULT AddLight(float x,
                             float y,
                             float z,
                             unsigned int lightType,
                             unsigned int color,
                             float size,
                             float range,
                             bool bAttenuateByAmbient) abstract;

    /**
        *  Add a light to the group
        *  @param    x                      x offset in meters from light group origin
        *  @param    y                      y offset in meters from light group origin
        *  @param    z                      z offset in meters from light group origin
        *  @param    pitch                  pitch offset in degrees from light group origin
        *  @param    bank                   bank offset in degrees from light group origin
        *  @param    heading                heading offset in degrees from light group origin
        *  @param    lightType              Light type
        *  @param    color                  Light color
        *  @param    intensityDay           Daytime light intensity in candelas
        *  @param    intensityNight         Nighttime light intensity in candelas
        *  @param    size                   Size of light
        *  @param    range                  Distance at which light should be visible
        *  @param    bAttenuateByAmbient    Attenuate light based on ambient light in the scene
        *  @param    fInnerAngleX           Horizontal inner angle in degrees
        *  @param    fInnerAngleY           Vertical inner angle in degrees
        *  @param    fOuterAngleX           Horizontal outer angle in degrees
        *  @param    fOuterAngleY           Vertical outer angle in degrees
        *  @param    fMinAttenuationX       Minimum vertical attenuation percentage (0.0 = full attenuation, 1.0 = no attenuation)
        *  @param    fMinAttenuationY       Minimum horizontal attenuation percentage (0.0 = full attenuation, 1.0 = no attenuation)
        */
    virtual HRESULT AddLight(float x,
                             float y,
                             float z,
                             float pitch,
                             float bank,
                             float heading,
                             unsigned int lightType,
                             unsigned int color,
                             float intensityDay,
                             float intensityNight,
                             float size,
                             float range,
                             bool bAttenuateByAmbient,
                             float fInnerAngleX,
                             float fInnerAngleY,
                             float fOuterAngleX,
                             float fOuterAngleY,
                             float fMinAttenuationX,
                             float fMinAttenuationY) abstract;

    /**
    * Begin a light group with a world transform as its origin
    * @param  groupOrigin   All lights in the group will be offset relative to this coordinate transformation
    */
    virtual HRESULT BeginLightGroup(ObjectWorldTransform& groupOrigin) abstract;

    /**
    * End a light group.
    * @param  sortGroup  If true, this group of lights will be sorted with other transparent objects
    * in the scene.  Lights placed up on poles or attached to aircraft should be sorted. Lights placed
    * on the ground generally do not need to be sorted.  Unsorted groups are combined into a single
    * draw call for better rendering performance.
    */
    virtual HRESULT EndLightGroup(bool sortGroup) abstract;

    /**
    * Apply body relative local transformation to a world transform
    *  @param    llapbhAtOrigin         World transformation of origin or base object
    *  @param    offsetXyzPbh           Local body relative transformation to apply as an offset
    *  @param    llapbhAtOffset         World transformation rusting from applying the body relative offset
    */
    virtual void ApplyBodyRelativeOffset(const ObjectWorldTransform& llapbhAtOrigin,
        const ObjectLocalTransform& offsetXyzPbh,
        ObjectWorldTransform& llapbhAtOffset) abstract;

    /**
    * Calculate body relative offset between two world transforms
    *  @param    llapbhAtOrigin         World transformation of the origin or base object
    *  @param    llapbhAtOffset         World transformation to use as a reference for calculating the offset
    *  @param    offsetXyzPbh           Body-relative offset needed to base from the base transform to the reference transform
    */
    virtual void CalculateBodyRelativeOffset(const ObjectWorldTransform& llapbhAtOrigin,
        const ObjectWorldTransform& llapbhAtOffset,
        ObjectLocalTransform& offsetXyzPbh) abstract;

    /**
    * Creates an instance of dynamic light data used to add a dynamic light to a view
    * @param    riid            The IID of the interface instance to be created (i.e. IID_IDynamicLightDataV500)
    * @param    ppLightData     Pointer to store the newly created instance
    * @return   Returns S_OK if the requested interface was successfully created, E_FAIL otherwise
    * @remarks  The instance is returned with a reference count of 1 and must be released when no longer needed
    */
    virtual HRESULT CreateDynamicLightData(REFIID riid, void** ppLightData) abstract;

    /**
    * Adds a dynamic light to the current view
    * @param    pLightData      Light data instance created by CreateDynamicLightData
    * @return   Returns S_OK if the light was successfully added to the current view, E_FAIL otherwise
    */
    virtual HRESULT AddDynamicLight(IDynamicLightDataV500* pLightData) abstract;
};

/**
* Interface used to store data needed to create a dynamic light
*/
DECLARE_INTERFACE_(IDynamicLightDataV600, IDynamicLightDataV500)
{
    /**
    * Sets the type of dynamic light
    * @param    type    The type of dynamic light (i.e. point or spot)
    */
    virtual void SetType(DYNAMIC_LIGHT type) abstract;

    /**
    * Gets the type of dynamic light
    * @return   The type of dynamic light (i.e. point or spot)
    */
    virtual DYNAMIC_LIGHT GetType() const abstract;

    /**
    * Sets the world position and orientation of the dynamic light
    * @param    position    The world position and orientation
    */
    virtual void SetPosition(const ObjectWorldTransform & position) abstract;

    /**
    * Gets the world position and orientation of the dynamic light
    * @return   The world position and orientation
    */
    virtual const ObjectWorldTransform& GetPosition() const abstract;

    /**
    * Sets the color of the dynamic light
    * @param    color   The diffuse color
    * @remarks The alpha value here applies to only non-PBR surfaces, a value of 0 will disable light.
    * 128 is equal to an intensity of 1. Lower than 128 is scaled to an intensity of 0. 255 is equal to an intensity of 50.
    */
    virtual void SetColor(ARGBColor color) abstract;

    /**
    * Gets the color of the dynamic light
    * @return   The diffuse color
    * @remarks The alpha value here applies to only non-PBR surfaces, a value of 0 will disable light.
    * 128 is equal to an intensity of 1. Lower than 128 is scaled to an intensity of 0. 255 is equal to an intensity of 50.
    */
    virtual ARGBColor GetColor() const abstract;

    /**
    * Sets the effective range of the dynamic light
    * @param    range   The effective range in meters
    */
    virtual void SetRange(float range) abstract;

    /**
    * Gets the effective range of the dynamic light
    * @return   The effective range in meters
    */
    virtual float GetRange() const abstract;

    /**
    * Sets the outer angle of the dynamic light
    * @param    outerAngle  The outer angle in degrees
    */
    virtual void SetOuterAngle(float outerAngle) abstract;

    /**
    * Gets the outer angle of the dynamic light
    * @return   The outer angle in degrees
    */
    virtual float GetOuterAngle() const abstract;

    /**
    * Sets the inner angle of the dynamic light
    * @param    innerAngle  The inner angle in degrees
    */
    virtual void SetInnerAngle(float innerAngle) abstract;

    /**
    * Gets the inner angle of the dynamic light
    * @return   The inner angle in degrees
    */
    virtual float GetInnerAngle() const abstract;

    /**
    * Sets the falloff exponent of the dynamic light
    * @param    falloffExponent The falloff exponent
    */
    virtual void SetFalloffExponent(float falloffExponent) abstract;

    /**
    * Gets the falloff exponent of the dynamic light
    * @return   The falloff exponent
    */
    virtual float GetFalloffExponent() const abstract;

    /**
    * DEPRECATED - Use day/night versions
    * Sets the intensity of the dynamic light
    * @param    intensity   The intensity in lumens
    */
    virtual void SetIntensity(float intensity) abstract;

    /**
    * DEPRECATED - Use day/night versions
    * Gets the intensity of the dynamic light
    * @return               The intensity in lumens
    */
    virtual float GetIntensity() const abstract;

    /**
    * Sets the daytime intensity of the dynamic light.
    * This value is interpolated with the nighttime value based on time of day.
    * @param    intensity   The intensity in lumens
    */
    virtual void SetDayIntensity(float intensity) abstract;

    /**
    * Gets the daytime intensity of the dynamic light.
    * This value is interpolated with the nighttime value based on time of day.
    * @return               The intensity in lumens
    */
    virtual float GetDayIntensity() const abstract;

    /**
    * Sets the nighttime intensity of the dynamic light.
    * This value is interpolated with the daytime value based on time of day.
    * @param    intensity   The intensity in lumens
    */
    virtual void SetNightIntensity(float intensity) abstract;

    /**
    * Gets the nighttime intensity of the dynamic light.
    * This value is interpolated with the daytime value based on time of day.
    * @return               The intensity in lumens
    */
    virtual float GetNightIntensity() const abstract;

    /**
    * Gets the interpolated intensity of the dynamic light based on the current time of day.
    * This value is interpolated using the daytime and nighttime values based on time of day.
    * @return               The intensity in lumens
    */
    virtual float GetCurrentIntensity() const abstract;
};

/**
* Interface used to set/get material properties
*/
DECLARE_INTERFACE_(IMaterialV600, IUnknown)
{
    /**
    * Gets a material property
    * @param   id               The property id
    * @param   value            The current property value
    * @return                   S_OK if successful, E_FAIL otherwise
    */
    virtual HRESULT GetProperty(MATERIAL_PROPERTY id, double& value) const abstract;

    /**
    * Gets a material property
    * @param   id               The property id
    * @param   value            The current property string value
    * @param   length           The length of the supplied string buffer
    * @return                   S_OK if successful, E_FAIL otherwise
    */
    virtual HRESULT GetProperty(MATERIAL_PROPERTY id, LPWSTR value, unsigned int length) const abstract;

    /**
    * Sets a material property
    * @param   id               The property id
    * @param   value            The new property value
    * @return                   S_OK if successful, E_FAIL otherwise
    */
    virtual HRESULT SetProperty(MATERIAL_PROPERTY id, double value) abstract;

    /**
    * Sets a material property
    * @param   id               The property id
    * @param   value            The new property string value
    * @return                   S_OK if successful, E_FAIL otherwise
    */
    virtual HRESULT SetProperty(MATERIAL_PROPERTY id, LPCWSTR value) abstract;
};

interface __declspec(uuid("{952FE945-4605-47B2-8DCC-95C169EC9C4C}")) IObjectRendererV600;
extern __declspec(selectany) REFIID  IID_IObjectRendererV600 = __uuidof(IObjectRendererV600);

interface __declspec(uuid("{3C8C3689-A85A-4C0A-92F0-77DD65A5F666}")) IDynamicLightDataV600;
extern __declspec(selectany) REFIID  IID_IDynamicLightDataV600 = __uuidof(IDynamicLightDataV600);

interface __declspec(uuid("{EE8456EF-5DE8-42FB-B756-CAFA73B7883A}")) IMaterialV600;
extern __declspec(selectany) REFIID  IID_IMaterialV600 = __uuidof(IMaterialV600);

extern __declspec(selectany) REFIID  SID_ObjectRenderer = IID_IObjectRendererV400;

/** @} */ // end of group

#endif
