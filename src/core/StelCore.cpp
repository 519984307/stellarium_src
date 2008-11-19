/*
 * Copyright (C) 2003 Fabien Chereau
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

#include "StelCore.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "MappingClasses.hpp"
#include "ToneReproducer.hpp"
#include "SkyDrawer.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "GeodesicGrid.hpp"
#include "StarMgr.hpp"
#include "SolarSystem.hpp"
#include "MovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "Planet.hpp"
#include "StelPainter.hpp"

#include <QSettings>
#include <QDebug>
#include <QMetaEnum>

/*************************************************************************
 Constructor
*************************************************************************/
StelCore::StelCore() : currentProjectionType(ProjectionStereographic)
{
	toneConverter = new ToneReproducer();
}


/*************************************************************************
 Destructor
*************************************************************************/
StelCore::~StelCore()
{
	delete navigation; navigation=NULL;
	delete toneConverter; toneConverter=NULL;
	delete geodesicGrid; geodesicGrid=NULL;
	delete skyDrawer; skyDrawer=NULL;
}

/*************************************************************************
 Load core data and initialize with default values
*************************************************************************/
void StelCore::init()
{
	StelPainter::initSystemGLInfo();
	
	QSettings* conf = StelApp::getInstance().getSettings();
	
	// Navigator
	navigation = new Navigator();
	navigation->init();
	
	movementMgr = new MovementMgr(this);
	movementMgr->init();
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);	
	
	QString tmpstr = conf->value("projection/type", "stereographic").toString();
	setCurrentProjectionTypeKey(tmpstr);
	
// 	double overwrite_max_fov = conf->value("projection/equal_area_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 360.0)
// 		overwrite_max_fov = 360.0;
// 	if (overwrite_max_fov > 0.0)
// 		MappingEqualArea::getMapping()->maxFov = overwrite_max_fov;
// 	overwrite_max_fov = conf->value("projection/stereographic_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 359.999999)
// 		overwrite_max_fov = 359.999999;
// 	if (overwrite_max_fov > 0.0)
// 		MappingStereographic::getMapping()->maxFov = overwrite_max_fov;
// 	overwrite_max_fov = conf->value("projection/fisheye_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 360.0)
// 		overwrite_max_fov = 360.0;
// 	if (overwrite_max_fov > 0.0)
// 		MappingFisheye::getMapping()->maxFov = overwrite_max_fov;
// 	overwrite_max_fov = conf->value("projection/cylinder_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 540.0)
// 		overwrite_max_fov = 540.0;
// 	if (overwrite_max_fov > 0.0)
// 		MappingCylinder::getMapping()->maxFov = overwrite_max_fov;
// 	overwrite_max_fov = conf->value("projection/perspective_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 179.999999)
// 		overwrite_max_fov = 179.999999;
// 	if (overwrite_max_fov > 0.0)
// 		MappingPerspective::getMapping()->maxFov = overwrite_max_fov;
// 	overwrite_max_fov = conf->value("projection/orthographic_max_fov",0.0).toDouble();
// 	if (overwrite_max_fov > 180.0)
// 		overwrite_max_fov = 180.0;
// 	if (overwrite_max_fov > 0.0)
// 		MappingOrthographic::getMapping()->maxFov = overwrite_max_fov;
	
	// Create and initialize the default projector params
	tmpstr = conf->value("projection/viewport").toString();
	currentProjectorParams.maskType = Projector::stringToMaskType(tmpstr);
	const int viewport_width = conf->value("projection/viewport_width", currentProjectorParams.viewportXywh[2]).toInt();
	const int viewport_height = conf->value("projection/viewport_height", currentProjectorParams.viewportXywh[3]).toInt();
	const int viewport_x = conf->value("projection/viewport_x", 0).toInt();
	const int viewport_y = conf->value("projection/viewport_y", 0).toInt();
	currentProjectorParams.viewportXywh.set(viewport_x,viewport_y,viewport_width,viewport_height);
	
	const double viewportCenterX = conf->value("projection/viewport_center_x",0.5*viewport_width).toDouble();
	const double viewportCenterY = conf->value("projection/viewport_center_y",0.5*viewport_height).toDouble();
	currentProjectorParams.viewportCenter.set(viewportCenterX, viewportCenterY);
	currentProjectorParams.viewportFovDiameter = conf->value("projection/viewport_fov_diameter", qMin(viewport_width,viewport_height)).toDouble();
	currentProjectorParams.fov = movementMgr->getInitFov();
	
	currentProjectorParams.flipHorz = conf->value("projection/flip_horz",false).toBool();
	currentProjectorParams.flipVert = conf->value("projection/flip_vert",false).toBool();
	
	currentProjectorParams.gravityLabels = conf->value("viewing/flag_gravity_labels").toBool();
	
	StarMgr* hip_stars = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
	int grid_level = hip_stars->getMaxGridLevel();
	geodesicGrid = new GeodesicGrid(grid_level);
	hip_stars->setGrid(geodesicGrid);
	
	skyDrawer = new SkyDrawer(this);
	skyDrawer->init();
	// Debug
	// Invert colors fragment shader
