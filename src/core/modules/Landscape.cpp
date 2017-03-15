/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2011 Bogdan Marinov
 * Copyright (C) 2014-17 Georg Zotti
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "Landscape.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocation.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelLocaleMgr.hpp"

#include <QDebug>
#include <QSettings>
#include <QVarLengthArray>
#include <QFile>
#include <QDir>
#include <QtAlgorithms>

Landscape::Landscape(float _radius)
	: radius(_radius)
	, id("uninitialized")
	, minBrightness(-1.)
	, landscapeBrightness(1.)
	, lightScapeBrightness(0.)
	, validLandscape(false)
	, rows(20)
	, cols(40)
	, angleRotateZ(0.)
	, angleRotateZOffset(0.)
	, sinMinAltitudeLimit(-0.035) //sin(-2 degrees))
	, defaultBortleIndex(-1)
	, defaultFogSetting(-1)
	, defaultExtinctionCoefficient(-1.)
	, defaultTemperature(-1000.)
	, defaultPressure(-2.)
	, horizonPolygon(NULL)
	, fontSize(18)
{
}

Landscape::~Landscape()
{}


// Load attributes common to all landscapes
void Landscape::loadCommon(const QSettings& landscapeIni, const QString& landscapeId)
{
	id = landscapeId;
	name = landscapeIni.value("landscape/name").toString();
	author = landscapeIni.value("landscape/author").toString();
	description = landscapeIni.value("landscape/description").toString();
	description = description.replace(QRegExp("\\\\n\\s*\\\\n"), "<br />");
	description = description.replace("\\n", " ");
	if (name.isEmpty())
	{
		qWarning() << "No valid landscape definition (no name) found for landscape ID "
			<< landscapeId << ". No landscape in use." << endl;
		validLandscape = false;
		return;
	}
	else
	{
		validLandscape = true;
	}

	// Optional data
	rows = landscapeIni.value("landscape/tesselate_rows", 20).toInt();
	cols = landscapeIni.value("landscape/tesselate_cols", 40).toInt();

	if (landscapeIni.childGroups().contains("location"))
	{
		if (landscapeIni.contains("location/planet"))
			location.planetName = landscapeIni.value("location/planet").toString();
		else
			location.planetName = "Earth";
		if (landscapeIni.contains("location/altitude"))
			location.altitude = landscapeIni.value("location/altitude").toInt();
		if (landscapeIni.contains("location/latitude"))
			location.latitude = StelUtils::getDecAngle(landscapeIni.value("location/latitude").toString())*180./M_PI;
		if (landscapeIni.contains("location/longitude"))
			location.longitude = StelUtils::getDecAngle(landscapeIni.value("location/longitude").toString())*180./M_PI;
		if (landscapeIni.contains("location/country"))
			location.country = landscapeIni.value("location/country").toString();
		if (landscapeIni.contains("location/state"))
			location.state = landscapeIni.value("location/state").toString();
		if (landscapeIni.contains("location/name"))
			location.name = landscapeIni.value("location/name").toString();
		else
			location.name = name;
		location.landscapeKey = name;
		defaultBortleIndex = landscapeIni.value("location/light_pollution", -1).toInt();
		if (defaultBortleIndex<=0) defaultBortleIndex=-1; // neg. values in ini file signal "no change".
		if (defaultBortleIndex>9) defaultBortleIndex=9; // correct bad values.

		defaultFogSetting = landscapeIni.value("location/display_fog", -1).toInt();
		defaultExtinctionCoefficient = landscapeIni.value("location/atmospheric_extinction_coefficient", -1.0).toDouble();
		defaultTemperature = landscapeIni.value("location/atmospheric_temperature", -1000.0).toDouble();
		defaultPressure = landscapeIni.value("location/atmospheric_pressure", -2.0).toDouble(); // -2=no change! [-1=computeFromAltitude]
	}

	// Set minimal brightness for landscape
	minBrightness = landscapeIni.value("landscape/minimal_brightness", -1.0).toDouble();

	// set a minimal altitude which the landscape covers. (new in 0.14)
	// This is to allow landscapes with "holes" in the ground (space station?) (Bug lp:1469407)
	sinMinAltitudeLimit = (float) std::sin(M_PI/180.0 * landscapeIni.value("landscape/minimal_altitude", -2.0).toDouble());

	// This is now optional for all classes, for mixing with a photo horizon:
	// they may have different offsets, like a south-centered pano and a geographically-oriented polygon.
	// In case they are aligned, we can use one value angle_rotatez, or define the polygon rotation individually.
	if (landscapeIni.contains("landscape/polygonal_horizon_list"))
	{
		createPolygonalHorizon(
					StelFileMgr::findFile("landscapes/" + landscapeId + "/" + landscapeIni.value("landscape/polygonal_horizon_list").toString()),
					landscapeIni.value("landscape/polygonal_angle_rotatez", 0.f).toFloat(),
					landscapeIni.value("landscape/polygonal_horizon_list_mode", "azDeg_altDeg").toString(),
					landscapeIni.value("landscape/polygonal_horizon_inverted", "false").toBool()
					);
		// This line can then be drawn in all classes with the color specified here. If not specified, don't draw it! (flagged by negative red)
		horizonPolygonLineColor=StelUtils::strToVec3f(landscapeIni.value("landscape/horizon_line_color", "-1,0,0" ).toString());
	}
	// we must get label color, this is global. (No sense to make that per-landscape!)
	QSettings *config = StelApp::getInstance().getSettings();
	labelColor=StelUtils::strToVec3f(config->value("landscape/label_color", "0.2,0.8,0.2").toString());
	fontSize=config->value("landscape/label_font_size", 18).toInt();
	loadLabels(landscapeId);
}

