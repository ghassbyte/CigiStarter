// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// ISimulatedRadar.Legacy.h

#pragma once

#include <atlcomcli.h>
#include <Unknwn.h>

#include "..\Helpers\SimulatedRadarTypes.h"

namespace Radar
{
    /** \defgroup    radarService      Air-To-Ground Radar Service
    * Prepar3D provides an air-to-ground radar simulation and visualization service for developers.
    * Because every radar system is different, the Prepar3D radar is provided as a highly configurable
    * service which can be controlled via C++ plug-ins and XML gauges. Some examples of controllable
    * parameters are: Range, Sweep Angle, Sweep Rate, Zoom Level, Image, and Data resolution. Some of
    * the advanced capabilities of the Prepar3D radar include accurate radar shadows,
    * far-shore-enhancement, and Doppler-beam-sharpening. Also, we developed the radar service entirely
    * through our SDK to showcase the power and flexibility that 3rd party developers have.
    *
    * ## Radar XML Interface
    * _______________________________________________
    *
    * The quickest and simplest way to get up and running using the Prepar3D radar is to write an XML
    * gauge using the C:P3DRadar variables provided by the P3DRadar IPanelCallback. This panel callback
    * is loaded automatically, so there is no need to add a dll to the DLL.XML. A sample gauge that uses
    * most of the provided functionality is included in the core application gauges folder
    * (gauges\P3DRadar\radar.xml). To add Prepar3D's sample radar gauge to a SimObject, simply open up
    * the SimObject's panel.cfg and add a panel entry that references the P3DRadar:radar gauge:
    *
    *     [Window Titles]
    *     Window00=Radar
    *     [Window00]
    *     size_mm=265,310
    *     Background_color=1,1,1
    *     visible=0
    *     position=8
    *     windowsize_ratio=0.4
    *     gauge00=P3DRadar!Radar, 5, 5, 255, 305
    *
    * To extend the example or write new XML gauges based on this interface, refer to the full list of
    * C:P3DRadar variables below. P3DRadar C Variable Description Units Settable Multiplayer
    *
    * | P3DRadar   C Variable            | Description                                                                                                                                                                                                                                                         | Units  | Settable | Multiplayer |
    * |----------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------|----------|-------------|
    * | ClearRadarImage                  | Set non zero value to trigger a   radar image clear.                                                                                                                                                                                                                | Number | Y        | N           |
    * | ShowRangeRings                   | 1 - Show range rings   0 - Hide range rings                                                                                                                                                                                                                         | Number | Y        | N           |
    * | ShowCursor                       | 1 - Show Cursor 0 - Hide Cursor                                                                                                                                                                                                                                     | Number | Y        | N           |
    * | FarShoreEnhance                  | 1 - Enable Far Shore Enhancement   0 - Disable Far Shore Enhancement                                                                                                                                                                                                | Number | Y        | N           |
    * | VisualZoom                       | Zoom in on radar display.   Display is centered on current cursor position. Values less than 1.0 are not   supported.                                                                                                                                               | Number | Y        | N           |
    * | DataZoom                         | Remap radar sensor coverage to   area surrounding the current cursor position to increase data resolution.   When set to the same value as visual zoom, this works well as an   approximation of Doppler Beam Sharpening. Values less than 1.0 are not   supported. | Number | Y        | N           |
    * | ScanAzimuth                      | Maximum deviation from center of   the radar system sweep. Valid Range is 1.0-60.0.                                                                                                                                                                                 | Number | Y        | N           |
    * | SweepRate                        | Sweep rate in degrees per second                                                                                                                                                                                                                                    | Number | Y        | N           |
    * | RangeMiles                       | Maximum range of radar in   Nautical Miles.                                                                                                                                                                                                                         | Number | Y        | N           |
    * | RenderingEnabled                 | The effectively disables the   radar service. This should be set to 0 when not displaying the radar texture   element with the radar for best performance. 1 - Enable rendering 0 - Disable   rendering                                                             | Number | Y        | N           |
    * | FreezeEnabled                    | 1 - Enable Image and Radar Beam   Freeze 0 - Disable Image and Radar Beam Freeze                                                                                                                                                                                    | Number | Y        | N           |
    * | CursorPositionX                  | Set cursor horizontal cursor   location. The range 0.0-1.0 maps from left to right across the extents of the   un-zoomed radar gauge image. Zoom calculation must be done by developer.                                                                             | Number | Y        | N           |
    * | CursorPositionY                  | Set cursor vertical cursor   location. The range 0.0-1.0 maps from top to bottom down the extents of the   un-zoomed radar gauge image. Zoom calculation must be done by developer.                                                                                 | Number | Y        | N           |
    * | CursorPositionLat                | Latitude of cursor position.   Actual cursor position in radar is relative to display. Getting or setting   this value will trigger conversions in the C++. For best results set Latitude   and Longitude in pairs.                                                 | Number | Y        | N           |
    * | FrontBlindSpotDegrees            | Width of the current front   blindspot in degrees. This is generally only used for Doppler Beam Sharpening   and should otherwise be set to 0.                                                                                                                      | Number | Y        | N           |
    * | SideBlindSpotDegrees             | Width of the current side   blindspot in degrees. This is generally only used for Doppler Beam Sharpening   and should otherwise be set to 0.                                                                                                                       | Number | Y        | N           |
    * | RadarResolutionX                 | Horizontal resolution of radar   data texture.                                                                                                                                                                                                                      | Number | Y        | N           |
    * | RadarResolutionY                 | Vertical resolution of radar   data texture.                                                                                                                                                                                                                        | Number | Y        | N           |
    * | GaugeResolutionX                 | Horizontal resolution of radar   gauge texture in pixels.                                                                                                                                                                                                           | Number | Y        | N           |
    * | GaugeResolutionY                 | Vertical resolution of radar   gauge texture in pixels.                                                                                                                                                                                                             | Number | Y        | N           |
    * | CurrentRadarBeamOffset           | Get the current angular offset   of the radar beam. Read-only value determined by radar simulation.                                                                                                                                                                 | Number | N        | N           |
    * | CurrentRadarScanElevationDegrees | Current elevation of the radar   beam in degrees. Read-only value set by radar system. This value will vary by   altitude, range, and data zoom.                                                                                                                    | Number | N        | N           |
    *
    * ## Radar Panel Callback Example
    * _______________________________________________
    *
    * For more advanced use cases, the Prepar3D radar can be controlled via C++ using the ISimulatedRadar PDK Service.
    * One possible use case for this would be to provide a customized IPanelCallback that extends the capabilities of
    * the P3DRadar callback provided by Prepar3D. The Radar Callback Sample is a great starting point for this use case
    * and can be found here:
    *
    * \<Prepar3D SDK Path\>\\PDK\\RadarPanelCallbackSample
    *
    * The Radar Callback Sample source can be viewed [here](../samples/radar_sample/RadarPanelCallbackCPP.html). The Radar Callback Sample is a mixed mode gauge example
    * similar to the CabinComfort example but it is far more advanced. The PanelCallback in the sample is functionally
    * equivalent to the PanelCallback provided by the core radar service, so it is a perfect starting point for extending
    * the existing XML interface. Controlling the radar from C++ provides more flexibility and higher performance, and
    * can greatly simplify the XML scripting required to create a robust radar gauge. Interfacing via C++ also can allow
    * multiple add-ons to interface with the radar which may be required to simulate more advanced aircraft. Consult the
    * ReadMe file in the example folder for further instructions on how to install and use it.
    * \{
    */

