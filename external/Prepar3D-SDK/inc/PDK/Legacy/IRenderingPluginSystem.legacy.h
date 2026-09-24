// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// IRenderingPluginSystem.legacy.h
// Description: Public interface for Prepar3D Plugin system
//----------------------------------------------------------------------------

#ifndef RS_IPLUGIN_SYSTEM_LEGACY_H
#define RS_IPLUGIN_SYSTEM_LEGACY_H

interface IRenderingPluginServiceV400;
interface IRenderingPluginV400;
interface IRenderDataV400;
interface IRenderDataResourceV500;

struct IWindowV400;
struct ICameraSystemV400;

struct TextureDescriptionV440
{
    const WCHAR* szName;
    unsigned int uWidth;
    unsigned int uHeight;
    DXGI_FORMAT  eFormat;
    bool         bFrameDependent;
    IRenderingPluginV400* pPlugin;
};

/**
 * Plugin service used to register custom rendering plugins.  These plugins can be used to render into
 * a texture, or to render on top of an existing view.  If requested, a plugin can also read from the current
 * view output allowing it to implement post process effects that are too complex to be implemented through
 * the xml/hlsl based custom post process system.
 **/
DECLARE_INTERFACE_(IRenderingPluginSystemV500, IUnknown)
{
    /**
     * Create a new texture given name, size, and a callback function for each update
     * @param   name    Name of texture.  Models and gauges can map to this texture by name.
     * @param   width   Width of texture in pixels
     * @param   height  Height of texture in pixels
     * @param   plugin  plugin that will be used to render into the texture
     * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
     **/
    virtual HRESULT CreateTexture(const WCHAR * name, unsigned int width, unsigned int height, IRenderingPluginV500 * plugin) = 0;

    /**
     * Remove a texture given the texture name
     * @param   name    Name of plugin
     * @return          HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
     **/
    virtual HRESULT RemoveTexture(const WCHAR * name) = 0;

    /**
     * Get list of textures created externally
     * @param[out]   nameList    list of names of all plugins
     **/
    virtual HRESULT GetCreatedTextures(INameList & nameList) = 0;

    /**
     * Create a new effect with given name and a callback function for each update
     * @param   name    Name of Effect
     * @param   plugin  plugin that will be used to render the effect
     * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
     **/
    virtual HRESULT CreateEffect(const WCHAR * name, IRenderingPluginV500 * plugin) = 0;

    /**
     * Remove an effect with given name that was created externally
     * @param   name    Name of Effect
     * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
     **/
    virtual HRESULT RemoveEffect(const WCHAR * name) = 0;

    /**
     * List of effects provided by the system
     * @param[out] names   list of names of effects provided by the system
     * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
     **/
    virtual HRESULT GetSystemEffects(INameList & names) = 0;

    /**
    * Get a Win32 HWND by adapter ID.  This provides a window associated with a
    * specific GPU using the AdapterID provided by IRenderData.
    * @param uAdapterID   Adapter ID for a GPU
    * @param[out] hWindow   Reference to an HWND window handle
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    */
    virtual HRESULT GetDeviceWindow(UINT uAdapterID, HWND & hWindow) = 0;

    /**
    * Create a new texture given name, size, and a callback function for each update
    * @param   textureDesc  descrition of the texture to be created
    * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
    **/
    virtual HRESULT CreateTexture(const TextureDescriptionV500 & textureDesc) = 0;

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
    * @return  bool, true if multi-projection is enabled
    **/
    virtual void SetMultiProjectionEnabled(bool bEnabled) = 0;
};

