/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

// class used to manage groups of Nebulas

#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QRegExp>

#include "StelApp.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelTexture.hpp"
#include "StelNavigator.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelLoadingBar.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelStyle.hpp"
#include "StelSkyImageTile.hpp"
#include "StelPainter.hpp"

void NebulaMgr::setLabelsColor(const Vec3f& c) {Nebula::labelColor = c;}
const Vec3f &NebulaMgr::getLabelsColor(void) const {return Nebula::labelColor;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::circleColor = c;}
const Vec3f &NebulaMgr::getCirclesColor(void) const {return Nebula::circleColor;}
void NebulaMgr::setCircleScale(float scale) {Nebula::circleScale = scale;}
float NebulaMgr::getCircleScale(void) const {return Nebula::circleScale;}


NebulaMgr::NebulaMgr(void) : nebGrid(200), displayNoTexture(false)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	Nebula::texCircle = StelTextureSP();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MilkyWay")->getCallOrder(actionName)+10;
	return 0;
}

// read from stream
void NebulaMgr::init()
{
	// TODO: mechanism to specify which sets get loaded at start time.
	// candidate methods:
	// 1. config file option (list of sets to load at startup)
	// 2. load all
	// 3. flag in nebula_textures.fab (yuk)
	// 4. info.ini file in each set containing a "load at startup" item
	// For now (0.9.0), just load the default set
	loadNebulaSet("default");

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	nebulaFont.setPixelSize(13);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setMinFilter(GL_LINEAR);
	Nebula::texCircle = StelApp::getInstance().getTextureManager().createTexture("neb.png");   // Load circle texture

	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur5.png");   // Load pointer texture

	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setHintsAmount(conf->value("astro/nebula_hints_amount", 3).toDouble());
	setLabelsAmount(conf->value("astro/nebula_labels_amount", 3).toDouble());
	setCircleScale(conf->value("astro/nebula_scale",1.0f).toDouble());
	setFlagDisplayNoTexture(conf->value("astro/flag_nebula_display_no_texture", false).toBool());

	updateI18n();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

struct DrawNebulaFuncObject
{
	DrawNebulaFuncObject(float amaxMagHints, float amaxMagLabels, const StelPainter& p, bool acheckMaxMagHints) : maxMagHints(amaxMagHints), maxMagLabels(amaxMagLabels), sPainter(p), checkMaxMagHints(acheckMaxMagHints)
	{
		angularSizeLimit = 5./sPainter.getProjector()->getPixelPerRadAtCenter()*180./M_PI;
	}
	void operator()(StelRegionObjectP obj)
	{
		Nebula* n = obj.staticCast<Nebula>().data();
		if (n->angularSize>angularSizeLimit || (checkMaxMagHints && n->mag <= maxMagHints))
		{
			sPainter.getProjector()->project(n->XYZ,n->XY);
			n->drawLabel(sPainter, maxMagLabels);
			n->drawHints(sPainter, maxMagHints);
		}
	}
	float maxMagHints;
	float maxMagLabels;
	const StelPainter& sPainter;
	float angularSizeLimit;
	bool checkMaxMagHints;
};

// Draw all the Nebulae
void NebulaMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	StelSkyDrawer* skyDrawer = core->getSkyDrawer();

	Nebula::hintsBrightness = hintsFader.getInterstate()*flagShow.getInterstate();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// Use a 1 degree margin
	const double margin = 1.*M_PI/180.*prj->getPixelPerRadAtCenter();
	const SphericalRegionP& p = prj->getViewportConvexPolygon(margin, margin);

	// Print all the nebulae of all the selected zones
	float maxMagHints = skyDrawer->getLimitMagnitude()*1.2-2.+(hintsAmount*1.2f)-2.f;
	float maxMagLabels = skyDrawer->getLimitMagnitude()-2.+(labelsAmount*1.2f)-2.f;
	sPainter.setFont(nebulaFont);
	DrawNebulaFuncObject func(maxMagHints, maxMagLabels, sPainter, hintsFader.getInterstate()>0.0001);
	nebGrid.processIntersectingRegions(p, func);
	
	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, sPainter);
}

