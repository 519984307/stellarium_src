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


// Class used to manage group of constellation
#include "constellation.h"
#include "constellation_mgr.h"

// constructor which loads all data from appropriate files
Constellation_mgr::Constellation_mgr(string _data_dir, string _sky_culture, Hip_Star_mgr *_hip_stars, string _font_filename, 
				     Vec3f _lines_color, Vec3f _names_color) :
  asterFont(NULL), lines_color(_lines_color), names_color(_names_color), hipStarMgr(_hip_stars), 
  dataDir( _data_dir), skyCulture(_sky_culture)
{

  // load font
  asterFont = new s_font(12.,"spacefont", dataDir + _font_filename);
  if (!asterFont)
    {
      printf("Can't create asterFont\n");
      exit(-1);
    }

  // check validity of main sky culture config file
  if( !validate_sky_culture(_sky_culture) ) {
      skyCulture = "western";
  }

  load(dataDir + "sky_cultures/" + skyCulture + "/constellationship.fab",
       dataDir + "sky_cultures/" + skyCulture + "/constellationsart.fab", hipStarMgr);

}


Constellation_mgr::~Constellation_mgr()
{
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
      delete (*iter);
      asterisms.erase(iter);
      iter--;  // important!
    }
    if (asterFont) delete asterFont;
    asterFont = NULL;
}

void Constellation_mgr::show_art(void)
{
  // unless just one selected... turn on all artwork

  vector<Constellation *>::iterator iter;
  for(iter=asterisms.begin();iter!=asterisms.end();iter++) {
    (*iter)->show_art();
  }

}


void Constellation_mgr::hide_art(void)
{
  // unless just one selected... turn on all artwork

  vector<Constellation *>::iterator iter;
  for(iter=asterisms.begin();iter!=asterisms.end();iter++) {
    (*iter)->hide_art();
  }
}

void Constellation_mgr::set_sky_culture(string _sky_culture)
{

  // check validity of main sky culture config file
  if( !validate_sky_culture(_sky_culture) ) {
      _sky_culture = "western";
  }

  if( _sky_culture == skyCulture ) return;  // no change

  skyCulture = _sky_culture;

  // remove old data
  vector<Constellation *>::iterator iter;
  for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
      delete (*iter);
      asterisms.erase(iter);
      iter--;  // important!
    }

  // load new culture data
  printf( "Changing sky culture to %s\n", skyCulture.c_str() );
  load(dataDir + "sky_cultures/" + skyCulture + "/constellationship.fab",
       dataDir + "sky_cultures/" + skyCulture + "/constellationsart.fab", hipStarMgr);

}

// test that sky culture seems valid (has config file)
bool Constellation_mgr::validate_sky_culture(string _sky_culture) {

  string filePath = dataDir + "sky_cultures/" + skyCulture + "/constellationship.fab";
  FILE * fic = fopen(filePath.c_str(),"r");
  if (!fic) {
    printf("Could not locate constellation data for %s culture, defaulting to western.\n",skyCulture.c_str());
    skyCulture = "western";
    return(0);
  } else {
    fclose(fic);
    return(1);
  }

}