void Landscape::createPolygonalHorizon(const QString& lineFileName, const float polyAngleRotateZ, const QString &listMode , const bool polygonInverted)
{
	// qDebug() << _name << " " << _fullpath << " " << _lineFileName ;

	QStringList horizonModeList;
	horizonModeList << "azDeg_altDeg" << "azDeg_zdDeg" << "azRad_altRad" << "azRad_zdRad" << "azGrad_zdGrad" << "azGrad_zdGrad";
	const horizonListMode coordMode=(horizonListMode) horizonModeList.indexOf(listMode);

	QVector<Vec3d> horiPoints(0);
	QFile file(lineFileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Landscape Horizon line data file" << QDir::toNativeSeparators(lineFileName) << "not found.";
		return;
	}
	QRegExp emptyLine("^\\s*$");
	QTextStream in(&file);
	while (!in.atEnd())
	{
		// Build list of vertices. The checks can certainly become more robust.
		QString line = in.readLine();
		if (line.length()==0) continue;
		if (emptyLine.exactMatch((line))) continue;
		if (line.at(0)=='#') continue; // skip comment lines.
		//QStringList list = line.split(QRegExp("\\b\\s+\\b"));
		QStringList list = line.trimmed().split(QRegExp("\\s+"));
		if (list.count() < 2)
		{
			qWarning() << "Landscape polygon file" << QDir::toNativeSeparators(lineFileName) << "has bad line:" << line << "with" << list.count() << "elements";
			continue;
		}
		//if (list.count() > 2) // use first two elements but give warning.
		//{
		//	qWarning() << "Landscape polygon file" << QDir::toNativeSeparators(lineFileName) << "has excessive elements in line:" << line << " (" << list.count() << ", not 2 elements)";
		//}
		Vec3d point;
		//qDebug() << "Creating point for az=" << list.at(0) << " alt/zd=" << list.at(1);
		float az = 0.f, alt = 0.f;

		switch (coordMode)
		{
			case azDeg_altDeg:
				az=(180.0f - polyAngleRotateZ - list.at(0).toFloat())*M_PI/180.f;
				alt=list.at(1).toFloat()*M_PI/180.f;
				break;
			case azDeg_zdDeg:
				az=(180.0f - polyAngleRotateZ - list.at(0).toFloat())*M_PI/180.f;
				alt=(90.0f-list.at(1).toFloat())*M_PI/180.f;
				break;
			case azRad_altRad:
				az=(M_PI - polyAngleRotateZ*M_PI/180.f - list.at(0).toFloat());
				alt=list.at(1).toFloat();
				break;
			case azRad_zdRad:
				az=(M_PI - polyAngleRotateZ*M_PI/180.f - list.at(0).toFloat());
				alt=M_PI/2.f-list.at(1).toFloat();
				break;
			case azGrad_altGrad:
				az=(200.0f  - list.at(0).toFloat())*M_PI/200.f    - polyAngleRotateZ*M_PI/180.f;
				alt=list.at(1).toFloat()*M_PI/200.f;
				break;
			case azGrad_zdGrad:
				az=(200.0f  - list.at(0).toFloat())*M_PI/200.f    - polyAngleRotateZ*M_PI/180.f;
				alt=(100.0f-list.at(1).toFloat())*M_PI/200.f;
				break;
			default: qWarning() << "invalid coordMode while reading horizon line.";
		}

		StelUtils::spheToRect(az, alt, point);
		if (polygonInverted)
			horiPoints.prepend(point);
		else
			horiPoints.append(point);
	}
	file.close();
	//horiPoints.append(horiPoints.at(0)); // close loop? Apparently not necessary.

	//qDebug() << "created horiPoints with " << horiPoints.count() << "points:";
	//for (int i=0; i<horiPoints.count(); ++i)
	//	qDebug() << horiPoints.at(i)[0] << "/" << horiPoints.at(i)[1] << "/" << horiPoints.at(i)[2] ;
	AllSkySphericalRegion allskyRegion;
	SphericalPolygon aboveHorizonPolygon;
	aboveHorizonPolygon.setContour(horiPoints);
	horizonPolygon = allskyRegion.getSubtraction(aboveHorizonPolygon);
	if (polygonInverted)
	{
		AllSkySphericalRegion allskyRegion2;
		horizonPolygon = allskyRegion2.getSubtraction(horizonPolygon);
		//horizonPolygon=&aboveHorizonPolygon;
	}
}

#include <iostream>
const QString Landscape::getTexturePath(const QString& basename, const QString& landscapeId) const
{
	// look in the landscape directory first, and if not found default to global textures directory
	QString path = StelFileMgr::findFile("landscapes/" + landscapeId + "/" + basename);
	if (path.isEmpty())
		path = StelFileMgr::findFile("textures/" + basename);
	return path;
}

// find optional file and fill landscapeLabels list.
void Landscape::loadLabels(const QString& landscapeId)
{
	// in case we have labels and this is called for a retranslation, clean list first.
	landscapeLabels.clear();

	QString lang, descFileName, locLabelFileName, engLabelFileName;

	lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	locLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId, StelFileMgr::Directory) + "/gazetteer." + lang + ".utf8";
	engLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId, StelFileMgr::Directory) + "/gazetteer.en.utf8";

	// Check the file with full name of locale
	if (!QFileInfo(locLabelFileName).exists())
	{
		// File not found. What about short name of locale?
		lang = lang.split("_").at(0);
		locLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId, StelFileMgr::Directory) + "/gazetteer." + lang + ".utf8";
	}

	// Get localized or at least English description for landscape
	if (QFileInfo(locLabelFileName).exists())
		descFileName = locLabelFileName;
	else if (QFileInfo(engLabelFileName).exists())
		descFileName = engLabelFileName;
	else
		return;

	// We have found some file now.
	QFile file(descFileName);
	if(file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&file);
		in.setCodec("UTF-8");
		while (!in.atEnd())
		{
			QString line=in.readLine();

			// Skip comments and all-empty lines (space allowed and ignored)
			if (line.startsWith('#') || line.trimmed().isEmpty() )
				continue;
			// Read entries, construct vectors, put in list.
			QStringList parts=line.split('|');
			if (parts.count() != 5)
			{
				qWarning() << "Invalid line in landscape gazetteer" << descFileName << ":" << line;
				continue;
			}
			LandscapeLabel newLabel;
			newLabel.name=parts.at(4).trimmed();
			StelUtils::spheToRect((180.0f-parts.at(0).toFloat()) *M_PI/180.0, parts.at(1).toFloat()*M_PI/180.0, newLabel.featurePoint);
			StelUtils::spheToRect((180.0f-parts.at(0).toFloat() - parts.at(3).toFloat())*M_PI/180.0, (parts.at(1).toFloat() + parts.at(2).toFloat())*M_PI/180.0, newLabel.labelPoint);
			landscapeLabels.append(newLabel);
			//qDebug() << "Added landscape label " << newLabel.name;
		}
		file.close();
	}
}

