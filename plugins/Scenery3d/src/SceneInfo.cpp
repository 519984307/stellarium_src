/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti
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

#include "SceneInfo.hpp"

#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

const QString SceneInfo::SCENES_PATH("scenery3d/");
int SceneInfo::metaTypeId = initMetaType();

int SceneInfo::initMetaType()
{
	return qRegisterMetaType<SceneInfo>();
}

bool SceneInfo::loadByID(const QString &id,SceneInfo& info)
{
	QString file = StelFileMgr::findFile(SCENES_PATH + id + "/scenery3d.ini", StelFileMgr::File);
	if(file.isEmpty())
	{
		qCritical()<<"scenery3d.ini file with id "<<id<<" does not exist!";
		return false;
	}
	//get full directory path
	QString path = QFileInfo(file).absolutePath();

	QSettings ini(file,StelIniFormat);
	if (ini.status() != QSettings::NoError)
	{
	    qCritical() << "ERROR parsing scenery3d.ini file: " << file;
	    return false;
	}

	//load QSettings file
	info.id = id;
	info.fullPath = path;

	//primary description of the scene
	ini.beginGroup("model");
	info.name = ini.value("name").toString();
	info.author = ini.value("author").toString();
	info.description = ini.value("description").toString();
	info.landscapeName = ini.value("landscape").toString();
	info.modelScenery = ini.value("scenery").toString();
	info.modelGround = ini.value("ground","").toString();
	info.vertexOrder = ini.value("obj_order","XYZ").toString();

	// In case we don't have an axis-aligned OBJ model, this is the chance to correct it.
	info.obj2gridMatrix = Mat4d::identity();
	if (ini.contains("obj2grid_trafo"))
	{
		QString str=ini.value("obj2grid_trafo").toString();
		QStringList strList=str.split(",");
		bool conversionOK[16];
		if (strList.length()==16)
		{
			info.obj2gridMatrix.set(strList.at(0).toDouble(&conversionOK[0]),
					strList.at(1).toDouble(&conversionOK[1]),
					strList.at(2).toDouble(&conversionOK[2]),
					strList.at(3).toDouble(&conversionOK[3]),
					strList.at(4).toDouble(&conversionOK[4]),
					strList.at(5).toDouble(&conversionOK[5]),
					strList.at(6).toDouble(&conversionOK[6]),
					strList.at(7).toDouble(&conversionOK[7]),
					strList.at(8).toDouble(&conversionOK[8]),
					strList.at(9).toDouble(&conversionOK[9]),
					strList.at(10).toDouble(&conversionOK[10]),
					strList.at(11).toDouble(&conversionOK[11]),
					strList.at(12).toDouble(&conversionOK[12]),
					strList.at(13).toDouble(&conversionOK[13]),
					strList.at(14).toDouble(&conversionOK[14]),
					strList.at(15).toDouble(&conversionOK[15])
					);
			for (int i=0; i<16; ++i)
			{
				if (!conversionOK[i]) qWarning() << "WARNING: scenery3d.ini: element " << i+1 << " of obj2grid_trafo invalid, set zo zero.";
			}
		}
		else qWarning() << "obj2grid_trafo invalid: not 16 comma-separated elements";
	}
	ini.endGroup();

	//some importing/rendering params
	ini.beginGroup("general");
	info.transparencyThreshold = ini.value("transparency_threshold", 0.5f).toFloat();
	info.sceneryGenerateNormals = ini.value("scenery_generate_normals", false).toBool();
	info.groundGenerateNormals = ini.value("ground_generate_normals", false).toBool();
	ini.endGroup();

	//load location data
	if(ini.childGroups().contains("location"))
	{
		ini.beginGroup("location");
		info.location.reset(new StelLocation());
		info.location->name = ini.value("name",info.name).toString();
		info.location->planetName = ini.value("planetName","Earth").toString();

		if(ini.contains("altitude"))
		{
			QVariant val = ini.value("altitude");
			if(val == "from_model")
			{
				info.altitudeFromModel = true;
				//info.location->altitude = -32766;
			}
			else
			{
				info.altitudeFromModel = false;
				info.location->altitude = val.toInt();
			}
		}

		if(ini.contains("latitude"))
			info.location->latitude = StelUtils::getDecAngle(ini.value("latitude").toString())*180./M_PI;
		if (ini.contains("longitude"))
			info.location->longitude = StelUtils::getDecAngle(ini.value("longitude").toString())*180./M_PI;
		if (ini.contains("country"))
			info.location->country = ini.value("country").toString();
		if (ini.contains("state"))
			info.location->state = ini.value("state").toString();

		info.location->landscapeKey = info.landscapeName;
		ini.endGroup();
	}
	else
		info.location.reset();

	//load coord info
	ini.beginGroup("coord");
	info.gridName = ini.value("grid_name","Unspecified Coordinate Frame").toString();
	double orig_x = ini.value("orig_E", 0.0).toDouble();
	double orig_y = ini.value("orig_N", 0.0).toDouble();
	double orig_z = ini.value("orig_H", 0.0).toDouble();
	info.modelWorldOffset=Vec3d(orig_x, orig_y, orig_z); // RealworldGridCoords=objCoords+modelWorldOffset

	// Find a rotation around vertical axis, most likely required by meridian convergence.
	double rot_z=0.0;
	QVariant convAngle = ini.value("convergence_angle",0.0);
	if (!convAngle.toString().compare("from_grid"))
	{ // compute rot_z from grid_meridian and location. Check their existence!
		if (ini.contains("grid_meridian"))
		{
			double gridCentralMeridian=StelUtils::getDecAngle(ini.value("grid_meridian").toString())*180./M_PI;
			if (!info.location.isNull())
			{
				// Formula from: http://en.wikipedia.org/wiki/Transverse_Mercator_projection, Convergence
				//rot_z=std::atan(std::tan((lng-gridCentralMeridian)*M_PI/180.)*std::sin(lat*M_PI/180.));
				// or from http://de.wikipedia.org/wiki/Meridiankonvergenz
				rot_z=(info.location->longitude - gridCentralMeridian)*M_PI/180.*std::sin(info.location->latitude*M_PI/180.);

				qDebug() << "With Longitude " << info.location->longitude
					 << ", Latitude " << info.location->latitude << " and CM="
					 << gridCentralMeridian << ", ";
				qDebug() << "--> setting meridian convergence to " << rot_z*180./M_PI << "degrees";
			}
			else
			{
				qWarning() << "scenery3d.ini: Convergence angle \"from_grid\" requires location section!";
			}
		}
		else
		{
			qWarning() << "scenery3d.ini: Convergence angle \"from_grid\": cannot compute without grid_meridian!";
		}
	}
	else
	{
		rot_z = convAngle.toDouble() * M_PI / 180.0;
	}
	// We must apply also a 90 degree rotation, plus convergence(rot_z)
	info.zRotateMatrix = Mat4d::zrotation(M_PI/2.0 + rot_z);

	// At last, find start points.
	if(ini.contains("start_E") && ini.contains("start_N"))
	{
		info.startPositionFromModel = false;
		info.startWorldOffset[0] = ini.value("start_E").toDouble();
		info.startWorldOffset[1] = ini.value("start_N").toDouble();
		//this is not really used anymore, i think
		info.startWorldOffset[2] = ini.value("start_H",0.0).toDouble();
	}
	else
	{
		info.startPositionFromModel = true;
	}
	info.eyeLevel=ini.value("start_Eye", 1.65).toDouble();

	//calc pos in model coords
	info.relativeStartPosition = info.startWorldOffset - info.modelWorldOffset;
	 // I love code without comments
	info.relativeStartPosition[1]*=-1.0;
	info.relativeStartPosition = info.zRotateMatrix.inverse() * info.relativeStartPosition;
	info.relativeStartPosition[0]*=-1.0;
	info.relativeStartPosition[2]*=-1.0;

	if(ini.contains("zero_ground_height"))
	{
		info.groundNullHeightFromModel=false;
		info.groundNullHeight = ini.value("zero_ground_height").toDouble();
	}
	else
	{
		info.groundNullHeightFromModel=true;
		info.groundNullHeight=0.;
	}

	if (ini.contains("start_az_alt_fov"))
	{
		qDebug() << "scenery3d.ini: setting initial dir/fov.";
		info.lookAt_fov=StelUtils::strToVec3f(ini.value("start_az_alt_fov").toString());
		info.lookAt_fov[0]=180.0f-info.lookAt_fov[0];
	}
	else
	{
		info.lookAt_fov=Vec3f(0.f, 0.f, -1000.f);
		qDebug() << "scenery3d.ini: No initial dir/fov given.";
	}
	ini.endGroup();

	info.isValid = true;
	return true;
}

