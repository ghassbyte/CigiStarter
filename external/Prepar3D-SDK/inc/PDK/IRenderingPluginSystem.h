// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// IRenderingPluginSystem.h
// Public interface for Prepar3D Plugin system
//----------------------------------------------------------------------------

#ifndef RS_IPLUGIN_SYSTEM_H
#define RS_IPLUGIN_SYSTEM_H

#include <windows.h>
#include <Helpers\ListBuilder.h>

//Forward Declarations
struct ID3D11Device;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;
struct ID3D12CommandQueue;
struct ID3D12Resource;
struct ID3D12Device;
struct D3D12_BOX;
enum   DXGI_FORMAT;
enum   D3D12_RESOURCE_STATES;
enum   D3D12_COMMAND_LIST_TYPE;

namespace P3D
{
    /** @defgroup rendering Rendering Services
    *  The rendering services enables external applications to render custom content into a Prepar3D 3D view.
    *  The rendering plugin system allows for custom DirectX® 12 rendering into
    *  textures or overlay onto a view using rendering plug-ins.  The object rendering services provides a
    *  way to add custom objects such as lights to a 3d scene.
    *  @{
    */

    /** This class holds a set of flags that describe how the render data will be used
    *  during the Render() callback. These should be set using SetRenderFlags
    *  during the PreRender() callback.
    *  @remark The IRenderDataV500 provided to the Render() call will only contain resources that were
    *  requested with these flags.
    **/
    struct PdkRenderFlags
    {
        PdkRenderFlags() noexcept : RenderingIsEnabled(true), WillWriteColor(true), WillWriteDepthStencil(false),
            WillReadColor(true), WillReadDepthStencil(false), DeviceSynchronization(true), Output8bpp(false) {}
        /** If false, the Render() function will not be called **/
        bool RenderingIsEnabled : 1;
        /** If false, OutputColor will not be provided **/
        bool WillWriteColor : 1;
        /** If false, OutputDepthStencil will not be provided **/
        bool WillWriteDepthStencil : 1;
        /** If false, InputColor will not be provided **/
        bool WillReadColor : 1;
        /** If false, InputDepthStencil will not be provided **/
        bool WillReadDepthStencil : 1;
        /** Set this to true for any external plugins that need device synchronization **/
        bool DeviceSynchronization : 1;
        /** Set This to ensure an 8888 format **/
        bool Output8bpp : 1;
    };

    enum PdkRenderPass {
        RenderPassPreVc = 0,
        RenderPassPreVcPanels,
        RenderPassDefault,
        RenderPassPreScene,
        RenderPassPrimaryThreadUpdate,
        RenderPassPreTonemapping
    };

    struct IWindowV400;
    struct ICameraSystemV400;
    interface IRenderingPluginServiceV500;
    interface IRenderingPluginV500;
    interface IRenderDataV500;

    struct TextureDescriptionV500
    {
        const WCHAR* szName;
        unsigned int uWidth; 
        unsigned int uHeight;
        DXGI_FORMAT  eFormat;
        bool         bFrameDependent;
        IRenderingPluginV500* pPlugin;
    };

    struct RenderTargetDescriptionV610
    {
        unsigned int    uWidth;
        unsigned int    uHeight;
        DXGI_FORMAT     eFormat;
        unsigned int    uNumMip;
        bool            bMultiSample;
        unsigned int    uArraySize;
        bool            bIsDepthTarget;
    };

    #include "Legacy\IRenderingPluginSystem.legacy.h"

