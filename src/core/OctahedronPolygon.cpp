/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#include "StelUtils.hpp"
#include "OctahedronPolygon.hpp"
#include "StelSphereGeometry.hpp"
#include "glues.h"

#include <QFile>

const Vec3d OctahedronPolygon::sideDirections[] = {	Vec3d(1,1,1), Vec3d(1,1,-1),Vec3d(-1,1,1),Vec3d(-1,1,-1),
	Vec3d(1,-1,1),Vec3d(1,-1,-1),Vec3d(-1,-1,1),Vec3d(-1,-1,-1)};

inline bool intersectsBoundingCap(const Vec3d& n1, double d1, const Vec3d& n2, double d2)
{
	return SphericalCap(n1, d1).intersects(SphericalCap(n2, d2));
}

inline bool containsBoundingCap(const Vec3d& n1, double d1, const Vec3d& n2, double d2)
{
	return SphericalCap(n1, d1).contains(SphericalCap(n2, d2));
}

static int getSide(const Vec3d& v, int onLine)
{
	Q_ASSERT(onLine>=0 && onLine<3);
	return v[onLine]>=0. ? 0 : 1;
}

QDataStream& operator<<(QDataStream& out, const EdgeVertex& v)
{
	out << v.vertex << v.edgeFlag;
	return out;
}

QDataStream& operator>>(QDataStream& in, EdgeVertex& v)
{
	in >> v.vertex >> v.edgeFlag;
	return in;
}

QString SubContour::toJSON() const
{
	QString res("[");
	double ra, dec;
	foreach (const EdgeVertex& v, *this)
	{
		StelUtils::rectToSphe(&ra, &dec, v.vertex);
		res += QString("[") + QString::number(ra*180./M_PI, 'g', 12) + "," + QString::number(dec*180./M_PI, 'g', 12) + "," + (v.edgeFlag ? QString("true"): QString("false")) + "],";
	}
	res[res.size()-1]=']';
	return res;
};

OctahedronPolygon::OctahedronPolygon(const QVector<Vec3d>& contour) : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines)
{
	sides.resize(8);
	appendSubContour(SubContour(contour));
	tesselate(WindingPositive);
	updateVertexArray();
}

OctahedronPolygon::OctahedronPolygon(const QVector<QVector<Vec3d> >& contours) : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines)
{
	sides.resize(8);
	foreach (const QVector<Vec3d>& contour, contours)
		appendSubContour(SubContour(contour));
	tesselate(WindingPositive);
	updateVertexArray();
}

OctahedronPolygon::OctahedronPolygon(const SubContour& initContour)
{
	sides.resize(8);
	appendSubContour(initContour);
	tesselate(WindingPositive);
	updateVertexArray();
}


void OctahedronPolygon::appendSubContour(const SubContour& inContour)
{
	QVarLengthArray<QVector<SubContour>,8 > resultSides;
	resultSides.resize(8);
	QVector<SubContour> splittedContour1[2];
	// Split the contour on the plan Y=0
	splitContourByPlan(1, inContour, splittedContour1);
	// Re-split the contours on the plan X=0
	QVector<SubContour> splittedVertices2[4];
	foreach (const SubContour& subContour, splittedContour1[0])
		splitContourByPlan(0, subContour, splittedVertices2);
	foreach (const SubContour& subContour, splittedContour1[1])
		splitContourByPlan(0, subContour, splittedVertices2+2);

	// Now complete the contours which cross the areas from one side to another by adding poles
	for (int c=0;c<4;++c)
	{
		for (int i=0;i<splittedVertices2[c].size();++i)
		{
			SubContour& tmpSubContour = splittedVertices2[c][i];
			// If the contour was not splitted, don't try to connect
			if (tmpSubContour.last().edgeFlag==true)
				continue;
			Vec3d v = tmpSubContour.first().vertex^tmpSubContour.last().vertex;
			if (v[2]>0.00000001)
			{
				// A south pole has to be added
				tmpSubContour << EdgeVertex(Vec3d(0,0,-1), false);
			}
			else if (v[2]<-0.0000001)
			{
				// A north pole has to be added
				tmpSubContour << EdgeVertex(Vec3d(0,0,1), false);
			}
			else
			{
				// else the contour ends on the same longitude line as it starts
				Q_ASSERT(std::fabs(v[0])<0.0000001 || std::fabs(v[1])<0.0000001);
			}
		}
		foreach (const SubContour& subContour, splittedVertices2[c])
		{
			splitContourByPlan(2, subContour, resultSides.data()+c*2);
		}
	}
	projectOnOctahedron(resultSides);

	// Append the new sides to this
	Q_ASSERT(sides.size()==8 && resultSides.size()==8);
	for (int i=0;i<8;++i)
	{
		sides[i] += resultSides[i];
	}
}

