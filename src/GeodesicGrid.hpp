/*
 
GeodesicGrid: a library for dividing the sphere into triangle zones
by subdividing the icosahedron
 
Author and Copyright: Johannes Gajdosik, 2006
 
This library requires a simple Vector library,
which may have different copyright and license,
for example Vec3d from vecmath.h.
 
In the moment I choose to distribute the library under the GPL,
later I may choose to additionally distribute it under a more
relaxed license like the LGPL. If you want to have the library
under another license, please ask me.
 
 
 
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/

#ifndef _GEODESIC_GRID_H_
#define _GEODESIC_GRID_H_

#include "SphereGeometry.hpp"

class GeodesicSearchResult;

class GeodesicGrid
{
	// Grid of triangles (zones) on the sphere with radius 1,
	// generated by subdividing the icosahedron.
	// level 0: just the icosahedron, 20 zones
	// level 1: 80 zones, level 2: 320 zones, ...
	// Each zone has a unique integer number in the range
	// [0,getNrOfZones()-1].
public:
	GeodesicGrid(int max_level);
	~GeodesicGrid(void);
	
	
	int getMaxLevel(void) const {return max_level;}
	
	static int nrOfZones(int level)
	{return (20<<(level<<1));} // 20*4^level
	
	int getNrOfZones(void) const {return nrOfZones(max_level);}
	
	//! Return the position of the 3 corners for the triangle at the given level and index
	void getTriangleCorners(int lev, int index, Vec3d& c0, Vec3d& c1, Vec3d& c2) const;
	//! Return the index of the partner triangle with which to form a paralelogram
	int getPartnerTriangle(int lev, int index) const;
	
	typedef void (VisitFunc)(int lev,int index,
	                         const Vec3d &c0,
	                         const Vec3d &c1,
	                         const Vec3d &c2,
	                         void *context);
	
	void visitTriangles(int max_visit_level, VisitFunc *func,void *context) const;

	//! Find the zone number in which a given point lies.
	//! prerequisite: v*v==1
	//! When the point lies on the border of two or more zones,
	//! one such zone is returned (always the same one,
	//! because the algorithm is deterministic).
	int searchZone(const Vec3d &v,int search_level) const;

	//! Find all zones that lie fully(inside) or partly(border)
	//! in the intersection of the given half spaces.
	//! The result is accurate when (0,0,0) lies on the border of
	//! each half space. If this is not the case,
	//! the result may be inaccurate, because it is assumed, that
	//! a zone lies in a half space when its 3 corners lie in this half space.
	//! inside[l] points to the begin of an integer array of size nrOfZones(l),
	//! border[l] points to one after the end of the same integer array.
	//! The array will be filled from the beginning with the inside zone numbers
	//! and from the end with the border zone numbers of the given level l
	//! for 0<=l<=getMaxLevel().
	//! inside[l] will not contain zones that are already contained
	//! in inside[l1] for some l1 < l.
	//! In order to restrict search depth set max_search_level < max_level,
	//! for full search depth set max_search_level = max_level,
	void searchZones(const StelGeom::ConvexS& convex,
	                 int **inside,int **border,int max_search_level) const;

	//! Return a search result matching the given spatial region
	//! The result is cached, meaning that it is very fast to search the same region consecutively
	//! @return a GeodesicSearchResult instance which must be used with GeodesicSearchBorderIterator and GeodesicSearchInsideIterator
	const GeodesicSearchResult* search(const StelGeom::ConvexS& convex, int max_search_level) const;
	
	//! Convenience function returning a search result matching the given spatial region
	//! The result is cached, meaning that it is very fast to search the same region consecutively
	//! @return a GeodesicSearchResult instance which must be used with GeodesicSearchBorderIterator and GeodesicSearchInsideIterator
	const GeodesicSearchResult* search(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2,const Vec3d &e3,int max_search_level) const;

private:
	const Vec3d& getTriangleCorner(int lev, int index, int cornerNumber) const;
	void initTriangle(int lev,int index,
	                  const Vec3d &c0,
	                  const Vec3d &c1,
	                  const Vec3d &c2);
	void visitTriangles(int lev,int index,
	                    const Vec3d &c0,
	                    const Vec3d &c1,
	                    const Vec3d &c2,
	                    int max_visit_level,
	                    VisitFunc *func,
	                    void *context) const;
	void searchZones(int lev,int index,
	                 const StelGeom::ConvexS& convex,
	                 const int *index_of_used_half_spaces,
	                 const int half_spaces_used,
	                 const bool *corner0_inside,
	                 const bool *corner1_inside,
	                 const bool *corner2_inside,
	                 int **inside,int **border,int max_search_level) const;

	const int max_level;
	struct Triangle
	{
		Vec3d e0,e1,e2;   // Seitenmittelpunkte
	};
	Triangle **triangles;
	// 20*(4^0+4^1+...+4^n)=20*(4*(4^n)-1)/3 triangles total
	// 2+10*4^n corners
	
	//! A cached search result used to avoid doing twice the same search
	mutable GeodesicSearchResult* cacheSearchResult;
	mutable int lastMaxSearchlevel;
	mutable StelGeom::ConvexS lastSearchRegion;
};

class GeodesicSearchResult
{
public:
	GeodesicSearchResult(const GeodesicGrid &grid);
	~GeodesicSearchResult(void);
	void print(void) const;
private:
	friend class GeodesicSearchInsideIterator;
	friend class GeodesicSearchBorderIterator;
	friend class GeodesicGrid;
	
	void search(const StelGeom::ConvexS& convex, int max_search_level);
	
	const GeodesicGrid &grid;
	int **const zones;
	int **const inside;
	int **const border;
};

class GeodesicSearchBorderIterator
{
public:
	GeodesicSearchBorderIterator(const GeodesicSearchResult &r,int level)
		: r(r),level((level<0)?0:(level>r.grid.getMaxLevel())
			             ?r.grid.getMaxLevel():level),
			end(r.zones[GeodesicSearchBorderIterator::level]+
			    GeodesicGrid::nrOfZones(GeodesicSearchBorderIterator::level))
	{reset();}
	void reset(void) {index = r.border[level];}
	int next(void) // returns -1 when finished
	{if (index < end) {return *index++;} return -1;}
private:
	const GeodesicSearchResult &r;
	const int level;
	const int *const end;
	const int *index;
};


class GeodesicSearchInsideIterator
{
public:
	GeodesicSearchInsideIterator(const GeodesicSearchResult &r,int level)
: r(r),max_level((level<0)?0:(level>r.grid.getMaxLevel())
	                 ?r.grid.getMaxLevel():level)
	{reset();}
	void reset(void);
	int next(void); // returns -1 when finished
private:
	const GeodesicSearchResult &r;
	const int max_level;
	int level;
	int max_count;
	int *index_p;
	int *end_p;
	int index;
	int count;
};

#endif