void Landscape::drawLabels(StelCore* core, StelPainter *painter)
{
	if (landscapeLabels.length()==0) // no labels
		return;
	if (labelFader.getInterstate() < 0.0001f) // switched off
		return;

	// We must reset painter to pure altaz coordinates without pano-based rotation
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	painter->setProjector(prj);
	QFont font;
	font.setPixelSize(fontSize);
	painter->setFont(font);
	QFontMetrics fm(font);
	painter->setColor(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate()*landFader.getInterstate());

	painter->setBlending(true);
	painter->setLineSmooth(true);

	for (int i = 0; i < landscapeLabels.size(); ++i)
	{
		// in case of gravityLabels, we cannot shift-adjust centered placename, sorry!
		if (prj->getFlagGravityLabels())
		{
			painter->drawText(landscapeLabels.at(i).labelPoint, landscapeLabels.at(i).name, 0, 0, 0, false);
		}
		else
		{
			int textWidth=fm.width(landscapeLabels.at(i).name);
			painter->drawText(landscapeLabels.at(i).labelPoint, landscapeLabels.at(i).name, 0, -textWidth/2, 2, true);
		}
		painter->drawGreatCircleArc(landscapeLabels.at(i).featurePoint, landscapeLabels.at(i).labelPoint, NULL);
	}

	painter->setLineSmooth(false);
	painter->setBlending(false);
}


LandscapeOldStyle::LandscapeOldStyle(float _radius)
	: Landscape(_radius)
	, sideTexs(NULL)
	, nbSideTexs(0)
	, nbSide(0)
	, sides(NULL)
	, nbDecorRepeat(0)
	, fogAltAngle(0.)
	, fogAngleShift(0.)
	, decorAltAngle(0.)
	, decorAngleShift(0.)
	, groundAngleShift(0.)
	, groundAngleRotateZ(0.)
	, drawGroundFirst(0)
	, tanMode(false)
	, calibrated(false)
	, memorySize(sizeof(LandscapeOldStyle)) // start with just the known entries.
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (sideTexs)
	{
		delete [] sideTexs;
		sideTexs = NULL;
	}

	if (sides) delete [] sides;
	if (sidesImages.size()>0)
	{
		qDeleteAll(sidesImages);
		sidesImages.clear();
	}
	landscapeLabels.clear();
}

