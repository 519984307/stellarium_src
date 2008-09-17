/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _STELAPP_HPP_
#define _STELAPP_HPP_

#include "fixx11h.h"
#include <QString>
#include <QObject>

// Predeclaration of some classes
class StelCore;
class SkyLocalizer;
class StelTextureMgr;
class StelObjectMgr;
class StelFontMgr;
class StelLocaleMgr;
class StelModuleMgr;
class StelSkyCultureMgr;
class StelFileMgr;
class QStringList;
class LoadingBar;
class QSettings;
class QNetworkAccessManager;
class StelStyle;
class QTime;
class LocationMgr;

//! @class StelApp 
//! Singleton main Stellarium application class.
//! This is the central class of Stellarium.  Only one singleton instance of
//! this class is created and can be accessed from anywhere else.  This class 
//! is the access point to several "Manager" class which provide application-wide 
//! services for managment of font, textures, localization, sky culture, and in 
//! theory all other services used by the other part of the program.
//!
//! The StelApp class is also the one managing the StelModule in a generic manner
//! by calling their update, drawing and other methods when needed.
//! @author Fabien Chereau
class StelApp : public QObject
{
	Q_OBJECT;
	
public:
	friend class StelAppGraphicsScene;
		
	//! Create and initialize the main Stellarium application.
	//! @param argc The number of command line parameters
	//! @param argv an array of char* command line arguments
	//! The configFile will be search for in the search path by the StelFileMgr,
	//! it is therefor possible to specify either just a file name or path within the
	//! search path, or use a full path or even a relative path to an existing file
	StelApp(int argc, char** argv, QObject* parent=NULL);

	//! Deinitialize and destroy the main Stellarium application.
	virtual ~StelApp();

	//! Initialize core and default modules.
	void init();

	//! Load and initialize external modules (plugins)
	void initPlugIns();
	
	//! Get the StelApp singleton instance.
	//! @return the StelApp singleton instance
	static StelApp& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Get the module manager to use for accessing any module loaded in the application.
	//! @return the module manager.
	StelModuleMgr& getModuleMgr() {return *moduleMgr;}
	
	//! Get the locale manager to use for i18n & date/time localization.
	//! @return the font manager to use for loading fonts.
	StelLocaleMgr& getLocaleMgr() {return *localeMgr;}
	
	//! Get the font manager to use for loading fonts.
	//! @return the font manager to use for loading fonts.
	StelFontMgr& getFontManager() {return *fontManager;}
	
	//! Get the sky cultures manager.
	//! @return the sky cultures manager
	StelSkyCultureMgr& getSkyCultureMgr() {return *skyCultureMgr;}
	
	//! Get the texture manager to use for loading textures.
	//! @return the texture manager to use for loading textures.
	StelTextureMgr& getTextureManager() {return *textureMgr;}
	
	//! Get the StelObject manager to use for querying from all stellarium objects.
	//! @return the StelObject manager to use for querying from all stellarium objects
	StelObjectMgr& getStelObjectMgr() {return *stelObjectMgr;}
	
	//! Get the Location manager to use for managing stored locations
	//! @return the Location manager to use for managing stored locations
	LocationMgr& getLocationMgr() {return *planetLocationMgr;}
	
	//! Get the StelFileMgr for performing file operations.
	//! @return the StelFileMgr manager to use for performing file operations
	StelFileMgr& getFileMgr() {return *stelFileMgr;}
	
	//! Get the core of the program.
	//! It is the one which provide the projection, navigation and tone converter.
	//! @return the StelCore instance of the program
	StelCore* getCore() {return core;}
	
	//! Get the main loading bar used by modules for displaying loading informations.
	//! @return the main LoadingBar instance of the program.
	LoadingBar* getLoadingBar() {return loadingBar;}
	
	//! Get the common instance of QNetworkAccessManager used in stellarium
	QNetworkAccessManager* getNetworkAccessManager() {return networkAccessManager;}
	
	//! Update translations, font for GUI and sky everywhere in the program.
	void updateI18n();	
	
	//! Update and reload sky culture informations everywhere in the program.
	void updateSkyCulture();	
	
