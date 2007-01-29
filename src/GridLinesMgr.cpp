
#include <set>
#include "GridLinesMgr.hpp"

#include "StelApp.hpp"
#include "Navigator.hpp"
#include "Translator.hpp"
#include "Projector.hpp"
#include "LoadingBar.hpp"
#include "Fader.hpp"
#include "Planet.hpp"

// Class which manages a grid to display in the sky
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(Projector::FRAME_TYPE frame = Projector::FRAME_EARTH_EQU, unsigned int _nb_meridian = 24, unsigned int _nb_parallel = 18);
    virtual ~SkyGrid();
	void draw(const Projector* prj) const;
	void setFontSize(double newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void set_top_transparancy(bool b) { transparent_top= b; }
private:
	unsigned int nb_meridian;
	unsigned int nb_parallel;
	bool transparent_top;
	Vec3f color;
	Projector::FRAME_TYPE frameType;
	double fontSize;
	SFont& font;
	LinearFader fader;
};


// Class which manages a line to display around the sky like the ecliptic line
class SkyLine
{
public:
	enum SKY_LINE_TYPE
	{
		EQUATOR,
		ECLIPTIC,
		LOCAL,
		MERIDIAN
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR, double _radius = 1., unsigned int _nb_segment = 48);
    virtual ~SkyLine();
	void draw(Projector *prj,const Navigator *nav) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void setFontSize(double newSize);
private:
	double radius;
	unsigned int nb_segment;
	SKY_LINE_TYPE line_type;
	Vec3f color;
	Vec3f* points;
	Projector::FRAME_TYPE frameType;
	LinearFader fader;
	double fontSize;
	SFont& font;
};


// rms added color as parameter
SkyGrid::SkyGrid(Projector::FRAME_TYPE frame, unsigned int _nb_meridian, unsigned int _nb_parallel) :
	nb_meridian(_nb_meridian), nb_parallel(_nb_parallel), color(0.2,0.2,0.2), frameType(frame), fontSize(12),
	font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	transparent_top = true;
}

SkyGrid::~SkyGrid()
{
}

void SkyGrid::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

// Conversion into mas = milli arcsecond
const static double RADIAN_MAS = 180./M_PI*1000.*60.*60.;
const static double RADIAN_DEG = 180./M_PI;
const static double DEGREE_MAS = 1000.*60.*60.;
const static double ARCMIN_MAS = 1000.*60;
const static double ARCSEC_MAS = 1000.;

//! Return the standard longitude in radian [-pi;+pi] for a position given in the viewport
static double getLonFrom2dPos(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	return std::atan2(v[1],v[0]);
}

//! Return the standard latitude in radian [-pi/2;+pi/2] for a position given in the viewport
static double getLatFrom2dPos(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	return std::asin(v[2]);
}


void rectToSpheLat180(double& lon, double& lat, const Vec3d& v)
{
	StelUtils::rect_to_sphe(&lon, &lat, v);
	// lon is now between -pi and pi, we want it between 0 and pi, like a latitude
	// lat is now between -pi/2 and pi/2, we want it between 0 and 2*pi like a longitude
	lat += M_PI/2;
	if (lon<0.)
	{
		lat = 2.*M_PI-lat;
		lon = -lon;
	}
	assert(lat>=0. && lat<=2.*M_PI);
	assert(lon>=0. && lon<=M_PI);
}

void spheToRectLat180(double lon, double lat, Vec3d& v)
{
	assert(lat>=0. && lat<=2.*M_PI);
	assert(lon>=0. && lon<=M_PI);
	if (lat>M_PI)
	{
		lat = 2.*M_PI-lat;
		lon = -lon;
	}
	lat -= M_PI/2;
	StelUtils::sphe_to_rect(lon, lat, v);
}

void spheToRectLat1802(double lon, double lat, Vec3d& v)
{
	assert(lat>=0. && lat<=2.*M_PI);
	assert(lon>=0. && lon<=M_PI);
	if (lat>M_PI)
	{
		lat = 2.*M_PI-lat;
		lon += M_PI;
	}
	lat -= M_PI/2;
	StelUtils::sphe_to_rect(lon, lat, v);
}

