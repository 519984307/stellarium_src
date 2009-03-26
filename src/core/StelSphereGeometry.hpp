/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
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

#ifndef _STELSPHEREGEOMETRY_HPP_
#define _STELSPHEREGEOMETRY_HPP_

#include <QVector>
#include <QVariant>
#include <vector>
#include "VecMath.hpp"

class SphericalPolygon;

//! @class SphericalPolygonBase
//! Abstract class defining default implementations for some spherical geometry methods.
//! All methods are reentrant.
class SphericalPolygonBase
{
public:
  	//! @enum WindingRule
	//! Define the possible winding rules to use when setting the contours for a polygon.
	enum PolyWindingRule
	{
		WindingPositive,	//!< Positive winding rule (used for union)
   		WindingAbsGeqTwo	//!< Abs greater or equal 2 winding rule (used for intersection)
	};
	
	//! Destructor
	virtual ~SphericalPolygonBase() {;}
	
	//! Return an openGL compatible array to be displayed using vertex arrays.
	virtual QVector<Vec3d> getVertexArray() const = 0;

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	virtual QVector<bool> getEdgeFlagArray() const = 0;
	
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the 
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, PolyWindingRule windingRule=WindingPositive) = 0;
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour) = 0;

	//! Get the contours defining the SphericalPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const;
	
	//! Return the area in steradians.
	virtual double getArea() const;

	//! Return true if the polygon is an empty polygon.
	virtual bool isEmpty() const {return getVertexArray().isEmpty();}
	
	//! Load the SphericalPolygon information from a QVariant.
	//! The QVariant should contain a list of contours, each contours being a list of ra,dec points
	//! with ra,dec expressed in degree in the ICRS reference frame.
	//! Use QJSONParser to transform a JSON representation into QVariant.
	virtual bool loadFromQVariant(const QVariantMap& contours);

	//! Output the SphericalPolygon information in the form of a QVariant.
	//! The QVariant will contain a list of contours, each contours being a list of ra,dec points
	//! with ra,dec expressed in degree in the ICRS reference frame.
	//! Use QJSONParser to transform a QVariant into its JSON representation.
	virtual QVariantMap toQVariant() const;
	
	//! Returns whether a point is contained into the SphericalPolygon.
	virtual bool contains(const Vec3d& p) const;
	
	//! Return a new SphericalPolygon consisting of the intersection of this and the given SphericalPolygon.
	SphericalPolygon getIntersection(const SphericalPolygonBase& mpoly);
	//! Return a new SphericalPolygon consisting of the union of this and the given SphericalPolygon.
	SphericalPolygon getUnion(const SphericalPolygonBase& mpoly);
	//! Return a new SphericalPolygon consisting of the subtraction of the given SphericalPolygon from this.
	SphericalPolygon getSubtraction(const SphericalPolygonBase& mpoly);
};

//! @class SphericalPolygon
//! A SphericalPolygon is a complex shape defined by the union of contours.
//! Each contour is composed of connected great circles segments with the last point connected to the first one.
//! Contours don't need to be convex (they are internally tesselated into triangles).
class SphericalPolygon : public SphericalPolygonBase
{
public:
	//! Default constructor.
	SphericalPolygon() {;}

	//! Constructor from a list of contours.
	SphericalPolygon(QVector<QVector<Vec3d> >& contours) {setContours(contours);}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	//! The array was precomputed therefore the method is very fast.
	virtual QVector<Vec3d> getVertexArray() const {return triangleVertices;}

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	//! The array was precomputed therefore the method is very fast.
	virtual QVector<bool> getEdgeFlagArray() const {return edgeFlags;}
	
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the 
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive);
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour);
	
private:
	friend void vertexCallback(void* vertexData, void* userData);
	
	//! A list of vertices describing the tesselated polygon. The vertices have to be
	//! used 3 by 3, forming triangles.
	QVector<Vec3d> triangleVertices;

	//! A list of booleans: one per vertex of the triangleVertices array.
	//! Value is true if the vertex belongs to an edge, false otherwise.
	QVector<bool> edgeFlags;
};

//! @class SphericalConvexPolygon
//! A special case of SphericalPolygon for which the polygon is convex.
class SphericalConvexPolygon : public SphericalPolygonBase
{
public:
	//! Default constructor.
	SphericalConvexPolygon() {;}

	//! Constructor from a list of contours.
	SphericalConvexPolygon(QVector<QVector<Vec3d> >& contours) {setContours(contours);}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	//! This method is not optimized for SphericalConvexPolygon instances.
	virtual QVector<Vec3d> getVertexArray() const;

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	//! This method is not optimized for SphericalConvexPolygon instances.
	virtual QVector<bool> getEdgeFlagArray() const;
	
	//! Set the contours defining the SphericalConvexPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule unused for SphericalConvexPolygon.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive)
	{
		Q_ASSERT(contours.size()==1);
		contour=contours.at(0);
	}
	
