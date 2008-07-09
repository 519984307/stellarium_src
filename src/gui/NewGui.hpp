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

#ifndef _NEWGUI_HPP_
#define _NEWGUI_HPP_

#include "StelModule.hpp"
#include "LocationDialog.hpp"
#include "ViewDialog.hpp"
#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "HelpDialog.hpp"
#include "DateTimeDialog.hpp"
#include "SearchDialog.hpp"
#include "ConfigurationDialog.hpp"
#include <QGraphicsPixmapItem>
#include <QDebug>

class QGraphicsSceneMouseEvent;
class QTimeLine;
class QAction;
class QGraphicsTextItem;
class QTimer;

//! Implement a button for use in Stellarium's graphic widgets
class StelButton : public QObject, public QGraphicsPixmapItem
{
	friend class BottomStelBar;
	
	Q_OBJECT;
public:
	//! Constructor
	//! @param parent the parent item
	//! @param pixOn the pixmap to display when the button is toggled
	//! @param pixOff the pixmap to display when the button is not toggled
	//! @param pixHover a pixmap slowly blended when mouse is over the button
	//! @param groupName the name of a button group in which to add the button. If the group doesn't exist, create a new group.
	//! @param action the associated action. Connections are automatically done with the signals if relevant.
	//! @param helpLabel the label in which the button will display it's help when hovered
	StelButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff, const QPixmap& pixHover=QPixmap(),
			   QAction* action=NULL, QGraphicsSimpleTextItem* helpLabel=NULL, bool noBackground=false);

	//! Get whether the button is checked
	bool isChecked() const {return checked;}

signals:
	//! Triggered when the button state changes
	void toggled(bool);
	//! Triggered when the button state changes
	void triggered();

public slots:
	//! set whether the button is checked
	void setChecked(bool b);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
private slots:
	void animValueChanged(qreal value);	
private:
	QPixmap pixOn;
	QPixmap pixOff;
	QPixmap pixHover;
	QPixmap pixBackground;
	bool checked;
	QTimeLine* timeLine;
	QAction* action;
	QGraphicsSimpleTextItem* helpLabel;
	bool noBckground;
};

//! The button bar on the left containing windows toggle buttons
class LeftStelBar : public QGraphicsItem
{
public:
	LeftStelBar(QGraphicsItem* parent);
	~LeftStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void addButton(StelButton* button);
private:
	QTimeLine* hideTimeLine;
};

//! The button bar on the bottom containing actions toggle buttons
class BottomStelBar : public QGraphicsItem
{
public:
	BottomStelBar(QGraphicsItem* parent, const QPixmap& pixLeft=QPixmap(), const QPixmap& pixRight=QPixmap(), const QPixmap& pixMiddle=QPixmap(), const QPixmap& pixSingle=QPixmap());
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	//! Add a button in a group in the button bar. Group are displayed in alphabetic order.
	//! @param b the button to add
	//! @param groupName the name of the button group to which the button belongs to. If the group doesn't exist yet, a new one is created.
	void addButton(StelButton* button, const QString& groupName="defaultGroup");
	//! Remove the button associated with the action of the passed name
	void removeButton(const QString& actionName);
	//! Set the color for all the sub elements
	void setColor(const QColor& c);
private:
	void updateText();
	void updateButtonsGroups();
	QRectF getButtonsBoundingRect();
	QGraphicsSimpleTextItem* location;
	QGraphicsSimpleTextItem* datetime;
	QGraphicsSimpleTextItem* fov;
	QGraphicsSimpleTextItem* fps;
	
	QMap<QString, QList<StelButton*> > buttonGroups;
	QPixmap pixBackgroundLeft;
	QPixmap pixBackgroundRight;
	QPixmap pixBackgroundMiddle;
	QPixmap pixBackgroundSingle;
};

//! The informations about the currently selected object
class InfoPanel : public QGraphicsItem
{
public:
	InfoPanel(QGraphicsItem* parent);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void setInfoTextFilters(const StelObject::InfoStringGroup& flags) {infoTextFilters=flags;}
	const StelObject::InfoStringGroup& getInfoTextFilters(void) const {return infoTextFilters;}

private:
	QGraphicsTextItem* text;
	StelObjectP object;
	StelObject::InfoStringGroup infoTextFilters;
};

//! The path around the bottom left button bars
class StelBarsPath : public QGraphicsPathItem
{
public:
	StelBarsPath(QGraphicsItem* parent);
	void updatePath(BottomStelBar* bot, LeftStelBar* lef);
	double getRoundSize() const {return roundSize;}
private:
	double roundSize;
};

//! Progess bars in the lower right corner
class StelProgressBarMgr : public QObject, public QGraphicsItem
{
	Q_OBJECT;
public:
	StelProgressBarMgr(QGraphicsItem* parent);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	class QProgressBar* addProgressBar();
private slots:
	void oneDestroyed(QObject* obj);
private:
	void updateBarsPositions();
};

//! @class NewGui
//! New GUI based on QGraphicView
class NewGui : public StelModule
{
	Q_OBJECT;
public:
	friend class ViewDialog;
	
	NewGui();
	virtual ~NewGui();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the NewGui object.
	virtual void init();
	
	//! Draws
	virtual void draw(StelCore* core) {;}
	
	//! Update state which is time dependent.
	virtual void update(double deltaTime);
	
	//! Update i18 names from English names according to passed translator.
	//! The translation is done using gettext with translated strings defined 
	//! in translations.h
	virtual void updateI18n();
	
	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	virtual void glWindowHasBeenResized(int w, int h);

	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);

	//! Load color scheme from the given ini file and section name
	//! @param conf application settings object
	//! @param section in the application settings object which contains desired color scheme
	virtual void setColorScheme(const QSettings* conf, const QString& section);
	
	//! Load a Qt style sheet to define the widgets style
	void loadStyle(const QString& fileName);

	//! Get a pointer on the info panel used to display selected object info
	InfoPanel* getInfoPanel(void) { return infoPanel; }
	
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgessBar();
	
private slots:
	void updateBarsPos(qreal value);
	
	//! Reload the current Qt Style Sheet (Debug only)
	void reloadStyle();
	
private:
	void addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable=true, bool autoRepeat=false);
	QAction* getGuiActions(const QString& actionName);
	void retranslateUi(QWidget *Form);
	
	LeftStelBar* winBar;
	BottomStelBar* buttonBar;
	InfoPanel* infoPanel;
	StelBarsPath* buttonBarPath;
	QGraphicsSimpleTextItem* buttonHelpLabel;

	QTimeLine* animLeftBarTimeLine;
	QTimeLine* animBottomBarTimeLine;
	
	StelButton* buttonTimeRewind;
	StelButton* buttonTimeRealTimeSpeed;
	StelButton* buttonTimeCurrent;
	StelButton* buttonTimeForward;
	
	StelButton* buttonGotoSelectedObject;
	
	LocationDialog locationDialog;
	HelpDialog helpDialog;
	DateTimeDialog dateTimeDialog;
	SearchDialog searchDialog;
	ViewDialog viewDialog;
	ConfigurationDialog configurationDialog;
	
	StelProgressBarMgr* progressBarMgr;
};

#endif // _NEWGUI_HPP_
