/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef _STELMAINGRAPHICSVIEW_HPP_
#define _STELMAINGRAPHICSVIEW_HPP_

#include <QCoreApplication>
#include <QGraphicsView>
#include <QEventLoop>
#include <QOpenGLContext>
#include <QTimer>

class StelGLWidget;
class StelGraphicsScene;
class QMoveEvent;
class QResizeEvent;
class StelGuiBase;
class QMoveEvent;
class QSettings;

//! @class StelMainView
//! Reimplement a QGraphicsView for Stellarium.
//! It is the class creating the singleton GL Widget, the main StelApp instance as well as the main GUI.
class StelMainView : public QGraphicsView
{
	friend class StelGuiItem;
	friend class StelSkyItem;
	Q_OBJECT
	Q_PROPERTY(bool fullScreen READ isFullScreen WRITE setFullScreen NOTIFY fullScreenChanged)

public:
	StelMainView(QSettings* settings);
	virtual ~StelMainView();

	//! Start the main initialization of Stellarium
	void init();
	void deinit();

	//! Set the application title for the current language.
	//! This is useful for e.g. chinese.
	void initTitleI18n();

	//! Get the StelMainView singleton instance.
	static StelMainView& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Delete openGL textures (to call before the GLContext disappears)
	void deinitGL();
	//! Return focus to the sky item.  To be used when we close a dialog.
	void focusSky();
	//! Return the parent gui widget, this should be used as parent to all
	//! the StelDialog instances.
	QGraphicsWidget* getGuiWidget() const {return guiItem;}
	//! Return mouse position coordinates
	QPoint getMousePos();

	void drawEnded();
public slots:

	//! Set whether fullscreen is activated or not
	void setFullScreen(bool);

	//! Set focus on the sky
	void setFocusOnSky();

	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg saveDir changes the directory where the screenshot is saved
	//! If saveDir is "" then StelFileMgr::getScreenshotDir() will be used
	//! @arg overwrite if true, @arg filePrefix is used as filename, and existing file will be overwritten.
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="", const bool overwrite=false);

	//! Get whether colors are inverted when saving screenshot
	bool getFlagInvertScreenShotColors() const {return flagInvertScreenShotColors;}
	//! Set whether colors should be inverted when saving screenshot
	void setFlagInvertScreenShotColors(bool b) {flagInvertScreenShotColors=b;}

	//! Get whether existing files are overwritten when saving screenshot
	bool getFlagOverwriteScreenShots() const {return flagOverwriteScreenshots;}
	//! Set whether existing files are overwritten when saving screenshot
	void setFlagOverwriteScreenShots(bool b) {flagOverwriteScreenshots=b;}

	//! Get the state of the mouse cursor timeout flag
	bool getFlagCursorTimeout() {return flagCursorTimeout;}
	//! Get the mouse cursor timeout in seconds
	float getCursorTimeout() const {return cursorTimeout;}
	//! Get the state of the mouse cursor timeout flag
	void setFlagCursorTimeout(bool b) {flagCursorTimeout=b;}
	//! Set the mouse cursor timeout in seconds
	void setCursorTimeout(float t) {cursorTimeout=t;}

	//! Set the minimum frames per second. Usually this minimum will be switched to after there are no
	//! user events for some seconds to save power. However, if can be useful to set this to a high
	//! value to improve playing smoothness in scripts.
	//! @param m the new minimum fps setting.
	void setMinFps(float m) {minfps=m; minFpsTimer->setInterval(1000/minfps);}
	//! Get the current minimum frames per second.
	float getMinFps() {return minfps;}
	//! Set the maximum frames per second.
	//! @param m the new maximum fps setting.
	//! @todo this setting currently does nothing
	void setMaxFps(float m) {maxfps = m;}
	//! Get the current maximum frames per second.
	float getMaxFps() {return maxfps;}

	//! Notify that an event was handled by the program and therefore the
	//! FPS should be maximized for a couple of seconds.
	void thereWasAnEvent();

	//! Determines if we should render as fast as possible,
	//! or limit the FPS. This depends on the time the last user event
	//! happened.
	bool needsMaxFPS() const;

protected:
	//! Hack to determine current monitor pixel ratio
	//! @todo Find a better way to handle this
	virtual void moveEvent(QMoveEvent* event);
	//! Handle window closed event, calling StelApp::quit()
	virtual void closeEvent(QCloseEvent* event);
	//! Handle window resized events, and change the size of the underlying
	//! QGraphicsScene to be the same
	virtual void resizeEvent(QResizeEvent* event);

signals:
	//! emitted when saveScreenShot is requested with saveScreenShot().
	//! doScreenshot() does the actual work (it has to do it in the main
	//! thread, where as saveScreenShot() might get called from another one.
	void screenshotRequested(void);
	void fullScreenChanged(bool b);

private slots:
	// Do the actual screenshot generation in the main thread with this method.
	void doScreenshot(void);
	void updateNightModeProperty();
	void minFPSUpdate();

private:
	//! Sets the desired OpenGL format settings
	void setOpenGLFormat() const;
	//! provide extended OpenGL diagnostics in logfile.
	void dumpOpenGLdiagnostics() const;
	//! Startup diagnostics, providing test for various circumstances of bad OS/OpenGL driver combinations
	//! to provide feedback to the user about bad OpenGL drivers.
	void processOpenGLdiagnosticsAndWarnings(QSettings *conf, QOpenGLContext* context) const;

	//! The StelMainView singleton
	static StelMainView* singleton;

	QSettings* configuration;

	QGraphicsWidget* rootItem;
	QGraphicsWidget* guiItem;
	QGraphicsEffect* nightModeEffect;

	//! The openGL viewport of the graphics scene
	//! Responsible for main GL setup, rendering is done in the scene background
	StelGLWidget* glWidget;
	//! Custom QGraphicsScene, this renders our scene background
	StelGraphicsScene* stelScene;

	StelGuiBase* gui;
	class StelApp* stelApp;

	bool updateQueued;
	bool flagInvertScreenShotColors;
	bool flagOverwriteScreenshots; //! if set to true, screenshot is named exactly screenShotPrefix.png and overwrites existing file

	QString screenShotPrefix;
	QString screenShotDir;

	// Number of second before the mouse cursor disappears
	float cursorTimeout;
	bool flagCursorTimeout;

	double lastEventTimeSec;

	//! The minimum desired frame rate in frame per second.
	float minfps;
	//! The maximum desired frame rate in frame per second.
	float maxfps;
	QTimer* minFpsTimer;
};


#endif // _STELMAINGRAPHICSVIEW_HPP_
