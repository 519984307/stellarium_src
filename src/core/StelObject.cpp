/*
 * Stellarium
 * Copyright (C) 2006 Johannes Gajdosik
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


#include "StelObject.hpp"
#include "Navigator.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "Observer.hpp"
#include "StelUtils.hpp"
#include "Translator.hpp"

#include <QRegExp>
#include <QDebug>

void intrusive_ptr_add_ref(StelObject* p)
{
	p->retain();
}

void intrusive_ptr_release(StelObject* p)
{
	p->release();
}

Vec3d StelObject::getObsEquatorialPos(const Navigator* nav) const
{
	return nav->j2000ToEarthEqu(getObsJ2000Pos(nav));
}

// Return the radius of a circle containing the object on screen
float StelObject::getOnScreenSize(const StelCore* core) const
{
	return getAngularSize(core)*M_PI/180.*core->getProjection()->getPixelPerRadAtCenter();
}

// Get observer local sideral coordinate
Vec3d StelObject::getObsSideralPos(const StelCore* core) const
{
	return Mat4d::zrotation(-core->getNavigation()->getLocalSideralTime())* getObsEquatorialPos(core->getNavigation());
}

// Get observer local alt/az coordinate
Vec3d StelObject::getAltAzPos(const Navigator* nav) const
{
	return nav->j2000ToLocal(getObsJ2000Pos(nav));
}

// Format the positional info string contain J2000/of date/altaz/hour angle positions for the object
QString StelObject::getPositionInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString res;
	const Navigator* nav = core->getNavigation();
	
	if (flags&RaDecJ2000)
	{
		double dec_j2000, ra_j2000;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,getObsJ2000Pos(nav));
		res += q_("RA/DE (J2000): %1/%2").arg(StelUtils::radToHmsStr(ra_j2000,true), StelUtils::radToDmsStr(dec_j2000,true)) + "<br>";
	}
	
	if (flags&RaDecOfDate)
	{
		double dec_equ, ra_equ;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getObsEquatorialPos(nav));
		res += q_("RA/DE (of date): %1/%2").arg(StelUtils::radToHmsStr(ra_equ), StelUtils::radToDmsStr(dec_equ)) + "<br>";
	}
	
	if (flags&HourAngle)
	{
		double dec_sideral, ra_sideral;
		StelUtils::rectToSphe(&ra_sideral,&dec_sideral,getObsSideralPos(core));
		ra_sideral = 2.*M_PI-ra_sideral;
		res += q_("Hour angle/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_sideral), StelUtils::radToDmsStr(dec_sideral)) + "<br>";
	}
	
	if (flags&AltAzi)
	{
		// calculate alt az
		double az,alt;
		StelUtils::rectToSphe(&az,&alt,getAltAzPos(nav));
		az = 3.*M_PI - az;  // N is zero, E is 90 degrees
		if (az > M_PI*2)
			az -= M_PI*2;    
		res += q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)) + "<br>";
	}
	return res;
}

// Apply post processing on the info string
void StelObject::postProcessInfoString(QString& str, const InfoStringGroup& flags)
{
	// chomp trailing line breaks
	str.replace(QRegExp("<br(\\s*/)?>\\s*$"), "");

	if (flags&PlainText)
	{
		str.replace("<b>", "");
		str.replace("</b>", "");
		str.replace("<h2>", "");
		str.replace("</h2>", "\n");
		str.replace("<br>", "\n");
	}
}
