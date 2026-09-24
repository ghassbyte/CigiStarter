// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// MaterialHelper.h

#include <ObjBase.h>
#include <atlcomcli.h>
#include <IUnknownHelper.h>
#include "IRenderingService.h"

#pragma once

namespace P3D
{
    /*
    * Helper class that wraps the IMaterialVXX interface and provides higher level access to material properties.
    */
    class PBRMaterial
    {
    public:

        enum class RenderMode
        {
            Opaque = 0,
            Masked = 1,
            Translucent = 2,
            Additive = 3,
        };

        enum class EmissiveMode
        {
            Additive = 0,
            AdditiveNightOnly
        };

        enum class SmoothnessSource
        {
            AlbedoAlpha,
            MetallicAlpha
        };

        enum class StencilMode
        {
            Default = 0,
            ProjectionMaskWrite = 1,
            ProjectionMaskRead = 2
        };

        PBRMaterial(IUnknown* pMaterial)
        {
            if (pMaterial)
            {
                pMaterial->QueryInterface(IID_IMaterialVXX, (void**)&m_spMaterial);
            }
        }

        HRESULT SetAlbedo(float red, float green, float blue, float alpha)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_RED, min(max(0.0f, red), 1.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_GREEN, min(max(0.0f, green), 1.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_BLUE, min(max(0.0f, blue), 1.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_ALPHA, min(max(0.0f, alpha), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetAlbedoTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMetallicTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::METALLIC_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetNormalTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::NORMAL_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetEmissiveTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetDetailTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DETAIL_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetClearCoatTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::CLEAR_COAT_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetPrecipitationTexture(LPCWSTR pszTexture)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::PRECIPITATION_TEXTURE, pszTexture)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetAlbedoTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::ALBEDO_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMetallicTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::METALLIC_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetNormalTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::NORMAL_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetEmissiveTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetDetailTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DETAIL_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetClearCoatTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::CLEAR_COAT_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetPrecipitationTextureUVChannel(unsigned int uvChannel)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::PRECIPITATION_TEXTURE_UV_CHANNEL, uvChannel)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMetallic(float fMetallic)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::METALLIC, min(max(0.0f, fMetallic), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetSmoothness(float fSmoothness)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::SMOOTHNESS, min(max(0.0f, fSmoothness), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMaskedThreshold(float fMaskedThreshold)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::MASKED_THRESHOLD, min(max(0.0f, fMaskedThreshold), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMetallicMapHasOcclusion(bool bMetallicMapHasOcclusion)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::METALLIC_MAP_HAS_OCCLUSION, bMetallicMapHasOcclusion)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetMetallicMapHasReflectance(bool bMetallicMapHasReflectance)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::METALLIC_MAP_HAS_REFLECTANCE, bMetallicMapHasReflectance)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetClearCoatContainsNormals(bool bClearCoatContainsNormals)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::CLEAR_COAT_CONTAINS_NORMALS, bClearCoatContainsNormals)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetPrecipitationEffects(bool bPrecipitationEffects)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::PRECIPITATION_EFFECTS, bPrecipitationEffects)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetPorousness(float fPorousness)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::POROUSNESS, min(max(0.0f, fPorousness), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetSnow(float fSnow)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::SNOW, min(max(0.0f, fSnow), 1.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetSnowOnVerticalSurfacesOnly(bool bSnowOnVerticalSurfacesOnly)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::SNOW_ON_VERTICAL_SURFACES_ONLY, bSnowOnVerticalSurfacesOnly)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetPuddles(bool bPuddles)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::PUDDLES, bPuddles)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetEmissiveMode(EmissiveMode eEmissiveMode)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_MODE, static_cast<double>(eEmissiveMode))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetEmissive(float red, float green, float blue, float alpha)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_RED, max(red, 0.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_GREEN, max(green, 0.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_BLUE, max(blue, 0.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::EMISSIVE_ALPHA, max(alpha, 0.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetRenderMode(RenderMode eRenderMode)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::RENDER_MODE, static_cast<double>(eRenderMode))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetSmoothnessSource(SmoothnessSource eSmoothnessSource)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::SMOOTHNESS_SOURCE, static_cast<double>(eSmoothnessSource))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetDecalOrder(int iDecalOrder)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DECAL_ORDER, iDecalOrder)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetNormalScale(float fNormalScaleU, float fNormalScaleV)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::NORMAL_SCALE_U, max(fNormalScaleU, 0.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::NORMAL_SCALE_V, max(fNormalScaleV, 0.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetDetailScale(float fDetailScaleU, float fDetailScaleV)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DETAIL_SCALE_U, max(fDetailScaleU, 0.0f))) &&
                SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DETAIL_SCALE_V, max(fDetailScaleV, 0.0f))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetWriteMask(unsigned int uWriteMask)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::WRITE_MASK, uWriteMask)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetNoShadowCast(bool bNoShadowCast)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::NO_SHADOW_CAST, bNoShadowCast)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetStencilMode(StencilMode eStencilMode)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::STENCIL_MODE, static_cast<double>(eStencilMode))))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetDepthRead(bool bDepthRead)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::DEPTH_READ, bDepthRead)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetTwoSidedThin(bool bTwoSidedThin)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::TWO_SIDED_THIN, bTwoSidedThin)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetCameraFacingNormals(bool bCameraFacingNormals)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::CAMERA_FACING_NORMALS, bCameraFacingNormals)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT SetHasPixelMovement(bool bHasPixelMovement)
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr && SUCCEEDED(m_spMaterial->SetProperty(MATERIAL_PROPERTY::HAS_PIXEL_MOVEMENT, bHasPixelMovement)))
            {
                hr = S_OK;
            }

            return hr;
        }

        HRESULT GetAlbedo(float& red, float& green, float& blue, float& alpha) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                double albedo[4] = { 0.0, 0.0, 0.0, 0.0 };

                if (SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_RED, albedo[0])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_GREEN, albedo[1])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_BLUE, albedo[2])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_ALPHA, albedo[3])))
                {
                    red = static_cast<float>(albedo[0]);
                    green = static_cast<float>(albedo[1]);
                    blue = static_cast<float>(albedo[2]);
                    alpha = static_cast<float>(albedo[3]);
                    hr = S_OK;
                }
            }

            return hr;
        }

        HRESULT GetAlbdeoTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetMetallicTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::METALLIC_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetNormalTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::NORMAL_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetEmissiveTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetDetailTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::DETAIL_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetClearCoatTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::CLEAR_COAT_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        HRESULT GetPrecipitationTexture(LPWSTR pszTexture, unsigned int uLength) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                hr = m_spMaterial->GetProperty(MATERIAL_PROPERTY::PRECIPITATION_TEXTURE, pszTexture, uLength);
            }

            return hr;
        }

        unsigned int GetAlbedoTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::ALBEDO_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetMetallicTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::METALLIC_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetNormalTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::NORMAL_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetEmissiveTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetDetailTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::DETAIL_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetClearCoatTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::CLEAR_COAT_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        unsigned int GetPrecipitationTextureUVChannel() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::PRECIPITATION_TEXTURE_UV_CHANNEL, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        float GetMetallic() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::METALLIC, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetSmoothness() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::SMOOTHNESS, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetMaskedThreshold() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::MASKED_THRESHOLD, dValue);
            }

            return static_cast<float>(dValue);
        }

        bool GetMetallicMapHasOcclusion() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::METALLIC_MAP_HAS_OCCLUSION, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetMetallicMapHasReflectance() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::METALLIC_MAP_HAS_REFLECTANCE, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetPrecipitationEffects() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::PRECIPITATION_EFFECTS, dValue);
            }

            return (dValue != 0.0);
        }

        float GetPorousness() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::POROUSNESS, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetSnow() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::SNOW, dValue);
            }

            return static_cast<float>(dValue);
        }

        bool GetSnowOnVerticalSurfacesOnly() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::SNOW_ON_VERTICAL_SURFACES_ONLY, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetPuddles() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::PUDDLES, dValue);
            }

            return (dValue != 0.0);
        }

        EmissiveMode GetEmissiveMode() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_MODE, dValue);
            }

            return static_cast<EmissiveMode>(dValue);
        }

        HRESULT GetEmissive(float& red, float& green, float& blue, float& alpha) const
        {
            HRESULT hr = E_FAIL;

            if (m_spMaterial != nullptr)
            {
                double emissive[4] = { 0.0, 0.0, 0.0, 0.0 };

                if (SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_RED, emissive[0])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_GREEN, emissive[1])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_BLUE, emissive[2])) &&
                    SUCCEEDED(m_spMaterial->GetProperty(MATERIAL_PROPERTY::EMISSIVE_ALPHA, emissive[3])))
                {
                    red = static_cast<float>(emissive[0]);
                    green = static_cast<float>(emissive[1]);
                    blue = static_cast<float>(emissive[2]);
                    alpha = static_cast<float>(emissive[3]);
                    hr = S_OK;
                }
            }

            return hr;
        }

        RenderMode GetRenderMode() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::RENDER_MODE, dValue);
            }

            return static_cast<RenderMode>(dValue);
        }

        SmoothnessSource GetSmoothnessSource() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::SMOOTHNESS_SOURCE, dValue);
            }

            return static_cast<SmoothnessSource>(dValue);
        }

        int GetDecalOrder() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::DECAL_ORDER, dValue);
            }

            return static_cast<int>(dValue);
        }

        float GetNormalScaleU() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::NORMAL_SCALE_U, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetNormalScaleV() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::NORMAL_SCALE_V, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetDetailScaleU() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::DETAIL_SCALE_U, dValue);
            }

            return static_cast<float>(dValue);
        }

        float GetDetailScaleV() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::DETAIL_SCALE_V, dValue);
            }

            return static_cast<float>(dValue);
        }

        unsigned int GetWriteMask() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::WRITE_MASK, dValue);
            }

            return static_cast<unsigned int>(dValue);
        }

        bool GetNoShadowCast() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::NO_SHADOW_CAST, dValue);
            }

            return (dValue != 0.0);
        }

        StencilMode GetStencilMode() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::STENCIL_MODE, dValue);
            }

            return static_cast<StencilMode>(dValue);
        }

        bool GetDepthRead() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::DEPTH_READ, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetTwoSidedThin() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::TWO_SIDED_THIN, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetCameraFacingNormals() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::CAMERA_FACING_NORMALS, dValue);
            }

            return (dValue != 0.0);
        }

        bool GetHasPixelMovement() const
        {
            double dValue = 0.0;

            if (m_spMaterial != nullptr)
            {
                m_spMaterial->GetProperty(MATERIAL_PROPERTY::HAS_PIXEL_MOVEMENT, dValue);
            }

            return (dValue != 0.0);
        }

    private:

        using IMaterialVXX = IMaterialV600;
        REFGUID IID_IMaterialVXX = __uuidof(IMaterialVXX);

        CComPtr<IMaterialVXX> m_spMaterial;
    };
}
