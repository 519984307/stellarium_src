/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
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


#include "planet.h"
#include "navigator.h"
#include "s_font.h"

s_font* planet::planet_name_font = NULL;

rotation_elements::rotation_elements() : period(1.), offset(0.), epoch(J2000),
		obliquity(0.), ascendingNode(0.), precessionRate(0.)
{
}

planet::planet(const char * _name, int _flagHalo, int _flag_lighting, double _radius, Vec3f _color,
	float _albedo, const char* tex_map_name, const char* tex_halo_name, pos_func_type _coord_func) :
		name(NULL), flagHalo(_flagHalo), flag_lighting(_flag_lighting), radius(_radius), color(_color),
		albedo(_albedo), axis_rotation(0.),	tex_map(NULL), tex_halo(NULL), rings(NULL), lastJD(J2000),
		deltaJD(JD_SECOND), coord_func(_coord_func), parent(NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	mat_local_to_parent = Mat4d::identity();
	if (_name) name=strdup(_name);
	tex_map = new s_texture(tex_map_name, TEX_LOAD_TYPE_PNG_SOLID);
	if (flagHalo) tex_halo = new s_texture(tex_halo_name);
}


planet::~planet()
{
	if (name) free(name);
	name=NULL;
	if (tex_map) delete tex_map;
	tex_map = NULL;
	if (tex_halo) delete tex_halo;
	tex_halo = NULL;
	if (rings) delete rings;
	rings = NULL;
}

// Return the information string "ready to print" :)
void planet::get_info_string(char * s, navigator * nav) const
{
	double tempDE, tempRA;
	Vec3d equPos = get_earth_equ_pos(nav);
	rect_to_sphe(&tempRA,&tempDE,equPos);
	sprintf(s,"Name :%s\nRA : %s\nDE : %s\n Distance : %.8f UA",
	name, print_angle_hms(tempRA*180./M_PI), print_angle_dms_stel(tempDE*180./M_PI), equPos.length());
}

// Set the orbital elements
void planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate)
{
    re.period = _period;
	re.offset = _offset;
    re.epoch = _epoch;
    re.obliquity = _obliquity;
    re.ascendingNode = _ascendingNode;
    re.precessionRate = _precessionRate;
}


// Return the planet position in rectangular earth equatorial coordinate
Vec3d planet::get_earth_equ_pos(navigator * nav) const
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return nav->helio_to_earth_pos_equ(v);		// this is earth equatorial but centered
												// on observer's position (latitude, longitude)
	//return navigation.helio_to_earth_equ(&v); this is the real equatorial centered on earth center
}

// Compute the position in the parent planet coordinate system
// Actually call the provided function to compute the ecliptical position
void planet::compute_position(double date)
{
	if (fabs(lastJD-date)>deltaJD)
	{
		coord_func(date, &(ecliptic_pos[0]), &(ecliptic_pos[1]), &(ecliptic_pos[2]));
		lastJD = date;
	}

}

// Compute the transformation matrix from the local planet coordinate to the parent planet coordinate
void planet::compute_trans_matrix(double date)
{
	mat_local_to_parent = Mat4d::translation(ecliptic_pos) // * Mat4d::zrotation(-re.ascendingNode)
		* Mat4d::xrotation(-re.obliquity);

	compute_geographic_rotation(date);
}


// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
Mat4d planet::get_helio_to_geo_matrix()
{
	Mat4d mat = mat_local_to_parent;
	mat = mat * Mat4d::zrotation(axis_rotation*M_PI/180.);

	// Iterate thru parents
	planet * p = parent;
	while (p!=NULL && p->parent!=NULL)
	{
		mat = p->mat_local_to_parent * mat;
		p=p->parent;
	}
	return mat;
}

// Compute the z rotation to use from equatorial to geographic coordinates
void planet::compute_geographic_rotation(double date)
{
    double t = date - re.epoch;
    double rotations = t / (double) re.period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

	axis_rotation = remainder * 360. + re.offset;

}

// Get the planet position in the parent planet ecliptic coordinate
Vec3d planet::get_ecliptic_pos() const
{
	return ecliptic_pos;
}

// Return the heliocentric ecliptical position
Vec3d planet::get_heliocentric_ecliptic_pos() const
{
	Vec3d pos = ecliptic_pos;
	planet * p = parent;
	while (p!=NULL)
	{
		pos.transfo4d(p->mat_local_to_parent);
		p=p->parent;
	}
	return pos;
}

// Compute the distance to the given position in heliocentric coordinate (in AU)
double planet::compute_distance(const Vec3d& obs_helio_pos)
{
	distance = (obs_helio_pos-get_heliocentric_ecliptic_pos()).length();
	return distance;
}

// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (dist in AU)
double planet::get_phase(Vec3d obs_pos)
{
	/* get distances */
	double R = obs_pos.length();
	Vec3d heliopos = get_heliocentric_ecliptic_pos();
	double r = heliopos.length();
	double delta = (obs_pos-heliopos).length();

	/* calc phase */
	return acos((r * r + delta * delta - R * R) / (2. * r * delta));
}

// TODO this doesn't work...
float planet::compute_magnitude(Vec3d obs_pos)
{
	Vec3d heliopos = get_heliocentric_ecliptic_pos();
	double p = heliopos.length();

	double R = M_PI * radius * radius / p;

	double s = obs_pos.length();
	double cos_chi = (p*p + R*R - s*s)/(2.f*p*R);
	//printf("cos_chi %f\n",cos_chi);
	float chi = acosf(cos_chi);
	float phase = (1.f - chi/M_PI) * cos_chi + sinf(chi) / M_PI;
	float F = 0.666666667f * albedo * phase * radius*s/(R*p) * radius*s/(R*p);
	return -26.73f - 2.5f * log10f(F);
}