//! Return a special latitude in radian [0;2*pi] for a position given in the viewport
static double getLatFrom2dPos180(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	double lon, lat;
	rectToSpheLat180(lon, lat, v);
	return lat;
}

//! Return a special longitude in radian [0;pi] for a position given in the viewport
static double getLonFrom2dPos180(const Projector* prj, const Vec2d& p)
{
	Vec3d v;
	prj->unProject(p[0], p[1], v);
	double lon, lat;
	rectToSpheLat180(lon, lat, v);
	return lon;
}


//! Return the 2D position in the viewport from a longitude and latitude in radian
static Vec3d get2dPosFromSpherical(const Projector* prj, double lon, double lat)
{
	Vec3d v;
	Vec3d win;
	StelUtils::sphe_to_rect(lon, lat, v);
	prj->project(v, win);
	return win;
}


//! Return the 2D position in the viewport from special longitude and latitude in radian
static Vec3d get2dPosFromSpherical1802(const Projector* prj, double lon, double lat)
{
	Vec3d v;
	Vec3d win;
	
	spheToRectLat1802(lon, lat, v);

	prj->project(v, win);
	return win;
}

//! Check if the given point from the viewport side is the beginning of a parallel or not
//! Beginning means that the direction of increasing longitude goes inside the viewport 
static bool isParallelEntering(const Projector* prj, const Vec2d& v, double lat)
{
	const double lon = getLonFrom2dPos(prj, v);
	return prj->check_in_viewport(get2dPosFromSpherical(prj, lon+0.001*prj->getFov(), lat));
}

//! Check if the given point from the viewport side is the beginning of a parallel or not
//! Beginning means that the direction of increasing longitude goes inside the viewport 
static bool isParallelEntering(const Projector* prj, double lon, double lat)
{
	return prj->check_in_viewport(get2dPosFromSpherical(prj, lon+0.001*prj->getFov(), lat));
}

//! Check if the given point from the viewport side is the beginning of a meridian or not
//! Beginning means that the direction of increasing latitude goes inside the viewport 
//! @param lon180 Modified longitude in radian
static bool isMeridianEnteringLat180(const Projector* prj, double lon1802, double lat1802)
{
	assert(lat1802>=0. && lat1802<=2.*M_PI);
	assert(lon1802>=0. && lon1802<=M_PI);
	double lat2 = lat1802+0.001*prj->getFov();
	if (lat2>2.*M_PI)
		lat2-=2.*M_PI;
	return prj->check_in_viewport(get2dPosFromSpherical1802(prj, lon1802, lat2));
}

////! Check if the given point from the viewport side is the beginning of a meridian or not
////! Beginning means that the direction of increasing latitude goes inside the viewport 
//static bool isMeridianEnteringLat180(const Projector* prj, const Vec2d& v, double lon180)
//{
//	const double lat180 = getLatFrom2dPos180(prj, v);
//	if (lat180>M_PI)
//		return prj->check_in_viewport(get2dPosFromSpherical180(prj, M_PI-lon180, lat180+0.001*prj->getFov()));
//	else
//		return prj->check_in_viewport(get2dPosFromSpherical180(prj, lon180, lat180+0.001*prj->getFov()));
//}

