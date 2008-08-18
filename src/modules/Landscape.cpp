/*
 * Stellarium
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

#include "Landscape.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"

#include <vector>

#include <QDebug>
#include <QSettings>

Landscape::Landscape(float _radius) : radius(_radius), skyBrightness(1.),
		     planet(""), latitude(-1000), longitude(-1000), altitude(1)
{
	validLandscape = 0;
}

Landscape::~Landscape()
{}


// Load attributes common to all landscapes
void Landscape::loadCommon(const QSettings& landscapeIni, const QString& landscapeId)
{
	name = landscapeIni.value("landscape/name").toString();
	author = landscapeIni.value("landscape/author").toString();
	description = landscapeIni.value("landscape/description").toString();
	if (name.isEmpty())
	{
		qWarning() << "No valid landscape definition found for landscape ID "
			<< landscapeId << ". No landscape in use." << endl;
		validLandscape = 0;
		return;
	}
	else
	{
		validLandscape = 1;
	}
	
	// Optional data
	if (landscapeIni.contains("location/planet")) planet = landscapeIni.value("location/planet").toString();
	if (landscapeIni.contains("location/altitude")) altitude = landscapeIni.value("location/altitude").toInt();
	if (landscapeIni.contains("location/latitude")) 
		latitude = StelUtils::getDecAngle(landscapeIni.value("location/latitude").toString());
	if (landscapeIni.contains("location/longitude")) 
		longitude = StelUtils::getDecAngle(landscapeIni.value("location/longitude").toString());
}

const QString Landscape::getTexturePath(const QString& basename, const QString& landscapeId)
{
	// look in the landscape directory first, and if not found default to global textures directory
	QString path;
	try
	{
		path = StelApp::getInstance().getFileMgr().findFile("landscapes/" + landscapeId + "/" + basename);
		return path;
	}
	catch (std::runtime_error& e)
	{
		path = StelApp::getInstance().getFileMgr().findFile("textures/" + basename);
		return path;
	}
}

LandscapeOldStyle::LandscapeOldStyle(float _radius) : Landscape(_radius), sideTexs(NULL), sides(NULL), tanMode(false)
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (sideTexs)
	{
		delete [] sideTexs;
		sideTexs = NULL;
	}

	if (sides) delete [] sides;
}

void LandscapeOldStyle::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// TODO: put values into hash and call create method to consolidate code
	loadCommon(landscapeIni, landscapeId);

	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "old_style")
	{
		qWarning() << "Landscape type mismatch for landscape " << landscapeId 
		           << ", expected old_style, found " << type << ".  No landscape in use.";
		validLandscape = 0;
		return;
	}

	// Load sides textures
	nbSideTexs = landscapeIni.value("landscape/nbsidetex", 0).toInt();
	sideTexs = new STextureSP[nbSideTexs];
	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setWrapMode(GL_CLAMP_TO_EDGE);
	for (int i=0;i<nbSideTexs;++i)
	{
		QString tmp = QString("tex%1").arg(i);
		sideTexs[i] = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(landscapeIni.value(QString("landscape/")+tmp).toString(), landscapeId));
	}

	// Init sides parameters
	nbSide = landscapeIni.value("landscape/nbside", 0).toInt();
	sides = new landscapeTexCoord[nbSide];
	QString s;
	int texnum;
	float a,b,c,d;
	for (int i=0;i<nbSide;++i)
	{
		QString tmp = QString("side%1").arg(i);
		s = landscapeIni.value(QString("landscape/")+tmp).toString();
		sscanf(s.toLocal8Bit(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		sides[i].tex = sideTexs[texnum];
		sides[i].texCoords[0] = a;
		sides[i].texCoords[1] = b;
		sides[i].texCoords[2] = c;
		sides[i].texCoords[3] = d;
		// qDebug("%f %f %f %f\n",a,b,c,d);
	}

	nbDecorRepeat = landscapeIni.value("landscape/nb_decor_repeat", 1).toInt();

	StelApp::getInstance().getTextureManager().setDefaultParams();
	groundTex = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(landscapeIni.value("landscape/groundtex").toString(), landscapeId));
	s = landscapeIni.value("landscape/ground").toString();
	sscanf(s.toLocal8Bit(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	groundTexCoord.tex = groundTex;
	groundTexCoord.texCoords[0] = a;
	groundTexCoord.texCoords[1] = b;
	groundTexCoord.texCoords[2] = c;
	groundTexCoord.texCoords[3] = d;

	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	fogTex = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(landscapeIni.value("landscape/fogtex").toString(), landscapeId));
	s = landscapeIni.value("landscape/fog").toString();
	sscanf(s.toLocal8Bit(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fogTexCoord.tex = fogTex;
	fogTexCoord.texCoords[0] = a;
	fogTexCoord.texCoords[1] = b;
	fogTexCoord.texCoords[2] = c;
	fogTexCoord.texCoords[3] = d;

	fogAltAngle          = landscapeIni.value("landscape/fog_alt_angle", 0.).toDouble();
	fogAngleShift      = landscapeIni.value("landscape/fog_angle_shift", 0.).toDouble();
	decorAltAngle      = landscapeIni.value("landscape/decor_alt_angle", 0.).toDouble();
	decorAngleShift    = landscapeIni.value("landscape/decor_angle_shift", 0.).toDouble();
	decorAngleRotatez  = landscapeIni.value("landscape/decor_angle_rotatez", 0.).toDouble();
	groundAngleShift   = landscapeIni.value("landscape/ground_angle_shift", 0.).toDouble();
	groundAngleRotatez = landscapeIni.value("landscape/ground_angle_rotatez", 0.).toDouble();
	drawGroundFirst      = landscapeIni.value("landscape/draw_ground_first", 0).toInt();
	tanMode              = landscapeIni.value("landscape/tan_mode", false).toBool();
}


// create from a hash of parameters (no ini file needed)
void LandscapeOldStyle::create(bool _fullpath, QMap<QString, QString> param)
{
	name = param["name"];
	validLandscape = 1;  // assume valid if got here

	// Load sides textures
	nbSideTexs = param["nbsidetex"].toInt();
	sideTexs = new STextureSP[nbSideTexs];
	
	char tmp[255];
	//StelApp::getInstance().getTextureManager().setMipmapsMode(true);
	//StelApp::getInstance().getTextureManager().setMagFilter(GL_NEAREST);
	for (int i=0;i<nbSideTexs;++i)
	{
		sprintf(tmp,"tex%d",i);
		sideTexs[i] = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param[tmp]);
	}

	// Init sides parameters
	nbSide = param["nbside"].toInt();
	sides = new landscapeTexCoord[nbSide];
	QString s;
	int texnum;
	float a,b,c,d;
	for (int i=0;i<nbSide;++i)
	{
		sprintf(tmp,"side%d",i);
		s = param[tmp];
		sscanf(s.toUtf8().constData(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		sides[i].tex = sideTexs[texnum];
		sides[i].texCoords[0] = a;
		sides[i].texCoords[1] = b;
		sides[i].texCoords[2] = c;
		sides[i].texCoords[3] = d;
		//qDebug("%f %f %f %f\n",a,b,c,d);
	}

	bool ok;
	nbDecorRepeat = param["nb_decor_repeat"].toInt(&ok);
	
	if (!ok)
		nbDecorRepeat = 1;

	groundTex = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param["groundtex"]);
	s = param["ground"];
	sscanf(s.toUtf8().constData(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	groundTexCoord.tex = groundTex;
	groundTexCoord.texCoords[0] = a;
	groundTexCoord.texCoords[1] = b;
	groundTexCoord.texCoords[2] = c;
	groundTexCoord.texCoords[3] = d;

	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	fogTex = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param["fogtex"]);
	s = param["fog"];
	sscanf(s.toUtf8().constData(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fogTexCoord.tex = fogTex;
	fogTexCoord.texCoords[0] = a;
	fogTexCoord.texCoords[1] = b;
	fogTexCoord.texCoords[2] = c;
	fogTexCoord.texCoords[3] = d;

	fogAltAngle = param["fog_alt_angle"].toDouble();
	fogAngleShift = param["fog_angle_shift"].toDouble();
	decorAltAngle = param["decor_alt_angle"].toDouble();
	decorAngleShift = param["decor_angle_shift"].toDouble();
	decorAngleRotatez = param["decor_angle_rotatez"].toDouble();
	groundAngleShift = param["ground_angle_shift"].toDouble();
	groundAngleRotatez = param["ground_angle_rotatez"].toDouble();
	drawGroundFirst = param["draw_ground_first"].toInt();
}

void LandscapeOldStyle::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!validLandscape) return;
	if (drawGroundFirst) drawGround(eye, prj, nav);
	drawDecor(eye, prj, nav);
	if (!drawGroundFirst) drawGround(eye, prj, nav);
	drawFog(eye, prj, nav);
}


// Draw the horizon fog
void LandscapeOldStyle::drawFog(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if(!fogFader.getInterstate()) return;
	glBlendFunc(GL_ONE, GL_ONE);
	glColor3f(fogFader.getInterstate()*(0.1f+0.1f*skyBrightness), fogFader.getInterstate()*(0.1f+0.1f*skyBrightness), fogFader.getInterstate()*(0.1f+0.1f*skyBrightness));
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	fogTex->bind();
	
	const double vpos = tanMode ? radius*std::tan(fogAngleShift*M_PI/180.) : radius*std::sin(fogAngleShift*M_PI/180.);
	prj->setCustomFrame(nav->getLocalToEyeMat() * Mat4d::translation(Vec3d(0.,0.,vpos)));
	
	const double height = tanMode ? radius*std::tan(fogAltAngle*M_PI/180.) : radius*std::sin(fogAltAngle*M_PI/180.);
	prj->sCylinder(radius, height, 128, 1, 1);
	
	glDisable(GL_CULL_FACE);
}

// Draw the mountains with a few pieces of texture
void LandscapeOldStyle::drawDecor(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if (!landFader.getInterstate()) return;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glColor4f(skyBrightness, skyBrightness, skyBrightness,
	          landFader.getInterstate());
	prj->setCurrentFrame(Projector::FrameLocal);

	const int stacks = 8;
	  // make slices_per_side=(3<<K) so that the innermost polygon of the
	  // fandisk becomes a triangle:
	int slices_per_side = 3*64/(nbDecorRepeat*nbSide);
	if (slices_per_side<=0) slices_per_side = 1;
	const double z0 = tanMode ? radius * std::tan(decorAngleShift*M_PI/180.0) : 
		radius * std::sin(decorAngleShift*M_PI/180.0);
	const double d_z = tanMode ? radius * std::tan(decorAltAngle*M_PI/180.0) / stacks : 
		radius * std::sin(decorAltAngle*M_PI/180.0) / stacks;
	const double alpha = 2.0*M_PI/(nbDecorRepeat*nbSide*slices_per_side);
	const double ca = cos(alpha);
	const double sa = sin(alpha);
	double y0 = radius*cos(decorAngleRotatez*M_PI/180.0);
	double x0 = radius*sin(decorAngleRotatez*M_PI/180.0);
	for (int n=0;n<nbDecorRepeat;n++) for (int i=0;i<nbSide;i++) {
		sides[i].tex->bind();
		double tx0 = sides[i].texCoords[0];
		const float d_tx0 = (sides[i].texCoords[2]-sides[i].texCoords[0])
		                  / slices_per_side;
		const float d_ty = (sides[i].texCoords[3]-sides[i].texCoords[1])
		                 / stacks;
		for (int j=0;j<slices_per_side;j++) {
			const double y1 = y0*ca - x0*sa;
			const double x1 = y0*sa + x0*ca;
			const float tx1 = tx0 + d_tx0;
			double z = z0;
			float ty0 = sides[i].texCoords[1];
			glBegin(GL_QUAD_STRIP);
			for (int k=0;k<=stacks;k++) {
				glTexCoord2f(tx0,ty0);
				prj->drawVertex3(x0, y0, z);
				glTexCoord2f(tx1,ty0);
				prj->drawVertex3(x1, y1, z);
				z += d_z;
				ty0 += d_ty;
			}
			glEnd();
			y0 = y1;
			x0 = x1;
			tx0 = tx1;
		}
	}
	glDisable(GL_CULL_FACE);
}


// Draw the ground
void LandscapeOldStyle::drawGround(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if (!landFader.getInterstate()) return;
	
	const double vshift = tanMode ? radius*std::tan(groundAngleShift*M_PI/180.) : radius*std::sin(groundAngleShift*M_PI/180.);
	Mat4d mat = nav->getLocalToEyeMat() * Mat4d::zrotation(groundAngleRotatez*M_PI/180.f) * Mat4d::translation(Vec3d(0,0,vshift));

	glColor4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	groundTex->bind();
	  // make slices_per_side=(3<<K) so that the innermost polygon of the
	  // fandisk becomes a triangle:
	int slices_per_side = 3*64/(nbDecorRepeat*nbSide);
	if (slices_per_side<=0) slices_per_side = 1;
	prj->setCustomFrame(mat);

	// draw a fan disk instead of a ordinary disk to that the inner slices
	// are not so slender. When they are too slender, culling errors occur
	// in cylinder projection mode.
	//prj->sDisk(radius,nbSide*slices_per_side*nbDecorRepeat,5, 1);
	int slices_inside = nbSide*slices_per_side*nbDecorRepeat;
	int level = 0;
	while ((slices_inside&1)==0 && slices_inside > 4) {
		level++;
		slices_inside>>=1;
	}
	prj->sFanDisk(radius,slices_inside,level);

	glDisable(GL_CULL_FACE);
}

LandscapeFisheye::LandscapeFisheye(float _radius) : Landscape(_radius)
{}

LandscapeFisheye::~LandscapeFisheye()
{
}

void LandscapeFisheye::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);
	
	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "fisheye")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId << ", expected fisheye, found " << type << ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}
	create(name, 0, getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
		landscapeIni.value("landscape/texturefov", 360).toDouble(),
		landscapeIni.value("landscape/angle_rotatez", 0.).toDouble());
}


// create a fisheye landscape from basic parameters (no ini file needed)
void LandscapeFisheye::create(const QString _name, bool _fullpath, const QString& _maptex,
	                          double _texturefov, double _angleRotatez)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = 1;  // assume ok...
	name = _name;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex);
	texFov = _texturefov*M_PI/180.;
	angleRotatez = _angleRotatez*M_PI/180.;
}


void LandscapeFisheye::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	mapTex->bind();
	prj->setCustomFrame(nav->getLocalToEyeMat() * Mat4d::zrotation(-angleRotatez));
	prj->sSphereMap(radius,40,20,texFov,1);

	glDisable(GL_CULL_FACE);
}


// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) : Landscape(_radius)
{}

LandscapeSpherical::~LandscapeSpherical()
{
}

void LandscapeSpherical::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);
	
	QString type = landscapeIni.value("landscape/type").toString();
	if (type != "spherical")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId 
			<< ", expected spherical, found " << type 
			<< ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}

	create(name, 0, getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
		landscapeIni.value("landscape/angle_rotatez", 0.).toDouble());
}


// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const QString _name, bool _fullpath, const QString& _maptex,
	                            double _angleRotatez)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = 1;  // assume ok...
	name = _name;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex);
	angleRotatez = _angleRotatez*M_PI/180.;
}


void LandscapeSpherical::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	// Need to flip texture usage horizontally due to glusphere convention
	// so that left-right is consistent in source texture and rendering
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glScalef(-1,1,1);
	glTranslatef(-1,0,0);
	glMatrixMode(GL_MODELVIEW);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	mapTex->bind();

	// TODO: verify that this works correctly for custom projections
	// seam is at East
	prj->setCustomFrame(nav->getLocalToEyeMat() * Mat4d::zrotation(-angleRotatez));
	prj->sSphere(radius,1.0,40,20,1);

	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