void LandscapeOldStyle::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// TODO: put values into hash and call create() method to consolidate code
	loadCommon(landscapeIni, landscapeId);
	// rows, cols have been loaded already, but with different defaults.
	// GZ Hey, they are not used altogether! Resolution is constant, below!
	//rows = landscapeIni.value("landscape/tesselate_rows", 8).toInt();
	//cols = landscapeIni.value("landscape/tesselate_cols", 16).toInt();
	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "old_style")
	{
		qWarning() << "Landscape type mismatch for landscape " << landscapeId
				   << ", expected old_style, found " << type << ".  No landscape in use.";
		validLandscape = false;
		return;
	}

	nbDecorRepeat      = landscapeIni.value("landscape/nb_decor_repeat", 1).toInt();
	fogAltAngle        = landscapeIni.value("landscape/fog_alt_angle", 0.).toFloat();
	fogAngleShift      = landscapeIni.value("landscape/fog_angle_shift", 0.).toFloat();
	decorAltAngle      = landscapeIni.value("landscape/decor_alt_angle", 0.).toFloat();
	decorAngleShift    = landscapeIni.value("landscape/decor_angle_shift", 0.).toFloat();
	angleRotateZ       = landscapeIni.value("landscape/decor_angle_rotatez", 0.).toFloat()  * M_PI/180.f;
	groundAngleShift   = landscapeIni.value("landscape/ground_angle_shift", 0.).toFloat()   * M_PI/180.f;
	groundAngleRotateZ = landscapeIni.value("landscape/ground_angle_rotatez", 0.).toFloat() * M_PI/180.f;
	drawGroundFirst    = landscapeIni.value("landscape/draw_ground_first", 0).toInt();
	tanMode            = landscapeIni.value("landscape/tan_mode", false).toBool();
	calibrated         = landscapeIni.value("landscape/calibrated", false).toBool();

	// Load sides textures
	nbSideTexs = landscapeIni.value("landscape/nbsidetex", 0).toInt();
	sideTexs = new StelTextureSP[2*nbSideTexs]; // 0.14: allow upper half for light textures!
	for (int i=0; i<nbSideTexs; ++i)
	{
		QString textureKey = QString("landscape/tex%1").arg(i);
		QString textureName = landscapeIni.value(textureKey).toString();
		const QString texturePath = getTexturePath(textureName, landscapeId);
		sideTexs[i] = StelApp::getInstance().getTextureManager().createTexture(texturePath);
		// GZ: To query the textures, also keep an array of QImage*, but only
		// if that query is not going to be prevented by the polygon that already has been loaded at that point...
		if ( (!horizonPolygon) && calibrated ) { // for uncalibrated landscapes the texture is currently never queried, so no need to store.
			QImage *image = new QImage(texturePath);
			sidesImages.append(image); // indices identical to those in sideTexs
			memorySize+=image->byteCount();
		}
		// Also allow light textures. The light textures must cover the same geometry as the sides. It is allowed that not all or even any light textures are present!
		textureKey = QString("landscape/light%1").arg(i);
		textureName = landscapeIni.value(textureKey).toString();
		if (textureName.length())
		{
			const QString lightTexturePath = getTexturePath(textureName, landscapeId);
			sideTexs[nbSideTexs+i] = StelApp::getInstance().getTextureManager().createTexture(lightTexturePath);
			if(sideTexs[nbSideTexs+i])
				memorySize+=sideTexs[nbSideTexs+i].data()->getGlSize();
		}
		else
			sideTexs[nbSideTexs+i].clear();
	}
	if ( (!horizonPolygon) && calibrated )
	{
		Q_ASSERT(sidesImages.size()==nbSideTexs);
	}
	QMap<int, int> texToSide;
	// Init sides parameters
	nbSide = landscapeIni.value("landscape/nbside", 0).toInt();
	sides = new landscapeTexCoord[nbSide];
	int texnum;
	for (int i=0;i<nbSide;++i)
	{
		QString key = QString("landscape/side%1").arg(i);                             // e.g. side0
		QString description = landscapeIni.value(key).toString();                     // e.g. tex0:0:0:1:1
		//sscanf(s.toLocal8Bit(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		QStringList parameters = description.split(':');
		//TODO: How should be handled an invalid texture description?
		QString textureName = parameters.value(0);                                    // tex0
		texnum = textureName.right(textureName.length() - 3).toInt();                 // 0
		sides[i].tex = sideTexs[texnum];
		sides[i].tex_illum = sideTexs[nbSide+texnum];
		sides[i].texCoords[0] = parameters.at(1).toFloat();
		sides[i].texCoords[1] = parameters.at(2).toFloat();
		sides[i].texCoords[2] = parameters.at(3).toFloat();
		sides[i].texCoords[3] = parameters.at(4).toFloat();
		//qDebug() << i << texnum << sides[i].texCoords[0] << sides[i].texCoords[1] << sides[i].texCoords[2] << sides[i].texCoords[3];

		// Prior to precomputing the sides, we used to match E to side0.
		// In r4598 the precomputing was put in place and caused a problem for
		// old_style landscapes which had a z rotation on the side textures
		// and where side0 did not map to tex0
		// texToSide is a nasty hack to replace the old behaviour.
		// GZ for V0.13: I put the zrotation to the draw call (like for all other landscapes).
		// Maybe this can be again simplified?
		texToSide[i] = texnum;
	}
	QString groundTexName = landscapeIni.value("landscape/groundtex").toString();
	QString groundTexPath = getTexturePath(groundTexName, landscapeId);
	groundTex = StelApp::getInstance().getTextureManager().createTexture(groundTexPath, StelTexture::StelTextureParams(true));
	if (groundTex)
		memorySize+=groundTex.data()->getGlSize();

	QString fogTexName = landscapeIni.value("landscape/fogtex").toString();
	QString fogTexPath = getTexturePath(fogTexName, landscapeId);
	fogTex = StelApp::getInstance().getTextureManager().createTexture(fogTexPath, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
	if (fogTex)
		memorySize+=fogTex.data()->getGlSize();

	// Precompute the vertex arrays for ground display
	// Make slices_per_side=(3<<K) so that the innermost polygon of the fandisk becomes a triangle:
	//const int slices_per_side = 3*64/(nbDecorRepeat*nbSide);
	//if (slices_per_side<=0) // GZ: How can negative ever happen?
	//	slices_per_side = 1;
	const int slices_per_side = qMax(3*64/(nbDecorRepeat*nbSide), 1);

	// draw a fan disk instead of a ordinary disk to that the inner slices
	// are not so slender. When they are too slender, culling errors occur
	// in cylinder projection mode.
	int slices_inside = nbSide*slices_per_side*nbDecorRepeat;
	int level = 0;
	while ((slices_inside&1)==0 && slices_inside > 4)
	{
		++level;
		slices_inside>>=1;
	}
	StelPainter::computeFanDisk(radius, slices_inside, level, groundVertexArr, groundTexCoordArr);


	// Precompute the vertex arrays for side display. The geometry of the sides is always a cylinder.
	// The texture is split into regular quads.

	// GZ: the original code for vertical placement makes unfortunately no sense. There are many approximately-fitted landscapes, though.
	// I added a switch "calibrated" for the ini file. If true, it works as this landscape apparently was originally intended,
	// if false (or missing) it uses the original code.
	// I corrected the texture coordinates so that decorAltAngle is the total vertical angle, decorAngleShift the lower angle,
	// and the texture in between is correctly stretched.
	// I located an undocumented switch tan_mode, maybe tan_mode=true means cylindrical panorama projection instead of equirectangular.
	// Since V0.13, calibrated&&tanMode also works!
	// In calibrated && !tan_mode, the vertical position is computed correctly, so that quads off the horizon are larger.
	// in calibrated &&  tan_mode, d_z can become a constant because the texture is already predistorted in cylindrical projection.
	static const int stacks = (calibrated ? 16 : 8); // GZ: 8->16, I need better precision.
	float z0, d_z;
	if (calibrated)
	{
		if (tanMode) // cylindrical pano: linear in d_z, simpler.
		{
			z0=radius*std::tan(decorAngleShift*M_PI/180.0f);
			d_z=(radius*std::tan((decorAltAngle+decorAngleShift)*M_PI/180.0f) - z0)/stacks;
		}
		else // equirectangular pano: angular z, requires more work in the loop below!
		{
			z0=decorAngleShift;
			d_z=decorAltAngle/stacks;
		}
	}
	else // buggy code, but legacy.
	{
		z0 =(tanMode ? radius * std::tan(decorAngleShift*M_PI/180.f)        : radius * std::sin(decorAngleShift*M_PI/180.f));
		d_z=(tanMode ? radius * std::tan(decorAltAngle  *M_PI/180.f)/stacks : radius * std::sin(decorAltAngle  *M_PI/180.0)/stacks);
	}

	const float alpha = 2.f*M_PI/(nbDecorRepeat*nbSide*slices_per_side); //delta_azimuth
	const float ca = std::cos(alpha);
	const float sa = std::sin(alpha);
	//float y0 = radius*std::cos(angleRotateZ);
	//float x0 = radius*std::sin(angleRotateZ);
	float y0 = radius;
	float x0 = 0.0f;

	LOSSide precompSide;
	precompSide.arr.primitiveType=StelVertexArray::Triangles;
	for (int n=0;n<nbDecorRepeat;n++)
	{
		for (int i=0;i<nbSide;i++)
		{
			int ti;
			if (texToSide.contains(i))
				ti = texToSide[i];
			else
			{
				qDebug() << QString("LandscapeOldStyle::load ERROR: found no corresponding tex value for side%1").arg(i);
				break;
			}
			precompSide.arr.vertex.resize(0);
			precompSide.arr.texCoords.resize(0);
			precompSide.arr.indices.resize(0);
			precompSide.tex=sideTexs[ti];
			precompSide.light=false;

			float tx0 = sides[ti].texCoords[0];
			const float d_tx = (sides[ti].texCoords[2]-sides[ti].texCoords[0]) / slices_per_side;
			const float d_ty = (sides[ti].texCoords[3]-sides[ti].texCoords[1]) / stacks;
			for (int j=0;j<slices_per_side;j++)
			{
				const float y1 = y0*ca - x0*sa;
				const float x1 = y0*sa + x0*ca;
				const float tx1 = tx0 + d_tx;
				float z = z0;
				float ty0 = sides[ti].texCoords[1];
				for (int k=0;k<=stacks*2;k+=2)
				{
					precompSide.arr.texCoords << Vec2f(tx0, ty0) << Vec2f(tx1, ty0);
					if (calibrated && !tanMode)
					{
						float tanZ=radius * std::tan(z*M_PI/180.f);
						precompSide.arr.vertex << Vec3d(x0, y0, tanZ) << Vec3d(x1, y1, tanZ);
					} else
					{
						precompSide.arr.vertex << Vec3d(x0, y0, z) << Vec3d(x1, y1, z);
					}
					z += d_z;
					ty0 += d_ty;
				}
				unsigned int offset = j*(stacks+1)*2;
				for (int k = 2;k<stacks*2+2;k+=2)
				{
					precompSide.arr.indices << offset+k-2 << offset+k-1 << offset+k;
					precompSide.arr.indices << offset+k   << offset+k-1 << offset+k+1;
				}
				y0 = y1;
				x0 = x1;
				tx0 = tx1;
			}
			precomputedSides.append(precompSide);
			if (sideTexs[ti+nbSide])
			{
				precompSide.light=true;
				precompSide.tex=sideTexs[ti+nbSide];
				precomputedSides.append(precompSide);	// These sides are not called by strict index!
									// May be 9 for 8 sidetexs plus 1-only light panel
			}
		}
	}
	//qDebug() << "OldStyleLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}

void LandscapeOldStyle::draw(StelCore* core)
{
	StelPainter painter(core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff));
	painter.setBlending(true);
	painter.setCullFace(true);

	if (!validLandscape)
		return;
	if (drawGroundFirst)
		drawGround(core, painter);
	drawDecor(core, painter, false);
	if (!drawGroundFirst)
		drawGround(core, painter);
	drawFog(core, painter);

	// Self-luminous layer (Light pollution etc). This looks striking!
	if (lightScapeBrightness>0.0f && illumFader.getInterstate())
	{
		painter.setBlending(true, GL_SRC_ALPHA, GL_ONE);
		drawDecor(core, painter, true);
	}

	// If a horizon line also has been defined, draw it.
	if (horizonPolygon && (horizonPolygonLineColor[0] >= 0))
	{
		//qDebug() << "drawing line";
		StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
		transfo->combine(Mat4d::zrotation(-angleRotateZOffset));
		const StelProjectorP prj = core->getProjection(transfo);
		painter.setProjector(prj);
		painter.setBlending(true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		painter.setColor(horizonPolygonLineColor[0], horizonPolygonLineColor[1], horizonPolygonLineColor[2], landFader.getInterstate());
		painter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
	}
	//else qDebug() << "no polygon defined";

	drawLabels(core, &painter);
}


// Draw the horizon fog
void LandscapeOldStyle::drawFog(StelCore* core, StelPainter& sPainter) const
{
	if (!fogFader.getInterstate())
		return;
	if(!landFader.getInterstate())
		return;
	if (!(core->getSkyDrawer()->getFlagHasAtmosphere()))
		return;

	const float vpos = (tanMode||calibrated) ? radius*std::tan(fogAngleShift*M_PI/180.) : radius*std::sin(fogAngleShift*M_PI/180.);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);

	if (calibrated) // new in V0.13: take proper care of the fog layer. This will work perfectly only for calibrated&&tanMode.
		transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));

	transfo->combine(Mat4d::translation(Vec3d(0.,0.,vpos)));
	sPainter.setProjector(core->getProjection(transfo));
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setColor(landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
			  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
			  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness), landFader.getInterstate());
	fogTex->bind();
	const float height = (calibrated?
				radius*(std::tan((fogAltAngle+fogAngleShift)*M_PI/180.)  - std::tan(fogAngleShift*M_PI/180.))
				: ((tanMode) ? radius*std::tan(fogAltAngle*M_PI/180.) : radius*std::sin(fogAltAngle*M_PI/180.)));
	sPainter.sCylinder(radius, height, 64, 1);
	sPainter.setBlending(true);
}