	//! Get the contours defining the SphericalConvexPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const {QVector<QVector<Vec3d> > contours; contours.append(contour); return contours;}
	
	//! Get the single contour defining the SphericalConvexPolygon.
	const QVector<Vec3d>& getConvexContour() const {return contour;}
	
private:
	//! A list of vertices of the convex contour.
	QVector<Vec3d> contour;
};

//! @namespace StelGeom In this namespace we define different geometrical shapes.
//! We also define two functions, contains(x, y) and intersect(x, y) = intersect(y, x)
//! which is defined for most of the pair of geometrical shapes.
namespace StelGeom
{
/****************
Now the geometrical objects
*****************/

template<class T>
bool intersect(const Vec3d& v, const T& o)
{ return contains(o, v); }

template<class T>
bool intersect(const T& o, const Vec3d& v)
{ return contains(o, v); }

//! @class HalfSpace
//! A HalfSpace is defined by a direction and an aperture.
//! It forms a cone from the center of the Coordinate frame with a radius d.
struct HalfSpace
{
	//! Construct a HalfSpace with a 90 deg aperture and an undefined direction.
	HalfSpace() : d(0) {}
	//! Construct a HalfSpace from its direction and assumes a 90 deg aperture.
	//! @param an a unit vector indicating the direction.
	HalfSpace(const Vec3d& an) : n(an), d(0) {}
	//! Construct a HalfSpace from its direction and aperture.
	//! @param an a unit vector indicating the direction.
	//! @param ar cosinus of the aperture.
	HalfSpace(const Vec3d& an, double ar) : n(an), d(ar) {Q_ASSERT(d==0 || std::fabs(n.lengthSquared()-1.)<0.0000001);}
	HalfSpace(const HalfSpace& other) : n(other.n), d(other.d) {}
	//! Find if a point is contained into the halfspace.
	//! @param v a unit vector.
	bool contains(const Vec3d &v) const {Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.)<0.0000001);return (v*n>=d);}
	bool operator==(const HalfSpace& other) const {return (n==other.n && d==other.d);}

	//! Get the area of the intersection of the halfspace on the sphere in steradian.
	double getArea() const {return 2.*M_PI*(1.-d);}
	
	//! The direction unit vector. Only if d==0, this vector doesn't need to be unit.
	Vec3d n;
	//! The cos of cone radius
	double d;
};

//! @class Polygon
//! A polygon is defined by a set of connected points.
//! The last point is connected to the first one
class Polygon : public std::vector<Vec3d>
{
public:
	//! Default contructor
	Polygon(int asize = 0) : std::vector<Vec3d>(asize) {;}
	//! Special constructor for 3 points polygon
	Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2);
	//! Special constructor for 4 points polygon
	Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3);
};


template<class T>
bool intersect(const Polygon& p, const T& o)
{
	return intersect(o, p);
}


//! @class ConvexS
//! A Convex is defined by several HalfSpaces defining a convex region.
//! A Convex region is not necessarily a ConvexPolygon, it can for example be a single HalfSpace.
//! Because in X11, Convex is \#defined as an int in X11/X.h: (\#define Convex 2) we needed to use another name (ConvexS).
class ConvexS : public std::vector<HalfSpace>
{
public:
	//! copy constructor
	ConvexS(const ConvexS& c) : std::vector<HalfSpace>(c) {}
	//! Default constructor
	ConvexS(int asize = 0) : std::vector<HalfSpace>(asize) {}
	//! Special constructor for 3 halfspaces convex
	ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2);
	//! Special constructor for 4 halfspaces convex
	ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3);

	//! Tell whether the points of the passed Polygon are all outside of at least one HalfSpace
	bool areAllPointsOutsideOneSide(const Polygon& poly) const
	{
		for (const_iterator iter=begin();iter!=end();++iter)
		{
			bool allOutside = true;
			for (Polygon::const_iterator v=poly.begin();v!=poly.end()&& allOutside==true;++v)
			{
				allOutside = allOutside && !iter->contains(*v);
			}
			if (allOutside)
				return true;
		}
		return false;
	}
};



//! @class ConvexPolygon
//! A special case of ConvexS for which all HalfSpace have an aperture of PI/2.
//! The operator [] behave as for a Polygon, i.e. return the vertex positions.
//! To acces the HalfSpaces, use the asConvex() method.
class ConvexPolygon : public ConvexS, public Polygon
{
public:

	//! Default constructor
	ConvexPolygon() {}

	//! Special constructor for 3 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2):
		ConvexS(e0, e1, e2), Polygon(e0, e1, e2)
	{}

