// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IRenderingService.Legacy.h

#ifndef RENDERINGSERVICE_LEGACY_H
#define RENDERINGSERVICE_LEGACY_H

interface IDynamicLightDataV500;

/**
* Service for rendering into a view
*/
DECLARE_INTERFACE_(IObjectRendererV400, IUnknown)
{

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
};

DECLARE_INTERFACE_(IObjectRendererV440, IObjectRendererV400)
{

    /**
    *  Draw a sphere
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawSphere(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawCylinder(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawLine(const LLADegreesMeters & startLocation,
                             const LLADegreesMeters & endLocation,
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
    virtual HRESULT DrawRectangle(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawTriangle(const ObjectWorldTransform & location,
                                 float width,
                                 float height,
                                 float depth,
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
                               TextDescription & textDescription,
                               RenderFlags renderFlags) abstract;

    /**
    *  Draws text in 3D world space or screen space
    *  @param    location               Text location (lla and pbh).  The text bounding box will be placed at this point.
    *  @param    szText                 Unicode text string to display
    *  @param    textColor              Text color
    *  @param    textDescription        Text description (i.e. font, alignment, additional flags)
    *  @param    renderFlags            Render flags to control drawing
    */
    virtual HRESULT DrawText3D(const ObjectWorldTransform & location,
                               LPCWSTR szText,
                               ARGBColor textColor,
                               TextDescription & textDescription,
                               RenderFlags renderFlags) abstract;

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
    * Begin a light group with a world transform as its origin
    * @param  groupOrigin   All lights in the group will be offset relative to this coordinate transformation
    */
    virtual HRESULT BeginLightGroup(ObjectWorldTransform & groupOrigin) abstract;

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
    virtual void ApplyBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
                                         const ObjectLocalTransform & offsetXyzPbh,
                                         ObjectWorldTransform & llapbhAtOffset) abstract;

    /**
    * Calculate body relative offset between two world transforms
    *  @param    llapbhAtOrigin         World transformation of the origin or base object
    *  @param    llapbhAtOffset         World transformation to use as a reference for calculating the offset
    *  @param    offsetXyzPbh           Body-relative offset needed to base from the base transform to the reference transform
    */
    virtual void CalculateBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
                                             const ObjectWorldTransform & llapbhAtOffset,
                                             ObjectLocalTransform & offsetXyzPbh) abstract;
};



/**
* Service for rendering into a view
*/
DECLARE_INTERFACE_(IObjectRendererV500, IObjectRendererV440)
{

    /**
    *  Draw a sphere
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawSphere(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawCylinder(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawLine(const LLADegreesMeters & startLocation,
        const LLADegreesMeters & endLocation,
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
    virtual HRESULT DrawRectangle(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawTriangle(const ObjectWorldTransform & location,
        float width,
        float height,
        float depth,
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
        TextDescription & textDescription,
        RenderFlags renderFlags) abstract;

    /**
    *  Draws text in 3D world space or screen space
    *  @param    location               Text location (lla and pbh).  The text bounding box will be placed at this point.
    *  @param    szText                 Unicode text string to display
    *  @param    textColor              Text color
    *  @param    textDescription        Text description (i.e. font, alignment, additional flags)
    *  @param    renderFlags            Render flags to control drawing
    */
    virtual HRESULT DrawText3D(const ObjectWorldTransform & location,
        LPCWSTR szText,
        ARGBColor textColor,
        TextDescription & textDescription,
        RenderFlags renderFlags) abstract;

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
    * Begin a light group with a world transform as its origin
    * @param  groupOrigin   All lights in the group will be offset relative to this coordinate transformation
    */
    virtual HRESULT BeginLightGroup(ObjectWorldTransform & groupOrigin) abstract;

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
    virtual void ApplyBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
        const ObjectLocalTransform & offsetXyzPbh,
        ObjectWorldTransform & llapbhAtOffset) abstract;

    /**
    * Calculate body relative offset between two world transforms
    *  @param    llapbhAtOrigin         World transformation of the origin or base object
    *  @param    llapbhAtOffset         World transformation to use as a reference for calculating the offset
    *  @param    offsetXyzPbh           Body-relative offset needed to base from the base transform to the reference transform
    */
    virtual void CalculateBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
        const ObjectWorldTransform & llapbhAtOffset,
        ObjectLocalTransform & offsetXyzPbh) abstract;

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
    virtual HRESULT AddDynamicLight(IDynamicLightDataV500 * pLightData) abstract;
};