// Add the given planet in the satellite list
void planet::add_satellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

// Return the radius of a circle containing the object on screen
float planet::get_on_screen_size(navigator * nav, Projector* prj)
{
	return atanf(radius*2.f/get_earth_equ_pos(nav).length())*180./M_PI/prj->get_fov()*prj->scrH();
}

// Draw the planet and all the related infos : name, circle etc..
void planet::draw(int hint_ON, Projector* prj, navigator * nav)
{
	Mat4d mat = mat_local_to_parent;
	planet * p = parent;
	while (p!=NULL && p->parent!=NULL)
	{
		mat = p->mat_local_to_parent * mat;
		p = p->parent;
	}

	// This removed totally the planet shaking bug!!!
	mat = nav->get_helio_to_eye_mat() * mat;

	glPushMatrix();

	glLoadMatrixd(mat);

	// Compute the 2D position and check if in the screen
	float screen_sz = get_on_screen_size(nav, prj);
	if (prj->project_custom(Vec3f(0,0,0), screenPos, mat) &&
		screenPos[1]>-screen_sz && screenPos[1]<prj->scrH()+screen_sz &&
		screenPos[0]>-screen_sz && screenPos[0]<prj->scrW()+screen_sz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
		float ang_dist = 300.f*atan(get_ecliptic_pos().length()/get_earth_equ_pos(nav).length())/prj->get_fov();
		if (ang_dist==0.f) ang_dist = 1.f; // if ang_dist == 0, the planet is sun..
     	if (hint_ON && ang_dist>0.25)
    	{
			if (ang_dist>1.f) ang_dist = 1.f;
			glColor4f(0.5f*ang_dist,0.5f*ang_dist,0.7f*ang_dist,1.f*ang_dist);
			draw_hints(nav, prj);
        }

		if (screen_sz>1)
		{
			if (rings)
			{
				double dist = get_earth_equ_pos(nav).length();
				prj->set_clipping_planes(dist-rings->get_size(), dist+rings->get_size());
				glEnable(GL_DEPTH_TEST);
				draw_sphere();
				rings->draw();
				glDisable(GL_DEPTH_TEST);
			}
			else draw_sphere();
		}

		if (tex_halo) draw_halo(nav, prj);
    }

	glPopMatrix();
}

void planet::draw_hints(navigator* nav, Projector* prj)
{
	prj->set_orthographic_projection();    // 2D coordinate

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	float tmp = 10.f + get_on_screen_size(nav, prj)/2.f; // Shift for name printing
	planet_name_font->print(screenPos[0]+tmp,screenPos[1]+tmp, name);

	// hint disapears smoothly on close view
	tmp -= 10.f;
	if (tmp<1) tmp=1;
 	glColor4f(0.5f/tmp,0.5f/tmp,0.7f/tmp,1.f/tmp);

	// Draw the 2D small circle
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_STRIP);
		for (float r=0.f; r<2.f*M_PI; r+=M_PI/5.f)
		{
			glVertex3f(screenPos[0] + 8. * sin(r), screenPos[1] + 8. * cos(r), 0.0f);
		}
	glEnd();

	prj->reset_perspective_projection();		// Restore the other coordinate
}

void planet::draw_sphere(void)
{
    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glPushMatrix();
	// Rotate and add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of the texture.
	glRotatef(axis_rotation + 180.,0.,0.,1.);

	if (flag_lighting) glEnable(GL_LIGHTING);
	else glDisable(GL_LIGHTING);
	glColor3fv(color);
	glBindTexture(GL_TEXTURE_2D, tex_map->getID());
	GLUquadricObj * p = gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	gluSphere(p,radius,40,40);
	gluDeleteQuadric(p);

	glPopMatrix();
    glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
}

void planet::draw_halo(navigator* nav, Projector* prj)
{
	float rmag = 5;//lim*du->screenH*100;
	if (rmag>0.5)
	{
		float cmag=1.;
		if (rmag<1.2)
		{
			cmag=pow(rmag,2)/1.44;
			rmag=1.2;
		}
		else
		{
			if (rmag>8.)
			{
				rmag=8.;
			}
		}

		glBlendFunc(GL_ONE, GL_ONE);

		float screen_r = get_on_screen_size(nav, prj);
		cmag *= rmag/screen_r;
		if (rmag<screen_r) rmag = screen_r;

		prj->set_orthographic_projection();    	// 2D coordinate

		glBindTexture(GL_TEXTURE_2D, tex_halo->getID());
		glEnable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glColor3f(color[0]*cmag, color[1]*cmag, color[2]*cmag);
		glTranslatef(screenPos[0], screenPos[1], 0.f);
		glBegin(GL_QUADS);
			glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
			glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
			glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
			glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
		glEnd();

		prj->reset_perspective_projection();		// Restore the other coordinate
	}
}


ring::ring(float _radius, const char* _texname) : radius(_radius), tex(NULL)
{
	tex = new s_texture(_texname,TEX_LOAD_TYPE_PNG_ALPHA);
}

ring::~ring()
{
	if (tex) delete tex;
	tex = NULL;
}

void ring::draw(void)
{
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glRotatef(axis_rotation + 180.,0.,0.,1.);
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
    glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	//glDisable(GL_LIGHTING);

    glBindTexture (GL_TEXTURE_2D, tex->getID());
	float r=radius;
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3d( r,-r, 0.);	// Bottom left
		glTexCoord2f(1,0); glVertex3d( r, r, 0.);	// Bottom right
		glTexCoord2f(1,1); glVertex3d(-r, r, 0.);	// Top right
		glTexCoord2f(0,1); glVertex3d(-r,-r, 0.);	// Top left
	glEnd ();
}