QString SceneInfo::getIDFromName(const QString &name)
{
	QMap<QString, QString> nameToDirMap = getNameToIDMap();

	return nameToDirMap.value(name);
}

bool SceneInfo::loadByName(const QString &name, SceneInfo &info)
{
	QString id = getIDFromName(name);
	if(!id.isEmpty())
		return loadByID(id,info);
	else
	{
	    qWarning() << "Can't find a 3D scenery with name=" << name;
	    return false;
	}
}

QStringList SceneInfo::getAllSceneIDs()
{
	QMap<QString,QString> nameToDirMap = getNameToIDMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the values
	foreach (QString i, nameToDirMap.values())
	{
		result += i;
	}
	return result;
}

QStringList SceneInfo::getAllSceneNames()
{
	QMap<QString,QString> nameToDirMap = getNameToIDMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	foreach (QString i, nameToDirMap.keys())
	{
		result += i;
	}
	return result;
}

QMap<QString, QString> SceneInfo::getNameToIDMap()
{
	QSet<QString> scenery3dDirs;
	QMap<QString, QString> result;

	scenery3dDirs = StelFileMgr::listContents(SceneInfo::SCENES_PATH, StelFileMgr::Directory);

	foreach (const QString& dir, scenery3dDirs)
	{
		QSettings scenery3dIni(StelFileMgr::findFile(SceneInfo::SCENES_PATH + dir + "/scenery3d.ini"), StelIniFormat);
		QString k = scenery3dIni.value("model/name").toString();
		result[k] = dir;
	}
	return result;
}