void NebulaMgr::drawPointer(const StelCore* core, const StelPainter& sPainter)
{
	const StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(nav);

		// Compute 2D pos and return if outside screen
		if (!prj->projectInPlace(pos)) return;
		glColor3f(0.4f,0.5f,0.8f);
		texPointer->bind();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		// Size on screen
		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();

		size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		sPainter.drawSprite2dMode(pos[0]-size/2, pos[1]-size/2, 10, 90);
		sPainter.drawSprite2dMode(pos[0]-size/2, pos[1]+size/2, 10, 0);
		sPainter.drawSprite2dMode(pos[0]+size/2, pos[1]+size/2, 10, -90);
		sPainter.drawSprite2dMode(pos[0]+size/2, pos[1]-size/2, 10, -180);
	}
}

void NebulaMgr::updateSkyCulture(const QString& skyCultureDir)
{;}

void NebulaMgr::setStelStyle(const StelStyle& style)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section = style.confSectionName;

	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelsColor(StelUtils::strToVec3f(conf->value(section+"/nebula_label_color", defaultColor).toString()));
	setCirclesColor(StelUtils::strToVec3f(conf->value(section+"/nebula_circle_color", defaultColor).toString()));
}

// Search by name
NebulaP NebulaMgr::search(const QString& name)
{
	QString uname = name.toUpper();

	foreach (const NebulaP& n, nebArray)
	{
		QString testName = n->getEnglishName().toUpper();
		if (testName==uname) return n;
	}

	// If no match found, try search by catalog reference
	QRegExp catNumRx("^(M|NGC|IC)\\s*(\\d+)$");
	if (catNumRx.exactMatch(uname))
	{
		QString cat = catNumRx.capturedTexts().at(1);
		int num = catNumRx.capturedTexts().at(2).toInt();

		if (cat == "M") return searchM(num);
		if (cat == "NGC") return searchNGC(num);
		if (cat == "IC") return searchIC(num);
	}
	return NebulaP();
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	try
	{
		loadNGC(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000.dat"));
		loadNGCNames(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000names.dat"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading nebula data set " << setName << ": " << e.what();
	}
}

// Look for a nebulae by XYZ coords
NebulaP NebulaMgr::search(const Vec3d& apos)
{
	Vec3d pos = apos;
	pos.normalize();
	NebulaP plusProche;
	float anglePlusProche=0.;
	foreach (const NebulaP& n, nebArray)
	{
		if (n->XYZ*pos>anglePlusProche)
		{
			anglePlusProche=n->XYZ*pos;
			plusProche=n;
		}
	}
	if (anglePlusProche>0.999)
	{
		return plusProche;
	}
	else return NebulaP();
}

// Return a stl vector containing the nebulas located inside the limFov circle around position v
QList<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagShow())
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;
	foreach (const NebulaP& n, nebArray)
	{
		equPos = n->XYZ;
		equPos.normalize();
		if (equPos*v>=cosLimFov)
		{
			result.push_back(qSharedPointerCast<StelObject>(n));
		}
	}
	return result;
}

