// Copyright (c) 2010-2019 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

#pragma once
#include <ObjBase.h>

namespace P3D
{
    /// Pass Through Camera Mode Options enumeration
    enum PassthroughModes {
        CAMERA,
        PORTALS,
        INVERTED_PORTALS,
    };

	/**
    * Interface for accessing Texture Compositor Rendering Information
    */
    DECLARE_INTERFACE_(ITextureCompositorV500, IUnknown)
    {
		/**
        * Used to initialized Texture Compositor with information from Graphics Card
        @param pDevice  Pointer to the DX12 device
        */	
        virtual HRESULT Init(ID3D12Device* pDevice, ID3D12CommandQueue* pQueue) abstract;
		
		/**
        * Sets viewport, render target, and constant buffer
        * @return   If opperation was sucessful
        * @param pRserouceView  DX11 resource for Pipeline States' resource views
        * @param pOverlaySRV    DX11 resource for Pipeline States' resource views
        */
        virtual HRESULT Composite(P3D::IRenderDataResourceV500 * pResourceView, P3D::IRenderDataResourceV500 * pOverlaySRV) abstract;

		/**
        * Sets viewport, render target, and constant buffer
        * @return   If opperation was sucessful
        * @param pRserouceView  DX11 resource for Pipeline States' resource views
        * @param pOverlaySRV    DX11 resource for Pipeline States' resource views
        * @param pRenderTargetOutput    DX11 Pipeline States' render target
        */
        virtual HRESULT Composite(P3D::IRenderDataResourceV500 * pResourceView, P3D::IRenderDataResourceV500 * pOverlaySRV, 
                                    P3D::IRenderDataResourceV500 * pRenderTargetOutput) abstract;

        /**
        * Set the color to use as the green screen
        * @param keyCodeColor   RGB values representing the color to use as the green screen
        */
        virtual void SetKeyCodeColor(P3DMath::float3 keyCodeColor) abstract;
        
        /**
        * Allows room for error in the RGB values used to represent the green screen color
        * @param keyCodeThreshold   Varience in the RGB values used to represent the green screen color
        */
        virtual void SetKeyCodeThreshold(P3DMath::float2 keyCodeThreshold) abstract;

        /**
        * Set the texture offset
        * @param uvOffset   offset value
        */
        virtual void SetTexutureOffset(P3DMath::float2 uvOffset) abstract;

        /**
        * Set the texture scale
        * @param uvOffset   offset value
        */
        virtual void SetTexutureScale(P3DMath::float2 uvOffset) abstract;


        /**
        * Sets if Swizzle or not
        * @param fSwizzle   0 if no swizzle, any other number if swizzle
        */
        virtual void SetSwizzle(float fSwizzle) abstract;

        /**
        * Used in setting the Camera Color format.
        * @param fYUY2  Value to set the camera color format
        */
        virtual void SetYUY2(float fYUY2) abstract;		
        
        /**
        * Set the camera matrix
        * @param mat    The new camera matrix
        */       
        virtual void SetCameraMatrix(P3DMath::float4x4& mat) abstract;

        /**
        * Set the projection matrix
        * @param mat    The new projection matrix
        */
        virtual void SetProjectionMatrix(P3DMath::float4x4& mat) abstract;

        /**
        * Set the distortion
        * @param bDistorted     Bool to set if distorted
        */
        virtual void SetIsDistorted(bool bDistorted) abstract;

        /**
        * Get if Texture Compositor is initialized by graphics device
        * @return Bool representing if the Texture Compositor has been initialized by graphics device
        */
        virtual bool GetIsInitialized() abstract;

		/**
        * Apply the camera settings such as Camera Color Format and Distortion
        * @params pCamera   Stereo Camera
        */
        virtual void ApplyCameraSettings(IStereoCameraV500* pCamera, bool bLeft) abstract;

		/**
        * Set the pass through mode of the camera
        * @param nMode  Represents which mode the pass through camera will use. Check enum PassthroughModes
        */
        virtual void SetPassthroughMode(int nMode) abstract;

        /**
        * Set if the green screen is enabled
        * @param nEnable Represents if the green screen is enabled.
        */
        virtual void SetEnableGreenScreen(int nEnable) abstract;

        /**
        * Sets whether or not to draw the scene
        * @param bScene     Boolean to set whether or not to draw the scene
        */
        virtual void SetDrawScene(bool bScene) abstract;
    };

    //---------------------------------------------------------------------------
    // Predefined UUIDs
    interface __declspec(uuid("{A0EA982C-CEDC-487D-A286-7967EA60F108}")) ITextureCompositorV500;
    extern __declspec(selectany) REFIID IID_ITextureCompositorV500 = __uuidof(ITextureCompositorV500);
    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID IID_ITextureCompositor = IID_ITextureCompositorV500;
    // Ease development through typedefs
    typedef ITextureCompositorV500 ITextureCompositor;
}