// Draw the side textures
void LandscapeOldStyle::drawDecor(StelCore* core, StelPainter& sPainter, const bool drawLight) const
{
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));
	sPainter.setProjector(core->getProjection(transfo));

	if (!landFader.getInterstate())
		return;
	if (drawLight)
		sPainter.setColor(illumFader.getInterstate()*lightScapeBrightness, illumFader.getInterstate()*lightScapeBrightness, illumFader.getInterstate()*lightScapeBrightness, landFader.getInterstate());
	else
		sPainter.setColor(landscapeBrightness, landscapeBrightness, landscapeBrightness, landFader.getInterstate());

	foreach (const LOSSide& side, precomputedSides)
	{
		if (side.light==drawLight)
		{
			side.tex->bind();
			sPainter.drawSphericalTriangles(side.arr, true, false, NULL, false);
		}
	}
}

// Draw the ground
void LandscapeOldStyle::drawGround(StelCore* core, StelPainter& sPainter) const
{
	if (!landFader.getInterstate())
		return;
	const float vshift = radius * ((tanMode || calibrated) ? std::tan(groundAngleShift) : std::sin(groundAngleShift));
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(groundAngleRotateZ-angleRotateZOffset) * Mat4d::translation(Vec3d(0,0,vshift)));

	sPainter.setProjector(core->getProjection(transfo));
	sPainter.setColor(landscapeBrightness, landscapeBrightness, landscapeBrightness, landFader.getInterstate());

	if(groundTex.isNull())
	{
		qWarning()<<"LandscapeOldStyle groundTex is invalid!";
	}
	else
	{
		groundTex->bind();
	}
	sPainter.setArrays((Vec3d*)groundVertexArr.constData(), (Vec2f*)groundTexCoordArr.constData());
	sPainter.drawFromArray(StelPainter::Triangles, groundVertexArr.size()/3);
}

