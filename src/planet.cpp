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

s_font * planet_name_font;

rotation_elements::rotation_elements() : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), precessionRate(0.)
{
}

planet::planet(char * _name, int _flagHalo, double _radius, vec3_t _color,
				s_texture * _planetTexture, s_texture * _haloTexture,
				void (*_coord_func)(double JD, double *, double *, double *)) :
					flagHalo(_flagHalo), radius(_radius), color(_color), axis_rotation(0.),
					planetTexture(_planetTexture), haloTexture(_haloTexture),
					coord_func(_coord_func), parent(NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	name=strdup(_name);
}


planet::~planet()
{
	if (name) free(name);
	name=NULL;
}

// Return the information string "ready to print" :)
void planet::get_info_string(char * s, navigator * nav)
{
	double tempDE, tempRA;
	Vec3d equPos = get_earth_equ_pos(nav);
	rect_to_sphe(&tempRA,&tempDE,&equPos);
	sprintf(s,"Name :%s\nRA : %s\nDE : %s\n Distance : %.8f UA",
	name, print_angle_hms(tempRA*180./M_PI), print_angle_dms_stel(tempDE*180./M_PI), equPos.length());
}

void planet::set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate)
{
    re.period = _period;
	re.offset = _offset;
    re.epoch = _epoch;
    re.obliquity = _obliquity;
    re.ascendingNode = _ascendingNode;
    re.precessionRate = _precessionRate;
}


// Return the rect earth equatorial position
Vec3d planet::get_earth_equ_pos(navigator * nav)
{
	Vec3d v = get_heliocentric_ecliptic_pos();
	return nav->helio_to_earth_pos_equ(&v); 	// this is earth equatorial but centered
												// on observer position (latitude, longitude)
	//return navigation.helio_to_earth_equ(&v); this is the real equatorial
}

// Call the provided function to compute the ecliptical position
void planet::compute_position(double date)
{
	coord_func(date, &(ecliptic_pos[0]), &(ecliptic_pos[1]), &(ecliptic_pos[2]));
	ecliptic_pos=ecliptic_pos;
	//printf("%s : %.30lf %.30lf %.30lf\n",name,ecliptic_pos[0],ecliptic_pos[1],ecliptic_pos[2]);
    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_position(date);
        iter++;
    }
}

// Compute the matrices which converts from the parent's ecliptic coordinates to local ecliptic and oposite
void planet::compute_trans_matrix(double date)
{
	mat_parent_to_local =	Mat4d::xrotation(re.obliquity) /* Mat4d::zrotation(re.ascendingNode)*/ * Mat4d::translation(-ecliptic_pos);
	mat_local_to_parent =   Mat4d::translation(ecliptic_pos) /* Mat4d::zrotation(-re.ascendingNode)*/ * Mat4d::xrotation(-re.obliquity);


	compute_geographic_rotation(date);

    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}