StoredViewList StoredView::getGlobalViewsForScene(const SceneInfo &scene)
{
	StoredViewList ret;

	//return empty
	if(!scene.isValid)
		return ret;

	//load global viewpoints
	QFileInfo globalfile( QDir(scene.fullPath), "viewpoints.ini");

	if(!globalfile.isFile())
	{
		qWarning()<<globalfile.absoluteFilePath()<<" is not a file";
	}
	else
	{
		QSettings ini(globalfile.absoluteFilePath(),StelIniFormat);
		if (ini.status() != QSettings::NoError)
		{
			qWarning() << "Error reading global viewpoint file " << globalfile.absoluteFilePath();
		}
		else
		{
			int size = ini.beginReadArray("StoredViews");
			readArray(ini,ret,size,true);
			ini.endArray();
		}
	}

	return ret;
}

StoredViewList StoredView::getUserViewsForScene(const SceneInfo &scene)
{
	StoredViewList ret;

	//return empty
	if(!scene.isValid)
		return ret;

	//load user viewpoints
	QString file = StelFileMgr::findFile(SceneInfo::SCENES_PATH + "userviews.ini", StelFileMgr::File);
	if(file.isEmpty())
	{
		qWarning()<<"No userviews.ini exists.";
	}
	else
	{
		QSettings ini(file,StelIniFormat);
		if (ini.status() != QSettings::NoError)
		{
			qWarning() << "Error reading user viewpoint file " << file;
		}
		else
		{
			int size = ini.beginReadArray(scene.id);
			readArray(ini,ret,size,false);
			ini.endArray();
		}
	}

	return ret;
}

void StoredView::readArray(QSettings &ini, StoredViewList &list, int size, bool isGlobal)
{
	for(int i =0;i<size;++i)
	{
		ini.setArrayIndex(i);

		StoredView sv;
		sv.isGlobal = isGlobal;
		sv.position = StelUtils::strToVec3f(ini.value("position").toString());
		sv.view_fov = StelUtils::strToVec3f(ini.value("view_fov").toString());

		list.append(sv);
	}
}