// Return the area in squared degrees.
double OctahedronPolygon::getArea() const
{
	// Use Girard's theorem for each subtriangles
	double area = 0.;
	Vec3d v1, v2, v3;
	const QVector<Vec3d>& trianglesArray = getFillVertexArray().vertex;
	Q_ASSERT(getFillVertexArray().primitiveType==StelVertexArray::Triangles);
	for (int i=0;i<trianglesArray.size()/3;++i)
	{
		v1 = trianglesArray[i*3+0] ^ trianglesArray[i*3+1];
		v2 = trianglesArray[i*3+1] ^ trianglesArray[i*3+2];
		v3 = trianglesArray[i*3+2] ^ trianglesArray[i*3+0];
		area += 2.*M_PI - v1.angle(v2) - v2.angle(v3) - v3.angle(v1);
	}
	return area;
}

// Return a point located inside the polygon.
Vec3d OctahedronPolygon::getPointInside() const
{
	const QVector<Vec3d>& trianglesArray = getFillVertexArray().vertex;
	Q_ASSERT(getFillVertexArray().primitiveType==StelVertexArray::Triangles);
	Vec3d res(trianglesArray[0]);
	res+=trianglesArray[1];
	res+=trianglesArray[2];
	res.normalize();
	return res;
}

void OctahedronPolygon::append(const OctahedronPolygon& other)
{
	Q_ASSERT(sides.size()==8 && other.sides.size()==8);
	for (int i=0;i<8;++i)
	{
		sides[i] += other.sides[i];
	}
}

void OctahedronPolygon::appendReversed(const OctahedronPolygon& other)
{
	Q_ASSERT(sides.size()==8 && other.sides.size()==8);
	for (int i=0;i<8;++i)
	{
		foreach (const SubContour& sub, other.sides[i])
		{
			sides[i] += sub.reversed();
		}
	}
}

void OctahedronPolygon::projectOnOctahedron(QVarLengthArray<QVector<SubContour>,8 >& inSides)
{
	Q_ASSERT(inSides.size()==8);
	QVector<SubContour>* subs = inSides.data();

	for (int i=0;i<8;++i)
	{
		for (QVector<SubContour>::Iterator iter=subs[i].begin();iter!=subs[i].end();++iter)
		{
			for (SubContour::Iterator v=iter->begin();v!=iter->end();++v)
			{
				// Project on the face with aperture = 90 deg
				v->vertex *= 1./(sideDirections[i]*v->vertex);
				// May want to add offsets after that to map TOAST projection
			}
		}
	}
}

void OctahedronPolygon::updateVertexArray()
{
	Q_ASSERT(sides.size()==8);
	fillCachedVertexArray.vertex.clear();
	outlineCachedVertexArray.vertex.clear();
	for (int i=0;i<8;++i)
	{
		for (QVector<SubContour>::Iterator triangle=sides[i].begin();triangle!=sides[i].end();++triangle)
		{
			const int idx = fillCachedVertexArray.vertex.size();
			Q_ASSERT(triangle->size()==3);	// Only triangles here
			fillCachedVertexArray.vertex.append(triangle->at(0).vertex);
			fillCachedVertexArray.vertex.last().normalize();
			fillCachedVertexArray.vertex.append(triangle->at(1).vertex);
			fillCachedVertexArray.vertex.last().normalize();
			fillCachedVertexArray.vertex.append(triangle->at(2).vertex);
			fillCachedVertexArray.vertex.last().normalize();

			if (triangle->at(0).edgeFlag)
			{
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx));
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx+1));
			}
			if (triangle->at(1).edgeFlag)
			{
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx+1));
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx+2));
			}
			if (triangle->at(2).edgeFlag)
			{
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx+2));
				outlineCachedVertexArray.vertex.append(fillCachedVertexArray.vertex.at(idx));
			}
		}
	}
	computeBoundingCap();
}

