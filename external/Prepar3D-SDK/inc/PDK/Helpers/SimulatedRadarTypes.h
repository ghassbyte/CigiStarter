// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// SimulatedRadarTypes.h

#pragma once

namespace Radar
{
	//----------------------------------------------------------------------------
	// Static Variables
	//----------------------------------------------------------------------------
	static const char RADAR_CALLBACK_NAME[] = ("P3DRadar");
	static const char RADAR_INTERFACE_NAME[] = ("ISimulatedRadar");

	//Convenience definitions of common structures used within radar system

	typedef double DEGREES;
	typedef double RADIANS;
	typedef double FEET;
	typedef double SECONDS;
	struct LLA
	{
	public:
		LLA() noexcept : Lat(0.0), Lon(0.0), Alt(0.0) {}
		LLA(double lat, double lon, double alt) : Lat(lat), Lon(lon), Alt(alt) {}
		RADIANS Lat;
		RADIANS Lon;
		FEET Alt;
	};

	struct PBH
	{
	public:
		DEGREES Pitch;
		DEGREES Bank;
		DEGREES Heading;
	};
}