	//! Special constructor for 4 points
	ConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3):
		ConvexS(e0, e1, e2, e3), Polygon(e0, e1, e2, e3)
	{}

	bool operator==(const ConvexPolygon& other) const {
		return ((const Polygon&)(*this)==(const Polygon&)other);
	}

	//! By default the [] operator return the vertexes
	const Vec3d& operator[](const Polygon::size_type& i)const {
		return Polygon::operator[](i);
	}

	//! By default the [] operator return the vertexes
	Vec3d& operator[](Polygon::size_type& i) {
		return Polygon::operator[](i);
	}

	//! Return the convex polygon area in steradians
	double getArea() const;

	//! Return the convex polygon barycenter
	Vec3d getBarycenter() const;

	//! Cast to Polygon in case of ambiguity
	Polygon& asPolygon() {return static_cast<Polygon&>(*this);}

	//! Same with const
	const Polygon& asPolygon() const {return static_cast<const Polygon&>(*this);}

	//! Cast to Convex in case of ambiguity
	ConvexS& asConvex() {return static_cast<ConvexS&>(*this);}

	//! Same with const
	const ConvexS& asConvex() const {return static_cast<const ConvexS&>(*this);}

	//! Check if the polygon is valid, i.e. it has no side >180 etc
	bool checkValid() const;

	//! Special case for degenerated polygons (>180 deg), assume full sky, i.e. intersect and contains is always true.
	static ConvexPolygon fullSky();
};


//! We rewrite the intersect for ConvexPolygon
inline bool intersect(const ConvexPolygon& cp1, const ConvexPolygon& cp2)
{
	const ConvexS& c1 = cp1;
	const ConvexS& c2 = cp2;
	return !c1.areAllPointsOutsideOneSide(cp2) && !c2.areAllPointsOutsideOneSide(cp1);
}

//! @class Disk
//! A Disk is defined by a single HalfSpace
struct Disk : HalfSpace
{
	//! Constructor.
	//! @param an a unit vector indicating the the disk center.
	//! @param r the disk radius in radian.
	Disk(const Vec3d& an, double r) : HalfSpace(an, std::cos(r))
	{}
};

//! We rewrite the intersect for ConvexPolygon.
inline bool intersect(const ConvexS& cp1, const ConvexPolygon& cp2)
{
	Q_ASSERT(0);
	// TODO
	return false;
}

template<class S1, class S2>
class Difference
{
public:
	Difference(const S1& s1_, const S2& s2_) : s1(s1_), s2(s2_)
	{}

	S1 s1;
	S2 s2;
};

template<class S1, class S2, class S>
inline bool intersect(const Difference<S1, S2>& d, const S& s)
{
	return !contains(d.s2, s) && intersect(d.s1, s);
}

template<class S1, class S2>
bool intersect(const Difference<S1, S2>& d, const Vec3d& v)
{ return !contains(d.s2, v) && intersect(d.s1, v); }

template<class S1, class S2, class S>
inline bool contains(const Difference<S1, S2>&d, const S& s)
{
	return !intersect(d.s2, s) && contains(d.s1, s);
}



inline bool contains(const HalfSpace& h,  const Vec3d& v)
{
	return h.contains(v);
}

inline bool contains(const HalfSpace& h, const Polygon& poly)
{
	for (Polygon::const_iterator iter=poly.begin();iter!=poly.end();++iter)
	{
		if (!h.contains(*iter))
			return false;
	}
	return true;
}

inline bool contains(const ConvexPolygon& c, const Vec3d& v)
{
	const ConvexS& conv = c;
	for (ConvexS::const_iterator iter=conv.begin();iter!=conv.end();++iter)
	{
		if (!iter->contains(v))
			return false;
	}
	return true;
}

inline bool contains(const ConvexPolygon& cp1, const ConvexPolygon& cp2)
{
	const Polygon& poly = cp2;
	for (Polygon::const_iterator iter=poly.begin();iter!=poly.end();++iter)
	{
		if (!contains(cp1, *iter))
			return false;
	}
	return true;
}

//! We rewrite the intersect for Disk/ConvexPolygon
//! This method checks that the minimum distance between the Disk center and each side of the ConvexPolygon
//! is smaller than the disk radius
inline bool intersect(const HalfSpace& h, const ConvexPolygon& cp)
{
	if (contains(cp, h.n))
		return true;
	const ConvexS& c = cp;
	for (ConvexS::const_iterator iter=c.begin();iter!=c.end();++iter)
	{
		const double cosAlpha = h.n*iter->n;
		if (!(std::sqrt(1.-cosAlpha*cosAlpha) > h.d))
			return true;
	}
	return false;
}

// special for ConvexPolygon
inline bool intersect(const ConvexPolygon& c, const HalfSpace& h)
{
	return intersect(h, c);
}

//! Compute the intersection of 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 HalfSpace don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const HalfSpace& h1, const HalfSpace& h2, Vec3d& p1, Vec3d& p2);

}	// namespace StelGeom

#endif // _STELSPHEREGEOMETRY_HPP_

