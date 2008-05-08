/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#include "SkyBackground.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "SkyImageTile.hpp"
#include "StelModuleMgr.hpp"

#include <stdexcept>
#include <QDebug>
#include <QString>

SkyBackground::SkyBackground(void)
{
	setObjectName("SkyBackground");
}

SkyBackground::~SkyBackground()
{
	foreach (SkyImageTile* s, allSkyImages)
		s->deleteLater();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SkyBackground::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return GETSTELMODULE("MilkyWay")->getCallOrder(actionName)+5;
	return 0;
}

// read from stream
void SkyBackground::init()
{
	allSkyImages.append(new SkyImageTile("http://voint1.hq.eso.org/fabienDSS/allDSS.json"));
	try
	{
		allSkyImages.append(new SkyImageTile(StelApp::getInstance().getFileMgr().findFile("nebulae/default/textures.json")));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading nebula texture set " << "default" << ": " << e.what();
	}
}

// Draw all the multi-res images collection
double SkyBackground::draw(StelCore* core)
{
	Projector* prj = core->getProjection();
	
	prj->setCurrentFrame(Projector::FRAME_J2000);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	
	foreach (SkyImageTile* s, allSkyImages)
		s->draw(core);
	
	return 0;
}
