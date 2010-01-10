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
#include "StelScriptMgr.hpp"
#include "StelViewportDistorter.hpp"
#include "StelPainter.hpp"
#include "StelGuiBase.hpp"
#include "StelMainScriptAPIProxy.hpp"

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

Q_IMPORT_PLUGIN(StelGui)

// Initialize static variables
StelMainGraphicsView* StelMainGraphicsView::singleton = NULL;

StelMainGraphicsView::StelMainGraphicsView(QWidget* parent)
	: QGraphicsView(parent), backItem(NULL), gui(NULL), scriptAPIProxy(NULL), scriptMgr(NULL),
	  wasDeinit(false),
	  flagInvertScreenShotColors(false),
	  screenShotPrefix("stellarium-"),
	  screenShotDir(""),
	  cursorTimeout(-1.f), flagCursorTimeout(false), distorter(NULL), minFpsTimer(NULL), maxfps(10000.f)
{
	// Can't create 2 StelMainGraphicsView instances
	Q_ASSERT(!singleton);
	singleton = this;

	setObjectName("Mainview");

	// Avoid white background at init
	setStyleSheet(QString("QGraphicsView {background: #000;}"));
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFocusPolicy(Qt::StrongFocus);
	// Allows for precise FPS control
	setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	connect(this, SIGNAL(screenshotRequested()), this, SLOT(doScreenshot()));

	qtime = new QTime();
	qtime->start();
	lastEventTimeSec = 0;

	// Create an openGL viewport
	QGLFormat glFormat(QGL::StencilBuffer | QGL::DepthBuffer);
	//glFormat.setSamples(16);
	//glFormat.setSampleBuffers(true);
	//glFormat.setDirectRendering(false);
	glWidget = new QGLWidget(glFormat, this);
	//glWidget->setAttribute(Qt::WA_PaintOnScreen);
	//glWidget->setAttribute(Qt::WA_NoSystemBackground);

	if (!glWidget->format().stencil())
		qWarning("Could not get stencil buffer; results will be suboptimal");
	if (!glWidget->format().depth())
		qWarning("Could not get depth buffer; results will be suboptimal");

	setViewport(glWidget);

	//setOptimizationFlags(QGraphicsView::DontClipPainter|QGraphicsView::DontSavePainterState|QGraphicsView::DontAdjustForAntialiasing);
	setScene(new QGraphicsScene());

	backItem = new QGraphicsWidget();
	backItem->setFocusPolicy(Qt::NoFocus);
}

StelMainGraphicsView::~StelMainGraphicsView()
{
	if (distorter)
	{
		delete distorter;
		distorter=NULL;
	}
}

void StelMainGraphicsView::swapBuffer()
{
	glWidget->swapBuffers();
}

void StelMainGraphicsView::makeGLContextCurrent()
{
	Q_ASSERT(glWidget!=NULL);
	Q_ASSERT(glWidget->isValid());
	glWidget->makeCurrent();
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
	setCursorTimeout(conf->value("gui/mouse_cursor_timeout", 10.).toDouble());
	maxfps = conf->value("video/maximum_fps",10000.).toDouble();
	minfps = conf->value("video/minimum_fps",10000.).toDouble();
	setViewPortDistorterType(conf->value("video/distorter","none").toString());
	//distorter = StelViewportDistorter::create("none",800,600,StelProjectorP());

	QPainter qPainter(glWidget);
	StelPainter::setQPainter(&qPainter);

	// Initialize the core, including the StelApp instance.
	mainSkyItem->init(conf);

	scriptAPIProxy = new StelMainScriptAPIProxy(this);
	scriptMgr = new StelScriptMgr(this);

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

	gui->init(backItem, mainSkyItem);
	StelApp::getInstance().setGui(gui);
	StelApp::getInstance().initPlugIns();

	// Force refreshing of button bars if plugins modified the GUI, e.g. added buttons.
	gui->forceRefreshGui();

	const QString& startupScript = conf->value("scripts/startup_script", "startup.ssc").toString();
	scriptMgr->runScript(startupScript);

	QThread::currentThread()->setPriority(QThread::HighestPriority);
	StelPainter::setQPainter(NULL);
	startMainLoop();
}

void StelMainGraphicsView::thereWasAnEvent()
{
	lastEventTimeSec = StelApp::getTotalRunTime();
}