DECLARE_INTERFACE_(IRenderingPluginSystemV400, IUnknown)
{
    /**
    * Create a new texture given name, size, and a callback function for each update
    * @param   name    Name of texture.  Models and gauges can map to this texture by name.
    * @param   width   Width of texture in pixels
    * @param   height  Height of texture in pixels
    * @param   plugin  plugin that will be used to render into the texture
    * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
    **/
    virtual HRESULT CreateTexture(const WCHAR * name, unsigned int width, unsigned int height, IRenderingPluginV400 * plugin) = 0;

    /**
    * Remove a texture given the texture name
    * @param   name    Name of plugin
    * @return          HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT RemoveTexture(const WCHAR * name) = 0;

    /**
    * Get list of textures created externally
    * @param[out]   nameList    list of names of all plugins
    **/
    virtual HRESULT GetCreatedTextures(INameList & nameList) = 0;

    /**
    * Create a new effect with given name and a callback function for each update
    * @param   name    Name of Effect
    * @param   plugin  plugin that will be used to render the effect
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT CreateEffect(const WCHAR * name, IRenderingPluginV400 * plugin) = 0;

    /**
    * Remove an effect with given name that was created externally
    * @param   name    Name of Effect
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT RemoveEffect(const WCHAR * name) = 0;

    /**
    * List of effects provided by the system
    * @param[out] names   list of names of effects provided by the system
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT GetSystemEffects(INameList & names) = 0;
};

/**
* Plugin service used to register custom rendering plugins.  These plugins can be used to render into
* a texture, or to render on top of an existing view.  If requested, a plugin can also read from the current
* view output allowing it to implement post process effects that are too complex to be implemented through
* the xml/hlsl based custom post process system.
**/
DECLARE_INTERFACE_(IRenderingPluginSystemV430, IRenderingPluginSystemV400)
{
    /**
    * Create a new texture given name, size, and a callback function for each update
    * @param   name    Name of texture.  Models and gauges can map to this texture by name.
    * @param   width   Width of texture in pixels
    * @param   height  Height of texture in pixels
    * @param   plugin  plugin that will be used to render into the texture
    * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
    **/
    virtual HRESULT CreateTexture(const WCHAR * name, unsigned int width, unsigned int height, IRenderingPluginV400 * plugin) = 0;

    /**
    * Remove a texture given the texture name
    * @param   name    Name of plugin
    * @return          HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT RemoveTexture(const WCHAR * name) = 0;

    /**
    * Get list of textures created externally
    * @param[out]   nameList    list of names of all plugins
    **/
    virtual HRESULT GetCreatedTextures(INameList & nameList) = 0;

    /**
    * Create a new effect with given name and a callback function for each update
    * @param   name    Name of Effect
    * @param   plugin  plugin that will be used to render the effect
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT CreateEffect(const WCHAR * name, IRenderingPluginV400 * plugin) = 0;

    /**
    * Remove an effect with given name that was created externally
    * @param   name    Name of Effect
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT RemoveEffect(const WCHAR * name) = 0;

    /**
    * List of effects provided by the system
    * @param[out] names   list of names of effects provided by the system
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    **/
    virtual HRESULT GetSystemEffects(INameList & names) = 0;

    /**
    * Get a Win32 HWND by adapter ID.  This provides a window associated with a
    * specific GPU using the AdapterID provided by IRenderData.
    * @param uAdapterID   Adapter ID for a GPU
    * @param[out] hWindow   Reference to an HWND window handle
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    */
    virtual HRESULT GetDeviceWindow(UINT uAdapterID, HWND & hWindow) = 0;
};

/**
    * Plugin service used to register custom rendering plugins.  These plugins can be used to render into
    * a texture, or to render on top of an existing view.  If requested, a plugin can also read from the current
    * view output allowing it to implement post process effects that are too complex to be implemented through
    * the xml/hlsl based custom post process system.
    **/
DECLARE_INTERFACE_(IRenderingPluginSystemV440, IRenderingPluginSystemV430)
{
    /**
        * Create a new texture given name, size, and a callback function for each update
        * @param   name    Name of texture.  Models and gauges can map to this texture by name.
        * @param   width   Width of texture in pixels
        * @param   height  Height of texture in pixels
        * @param   plugin  plugin that will be used to render into the texture
        * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
    virtual HRESULT CreateTexture(const WCHAR * name, unsigned int width, unsigned int height, IRenderingPluginV400 * plugin) = 0;

    /**
        * Remove a texture given the texture name
        * @param   name    Name of plugin
        * @return          HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
        **/
    virtual HRESULT RemoveTexture(const WCHAR * name) = 0;

    /**
        * Get list of textures created externally
        * @param[out]   nameList    list of names of all plugins
        **/
    virtual HRESULT GetCreatedTextures(INameList & nameList) = 0;

    /**
        * Create a new effect with given name and a callback function for each update
        * @param   name    Name of Effect
        * @param   plugin  plugin that will be used to render the effect
        * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
        **/
    virtual HRESULT CreateEffect(const WCHAR * name, IRenderingPluginV400 * plugin) = 0;

    /**
        * Remove an effect with given name that was created externally
        * @param   name    Name of Effect
        * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
        **/
    virtual HRESULT RemoveEffect(const WCHAR * name) = 0;

    /**
        * List of effects provided by the system
        * @param[out] names   list of names of effects provided by the system
        * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
        **/
    virtual HRESULT GetSystemEffects(INameList & names) = 0;

    /**
    * Get a Win32 HWND by adapter ID.  This provides a window associated with a
    * specific GPU using the AdapterID provided by IRenderData.
    * @param uAdapterID   Adapter ID for a GPU
    * @param[out] hWindow   Reference to an HWND window handle
    * @return HRESULT, ERR_NONE if function succeeds or ERR_FAIL if it fails
    */
    virtual HRESULT GetDeviceWindow(UINT uAdapterID, HWND & hWindow) = 0;

    /**
    * Create a new texture given name, size, and a callback function for each update
    * @param   textureDesc  descrition of the texture to be created
    * @return          HRESULT, S_OK if function succeeds or E_FAIL if it fails
    **/
    virtual HRESULT CreateTexture(const TextureDescriptionV440 & textureDesc) = 0;
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
    * @return  bool, true if multi-projection is enabled
    **/
    virtual void SetMultiProjectionEnabled(bool bEnabled) = 0;
};

/**
    *  Rendering plugin interface used to implement texture and effect plugins.
    *  These plugins can be used to render into a texture, or to render on top
    *   of an existing view.  If requested, a plugin can also read from the current
    *  view output allowing it to implement post process effects that are too complex
    *  to be implemented through the xml/hlsl based custom post process system.
    **/
DECLARE_INTERFACE_(IRenderingPluginV400, IUnknown)
{
    /**
        * A callback to render content
        * @param  pRenderData   Interface to rendering device and resources used for rendering.
        * @remark Render will be called during each rendering pass unless the RenderingIsEnabled
        * flag is set to false in PreRender.
        **/
    virtual void Render(IRenderDataV400 * pRenderData) abstract;

    /**
    * Called before calling Render.  This function should call
    * pRenderData->SetRenderFlags() to let the plugin system know
    * what resources in the render data will be read or written.
    **/
    virtual void PreRender(IRenderDataV400 * pRenderData) abstract;
};

DECLARE_INTERFACE_(IRenderDataV400, IUnknown)
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
    * @return  D3D11 device.
    **/
    virtual ID3D11Device* GetDevice() abstract;
    /**
    * Get Output Color
    * @return  Ouput color render target view.
    * @remark  Must set WillWriteColor render flag to true in PreRender to use this
    **/
    virtual ID3D11RenderTargetView* GetOutputColor() abstract;
    /**
    * Get Output Depth Stencil
    * @return  Ouput depth stencil view.
    * @remark  Must set WillWriteDepthStencil render flag to true in PreRender to use this
    **/
    virtual ID3D11DepthStencilView* GetOutputDepthStencil() abstract;
    /**
    * Get input color
    * @return  input color shader resource view.
    * @remark  Must set WillReadColor render flag to true in PreRender to use this
    **/
    virtual ID3D11ShaderResourceView* GetInputColor() abstract;
    /**
    * Get Input Depth Stencil
    * @return  Input depth stencil as shader resource view.
    * @remark  Must set WillReadDepthStencil render flag to true in PreRender to use this
    **/
    virtual ID3D11ShaderResourceView* GetInputDepthStencil() abstract;
    /**
    * Get texture by name
    * @param  name    name of texture
    * @return  shader resource view of texture
    **/
    virtual ID3D11ShaderResourceView* GetTexture(const char* name) abstract;
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
    * Get Render Pass
    * @return  Current render pass.
    **/
    virtual PdkRenderPass GetRenderPass() abstract;
};


