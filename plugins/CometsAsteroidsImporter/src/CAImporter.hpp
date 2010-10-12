/*
 * Comet and asteroids importer plug-in for Stellarium
 * 
 * Copyright (C) 2010 Bogdan Marinov
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

#ifndef _C_A_IMPORTER_HPP_
#define _C_A_IMPORTER_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"
//#include "CAIMainWindow.hpp"

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

class SolarSystemManagerWindow;
class QSettings;
/*!
 \class CAImporter
 \brief Main class of the Comets and Asteroids Importer plug-in.
 \author Bogdan Marinov
*/
class CAImporter : public StelModule
{
	Q_OBJECT

public:
	CAImporter();
	virtual ~CAImporter();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from the StelModule class
	//! called when the plug-in is loaded.
	//! All initializations should be done here.
	virtual void init();
	//! called before the plug-in is un-loaded.
	//! Useful for stopping processes, unloading textures, etc.
	virtual void deinit();
	virtual void update(double deltaTime);
	//! draws on the view port.
	//! Dialog windows don't need explicit drawing, it's done automatically.
	//! If a plug-in draws on the screen, it should be able to respect
	//! the night vision mode.
	virtual void draw(StelCore * core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! called when the "configure" button in the "Plugins" tab is pressed
	virtual bool configureGui(bool show);

	//! Convenience type for storage of SSO properties in ssystem.ini format.
	//! This is an easy way of storing data in the format used in Stellarium's
	//! solar system configuration file.
	//! What would be key/value pairs in a section in the ssystem.ini file
	//! are key/value pairs in the hash. The section name is stored with key
	//! "section_name".
	//! As it is a hash, key names are not stored alphabetically. This allows
	//! for rapid addition and look-up of values, unlike a real QSettings
	//! object in StelIniFormat.
	//! Also, using this way may allow scripts to define SSOs.
	//! \todo Better name.
	typedef QHash<QString, QVariant> SsoElements;
	
	//! Reads a single comet's orbital elements from a string.
	//! This function converts a line of comet orbital elements in MPC format
	//! to a hash in Stellarium's ssystem.ini format.
	//! The MPC's one-line orbital elements format for comets
	//! is described on their website:
	//! http://www.minorplanetcenter.org/iau/info/CometOrbitFormat.html
	//! \returns an empty hash if there is an error or the source string is not
	//! a valid line in MPC format.
	SsoElements readMpcOneLineCometElements (QString oneLineElements);

	//! Reads a single minor planet's orbital elements from a string.
	//! This function converts a line of minor planet orbital elements in
	//! MPC format to a hash in Stellarium's ssystem.ini format.
	//! The MPC's one-line orbital elements format for minor planets
	//! is described on their website:
	//! http://www.minorplanetcenter.org/iau/info/MPOrbitFormat.html
	//! \returns an empty hash if there is an error or the source string is not
	//! a valid line in MPC format.
	SsoElements readMpcOneLineMinorPlanetElements (QString oneLineElements);

	//! Reads a list of comet orbital elements from a file.
	//! This function reads a list of comet orbital elements in MPC's one-line
	//! format from a file (one comet per line) and converts it to a list of
	//! hashes in Stellarium's ssystem.ini format.
	//! Example source file is the list of observable comets on the MPC's site:
	//! http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
	//! readMpcOneLineCometElements() is used internally to parse each line.
	QList<SsoElements> readMpcOneLineCometElementsFromFile (QString filePath);

	//! Reads a list of minor planet orbital elements from a file.
	//! This function reads a list of minor planets orbital elements in MPC's
	//! one-line format from a file (one comet per line) and converts it to
	//! a list of hashes in Stellarium's ssystem.ini format.
	//! Example source file is the list of bright asteroids on the MPC's site:
	//! http://www.minorplanetcenter.org/iau/Ephemerides/Bright/2010/Soft00Bright.txt
	//! readMpcOneLineMinorPlanetElements() is used internally to parse each line.
	QList<SsoElements> readMpcOneLineMinorPlanetElementsFromFile (QString filePath);

	//! Adds a new entry at the end of the user solar system configuration file.
	//! This function writes directly to the file. See the note on why QSettings
	//! was not used in the description of
	//! appendToSolarSystemConfigurationFile(QList<SsoElements>)
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of the inserted entry, it is removed.
	bool appendToSolarSystemConfigurationFile(SsoElements object);

	//! Adds new entries at the end of the user solar system configuration file.
	//! This function writes directly to the file. QSettings was not used, as:
	//!  - Using QSettings with QSettings::IniFormat causes the list in the
	//! "color" field (e.g. "1.0, 1.0, 1.0") to be wrapped in double quotation
	//! marks (Stellarium requires no quotation marks).
	//!  - Using QSettings with StelIniFormat causes unaccepptable append times
	//! when the file grows (>~40 entries). This most probably happens because
	//! StelIniParser uses QMap internally for the entry list. QMap orders its
	//! keys (in the case of strings - alphabetically) and it has to find
	//! the appropriate place in the ordering for every new key, which takes
	//! more and more time as the list grows.
	//!
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of a new entry, it is removed.
	//! Invalid entries in the list (that don't contain a value for
	//! "section_name" or it is an empty string) are skipped and the processing
	//! continues from the next entry.
	//! \todo Protect the default Solar System configuration?
	//! \todo At least warn when overwriting old entries?
	bool appendToSolarSystemConfigurationFile(QList<SsoElements>);

	//! Returns the IDs of the objects listed in the default ssystem.ini.
	//! The default solar system configuration file is assumed to be the one
	//! in the installation directory.
	QStringList getAllDefaultSsoIds() {return defaultSsoIds;}

	//! Gets the IDs of the objects listed in the current user ssystem.ini.
	//! As the name suggests, the list is compiled when the function is run.
	QStringList readAllCurrentSsoIds();

	//! Removes an object from the user Solar System configuration file.
	//! Reloads the Solar System on successfull removal.
	//! \arg id object identifier (group name in the configuration file)
	//! \returns true if the entry has been removed successfully or there is
	//! no such entry
	//! \returns false if there was an error
	bool removeSsoWithId(QString id);

	//! returns the path
	QString getCustomSolarSystemFilePath() const {return customSolarSystemFilePath;}
	
public slots:
	//! Resets the Solar System configuration file and reloads the Solar System.
	void resetSolarSystemToDefault();

signals:
	//TODO: This should be part of SolarSystem::reloadPlanets()
	void solarSystemChanged();
	
private:
	bool isInitialized;

	//! Main window of the module's GUI
	SolarSystemManagerWindow * mainWindow;

	QSettings * solarSystemConfigurationFile;

	QString customSolarSystemFilePath;
	QString defaultSolarSystemFilePath;

	//! List of the IDs of all Solar System objects in the default ssystem.ini.
	//! Initialized in init().
	QStringList defaultSsoIds;

	//! Gets the IDs of the objects listed in a ssystem.ini-formatted file.
	//! Used internally in readAllCurrentSsoIds() and in init() to initialize
	//! defaultSsoIds.
	//! Does not check if the file exists.
	QStringList readAllActiveSsoIdsInFile(QString filePath);

	//! Creates a copy of the default ssystem.ini file in the user data directory.
	//! \returns true if a file already exists or the copying has been successful
	bool cloneSolarSystemConfigurationFile();

	//! Replaces the user copy of ssystem.ini with the default one.
	//! This function simply deletes the file, if it exists, and calls
	//! cloneSolarSystemConfigurationFile().
	//! \returns true if the replacement has been successfull.
	bool resetSolarSystemConfigurationFile();

	//! Converts an alphanumeric digit as used in MPC packed dates to an integer.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDates.html
	//! Interprets the digits from 1 to 9 normally, and the capital leters
	//! from A to V as numbers between 10 and 31.
	//! \returns 0 if the digit is invalid (0 is also an invalid ordinal number
	//! for a day or month, so this is not a problem)
	int unpackDayOrMonthNumber (QChar digit);
	//! Converts an alphanumeric year number as used in MPC packed dates to an integer.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDates.html
	//! Also used in packed provisional designations, see
	//! http://www.minorplanetcenter.org/iau/info/PackedDes.html
	int unpackYearNumber (QChar prefix, int lastTwoDigits);
	//! Converts a two-character number used in MPC packed provisional designations.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDes.html
	//! This function is used for both asteroid and comet designations.
	int unpackAlphanumericNumber (QChar prefix, int lastDigit);

	//TODO: This should be public and static, perhaps?
	//! Unpacks an MPC packed minor planet provisional designation.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDes.html
	//! \returns an empty string if the argument is not a valid packed
	//! provisional designation.
	QString unpackMinorPlanetProvisionalDesignation(QString packedDesignation);
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CAImporterStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif //_C_A_IMPORTER_HPP_
