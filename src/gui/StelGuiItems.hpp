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

#endif // _STELGUIITEMS_HPP_
