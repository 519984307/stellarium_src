/*
 * Stellarium
 * Copyright (C) 2008 Nigel Kerr
 * Copyright (C) 2012 Timothy Reaves
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

#include "Dialog.hpp"
#include "DateTimeDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelUtils.hpp"

#include "ui_dateTimeDialogGui.h"

#include <QDebug>
#include <QFrame>
#include <QLineEdit>

DateTimeDialog::DateTimeDialog(QObject* parent) :
	StelDialog(parent),
	year(0),
	month(0),
	day(0),
	hour(0),
	minute(0),
	second(0),
	jd(0)
{
	ui = new Ui_dateTimeDialogForm;
	dialogName = "DateTime";
}

DateTimeDialog::~DateTimeDialog()
{
	delete ui;
	ui=NULL;
}

void DateTimeDialog::createDialogContent()
{
	ui->setupUi(dialog);
	StelCore *core = StelApp::getInstance().getCore();
	double jd = core->getJD();
	// UTC -> local tz
	setDateTime(jd + (core->getUTCOffset(jd)/24.0));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectSpinnerEvents();
}

void DateTimeDialog::connectSpinnerEvents() const
{
	connect(ui->spinner_year, SIGNAL(valueChanged(int)), this, SLOT(yearChanged(int)));
	connect(ui->spinner_month, SIGNAL(valueChanged(int)), this, SLOT(monthChanged(int)));
	connect(ui->spinner_day, SIGNAL(valueChanged(int)), this, SLOT(dayChanged(int)));
	connect(ui->spinner_hour, SIGNAL(valueChanged(int)), this, SLOT(hourChanged(int)));
	connect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	connect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));
	connect(ui->spinner_jd, SIGNAL(valueChanged(double)), this, SLOT(jdChanged(double)));
	connect(ui->spinner_mjd, SIGNAL(valueChanged(double)), this, SLOT(mjdChanged(double)));
}

void DateTimeDialog::disconnectSpinnerEvents()const
{
	disconnect(ui->spinner_year, SIGNAL(valueChanged(int)), this, SLOT(yearChanged(int)));
	disconnect(ui->spinner_month, SIGNAL(valueChanged(int)), this, SLOT(monthChanged(int)));
	disconnect(ui->spinner_day, SIGNAL(valueChanged(int)), this, SLOT(dayChanged(int)));
	disconnect(ui->spinner_hour, SIGNAL(valueChanged(int)), this, SLOT(hourChanged(int)));
	disconnect(ui->spinner_minute, SIGNAL(valueChanged(int)), this, SLOT(minuteChanged(int)));
	disconnect(ui->spinner_second, SIGNAL(valueChanged(int)), this, SLOT(secondChanged(int)));
	disconnect(ui->spinner_jd, SIGNAL(valueChanged(double)), this, SLOT(jdChanged(double)));
	disconnect(ui->spinner_mjd, SIGNAL(valueChanged(double)), this, SLOT(mjdChanged(double)));
}


//! take in values, adjust for calendrical correctness if needed, and push to
//! the widgets and signals
bool DateTimeDialog::valid(int y, int m, int d, int h, int min, int s)
{
	int dy, dm, dd, dh, dmin, ds;

	if (!StelUtils::changeDateTimeForRollover(y, m, d, h, min, s, &dy, &dm, &dd, &dh, &dmin, &ds)) {
		dy = y;
		dm = m;
		dd = d;
		dh = h;
		dmin = min;
		ds = s;
	}

	year = dy;
	month = dm;
	day = dd;
	hour = dh;
	minute = dmin;
	second = ds;
	pushToWidgets();
	StelApp::getInstance().getCore()->setJD(newJd());
	return true;
}

bool DateTimeDialog::validJd(double jday)
{
	pushToWidgets();
	StelApp::getInstance().getCore()->setJD(jday);
	return true;
}

void DateTimeDialog::retranslate()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void DateTimeDialog::styleChanged()
{
	// Nothing for now
}

void DateTimeDialog::close()
{
	ui->dateTimeTab->setFocus();
	StelDialog::close();
}

/************************************************************************
 year slider or dial changed
************************************************************************/

void DateTimeDialog::yearChanged(int newyear)
{
	if ( year != newyear )
	{
		valid( newyear, month, day, hour, minute, second );		
	}
}

void DateTimeDialog::monthChanged(int newmonth)
{
	if ( month != newmonth )
	{
		valid( year, newmonth, day, hour, minute, second );		
	}
}

void DateTimeDialog::dayChanged(int newday)
{
	int delta = newday - day;
	validJd(jd + delta);	
}

void DateTimeDialog::hourChanged(int newhour)
{
	int delta = newhour - hour;
	validJd(jd + delta/24.);
}
void DateTimeDialog::minuteChanged(int newminute)
{
	int delta = newminute - minute;
	validJd(jd + delta/1440.);

}
void DateTimeDialog::secondChanged(int newsecond)
{
	int delta = newsecond - second;
	validJd(jd + delta/86400.);
}
void DateTimeDialog::jdChanged(double njd)
{
	if ( jd != njd)
	{
		validJd(njd);
	}
}
void DateTimeDialog::mjdChanged(double nmjd)
{
	double delta = nmjd - getMjd();
	validJd(jd + delta);
}


double DateTimeDialog::newJd()
{
	double cjd;
	StelUtils::getJDFromDate(&cjd, year, month, day, hour, minute, second);
	cjd -= (StelApp::getInstance().getCore()->getUTCOffset(cjd)/24.0); // local tz -> UTC

	return cjd;
}

void DateTimeDialog::pushToWidgets()
{
	disconnectSpinnerEvents();
	ui->spinner_year->setValue(year);
	ui->spinner_month->setValue(month);
	ui->spinner_day->setValue(day);
	ui->spinner_hour->setValue(hour);
	ui->spinner_minute->setValue(minute);
	ui->spinner_second->setValue(second);
	ui->spinner_jd->setValue(jd);
	ui->spinner_mjd->setValue(getMjd());
	if (jd<2299161) // 1582-10-15
		ui->dateTimeTab->setToolTip(q_("Date and Time in Julian calendar"));
	else
		ui->dateTimeTab->setToolTip(q_("Date and Time in Gregorian calendar"));
	connectSpinnerEvents();
}

/************************************************************************
Prepare date elements from newJd and send to spinner_*
 ************************************************************************/
void DateTimeDialog::setDateTime(double newJd)
{
	if (this->visible()) {
		// JD and MJD should be at the UTC scale on the window!
		double newJdC = newJd + StelApp::getInstance().getCore()->getUTCOffset(newJd)/24.0; // UTC -> local tz
		StelUtils::getDateFromJulianDay(newJdC, &year, &month, &day);
		StelUtils::getTimeFromJulianDay(newJdC, &hour, &minute, &second);
		jd = newJd;

		pushToWidgets();
	}
}