//! Return all the points p on the segment [p0 p1] for which the value of func(p) == k*stepMas
//! with a precision < 0.5 pixels
//! For each value of k*stepMas, the result is then sorted ordered according to the value of func2(p)
static void getPs(map<int, map<double, Vec2d> > & result, const Projector* prj, 
	const Vec2d& p0, const Vec2d& p1, double step, 
	double (*func)(const Projector* prj, const Vec2d& p),
	double (*func2)(const Projector* prj, const Vec2d& p))
{
	const Vec2d deltaP(p1-p0);
	Vec2d p = p0;
	const Vec2d dPix1 = deltaP/(deltaP.length());	// 1 pixel step
	const Vec2d dPixPrec = deltaP/(deltaP.length()*2.);	// 0.5 pixel step
	double funcp, funcpDpix, target, deriv, u=0.;
	
	funcp = func(prj, p);
	funcpDpix = func(prj, p+dPixPrec);
	deriv = (funcpDpix-funcp)/0.5;
	target = step*(std::floor(funcp/step) + (deriv>0 ? 1:0));
	bool sureThatTargetExist = false;
	while (u<deltaP.length())
	{
		// Find next point
		if ((funcpDpix>=target && funcp<target) || (funcpDpix<=target && funcp>target))
		{
			// If more that one target was inside the range [funcp;funcpDpix] add them to the result list
			while ((funcpDpix>=target && funcp<target) || (funcpDpix<=target && funcp>target))
			{
				if (result.find((int)(target*RADIAN_MAS))!=result.end() && result[(int)(target*RADIAN_MAS)].find(func2(prj, p))!=result[(int)(target*DEGREE_MAS)].end())
					cerr << "Err" << endl;
				result[(int)(target*RADIAN_MAS)][func2(prj, p)]=p;
				target+=(deriv>0) ? step:-step;
			}
		
			p = p+dPixPrec;
			u+=0.5;
			funcp = funcpDpix;
			funcpDpix = func(prj, p+dPixPrec);
			deriv = (funcpDpix-funcp)/0.5;
			target = step*(std::floor(funcp/step) + (deriv>0 ? 1:0));
			sureThatTargetExist = false;
		}
		else
		{
			if ((deriv>0 && funcp>target) || (deriv<0 && funcp<target))
				sureThatTargetExist = true;	// We went too "far", thus we know that the target exists
				
			deriv = (funcpDpix-funcp)/0.5;
			if (sureThatTargetExist==false)
				target = step*(std::floor(funcp/step) + (deriv>0 ? 1:0));
			double dU = (target-funcp)/deriv;
			// TODO handle this properly, maybe using 2nd derivatives?
			if (fabs(dU)<0.05)
				dU = 0.05*(dU/fabs(dU));
			if (dU>100.)
				dU = 100.;
			if (dU<-100.)
				dU = -100;
			u += dU;
			p += dPix1*dU;
			funcp = func(prj, p);
			funcpDpix = func(prj, p+dPixPrec);
		}
	}
}


//! Return all the points p on the segment [p0 p1] for which the value of func(p) == k*stepMas
//! with a precision < 0.5 pixels
//! For each value of k*stepMas, the result is then sorted ordered according to the value of func2(p)
static void getPslow(map<int, set<double> > & result, const Projector* prj, 
	const Vec2d& p0, const Vec2d& p1, double step, 
	double (*func)(const Projector* prj, const Vec2d& p),
	double (*func2)(const Projector* prj, const Vec2d& p))
{
	double precision = 5;
	const Vec2d deltaP(p1-p0);
	Vec2d p = p0;
	const Vec2d dPix1 = deltaP/(deltaP.length());	// 1 pixel step
	double funcp, funcpDpix;
	
	funcp = func(prj, p);
	funcpDpix = func(prj, p+dPix1*precision);
		
	double u=0;
	do
	{	
		if (funcp<funcpDpix)
		{
			// If targets are included inside the range, add them
			const double r1 = step*(std::floor(funcp/step));
			const double r2 = step*(std::ceil(funcpDpix/step));
			
			for (double v=r1;v<r2;v+=step)
				if (funcp<=v && funcpDpix>v)
					result[(int)(v*RADIAN_MAS)].insert(func2(prj, p-dPix1*(precision*0.5)));
		}
		else
		{
			// If targets are included inside the range, add them
			const double r1 = step*(std::ceil(funcp/step));
			const double r2 = step*(std::floor(funcpDpix/step));
			
			for (double v=r2;v<r1;v+=step)
				if (funcp>=v && funcpDpix<v)
					result[(int)(v*RADIAN_MAS)].insert(func2(prj, p-dPix1*(precision*0.5)));
		}
		
		precision = step/ (fabs(funcpDpix-funcp)/precision) * 0.5;
		if (precision>2)
			precision = 2.;
		else if (precision<0.1)
		{
			precision = 0.1;
		}
		u+=precision;
		p+=dPix1*precision;
		funcp = funcpDpix;
		funcpDpix = func(prj,p);
	}
	while(u<deltaP.length());
}

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {1., 10., 60., 600., 3600., 3600.*5., 3600.*10.};
static const double STEP_SIZES_HMS[] = {1., 10., 60., 600., 3600., 3600.*2.5, 3600.*15.};