NebulaP NebulaMgr::searchM(unsigned int M)
{
	foreach (const NebulaP& n, nebArray)
		if (n->M_nb == M)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchNGC(unsigned int NGC)
{
	if (ngcIndex.contains(NGC))
		return ngcIndex[NGC];
	return NebulaP();
}

NebulaP NebulaMgr::searchIC(unsigned int IC)
{
	foreach (const NebulaP& n, nebArray)
		if (n->IC_nb == IC) return n;
	return NebulaP();
}

#if 0
// read from stream
bool NebulaMgr::loadNGCOld(const QString& catNGC)
{
	StelLoadingBar& lb = *StelApp::getInstance().getStelLoadingBar();
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	while (!in.atEnd())
	{
		in.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records weree rad without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
			continue;
		++currentRecordNumber;

		// Update the status bar every 200 record
		if (!(currentRecordNumber%200) || (currentRecordNumber == totalRecords))
		{
			lb.SetMessage(q_("Loading NGC catalog: %1/%2").arg(currentRecordNumber).arg(totalRecords));
			lb.Draw((float)currentRecordNumber/totalRecords);
		}

		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		if (!e->readNGC((char*)record.toLocal8Bit().data())) // reading error
		{
			e.clear();
		}
		else
		{
			nebArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			if (e->NGC_nb!=0)
				ngcIndex.insert(e->NGC_nb, e);
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC records";
	return true;
}
#endif

bool NebulaMgr::loadNGC(const QString& catNGC)
{
	StelLoadingBar& lb = *StelApp::getInstance().getStelLoadingBar();
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly))
		return false;
	QDataStream ins(&in);
	lb.SetMessage(q_("Loading NGC catalog"));
	lb.Draw(0);
	
	int totalRecords=0;
	while (!ins.atEnd())
	{
		// Create a new Nebula record
		NebulaP e = NebulaP(new Nebula);
		e->readNGC(ins);
		
		nebArray.append(e);
		nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
		if (e->NGC_nb!=0)
			ngcIndex.insert(e->NGC_nb, e);
		++totalRecords;
	}
	in.close();
	qDebug() << "Loaded" << totalRecords << "NGC records";
	return true;
}

bool NebulaMgr::loadNGCNames(const QString& catNGCNames)
{
	qDebug() << "Loading NGC name data ...";
	QFile ngcNameFile(catNGCNames);
	if (!ngcNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "NGC name data file" << catNGCNames << "not found.";
		return false;
	}

	// Read the names of the NGC objects
	QString name, record;
	int totalRecords=0;
	int lineNumber=0;
	int readOk=0;
	int nb;
	NebulaP e;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!ngcNameFile.atEnd())
	{
		record = QString::fromUtf8(ngcNameFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;
		nb = record.mid(38,4).toInt();
		if (record[37] == 'I')
		{
			e = searchIC(nb);
		}
		else
		{
			e = searchNGC(nb);
		}

		// get name, trimmed of whitespace
		name = record.left(36).trimmed();

		if (e)
		{
			// If the name is not a messier number perhaps one is already
			// defined for this object
			if (name.left(2).toUpper() != "M ")
			{
				e->englishName = name;
			}
			else
			{
				// If it's a messiernumber, we will call it a messier if there is no better name
				name = name.mid(2); // remove "M "

				// read the Messier number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Messier number at line" << lineNumber << "of" << catNGCNames;
					continue;
				}

				e->M_nb=(unsigned int)(num);
				e->englishName = QString("M%1").arg(num);
			}

			readOk++;
		}
		else
			qWarning() << "no position data for " << name << "at line" << lineNumber << "of" << catNGCNames;
	}
	ngcNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC name records successfully";

	return true;
}


void NebulaMgr::updateI18n()
{
	StelTranslator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	foreach (NebulaP n, nebArray)
		n->translateName(trans);
}


//! Return the matching Nebula object's pointer if exists or NULL
StelObjectP NebulaMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.mid(0, 3) == "NGC")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("NGC%1").arg(n->NGC_nb) == objw || QString("NGC %1").arg(n->NGC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		QString objwcap = n->nameI18.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.mid(0, 1) == "M")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("M%1").arg(n->M_nb) == objw || QString("M %1").arg(n->M_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	return StelObjectP();
}


//! Return the matching Nebula object's pointer if exists or NULL
//! TODO split common parts of this and I18 fn above into a separate fn.
StelObjectP NebulaMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.mid(0, 3) == "NGC")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("NGC%1").arg(n->NGC_nb) == objw || QString("NGC %1").arg(n->NGC_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		QString objwcap = n->englishName.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.mid(0, 1) == "M")
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (QString("M%1").arg(n->M_nb) == objw || QString("M %1").arg(n->M_nb) == objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	return NULL;
}


//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
QStringList NebulaMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	// Search by messier objects number (possible formats are "M31" or "M 31")
	if (objw.size()>=1 && objw[0]=='M')
	{
		foreach (const NebulaP& n, nebArray)
		{
			if (n->M_nb==0) continue;
			QString constw = QString("M%1").arg(n->M_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constw;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg(n->M_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	foreach (const NebulaP& n, nebArray)
	{
		if (n->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg(n->NGC_nb);
		QString constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constw;
			continue;
		}
		constw = QString("NGC %1").arg(n->NGC_nb);
		constws = constw.mid(0, objw.size());
		if (constws==objw)
			result << constw;
	}

	// Search by common names
	foreach (const NebulaP& n, nebArray)
	{
		QString constw = n->nameI18.mid(0, objw.size()).toUpper();
		if (constw==objw)
			result << n->nameI18;
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

