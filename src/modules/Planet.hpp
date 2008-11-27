/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _PLANET_HPP_
#define _PLANET_HPP_

#include <QString>

#include "StelObject.hpp"
#include "ToneReproducer.hpp"
#include "vecmath.h"
#include "callbacks.hpp"
#include "StelFader.hpp"
#include "Translator.hpp"
#include "StelTextureTypes.hpp"
#include "ProjectorType.hpp"

// The callback type for the external position computation function
typedef boost::callback<void, double, double*> posFuncType;

typedef void (OsulatingFunctType)(double jd0,double jd,double xyz[3]);

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0
#define ORBIT_SEGMENTS 72

class StelFont;
class StelPainter;

struct TrailPoint
{
	Vec3d point;
	double date;
};


// Class used to store orbital elements
class RotationElements
{
public:
    RotationElements(void) : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), precessionRate(0.) {}
    float period;          // rotation period
    float offset;          // rotation at epoch
    double epoch;
    float obliquity;       // tilt of rotation axis w.r.t. ecliptic
    float ascendingNode;   // long. of ascending node of equator on the ecliptic
    float precessionRate;  // rate of precession of rotation axis in rads/day
    double siderealPeriod; // sidereal period (Planet year in earth days)
};

// Class to manage rings for planets like saturn
class Ring
{
public:
	Ring(double radiusMin,double radiusMax,const QString &texname);
	~Ring(void);
	void draw(const StelPainter* painter,const Mat4d& mat,double screenSz);
	double getSize(void) const {return radiusMax;}
private:
	const double radiusMin;
	const double radiusMax;
	StelTextureSP tex;
};


class Planet : public StelObject
{
public:
	friend class SolarSystem;
	Planet(Planet *parent,
	       const QString& englishName,
	       int flagLighting,
	       double radius,
	       double oblateness,
	       Vec3f color,
	       float albedo,
	       const QString& texMapName,
	       const QString& texHaloName,
	       posFuncType _coordFunc,
	       OsulatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden, 
	       bool hasAtmosphere);

