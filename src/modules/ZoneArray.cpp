/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
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

#include <QDebug>
#include <QFile>
#ifdef WIN32
#include <io.h>
#include <windows.h>
#endif

#include "ZoneArray.hpp"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelObject.hpp"

namespace BigStarCatalogExtension
{

static const Vec3d north(0,0,1);

void ZoneArray::initTriangle(int index, const Vec3d &c0, const Vec3d &c1,
                             const Vec3d &c2)
{
	// initialize center,axis0,axis1:
	ZoneData &z(zones[index]);
	z.center = c0+c1+c2;
	z.center.normalize();
	z.axis0 = north ^ z.center;
	z.axis0.normalize();
	z.axis1 = z.center ^ z.axis0;
	// initialize star_position_scale:
	double mu0,mu1,f,h;
	mu0 = (c0-z.center)*z.axis0;
	mu1 = (c0-z.center)*z.axis1;
	f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
	h = fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
	mu0 = (c1-z.center)*z.axis0;
	mu1 = (c1-z.center)*z.axis1;
	f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
	h = fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
	mu0 = (c2-z.center)*z.axis0;
	mu1 = (c2-z.center)*z.axis1;
	f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
	h = fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
}

static inline
int ReadInt(QFile& file, unsigned int &x)
{
	const int rval = (4 == file.read((char*)&x, 4))
			 ? 0 : -1;
	return rval;
}

#if (!defined(__GNUC__))
#warning Star catalogue loading has only been tested with gcc
#endif

ZoneArray *ZoneArray::create(const QString& extended_file_name, bool use_mmap,
                             StelLoadingBar &lb)
{
	QString fname(extended_file_name);
	QString dbStr; // for debugging output.
	try
	{
		fname = StelApp::getInstance().getFileMgr().findFile("stars/default/"+fname);
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "Loading" << extended_file_name << e.what();
		return 0;
	}
	QFile* file = new QFile(fname);
	if(!file->open(QIODevice::ReadOnly))
	{
		qDebug() << "Loading" << extended_file_name << "failed to open file.";
		return 0;
	}
	dbStr = "Loading \"" + extended_file_name + "\": ";
	unsigned int magic,major,minor,type,level,mag_min,mag_range,mag_steps;
	if (ReadInt(*file,magic) < 0 ||
	        ReadInt(*file,type) < 0 ||
	        ReadInt(*file,major) < 0 ||
	        ReadInt(*file,minor) < 0 ||
	        ReadInt(*file,level) < 0 ||
	        ReadInt(*file,mag_min) < 0 ||
	        ReadInt(*file,mag_range) < 0 ||
	        ReadInt(*file,mag_steps) < 0)
	{
		dbStr += "error - file format is bad.";
		qDebug() << dbStr;
		return 0;
	}
	const bool byte_swap = (magic == FILE_MAGIC_OTHER_ENDIAN);
	if (byte_swap)
	{
		// ok, FILE_MAGIC_OTHER_ENDIAN, must swap
		if (use_mmap)
		{
			dbStr += "warning - must convert catalogue ";
#if (!defined(__GNUC__))
			dbStr += "to native format ";
#endif
			dbStr += "before mmap loading";
			qDebug() << dbStr;
			return 0;
		}
		dbStr += "byteswap ";
		type = bswap_32(type);
		major = bswap_32(major);
		minor = bswap_32(minor);
		level = bswap_32(level);
		mag_min = bswap_32(mag_min);
		mag_range = bswap_32(mag_range);
		mag_steps = bswap_32(mag_steps);
	}
	else if (magic == FILE_MAGIC)
	{
		// ok, FILE_MAGIC
#if (!defined(__GNUC__))
		if (use_mmap)
		{
			// mmap only with gcc:
			dbStr += "warning - you must convert catalogue "
			      += "to native format before mmap loading";
			qDebug(qPrintable(dbStr));

			return 0;
		}
#endif
	}
	else if (magic == FILE_MAGIC_NATIVE)
	{
		// ok, will work for any architecture and any compiler
	}
	else
	{
		dbStr += "error - not a catalogue file.";
		qDebug() << dbStr;
		return 0;
	}
	ZoneArray *rval = 0;
	dbStr += QString("%1_%2v%3_%4; ").arg(level)
	         .arg(type)
	         .arg(major)
	         .arg(minor);
	
	switch (type)
	{
	case 0:
		if (major > MAX_MAJOR_FILE_VERSION)
		{
			dbStr += "warning - unsupported version ";
		}
		else
		{
			// When this assertion fails you must redefine Star1
			// for your compiler.
			// Because your compiler does not pack the data,
			// which is crucial for this application.
			Q_ASSERT(sizeof(Star1) == 28);
			rval = new HipZoneArray(file, byte_swap, use_mmap, lb,
						level, mag_min, mag_range, mag_steps);
			if (rval == 0)
			{
				dbStr += "error - no memory ";
			}
		}
		break;
	case 1:
		if (major > MAX_MAJOR_FILE_VERSION)
		{
			dbStr += "warning - unsupported version ";
		}
		else
		{
			// When this assertion fails you must redefine Star2
			// for your compiler.
			// Because your compiler does not pack the data,
			// which is crucial for this application.
			Q_ASSERT(sizeof(Star2) == 10);
			rval = new SpecialZoneArray<Star2>(file, byte_swap, use_mmap,
							   lb, level, mag_min,
							   mag_range, mag_steps);
			if (rval == 0)
			{
				dbStr += "error - no memory ";
			}
		}
		break;
	case 2:
		if (major > MAX_MAJOR_FILE_VERSION)
		{
			dbStr += "warning - unsupported version ";
		}
		else
		{
			// When this assertion fails you must redefine Star3
			// for your compiler.
			// Because your compiler does not pack the data,
			// which is crucial for this application.
			Q_ASSERT(sizeof(Star3) == 6);
			rval = new SpecialZoneArray<Star3>(file, byte_swap, use_mmap,
							   lb, level, mag_min,
							   mag_range, mag_steps);
			if (rval == 0)
			{
				dbStr += "error - no memory ";
			}
		}
		break;
	default:
		dbStr += "error - bad file type ";
		break;
	}
	if (rval && rval->isInitialized())
	{
		dbStr += QString("%1").arg(rval->getNrOfStars());
		qDebug() << qPrintable(dbStr);
	}
	else
	{
		dbStr += " - initialization failed";
		qDebug() << dbStr;
		if (rval)
		{
			delete rval;
			rval = 0;
		}
	}
	return rval;
}

ZoneArray::ZoneArray(const QString& fname, QFile* file, int level, int mag_min,
		     int mag_range, int mag_steps)
			: fname(fname), level(level), mag_min(mag_min),
			  mag_range(mag_range), mag_steps(mag_steps),
			  star_position_scale(0.0), zones(0), file(file)
{
	nr_of_zones = StelGeodesicGrid::nrOfZones(level);
	nr_of_stars = 0;
}

bool ZoneArray::readFileWithStelLoadingBar(QFile& file, void *data, qint64 size,
        StelLoadingBar &lb)
{
	int parts = 256;
	int part_size = (size + (parts>>1)) / parts;
	if (part_size < 64*1024)
	{
		part_size = 64*1024;
		parts = (size + (part_size>>1)) / part_size;
	}
	float i = 0.f;
	lb.Draw(i / parts);
	i += 1.f;
	while (size > 0)
	{
		const int to_read = (part_size < size) ? part_size : size;
		const int read_rc = file.read((char*)data, to_read);
		if (read_rc != to_read) return false;
		size -= read_rc;
		data = ((char*)data) + read_rc;
		lb.Draw(i / parts);
		i += 1.f;
	}
	return true;
}

void HipZoneArray::updateHipIndex(HipIndexStruct hipIndex[]) const
{
	for (const SpecialZoneData<Star1> *z=getZones()+(nr_of_zones-1);
	        z>=getZones();z--)
	{
		for (const Star1 *s = z->getStars()+z->size-1;s>=z->getStars();s--)
		{
			const int hip = s->hip;
			if (hip < 0 || NR_OF_HIP < hip)
			{
				qDebug() << "ERROR: HipZoneArray::updateHipIndex: invalid HP number:"
					 << hip;
				exit(1);
			}
			if (hip != 0)
			{
				hipIndex[hip].a = this;
				hipIndex[hip].z = z;
				hipIndex[hip].s = s;
			}
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::scaleAxis(void)
{
	star_position_scale /= Star::MaxPosVal;
	for (ZoneData *z=zones+(nr_of_zones-1);z>=zones;z--)
	{
		z->axis0 *= star_position_scale;
		z->axis1 *= star_position_scale;
	}
}

template<class Star>
SpecialZoneArray<Star>::SpecialZoneArray(QFile* file, bool byte_swap,bool use_mmap,
					 StelLoadingBar &lb, int level, int mag_min,
					 int mag_range, int mag_steps)
		: ZoneArray(file->fileName(), file, level, mag_min, mag_range, mag_steps),
		  stars(0), mmap_start(0)
{
	if (nr_of_zones > 0)
	{
		lb.Draw(0.f);
		zones = new SpecialZoneData<Star>[nr_of_zones];
		if (zones == 0)
		{
			qDebug() << "ERROR: SpecialZoneArray(" << level
				 << ")::SpecialZoneArray: no memory (1)";
			exit(1);
		}
		
		unsigned int *zone_size = new unsigned int[nr_of_zones];
		if (zone_size == 0)
		{
			qDebug() << "ERROR: SpecialZoneArray(" << level
				 << ")::SpecialZoneArray: no memory (2)";
			exit(1);
		}
		if ((qint64)(sizeof(unsigned int)*nr_of_zones) != file->read((char*)zone_size, sizeof(unsigned int)*nr_of_zones))
		{
			qDebug() << "Error reading zones from catalog:"
				 << file->fileName();
			delete[] getZones();
			zones = 0;
			nr_of_zones = 0;
		}
		else
		{
			const unsigned int *tmp = zone_size;
			for (unsigned int z=0;z<nr_of_zones;z++,tmp++)
			{
				const unsigned int tmp_spu_int32 = byte_swap?bswap_32(*tmp):*tmp;
				nr_of_stars += tmp_spu_int32;
				getZones()[z].size = tmp_spu_int32;
			}
		}
		// delete zone_size before allocating stars
		// in order to avoid memory fragmentation:
		delete[] zone_size;

		if (nr_of_stars == 0)
		{
			// no stars ?
			if (zones) delete[] getZones();
			zones = 0;
			nr_of_zones = 0;
		}
		else
		{
			if (use_mmap)
			{
				mmap_start = file->map(file->pos(), sizeof(Star)*nr_of_stars);
				if (mmap_start == 0)
				{
					qDebug() << "ERROR: SpecialZoneArray(" << level
						 << ")::SpecialZoneArray: QFile(" << file->fileName()
						 << ".map(" << file->pos()
						 << ',' << sizeof(Star)*nr_of_stars
						 << ") failed: " << file->errorString();
					stars = 0;
					nr_of_stars = 0;
					delete[] getZones();
					zones = 0;
					nr_of_zones = 0;
				}
				else
				{
					stars = (Star*)mmap_start;
					Star *s = stars;
					for (unsigned int z=0;z<nr_of_zones;z++)
					{
						
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
				}
				file->close();
			}
			else
			{
				stars = new Star[nr_of_stars];
				if (stars == 0)
				{
					qDebug() << "ERROR: SpecialZoneArray(" << level
						 << ")::SpecialZoneArray: no memory (3)";
					exit(1);
				}
				if (!readFileWithStelLoadingBar(*file,stars,sizeof(Star)*nr_of_stars,lb))
				{
					delete[] stars;
					stars = 0;
					nr_of_stars = 0;
					delete[] getZones();
					zones = 0;
					nr_of_zones = 0;
				}
				else
				{
					Star *s = stars;
					for (unsigned int z=0;z<nr_of_zones;z++)
					{
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
					if (
#if (!defined(__GNUC__))
					    true
#else
					    byte_swap
#endif
					)
					{
						s = stars;
						for (unsigned int i=0;i<nr_of_stars;i++,s++)
						{
							s->repack(
#ifdef WORDS_BIGENDIAN
							    // need for byte_swap on a BE machine means that catalog is LE
							    !byte_swap
#else
							    // need for byte_swap on a LE machine means that catalog is BE
							    byte_swap
#endif
							);
						}
					}
				}
				file->close();
			}
		}
		lb.Draw(1.f);
	}
}

template<class Star>
SpecialZoneArray<Star>::~SpecialZoneArray(void)
{
	if (stars)
	{
		if (mmap_start != 0)
		{
			file->unmap(mmap_start);
		}
		else
		{
			delete[] stars;
		}
		delete file;
		stars = 0;
	}
	if (zones)
	{
		delete[] getZones();
		zones = NULL;
	}
	nr_of_zones = 0;
	nr_of_stars = 0;
}

template<class Star>
void SpecialZoneArray<Star>::draw(int index, bool is_inside, const float *rcmag_table,
				  const StelProjectorP& prj, unsigned int maxMagStarName,
				  float names_brightness, StelFont *starFont) const
{
	StelSkyDrawer* drawer = StelApp::getInstance().getCore()->getSkyDrawer();
	SpecialZoneData<Star> *const z = getZones() + index;
	Vec3d xy;
	const Star *const end = z->getStars() + z->size;
	const double d2000 = 2451545.0;
	const double movementFactor = (M_PI/180)*(0.0001/3600) * ((StarMgr::getCurrentJDay()-d2000)/365.25) / star_position_scale;
	for (const Star *s=z->getStars();s<end;++s)
	{
		if (is_inside ? prj->project(s->getJ2000Pos(z,movementFactor),xy) : prj->projectCheck(s->getJ2000Pos(z,movementFactor),xy))
		{
			if (drawer->drawPointSource(xy[0],xy[1],rcmag_table + 2*(s->mag),s->bV)==false)
			{
				break;
			}
			if (s->mag < maxMagStarName)
			{
				const QString& starname = s->getNameI18n();
				if (!starname.isEmpty())
				{
					const float offset = (rcmag_table + 2*(s->mag))[0]*0.7;
					const Vec3f& colorr = (StelApp::getInstance().getVisionModeNight() ? Vec3f(0.8, 0.2, 0.2) : StelSkyDrawer::indexToColor(s->bV))*0.75;
					glColor4f(colorr[0], colorr[1], colorr[2],names_brightness);
					drawer->getPainter()->drawText(starFont,xy[0],xy[1], starname, 0, offset, offset, false);
				}
			}
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::searchAround(int index, const Vec3d &v, double cosLimFov,
					  QList<StelObjectP > &result)
{
	const double d2000 = 2451545.0;
	const double movementFactor = (M_PI/180)*(0.0001/3600)
	                              * ((StarMgr::getCurrentJDay()-d2000)/365.25)
	                              / star_position_scale;
	const SpecialZoneData<Star> *const z = getZones()+index;
	for (int i=0;i<z->size;i++)
	{
		if (z->getStars()[i].getJ2000Pos(z,movementFactor)*v >= cosLimFov)
		{
			// TODO: do not select stars that are too faint to display
			result.push_back(z->getStars()[i].createStelObject(this,z));
		}
	}
}

} // namespace BigStarCatalogExtension