static double getClosestResolutionParallel(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<7;++i)
		if (STEP_SIZES_DMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_DMS[i]/3600.;
		}
	return 10.;
}

static double getClosestResolutionMeridian(double pixelPerRad)
{
	double minResolution = 50.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<6;++i)
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_HMS[i]/3600.;
		}
	return 15.;
}

void SkyGrid::draw(const Projector* prj) const
{
	if (!fader.getInterstate()) return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	glColor4f(color[0],color[1],color[2], fader.getInterstate());

	prj->setCurrentFrame(frameType);	// set 2D coordinate

	// Check whether the pole are in the viewport
	bool northPoleInViewport = false;
	bool southPoleInViewport = false;
	Vec3d win;
	prj->project(Vec3d(0,0,1), win);
	if (prj->check_in_viewport(win))
		northPoleInViewport = true;
	prj->project(Vec3d(0,0,-1), win);
	if (prj->check_in_viewport(win))
		southPoleInViewport = true;


	// Get the longitude and latitude resolution at the center of the viewport
	Vec3d centerV;
	double lon0, lat0, lon1, lat1, lon2, lat2;
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2, centerV);
	StelUtils::rect_to_sphe(&lon0, &lat0, centerV);
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2+1, prj->getViewportPosY()+prj->getViewportHeight()/2, centerV);
	StelUtils::rect_to_sphe(&lon1, &lat1, centerV);
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2+1, centerV);
	StelUtils::rect_to_sphe(&lon2, &lat2, centerV);
	
	const double gridStepParallelRad = M_PI/180.*getClosestResolutionParallel(1./std::sqrt((lat1-lat0)*(lat1-lat0)+(lat2-lat0)*(lat2-lat0)));
	const double gridStepMeridianRad = M_PI/180.* ((northPoleInViewport || southPoleInViewport) ? 15. : getClosestResolutionMeridian(1./std::sqrt((lon1-lon0)*(lon1-lon0)+(lon2-lon0)*(lon2-lon0))));
	
	map<int, set<double> > resultsParallels;
	map<int, set<double> > resultsMeridians;
	const vector<Vec2d> viewportVertices = prj->getViewportVertices();
	for (unsigned int i=0;i<viewportVertices.size();++i)
	{
		// The segment of the viewport is between v0 and v1
		Vec2d vertex0 = viewportVertices[i];
		Vec2d vertex1 = viewportVertices[(i+1)%viewportVertices.size()];
		getPslow(resultsParallels, prj, vertex0, vertex1, gridStepParallelRad, getLatFrom2dPos, getLonFrom2dPos);
		getPslow(resultsMeridians, prj, vertex0, vertex1, gridStepMeridianRad, getLonFrom2dPos180, getLatFrom2dPos180);
	}

	// Draw the parallels
	for (map<int, set<double> >::const_iterator iter=resultsParallels.begin(); iter!=resultsParallels.end(); ++iter)
	{
		if (iter->second.size()%2!=0)
		{
			cerr << "Error parallel "<< (double)iter->first/DEGREE_MAS << " " << iter->second.size() << endl;
		}
		else
		{
			set<double>::const_iterator ii = iter->second.begin();
			if (!isParallelEntering(prj, *iter->second.begin(), (double)iter->first/RADIAN_MAS))
			{
				++ii;
			}
			
			Vec3d vv;
			double size;
			for (unsigned int i=0;i<iter->second.size()/2;++i)
			{
				double lon = *ii;
				StelUtils::sphe_to_rect(lon, (double)iter->first/RADIAN_MAS, vv);
				++ii;
				if (ii==iter->second.end())
					size = *iter->second.begin() - lon;
				else
					size = *ii - lon;
				if (size<0) size+=2.*M_PI;
				prj->drawParallel(vv, size, true, &font);
				++ii;
			}
		}
	}
	
	// Draw the parallels which didn't intersect the viewport but are in the screen
	// This can only happen for parallels around the poles fully included in the viewport (At least I hope!)
	if (northPoleInViewport)
	{
		const double lastLat = (double)(--resultsParallels.end())->first/RADIAN_MAS;
		for (double lat=lastLat+gridStepParallelRad;lat<M_PI/2-0.00001;lat+=gridStepParallelRad)
		{
			Vec3d vv(std::cos(lat), 0, std::sin(lat));
			prj->drawParallel(vv, 2.*M_PI);
		}
	}
	if (southPoleInViewport)
	{
		const double lastLat = (double)resultsParallels.begin()->first/RADIAN_MAS;
		for (double lat=lastLat-gridStepParallelRad;lat>-M_PI/2+0.00001;lat-=gridStepParallelRad)
		{
			Vec3d vv(std::cos(lat), 0, std::sin(lat));
			prj->drawParallel(vv, 2.*M_PI);
		}
	}
	
	// Draw meridians
	
	// Discriminate meridian categories, if latitude is > pi, the real longitude180 is -longitude+pi 
	map<int, set<double> > resultsMeridiansOrdered;
	for (map<int, set<double> >::const_iterator iter=resultsMeridians.begin(); iter!=resultsMeridians.end(); ++iter)
	{
		for (set<double>::const_iterator k=iter->second.begin();k!=iter->second.end();++k)
		{
			assert(*k>=0. && *k<=2.*M_PI);
			assert((double)iter->first/RADIAN_MAS>=0. && (double)iter->first/RADIAN_MAS<=M_PI);
			if (*k>M_PI)
				resultsMeridiansOrdered[(int)(10*std::floor((M_PI*RADIAN_MAS-iter->first+5)/10))].insert(*k);
			else
				resultsMeridiansOrdered[(int)(10*std::floor((iter->first+5)/10))].insert(*k);
		}
	}
	
	for (map<int, set<double> >::const_iterator iter=resultsMeridiansOrdered.begin(); iter!=resultsMeridiansOrdered.end(); ++iter)
	{
//		cerr << "------- lon1802=" << iter->first << "--------" << endl;
//		for (map<double, Vec2d>::const_iterator k = iter->second.begin();k!=iter->second.end();++k)
//		{
//			Vec3d v;
//			prj->unProject(k->second[0], k->second[1], v);
//			double llon, llat;
//			rectToSpheLat180(llon, llat, v);
//			cerr << k->second << " lat=" << k->first*180./M_PI << " Llon="<< llon*180./M_PI << " Llat=" << llat*180./M_PI << (isMeridianEnteringLat180(prj, (double)iter->first/RADIAN_MAS, k->first) ?" *":"") << endl;
//		}
		
		if (iter->second.size()%2!=0)
		{
			cerr << "Error meridian " << (double)iter->first/DEGREE_MAS << " " << iter->second.size() << endl;
		}
		else
		{
			set<double>::const_iterator k = iter->second.begin();
			if (!isMeridianEnteringLat180(prj, (double)iter->first/RADIAN_MAS, *k))
			{
				++k;
			}
			
			Vec3d vv;
			double size;
			for (unsigned int i=0;i<iter->second.size()/2;++i)
			{
				double lat180 = *k;
				spheToRectLat1802((double)iter->first/RADIAN_MAS, lat180, vv);
				++k;
				if (k==iter->second.end())
					size = *iter->second.begin() - lat180;
				else
					size = *k - lat180;
				if (size<0.) size+=2.*M_PI;
				prj->drawMeridian(vv, size, true, &font);
				++k;
			}
		}
	
		// Debug, draw a cross for all the points
//		for (map<double, Vec2d>::const_iterator k=iter->second.begin();k!=iter->second.end();++k)
//		{
//			//const double lon180 = k->first>M_PI? (double)iter->first/RADIAN_MAS-M_PI : (double)iter->first/RADIAN_MAS;
//			if (isMeridianEnteringLat180(prj, (double)iter->first/RADIAN_MAS, k->first))
//			{
//				glColor3f(1,1,0);
//			}
//			else
//			{
//				glColor3f(0,0,1);
//			}
//			Vec3d vv, win;
//			spheToRectLat1802((double)iter->first/RADIAN_MAS, k->first, vv);
//			prj->project(vv, win);
//			glBegin(GL_LINES);
//				glVertex2f(win[0]-30,win[1]);
//				glVertex2f(win[0]+30,win[1]);
//				glVertex2f(win[0],win[1]-30);
//				glVertex2f(win[0],win[1]+30);
//			glEnd();
//		}
	}
	
	// Draw meridian zero which can't be found by the normal algo..
	Vec3d vv(1,0,0);
	prj->drawMeridian(vv, 2.*M_PI, true, &font);
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type, double _radius, unsigned int _nb_segment) :
		radius(_radius), nb_segment(_nb_segment), color(0.f, 0.f, 1.f), fontSize(1.),
