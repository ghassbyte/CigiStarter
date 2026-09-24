// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// RenderingTypes.h

/** @addtogroup types
* @{
*/

/**
*  Struct that stores world position lat/lon in degrees, and altitude in meters.
*/
#pragma once

struct LLADegreesMeters
{
    double Latitude;
    double Longitude;
    double Altitude;

    LLADegreesMeters() noexcept : Latitude(0.0), Longitude(0.0), Altitude(0.0) {}

    /**
        *  @param   latitude     Latitude in degrees
        *  @param   longitude    longitude in degrees
        *  @param   altitude     altitude in meters
        */
    LLADegreesMeters(double latitude, double longitude, double altitude)
        : Latitude(latitude), Longitude(longitude), Altitude(altitude) {}
};

/**
    *  Struct that stores orientation PBH (pitch, bank, heading) in degrees
    */
struct PBHDegrees
{
    float Pitch;
    float Bank;
    float Heading;

    PBHDegrees() noexcept : Pitch(0.0f), Bank(0.0f), Heading(0.0f) {}
    /**
        *  @param   pitch       pitch in degrees
        *  @param   bank        bank in degrees
        *  @param   heading     heading in degrees
        */
    PBHDegrees(float pitch, float bank, float heading)
        : Pitch(pitch), Bank(bank), Heading(heading) {}
};

struct XYZMeters
{
    float X;
    float Y;
    float Z;

    XYZMeters() noexcept : X(0.0f), Y(0.0f), Z(0.0f) {}
    /**
        *  @param   x     x offset in meters
        *  @param   y     y offset in meters
        *  @param   z     z offset in meters
        */
    XYZMeters(float x, float y, float z) : X(x), Y(y), Z(z) {}

    XYZMeters operator - (const XYZMeters& rhs) const
    {
        return XYZMeters(this->X - rhs.X, this->Y - rhs.Y, this->Z - rhs.Z);
    }

    XYZMeters& operator -= (const XYZMeters& rhs)
    {
        return *this = *this - (rhs);
    }

    XYZMeters operator + (const XYZMeters& rhs) const
    {
        return XYZMeters(this->X + rhs.X, this->Y + rhs.Y, this->Z + rhs.Z);
    }

    XYZMeters& operator += (const XYZMeters& rhs)
    {
        return *this = *this + (rhs);
    }

    XYZMeters operator * (const XYZMeters& rhs) const
    {
        return XYZMeters(this->X * rhs.X, this->Y * rhs.Y, this->Z * rhs.Z);
    }

    XYZMeters& operator *= (const XYZMeters& rhs)
    {
        return *this = *this * (rhs);
    }

    XYZMeters operator * (const float& rhs) const
    {
        return XYZMeters(this->X * rhs, this->Y * rhs, this->Z * rhs);
    }

    XYZMeters& operator *= (const float& rhs)
    {
        return *this = *this * (rhs);
    }

    XYZMeters operator / (const XYZMeters& rhs) const
    {
        return XYZMeters(this->X / rhs.X, this->Y / rhs.Y, this->Z / rhs.Z);
    }

    XYZMeters& operator /= (const XYZMeters& rhs)
    {
        return *this = *this / (rhs);
    }

    XYZMeters operator / (const float& rhs) const
    {
        return XYZMeters(this->X / rhs, this->Y / rhs, this->Z / rhs);
    }

    XYZMeters& operator /= (const float& rhs)
    {
        return *this = *this / (rhs);
    }

    bool operator == (const XYZMeters& src) const
    {
        if (this->X != src.X || this->Y != src.Y || this->Z != src.Z)
        {
            return false;
        }
        return true;
    }

    bool operator != (const XYZMeters& src) const
    {
        return !(*this == src);
    }
};

struct ScreenCoord
{
    float XPixels;
    float YPixels;
    float DistanceMeters;

    ScreenCoord() noexcept : XPixels(0.0f), YPixels(0.0f), DistanceMeters(0.0f) {}
    /**
    *  @param   x     x offset in meters
    *  @param   y     y offset in meters
    *  @param   z     z offset in meters
    */
    ScreenCoord(float x, float y, float dist)
        : XPixels(x), YPixels(y), DistanceMeters(dist) {}
};

/**
    *  Defines a local transformation which includes a position and orientation offset
    */
struct ObjectLocalTransform
{
    XYZMeters XYZ;
    PBHDegrees PBH;

    ObjectLocalTransform() noexcept {}
    /**
        *  @param   x           x offset in meters
        *  @param   y           y offset in meters
        *  @param   z           z offset in meters
        *  @param   pitch       pitch in degrees
        *  @param   bank        bank in degrees
        *  @param   heading     heading in degrees
        */
    ObjectLocalTransform(float x, float y, float z, float pitch, float bank, float heading)
        : XYZ(x, y, z), PBH(pitch, bank, heading) {}
};

/**
    *  Defines a world transformation which includes an LLA for position and PBH for orientation.
    *  Altitude is in meters.  All angle values are in degrees.
    */
struct ObjectWorldTransform
{
    LLADegreesMeters LLA;
    PBHDegrees PBH;

