/*
 * Copyright (C) 2009 Timothy Reaves
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

#include "Ocular.hpp"
#include "Telescope.hpp"

#include <QSettings>

Ocular::Ocular()
{
}

Ocular::Ocular(const QObject* other)
{
	Q_ASSERT(other);
	Q_ASSERT(other->metaObject()->className() == "Ocular");
	this->m_appearentFOV = other->property("appearentFOV").toDouble();
	this->m_effectiveFocalLength = other->property("effectiveFocalLength").toDouble();
	this->m_fieldStop = other->property("fieldStop").toDouble();
	this->m_name = other->property("name").toString();
}

Ocular::~Ocular()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Ocular::propertyMap()
{
	if(mapping.isEmpty()) {
		mapping = QMap<int, QString>();
		mapping[0] = "name";
		mapping[1] = "appearentFOV";
		mapping[2] = "effectiveFocalLength";
		mapping[3] = "fieldStop";
	}
	return mapping;
}


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
double Ocular::actualFOV(Telescope *telescope) const
{
	double actualFOV = 0.0;
	if (fieldStop() > 0.0) {
		actualFOV =  fieldStop() / telescope->focalLength() * 57.3;
	} else {
		//actualFOV = apparent / mag
		actualFOV = appearentFOV() / (telescope->focalLength() / effectiveFocalLength());
	}
	return actualFOV;
}

double Ocular::magnification(Telescope *telescope) const
{
	return telescope->focalLength() / effectiveFocalLength();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
const QString Ocular::name() const
{
	return m_name;
}

void Ocular::setName(QString aName)
{
	m_name = aName;
}

double Ocular::appearentFOV() const
{
	return m_appearentFOV;
}

void Ocular::setAppearentFOV(double fov)
{
	m_appearentFOV = fov;
}

double Ocular::effectiveFocalLength() const
{
	return m_effectiveFocalLength;
}

void Ocular::setEffectiveFocalLength(double fl)
{
	m_effectiveFocalLength = fl;
}

double Ocular::fieldStop() const
{
	return m_fieldStop;
}

void Ocular::setFieldStop(double fs)
{
	m_fieldStop = fs;
}