font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize))
{
	float inclinaison = 0.f;
	line_type = _line_type;

	switch (line_type)
		{
		case LOCAL : frameType = Projector::FRAME_LOCAL; break;
		case MERIDIAN : frameType = Projector::FRAME_LOCAL;
			inclinaison = 90; break;
		case ECLIPTIC : frameType = Projector::FRAME_J2000;
			inclinaison = 23.4392803055555555556; break;
		case EQUATOR : frameType = Projector::FRAME_EARTH_EQU; break;
		default : frameType = Projector::FRAME_EARTH_EQU;
	}

	Mat4f r = Mat4f::xrotation(inclinaison*M_PI/180.f);

	// Ecliptic month labels need to be redone
	// correct for month labels
	// TODO: can make this more accurate
	//	if(line_type == ECLIPTIC ) r = r * Mat4f::zrotation(-77.9*M_PI/180.);

	// Points to draw along the circle
	points = new Vec3f[nb_segment+1];
	for (unsigned int i=0;i<nb_segment+1;++i)
	{
		StelUtils::sphe_to_rect((float)i/(nb_segment)*2.f*M_PI, 0.f, points[i]);
		points[i] *= radius;
		points[i].transfo4d(r);
	}
}

SkyLine::~SkyLine()
{
	delete [] points;
	points = NULL;
}

