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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelMainGraphicsView.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelGuiBase.hpp"
#include "StelMainWindow.hpp"

#include <QGLFormat>
#include <QPaintEngine>
#include <QGraphicsView>
#include <QGLWidget>
#include <QResizeEvent>
#include <QSettings>
#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>
#include <QEventLoop>
#include <QPluginLoader>
#include <QtPlugin>
#include <QThread>
#include <QTimer>

#ifndef DISABLE_SCRIPTING
 #include "StelScriptMgr.hpp"
 #include "StelMainScriptAPIProxy.hpp"
#endif

// The static plugins need to be imported here so that they belong to the
// libStelMain required on win32.
Q_IMPORT_PLUGIN(StelGui)

#ifdef USE_STATIC_PLUGIN_VIRGO
Q_IMPORT_PLUGIN(VirGO)
#endif

#ifdef USE_STATIC_PLUGIN_SVMT
Q_IMPORT_PLUGIN(SVMT)
#endif

#ifdef USE_STATIC_PLUGIN_HELLOSTELMODULE
Q_IMPORT_PLUGIN(HelloStelModule)
#endif

#ifdef USE_STATIC_PLUGIN_ANGLEMEASURE
Q_IMPORT_PLUGIN(AngleMeasure)
#endif

#ifdef USE_STATIC_PLUGIN_COMPASSMARKS
Q_IMPORT_PLUGIN(CompassMarks)
#endif

#ifdef USE_STATIC_PLUGIN_SATELLITES
Q_IMPORT_PLUGIN(Satellites)
#endif

#ifdef USE_STATIC_PLUGIN_TEXTUSERINTERFACE
Q_IMPORT_PLUGIN(TextUserInterface)
#endif

#ifdef USE_STATIC_PLUGIN_LOGBOOK
Q_IMPORT_PLUGIN(LogBook)
#endif

#ifdef USE_STATIC_PLUGIN_OCULARS
Q_IMPORT_PLUGIN(Oculars)
#endif

#ifdef USE_STATIC_PLUGIN_TELESCOPECONTROL
Q_IMPORT_PLUGIN(TelescopeControl)
#endif

// Initialize static variables
StelMainGraphicsView* StelMainGraphicsView::singleton = NULL;

class StelQGLWidget : public QGLWidget
{
public:
	StelQGLWidget(QGLContext* ctx, QWidget* parent) : QGLWidget(ctx, parent)
	{
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
		//setAutoFillBackground(false);
		setBackgroundRole(QPalette::Window);
	}

protected:
	virtual void initializeGL()
	{
		QGLWidget::initializeGL();

		if (!format().stencil())
			qWarning("Could not get stencil buffer; results will be suboptimal");
		if (!format().depth())
			qWarning("Could not get depth buffer; results will be suboptimal");
		if (!format().doubleBuffer())
			qWarning("Could not get double buffer; results will be suboptimal");

		QString paintEngineStr;
		switch (paintEngine()->type())
		{
		case QPaintEngine::OpenGL:
			paintEngineStr = "OpenGL";
			break;
		case QPaintEngine::OpenGL2:
			paintEngineStr = "OpenGL2";
			break;
		default:
			paintEngineStr = "Other";
		}
		qDebug() << "Qt GL paint engine is: " << paintEngineStr;
	}
};

StelMainGraphicsView::StelMainGraphicsView(QWidget* parent)
	: QGraphicsView(parent), backItem(NULL), gui(NULL),
#ifndef DISABLE_SCRIPTING
	scriptAPIProxy(NULL), scriptMgr(NULL),