DECLARE_INTERFACE_(IRenderDataV500, IUnknown)
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
};

DECLARE_INTERFACE_(IRenderDataV510, IRenderDataV500)
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
};

DECLARE_INTERFACE_(IRenderDataV430, IRenderDataV400)
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
    * @return  D3D11 device.
    **/
    virtual ID3D11Device* GetDevice() abstract;

    /**
    * Get Output Color
    * @return  Ouput color render target view.
    * @remark  Must set WillWriteColor render flag to true in PreRender to use this
    **/
    virtual ID3D11RenderTargetView* GetOutputColor() abstract;

    /**
    * Get Output Depth Stencil
    * @return  Ouput depth stencil view.
    * @remark  Must set WillWriteDepthStencil render flag to true in PreRender to use this
    **/
    virtual ID3D11DepthStencilView* GetOutputDepthStencil() abstract;

    /**
    * Get input color
    * @return  input color shader resource view.
    * @remark  Must set WillReadColor render flag to true in PreRender to use this
    **/
    virtual ID3D11ShaderResourceView* GetInputColor() abstract;

    /**
    * Get Input Depth Stencil
    * @return  Input depth stencil as shader resource view.
    * @remark  Must set WillReadDepthStencil render flag to true in PreRender to use this
    **/
    virtual ID3D11ShaderResourceView* GetInputDepthStencil() abstract;

    /**
    * Get Input Stencil
    * @return  Input stencil as shader resource view.
    * @remark  Will be provided only for RenderPassPreVc.
    * @remark  Stencil available through green channel as uint.
    **/
    virtual ID3D11ShaderResourceView* GetInputStencil() abstract;

    /**
    * Get texture by name
    * @param  name    name of texture
    * @return  shader resource view of texture
    **/
    virtual ID3D11ShaderResourceView* GetTexture(const char* name) abstract;

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
    * Get Render Pass
    * @return  Current render pass.
    **/
    virtual PdkRenderPass GetRenderPass() abstract;
};


