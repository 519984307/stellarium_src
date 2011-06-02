/*
 * Copyright (C) 2011 Alexander Wolf
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

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "SNe.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QFile>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* SNeStelPluginInterface::getStelModule() const
{
	return new SNe();
}

StelPluginInfo SNeStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(SNe);

	StelPluginInfo info;
	info.id = "SNe";
	info.displayedName = q_("Historical supernova");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = q_("The plugin for visualization of some historical supernovaes.");
	return info;
}

Q_EXPORT_PLUGIN2(SNe, SNeStelPluginInterface)


/*************************************************************************
 Constructor
*************************************************************************/
SNe::SNe()
{
	setObjectName("SNe");
	font.setPixelSize(25);
}

/*************************************************************************
 Destructor
*************************************************************************/
SNe::~SNe()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SNe::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void SNe::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/SNe");

		sneJsonPath = StelFileMgr::findFile("modules/SNe", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/sne.json";
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "SNe::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(sneJsonPath).exists())
	{
		qDebug() << "SNe::init sne.json does not exist - copying default file to " << sneJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "SNe::init using sne.json file: " << sneJsonPath;

	readJsonFile();

}

/*************************************************************************
 Draw our module. This should print "Hello world!" in the main window
*************************************************************************/
void SNe::draw(StelCore* core)
{
	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);
	painter.drawText(300, 300, "Hello World!");
}

void SNe::restoreDefaultJsonFile(void)
{
	if (QFileInfo(sneJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/SNe/sne.json");
	if (!src.copy(sneJsonPath))
	{
		qWarning() << "SNe::restoreDefaultJsonFile cannot copy json resource to " + sneJsonPath;
	}
	else
	{
		qDebug() << "SNe::init copied default meteors.json to " << sneJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(sneJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

bool SNe::backupJsonFile(bool deleteOriginal)
{
	QFile old(sneJsonPath);
	if (!old.exists())
	{
		qWarning() << "SNe::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = sneJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "SNe::backupJsonFile WARNING - could not remove old sne.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "SNe::backupJsonFile WARNING - failed to copy meteors.json to sne.json.old";
		return false;
	}

	return true;
}

void SNe::readJsonFile(void)
{
	//
}