    /**
     * Plugin service used to register custom rendering plugins.  These plugins can be used to render into
     * a texture, or to render on top of an existing view.  If requested, a plugin can also read from the current
     * view output allowing it to implement post process effects that are too complex to be implemented through
     * the xml/hlsl based custom post process system.
     **/
    DECLARE_INTERFACE_(IRenderingPluginSystemV510, IRenderingPluginSystemV500)
    {
        /**
         * Create a new texture given name, size, and a callback function for each update
         * @param   name    Name of texture.  Models and gauges can map to this texture by name.
         * @param   width   Width of texture in pixels
         * @param   height  Height of texture in pixels
         * @param   plugin  plugin that will be used to render into the texture
         * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT CreateTexture(const WCHAR* name, unsigned int width, unsigned int height, IRenderingPluginV500* plugin) = 0;

        /**
         * Remove a texture given the texture name
         * @param   name    Name of plugin
         * @return          HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
         **/
        virtual HRESULT RemoveTexture(const WCHAR* name) = 0;

        /**
         * Get list of textures created externally
         * @param[out]   nameList    list of names of all plugins
         **/
        virtual HRESULT GetCreatedTextures(INameList& nameList) = 0;

        /**
         * Create a new effect with given name and a callback function for each update
         * @param   name    Name of Effect
         * @param   plugin  plugin that will be used to render the effect
         * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
         **/
        virtual HRESULT CreateEffect(const WCHAR* name, IRenderingPluginV500* plugin) = 0;

        /**
         * Remove an effect with given name that was created externally
         * @param   name    Name of Effect
         * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
         **/
        virtual HRESULT RemoveEffect(const WCHAR* name) = 0;

        /**
         * List of effects provided by the system
         * @param[out] names   list of names of effects provided by the system
         * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
         **/
        virtual HRESULT GetSystemEffects(INameList& names) = 0;

        /**
        * Get a Win32 HWND by adapter ID.  This provides a window associated with a
        * specific GPU using the AdapterID provided by IRenderData.
        * @param uAdapterID   Adapter ID for a GPU
        * @param[out] hWindow   Reference to an HWND window handle
        * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
        */
        virtual HRESULT GetDeviceWindow(UINT uAdapterID, HWND& hWindow) = 0;

        /**
        * Create a new texture given name, size, and a callback function for each update
        * @param   textureDesc  descrition of the texture to be created
        * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT CreateTexture(const TextureDescriptionV500& textureDesc) = 0;

        /**
        * Get alternate frame rendering (AFR) group index.  When using SLI, this value indicates which GPU is in use.
        * some rendering plugins may need this information to keep resources synchronized
        * @return  UINT, index of current alternate frame rendering group
        **/
        virtual UINT GetAFRGroup() = 0;

        /**
        * Is multi-project enabled.  This is used for SinglePass VR
        * @return  bool, true if multi-projection is enabled
        **/
        virtual bool IsMultiProjectionEnabled() = 0;

        /**
        * Set multi-project enabled.  Do this to enable Single-Pass for custom VR plugin creation
        * @param  bEnabled, true if multi-projection is enabled
        **/
        virtual void SetMultiProjectionEnabled(bool bEnabled) = 0;


        /**
        * Get minimum object pixel radius.  Objects with a smaller calculated pixel radius will be culled from 3d views.
        * @return  float, radius in pixels of smallest object to draw
        **/
        virtual float GetMinObjectPixelRadius() = 0;
        /**
        * Get texture request distance.  Objects within this distance will request their textures even if they are not visibile in the current view
        * @return  float, texture request distance in meters
        **/
        virtual float GetTextureRequestDistance() = 0;
        /**
        * Get SimObject Force Draw Minimum Pixel Radius.  
        * When forced to draw, objects at a distance will be scaled up visually to maintain the desired on-screen pixel radius.  The software will calculate
        * the scale required to meet the desired pixel and then clamp that value based on SimObjectForceDrawMaxScale.  Scaling will only be applied
        * if the distance to the object is less than or equal to SimObjectForceDrawMaxDistance.
        * @return  float, radius in pixels of smallest object to draw
        **/
        virtual float GetSimObjectForceDrawMinPixelRadius() = 0;
        /**
        * Get force draw max distance.  Objects within this distance will scale to maintain force draw pixel radius size.
        * This draw distance variable sets an optional range at which force draw scaling will be applied.
        * Setting the variable to 0 will disable force drawing.  Setting the to a large value like 100,000 to force Sim objects to always draw.
        * @return  float, force draw max distance in meters
        **/
        virtual float GetSimObjectForceDrawMaxDistance() = 0;
        /**
        * Get force draw max scale.
        * This maximum scale variable sets a max scale factor to be applied when scaling objects up based on SimObjectForceDrawMinPixelRadius.  
        * This allows the force draw feature to better account for simulation objects with variances in size.   It also provides smooth 
        * transition of objects at the edge of their visible range.
        * @return float, force draw max scale value
        **/
        virtual float GetSimObjectForceDrawMaxScale() = 0;

        /**
        * Set minimum object pixel radius.  Objects with a smaller calculated pixel radius will be culled from 3d views.
        * @param  fPixelRadius, radius in pixels of smallest object to draw
        **/
        virtual void  SetMinObjectPixelRadius(float fPixelRadius) = 0;
        /**
        * Set texture request distance.  Objects within this distance will request their textures even if they are not visibile in the current view
        * Setting this overrides the value in the Prepar3D.cfg.
        * @param  fDistance, texture request distance in meters
        **/
        virtual void  SetTextureRequestDistance(float fDistanceMeters) = 0;
        /**
        * Set SimObject Force Draw Minimum Pixel Radius.  
        * When forced to draw, objects at a distance will be scaled up to maintain the desired on-screen pixel radius.  The software will calculate
        * the scale required to meet the desired pixel and then clamp that value based on SimObjectForceDrawMaxScale.  Scaling will only be applied
        * if the distance to the object is less than or equal to SimObjectForceDrawMaxDistance. Setting this overrides the value in the Prepar3D.cfg
        * @param  fPixelRadius, force draw min radius in pixels
        **/
        virtual void  SetSimObjectForceDrawMinPixelRadius(float fPixelRadius) = 0;
        /**
        * Set force draw max distance.  Objects within this distance will scale to maintain force draw pixel radius size. 
        * This draw distance variable sets an optional range at which force draw scaling will be applied.  
        * Setting the variable to 0 will disable force drawing.  Setting the to a large value like 100,000 to force Sim objects to always draw. 
        * Setting this overrides the value in the Prepar3D.cfg
        * @param  fDistance, force draw max distance in meters. 
        **/
        virtual void  SetSimObjectForceDrawMaxDistance(float fDistanceMeters) = 0;
        /**
        * Set force draw max scale. 
        * This maximum scale variable sets a max scale factor to be applied when scaling objects up based on SimObjectForceDrawMinPixelRadius.  
        * This allows the force draw feature to better account for simulation objects with variances in size.   It also provides smooth 
        * transition of objects at the edge of their visible range.  Setting this overrides the value in the Prepar3D.cfg
        * @param  fScale, force draw max scale
        **/
        virtual void  SetSimObjectForceDrawMaxScale(float fScale) = 0;
    };