// Store data for the GLUES tesselation callbacks
struct OctTessCallbackData
{
	SubContour result;				//! Contains the resulting tesselated vertices.
	bool edgeFlag;					//! Used to store temporary edgeFlag found by the tesselator.
	QList<EdgeVertex> tempVertices;	//! Used to store the temporary combined vertices
};

void errorCallback(GLenum errno)
{
	qWarning() << "Tesselator error:" << QString::fromAscii((char*)gluesErrorString(errno));
	Q_ASSERT(0);
}

void vertexCallback(EdgeVertex* vertexData, OctTessCallbackData* userData)
{
	userData->result.append(EdgeVertex(vertexData->vertex, userData->edgeFlag && vertexData->edgeFlag));
}

void edgeFlagCallback(GLboolean flag, OctTessCallbackData* userData)
{
	userData->edgeFlag=flag;
}

void combineCallback(double coords[3], EdgeVertex* vertex_data[4], GLfloat weight[4], EdgeVertex** outData, OctTessCallbackData* userData)
{
	bool newFlag=false;
	//qDebug() << "Combine data" << coords[0] << coords[1] << coords[2];
	for (int i=0;i<4;++i)
	{
		if (vertex_data[i]==NULL)
			break;
		EdgeVertex* dd = vertex_data[i];
		//qDebug() << "Vertex " << i << dd->vertex.toString() << weight[i];
		newFlag = newFlag && dd->edgeFlag;
	}
	// Check that the new coordinate lay on the octahedron plane
	Q_ASSERT(fabs(fabs(coords[0])+fabs(coords[1])+fabs(coords[2])-1.)<0.000001);
	userData->tempVertices.append(EdgeVertex(Vec3d(coords[0], coords[1], coords[2]),newFlag));
	*outData = &(userData->tempVertices.last());
}

void checkBeginCallback(GLenum type)
{
	Q_ASSERT(type==GL_TRIANGLES);
}

bool OctahedronPolygon::isTriangleConvexPositive2D(const Vec3d& a, const Vec3d& b, const Vec3d& c)
{
	return	(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>=0. &&
			(c[0]-b[0])*(a[1]-b[1])-(c[1]-b[1])*(a[0]-b[0])>=0. &&
			(a[0]-c[0])*(b[1]-c[1])-(a[1]-c[1])*(b[0]-c[0])>=0.;
}

bool OctahedronPolygon::triangleContains2D(const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& p)
{
	return	(b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])>=0. &&
			(c[0]-b[0])*(p[1]-b[1])-(c[1]-b[1])*(p[0]-b[0])>=0. &&
			(a[0]-c[0])*(p[1]-c[1])-(a[1]-c[1])*(p[0]-c[0])>=0.;
}

SubContour OctahedronPolygon::tesselateOneSide(GLUEStesselator* tess, int sidenb) const
{
	const QVector<SubContour>& contours = sides[sidenb];
	Q_ASSERT(!contours.isEmpty());
	OctTessCallbackData data;
	gluesTessNormal(tess, 0.,0., (sidenb%2==0 ? -1. : 1.));
	gluesTessBeginPolygon(tess, &data);
	for (int c=0;c<contours.size();++c)
	{
		//qDebug() << contours.at(c).toJSON();
		gluesTessBeginContour(tess);
		for (int i=0;i<contours.at(c).size();++i)
		{
			//qDebug() << contours[c][i].vertex.toString();
			gluesTessVertex(tess, const_cast<double*>((const double*)contours[c][i].vertex.data()), (void*)&(contours[c][i]));
		}
		gluesTessEndContour(tess);
	}
	gluesTessEndPolygon(tess);
	Q_ASSERT(data.result.size()%3==0);	// There should be only positive triangles here
//#ifndef NDEBUG
//	for (int i=0;i<data.result.size()/3;++i)
//	{
//		if ((sidenb%2==0 ?
//			!isTriangleConvexPositive2D(data.result[i*3+2].vertex, data.result[i*3+1].vertex, data.result[i*3].vertex) :
//			!isTriangleConvexPositive2D(data.result[i*3].vertex, data.result[i*3+1].vertex, data.result[i*3+2].vertex)))
//		{
//			qDebug() << "Error, negative triangle in side " << sidenb;
//			qDebug() << data.result[i*3].vertex.toStringLonLat() << data.result[i*3+1].vertex.toStringLonLat() << data.result[i*3+2].vertex.toStringLonLat();
//			Q_ASSERT(0);
//		}
//	}
//#endif
	return data.result;
}