    /**
        Interface for Radar PDK service
    */
    DECLARE_INTERFACE_(ISimulatedRadarV400, IUnknown)
    {
    public:
        /**
            Initialize the radar system with the specified RTT name, and resolution
            @param sRenderToTextureName Name of RTT texture
            @param width Initial width for radar image and gauge
            @param height Initial height for radar image and gauge
        */
        virtual HRESULT Init( const WCHAR* sRenderToTextureName, UINT width, UINT height ) abstract;

        /**
        Deinit the radar system
        */ 
        virtual HRESULT DeInit( void ) abstract;


        //----------------------------------------------------------------------------
        // Radar System Getters
        //----------------------------------------------------------------------------
        /** 
            Returns true if radar system has been initialized.
        */
        virtual bool IsInitialized() const abstract;

        /** 
            Since gauges might outpace framerate, gauge code should ensure that the radar
            System is not in need of an update before re-doing any work.
        */
        virtual bool NeedsUpdate() const abstract;

        /**
            Is range-ring display enabled?
            @return Range-rings enabled
        */
        virtual bool   ShowRangeRings() const abstract;
        
        /**
            Is cursor drawing enabled?
            @return Cursor enabled
        */
        virtual bool   ShowCursor() const abstract;

        /**
            Is the radar image enhancing far shore pixels
            @return Far shore enhancement enabled
        */
        virtual bool FarShoreEnhance() const abstract;

        /**
            Get the current visual zoom level
            @return Visual zoom
        */
        virtual double GetVisualZoom() const abstract;

        /**
            Get the current level of data zoom
            @return Data zoom
        */
        virtual double GetDataZoom() const abstract;

        /** 
            Get the maximum deviation from center of the radar system sweep
            @return Sweep azimuth
        */
        virtual double GetScanAzimuth() const abstract;

        /**
            Get the rate at which the radar system sweeps side to side
            @return Sweep rate in degrees per second
        */
        virtual double GetSweepRate() const abstract;

        /**
            Get the current range setting of the radar system in miles
            @return Radar range setting, in miles
        */
        virtual double GetRangeMiles() const abstract;

        /**
            Is the radar currently rendering to the intermediate and final gauge texture?
            Note: the radar camera is still updated, along with radar variables
            @return Internal gauge rendering enabled
        */
        virtual bool RenderingEnabled() const abstract;

        /**
            Is the radar beam actively sweeping and updating its intermediate texture?
            Note: When in freeze mode, the radar camera and intermediate texture compositing
            is disabled, but the gauge is still rendered
            @return Radar is actively sweeping
        */
        virtual bool FreezeEnabled() const abstract;

        /**
            Get the current x and y coordinates of the cursor
            @param x Out variable for X coordinate of cursor
            @param y Out variable for y coordinate of cursor
        */
        virtual void   GetCursorPositionXY( double& x, double& y ) const abstract;

        /**
            Get the LLA of the current cursor position
            @param lla Out variable for the current LLA of the radar cursor
        */
        virtual void   GetCursorPositionLLA( LLA& lla ) const abstract;

        /**
            Get the width of the current front blind spot in degrees
            This will usually be 0 unless the radar is in a DBS mode
            @return Width of front blind spot in degrees
        */
        virtual double   GetFrontBlindspotDegrees() const abstract;

        /**
            Get the width of the current side blind spots in degrees
            This will usually be 0 unless the radar is in a DBS mode
            This width is measured from the maximum extents of the current azimuth
            @return Side blind spot width in degrees
        */
        virtual double   GetSideBlindspotDegrees() const abstract;

        /** 
            Get the resolution of the radar image
            @param x Width of radar image
            @param y Height of radar image
        */
        virtual void GetRadarResolution( double& x, double& y ) const abstract;

        /** 
            Get the resolution of the radar gauge
            @param x Width of radar gauge
            @param y Height of radar gauge
        */
        virtual void GetGaugeResolution( double& x, double& y ) const abstract;

        // Getters for current information about radar beam

        /**
            Get the current elevation of the radar beam in degrees
            @return Elevation of radar beam
        */
        virtual double GetCurrentRadarScanElevationDegrees() const abstract;

        /**
            Get the current angular offset of the radar beam
            @return Beam offset in degrees
        */
        virtual double GetCurrentRadarBeamOffsetDegrees() const abstract;
        
        //----------------------------------------------------------------------------
        // Radar System Setters
        //----------------------------------------------------------------------------

        /**
            Clear the radar image on update
        */
        virtual void ClearRadarImage() abstract;

        /**
           Set range rings enabled
           @param show Enable range ring display
        */
        virtual void SetShowRangeRings( bool show ) abstract;

        /**
            Set cursor drawing enabled
            @param enabled Enable simple cursor display
        */
        virtual void SetShowCursor( bool enabled ) abstract;

        /**
            Set far shore enhancement enabled
            @param enabled Enable far shore enhancement
        */
        virtual void SetFarShoreEnhancementEnabled( bool enabled ) abstract;

        /**
            Set the visual zoom level
            Increasing visual zoom is equivalent to zooming on a fixed resolution image
            @param visualZoom Visual zoom level
        */
        virtual void SetVisualZoom( double visualZoom ) abstract;

        /**
            Set the data zoom level
            Increasing visual zoom will improve resolution by reducing the area covered
            @param dataZoom Data zoom level
        */
        virtual void SetDataZoom( double dataZoom ) abstract;

        /**
            Set the scan rate of the radar beam
            @param rate New sweep rate
        */
        virtual void SetScanRateDegreesPerSecond( double rate ) abstract;

        /**
            Set the range of the radar system in miles
            @param range New radar rang in miles
        */
        virtual void SetRangeMiles( double range ) abstract;
        
        /**
            Set whether the radar system is updating its intermediate texture and 
            the final gauge image
            @param enabled Radar rendering enabled
        */
        virtual void SetRenderingEnabled( bool enabled ) abstract;

        /**
            Set whether the radar is updating the sweep of the beam and intermediate texture
            @param freeze Radar freeze mode enabled
        */
        virtual void SetFreeze( bool freeze ) abstract;

        /**
            Set the cursor position with local x and y coordinates
            X and Y are 0-1, top-left  to bottom-right of radar display.
            @param dX Local x coordinate
            @param dY Local y coordinate
        */
        virtual void SetCursorPositionXY( double dX, double dY ) abstract;

        /**
            Set the cursor over the specified Lat Lon Alt
            @param lla New latlonalt of cursor, note that this must fall within radar picture
        */
        virtual void SetCursorPositionLLA( const LLA& lla ) abstract;
        
        /**
            Set the resolution of the radar's internal texture.
            @param resolutionX New width for radar image
            @param resolutionY New height for radar image
        */
        virtual void SetRadarImageResolution( double resolutionX, double resolutionY )  abstract;

        /** 
            Set the resolution of the gauge being rendered into.
            If the gauge resolution is set incorrectly the radar image will not
            display properly.
            @param resolutionX New width for gauge image
            @param resolutionY New height for gauge image
        */
        virtual void SetRadarGaugeResolution( double resolutionX, double resolutionY )  abstract;

        /**
            Set the maximum extent of the radar sweep
            @param azimuth New azimuth in degrees
        */
        virtual void SetScanAzimuthDegrees( double azimuth ) abstract;

        /**
            Set the front DBS blind spot width
            @param frontBlindSpot New width of front blind spot in degrees
        */
        virtual void SetFrontBlindSpotDegrees( double frontBlindSpot ) abstract;

        /**
            Set the width of the side DBS blind spots
            @param sideBlindSpots New width of side blind spots
        */
        virtual void SetSideBlindSpotDegrees( double sideBlindSpots ) abstract;

        //Utility functions for converting between LLA and radar image coordinates

        /**
           Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of radar display.
           @param LLA[out] Global LLA position to calculated 
           @param dX  Local x coordinate relative to radar display
           @param dY  Local y coordinate relative to radar display
        */
        virtual void GetLLAFromLocalXY( LLA &LLA, double dX, double dY ) const abstract;

        /**
          Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
          @param LLA[out] Global LLA position to calculated
          @param dX  Local x coordinate relative to un-zoomed radar display
          @param dY  Local y coordinate relative to un-zoomed radar display
        */
        virtual void GetLLAFromGlobalXY( LLA &LLA, double dX, double dY ) const abstract;

        /**
          Get XY position of an LLA relative to zoomed radar display. X and Y are 0-1, top-left to bottom-right of zoomed radar display.
          @param dX[out]  Local x coordinate relative to zoomed radar display
          @param dY[out]  Local y coordinate relative to zoomed radar display
          @param LLA Global LLA position to calculated
        */
        virtual void GetLocalXYCoord( double &dX, double &dY, const LLA& LLA ) const abstract; 

        /**
          Get XY position of an LLA relative to un-zoomed radar display. X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
          @param dX[out]  Local x coordinate relative to un-zoomed radar display
          @param dY[out]  Local y coordinate relative to un-zoomed radar display
          @param LLA Global LLA position to calculated
        */
        virtual void GetGlobalXYCoord( double &dX, double &dY, const LLA& LLA ) const abstract;
    };