    ObjectWorldTransform() noexcept {}
    /**
        *  @param   latitude     Latitude in degrees
        *  @param   longitude    longitude in degrees
        *  @param   altitude     altitude in meters
        *  @param   pitch       pitch in degrees
        *  @param   bank        bank in degrees
        *  @param   heading     heading in degrees
        */
    ObjectWorldTransform(double latitude, double longitude, double altitude,
        float pitch, float bank, float heading)
        : LLA(latitude, longitude, altitude), PBH(pitch, bank, heading) {}
};

/**
*  Union that stores color in ARGB 8 bit format
*/
union ARGBColor
{
    unsigned int Color;
    struct
    {
        unsigned int Blue : 8;
        unsigned int Green : 8;
        unsigned int Red : 8;
        unsigned int Alpha : 8;
    };

    ARGBColor() noexcept : Color(0) {}

    /**
    *  @param   alpha     Alpha component of the color
    *  @param   red       Red component of the color
    *  @param   green     Green component of the color
    *  @param   blue      Blue component of the color
    */
    ARGBColor(unsigned int alpha, unsigned int red, unsigned int green, unsigned int blue)
        :Alpha(alpha), Red(red), Green(green), Blue(blue) {}
};

/**
*  Union that stores RenderFlags to control drawing
*/
union RenderFlags
{
    unsigned int Flags;
    struct
    {
        bool DrawFromBase : 1; //Draw the object from the center of the base
        bool DrawWithVC : 1; //Draw the object with the virtual cockpit
        bool DepthReadDisable : 1; //Draw on top without reading depth
        bool AlphaWriteEnable : 1; //Enable writing alpha
        bool ActAsStencil : 1; //Use this object as a stencil. If DrawWithVC is used, will draw before VC parts.
        bool AlphaToCoverage : 1; //For textured quads, use texture alpha as a coverage mask.  Useful for icons or hud overlays.
        bool ProjectedImage : 1; // For stencil masked image projection such as aircraft HUDs
        bool ColorWriteDisable : 1; // Disable RGB write.  Use in combination with AlphaWriteEnable for alpha-only AR mask rendering
        bool EnableLighting : 1; // Enables lighting (i.e. primitive draws)
        bool CustomMaterial : 1; // Draw using custom material at the top of the stack
    };

    RenderFlags() noexcept : Flags(0) {}

    RenderFlags(unsigned int flags) : Flags(flags) {}
};

union WorldObjectFlags
{
    unsigned int Flags;
    struct
    {
        bool       bisOnGround : 1; //Enable to ignore the specified altitude and get pulled to the ground.
        bool       bRenderInVirtualCockpit : 1; //Set object to render on top of VC.
        bool       bCanMove : 1;
        bool       bNoCrash : 1;
    };

    WorldObjectFlags() noexcept : Flags(0) {}

    WorldObjectFlags(unsigned int flags) : Flags(flags) {}
};

/// Dynamic light type enum.
enum class DYNAMIC_LIGHT
{
    DYNAMIC_LIGHT_POINT,
    DYNAMIC_LIGHT_SPOT
};

/// Text font type enum.
enum class TEXT_FONT
{
    TEXT_FONT_DEFAULT,
    TEXT_FONT_SMALL,
    TEXT_FONT_SMALL_BOLD,
    TEXT_FONT_MEDIUM_BOLD,
    TEXT_FONT_LARGE_BOLD,
    TEXT_FONT_XLARGE_BOLD,
};

/// Text horizontal alignment enum.
enum class HORIZONTAL_ALIGNMENT
{
    HORIZONTAL_ALIGNMENT_LEFT,
    HORIZONTAL_ALIGNMENT_CENTER,
    HORIZONTAL_ALIGNMENT_RIGHT
};

/// Text vertical alignment enum.
enum class VERTICAL_ALIGNMENT
{
    VERTICAL_ALIGNMENT_TOP,
    VERTICAL_ALIGNMENT_CENTER,
    VERTICAL_ALIGNMENT_BOTTOM
};

/// Material type enum.
enum class MATERIAL_TYPE
{
    MATERIAL_TYPE_GENERAL,
    MATERIAL_TYPE_PBR
};

/// Material property enum.
enum class MATERIAL_PROPERTY
{
    UNKNOWN = -1,

    DIFFUSE_COLOR_RED,
    DIFFUSE_COLOR_GREEN,
    DIFFUSE_COLOR_BLUE,
    DIFFUSE_COLOR_ALPHA,

    SPECULAR_COLOR_RED,
    SPECULAR_COLOR_GREEN,
    SPECULAR_COLOR_BLUE,
    SPECULAR_COLOR_ALPHA,

    DETAIL_COLOR_RED,
    DETAIL_COLOR_GREEN,
    DETAIL_COLOR_BLUE,
    DETAIL_COLOR_ALPHA,

    DIFFUSE_TEXTURE,
    DETAIL_TEXTURE,
    BUMP_TEXTURE,
    SPECULAR_TEXTURE,
    EMISSIVE_TEXTURE,
    FRESNEL_TEXTURE,
    ENVIRONMENT_TEXTURE,

