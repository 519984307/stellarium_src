/*
 * Stellarium
 * Copyright (C) 2008 Stellarium Developers
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

#ifndef _STELFILEMGR_HPP
#define _STELFILEMGR_HPP_

#define CHECK_FILE "data/ssystem.ini"

#include <stdexcept>
#include <QSet>
#include <QString>
#include <QStringList>

//! Provides utilities for locating and handling files.
//! StelFileMgr provides functions for locating files.  It maintains a list of 
//! directories in which to look for files called the search path. Typcially this
//! includes the Stellarium installation directory, and a per-user settings 
//! directory (on platforms which support it).
//! The concept is that the StelFileMgr will be asked for a named path, and it
//! will try to locate that path within each of the search directories.
//! @author Lippo Huhtala <lippo.huhtala@meridea.com>
//! @author Matthew Gates <matthew@porpoisehead.net>
class StelFileMgr
{
public:
	//! @enum Flags used as named bitfield flags as specifiers to filter results of StelFileMgr methods.
	enum Flags {
		RemovableMedia = 0x00000001,  //!< Search on removable media if present (default is not to).
		Writable       = 0x00000002,  //!< Only return writable paths. For directories this means
		                              //!< that it is possible to create files within the directory.
		Directory      = 0x00000004, //!< Exclude non-directories.
		File           = 0x00000008, //!< Exclude non-files.
		New            = 0x00000010, //!< Exclude existing paths.
  		Hidden         = 0x00000020  //!< Include "hidden" paths (starting with a . on POSIX systems).
	};
				
	//! Constructor.
	//! By default, StelFileMgr will be created with the Stellarium installation directory
	//! config_root in the search path.  On systems which provide a per-user data/settings
	//! directory (which we call the user_settings directory, this is also included in 
	//! the search path, before the <config_root> directory.
	StelFileMgr();
	
	//! Destructor.
	~StelFileMgr();

	//! Search for a path within the search paths, for example "textures/fog.png".
	//! findFile looks through the search paths in order, returning the first instance
	//! of the specified path.  By specifying a flags parameter it is possible to constrain
	//! the results to those matching various criteria.
	//! If the path argument is a complete path (is a full path on single root OSes, or
	//! unanbigiously identifies one and only one file on multi-root OSes), it will 
	//! be tested for compliance with other conditions - the regular search path will
	//! not be tested.
	//! If you wish to search for a non-exiting file which is not in the search path 
	//! you should explicitly prefix it with "./", or otherwise have a . at the start of
	//! the path parameter, e.g. path="./my_config_file_in_the_pwd.ini"
	//! @param path the name of the file to search for, for example "textures/fog.png".
	//! @param flags options which constrain the result.
	//! @return returns a full path of the file if found, else return an empty path.
	//! @exception std::exception what() -> "file not found: [filename]"
	//! @exception std::exception what() -> "file does not match flags: [fullpath]".
	//! 		This exception occurs if a full path is passes at the path argument, but 
	//!		that path does not match the flags specified.
	QString findFile(const QString& path, const Flags& flags=(Flags)0);
	
	//! Set a set of all possible files/directories in any Stellarium search directory
	//! @param path the path to search inside, e.g. "landscapes"
	//! @param flags options which constrain the result
	//! @return returns a std::set of file and directory names (just the basename, 
	//!         not the whole path), which are available in any of the search
	//!         paths + path.  Returns empty list if none were found or the path
	//!         is invalid (not a directory / not existing)
	QSet<QString> listContents(const QString& path, const Flags& flags=(Flags)0);
		
	//! Get a vector of strings which describes the current search paths.
	//! @return returns a vector of strings representing the current search paths.
	const QStringList& getSearchPaths(void) { return fileLocations; }
	
	//! Set the search paths.
	//! @param paths is a vector of strings which will become the new search paths
	void setSearchPaths(const QStringList& paths);
		
	//! Check if a path exists.  Note it might be a file or a directory.
	//! @param path to check
	static bool exists(const QString& path);
	
	//! Check if a path is writable
	//! For files, true is returned if the file exists and is writable
	//! or if the file doesn't exist, but it's parent directory does,
	//! if the file can be created.
	//! In the case of directories, return true if the directory can
	//! have files created in it.
	//! @param path to check
	static bool isWritable(const QString& path);
	
	//! Check if a path exists and is a directory.
	//! @param path to check
	static bool isDirectory(const QString& path);
	
	//! Return the size of the file at the path.
	//! @param path to file
	static qint64 size(const QString& path);
	
	//! Make a directory
	//! @param path the path of the directory to create.
	//! @return true if success, else false
	static bool mkDir(const QString& path);
	
	//! Convenience function to find the parent directory of a given path
	//! May return relative paths if the parameter is a relative path
	//! @param path the path whose parent directory is to be returned
	QString dirName(const QString& path);
	
	//! Get the user's Desktop directory
	//! This is a portable way to retrieve the directory for the user's desktop.
	//! On Linux and OSX this is $HOME/Desktop.  For Windows, the system is queried
	//! using SHGetSpecialFolderLocation.  If that doesn't work, the USERPROFILE
	//! environment variable is checked, and if set, \\Desktop is appended, else
	//! C:\\Windows\\Desktop is used.
	//! @return the path to the user's desktop directory
	//! @exception NOT_FOUND when the directory cannot be determined, or the
	//!            OS doesn't provide one.
	QString getDesktopDir(void);
	
	//! Returns the path to the user directory
	//! This is the directory where we expect to find the [default] writable 
	//! configuration file, user versions of scripts, nebulae, stars, skycultures etc.
	//! It will be the first directory in the path which is used when
	//! trying to find most data files
	//! @return the path to the user private data directory	
	//! @exception NOT_FOUND if the directory could not be found
	QString getUserDir(void);

	//! Sets the user directory.  This updates the search paths (first element)
	//! @param newDir the new value of the user directory
	//! @exception NOT_VALID if the specified user directory is not usable
	void setUserDir(const QString& newDir);
	
	//! This is the directory into which screenshots will be saved
	//! It is $HOME on Linux, BSD, Solaris etc.
	//! It is the user's Desktop on MacOS X (??? - someone please verify this)
	//! It is ??? on Windows
	//! @return the path to the directory where screenshots are saved
	//! @exception NOT_FOUND if the directory could not be found
	QString getScreenshotDir(void);

	//! Sets the screenshot directory.
	//! This is set to platform-specific values in the StelFileMgr constructor,
	//! but it is settable using this function to make it possible to implement
	//! the command-line option which specifies where screenshots go.
	//! @param newDir the new value of the screenshot directory
	void setScreenshotDir(const QString& newDir);
		
	//! get the directory for locate files (i18n)
	//! @return the path to the locale directory or "" if the locale directory could not be found.
	QString getLocaleDir(void);

private:
	//! Check if the user directory exists, is writable and a directory
	//! Creates it if it does not exist.  Exits the program if any of this
	//! process fails.
	void checkUserDir();
	
	//! Convenience function to find the basename of a given path
	//! May return relative paths if the parameter is a relative path
	//! @param path the path whose parent directory is to be returned
	QString baseName(const QString& path);
	
	//! Returns the path to the installation directory
	//! This is the directory where we expect to find scripts, nebulae, stars, 
	//! skycultures etc, and will be added at the end of the search path
	//! @return the path to the installation data directory	
	//! @exception NOT_FOUND if the directory could not be found
	QString getInstallationDir(void);
	
	//! Check if a (complete) path matches a set of flags
	//! @param path a complete path
	//! @param flags a set of StelFileMgr::Flags to test against path
	//! @return true if path passes all flag tests, else false
	//! @exception misc 
	bool fileFlagsCheck(const QString& path, const Flags& flags=(Flags)0);
	
	QStringList fileLocations;

	//! Used to store the user data directory
	QString userDir;

	//! Used to store the screenshot directory
	QString screenshotDir;

#if defined(WIN32)
	//! For internal use - retreives windows special named directories.
	//! @param csidlId identifier for directoy, e.g. CSIDL_APPDATA
	static QString getWin32SpecialDirPath(const int csidlId);
#endif

};

#endif // _STELFILEMGR_HPP_