/**
* Service for rendering into a view
*/
DECLARE_INTERFACE_(IObjectRendererV520, IObjectRendererV500)
{

    /**
    *  Draw a sphere
    *  @param    location           Location the object will be drawn
    *  @param    radius             Radius of the object in meters
    *  @param    color              Color of the object
    *  @param    RenderFlags        Render flags to control drawing
    */
    virtual HRESULT DrawSphere(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawCylinder(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawLine(const LLADegreesMeters & startLocation,
                             const LLADegreesMeters & endLocation,
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
    virtual HRESULT DrawRectangle(const ObjectWorldTransform & location,
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
    virtual HRESULT DrawTriangle(const ObjectWorldTransform & location,
                                 float width,
                                 float height,
                                 float depth,
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
                               TextDescription & textDescription,
                               RenderFlags renderFlags) abstract;

    /**
    *  Draws text in 3D world space or screen space
    *  @param    location               Text location (lla and pbh).  The text bounding box will be placed at this point.
    *  @param    szText                 Unicode text string to display
    *  @param    textColor              Text color
    *  @param    textDescription        Text description (i.e. font, alignment, additional flags)
    *  @param    renderFlags            Render flags to control drawing
    */
    virtual HRESULT DrawText3D(const ObjectWorldTransform & location,
                               LPCWSTR szText,
                               ARGBColor textColor,
                               TextDescription & textDescription,
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
    virtual HRESULT DrawQuad(const ObjectWorldTransform & location,
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
    * Begin a light group with a world transform as its origin
    * @param  groupOrigin   All lights in the group will be offset relative to this coordinate transformation
    */
    virtual HRESULT BeginLightGroup(ObjectWorldTransform & groupOrigin) abstract;

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
    virtual void ApplyBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
                                         const ObjectLocalTransform & offsetXyzPbh,
                                         ObjectWorldTransform & llapbhAtOffset) abstract;

    /**
    * Calculate body relative offset between two world transforms
    *  @param    llapbhAtOrigin         World transformation of the origin or base object
    *  @param    llapbhAtOffset         World transformation to use as a reference for calculating the offset
    *  @param    offsetXyzPbh           Body-relative offset needed to base from the base transform to the reference transform
    */
    virtual void CalculateBodyRelativeOffset(const ObjectWorldTransform & llapbhAtOrigin,
                                             const ObjectWorldTransform & llapbhAtOffset,
                                             ObjectLocalTransform & offsetXyzPbh) abstract;

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
    virtual HRESULT AddDynamicLight(IDynamicLightDataV500 * pLightData) abstract;
};

/**
* Interface used to store data needed to create a dynamic light
*/
DECLARE_INTERFACE_(IDynamicLightDataV500, IUnknown)
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
    * Sets the intensity of the dynamic light
    * @param    intensity   The intensity in lumens
    */
    virtual void SetIntensity(float intensity) abstract;

    /**
    * Gets the intensity of the dynamic light
    * @return               The intensity in lumens
    */
    virtual float GetIntensity() const abstract;
};

interface __declspec(uuid("{2E014D89-57B1-47F7-A92B-5C65ED171AC4}")) IObjectRendererV520;
extern __declspec(selectany) REFIID  IID_IObjectRendererV520 = __uuidof(IObjectRendererV520);

interface __declspec(uuid("{2836F14E-AFEF-4128-A75E-C7B081457F63}")) IObjectRendererV500;
extern __declspec(selectany) REFIID  IID_IObjectRendererV500 = __uuidof(IObjectRendererV500);

interface __declspec(uuid("{c32762e6-b6c5-4e05-bad7-0d54da537139}")) IObjectRendererV400;
extern __declspec(selectany) REFIID  IID_IObjectRendererV400 = __uuidof(IObjectRendererV400);

interface __declspec(uuid("{553C3E40-8ADF-484C-B996-3CEBEECE4244}")) IObjectRendererV440;
extern __declspec(selectany) REFIID  IID_IObjectRendererV440 = __uuidof(IObjectRendererV440);

interface __declspec(uuid("{767E87DC-7127-4DB5-9518-2FF34EFAE2F4}")) IDynamicLightDataV500;
extern __declspec(selectany) REFIID  IID_IDynamicLightDataV500 = __uuidof(IDynamicLightDataV500);

#endif