float LandscapeOldStyle::getOpacity(Vec3d azalt) const
{
	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(angleRotateZOffset));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the images...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	if (alt_rad < decorAngleShift*M_PI/180.0f) return 1.0f; // below decor, i.e. certainly opaque ground.
	if (alt_rad > (decorAltAngle+decorAngleShift)*M_PI/180.0f) return 0.0f; // above decor, i.e. certainly free sky.
	if (!calibrated) // the result of this function has no real use here: just complain and return result for math. horizon.
	{
		static QString lastLandscapeName;
		if (lastLandscapeName != name)
		{
			qWarning() << "Dubious result: Landscape " << name << " not calibrated. Opacity test represents mathematical horizon only.";
			lastLandscapeName=name;
		}
		return (azalt[2] > 0 ? 0.0f : 1.0f);
	}
	az = (M_PI-az) / M_PI;                             //  0..2 = N.E.S.W.N
	// we go to 0..1 domain, it's easier to think.
	const float xShift=angleRotateZ /(2.0f*M_PI); // shift value in -1..1 domain
	Q_ASSERT(xShift >= -1.0f);
	Q_ASSERT(xShift <=  1.0f);
	float az_phot=az*0.5f - 0.25f - xShift;      // The 0.25 is caused by regular pano left edge being East. The xShift compensates any configured angleRotateZ
	az_phot=fmodf(az_phot, 1.0f);
	if (az_phot<0) az_phot+=1.0f;                                //  0..1 = image-X for a non-repeating pano photo
	float az_panel =  nbSide*nbDecorRepeat * az_phot; // azimuth in "panel space". Ex for nbS=4, nbDR=3: [0..[12, say 11.4
	float x_in_panel=fmodf(az_panel, 1.0f);
	int currentSide = (int) floor(fmodf(az_panel, nbSide));
	Q_ASSERT(currentSide>=0);
	Q_ASSERT(currentSide<nbSideTexs);
	int x= (sides[currentSide].texCoords[0] + x_in_panel*(sides[currentSide].texCoords[2]-sides[currentSide].texCoords[0]))
			* sidesImages[currentSide]->width(); // pixel X from left.

	// QImage has pixel 0/0 in top left corner. We must find image Y for optionally cropped images.
	// It should no longer be possible that sample position is outside cropped texture. in this case, assert(0) but again assume full transparency and exit early.

	float y_img_1; // y of the sampled altitude in 0..1 visible image height from bottom
	if (tanMode)
	{
		const float tanAlt=std::tan(alt_rad);
		const float tanTop=std::tan((decorAltAngle+decorAngleShift)*M_PI/180.0f);
		const float tanBot=std::tan(decorAngleShift*M_PI/180.0f);
		y_img_1=(tanAlt-tanBot)/(tanTop-tanBot); // Y position 0..1 in visible image height from bottom
	}
	else
	{ // adapted from spherical...
		const float alt_pm1 = 2.0f * alt_rad  / M_PI;  // sampled altitude, -1...+1 linear in altitude angle
		const float img_top_pm1 = 1.0f-( (90.0f-decorAltAngle-decorAngleShift) / 90.0f); // the top line in -1..+1 (angular)
		if (alt_pm1>img_top_pm1) { Q_ASSERT(0); return 0.0f; } // should have been caught above with alt_rad tests
		const float img_bot_pm1 = 1.0f-((90.0f-decorAngleShift) / 90.0f); // the bottom line in -1..+1 (angular)
		if (alt_pm1<img_bot_pm1) { Q_ASSERT(0); return 1.0f; } // should have been caught above with alt_rad tests

		y_img_1=(alt_pm1-img_bot_pm1)/(img_top_pm1-img_bot_pm1); // the sampled altitude in 0..1 visible image height from bottom
		Q_ASSERT(y_img_1<=1.f);
		Q_ASSERT(y_img_1>=0.f);
	}
	// x0/y0 is lower left, x1/y1 upper right corner.
	float y_baseImg_1 = sides[currentSide].texCoords[1]+ y_img_1*(sides[currentSide].texCoords[3]-sides[currentSide].texCoords[1]);
	int y=(1.0-y_baseImg_1)*sidesImages[currentSide]->height();           // pixel Y from top.
	QRgb pixVal=sidesImages[currentSide]->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Oldstyle Landscape sampling: az=" << az*180.0 << "° alt=" << alt_rad*180.0f/M_PI
			 << "°, xShift[-1..+1]=" << xShift << " az_phot[0..1]=" << az_phot
			 << " --> current side panel " << currentSide
			 << ", w=" << sidesImages[currentSide]->width() << " h=" << sidesImages[currentSide]->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LandscapePolygonal::LandscapePolygonal(float _radius) : Landscape(_radius)
{}

LandscapePolygonal::~LandscapePolygonal()
{
	landscapeLabels.clear();
}

void LandscapePolygonal::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// loading the polygon has been moved to Landscape::loadCommon(), so that all Landscape classes can use a polygon line.
	loadCommon(landscapeIni, landscapeId);
	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "polygonal")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId << ", expected polygonal, found " << type << ".  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	if (horizonPolygon.isNull())
	{
		qWarning() << "Landscape " << landscapeId << " does not declare a valid polygonal_horizon_list.  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	groundColor=StelUtils::strToVec3f( landscapeIni.value("landscape/ground_color", "0,0,0" ).toString() );
	validLandscape = true;  // assume ok...
	//qDebug() << "PolygonalLandscape" << landscapeId << "loaded, mem size:" << getMemorySize();
}

void LandscapePolygonal::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-angleRotateZOffset));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode for the transition blending
	sPainter.setBlending(true);
	sPainter.setCullFace(true);

	sPainter.setColor(landscapeBrightness*groundColor[0], landscapeBrightness*groundColor[1], landscapeBrightness*groundColor[2], landFader.getInterstate());
	sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeFill);

	if (horizonPolygonLineColor[0] >= 0)
	{
		sPainter.setLineSmooth(true);
		sPainter.setColor(horizonPolygonLineColor[0], horizonPolygonLineColor[1], horizonPolygonLineColor[2], landFader.getInterstate());
		sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
		sPainter.setLineSmooth(false);
	}
	sPainter.setCullFace(false);
	drawLabels(core, &sPainter);
}

float LandscapePolygonal::getOpacity(Vec3d azalt) const
{
	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(angleRotateZOffset));

	if (horizonPolygon->contains(azalt)	) return 1.0f; else return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
// LandscapeFisheye
//

LandscapeFisheye::LandscapeFisheye(float _radius)
	: Landscape(_radius)
	, mapTex(StelTextureSP())
	, mapTexFog(StelTextureSP())
	, mapTexIllum(StelTextureSP())
	, mapImage(NULL)
	, texFov(360.)
	, memorySize(0)
{}

LandscapeFisheye::~LandscapeFisheye()
{
	if (mapImage) delete mapImage;
	landscapeLabels.clear();
}

