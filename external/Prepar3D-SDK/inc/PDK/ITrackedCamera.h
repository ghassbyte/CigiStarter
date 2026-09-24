// Copyright (c) 2010-2019 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

#pragma once
#include <ObjBase.h>
#include "Helpers\P3DMath.h"

namespace P3D
{
    interface IVRInterfaceV451;
    interface ITextureCompositorV500;

    /// Camera Color Format Enumeration
    enum CameraColorFormat
    {
        CameraColorRGBA,
        CameraColorBGRA,
        CameraColorYUY2
    };

    /// Camera Texture Layout Enumeration
    enum CameraTextureLayout
    {
        CameraTextureLayoutStandard,
        CameraTextureLayoutSideBySide
    };

    DECLARE_INTERFACE_(ITrackedCameraV500, IUnknown)
    {
        /**
        * Get the tranform matrix of the mono Tracked Camera
        * @return Tranform Matrix
        */
        virtual P3DMath::float4x4& GetTransformMatrix() abstract;

        /**
        * Set the transform matric of the mono Tracked Camera
        * @param mat New transformation matrix
        */
        virtual void SetTransformMatrix(P3DMath::float4x4& mat) abstract;

        /**
        * Get Texture from device
        * @return DX12 resource representing the texture
        */
        virtual P3D::IRenderDataResourceV500* GetTexture() abstract;

        /**
        * Set texture for mono Tracked Camera
        * @param pTexture Pointer to the new texture
        */
        virtual void SetTexture(P3D::IRenderDataResourceV500 * pTexture) abstract;

		/**
        * Get Texture from device
        @return DX12 resource representing the texture
        */
        virtual ITextureCompositorV500* GetRenderer() abstract;

		/**
        * Set texture for mono Tracked Camera
        * @param pTexture Pointer to the new texture
        */
        virtual void SetRenderer(ITextureCompositorV500* pRenderer) abstract;
    };

	/**
    * Interface for accessing Stereo Camera Information
    */
    DECLARE_INTERFACE_(IStereoCameraV500, IUnknown)
    {

        /**
        * Get the projection matrix of Tracked Left Camera
        * @return Projection Matrix
        */
        virtual P3DMath::float4x4& GetProjectionMatrixLeft() abstract;

        /**
        * Set new projection matrix for Tracked Left Camera
        * @param mat    Matrix to set as new projection matrix
        */
        virtual void SetProjectionMatrixLeft(P3DMath::float4x4& mat) abstract;

        /**
        * Get the projection matrix of Tracked Right Camera
        * @return Projection Matrix
        */
        virtual P3DMath::float4x4& GetProjectionMatrixRight() abstract;

        /**
        * Set new projection matrix for Tracked Right Camera
        * @param mat    Matrix to set as new projection matrix
        */
        virtual void SetProjectionMatrixRight(P3DMath::float4x4& mat) abstract;

        /**
        * Get the color format of Stereo Camera
        * @return Enum representing Color Format
        */
        virtual CameraColorFormat GetColorFormat() abstract;
        
        /**
        * Set the color format of Stereo Camera
        * @param eColorFormat   Enum for the Color Format
        */
        virtual void SetColorFormat(CameraColorFormat eColorFormat) abstract;

        /**
        * Get the Texture Layout of Stereo Camera
        * @return   Enum representing Texture Layout
        */
        virtual CameraTextureLayout GetTextureLayout() abstract;
        
        /**
        * Set the Texture Layout of Stereo Camera
        * @param eLayout    Enum for the Texture Layout
        */
        virtual void SetTextureLayout(CameraTextureLayout eLayout) abstract;

        /**
        * Get whether or not the camera is distorted
        * @return Bool representing if camera is distorted
        */
        virtual bool IsDistorted() abstract;
        
        /**
        * Set whether or not the camera is distorted
        * @param bIsDistorted    Bool to set as new distortion value
        */
        virtual void SetIsDistorted(bool bIsDistorted) abstract;

        /**
        * Get the Stereo Camera name
        * @return   string representing camera name
        */
        virtual PCWSTR GetCameraName() abstract;

		/**
        * Update the Stereo Camera
        * @param pRenderData    Pointer to data containing texture information
        * @param pVr    Pointer to data containing headset information
        */
        virtual void Update(P3D::IRenderDataV500* pRenderData, IVRInterfaceV451* pVr) abstract;

        /**
        * Set if Stereo Camera is inabled
        * @param isEnabled  Bool to set as new enabled value
        */
        virtual void SetIsEnabled(bool isEnabled) abstract;

        /**
        * Get if Stereo Camera is enabled
        * @return   Bool representing if the camera is enabled
        */
        virtual bool IsEnabled() abstract;

        /**
        * Get if Stereo Camera is connected
        * @return   Bool representing if the camera is connected
        */
        virtual bool IsConnected() abstract;
        virtual HRESULT Start() abstract;
        virtual HRESULT Stop() abstract;

        /**
        * Get left "eye" or camera from Stereo Camera
        */
        virtual ITrackedCameraV500& GetLeft() abstract;

		/**
        * Get right "eye" or camera from Stereo Camera
        */
        virtual ITrackedCameraV500& GetRight() abstract;

        virtual bool CaptureTexture(P3DMath::uint4& pixel) abstract;

        /**
        * Add Stereo Camera to UI menu
        @param parentID     ID of caller
        */
        virtual void AddToMenu(unsigned short parentID) abstract;

        /**
        * Remove Stereo Camera from UI menu
        */
        virtual void RemoveFromMenu() abstract;
    };

    //---------------------------------------------------------------------------
    // Predefined UUIDs

    interface __declspec(uuid("{BB53AA29-1A36-44AA-9514-605D4F7A16C4}")) ITrackedCameraV500;
    extern __declspec(selectany) REFIID IID_ITrackedCameraV500 = __uuidof(ITrackedCameraV500);

    interface __declspec(uuid("{89259390-31C2-458D-9BE8-1E98333EA829}")) IStereoCameraV500;
    extern __declspec(selectany) REFIID  IID_IStereoCameraV500 = __uuidof(IStereoCameraV500);
    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID IID_ITrackedCamera = IID_ITrackedCameraV500;
    extern __declspec(selectany) REFIID IID_IStereoCamera = IID_IStereoCameraV500;

    // Ease development through typedefs
    typedef ITrackedCameraV500 ITrackedCamera;
    typedef IStereoCameraV500 IStereoCamera;
}