void SkyLine::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize);
}

void SkyLine::draw(Projector *prj,const Navigator *nav) const
{
	if (!fader.getInterstate()) return;

	Vec3d pt1;
	Vec3d pt2;

	glColor4f(color[0], color[1], color[2], fader.getInterstate());
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

  if (line_type == ECLIPTIC) {
      // special drawing of the ecliptic line
    const Mat4d m = nav->getHomePlanet()->getRotEquatorialToVsop87().transpose();
    const bool draw_labels = nav->getHomePlanet()->getEnglishName()=="Earth";
       // start labeling from the vernal equinox
    const double corr = draw_labels ? (atan2(m.r[4],m.r[0]) - 3*M_PI/6) : 0.0;
    Vec3d point(radius*cos(corr),radius*sin(corr),0.0);
    point.transfo4d(m);
    
    prj->setCurrentFrame(Projector::FRAME_EARTH_EQU);
    
    bool prev_on_screen = prj->project(point,pt1);
    for (unsigned int i=1;i<nb_segment+1;++i) {
      const double phi = corr+2*i*M_PI/nb_segment;
      Vec3d point(radius*cos(phi),radius*sin(phi),0.0);
      point.transfo4d(m);
      const bool on_screen = prj->project(point,pt2);
      if (on_screen && prev_on_screen) {
        const double dx = pt2[0]-pt1[0];
        const double dy = pt2[1]-pt1[1];
        const double dq = dx*dx+dy*dy;
        if (dq < 1024*1024) {
		  glBegin (GL_LINES);
		    glVertex2f(pt2[0],pt2[1]);
		    glVertex2f(pt1[0],pt1[1]);
       	  glEnd();
        }
		if (draw_labels && (i+2) % 4 == 0) {

			const double d = sqrt(dq);

			double angle = acos((pt1[1]-pt2[1])/d);
			if( pt1[0] < pt2[0] ) {
				angle *= -1;
			}

			// draw text label
			std::ostringstream oss;	

			oss << (i+3)/4;

			glPushMatrix();
			glTranslatef(pt2[0],pt2[1],0);
			glRotatef(-90+angle*180./M_PI,0,0,-1);

			glEnable(GL_TEXTURE_2D);

			font.print(0,-2,oss.str());
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);

		}
      }
      prev_on_screen = on_screen;
      pt1 = pt2;
    }
  } else {

	prj->setCurrentFrame(frameType);
	for (unsigned int i=0;i<nb_segment;++i)
	{
		if (prj->project(points[i], pt1) && prj->project(points[i+1], pt2))
		{
          const double dx = pt1[0]-pt2[0];
          const double dy = pt1[1]-pt2[1];
          const double dq = dx*dx+dy*dy;
          if (dq < 1024*1024) {

			double angle;

			// TODO: allow for other numbers of meridians and parallels without
			// screwing up labels?

			glBegin (GL_LINES);
				glVertex2f(pt1[0],pt1[1]);
				glVertex2f(pt2[0],pt2[1]);
       		glEnd();


			if(line_type == MERIDIAN) {
				const double d = sqrt(dq);
				  
				angle = acos((pt1[1]-pt2[1])/d);
				if( pt1[0] < pt2[0] ) {
					angle *= -1;
				}

				// draw text label
				std::ostringstream oss;	
				
				if(i<=8) oss << (i+1)*10;
				else if(i<=16) {
					oss << (17-i)*10;
					angle += M_PI;
				}
				else oss << "";
				
				glPushMatrix();
				glTranslatef(pt2[0],pt2[1],0);
				glRotatef(180+angle*180./M_PI,0,0,-1);
				
				glBegin (GL_LINES);
				glVertex2f(-3,0);
				glVertex2f(3,0);
				glEnd();
				glEnable(GL_TEXTURE_2D);

				font.print(2,-2,oss.str());
				glPopMatrix();
				glDisable(GL_TEXTURE_2D);

			}

				  
			if(line_type == EQUATOR && (i+1) % 2 == 0) {

				const double d = sqrt(dq);
				  
				angle = acos((pt1[1]-pt2[1])/d);
				if( pt1[0] < pt2[0] ) {
					angle *= -1;
				}

				// draw text label
				std::ostringstream oss;	

				if((i+1)/2 == 24) oss << "0h";
				else oss << (i+1)/2 << "h";

				glPushMatrix();
				glTranslatef(pt2[0],pt2[1],0);
				glRotatef(180+angle*180./M_PI,0,0,-1);
				
				glBegin (GL_LINES);
				glVertex2f(-3,0);
				glVertex2f(3,0);
				glEnd();
				glEnable(GL_TEXTURE_2D);

				font.print(2,-2,oss.str());
				glPopMatrix();
				glDisable(GL_TEXTURE_2D);

			}

		  }

		}
	}
  }
}