void OctahedronPolygon::tesselate(TessWindingRule windingRule)
{
	Q_ASSERT(sides.size()==8);
	// Use GLUES tesselation functions to transform the polygon into a list of triangles
	GLUEStesselator* tess = gluesNewTess();
#ifndef NDEBUG
	gluesTessCallback(tess, GLUES_TESS_BEGIN, (GLvoid(*)()) &checkBeginCallback);
#endif
	gluesTessCallback(tess, GLUES_TESS_VERTEX_DATA, (GLvoid(*)()) &vertexCallback);
	gluesTessCallback(tess, GLUES_TESS_EDGE_FLAG_DATA, (GLvoid(*)()) &edgeFlagCallback);
	gluesTessCallback(tess, GLUES_TESS_ERROR, (GLvoid(*)()) &errorCallback);
	gluesTessCallback(tess, GLUES_TESS_COMBINE_DATA, (GLvoid(*)()) &combineCallback);
	const double windRule = (windingRule==OctahedronPolygon::WindingPositive) ? GLUES_TESS_WINDING_POSITIVE : GLUES_TESS_WINDING_ABS_GEQ_TWO;
	gluesTessProperty(tess, GLUES_TESS_WINDING_RULE, windRule);
	//gluesTessProperty(tess, GLUES_TESS_BOUNDARY_ONLY, GL_TRUE);
	// Call the tesselator on each side
	SubContour res2;
	res2.resize(3);
	for (int i=0;i<8;++i)
	{
		if (sides[i].isEmpty())
			continue;
		qDebug() << "Before" << sides[i].size();
		SubContour res = tesselateOneSide(tess, i);
		sides[i].clear();
		Q_ASSERT(res.size()%3==0);	// There should be only triangles here
		for (int j=0;j<res.size()/3;++j)
		{
			res2[0]=res[j*3];
			res2[1]=res[j*3+1];
			res2[2]=res[j*3+2];

			// Post processing, GLU seems to sometimes output triangles oriented in the wrong direction..
			// Get rid of them in an ugly way. TODO Need to find the real cause.
			if ((i%2==0 ?
			isTriangleConvexPositive2D(res2[2].vertex, res2[1].vertex, res2[0].vertex) :
			isTriangleConvexPositive2D(res2[0].vertex, res2[1].vertex, res2[2].vertex)))
			{
				sides[i].append(res2);
			}
			else
			{
				qDebug() << "Discarded a CW triangle";
			}
		}
		qDebug() << "After" << sides[i].size();
	}
	gluesDeleteTess(tess);
}

#ifndef NDEBUG
bool OctahedronPolygon::checkAllTrianglesPositive() const
{
	for (int sidenb=0;sidenb<8;++sidenb)
	{
		foreach (const SubContour& c, sides[sidenb])
		{
			for (int i=0;i<c.size()/3;++i)
			{
				if ((sidenb%2==0 ?
					!isTriangleConvexPositive2D(c[i*3+2].vertex, c[i*3+1].vertex, c[i*3].vertex) :
					!isTriangleConvexPositive2D(c[i*3].vertex, c[i*3+1].vertex, c[i*3+2].vertex)))
				{
					qDebug() << "Error, negative triangle in side " << sidenb;
					qDebug() << c[i*3].vertex.toStringLonLat() << c[i*3+1].vertex.toStringLonLat() << c[i*3+2].vertex.toStringLonLat();
					return false;
				}
			}
		}
	}
	// Also check that not 2 triangles overlap
	OctahedronPolygon tmp = *this;
	tmp.tesselate(WindingAbsGeqTwo);
	if (!tmp.isEmpty())
	{
		qDebug() << "Error, overlapping triangles";
		return false;
	}
	return true;
}
#endif