	~Planet();

	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from StelObject
	//! Get a string with data about the Planet.
	//! Planets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - PlainText
	//! @param core the Stelore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Planet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual double getCloseViewFov(const Navigator * nav) const;
	virtual double getSatellitesFov(const Navigator * nav) const;
	virtual double getParentSatellitesFov(const Navigator * nav) const;
	virtual float getVMagnitude(const Navigator * nav) const;
	virtual float getSelectPriority(const Navigator *nav) const;
	virtual Vec3f getInfoColor(void) const;
	virtual QString getType(void) const {return "Planet";}
	virtual Vec3d getJ2000EquatorialPos(const Navigator *nav) const;
	virtual QString getEnglishName(void) const {return englishName;}
	virtual QString getNameI18n(void) const {return nameI18;}
	virtual double getAngularSize(const StelCore* core) const;
	virtual bool hasAtmosphere(void) {return atmosphere;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods of SolarSystem object
	//! Translate planet name using the passed translator
	void translateName(Translator& trans) {nameI18 = trans.qtranslate(englishName);}
	
	// Draw the Planet
	void draw(StelCore* core, float maxMagLabels);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to Planet
	//! Get the radius of the planet in AU.
	//! @return the radius of the planet in astronomical units.
	double getRadius(void) const {return radius;}
	double getSiderealDay(void) const {return re.period;}
	
	const QString& getTextMapName() const {return texMapName;}
	
	// Compute the z rotation to use from equatorial to geographic coordinates
	double getSiderealTime(double jd) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	void setRotEquatorialToVsop87(const Mat4d &m);

	const RotationElements &getRotationElements(void) const {return re;}

	// Compute the position in the parent Planet coordinate system
	void computePositionWithoutOrbits(const double date);
	void computePosition(const double date);

	// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
	void computeTransMatrix(double date);

	// Get the phase angle for an observer at pos obsPos in the heliocentric coordinate (in AU)
	double getPhase(Vec3d obsPos) const;
	// Get the angular size of the spheroid of the planet (i.e. without the rings)
	double getSpheroidAngularSize(const StelCore* core) const;
				
	// Set the orbital elements
	void setRotationElements(float _period, float _offset, double _epoch,
		float _obliquity, float _ascendingNode, float _precessionRate, double _siderealPeriod);
	double getRotAscendingnode(void) const {return re.ascendingNode;}
	double getRotObliquity(void) const {return re.obliquity;}

	// Get the Planet position in the parent Planet ecliptic coordinate
	Vec3d getEclipticPos() const;

	// Return the heliocentric ecliptical position
	Vec3d getHeliocentricEclipticPos() const;
	void setHeliocentricEclipticPos(const Vec3d &pos);

	// Compute the distance to the given position in heliocentric coordinate (in AU)
	double computeDistance(const Vec3d& obsHelioPos);
	double getDistance(void) const {return distance;}

	void setRings(Ring* r) {rings = r;}

	void setSphereScale(float s) {sphereScale = s;}
	float getSphereScale(void) const {return sphereScale;}

	const Planet* getParent(void) const {return parent;}

	static void setFont(StelFont* f) {planetNameFont = f;}
	
	static void setLabelColor(const Vec3f& lc) {labelColor = lc;}
	static const Vec3f& getLabelColor(void) {return labelColor;}

	void update(int deltaTime);
	
	void setFlagHints(bool b){hintFader = b;}
	bool getFlagHints(void) const {return hintFader;}
	
	void setFlagLabels(bool b){flagLabels = b;}
	bool getFlagLabels(void) const {return flagLabels;}
	
	///////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	
	///// Trail related code
	// Should move to a TrailPath class which works on a StelObject, not on a Planet
	void updateTrail(const Navigator* nav);
	void drawTrail(const StelCore* core);
	//! Start/stop accumulating new trail data (clear old data)
	void startTrail(bool b);
	void setFlagTrail(bool b){if(b == trailFader) return; trailFader = b; startTrail(b);}
	bool getFlagTrail(void) const {return trailFader;}
	static void setTrailColor(const Vec3f& c) { trailColor = c; }
	static const Vec3f& getTrailColor() { return trailColor; }
	static Vec3f trailColor;
	std::list<TrailPoint>trail;
	bool trailOn;                  // accumulate trail data if true
	double DeltaTrail;
	int MaxTrail;
	double lastTrailJD;
	bool firstPoint;               // if need to take first point of trail still
	LinearFader trailFader;
	
	///// Orbit related code
	// Should move to an OrbitPath class which works on a SolarSystemObject, not a Planet
	void setFlagOrbits(bool b){orbitFader = b;}
	bool getFlagOrbits(void) const {return orbitFader;}
	LinearFader orbitFader;
	// draw orbital path of Planet
	void drawOrbit(const StelCore*);
	Vec3d orbit[ORBIT_SEGMENTS];   // store heliocentric coordinates for drawing the orbit
	double lastOrbitJD;
	double deltaJD;
	double deltaOrbitJD;
	bool orbitCached;              // whether orbit calculations are cached for drawing orbit yet
	bool closeOrbit;               // whether to connect the beginning of the orbit line to
	                               // the end: good for elliptical orbits, bad for parabolic
	                               // and hyperbolic orbits
	
	static Vec3f orbitColor;
	static void setOrbitColor(const Vec3f& oc) {orbitColor = oc;}
	static const Vec3f& getOrbitColor() {return orbitColor;}
	
protected:
	static StelTextureSP texEarthShadow;     // for lunar eclipses
	
	// draw earth shadow on moon for lunar eclipses
	void drawEarthShadow(StelCore* core); 
	
	// Return the information string "ready to print" :)
	QString getSkyLabel(const Navigator * nav) const;
	
	// Draw the 3d model. Call the proper functions if there are rings etc..
	void draw3dModel(StelCore* core, const Mat4d& mat, float screenSz);
	
	// Draw the 3D sphere
	void drawSphere(const StelPainter* painter, float screenSz);

	// Draw the circle and name of the Planet
	void drawHints(const StelCore* core);

	QString englishName;            // english planet name
	QString nameI18;                // International translated name
	QString texMapName;				// Texture file path
	int flagLighting;               // Set wether light computation has to be proceed
	RotationElements re;            // Rotation param
	double radius;                  // Planet radius in UA
	double oneMinusOblateness;      // (polar radius)/(equatorial radius)
	Vec3d eclipticPos;             // Position in UA in the rectangular ecliptic coordinate system
	                                // centered on the parent Planet
	Vec3d screenPos;                // Used to store temporarily the 2D position on screen
	Vec3d previousScreenPos;        // The position of this planet in the previous frame.
	Vec3f color;
	float albedo;                   // Planet albedo
	Mat4d rotLocalToParent;
	float axisRotation;            // Rotation angle of the Planet on it's axis
	StelTextureSP texMap;             // Planet map texture
	Ring* rings;                    // Planet rings
	double distance;                // Temporary variable used to store the distance to a given point
	                                // it is used for sorting while drawing
	float sphereScale;             // Artificial scaling for better viewing
	double lastJD;
	// The callback for the calculation of the equatorial rect heliocentric position at time JD.
	posFuncType coordFunc;
	OsulatingFunctType *const osculatingFunc;
	const Planet *parent;           // Planet parent i.e. sun for earth
	QList<Planet*> satellites;      // satellites of the Planet
	LinearFader hintFader;
	LinearFader labelsFader;        // Store the current state of the label for this planet
	bool flagLabels;                // Define whether labels should be displayed
	bool hidden;                    // useful for fake planets used as observation positions - not drawn or labeled
	bool atmosphere;                // Does the planet have an atmosphere?
	
	static StelFont* planetNameFont; // Font for names
	static Vec3f labelColor;
	static StelTextureSP hintCircleTex;
};

#endif // _PLANET_HPP_