	/**
	Interface for Radar PDK service
	*/
	DECLARE_INTERFACE_(ISimulatedRadarV430, ISimulatedRadarV400)
	{
	public:
		/**
		Initialize the radar system with the specified RTT name, and resolution
		@param sRenderToTextureName Name of RTT texture
		@param width Initial width for radar image and gauge
		@param height Initial height for radar image and gauge
		*/
		virtual HRESULT Init(const WCHAR* sRenderToTextureName, UINT width, UINT height) abstract;

		/**
		Deinit the radar system
		*/
		virtual HRESULT DeInit(void) abstract;


		//----------------------------------------------------------------------------
		// Radar System Getters
		//----------------------------------------------------------------------------
		/**
		Returns true if radar system has been initialized.
		*/
		virtual bool IsInitialized() const abstract;

		/**
		Since gauges might outpace framerate, gauge code should ensure that the radar
		System is not in need of an update before re-doing any work.
		*/
		virtual bool NeedsUpdate() const abstract;

		/**
		Is range-ring display enabled?
		@return Range-rings enabled
		*/
		virtual bool   ShowRangeRings() const abstract;

		/**
		Is cursor drawing enabled?
		@return Cursor enabled
		*/
		virtual bool   ShowCursor() const abstract;

		/**
		Is the radar image enhancing far shore pixels
		@return Far shore enhancement enabled
		*/
		virtual bool FarShoreEnhance() const abstract;

		/**
		Get the current visual zoom level
		@return Visual zoom
		*/
		virtual double GetVisualZoom() const abstract;

		/**
		Get the current level of data zoom
		@return Data zoom
		*/
		virtual double GetDataZoom() const abstract;

		/**
		Get the maximum deviation from center of the radar system sweep
		@return Sweep azimuth
		*/
		virtual double GetScanAzimuth() const abstract;

		/**
		Get the rate at which the radar system sweeps side to side
		@return Sweep rate in degrees per second
		*/
		virtual double GetSweepRate() const abstract;

		/**
		Get the current range setting of the radar system in miles
		@return Radar range setting, in miles
		*/
		virtual double GetRangeMiles() const abstract;

		/**
		Is the radar currently rendering to the intermediate and final gauge texture?
		Note: the radar camera is still updated, along with radar variables
		@return Internal gauge rendering enabled
		*/
		virtual bool RenderingEnabled() const abstract;

		/**
		Is the radar beam actively sweeping and updating its intermediate texture?
		Note: When in freeze mode, the radar camera and intermediate texture compositing
		is disabled, but the gauge is still rendered
		@return Radar is actively sweeping
		*/
		virtual bool FreezeEnabled() const abstract;

		/**
		Get the current x and y coordinates of the cursor
		@param x Out variable for X coordinate of cursor
		@param y Out variable for y coordinate of cursor
		*/
		virtual void   GetCursorPositionXY(double& x, double& y) const abstract;

		/**
		Get the LLA of the current cursor position
		@param lla Out variable for the current LLA of the radar cursor
		*/
		virtual void   GetCursorPositionLLA(LLA& lla) const abstract;

		/**
		Get the width of the current front blind spot in degrees
		This will usually be 0 unless the radar is in a DBS mode
		@return Width of front blind spot in degrees
		*/
		virtual double   GetFrontBlindspotDegrees() const abstract;

		/**
		Get the width of the current side blind spots in degrees
		This will usually be 0 unless the radar is in a DBS mode
		This width is measured from the maximum extents of the current azimuth
		@return Side blind spot width in degrees
		*/
		virtual double   GetSideBlindspotDegrees() const abstract;

		/**
		Get the resolution of the radar image
		@param x Width of radar image
		@param y Height of radar image
		*/
		virtual void GetRadarResolution(double& x, double& y) const abstract;

		/**
		Get the resolution of the radar gauge
		@param x Width of radar gauge
		@param y Height of radar gauge
		*/
		virtual void GetGaugeResolution(double& x, double& y) const abstract;

		// Getters for current information about radar beam

		/**
		Get the current elevation of the radar beam in degrees
		@return Elevation of radar beam
		*/
		virtual double GetCurrentRadarScanElevationDegrees() const abstract;

		/**
		Get the current angular offset of the radar beam
		@return Beam offset in degrees
		*/
		virtual double GetCurrentRadarBeamOffsetDegrees() const abstract;

		//----------------------------------------------------------------------------
		// Radar System Setters
		//----------------------------------------------------------------------------

		/**
		Clear the radar image on update
		*/
		virtual void ClearRadarImage() abstract;

		/**
		Set range rings enabled
		@param show Enable range ring display
		*/
		virtual void SetShowRangeRings(bool show) abstract;

		/**
		Set cursor drawing enabled
		@param enabled Enable simple cursor display
		*/
		virtual void SetShowCursor(bool enabled) abstract;

		/**
		Set far shore enhancement enabled
		@param enabled Enable far shore enhancement
		*/
		virtual void SetFarShoreEnhancementEnabled(bool enabled) abstract;

		/**
		Set the visual zoom level
		Increasing visual zoom is equivalent to zooming on a fixed resolution image
		@param visualZoom Visual zoom level
		*/
		virtual void SetVisualZoom(double visualZoom) abstract;

		/**
		Set the data zoom level
		Increasing visual zoom will improve resolution by reducing the area covered
		@param dataZoom Data zoom level
		*/
		virtual void SetDataZoom(double dataZoom) abstract;

		/**
		Set the scan rate of the radar beam
		@param rate New sweep rate
		*/
		virtual void SetScanRateDegreesPerSecond(double rate) abstract;

		/**
		Set the range of the radar system in miles
		@param range New radar rang in miles
		*/
		virtual void SetRangeMiles(double range) abstract;

		/**
		Set whether the radar system is updating its intermediate texture and
		the final gauge image
		@param enabled Radar rendering enabled
		*/
		virtual void SetRenderingEnabled(bool enabled) abstract;

		/**
		Set whether the radar is updating the sweep of the beam and intermediate texture
		@param freeze Radar freeze mode enabled
		*/
		virtual void SetFreeze(bool freeze) abstract;

		/**
		Set the cursor position with local x and y coordinates
		X and Y are 0-1, top-left  to bottom-right of radar display.
		@param dX Local x coordinate
		@param dY Local y coordinate
		*/
		virtual void SetCursorPositionXY(double dX, double dY) abstract;

		/**
		Set the cursor over the specified Lat Lon Alt
		@param lla New latlonalt of cursor, note that this must fall within radar picture
		*/
		virtual void SetCursorPositionLLA(const LLA& lla) abstract;

		/**
		Set the resolution of the radar's internal texture.
		@param resolutionX New width for radar image
		@param resolutionY New height for radar image
		*/
		virtual void SetRadarImageResolution(double resolutionX, double resolutionY)  abstract;

		/**
		Set the resolution of the gauge being rendered into.
		If the gauge resolution is set incorrectly the radar image will not
		display properly.
		@param resolutionX New width for gauge image
		@param resolutionY New height for gauge image
		*/
		virtual void SetRadarGaugeResolution(double resolutionX, double resolutionY)  abstract;

		/**
		Set the maximum extent of the radar sweep
		@param azimuth New azimuth in degrees
		*/
		virtual void SetScanAzimuthDegrees(double azimuth) abstract;

		/**
		Set the front DBS blind spot width
		@param frontBlindSpot New width of front blind spot in degrees
		*/
		virtual void SetFrontBlindSpotDegrees(double frontBlindSpot) abstract;

		/**
		Set the width of the side DBS blind spots
		@param sideBlindSpots New width of side blind spots
		*/
		virtual void SetSideBlindSpotDegrees(double sideBlindSpots) abstract;

		/**
		Set the base color of the radar scan
		@param red New red value 0.0 - 1.0
		@param green New green value 0.0 - 1.0
		@param blue New blue value 0.0 - 1.0
		*/
		virtual void SetRadarScanColor(float red, float green, float blue) abstract;

		//Utility functions for converting between LLA and radar image coordinates

		/**
		Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of radar display.
		@param LLA[out] Global LLA position to calculated
		@param dX  Local x coordinate relative to radar display
		@param dY  Local y coordinate relative to radar display
		*/
		virtual void GetLLAFromLocalXY(LLA &LLA, double dX, double dY) const abstract;

		/**
		Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
		@param LLA[out] Global LLA position to calculated
		@param dX  Local x coordinate relative to un-zoomed radar display
		@param dY  Local y coordinate relative to un-zoomed radar display
		*/
		virtual void GetLLAFromGlobalXY(LLA &LLA, double dX, double dY) const abstract;

		/**
		Get XY position of an LLA relative to zoomed radar display. X and Y are 0-1, top-left to bottom-right of zoomed radar display.
		@param dX[out]  Local x coordinate relative to zoomed radar display
		@param dY[out]  Local y coordinate relative to zoomed radar display
		@param LLA Global LLA position to calculated
		*/
		virtual void GetLocalXYCoord(double &dX, double &dY, const LLA& LLA) const abstract;

		/**
		Get XY position of an LLA relative to un-zoomed radar display. X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
		@param dX[out]  Local x coordinate relative to un-zoomed radar display
		@param dY[out]  Local y coordinate relative to un-zoomed radar display
		@param LLA Global LLA position to calculated
		*/
		virtual void GetGlobalXYCoord(double &dX, double &dY, const LLA& LLA) const abstract;
	};