    /**
     *  Rendering plugin interface used to implement texture and effect plugins.
     *  These plugins can be used to render into a texture, or to render on top
     *   of an existing view.  If requested, a plugin can also read from the current
     *  view output allowing it to implement post process effects that are too complex 
     *  to be implemented through the xml/hlsl based custom post process system.
     **/
    DECLARE_INTERFACE_(IRenderingPluginV500, IUnknown)
    {
        /**
         * A callback to render content
         * @param  pRenderData   Interface to rendering device and resources used for rendering.
         * @remark Render will be called during each rendering pass unless the RenderingIsEnabled
         * flag is set to false in PreRender.
         **/
        virtual void Render(IRenderDataV500* pRenderData) abstract;

        /**
        * Called before calling Render.  This function should call 
        * pRenderData->SetRenderFlags() to let the plugin system know
        * what resources in the render data will be read or written.
        **/
        virtual void PreRender(IRenderDataV500* pRenderData) abstract;
    };

    /**
    *  Render Data Resource interface used to get the D3D12 Resource and track the current state.
    **/
    DECLARE_INTERFACE_(IRenderDataResourceV500, IUnknown)
    {
        /**
        * Get the D3D12 Resource
        * @return  D3D12 Resource
        **/
        virtual ID3D12Resource* GetD3D12Resource() abstract;

        /**
        * Get the resource state
        * @return  current D3D12 Resource State
        **/
        virtual D3D12_RESOURCE_STATES GetResourceState() abstract;

        /**
        * Sets the resource state
        * @param  eState    D3D12 Enumeration for desired resource state
        * @return  shader resource view of texture
        **/
        virtual void SetResourceState(D3D12_RESOURCE_STATES eState) abstract;
    };

    DECLARE_INTERFACE_(IRenderDataV610, IRenderDataV510)
    {
        /**
        * Get Texture width
        * @return  Texture width in pixels
        **/
        virtual FLOAT GetTextureWidth() = 0;
        /**
        * Get Texture height
        * @return  Texture height in pixels
        **/
        virtual FLOAT GetTextureHeight() = 0;

        /**
        * Get Adapter id
        * @return  Current Adapter of RenderData
        **/
        virtual UINT GetAdapterID() = 0;

        /**
        * Get Device
        * @return  D3D12 device.
        **/
        virtual ID3D12Device* GetDevice() abstract;

        /**
        * Get Command Queue
        * @param  eType  D3D12 enumeration for desired command que type
        * @return  D3D12 Command Queue.
        **/
        virtual ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE eType) abstract;

        /**
        * Get Output Color
        * @return  Ouput color render target resource.
        * @remark  Must set WillWriteColor render flag to true in PreRender to use this
        **/
        virtual IRenderDataResourceV500* GetOutputColor() abstract;

        /**
        * Get Output Depth Stencil
        * @return  Ouput depth stencil resource.
        * @remark  Must set WillWriteDepthStencil render flag to true in PreRender to use this
        **/
        virtual IRenderDataResourceV500* GetOutputDepthStencil() abstract;