///---------------------------------------------------------------------------
/// Predefined UUIDs
/// 

interface __declspec(uuid("{8a74481c-f6bc-4a61-8b6e-af6c50bc9c65}")) IRenderingPluginSystemV500;
extern __declspec(selectany) REFIID  IID_IRenderingPluginSystemV500 = __uuidof(IRenderingPluginSystemV500);

interface __declspec(uuid("{03de3b80-f2af-11e1-aff1-0800200c9a66}")) IRenderingPluginSystemV400;
extern __declspec(selectany) REFIID  IID_IRenderingPluginSystemV400 = __uuidof(IRenderingPluginSystemV400);

interface __declspec(uuid("{1336EB04-8030-4C51-A7B2-BD289200049B}")) IRenderingPluginSystemV430;
extern __declspec(selectany) REFIID  IID_IRenderingPluginSystemV430 = __uuidof(IRenderingPluginSystemV430);

interface __declspec(uuid("{0F97F3D0-0B68-487B-9576-D6C851BAFC7F}")) IRenderingPluginSystemV440;
extern __declspec(selectany) REFIID  IID_IRenderingPluginSystemV440 = __uuidof(IRenderingPluginSystemV440);

interface __declspec(uuid("{1f5f97a0-93f5-46ab-ba20-2e64b72825f9}")) IRenderDataV500;
extern __declspec(selectany) REFIID  IID_IRenderDataV500 = __uuidof(IRenderDataV500);

interface __declspec(uuid("{F6620A89-7659-48F2-9D63-B1E932EC7966}")) IRenderDataV510;
extern __declspec(selectany) REFIID  IID_IRenderDataV510 = __uuidof(IRenderDataV510);

interface __declspec(uuid("{af02252d-6a99-498e-86e5-59c736ad3e00}")) IRenderDataV400;
extern __declspec(selectany) REFIID  IID_IRenderDataV400 = __uuidof(IRenderDataV400);

interface __declspec(uuid("{60D1DEEC-0EC3-4333-B423-E4320C8B6DE7}")) IRenderDataV430;
extern __declspec(selectany) REFIID  IID_IRenderDataV430 = __uuidof(IRenderDataV430);

interface __declspec(uuid("{03de3b83-f2af-11e1-aff1-0800200c9a66}")) IRenderingPluginV400;
extern __declspec(selectany) REFIID  IID_IRenderingPluginV400 = __uuidof(IRenderingPluginV400);

#endif