// Get a matrix which convert from local geographic coordinate to heliocentric ecliptic coordinate
Mat4d planet::get_helio_to_geo_matrix()
{
	Mat4d mat = mat_local_to_parent;

	mat = mat * Mat4d::zrotation(axis_rotation*M_PI/180.);
	planet * p = this;
	while (p->parent!=NULL)
	{
		mat = p->parent->mat_local_to_parent * mat;
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

Vec3d planet::get_ecliptic_pos()
{
	return ecliptic_pos;
}

Vec3d planet::get_heliocentric_ecliptic_pos()
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

void planet::addSatellite(planet*p)
{
	satellites.push_back(p);
	p->parent=this;
}

// Draw the planet and all the related infos : name, circle etc..
void planet::draw(int hint_ON, draw_utility * du, navigator * nav)
{
	glPushMatrix();
    glMultMatrixd(mat_local_to_parent); // Go in planet local coordinate

	double screenX, screenY, screenZ;
	du->project(0., 0., 0., screenX, screenY, screenZ);

	// Check if in the screen
	Vec3d equPos = get_earth_equ_pos(nav);
	float angl = radius/equPos.length();
	float lim = atan(angl)*180./M_PI/du->fov;
	if (screenZ < 1 && screenX>(-lim*du->screenW) &&
		screenX<((1.f+lim)*du->screenW) &&
		screenY>(-lim*du->screenH) && screenY<((1.f+lim)*du->screenH))
	{
		if (haloTexture)
		{
			float rmag = lim*du->screenH*100;
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

				du->set_orthographic_projection();    // 2D coordinate
				glBindTexture(GL_TEXTURE_2D, haloTexture->getID());
            	glEnable(GL_BLEND);
            	glDisable(GL_LIGHTING);
				glEnable(GL_TEXTURE_2D);
            	glColor3f(cmag,cmag,cmag);
				glTranslatef(screenX,screenY,0.);
				glBegin(GL_QUADS);
					glTexCoord2i(0,0);	glVertex3f(-rmag, rmag,0.f);	// Bottom Left
					glTexCoord2i(1,0);	glVertex3f( rmag, rmag,0.f);	// Bottom Right
					glTexCoord2i(1,1);	glVertex3f( rmag,-rmag,0.f);	// Top Right
					glTexCoord2i(0,1);	glVertex3f(-rmag,-rmag,0.f);	// Top Left
				glEnd();
				du->reset_perspective_projection();		// Restore the other coordinate
			}
		}

		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
    	if (hint_ON && atan(get_ecliptic_pos().length()/equPos.length())/du->fov>0.0005)
    	{
            du->set_orthographic_projection();    // 2D coordinate
            glEnable(GL_BLEND);
            glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
            glColor3f(0.5,0.5,0.7);
            float tmp = 8.f + angl*du->screenH*60./du->fov; // Shift for name printing
            planet_name_font->print(screenX+tmp,screenY+tmp, name);

            // Draw the 2D small circle : disapears smoothly on close view
			tmp-=8.;
			if (tmp<1) tmp=1.;
			glColor4f(0.5/tmp,0.5/tmp,0.7/tmp,1/tmp);
			glDisable(GL_TEXTURE_2D);
            glBegin(GL_LINE_STRIP);
            for (float r = 0; r < 6.28; r += 0.2)
            {
                glVertex3f(screenX + 8. * sin(r), screenY + 8. * cos(r), 0.0f);
            }
            glEnd();
            du->reset_perspective_projection();		// Restore the other coordinate
        }

    	glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);


		glPushMatrix();

		// Rotate and add an extra half rotation because of the convention in all
    	// planet texture maps where zero deg long. is in the middle of the texture.
		glRotatef(axis_rotation + 180.,0.,0.,1.);

		glEnable(GL_DEPTH_TEST); // Enable this for eclipse correct vision
		glBindTexture(GL_TEXTURE_2D, planetTexture->getID());
		GLUquadricObj * p=gluNewQuadric();
		gluQuadricTexture(p,GL_TRUE);
		gluSphere(p,radius,60,60);
		gluDeleteQuadric(p);
		glDisable(GL_DEPTH_TEST);

		glPopMatrix();
    }

    // Draw the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->draw(hint_ON, du, nav);
        iter++;
    }

    glPopMatrix();

    glDisable(GL_CULL_FACE);
}

// Search if any planet is close to position given in earth equatorial position and return the distance
planet* planet::search(Vec3d pos, double * angleClosest, navigator * nav)
{
    pos.normalize();
    planet * closest = NULL;
	planet * p = NULL;

	Vec3d equPos = get_earth_equ_pos(nav);
	equPos.normalize();
    double angleClos = equPos[0]*pos[0] + equPos[1]*pos[1] + equPos[2]*pos[2];

	closest = this;
	if (angleClos>*angleClosest)
	{
		*angleClosest = angleClos;
	}

    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        p = (*iter)->search(pos,&angleClos,nav);
		if (angleClos>*angleClosest)
		{
			closest = p;
			*angleClosest = angleClos;
		}
        iter++;
    }
    if (*angleClosest>0.999)
    {
	    return closest;
    }
    else return NULL;
}