	//! Retrieve the full path of the current configuration file.
	//! @return the full path of the configuration file
	const QString& getConfigFilePath() { return configFile; }
	
	//! Return the main configuration options
	QSettings* getSettings() {return confSettings;}

	//! Return the currently used style
	const StelStyle* getCurrentStelStyle() {return currentStelStyle;}
	
	///////////////////////////////////////////////////////////////////////////
	// Deprecated methods
	//! Set the time multiplier used when fast forwarding scripts.
	//! n.b. - do not confuse this with sky time rate
	//! @param multiplier new value for the time multiplier
	void setTimeMultiplier(const int multiplier) { timeMultiplier = multiplier; }

	//! Get the time multiplier used when fast forwarding scripts.
	//! n.b. - do not confuse this with sky time rate
	//! @return the integer time multiplier
	const int getTimeMultiplier() { return timeMultiplier; }
	
	///////////////////////////////////////////////////////////////////////////
	// Scriptable methods
public slots:
	//! Return the full name of stellarium, i.e. "Stellarium 0.9.0".
	static QString getApplicationName();
	
	//! Set flag for activating night vision mode.
	void setVisionModeNight(bool);
	//! Get flag for activating night vision mode.
	bool getVisionModeNight() const {return flagNightVision;}
	
	//! Get the current number of frame per second.
	//! @return the FPS averaged on the last second
	float getFps() const {return fps;}

	//! Return the time since when stellarium is running in second.
	static double getTotalRunTime();
	
private:
	//! Update all object according to the deltaTime in seconds.
	void update(double deltaTime);

	//! Draw all registered StelModule in the order defined by the order lists.
	//! @return the max squared distance in pixels that any object has travelled since the last update.
	void draw();
	
	//! Handle mouse clics.
	void handleClick(class QMouseEvent* event);
	//! Handle mouse wheel.
	void handleWheel(class QWheelEvent* event);
	//! Handle mouse move.
	void handleMove(int x, int y, Qt::MouseButtons b);
	//! Handle key press and release.
	void handleKeys(class QKeyEvent* event);

	//! Call this when the size of the GL window has changed.
	void glWindowHasBeenResized(int w, int h);

	//! Set the colorscheme for all the modules
	void setColorScheme(const QString& section);
	
	//! Sets the name of the configuration file.
	//! It is possible to set the configuration by passing either a full path
	//! a relative path of an existing file, or path segment which will be appended
	//! to the serach path.  The configuration file must be writable, or there will
	//! be trouble!
	//! @param configName the name or full path of the configuration file
	void setConfigFile(const QString& configName);
	
	//! Copies the default configuration file.
	//! This function copies the default_config.ini file to config.ini (or other
	//! name specified on the command line located in the user data directory.
	void copyDefaultConfigFile();

	//! Somewhere to save the command line arguments
	QStringList* argList;
	
	//! Check if a QStringList has a CLI-style option in it (before the first --).
	//! @param args a list of strings, think argv
	//! @param shortOpt a short-form option string, e.g, "-h"
	//! @param longOpt a long-form option string, e.g. "--help"
	//! @return true if the option exists in args before any element which is "--"
	bool argsGetOption(QStringList* args, QString shortOpt, QString longOpt);
	
	//! Retrieve the argument to an option from a QStringList.
	//! Given a list of strings, this function will extract the argument of 
	//! type T to an option, where the option in an element which matches
	//! either the short or long forms, and the argument to that option
	//! is the following element in the list, e.g. ("--option", "arg").
	//! It is also possible to extract argument to options which are
	//! part of the option element, separated by the "=" character, e.g.
	//! ( "--option=arg" ).
	//! Type conversion is done using the QTextStream class, and as such
	//! possible types which this template function may use are restricted
	//! to those where there is a value operator<<() defined in the 
	//! QTextStream class for that type.
	//! The argument list is only processed as far as the first value "--".
	//! If an argument "--" is to be retrieved, it must be apecified using
	//! the "--option=--" form.
	//! @param args a list of strings, think argv.
	//! @param shortOpt the short form of the option, e.g. "-n".
	//! @param longOpt the long form of the option, e.g. "--number".
	//! @param defaultValue the default value to return if the option was
	//! not found in args.
	//! @exception runtime_error("no_optarg") the expected argument to the 
	//! option was not found.
	//! @exception runtime_error("optarg_type") the expected argument to 
	//! the option could not be converted.
	//! @return The value of the argument to the specified option which 
	//! occurs before the first element with the value "--".  If the option 
	//! is not found, defaultValue is returned.
	template<class T>
			T argsGetOptionWithArg(QStringList* args, QString shortOpt, QString longOpt, T defaultValue);
	