#endif
	  wasDeinit(false),
	  flagInvertScreenShotColors(false),
	  screenShotPrefix("stellarium-"),
	  screenShotDir(""),
	  cursorTimeout(-1.f), flagCursorTimeout(false), minFpsTimer(NULL), maxfps(10000.f)
{
	StelApp::initStatic();

	// Can't create 2 StelMainGraphicsView instances
	Q_ASSERT(!singleton);
	singleton = this;

	setObjectName("Mainview");

	// Avoid white background at init
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::Window);
	QPalette pal;
	pal.setColor(QPalette::Window, Qt::black);
	setPalette(pal);

	// Allows for precise FPS control
	setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFocusPolicy(Qt::StrongFocus);
	connect(this, SIGNAL(screenshotRequested()), this, SLOT(doScreenshot()));

	lastEventTimeSec = 0;

	// Create an openGL viewport
	QGLFormat glFormat(QGL::StencilBuffer | QGL::DepthBuffer | QGL::DoubleBuffer);
	glContext = new QGLContext(glFormat);
	glWidget = new StelQGLWidget(glContext, this);
	glWidget->updateGL();
	setViewport(glWidget);

	// This line seems to cause font aliasing troubles on win32
	// setOptimizationFlags(QGraphicsView::DontSavePainterState);

	setScene(new QGraphicsScene(this));
	scene()->setItemIndexMethod(QGraphicsScene::NoIndex);

	backItem = new QGraphicsWidget();
	backItem->setFocusPolicy(Qt::NoFocus);
}

StelMainGraphicsView::~StelMainGraphicsView()
{
}

void StelMainGraphicsView::init(QSettings* conf)
{
	Q_ASSERT(glWidget->isValid());
	glWidget->makeCurrent();

	// Create the main widget for stellarium, which in turn creates the main StelApp instance.
	mainSkyItem = new StelAppGraphicsWidget();
	mainSkyItem->setZValue(-10);
	QGraphicsGridLayout* l = new QGraphicsGridLayout(backItem);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	l->addItem(mainSkyItem, 0, 0);
	scene()->addItem(backItem);

	// Activate the resizing caused by the layout
	QCoreApplication::processEvents();

	mainSkyItem->setFocus();

	flagInvertScreenShotColors = conf->value("main/invert_screenshots_colors", false).toBool();
	setFlagCursorTimeout(conf->value("gui/flag_mouse_cursor_timeout", false).toBool());
	setCursorTimeout(conf->value("gui/mouse_cursor_timeout", 10.f).toFloat());
	maxfps = conf->value("video/maximum_fps",10000.f).toFloat();
	minfps = conf->value("video/minimum_fps",10000.f).toFloat();

	StelPainter::initSystemGLInfo(glContext);

	QPainter qPainter(glWidget);
	StelPainter::setQPainter(&qPainter);

	// Initialize the core, including the StelApp instance.
	mainSkyItem->init(conf);
	// Prevent flickering on mac Leopard/Snow Leopard
	glWidget->setAutoFillBackground (false);

#ifndef DISABLE_SCRIPTING
	scriptAPIProxy = new StelMainScriptAPIProxy(this);
	scriptMgr = new StelScriptMgr(this);
#endif

	// Look for a static GUI plugins.
	foreach (QObject *plugin, QPluginLoader::staticInstances())
	{
		StelGuiPluginInterface* pluginInterface = qobject_cast<StelGuiPluginInterface*>(plugin);
		if (pluginInterface)
		{
			gui = pluginInterface->getStelGuiBase();
		}
		break;
	}
	Q_ASSERT(gui);	// There was no GUI plugin found

	StelApp::getInstance().setGui(gui);
	gui->init(backItem, mainSkyItem);
	StelApp::getInstance().initPlugIns();

	// Force refreshing of button bars if plugins modified the GUI, e.g. added buttons.
	gui->forceRefreshGui();

#ifndef DISABLE_SCRIPTING
	QString startupScript;
	if (qApp->property("onetime_startup_script").isValid())
		qApp->property("onetime_startup_script").toString();
	else
		startupScript = conf->value("scripts/startup_script", "startup.ssc").toString();
	scriptMgr->runScript(startupScript);
#endif

	QThread::currentThread()->setPriority(QThread::HighestPriority);
	StelPainter::setQPainter(NULL);
	startMainLoop();
}

void StelMainGraphicsView::thereWasAnEvent()
{
	lastEventTimeSec = StelApp::getTotalRunTime();
}