// Search if any planet is close to position given in earth equatorial position.
planet* planet::search(Vec3d pos, navigator * nav)
{
	double temp = 0.;
	return search(pos,&temp, nav);
}


sun_planet::sun_planet(char * _name, int _flagHalo, double _radius, vec3_t _color,
				s_texture * _planetTexture, s_texture * _haloTexture, s_texture * _bigHaloTexture) : planet(_name,_flagHalo,_radius,_color,_planetTexture,_haloTexture,NULL)
{
	ecliptic_pos=Vec3d(0.,0.,0.);
	mat_local_to_parent = Mat4d::identity();
	name=strdup(_name);
}


void sun_planet::compute_position(double date)
{
    // The sun is fixed in the heliocentric coordinate

    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_position(date);
        iter++;
    }
	glDisable(GL_LIGHTING);
}

// Get a matrix which converts from the parent's ecliptic coordinates to local ecliptic
void sun_planet::compute_trans_matrix(double date)
{
    // Compute for the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}

void sun_planet::draw(int hint_ON, draw_utility * du, navigator* nav)
{
	// We are supposed to be in heliocentric coordinate already so no matrix change
	//glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, planetTexture->getID());
	GLUquadricObj * p=gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	gluSphere(p,radius,40,40);
	gluDeleteQuadric(p);
	glDisable(GL_DEPTH_TEST);

	// Draw the name, and the circle
    // Thanks to Nick Porcino for this addition
    if (hint_ON)
    {
        double screenX, screenY, screenZ;
        du->project(0., 0., 0., screenX, screenY, screenZ);

        if (screenZ < 1)
        {
		    glEnable(GL_BLEND);

            du->set_orthographic_projection();		// 2D coordinate

            glColor3f(0.5,0.5,0.7);
            planet_name_font->print(screenX+5.,screenY+5., name);

            // Draw the circle
			glDisable(GL_TEXTURE_2D);
            glBegin(GL_LINE_STRIP);
            	for (float r = 0; r < 6.28; r += 0.2)
            	{
                	glVertex3f(screenX + 8. * sin(r), screenY + 8. * cos(r), 0);
            	}
            glEnd();
            du->reset_perspective_projection();		// Restore the other coordinate
        }
    }

	// Set the lighting with the sun as light source
    float tmp[4] = {0,0,0,0};
	float tmp2[4] = {0.1,0.1,0.1,0.1};
    float tmp3[4] = {2,2,2,2};
    float tmp4[4] = {1,1,1,1};
    glLightfv(GL_LIGHT0,GL_AMBIENT,tmp2);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,tmp3);
    glLightfv(GL_LIGHT0,GL_SPECULAR,tmp);

    glMaterialfv(GL_FRONT,GL_AMBIENT ,tmp2);
    glMaterialfv(GL_FRONT,GL_DIFFUSE ,tmp4);
    glMaterialfv(GL_FRONT,GL_EMISSION ,tmp);
    glMaterialfv(GL_FRONT,GL_SHININESS ,tmp);
    glMaterialfv(GL_FRONT,GL_SPECULAR ,tmp);

	float zero4[4] = {0.,0.,0.,1.};
    glLightfv(GL_LIGHT0,GL_POSITION,zero4);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);


    // Draw the satellites
    list<planet*>::iterator iter = satellites.begin();
    while (iter != satellites.end())
    {
        (*iter)->draw(hint_ON, du, nav);
        iter++;
    }

	glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);
}



ring_planet::ring_planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, void (*_coord_func)(double JD, double *, double *, double *), ring * _planet_ring) : planet(_name, _flagHalo, _radius, _color, _planetTexture, _haloTexture, _coord_func), planet_ring(_planet_ring)
{
}

void ring_planet::draw(int hint_ON, draw_utility * du, navigator* nav)
{
	planet::draw(hint_ON, du, nav);

	glPushMatrix();
    glMultMatrixd(mat_local_to_parent); // Go in planet local coordinate
	glPushMatrix();
	glRotatef(axis_rotation + 180.,0.,0.,1.);
	planet_ring->draw(nav);
	glPopMatrix();
	glPopMatrix();
}