void StelMainGraphicsView::drawBackground(QPainter* painter, const QRectF& rect)
{
	if (painter->paintEngine()->type()!=QPaintEngine::OpenGL && painter->paintEngine()->type()!=QPaintEngine::OpenGL2)
	{
		qWarning("StelMainGraphicsView: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}

	if (distorter)
	{
		StelPainter::setQPainter(painter);
		distorter->prepare();
		StelPainter::setQPainter(NULL);
	}

	const double now = ((double)qtime->elapsed())/1000.;

	// Determines when the next display will need to be triggered
	// The current policy is that after an event, the FPS is maximum for 2.5 seconds
	// after that, it switches back to the default minfps value to save power
	if (now-lastEventTimeSec<2.5)
	{
		double duration = 1./getMaxFps();
		int dur = (int)(duration*1000);
		QTimer::singleShot(dur<5 ? 5 : dur, scene(), SLOT(update()));
	}
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
	//QGraphicsView::drawBackground(painter, rect);
}

void StelMainGraphicsView::drawForeground(QPainter* painter, const QRectF &rect)
{
	//QGraphicsView::drawForeground(painter, rect);
	if (distorter)
	{
		StelPainter::setQPainter(painter);
		distorter->distort();
		StelPainter::setQPainter(NULL);
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

	// Apply distortion on the mouse position.
	QPoint pos = event->pos();
	distortPos(&pos);
	QPoint globalPos = event->globalPos();
	distortPos(&globalPos);
	QMouseEvent newEvent(event->type(), pos, globalPos, event->button(), event->buttons(), event->modifiers());
	QGraphicsView::mouseMoveEvent(&newEvent);
}


void StelMainGraphicsView::mousePressEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP

	// Apply distortion on the mouse position.
	QPoint pos = event->pos();
	distortPos(&pos);
	QPoint globalPos = event->globalPos();
	distortPos(&globalPos);
	QMouseEvent newEvent(event->type(), pos, globalPos, event->button(), event->buttons(), event->modifiers());
	QGraphicsView::mousePressEvent(&newEvent);
}

void StelMainGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP

	// Apply distortion on the mouse position.
	QPoint pos = event->pos();
	distortPos(&pos);
	QPoint globalPos = event->globalPos();
	distortPos(&globalPos);
	QMouseEvent newEvent(event->type(), pos, globalPos, event->button(), event->buttons(), event->modifiers());
	QGraphicsView::mouseReleaseEvent(&newEvent);
}

void StelMainGraphicsView::wheelEvent(QWheelEvent* event)
{
	thereWasAnEvent(); // Refresh screen ASAP

	// Apply distortion on the mouse position.
	QPoint pos = event->pos();
	distortPos(&pos);
	QPoint globalPos = event->globalPos();
	distortPos(&globalPos);
	QWheelEvent newEvent(pos, globalPos, event->delta(), event->buttons(), event->modifiers(), event->orientation());
	QGraphicsView::wheelEvent(&newEvent);
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

void StelMainGraphicsView::setViewPortDistorterType(const QString &type)
{
	if (type != getViewPortDistorterType())
	{
		if (type == "none")
		{
			glWidget->setMaximumSize(10000,10000);
		}
		else
		{
			glWidget->setFixedSize(width(), height());
		}
	}
	if (distorter)
	{
		delete distorter;
		distorter = NULL;
	}
	if (type!="none")
		distorter = StelViewportDistorter::create(type,width(),height(),StelApp::getInstance().getCore()->getProjection2d());
}

QString StelMainGraphicsView::getViewPortDistorterType() const
{
	if (distorter)
		return distorter->getType();
	return "none";
}

void StelMainGraphicsView::distortPos(QPoint* pos)
{
	if (!distorter)
		return;
	int x = pos->x();
	int y = pos->y();
	y = height() - 1 - y;
	distorter->distortXY(x,y);
	pos->setX(x);
	pos->setY(height() - 1 - y);
}

//! Delete openGL textures (to call before the GLContext disappears)
void StelMainGraphicsView::deinitGL()
{
	// Can be called only once
	if (wasDeinit==true)
		return;
	wasDeinit = true;
	if (scriptMgr->scriptIsRunning())
		scriptMgr->stopScript();
	StelApp::getInstance().getModuleMgr().unloadAllPlugins();
	QCoreApplication::processEvents();
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
