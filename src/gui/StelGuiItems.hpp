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

#ifndef _STELGUIITEMS_HPP_
#define _STELGUIITEMS_HPP_

#include <QGraphicsPixmapItem>
#include <QDebug>

class QGraphicsSceneMouseEvent;
class QTimeLine;
class QAction;
class QGraphicsTextItem;
class QTimer;

// Progess bars in the lower right corner
class StelProgressBarMgr : public QObject, public QGraphicsItem
{
	Q_OBJECT
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

// Buttons in the bottom left corner
class CornerButtons : public QObject, public QGraphicsItem
{
Q_OBJECT
public:
	CornerButtons(QGraphicsItem* parent=NULL);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void setOpacity(double opacity);
private:
	mutable double lastOpacity;
};

//! A Button Graphicsitem for use in Stellarium's graphic widgets
class StelButton : public QObject, public QGraphicsPixmapItem
{
	friend class BottomStelBar;
	friend class LeftStelBar;
	Q_OBJECT
public:
	//! Constructor
	//! @param parent the parent item
	//! @param pixOn the pixmap to display when the button is toggled
	//! @param pixOff the pixmap to display when the button is not toggled
	//! @param pixHover a pixmap slowly blended when mouse is over the button
	//! @param action the associated action. Connections are automatically done with the signals if relevant.
	//! @param noBackground define whether the button background image have to be used
	StelButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff, const QPixmap& pixHover=QPixmap(),
			   QAction* action=NULL, bool noBackground=false);

	//! Get whether the button is checked
	bool isChecked() const {return checked;}

	//! Set the button opacity
	void setOpacity(double v) {opacity=v; updateIcon();}
	
	//! Activate red mode for this button, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b) {redMode=b; updateIcon();}
	
signals:
	//! Triggered when the button state changes
	void toggled(bool);
	//! Triggered when the button state changes
	void triggered();
	//! Emitted when the hover state change
	//! @param b true if the mouse entered the button
	void hoverChanged(bool b);

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
	void updateIcon();
	
	QPixmap pixOn;
	QPixmap pixOff;
	QPixmap pixHover;
	QPixmap pixBackground;
	
	QPixmap pixOnRed;
	QPixmap pixOffRed;
	QPixmap pixHoverRed;
	QPixmap pixBackgroundRed;
	
	bool checked;
	QTimeLine* timeLine;
	QAction* action;
	bool noBckground;
	double opacity;
	double hoverOpacity;
	
	bool redMode;
};

// The button bar on the left containing windows toggle buttons
class LeftStelBar : public QObject, public QGraphicsItem
{
	Q_OBJECT
public:
	LeftStelBar(QGraphicsItem* parent);
	~LeftStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void addButton(StelButton* button);
	QRectF boundingRectNoHelpLabel() const;
	//! Set the color for all the sub elements
	void setColor(const QColor& c);
	//! Activate red mode for the buttons, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b);
private slots:	
	//! Update the help label when a button is hovered
	void buttonHoverChanged(bool b);
private:
	QTimeLine* hideTimeLine;
	QGraphicsSimpleTextItem* helpLabel;
};

// The button bar on the bottom containing actions toggle buttons
class BottomStelBar : public QObject, public QGraphicsItem
{
	Q_OBJECT
public:
	BottomStelBar(QGraphicsItem* parent, const QPixmap& pixLeft=QPixmap(), const QPixmap& pixRight=QPixmap(), const QPixmap& pixMiddle=QPixmap(), const QPixmap& pixSingle=QPixmap());
	virtual ~BottomStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	QRectF boundingRectNoHelpLabel() const;
			
	//! Add a button in a group in the button bar. Group are displayed in alphabetic order.
	//! @param b the button to add
	//! @param groupName the name of the button group to which the button belongs to. If the group doesn't exist yet, a new one is created.
	//! @param beforeActionName insert the button before the button associated to the given action. If the action doesn't exist, insert it at the end of the group.
	void addButton(StelButton* button, const QString& groupName="defaultGroup", const QString& beforeActionName="");
	//! Hide the button associated with the action of the passed name
	StelButton* hideButton(const QString& actionName);
	
	//! Set the margin at the left and right of a button group in pixels
	void setGroupMargin(const QString& groupName, int left, int right);
	
	//! Set the color for all the sub elements
	void setColor(const QColor& c);
	
	//! Activate red mode for the buttons, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b);
	
	//! Set whether time must be displayed in the bottom bar
	void setFlagShowTime(bool b) {flagShowTime=b;}
	//! Set whether location info must be displayed in the bottom bar
	void setFlagShowLocation(bool b) {flagShowLocation=b;}

private slots:	
	//! Update the help label when a button is hovered
	void buttonHoverChanged(bool b);
	
private:
	void updateText();
	void updateButtonsGroups();
	QRectF getButtonsBoundingRect() const;
	QGraphicsSimpleTextItem* location;
	QGraphicsSimpleTextItem* datetime;
	QGraphicsSimpleTextItem* fov;
	QGraphicsSimpleTextItem* fps;
	
	struct ButtonGroup
	{
		ButtonGroup() : leftMargin(0), rightMargin(0) {;}
		//! Elements of the group
		QList<StelButton*> elems;
		//! Left margin size in pixel
		int leftMargin;
		//! Right margin size in pixel
		int rightMargin;
	};
	
	QMap<QString, ButtonGroup> buttonGroups;
	QPixmap pixBackgroundLeft;
	QPixmap pixBackgroundRight;
	QPixmap pixBackgroundMiddle;
	QPixmap pixBackgroundSingle;
	
	bool flagShowTime;
	bool flagShowLocation;
	
	QGraphicsSimpleTextItem* helpLabel;
};

// The path around the bottom left button bars
class StelBarsPath : public QGraphicsPathItem
{
	public:
		StelBarsPath(QGraphicsItem* parent);
		void updatePath(BottomStelBar* bot, LeftStelBar* lef);
		double getRoundSize() const {return roundSize;}
	private:
		double roundSize;
};

#endif // _STELGUIITEMS_HPP_