    DECLARE_INTERFACE_(ISimulatedRadarV440, ISimulatedRadarV430)
    {
    public:
        /**
            Initialize the radar system with the specified RTT name, and resolution
            @param sRenderToTextureName Name of RTT texture
            @param width Initial width for radar image and gauge
            @param height Initial height for radar image and gauge
        */
        virtual HRESULT Init(const WCHAR * sRenderToTextureName, UINT width, UINT height) abstract;

        /**
        Deinit the radar system
        */
        virtual HRESULT DeInit(void) abstract;


        //----------------------------------------------------------------------------
        // Radar System Getters
        //----------------------------------------------------------------------------
        /**
            Returns true if radar system has been initialized.
        */
        virtual bool IsInitialized() const abstract;

        /**
            Since gauges might outpace framerate, gauge code should ensure that the radar
            System is not in need of an update before re-doing any work.
        */
        virtual bool NeedsUpdate() const abstract;

        /**
            Is range-ring display enabled?
            @return Range-rings enabled
        */
        virtual bool   ShowRangeRings() const abstract;

        /**
            Is cursor drawing enabled?
            @return Cursor enabled
        */
        virtual bool   ShowCursor() const abstract;

        /**
            Is the radar image enhancing far shore pixels
            @return Far shore enhancement enabled
        */
        virtual bool FarShoreEnhance() const abstract;

        /**
            Get the current visual zoom level
            @return Visual zoom
        */
        virtual double GetVisualZoom() const abstract;

        /**
            Get the current level of data zoom
            @return Data zoom
        */
        virtual double GetDataZoom() const abstract;

        /**
            Get the maximum deviation from center of the radar system sweep
            @return Sweep azimuth
        */
        virtual double GetScanAzimuth() const abstract;

        /**
            Get the rate at which the radar system sweeps side to side
            @return Sweep rate in degrees per second
        */
        virtual double GetSweepRate() const abstract;

        /**
            Get the current range setting of the radar system in miles
            @return Radar range setting, in miles
        */
        virtual double GetRangeMiles() const abstract;

        /**
            Is the radar currently rendering to the intermediate and final gauge texture?
            Note: the radar camera is still updated, along with radar variables
            @return Internal gauge rendering enabled
        */
        virtual bool RenderingEnabled() const abstract;

        /**
            Is the radar beam actively sweeping and updating its intermediate texture?
            Note: When in freeze mode, the radar camera and intermediate texture compositing
            is disabled, but the gauge is still rendered
            @return Radar is actively sweeping
        */
        virtual bool FreezeEnabled() const abstract;

        /**
            Get the current x and y coordinates of the cursor
            @param x Out variable for X coordinate of cursor
            @param y Out variable for y coordinate of cursor
        */
        virtual void   GetCursorPositionXY(double& x, double& y) const abstract;

        /**
            Get the LLA of the current cursor position
            @param lla Out variable for the current LLA of the radar cursor
        */
        virtual void   GetCursorPositionLLA(LLA & lla) const abstract;

        /**
            Get the width of the current front blind spot in degrees
            This will usually be 0 unless the radar is in a DBS mode
            @return Width of front blind spot in degrees
        */
        virtual double   GetFrontBlindspotDegrees() const abstract;

        /**
            Get the width of the current side blind spots in degrees
            This will usually be 0 unless the radar is in a DBS mode
            This width is measured from the maximum extents of the current azimuth
            @return Side blind spot width in degrees
        */
        virtual double   GetSideBlindspotDegrees() const abstract;

        /**
            Get the resolution of the radar image
            @param x Width of radar image
            @param y Height of radar image
        */
        virtual void GetRadarResolution(double& x, double& y) const abstract;

        /**
            Get the resolution of the radar gauge
            @param x Width of radar gauge
            @param y Height of radar gauge
        */
        virtual void GetGaugeResolution(double& x, double& y) const abstract;

        // Getters for current information about radar beam

        /**
            Get the current elevation of the radar beam in degrees
            @return Elevation of radar beam
        */
        virtual double GetCurrentRadarScanElevationDegrees() const abstract;

        /**
            Get the current angular offset of the radar beam
            @return Beam offset in degrees
        */
        virtual double GetCurrentRadarBeamOffsetDegrees() const abstract;

        //----------------------------------------------------------------------------
        // Radar System Setters
        //----------------------------------------------------------------------------

        /**
            Clear the radar image on update
        */
        virtual void ClearRadarImage() abstract;

        /**
           Set range rings enabled
           @param show Enable range ring display
        */
        virtual void SetShowRangeRings(bool show) abstract;

        /**
            Set cursor drawing enabled
            @param enabled Enable simple cursor display
        */
        virtual void SetShowCursor(bool enabled) abstract;

        /**
            Set far shore enhancement enabled
            @param enabled Enable far shore enhancement
        */
        virtual void SetFarShoreEnhancementEnabled(bool enabled) abstract;

        /**
            Set the visual zoom level
            Increasing visual zoom is equivalent to zooming on a fixed resolution image
            @param visualZoom Visual zoom level
        */
        virtual void SetVisualZoom(double visualZoom) abstract;

        /**
            Set the data zoom level
            Increasing visual zoom will improve resolution by reducing the area covered
            @param dataZoom Data zoom level
        */
        virtual void SetDataZoom(double dataZoom) abstract;

        /**
            Set the scan rate of the radar beam
            @param rate New sweep rate
        */
        virtual void SetScanRateDegreesPerSecond(double rate) abstract;

        /**
            Set the range of the radar system in miles
            @param range New radar rang in miles
        */
        virtual void SetRangeMiles(double range) abstract;

        /**
            Set whether the radar system is updating its intermediate texture and
            the final gauge image
            @param enabled Radar rendering enabled
        */
        virtual void SetRenderingEnabled(bool enabled) abstract;

        /**
            Set whether the radar is updating the sweep of the beam and intermediate texture
            @param freeze Radar freeze mode enabled
        */
        virtual void SetFreeze(bool freeze) abstract;

        /**
            Set the cursor position with local x and y coordinates
            X and Y are 0-1, top-left  to bottom-right of radar display.
            @param dX Local x coordinate
            @param dY Local y coordinate
        */
        virtual void SetCursorPositionXY(double dX, double dY) abstract;

        /**
            Set the cursor over the specified Lat Lon Alt
            @param lla New latlonalt of cursor, note that this must fall within radar picture
        */
        virtual void SetCursorPositionLLA(const LLA & lla) abstract;

        /**
            Set the resolution of the radar's internal texture.
            @param resolutionX New width for radar image
            @param resolutionY New height for radar image
        */
        virtual void SetRadarImageResolution(double resolutionX, double resolutionY)  abstract;

        /**
            Set the resolution of the gauge being rendered into.
            If the gauge resolution is set incorrectly the radar image will not
            display properly.
            @param resolutionX New width for gauge image
            @param resolutionY New height for gauge image
        */
        virtual void SetRadarGaugeResolution(double resolutionX, double resolutionY)  abstract;

        /**
            Set the maximum extent of the radar sweep
            @param azimuth New azimuth in degrees
        */
        virtual void SetScanAzimuthDegrees(double azimuth) abstract;

        /**
            Set the front DBS blind spot width
            @param frontBlindSpot New width of front blind spot in degrees
        */
        virtual void SetFrontBlindSpotDegrees(double frontBlindSpot) abstract;

        /**
            Set the width of the side DBS blind spots
            @param sideBlindSpots New width of side blind spots
        */
        virtual void SetSideBlindSpotDegrees(double sideBlindSpots) abstract;

        /**
            Set the base color of the radar scan
            @param red New red value 0.0 - 1.0
            @param green New green value 0.0 - 1.0
            @param blue New blue value 0.0 - 1.0
        */
        virtual void SetRadarScanColor(float red, float green, float blue) abstract;

        /**
        Set the gain of the radar scan
        @param gainOffset New gain value -1.0 - 1.0
        */
        virtual void SetRadarGain(float gain) abstract;

        /**
        Set the gauge texture to use a interpolation
        @param interpolate bool to turn on/off radar texture interpolation
        */
        virtual void SetRadarTextureInterpolation(bool interpolate) abstract;

        /**
        Set the radar to use the custom radar angles
        @param useCustomRadarAntenna bool to turn on/off using the custom radar angles
        */
        virtual void UseCustomRadarAngles(bool useCustomRadarAntenna) abstract;

        /**
        Set the gauge texture to use a interpolation
        @param antennaElevation double declination angle in degrees below aircraft horizon
        @param antennaFov double FOV angle in degrees centered about the antennaElevation
        */
        virtual void SetCustomRadarAngles(double antennaElevation, double antennaFov) abstract;

        //Utility functions for converting between LLA and radar image coordinates

        /**
           Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of radar display.
           @param LLA[out] Global LLA position to calculated
           @param dX  Local x coordinate relative to radar display
           @param dY  Local y coordinate relative to radar display
        */
        virtual void GetLLAFromLocalXY(LLA & LLA, double dX, double dY) const abstract;

        /**
          Get LLA of a point on the radar display.  X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
          @param LLA[out] Global LLA position to calculated
          @param dX  Local x coordinate relative to un-zoomed radar display
          @param dY  Local y coordinate relative to un-zoomed radar display
        */
        virtual void GetLLAFromGlobalXY(LLA & LLA, double dX, double dY) const abstract;

        /**
          Get XY position of an LLA relative to zoomed radar display. X and Y are 0-1, top-left to bottom-right of zoomed radar display.
          @param dX[out]  Local x coordinate relative to zoomed radar display
          @param dY[out]  Local y coordinate relative to zoomed radar display
          @param LLA Global LLA position to calculated
        */
        virtual void GetLocalXYCoord(double& dX, double& dY, const LLA & LLA) const abstract;

        /**
          Get XY position of an LLA relative to un-zoomed radar display. X and Y are 0-1, top-left to bottom-right of un-zoomed radar display.
          @param dX[out]  Local x coordinate relative to un-zoomed radar display
          @param dY[out]  Local y coordinate relative to un-zoomed radar display
          @param LLA Global LLA position to calculated
        */
        virtual void GetGlobalXYCoord(double& dX, double& dY, const LLA & LLA) const abstract;
    };

    //{C791A890-CF44-4B58-84B1-0CD0AB2798A9}
    interface __declspec(uuid("{C791A890-CF44-4B58-84B1-0CD0AB2798A9}")) ISimulatedRadarV400;
    extern __declspec(selectany) REFIID  IID_ISimulatedRadarV400 = __uuidof(ISimulatedRadarV400);

	//{949A4F3A-1666-4B69-BF86-C235A9749BAE}
	interface __declspec(uuid("{949A4F3A-1666-4B69-BF86-C235A9749BAE}")) ISimulatedRadarV430;
	extern __declspec(selectany) REFIID  IID_ISimulatedRadarV430 = __uuidof(ISimulatedRadarV430);

    //{3E09D35B-7EE7-4D5A-82DC-DDFD564258F9}
    interface __declspec(uuid("{3E09D35B-7EE7-4D5A-82DC-DDFD564258F9}")) ISimulatedRadarV440;
    extern __declspec(selectany) REFIID  IID_ISimulatedRadarV440 = __uuidof(ISimulatedRadarV440);

    /** \} */
}