        /**
        * Get Input color
        * @return  input color resource.
        * @remark  Must set WillReadColor render flag to true in PreRender to use this
        **/
        virtual IRenderDataResourceV500* GetInputColor() abstract;

        /**
        * Get Input Depth Stencil
        * @return  Input depth stencil resource.
        * @remark  Must set WillReadDepthStencil render flag to true in PreRender to use this
        **/
        virtual IRenderDataResourceV500* GetInputDepthStencil() abstract;

        /**
        * Get Input Stencil
        * @return  Input stencil resource.
        * @remark  Will be provided only for RenderPassPreVc.
        * @remark  Stencil available through green channel as uint.
        **/
        virtual IRenderDataResourceV500* GetInputStencil() abstract;

        /**
        * Get texture by name
        * @param  name    name of texture
        * @return  shader resource view of texture
        **/
        virtual IRenderDataResourceV500* GetTexture(const char* name) abstract;

        /**
        * Get PdkRenderFlags
        * @return  PdkRenderFlags
        **/
        virtual PdkRenderFlags GetRenderFlags() abstract;

        /**
        * Set PdkRenderFlags.  This must be called from PreRender() for Render() to work properly.
        * @param  flags  render flags associated with the current window and render pass
        **/
        virtual void SetRenderFlags(PdkRenderFlags flags) abstract;

        /**
        * Get Window
        * @return  Window used for rendering
        **/
        virtual IWindowV400* GetWindow() abstract;

        /**
        * Get Camera
        * @return  Camera used for rendering
        **/
        virtual ICameraSystemV400* GetCamera() abstract;

        /**
        * Get Render Pass
        * @return  Current render pass.
        **/
        virtual PdkRenderPass GetRenderPass() abstract;

        /**
        * Get shading rate image used for variable rate shading
        * @return  Shading rate image resource
        **/
        virtual IRenderDataResourceV500* GetShadingRateImage() abstract;

        /**
        * Get shading rate image tile size for variable rate shading
        * @return  Shading rate image tile size
        **/
        virtual UINT GetShadingRateTileSize() abstract;


        /**
        * Prepar3D's rendering system will create a temporary render target for the current render callback to use
        * This resource lifetime will be managed by Prepar3D so that it is not released while still in flight on the GPU.
        * Any command lists using this resource should execute during the current render callback. 
        * 
        * @param RTTDesc Descriptor with render target settings
        * @return  Shading rate image tile size
        **/
        virtual IRenderDataResourceV500* CreateTemporaryRenderTarget(const RenderTargetDescriptionV610& RTTDesc) abstract;
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs
    interface __declspec(uuid("{11D5AA79-65FA-4AE5-B6FF-CBA1CEE17C57}")) IRenderingPluginSystemV510;
    extern __declspec(selectany) REFIID  IID_IRenderingPluginSystemV510 = __uuidof(IRenderingPluginSystemV510);
    extern __declspec(selectany) REFGUID SID_RenderingPluginSystem = __uuidof(IRenderingPluginSystemV400);

    interface __declspec(uuid("{78038d14-5b5b-4504-8aa4-86ed6bdeed10}")) IRenderingPluginV500;
    extern __declspec(selectany) REFIID  IID_IRenderingPluginV500 = __uuidof(IRenderingPluginV500);

    interface __declspec(uuid("{8973041D-6B02-41D2-B340-D0E13729B1AB}")) IRenderDataV610;
    extern __declspec(selectany) REFIID  IID_IRenderDataV610 = __uuidof(IRenderDataV610);

    interface __declspec(uuid("{925b7fa8-a6df-4869-aa1f-1980d5a9cca3}")) IRenderDataResourceV500;
    extern __declspec(selectany) REFIID  IID_IRenderDataResourceV500 = __uuidof(IRenderDataResourceV500);

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.
    extern __declspec(selectany) REFIID  IID_IRenderingPlugin = IID_IRenderingPluginV500;
    extern __declspec(selectany) REFIID  IID_IRenderData = IID_IRenderDataV610;
    extern __declspec(selectany) REFIID  IID_IRenderDataResource = IID_IRenderDataResourceV500;
    extern __declspec(selectany) REFIID  IID_IRenderingPluginSystem = IID_IRenderingPluginSystemV510;

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.
    typedef IRenderDataV610 IRenderData;
    typedef IRenderDataResourceV500 IRenderDataResource;
    typedef IRenderingPluginV500 IRenderingPlugin;
    typedef IRenderingPluginSystemV510 IRenderingPluginSystem;
    /** @} */ // end of group
}

#endif