GridLinesMgr::GridLinesMgr()
{
	dependenciesOrder["draw"]="stars";
	equ_grid = new SkyGrid(Projector::FRAME_EARTH_EQU);
	azi_grid = new SkyGrid(Projector::FRAME_LOCAL);
	equator_line = new SkyLine(SkyLine::EQUATOR);
	ecliptic_line = new SkyLine(SkyLine::ECLIPTIC);
	meridian_line = new SkyLine(SkyLine::MERIDIAN, 1, 36);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equ_grid;
	delete azi_grid;
	delete equator_line;
	delete ecliptic_line;
	delete meridian_line;
}

void GridLinesMgr::init(const InitParser& conf, LoadingBar& lb)
{
	setFlagAzimutalGrid(conf.get_boolean("viewing:flag_azimutal_grid"));
	setFlagEquatorGrid(conf.get_boolean("viewing:flag_equatorial_grid"));
	setFlagEquatorLine(conf.get_boolean("viewing:flag_equator_line"));
	setFlagEclipticLine(conf.get_boolean("viewing:flag_ecliptic_line"));
	setFlagMeridianLine(conf.get_boolean("viewing:flag_meridian_line"));
}	
	
void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equ_grid->update(deltaTime);
	azi_grid->update(deltaTime);
	equator_line->update(deltaTime);
	ecliptic_line->update(deltaTime);
	meridian_line->update(deltaTime);
}