ring::ring(float _radius, s_texture * _tex) : radius(_radius), tex(_tex)
{
}

void ring::draw(navigator* nav)
{
	glColor3f(1.0f, 0.88f, 0.82f); // For saturn only..
    glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
    glBindTexture (GL_TEXTURE_2D, tex->getID());
	float r=radius;
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3d( r,-r, 0.);	// Bottom left
		glTexCoord2f(1,0); glVertex3d( r, r, 0.);	// Bottom right
		glTexCoord2f(1,1); glVertex3d(-r, r, 0.);	// Top right
		glTexCoord2f(0,1); glVertex3d(-r,-r, 0.);	// Top left
	glEnd ();
	glDisable(GL_DEPTH_TEST);

}

	/*
    float rmag;
    if (num==0)                                                                     // Sun
    {   glBindTexture (GL_TEXTURE_2D, texIds[48]->getID());
        rmag=25*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Rayon du halo
        glColor3fv(Colour);
    }
    else
    {   glBindTexture (GL_TEXTURE_2D, texIds[25]->getID());
        glColor3fv(Colour*(1.8-global.SkyBrightness));                              // Calcul de la couleur
        rmag=300*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;                   // Rayon de "l'eclat"
        if (num==5) rmag=150*Rayon*DIST_PLANET/Distance_to_obs*global.Fov/60;
    }

    if (num!=10)                                // != luna
    {   glEnable(GL_BLEND);
                                                // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();      
    }


    if (num==10)
    {   glBindTexture (GL_TEXTURE_2D, texIds[50]->getID());
        rmag=20*Rayon*DIST_PLANET/Distance_to_obs*(global.Fov/60);                  // Lune : Rayon du halo
        glColor3f(0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)),0.07*(1-posSun.Dot(normGeoCoord)));
        glEnable(GL_BLEND);
                                        // Draw a light point like a star for naked eye simulation
        glPushMatrix();
        glRotatef(RaRad*180/PI,0,1,0);
        glRotatef(DecRad*180/PI,-1,0,0);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2i(1,0);                  //Bas Droite
            glVertex3f(rmag,-rmag,0.0f);
            glTexCoord2i(0,0);                  //Bas Gauche
            glVertex3f(-rmag,-rmag,0.0f);
            glTexCoord2i(1,1);                  //Haut Droit
            glVertex3f(rmag,rmag,0.0f);
            glTexCoord2i(0,1);                  //Haut Gauche
            glVertex3f(-rmag,rmag,0.0f);
        glEnd ();
        glPopMatrix();  
    }



    glRotatef(90,1,0,0);


    if (num==6)                             // Draw saturn rings 1/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f);
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();

        glRotatef(RaRad*180./PI,0,0,1);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);              //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);              //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);            //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);            //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    if (asin(Rayon*2/Distance_to_obs)*180./PI>3*global.Fov/global.Y_Resolution)     //Draw the sphere if big enough


    if (num==6)                                 // Draw saturn rings 2/2
    {   double rAn=2.5*Rayon*DIST_PLANET/Distance_to_obs;
        glColor3f(1.0f, 1.0f, 1.0f); //SkyBrightness
        glBindTexture (GL_TEXTURE_2D, texIds[47]->getID());
        glEnable(GL_BLEND);
        glPushMatrix();
        glRotatef(RaRad*180./PI+180,0,0,1);
        glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(1,0);                  //Bas Droite
            glVertex3f(rAn,-rAn,0.0f);
            glTexCoord2f(0,0);                  //Bas Gauche
            glVertex3f(-rAn,-rAn,0.0f);
            glTexCoord2f(1,0.5);                //Haut Droit
            glVertex3f(rAn,0.0f,0.0f);
            glTexCoord2f(0,0.5);                //Haut Gauche
            glVertex3f(-rAn,0.0f,0.0f);
        glEnd ();
        glPopMatrix();
    }

    glPopMatrix();
}
*/

