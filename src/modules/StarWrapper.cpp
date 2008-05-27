/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 * The implementation of most functions in this file
 * (getInfoString,getShortInfoString,...) is taken from
 * Stellarium, Copyright (C) 2002 Fabien Chereau,
 * and therefore the copyright of these belongs to Fabien Chereau.
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

#include "StarWrapper.hpp"
#include "ZoneArray.hpp"

#include "StelUtils.hpp"
#include "Translator.hpp"

#include <QTextStream>

namespace BigStarCatalogExtension {

QString StarWrapperBase::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  StelUtils::rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  StelUtils::rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  QString str;
  QTextStream oss(&str);
  const Vec3f& c = getInfoColor();
  oss << QString("<font color=#%1%2%3>").arg(int(c[0]*255), 2, 16).arg(int(c[1]*255), 2, 16).arg(int(c[2]*255), 2, 16);
  oss << q_("Magnitude: <b>%1</b> (B-V: %2)").arg(QString::number(getMagnitude(nav), 'f', 2),
						  QString::number(getBV(), 'f', 2)) << "<br>";
  oss << q_("J2000 RA/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_j2000,true),
				      StelUtils::radToDmsStr(dec_j2000,true)) << "<br>";
  oss << q_("Equ of date RA/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_equ),
					    StelUtils::radToDmsStr(dec_equ)) << "<br>";

    // calculate alt az
  double az,alt;
  StelUtils::rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if(az > M_PI*2) az -= M_PI*2;    
  oss << q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt));
  
  return str;
}

QString StarWrapperBase::getShortInfoString(const Navigator *nav) const
{
	return q_("Magnitude: %1").arg(getMagnitude(nav), 0, 'f', 2);
}


QString StarWrapper1::getEnglishName(void) const {
	if (s->hip) {
		return QString("HP %1").arg(s->hip);
	}
	return StarWrapperBase::getEnglishName();
}

QString StarWrapper1::getInfoString(const Navigator *nav) const {
  const Vec3d j2000_pos = getObsJ2000Pos(nav);
  double dec_j2000, ra_j2000;
  StelUtils::rect_to_sphe(&ra_j2000,&dec_j2000,j2000_pos);
  const Vec3d equatorial_pos = nav->j2000_to_earth_equ(j2000_pos);
  double dec_equ, ra_equ;
  StelUtils::rect_to_sphe(&ra_equ,&dec_equ,equatorial_pos);
  QString str;
  QTextStream oss(&str);
  const Vec3f& c = getInfoColor();
  oss << QString("<font color=#%1%2%3>").arg(int(c[0]*255), 2, 16).arg(int(c[1]*255), 2, 16).arg(int(c[2]*255), 2, 16);
  if (s->hip)
  {
	  oss << "<h2>";
    const QString commonNameI18 = StarMgr::getCommonName(s->hip);
    const QString sciName = StarMgr::getSciName(s->hip);
    if (commonNameI18!="" || sciName!="")
    {
      oss << commonNameI18 << (commonNameI18 == "" ? "" : " ");
      if (commonNameI18!="" && sciName!="")
		  oss << "(";
      oss << (sciName=="" ? "" : sciName);
      if (commonNameI18!="" && sciName!="")
		  oss << ")";
	  oss << " - ";
    }
    oss << "HP " << s->hip;
    if (s->component_ids)
	{
		oss << " " << StarMgr::convertToComponentIds(s->component_ids);
    }
    oss << "</h2>";
  }

  oss << q_("Magnitude: <b>%1</b> (B-V: %2)").arg(QString::number(getMagnitude(nav), 'f', 2),
						  QString::number(s->getBV(), 'f', 2)) << "<br>";
  oss << q_("J2000 RA/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_j2000,true),
				      StelUtils::radToDmsStr(dec_j2000,true)) << "<br>";
  
  oss << q_("Equ of date RA/DE: %1/%2").arg(StelUtils::radToHmsStr(ra_equ),
					    StelUtils::radToDmsStr(dec_equ)) << "<br>";

    // calculate alt az
  double az,alt;
  StelUtils::rect_to_sphe(&az,&alt,nav->earth_equ_to_local(equatorial_pos));
  az = 3*M_PI - az;  // N is zero, E is 90 degrees
  if (az > M_PI*2)
	  az -= M_PI*2;    
  oss << q_("Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)) << "<br>";

  if (s->plx)
  {
		oss << q_("Parallax: %1").arg(0.00001*s->plx, 0, 'f', 5) << "<br>";
		oss << q_("Distance: %1 Light Years").arg((AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->plx*((0.00001/3600)*(M_PI/180))), 0, 'f', 2)
				<< "<br>";
  }

  if (s->sp_int)
  {
	  oss << q_("Spectral Type: %1").arg(StarMgr::convertToSpectralType(s->sp_int)) << "<br>";
  }
  return str;
}


QString StarWrapper1::getShortInfoString(const Navigator *nav) const
{
	QString str;
	QTextStream oss(&str);
	if (s->hip)
	{
		const QString commonNameI18 = StarMgr::getCommonName(s->hip);
		const QString sciName = StarMgr::getSciName(s->hip);
		if (commonNameI18!="" || sciName!="")
		{
			oss << commonNameI18 << (commonNameI18 == "" ? "" : " ");
			if (commonNameI18!="" && sciName!="") oss << "(";
			oss << (sciName=="" ? "" : sciName);
			if (commonNameI18!="" && sciName!="") oss << ")";
			oss << "  ";
		}
		oss << "HP " << s->hip;
		if (s->component_ids)
		{
			oss << " " << StarMgr::convertToComponentIds(s->component_ids);
		}
		oss << "  ";
	}
	
	oss << q_("Magnitude: %1").arg(getMagnitude(nav), 0, 'f', 2) << "  ";

	if (s->plx)
	{
		oss << q_("Distance: %1 Light Years").arg((AU/(SPEED_OF_LIGHT*86400*365.25)) / (s->plx*((0.00001/3600)*(M_PI/180))), 0, 'f', 2)
			<< "  ";
	}
	
	if (s->sp_int)
	{
		oss << q_("Spectral Type: %1").arg(StarMgr::convertToSpectralType(s->sp_int));
	}
	return str;
}


StelObjectP Star1::createStelObject(const SpecialZoneArray<Star1> *a,
                                    const SpecialZoneData<Star1> *z) const {
  return StelObjectP(new StarWrapper1(a,z,this));
}

StelObjectP Star2::createStelObject(const SpecialZoneArray<Star2> *a,
                                    const SpecialZoneData<Star2> *z) const {
  return StelObjectP(new StarWrapper2(a,z,this));
}

StelObjectP Star3::createStelObject(const SpecialZoneArray<Star3> *a,
                                    const SpecialZoneData<Star3> *z) const {
  return StelObjectP(new StarWrapper3(a,z,this));
}


} // namespace BigStarCatalogExtension