void LandscapeFisheye::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);

	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "fisheye")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId << ", expected fisheye, found " << type << ".  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	create(name,
	       landscapeIni.value("landscape/texturefov", 360).toFloat(),
	       getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_fog").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_illum").toString(), landscapeId),
	       landscapeIni.value("landscape/angle_rotatez", 0.f).toFloat());
	//qDebug() << "FisheyeLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}


void LandscapeFisheye::create(const QString _name, float _texturefov, const QString& _maptex, const QString &_maptexFog, const QString& _maptexIllum, const float _angleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = true;  // assume ok...
	name = _name;
	texFov = _texturefov*M_PI/180.f;
	angleRotateZ = _angleRotateZ*M_PI/180.f;

	if (!horizonPolygon)
	{
		mapImage = new QImage(_maptex);
		memorySize+=mapImage->byteCount();
	}
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));
	memorySize+=mapTex.data()->getGlSize();

	if (_maptexIllum.length() && (!_maptexIllum.endsWith("/")))
	{
		mapTexIllum = StelApp::getInstance().getTextureManager().createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
		if (mapTexIllum)
			memorySize+=mapTexIllum.data()->getGlSize();
	}
	if (_maptexFog.length() && (!_maptexFog.endsWith("/")))
	{
		mapTexFog = StelApp::getInstance().getTextureManager().createTexture(_maptexFog, StelTexture::StelTextureParams(true));
		if (mapTexFog)
			memorySize+=mapTexFog.data()->getGlSize();
	}
}


void LandscapeFisheye::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	sPainter.setBlending(true);
	sPainter.setCullFace(true);
	sPainter.setColor(landscapeBrightness, landscapeBrightness, landscapeBrightness, landFader.getInterstate());
	mapTex->bind();
	sPainter.sSphereMap(radius,cols,rows,texFov,1);
	// NEW since 0.13: Fog also for fisheye...
	if ((mapTexFog) && (core->getSkyDrawer()->getFlagHasAtmosphere()))
	{
		//glBlendFunc(GL_ONE, GL_ONE); // GZ: Take blending mode as found in the old_style landscapes...
		sPainter.setBlending(true, GL_ONE, GL_ONE_MINUS_SRC_COLOR); // GZ: better?
		sPainter.setColor(landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
				  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
				  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness), landFader.getInterstate());
		mapTexFog->bind();
		sPainter.sSphereMap(radius,cols,rows,texFov,1);
	}

	if (mapTexIllum && lightScapeBrightness>0.0f && illumFader.getInterstate())
	{
		sPainter.setBlending(true, GL_SRC_ALPHA, GL_ONE);
		sPainter.setColor(illumFader.getInterstate()*lightScapeBrightness,
				  illumFader.getInterstate()*lightScapeBrightness,
				  illumFader.getInterstate()*lightScapeBrightness, landFader.getInterstate());
		mapTexIllum->bind();
		sPainter.sSphereMap(radius, cols, rows, texFov, 1);
	}

	sPainter.setCullFace(false);
	drawLabels(core, &sPainter);
}

float LandscapeFisheye::getOpacity(Vec3d azalt) const
{
	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(angleRotateZOffset));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	// QImage has pixel 0/0 in top left corner.
	// The texture is taken from the center circle in the square texture.
	// It is possible that sample position is outside. in this case, assume full opacity and exit early.
	if (M_PI/2-alt_rad > texFov/2.0f ) return 1.0f; // outside fov, in the clamped texture zone: always opaque.

	float radius=(M_PI/2-alt_rad)*2.0f/texFov; // radius in units of mapImage.height/2

	az = (M_PI-az) - angleRotateZ; // 0..+2pi -angleRotateZ, real azimuth. NESW
	//  The texture map has south on top, east at right (if anglerotateZ=0)
	int x= mapImage->height()/2*(1 + radius*std::sin(az));
	int y= mapImage->height()/2*(1 + radius*std::cos(az));

	QRgb pixVal=mapImage->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Landscape sampling: az=" << (az+angleRotateZ)/M_PI*180.0f << "° alt=" << alt_rad/M_PI*180.f
			 << "°, w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;


}
/////////////////////////////////////////////////////////////////////////////////////////////////
// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius)
	: Landscape(_radius)
	, mapTex(StelTextureSP())
	, mapTexFog(StelTextureSP())
	, mapTexIllum(StelTextureSP())
	, mapTexTop(0.)
	, mapTexBottom(0.)
	, fogTexTop(0.)
	, fogTexBottom(0.)
	, illumTexTop(0.)
	, illumTexBottom(0.)
	, mapImage(NULL)
	, memorySize(sizeof(LandscapeSpherical))
{}

LandscapeSpherical::~LandscapeSpherical()
{
	if (mapImage) delete mapImage;
	landscapeLabels.clear();
}

void LandscapeSpherical::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);
//	if (horizonPolygon)
//		qDebug() << "This landscape, " << landscapeId << ", has a polygon defined!" ;
//	else
//		qDebug() << "This landscape, " << landscapeId << ", has no polygon defined!" ;

	QString type = landscapeIni.value("landscape/type").toString();
	if (type != "spherical")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId
			<< ", expected spherical, found " << type
			<< ".  No landscape in use.\n";
		validLandscape = false;
		return;
	}

	create(name,
	       getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_fog").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_illum").toString(), landscapeId),
	       landscapeIni.value("landscape/angle_rotatez"      ,   0.f).toFloat(),
	       landscapeIni.value("landscape/maptex_top"         ,  90.f).toFloat(),
	       landscapeIni.value("landscape/maptex_bottom"      , -90.f).toFloat(),
	       landscapeIni.value("landscape/maptex_fog_top"     ,  90.f).toFloat(),
	       landscapeIni.value("landscape/maptex_fog_bottom"  , -90.f).toFloat(),
	       landscapeIni.value("landscape/maptex_illum_top"   ,  90.f).toFloat(),
	       landscapeIni.value("landscape/maptex_illum_bottom", -90.f).toFloat());
	//qDebug() << "SphericalLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}


//// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const QString _name, const QString& _maptex, const QString& _maptexFog, const QString& _maptexIllum, const float _angleRotateZ,
								const float _mapTexTop, const float _mapTexBottom,
								const float _fogTexTop, const float _fogTexBottom,
								const float _illumTexTop, const float _illumTexBottom)
{
	//qDebug() << "LandscapeSpherical::create():"<< _name << " : " << _maptex << " : " << _maptexFog << " : " << _maptexIllum << " : " << _angleRotateZ;
	validLandscape = true;  // assume ok...
	name = _name;
	angleRotateZ  = _angleRotateZ         *M_PI/180.f; // Defined in ini --> internal prg value
	mapTexTop     = (90.f-_mapTexTop)     *M_PI/180.f; // top     90     -->   0
	mapTexBottom  = (90.f-_mapTexBottom)  *M_PI/180.f; // bottom -90     -->  pi
	fogTexTop     = (90.f-_fogTexTop)     *M_PI/180.f;
	fogTexBottom  = (90.f-_fogTexBottom)  *M_PI/180.f;
	illumTexTop   = (90.f-_illumTexTop)   *M_PI/180.f;
	illumTexBottom= (90.f-_illumTexBottom)*M_PI/180.f;
	if (!horizonPolygon)
	{
		mapImage = new QImage(_maptex);
		memorySize+=mapImage->byteCount();
	}
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));
	memorySize+=mapTex.data()->getGlSize();

	if (_maptexIllum.length() && (!_maptexIllum.endsWith("/")))
	{
		mapTexIllum = StelApp::getInstance().getTextureManager().createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
		if (mapTexIllum)
			memorySize+=mapTexIllum.data()->getGlSize();
	}
	if (_maptexFog.length() && (!_maptexFog.endsWith("/")))
	{
		mapTexFog = StelApp::getInstance().getTextureManager().createTexture(_maptexFog, StelTexture::StelTextureParams(true));
		if (mapTexFog)
			memorySize+=mapTexFog.data()->getGlSize();
	}
}

void LandscapeSpherical::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	sPainter.setColor(landscapeBrightness, landscapeBrightness, landscapeBrightness, landFader.getInterstate());
	sPainter.setBlending(true);
	sPainter.setCullFace(true);

	mapTex->bind();

	// TODO: verify that this works correctly for custom projections [comment not by GZ]
	// seam is at East, except if angleRotateZ has been given.
	sPainter.sSphere(radius, 1.0, cols, rows, 1, true, mapTexTop, mapTexBottom);
	// Since 0.13: Fog also for sphericals...
	if ((mapTexFog) && (core->getSkyDrawer()->getFlagHasAtmosphere()))
	{
		sPainter.setBlending(true, GL_ONE, GL_ONE_MINUS_SRC_COLOR);
		sPainter.setColor(landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
				  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness),
				  landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness), landFader.getInterstate());
		mapTexFog->bind();
		sPainter.sSphere(radius, 1.0, cols, (int) ceil(rows*(fogTexTop-fogTexBottom)/(mapTexTop-mapTexBottom)), 1, true, fogTexTop, fogTexBottom);
	}

	// Self-luminous layer (Light pollution etc). This looks striking!
	if (mapTexIllum && (lightScapeBrightness>0.0f) && illumFader.getInterstate())
	{
		sPainter.setBlending(true, GL_SRC_ALPHA, GL_ONE);
		sPainter.setColor(lightScapeBrightness*illumFader.getInterstate(),
				  lightScapeBrightness*illumFader.getInterstate(),
				  lightScapeBrightness*illumFader.getInterstate(), landFader.getInterstate());
		mapTexIllum->bind();
		sPainter.sSphere(radius, 1.0, cols, (int) ceil(rows*(illumTexTop-illumTexBottom)/(mapTexTop-mapTexBottom)), 1, true, illumTexTop, illumTexBottom);
	}	
	//qDebug() << "before drawing line";

	// If a horizon line also has been defined, draw it.
	if (horizonPolygon && (horizonPolygonLineColor[0] >= 0))
	{
		//qDebug() << "drawing line";
		transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
		transfo->combine(Mat4d::zrotation(-angleRotateZOffset));
		const StelProjectorP prj = core->getProjection(transfo);
		sPainter.setProjector(prj);
		sPainter.setBlending(true);
		sPainter.setColor(horizonPolygonLineColor[0], horizonPolygonLineColor[1], horizonPolygonLineColor[2], landFader.getInterstate());
		sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
	}
	//else qDebug() << "no polygon defined";
	sPainter.setCullFace(false);
	drawLabels(core, &sPainter);
}

//! Sample landscape texture for transparency. May be used for advanced visibility computation like sunrise on the visible horizon etc.
//! @param azalt: normalized direction in alt-az frame
//! @retval alpha (0..1), where 0=fully transparent.
float LandscapeSpherical::getOpacity(Vec3d azalt) const
{
	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(angleRotateZOffset));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	// QImage has pixel 0/0 in top left corner. We must first find image Y for optionally cropped images.
	// It is possible that sample position is outside cropped texture. in this case, assume full transparency and exit early.

	const float alt_pm1 = 2.0f * std::asin(alt_rad)  / M_PI;  // sampled altitude, -1...+1 linear in altitude angle
	const float img_top_pm1 = 1.0f-2.0f*(mapTexTop    / M_PI); // the top    line in -1..+1
	if (alt_pm1>img_top_pm1) return 0.0f;
	const float img_bot_pm1 = 1.0f-2.0f*(mapTexBottom / M_PI); // the bottom line in -1..+1
	if (alt_pm1<img_bot_pm1) return 1.0f; // rare case of a hole in the ground. Even though there is a visible hole, play opaque.

	float y_img_1=(alt_pm1-img_bot_pm1)/(img_top_pm1-img_bot_pm1); // the sampled altitude in 0..1 image height from bottom
	Q_ASSERT(y_img_1<=1.f);
	Q_ASSERT(y_img_1>=0.f);

	int y=(1.0-y_img_1)*mapImage->height();           // pixel Y from top.

	az = (M_PI-az) / M_PI;                            //  0..2 = N.E.S.W.N

	const float xShift=(angleRotateZ) /M_PI; // shift value in -2..2
	float az_phot=az - 0.5f - xShift;      // The 0.5 is caused by regular pano left edge being East. The xShift compensates any configured angleRotateZ
	az_phot=fmodf(az_phot, 2.0f);
	if (az_phot<0) az_phot+=2.0f;                                //  0..2 = image-X

	int x=(az_phot/2.0f) * mapImage->width(); // pixel X from left.

	QRgb pixVal=mapImage->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Landscape sampling: az=" << az*180.0 << "° alt=" << alt_pm1*90.0f
			 << "°, xShift[-2..+2]=" << xShift << " az_phot[0..2]=" << az_phot
			 << ", w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;

}