void StelMainGraphicsView::drawBackground(QPainter*, const QRectF&)
{
	const double now = StelApp::getTotalRunTime();

	// Determines when the next display will need to be triggered
	// The current policy is that after an event, the FPS is maximum for 2.5 seconds
	// after that, it switches back to the default minfps value to save power
	if (now-lastEventTimeSec<2.5)
	{
		double duration = 1./getMaxFps();
		int dur = (int)(duration*1000);
		QTimer::singleShot(dur<5 ? 5 : dur, scene(), SLOT(update()));
	}
	#ifdef QT_MAC_USE_COCOA
		// This call solves the problems with the qt event dispatcher in Cocoa. The stack grew huge and many events were discarded
		// http://bugreports.qt.nokia.com/browse/QTBUG-7502
		QCoreApplication::processEvents(QEventLoop::AllEvents);
	#endif
	// Manage cursor timeout
	if (cursorTimeout>0.f && (now-lastEventTimeSec>cursorTimeout) && flagCursorTimeout)
	{
		if (QApplication::overrideCursor()==0)
			QApplication::setOverrideCursor(Qt::BlankCursor);
	}
	else
	{
		if (QApplication::overrideCursor()!=0)
			QApplication::restoreOverrideCursor();
	}
}

void StelMainGraphicsView::startMainLoop()
{
	// Set a timer refreshing for every minfps frames
	minFpsChanged();
}

void StelMainGraphicsView::minFpsChanged()
{
	if (minFpsTimer!=NULL)
	{
		disconnect(minFpsTimer, SIGNAL(timeout()), 0, 0);
		delete minFpsTimer;
		minFpsTimer = NULL;
	}

	minFpsTimer = new QTimer(this);
	connect(minFpsTimer, SIGNAL(timeout()), scene(), SLOT(update()));
	minFpsTimer->start((int)(1./getMinFps()*1000.));
}

void StelMainGraphicsView::resizeEvent(QResizeEvent* event)
{
	scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
	backItem->setGeometry(0,0,event->size().width(),event->size().height());
	QGraphicsView::resizeEvent(event);
}

void StelMainGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::mouseMoveEvent(event);
}


void StelMainGraphicsView::mousePressEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::mousePressEvent(event);
}

void StelMainGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::mouseReleaseEvent(event);
}

void StelMainGraphicsView::wheelEvent(QWheelEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::wheelEvent(event);
}

void StelMainGraphicsView::keyPressEvent(QKeyEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::keyPressEvent(event);
}

void StelMainGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP
	QGraphicsView::keyReleaseEvent(event);
}

//! Delete openGL textures (to call before the GLContext disappears)
void StelMainGraphicsView::deinitGL()
{
	// Can be called only once
	if (wasDeinit==true)
		return;
	wasDeinit = true;
#ifndef DISABLE_SCRIPTING
	if (scriptMgr->scriptIsRunning())
		scriptMgr->stopScript();
#endif
	QCoreApplication::processEvents();
	StelApp::getInstance().getModuleMgr().unloadAllPlugins();
	QCoreApplication::processEvents();
	delete gui;
	delete mainSkyItem;
}

void StelMainGraphicsView::saveScreenShot(const QString& filePrefix, const QString& saveDir)
{
	screenShotPrefix = filePrefix;
	screenShotDir = saveDir;
	emit(screenshotRequested());
}

void StelMainGraphicsView::doScreenshot(void)
{
	QFileInfo shotDir;
	QImage im = glWidget->grabFrameBuffer();
	if (flagInvertScreenShotColors)
		im.invertPixels();

	if (screenShotDir == "")
		shotDir = QFileInfo(StelFileMgr::getScreenshotDir());
	else
		shotDir = QFileInfo(screenShotDir);

	if (!shotDir.isDir())
	{
		qWarning() << "ERROR requested screenshot directory is not a directory: " << shotDir.filePath();
		return;
	}
	else if (!shotDir.isWritable())
	{
		qWarning() << "ERROR requested screenshot directory is not writable: " << shotDir.filePath();
		return;
	}

	QFileInfo shotPath;
	for (int j=0; j<100000; ++j)
	{
		shotPath = QFileInfo(shotDir.filePath() + "/" + screenShotPrefix + QString("%1").arg(j, 3, 10, QLatin1Char('0')) + ".png");
		if (!shotPath.exists())
			break;
	}

	qDebug() << "INFO Saving screenshot in file: " << shotPath.filePath();
	if (!im.save(shotPath.filePath())) {
		qWarning() << "WARNING failed to write screenshot to: " << shotPath.filePath();
	}
}