// Load from file
void Constellation_mgr::load(const string& fileName, const string& artfileName, Hip_Star_mgr * _VouteCeleste)
{
	printf("Loading constellation data...\n");


	FILE * fic = fopen(fileName.c_str(),"r");
    if (!fic)
    {
		printf("Can't open %s\n",fileName.c_str());
        exit(-1);
    }

    Constellation * cons = NULL;
    while(!feof(fic))
    {
        cons = new Constellation;
        if (cons && cons->read(fic, _VouteCeleste))
        {
            asterisms.push_back(cons);
        }
        else
        {
        	if (cons) delete cons;
        }
    }
    fclose(fic);

	fic = fopen(artfileName.c_str(),"r");
    if (!fic)
    {
		printf("Can't open %s\n",artfileName.c_str());
        exit(-1);
    }

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	char shortname[20];
	char texfile[255];
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;
	int texSize;
    while(!feof(fic))
    {
        if (fscanf(fic,"%s %s %u %u %u %u %u %u %u %u %u\n",shortname,texfile,&x1,&y1,&hp1,&x2,&y2,&hp2,
			&x3,&y3,&hp3)!=11)
		{
			if (feof(fic))
			{
				// Empty constellation file
				fclose(fic);
				return;
			}

			printf("ERROR while loading art for constellation %s\n", shortname);
			exit(-1);
		}

		cons = NULL;
		cons = find_from_short_name(shortname);
		if (!cons)
		{
			printf("ERROR : Can't find constellation called : %s\n",shortname);
			exit(-1);
		}

		cons->art_tex = new s_texture(texfile);
		texSize = cons->art_tex->getSize();

		Vec3f s1 = _VouteCeleste->search(hp1)->get_earth_equ_pos();
		Vec3f s2 = _VouteCeleste->search(hp2)->get_earth_equ_pos();
		Vec3f s3 = _VouteCeleste->search(hp3)->get_earth_equ_pos();

		// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
		// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
		// We need 3 stars and the 4th point is deduced from the other to get an normal base
		// X = B inv(A)
		Vec3f s4 = s1 + (s2-s1)^(s3-s1);
		Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
		Mat4f A(x1, texSize-y1, 0.f, 1.f, x2, texSize-y2, 0.f, 1.f,
			x3, texSize-y3, 0.f, 1.f, x1, texSize-y1, texSize, 1.f);
		Mat4f X = B * A.inverse();

		/*cons->art_vertex[0] = Vec3f(X*Vec3f(0,0,0));
		cons->art_vertex[1] = Vec3f(X*Vec3f(texSize,0,0));
		cons->art_vertex[2] = Vec3f(X*Vec3f(texSize,texSize,0));
		cons->art_vertex[3] = Vec3f(X*Vec3f(0,texSize,0));*/

		cons->art_vertex[0] = Vec3f(X*Vec3f(0,0,0));
		cons->art_vertex[1] = Vec3f(X*Vec3f(texSize/2,0,0));
		cons->art_vertex[2] = Vec3f(X*Vec3f(texSize/2,texSize/2,0));
		cons->art_vertex[3] = Vec3f(X*Vec3f(0,texSize/2,0));
		cons->art_vertex[4] = Vec3f(X*Vec3f(texSize/2 + 0,0,0));
		cons->art_vertex[5] = Vec3f(X*Vec3f(texSize/2 + texSize/2,0,0));
		cons->art_vertex[6] = Vec3f(X*Vec3f(texSize/2 + texSize/2,texSize/2,0));
		cons->art_vertex[7] = Vec3f(X*Vec3f(texSize/2 + 0,texSize/2,0));
		cons->art_vertex[8] = Vec3f(X*Vec3f(texSize/2 + 0, texSize/2 + 0,0));
		cons->art_vertex[9] = Vec3f(X*Vec3f(texSize/2 + texSize/2, texSize/2 + 0,0));
		cons->art_vertex[10]= Vec3f(X*Vec3f(texSize/2 + texSize/2, texSize/2 + texSize/2,0));
		cons->art_vertex[11]= Vec3f(X*Vec3f(texSize/2 + 0, texSize/2 + texSize/2,0));
		cons->art_vertex[12] = Vec3f(X*Vec3f(0,texSize/2 + 0,0));
		cons->art_vertex[13] = Vec3f(X*Vec3f(texSize/2, texSize/2 + 0,0));
		cons->art_vertex[14] = Vec3f(X*Vec3f(texSize/2, texSize/2 + texSize/2,0));
		cons->art_vertex[15] = Vec3f(X*Vec3f(0, texSize/2 + texSize/2,0));
    }
    fclose(fic);
}

// Draw all the constellations in the vector
void Constellation_mgr::draw(Projector* prj) const
{
	glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3fv(lines_color);
	prj->set_orthographic_projection();	// set 2D coordinate
    vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		(*iter)->draw_optim(prj);
    }
	prj->reset_perspective_projection();
}

// Draw one constellation of internationnal name abr
void Constellation_mgr::draw(Projector* prj, char abr[4]) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		if (!strcmp((*iter)->short_name,abr)) break;
	}
    (*iter)->draw(prj, lines_color);
}

void Constellation_mgr::draw_art(Projector* prj, int delta_time) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// for fading in art
	//    glColor3f(1,1,1);

	prj->set_orthographic_projection();
    vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		(*iter)->draw_art_optim(prj, delta_time);
    }
	prj->reset_perspective_projection();
	glDisable(GL_CULL_FACE);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector* prj, bool _gravity_label)
{
	Constellation::gravity_label = _gravity_label;
    glColor3fv(names_color);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    prj->set_orthographic_projection();	// set 2D coordinate
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
		// Check if in the field of view
    	if ( prj->project_earth_equ_check((*iter)->XYZname, (*iter)->XYname) )
			(*iter)->draw_name(asterFont, prj);
    }
    prj->reset_perspective_projection();
}

void Constellation_mgr::draw_one_name(Projector* prj, Constellation* c, bool _gravity_label) const
{
	Constellation::gravity_label = _gravity_label;
    glColor3fv(names_color);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    prj->set_orthographic_projection();	// set 2D coordinate
	// Check if in the field of view
    if ( prj->project_earth_equ_check(c->XYZname, c->XYname) )
	if (c) c->draw_name(asterFont, prj);
    prj->reset_perspective_projection();
}

Constellation* Constellation_mgr::is_star_in(const Hip_Star * s) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if ((*iter)->is_star_in(s)) return (*iter);
    }
	return NULL;
}

Constellation* Constellation_mgr::find_from_short_name(const string& shortname) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if (string((*iter)->short_name)==shortname) return (*iter);
    }
	return NULL;
}