// 	const QByteArray a("void main(void) {float gray = dot(gl_Color.rgb, vec3(0.299, 0.587, 0.114)); gl_FragColor = vec4(gray * vec3(1.2, 1.0, 0.8), 1.0);}");
// 	GLuint fs;	// Fragment Shader
// 	GLuint sp;	// Shader Program
// 	fs = glCreateShader(GL_FRAGMENT_SHADER);
// 	const char* str = a.constData();
// 	glShaderSource(fs, 1, &str, NULL);
// 	glCompileShader(fs);
// 	printLog(fs);
// 
// 	sp = glCreateProgram();
// 	glAttachShader(sp, fs);
// 	glLinkProgram(sp);
// 	printLog(sp);
// 	glUseProgram(sp);
}

const ProjectorP StelCore::getProjection2d() const
{
	ProjectorP prj(new Mapping2d());
	prj->init(currentProjectorParams);
	return prj;
}

// Get an instance of projector using the current display parameters from Navigation, MovementMgr
// and using the given modelview matrix
const ProjectorP StelCore::getProjection(const Mat4d& modelViewMat, ProjectionType projType) const
{
	if (projType==1000)
		projType = currentProjectionType;
	
	ProjectorP prj;
	switch (projType)
	{
		case ProjectionPerspective:
			prj = ProjectorP(new MappingPerspective(modelViewMat));
			break;
		case ProjectionEqualArea:
			prj = ProjectorP(new MappingEqualArea(modelViewMat));
			break;
		case ProjectionStereographic:
			prj = ProjectorP(new MappingStereographic(modelViewMat));
			break;
		case ProjectionFisheye:
			prj = ProjectorP(new MappingFisheye(modelViewMat));
			break;
		case ProjectionCylinder:
			prj = ProjectorP(new MappingCylinder(modelViewMat));
			break;
		case ProjectionMercator:
			prj = ProjectorP(new MappingMercator(modelViewMat));
			break;
		case ProjectionOrthographic:
			prj = ProjectorP(new MappingOrthographic(modelViewMat));
			break;
		default:
			qWarning() << "Unknown projection type: " << projType << "using ProjectionStereographic instead";
			prj = ProjectorP(new MappingStereographic(modelViewMat));
			Q_ASSERT(0);
	}
	prj->init(currentProjectorParams);
	return prj;
}
		
// Get an instance of projector using the current display parameters from Navigation, MovementMgr
const ProjectorP StelCore::getProjection(FrameType frameType, ProjectionType projType) const
{
	
	switch (frameType)
	{
		case FrameLocal:
			return getProjection(navigation->getAltAzModelViewMat(), projType);
		case FrameHelio:
			return getProjection(navigation->getHeliocentricEclipticModelViewMat(), projType);
		case FrameEquinoxEqu:
			return getProjection(navigation->getEquinoxEquModelViewMat(), projType);
		case FrameJ2000:
			return getProjection(navigation->getJ2000ModelViewMat(), projType);
		default:
			qDebug() << "Unknown reference frame type: " << (int)frameType << ".";
	}
	Q_ASSERT(0);
	return getProjection2d();
}

// Handle the resizing of the window
void StelCore::windowHasBeenResized(int width,int height)
{
	// Maximize display when resized since it invalidates previous options anyway
	currentProjectorParams.viewportXywh.set(0, 0, width, height);
	currentProjectorParams.viewportCenter.set(0.5*width, 0.5*height);
	currentProjectorParams.viewportFovDiameter = qMin(width,height);
}
	