double GridLinesMgr::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	// Draw the equatorial grid
	equ_grid->draw(prj);
	// Draw the altazimutal grid
	azi_grid->draw(prj);
	// Draw the celestial equator line
	equator_line->draw(prj,nav);
	// Draw the ecliptic line
	ecliptic_line->draw(prj,nav);
	// Draw the meridian line
	meridian_line->draw(prj,nav);
	return 0.;
}

void GridLinesMgr::setColorScheme(const InitParser& conf, const std::string& section)
{
	// Load colors from config file
	string defaultColor = conf.get_str(section,"default_color");
	setColorEquatorGrid(StelUtils::str_to_vec3f(conf.get_str(section,"equatorial_color", defaultColor)));
	setColorAzimutalGrid(StelUtils::str_to_vec3f(conf.get_str(section,"azimuthal_color", defaultColor)));
	setColorEquatorLine(StelUtils::str_to_vec3f(conf.get_str(section,"equator_color", defaultColor)));
	setColorEclipticLine(StelUtils::str_to_vec3f(conf.get_str(section,"ecliptic_color", defaultColor)));
	setColorMeridianLine(StelUtils::str_to_vec3f(conf.get_str(section,"meridian_color", defaultColor)));
}

//! Set flag for displaying Azimutal Grid
void GridLinesMgr::setFlagAzimutalGrid(bool b) {azi_grid->setFlagshow(b);}
//! Get flag for displaying Azimutal Grid
bool GridLinesMgr::getFlagAzimutalGrid(void) const {return azi_grid->getFlagshow();}
Vec3f GridLinesMgr::getColorAzimutalGrid(void) const {return azi_grid->getColor();}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setFlagEquatorGrid(bool b) {equ_grid->setFlagshow(b);}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::getFlagEquatorGrid(void) const {return equ_grid->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorGrid(void) const {return equ_grid->getColor();}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setFlagEquatorLine(bool b) {equator_line->setFlagshow(b);}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::getFlagEquatorLine(void) const {return equator_line->getFlagshow();}
Vec3f GridLinesMgr::getColorEquatorLine(void) const {return equator_line->getColor();}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setFlagEclipticLine(bool b) {ecliptic_line->setFlagshow(b);}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::getFlagEclipticLine(void) const {return ecliptic_line->getFlagshow();}
Vec3f GridLinesMgr::getColorEclipticLine(void) const {return ecliptic_line->getColor();}


//! Set flag for displaying Meridian Line
void GridLinesMgr::setFlagMeridianLine(bool b) {meridian_line->setFlagshow(b);}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::getFlagMeridianLine(void) const {return meridian_line->getFlagshow();}
Vec3f GridLinesMgr::getColorMeridianLine(void) const {return meridian_line->getColor();}

void GridLinesMgr::setColorAzimutalGrid(const Vec3f& v) { azi_grid->setColor(v);}
void GridLinesMgr::setColorEquatorGrid(const Vec3f& v) { equ_grid->setColor(v);}
void GridLinesMgr::setColorEquatorLine(const Vec3f& v) { equator_line->setColor(v);}
void GridLinesMgr::setColorEclipticLine(const Vec3f& v) { ecliptic_line->setColor(v);}
void GridLinesMgr::setColorMeridianLine(const Vec3f& v) { meridian_line->setColor(v);}
