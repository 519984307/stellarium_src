/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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


#ifndef _DIALOG_H_
#define _DIALOG_H_

#include <QWidget>
#include <QFrame>
#include <QMouseEvent>


class DialogFrame : public QFrame
{
Q_OBJECT
public:
    DialogFrame(QWidget *parent) : QFrame(parent) {}
};


class BarFrame : public QFrame
{
Q_OBJECT
public:
  QPoint mouse_pos;
  
  BarFrame(QWidget* parent) : QFrame(parent) {}
  
  void mousePressEvent(QMouseEvent *event) {
    mouse_pos = event->pos();
  }
  void mouseMoveEvent(QMouseEvent *event);
};

class ResizeFrame : public QFrame
{
Q_OBJECT
public:
  QPoint mouse_pos;
  
  ResizeFrame(QWidget* parent) : QFrame(parent) {}
  
  void mousePressEvent(QMouseEvent *event) {
    mouse_pos = event->pos();
  }
  void mouseMoveEvent(QMouseEvent *event);
};


#endif // _DIALOG_H_
