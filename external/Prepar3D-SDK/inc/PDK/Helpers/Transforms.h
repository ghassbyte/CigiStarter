#ifndef ITD_MATH_H
#define ITD_MATH_H

#include <RenderingTypes.h>

#include <cmath>

namespace P3DMath
{
   static constexpr double DegreesPerRadian = (180.0 / 3.141592653589793238);
   static constexpr double RadiansPerDegree = (3.141592653589793238 / 180.0);

   static float XYZMetersLength(const XYZMeters& xyzMeters)
   {
       XYZMeters xyzSq = xyzMeters * xyzMeters; 
       return static_cast<float>(std::sqrt(xyzSq.X + xyzSq.Y + xyzSq.Z));
   }

   static PBHDegrees XYZToPBH(const XYZMeters& xyz)
   {
      PBHDegrees result;
      result.Heading = static_cast<float>(std::atan2(xyz.X, xyz.Z) * DegreesPerRadian);
      result.Pitch = static_cast<float>(std::atan2(-xyz.Y, std::sqrt(xyz.X*xyz.X + xyz.Z*xyz.Z)) * DegreesPerRadian);
      result.Bank = 0.0f;

      return result;
   }

   static XYZMeters PBHToXYZ(PBHDegrees pbh)
   {
       XYZMeters xyz;
       float cos_p = static_cast<float>(cos(RadiansPerDegree * pbh.Pitch));
       float sin_p = static_cast<float>(sin(RadiansPerDegree * pbh.Pitch));
       float cos_h = static_cast<float>(cos(RadiansPerDegree * pbh.Heading));
       float sin_h = static_cast<float>(sin(RadiansPerDegree * pbh.Heading));
       
       xyz.X = -sin_h;
       xyz.Y = cos_h * sin_p;
       xyz.Z = cos_h * cos_p;
       return xyz;
   }

   static LLADegreesMeters LinearInterpolate(const LLADegreesMeters& point1, const LLADegreesMeters& point2, double weight)
   {
       double weight2 = max(0.0, min(1.0, weight));
       double weight1 = 1.0 - weight2;
       
       LLADegreesMeters midPoint(
           (point1.Latitude * weight1 + point2.Latitude * weight2),
           (point1.Longitude * weight1 + point2.Longitude * weight2),
           (point1.Altitude * weight1 + point2.Altitude * weight2)
       );

       return midPoint;
   }

   static XYZMeters LinearInterpolate(XYZMeters& point1, const XYZMeters& point2, float weight)
   {
       float weight2 = max(0.0f, min(1.0f, weight));
       float weight1 = 1.0f - weight2;

       XYZMeters midPoint(
           (point1.X * weight1 + point2.X * weight2),
           (point1.Y * weight1 + point2.Y * weight2),
           (point1.Z * weight1 + point2.Z * weight2)
       );

       return midPoint;
   }
}

#endif