QString OctahedronPolygon::toJson() const
{
	QString res = "[";
	for (int sidenb=0;sidenb<8;++sidenb)
	{
		res += "[";
		foreach (const SubContour& c, sides[sidenb])
		{
			res += c.toJSON();
		}
		res += "]";
	}
	res += "]";
	return res;
}

void OctahedronPolygon::inPlaceIntersection(const OctahedronPolygon& mpoly)
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return;
	Q_ASSERT(checkAllTrianglesPositive());
	Q_ASSERT(mpoly.checkAllTrianglesPositive());
	append(mpoly);
	tesselate(WindingAbsGeqTwo);
	tesselate(WindingPositive);
#ifndef NDEBUG
	// Check that we are made of already tesselated positive triangles
	if (!checkAllTrianglesPositive())
	{
		qDebug() << "A buggy OctahedronPolygon was found";
		//qDebug() << this->toJson();

//		Q_ASSERT(!tmp.isEmpty());
//		QFile f1("buggyOctahedronPolygon-intersect1.dat");
//		f1.open(QIODevice::WriteOnly);
//		QDataStream out1(&f1);
//		out1 << tmp;
//		f1.close();
//
//		QFile f2("buggyOctahedronPolygon-intersect2.dat");
//		f2.open(QIODevice::WriteOnly);
//		QDataStream out2(&f2);
//		out2 << mpoly;
//		f2.close();
		Q_ASSERT(0);
	}
#endif

	updateVertexArray();
}

void OctahedronPolygon::inPlaceUnion(const OctahedronPolygon& mpoly)
{
	const bool intersect = intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD);
	append(mpoly);
	if (intersect)
		tesselate(WindingPositive);
	updateVertexArray();
}

void OctahedronPolygon::inPlaceSubtraction(const OctahedronPolygon& mpoly)
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return;
	appendReversed(mpoly);
	tesselate(WindingPositive);
	updateVertexArray();
}

bool OctahedronPolygon::intersects(const OctahedronPolygon& mpoly) const
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return false;
	OctahedronPolygon resOct(*this);
	resOct.inPlaceIntersection(mpoly);
	return !resOct.isEmpty();
}