/*************************************************************************
 Update all the objects in function of the time
*************************************************************************/
void StelCore::update(double deltaTime)
{
	// Update the position of observation and time etc...
	navigation->updateTime(deltaTime);

	// Position of sun and all the satellites (ie planets)
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	solsystem->computePositions(navigation->getJDay(), navigation->getHomePlanet()->getHeliocentricEclipticPos());

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	
	// Update direction of vision/Zoom level
	movementMgr->updateMotion(deltaTime);	
	
	currentProjectorParams.fov = movementMgr->getCurrentFov();
	
	skyDrawer->update(deltaTime);
}


/*************************************************************************
 Execute all the pre-drawing functions
*************************************************************************/
void StelCore::preDraw()
{
	// Init openGL viewing with fov, screen size and clip planes
	currentProjectorParams.zNear = 0.000001;
	currentProjectorParams.zFar = 50.;
	
	skyDrawer->preDraw();
	
	// Clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	glClear(GL_COLOR_BUFFER_BIT);
}


/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw()
{
	StelPainter sPainter(getProjection(StelCore::FrameJ2000));
	sPainter.drawViewportShape();
	
// 	// Inverted mode
// 	glPixelTransferi(GL_RED_BIAS, 1);
// 	glPixelTransferi(GL_GREEN_BIAS, 1);
// 	glPixelTransferi(GL_BLUE_BIAS, 1);
// 	glPixelTransferi(GL_RED_SCALE, -1);
// 	glPixelTransferi(GL_GREEN_SCALE, -1);
// 	glPixelTransferi(GL_BLUE_SCALE, -1);
	
// 	// Night red mode
//  glPixelTransferf(GL_GREEN_SCALE, 0.1f);
//  glPixelTransferf(GL_BLUE_SCALE, 0.1f);
// 	
// 	glDisable(GL_TEXTURE_2D);
// 	glShadeModel(GL_FLAT);
// 	glDisable(GL_DEPTH_TEST);
// 	glDisable(GL_CULL_FACE);
// 	glDisable(GL_LIGHTING);
// 	glDisable(GL_MULTISAMPLE);
// 	glDisable(GL_DITHER);
// 	glDisable(GL_ALPHA_TEST);
// 	glRasterPos2i(0,0);
// 	
// 	glReadBuffer(GL_BACK);
// 	glDrawBuffer(GL_BACK);
// 	glCopyPixels(1, 1, 200, 200, GL_COLOR);
}

//! Set the current projection type to use
void StelCore::setCurrentProjectionTypeKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	currentProjectionType = (ProjectionType)en.keyToValue(key.toAscii().data());
	if (currentProjectionType<0)
	{
		qWarning() << "Unknown projection type: " << key << "setting \"ProjectionStereographic\" instead";
		currentProjectionType = ProjectionStereographic;
	}
	const double savedFov = currentProjectorParams.fov;
	currentProjectorParams.fov = 0.0001;	// Avoid crash
	double newMaxFov = getProjection(Mat4d())->getMaxFov();
	movementMgr->setMaxFov(newMaxFov);
	currentProjectorParams.fov = qMin(newMaxFov, savedFov);
}
	
//! Get the current Mapping used by the Projection
QString StelCore::getCurrentProjectionTypeKey(void) const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType")).key(currentProjectionType);
}
	
//! Get the list of all the available projections
QStringList StelCore::getAllProjectionTypeKeys() const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	QStringList l;
	for (int i=0;i<en.keyCount();++i)
		l << en.key(i);
	return l;
}

//! Get the translated projection name from its TypeKey for the current locale
QString StelCore::projectionTypeKeyToNameI18n(const QString& key) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	QString s(getProjection(Mat4d(), (ProjectionType)en.keysToValue(key.toAscii()))->getNameI18());
	return s;
}

//! Get the projection TypeKey from its translated name for the current locale
QString StelCore::projectionNameI18nToTypeKey(const QString& nameI18n) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	for (int i=0;i<en.keyCount();++i)
	{
		if (getProjection(Mat4d(), (ProjectionType)i)->getNameI18()==nameI18n)
			return en.valueToKey(i);
	}
	// Unknown translated name
	Q_ASSERT(0);
	return en.valueToKey(ProjectionStereographic);
}