	//! Check if a QStringList has a yes/no CLI-style option in it, and 
	//! find out the argument to that parameter.
	//! e.g. option --use-foo can have parameter "yes" or "no"
	//! It is also possible for the argument to take values, "1", "0"; 
	//! "true", "false";
	//! @param args a list of strings, think argv
	//! @param shortOpt a short-form option string, e.g, "-h"
	//! @param longOpt a long-form option string, e.g. "--help"
	//! @param defaultValue the default value to return if the option was 
	//! not found in args.
	//! @exception runtime_error("no_optarg") the expected argument to the 
	//! option was not found. The longOpt value is appended in parenthesis.
	//! @exception runtime_error("optarg_type") the expected argument to 
	//! the option could not be converted. The longOpt value is appended 
	//! in parenthesis.
	//! @return 1 if the argument to the specified opion is "yes", "y", 
	//! "true", "on" or 1; 0 if the argument to the specified opion is "no", 
	//! "n", "false", "off" or 0; the value of the defaultValue parameter if 
	//! the option was not found in the argument list before an element which 
	//! has the value "--".
	int argsGetYesNoOption(QStringList* args, QString shortOpt, QString longOpt, int defaultValue);
	
	//! Processing of command line options which is to be done before config file is read.
	//! This includes the chance to set the configuration file name.  It is to be done
	//! in the sub-class of the StelApp, as the sub-class may want to manage the 
	//! argument list, as is the case with the StelMainWindow version.
	void parseCLIArgsPreConfig(void);	

	//! Processing of command line options which is to be done after the config file is
	//! read.  This gives us the chance to over-ride settings which are in the configuration
	//! file.
	void parseCLIArgsPostConfig();

	// The StelApp singleton
	static StelApp* singleton;

	// The associated StelCore instance
	StelCore* core;

	// Module manager for the application
	StelModuleMgr* moduleMgr;
	
	// Font manager for the application
	StelFontMgr* fontManager;
	
	// Locale manager for the application
	StelLocaleMgr* localeMgr;
	
	// Sky cultures manager for the application
	StelSkyCultureMgr* skyCultureMgr;
	
	// Textures manager for the application
	StelTextureMgr* textureMgr;
	
	// Manager for all the StelObjects of the program
	StelObjectMgr* stelObjectMgr;

	// Manager for the list of observer locations on planets
	LocationMgr* planetLocationMgr;
	
	//! Utility class for file operations, mainly locating files by name
	StelFileMgr* stelFileMgr;
	
	// Main network manager used for the program
	QNetworkAccessManager* networkAccessManager;
	
	// The main loading bar
	LoadingBar* loadingBar;
	
	// Currently used StelStyle
	StelStyle* currentStelStyle;
	
	float fps;
	//! The minimum desired frame rate in frame per second.
	float minfps;
	//! The maximum desired frame rate in frame per second.
	float maxfps;
	
	int frame;
	double timefr, timeBase;		// Used for fps counter

	int timeMultiplier;	// used for adjusting deltaTime for script speeds
	
	//! Define whether we are in night vision mode
	bool flagNightVision;
	
	QString configFile;
	QSettings* confSettings;
	
	// Define whether the StelApp instance has completed initialization
	bool initialized;
	
	static QTime* qtime;
	
	// Temporary variables used to store the last gl window resize
	// if the core was not yet initialized
	int saveProjW;
	int saveProjH;
};

#endif // _STELAPP_HPP_