bool OctahedronPolygon::contains(const OctahedronPolygon& mpoly) const
{
	if (!containsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return false;
	OctahedronPolygon resOct(*this);
	resOct.inPlaceUnion(mpoly);
	return resOct.getArea()-getArea()<0.00000000001;
}

bool OctahedronPolygon::sideContains2D(const Vec3d& p, int sideNb) const
{
	foreach (const SubContour& subContour, sides[sideNb])
	{
		// There should be only triangles here.
		Q_ASSERT(subContour.size()==3);
		if ((sideNb%2==1 ?
			triangleContains2D(subContour.at(0).vertex, subContour.at(1).vertex, subContour.at(2).vertex, p) :
			triangleContains2D(subContour.at(2).vertex, subContour.at(1).vertex, subContour.at(0).vertex, p)))
			return true;
	}
	return false;
}

bool OctahedronPolygon::contains(const Vec3d& p) const
{
	const int fNb = getSideNumber(p);
	Vec3d p2(p);
	p2 *= 1./(sideDirections[fNb]*p);
	return sideContains2D(p2, fNb);
}

bool OctahedronPolygon::isEmpty() const
{
	return sides[0].isEmpty() && sides[1].isEmpty() && sides[2].isEmpty() && sides[3].isEmpty() &&
			sides[4].isEmpty() && sides[5].isEmpty() && sides[6].isEmpty() && sides[7].isEmpty();
}


void OctahedronPolygon::splitContourByPlan(int onLine, const SubContour& inputContour, QVector<SubContour> result[2])
{
	SubContour currentSubContour;
	SubContour unfinishedSubContour;
	int previousQuadrant=getSide(inputContour.first().vertex, onLine);
	int currentQuadrant=0;
	Vec3d tmpVertex;
	EdgeVertex previousVertex=inputContour.first();
	EdgeVertex currentVertex;
	int i;
	bool ok=true;
	const Vec3d plan(onLine==0?1:0, onLine==1?1:0, onLine==2?1:0);
	// Take care first of the unfinished contour
	for (i=0;i<inputContour.size();++i)
	{
		currentVertex = inputContour.at(i);
		currentQuadrant = getSide(currentVertex.vertex, onLine);
		if (currentQuadrant==previousQuadrant)
		{
			unfinishedSubContour << currentVertex;
		}
		else
		{
			Q_ASSERT(currentSubContour.isEmpty());
			// We crossed the line
			tmpVertex = greatCircleIntersection(previousVertex.vertex, currentVertex.vertex, plan, ok);
			if (!ok)
			{
				// There was a problem, probably the 2 vertices are too close, just keep them like that
				// since they are each at a different side of the plan.
			}
			else
			{
				unfinishedSubContour << EdgeVertex(tmpVertex, false); // Last point of the contour, it's not an edge
				currentSubContour << EdgeVertex(tmpVertex, previousVertex.edgeFlag);
			}
			previousQuadrant = currentQuadrant;
			break;
		}
		previousVertex=currentVertex;
	}
	// Now handle the other ones
	for (;i<inputContour.size();++i)
	{
		currentVertex = inputContour.at(i);
		currentQuadrant = getSide(currentVertex.vertex, onLine);
		if (currentQuadrant==previousQuadrant)
		{
			currentSubContour << currentVertex;
		}
		else
		{
			// We crossed the line
			tmpVertex = greatCircleIntersection(previousVertex.vertex, currentVertex.vertex, plan, ok);
			if (!ok)
			{
				// There was a problem, probably the 2 vertices are too close, just keep them like that
				// since they are each at a different side of the plan.
				result[previousQuadrant] << currentSubContour;
				currentSubContour.clear();
				currentSubContour << currentVertex;
			}
			else
			{
				currentSubContour << EdgeVertex(tmpVertex, false); // Last point of the contour, it's not an edge
				result[previousQuadrant] << currentSubContour;
				currentSubContour.clear();
				currentSubContour << EdgeVertex(tmpVertex, previousVertex.edgeFlag);
				currentSubContour << currentVertex;
			}
			previousQuadrant = currentQuadrant;
		}
		previousVertex=currentVertex;
	}

	// Handle the last line between the last and first point
	previousQuadrant = currentQuadrant;
	currentQuadrant = getSide(inputContour.first().vertex, onLine);
	if (currentQuadrant==previousQuadrant)
	{
	}
	else
	{
		// We crossed the line
		tmpVertex = greatCircleIntersection(previousVertex.vertex, inputContour.first().vertex, plan, ok);
		if (!ok)
		{
			// There was a problem, probably the 2 vertices are too close, just keep them like that
			// since they are each at a different side of the plan.
			result[previousQuadrant] << currentSubContour;
			currentSubContour.clear();
		}
		else
		{
			currentSubContour << EdgeVertex(tmpVertex, false);	// Last point of the contour, it's not an edge
			result[previousQuadrant] << currentSubContour;
			currentSubContour.clear();
			currentSubContour << EdgeVertex(tmpVertex, previousVertex.edgeFlag);
		}
	}

	// Append the last contour made from the last vertices + the previous unfinished ones
	currentSubContour << unfinishedSubContour;
	result[currentQuadrant] << currentSubContour;
}

SubContour::SubContour(const QVector<Vec3d>& vertices, bool closed) : QVector<EdgeVertex>(vertices.size(), EdgeVertex(true))
{
	// Create the contour list by adding the matching edge flags
	for (int i=0;i<vertices.size();++i)
		(*this)[i].vertex = vertices.at(i);
	if (closed==false)
	{
		this->last().edgeFlag=false;
	}
}

SubContour SubContour::reversed() const
{
	SubContour res;
	QVectorIterator<EdgeVertex> iter(*this);
	iter.toBack();
	while (iter.hasPrevious())
		res.append(iter.previous());
	return res;
}

void OctahedronPolygon::computeBoundingCap()
{
	Vec3d p1(1,0,0), p2(1,0,0);
	double maxDist=1.;
	const QVector<Vec3d>& trianglesArray = getFillVertexArray().vertex;
	foreach (const Vec3d& v1, trianglesArray)
	{
		foreach (const Vec3d& v2, trianglesArray)
		{
			if (v1*v2<maxDist)
			{
				p1 = v1;
				p2 = v2;
				maxDist = v1*v2;
			}
		}
	}
	p2+=p1;
	p2.normalize();
	capN = p2;
	capD = p2*p1;
}

/////////////////////////////////////////////////////////////////////////////////
//// Methods for SphericalTexturedPolygon
/////////////////////////////////////////////////////////////////////////////////
//void  vertexTextureCallback(void* vertexData, void* userData)
//{
//	SphericalTexturedPolygon* mp = static_cast<SphericalTexturedPolygon*>(((GluTessCallbackData*)userData)->thisPolygon);
//	const SphericalTexturedPolygon::TextureVertex* vData = (SphericalTexturedPolygon::TextureVertex*)vertexData;
//	mp->triangleVertices.append(vData->vertex);
//	mp->textureCoords.append(vData->texCoord);
//	mp->edgeFlags.append(((GluTessCallbackData*)userData)->edgeFlag);
//}
//
//void SphericalTexturedPolygon::setContours(const QVector<QVector<TextureVertex> >& contours, SphericalPolygon::PolyWindingRule windingRule)
//{
//	triangleVertices.clear();
//	edgeFlags.clear();
//	textureCoords.clear();
//
//	// Use GLUES tesselation functions to transform the polygon into a list of triangles
//	GLUEStesselator* tess = gluesNewTess();
//	gluesTessCallback(tess, GLUES_TESS_VERTEX_DATA, (GLvoid(*)()) &vertexTextureCallback);
//	gluesTessCallback(tess, GLUES_TESS_EDGE_FLAG_DATA, (GLvoid(*)()) &edgeFlagCallback);
//	gluesTessCallback(tess, GLUES_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
//		const double windRule = (windingRule==SphericalPolygon::WindingPositive) ? GLUES_TESS_WINDING_POSITIVE : GLUES_TESS_WINDING_ABS_GEQ_TWO;
//	gluesTessProperty(tess, GLUES_TESS_WINDING_RULE, windRule);
//	GluTessCallbackData data;
//	data.thisPolygon=this;
//	gluesTessBeginPolygon(tess, &data);
//	for (int c=0;c<contours.size();++c)
//	{
//		gluesTessBeginContour(tess);
//		for (int i=0;i<contours[c].size();++i)
//		{
//						gluesTessVertex(tess, const_cast<double*>((const double*)contours[c][i].vertex), const_cast<void*>((const void*)&(contours[c][i])));
//		}
//		gluesTessEndContour(tess);
//	}
//	gluesTessEndPolygon(tess);
//	gluesDeleteTess(tess);
//
//	// There should always be a texture coord matching each vertex.
//	Q_ASSERT(triangleVertices.size() == edgeFlags.size());
//	Q_ASSERT(triangleVertices.size() == textureCoords.size());
//
//	// There should always be an edge flag matching each vertex.
//	Q_ASSERT(triangleVertices.size() == edgeFlags.size());
//#ifndef NDEBUG
//	// Check that all vectors are normalized
//	foreach (const Vec3d& v, triangleVertices)
//		Q_ASSERT(std::fabs(v.lengthSquared()-1.)<0.000001);
//	// Check that the orientation of all the triangles is positive
//	for (int i=0;i<triangleVertices.size()/3;++i)
//	{
//		Q_ASSERT((triangleVertices.at(i*3+1)^triangleVertices.at(i*3))*triangleVertices.at(i*3+2)>=0);
//	}
//#endif
//}

const OctahedronPolygon& OctahedronPolygon::getAllSkyOctahedronPolygon()
{
	static OctahedronPolygon poly;
	//QVector<SubContour> side0;
	//side0.append();
	//poly.sides[0]=
	Q_ASSERT(0); // Unimplemented
	return poly;
}

QDataStream& operator<<(QDataStream& out, const OctahedronPolygon& p)
{
	for (int i=0;i<8;++i)
	{
		out << p.sides[i];
	}
	return out;
}

QDataStream& operator>>(QDataStream& in, OctahedronPolygon& p)
{
	for (int i=0;i<8;++i)
	{
		in >> p.sides[i];
	}
	p.updateVertexArray();
	Q_ASSERT(p.checkAllTrianglesPositive());
	return in;
}