    DIFFUSE_TEXTURE_UV_CHANNEL,
    DETAIL_TEXTURE_UV_CHANNEL,
    BUMP_TEXTURE_UV_CHANNEL,
    SPECULAR_TEXTURE_UV_CHANNEL,
    EMISSIVE_TEXTURE_UV_CHANNEL,

    DETAIL_OFFSET_U,
    DETAIL_OFFSET_V,
    DETAIL_ROTATION,
    DETAIL_SCALE_U,
    DETAIL_SCALE_V,
    DETAIL_BLEND_MODE,
    DETAIL_BLEND_WEIGHT,
    DETAIL_ALPHA_AS_BLEND_MASK,

    MASK_DIFFUSE_BLEND_BY_DETAIL_BLEND_MASK,
    MASK_FINAL_ALPHA_BLEND_BY_DETAIL_BLEND_MASK,

    BUMP_SCALE,
    SPECULAR_MAP_POWER_SCALE,
    ENVIRONMENT_LEVEL_SCALE,

    FINAL_ALPHA_MULTIPLY,

    TEXTURECOORD_OFFSET_U,
    TEXTURECOORD_OFFSET_V,
    TEXTURECOORD_ROTATION,
    TEXTURECOORD_SCALE_U,
    TEXTURECOORD_SCALE_V,

    TEMPERATURE_SCALE,

    DOUBLE_SIDED_LIGHTING,
    LIGHTING_BACKFACE_WEIGHT,
    LIGHTING_FRONTFACE_WEIGHT,

    WRITE_MASK,

    ALBEDO_TEXTURE,
    METALLIC_TEXTURE,
    NORMAL_TEXTURE,
    CLEAR_COAT_TEXTURE,
    PRECIPITATION_TEXTURE,

    ALBEDO_TEXTURE_UV_CHANNEL,
    METALLIC_TEXTURE_UV_CHANNEL,
    NORMAL_TEXTURE_UV_CHANNEL,
    CLEAR_COAT_TEXTURE_UV_CHANNEL,
    PRECIPITATION_TEXTURE_UV_CHANNEL,

    ALBEDO_RED,
    ALBEDO_GREEN,
    ALBEDO_BLUE,
    ALBEDO_ALPHA,

    METALLIC,
    SMOOTHNESS,
    MASKED_THRESHOLD,
    METALLIC_MAP_HAS_OCCLUSION,
    METALLIC_MAP_HAS_REFLECTANCE,
    SMOOTHNESS_SOURCE,
    CLEAR_COAT_CONTAINS_NORMALS,

    NORMAL_SCALE_U,
    NORMAL_SCALE_V,

    EMISSIVE_MODE,
    EMISSIVE_SCALE, // Legacy emissive scale [0,1]
    EMISSIVE_RED,
    EMISSIVE_GREEN,
    EMISSIVE_BLUE,
    EMISSIVE_ALPHA,

    PRECIPITATION_EFFECTS,
    POROUSNESS,

    RENDER_MODE,
    DECAL_ORDER,
    NO_SHADOW_CAST,

    STENCIL_MODE,
    DEPTH_READ,

    SNOW,
    SNOW_ON_VERTICAL_SURFACES_ONLY,
    PUDDLES,

    TWO_SIDED_THIN,
    CAMERA_FACING_NORMALS,
    HAS_PIXEL_MOVEMENT,

    // Add new properties to the end.

    COUNT
};

/**
* Describes a 2D bounding box in pixels.
*/
struct BoundingBox2D
{
    int left;
    int top;
    int right;
    int bottom;

    BoundingBox2D() : left(0), top(0), right(0), bottom(0) { }
};

/**
* Describes how 2D and 3D text should be drawn to the screen.
*/
struct TextDescription
{
    TEXT_FONT Font;                             /**< The font type. */
    HORIZONTAL_ALIGNMENT HorizontalAlignment;   /**< Alignment in the horizontal direction. */
    VERTICAL_ALIGNMENT VerticalAlignment;       /**< Alignment in the vertical direction. */
    BoundingBox2D BoundingBox;                  /**< The size of the text's bounding box. @remarks Negative values are currently not supported. */

    /// Union that supports text description flags.
    union
    {
        struct
        {
            bool WorldSpace : 1;        /**< True if the text box should be draw in world space, false if it should be drawn in screen space. */
            bool DisplayOnTop : 1;      /**< True if text is to be rendered on top of the scene. */
            bool DropShadow : 1;        /**< True if a drop shadow should be drawn with the text. */
            bool CalculateBox : 1;      /**< True if the bounding box should be calculated based on the given string, false if the user-defined bounding box should be used. */
            bool NoPostProcess : 1;     /**< Prevents post-process from being applied to the text. */
        };
        unsigned int Flags;
    };
    
    TextDescription() :
        Font(TEXT_FONT::TEXT_FONT_DEFAULT),
        HorizontalAlignment(HORIZONTAL_ALIGNMENT::HORIZONTAL_ALIGNMENT_LEFT),
        VerticalAlignment(VERTICAL_ALIGNMENT::VERTICAL_ALIGNMENT_TOP),
        Flags(0)
    {
    }
};

/** @} */ // end of group