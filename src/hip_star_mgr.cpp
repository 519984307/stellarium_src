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

// class used to manage groups of Stars
// TODO : Can optimize by sorting the Hip_star* array in magnitude

#include "hip_star_mgr.h"
#include "s_texture.h"
#include "grid.h"
#include "bytes.h"
#include "stellarium.h"
#include "navigator.h"

#define RADIUS_STAR 1.

// construct and load all data
Hip_Star_mgr::Hip_Star_mgr(string _data_dir, string _sky_locale, string _font_filename) :
  starZones(NULL), HipGrid(), StarArray(NULL), StarArraySize(0), starTexture(NULL), 
  starFont(NULL), dataDir(_data_dir), skyLocale(_sky_locale) 
{

  starZones = new vector<Hip_Star*>[HipGrid.getNbPoints()];

  load( dataDir + _font_filename, 
	dataDir + "hipparcos.fab", 
	dataDir + "star_names." + skyLocale + ".fab",
	dataDir + "name.fab" );

}


Hip_Star_mgr::~Hip_Star_mgr()
{
  if (starZones) delete [] starZones;

  for (int i=0;i<StarArraySize;i++)
    {
      if (StarArray[i]) delete StarArray[i];
      StarArray[i]=NULL;
    }
  
  if (starTexture) delete starTexture;
  starTexture=NULL;
  exit(-1);
  if (starFont) delete starFont;
  starFont=NULL;
  if (StarArray) delete StarArray;
  StarArray = NULL;
}

// change star common names to a different sky locale
void Hip_Star_mgr::set_sky_locale(string _sky_locale) {

  if(skyLocale == _sky_locale) return;

  // assuming validated locale already
  load_common_names( dataDir + "star_names." + _sky_locale + ".fab" );
  skyLocale = _sky_locale;

}

// Load from file ( create the stream and call the Read function )
void Hip_Star_mgr::load(const string& font_fileName, const string& hipCatFile,
	const string& commonNameFile, const string& nameFile)
{
    printf("Loading Hipparcos star data...\n");
    FILE * hipFile, *cnFile, * nFile;
    hipFile = NULL;
    cnFile = NULL;
    nFile = NULL;

    hipFile=fopen(hipCatFile.c_str(),"rb");
    if (!hipFile)
    {
        printf("ERROR %s NOT FOUND\n",hipCatFile.c_str());
        exit(-1);
    }
    cnFile=fopen(commonNameFile.c_str(),"r");
    if (!cnFile)
    {   
        printf("ERROR %s NOT FOUND\n",commonNameFile.c_str());
        exit(-1);
    }
    nFile=fopen(nameFile.c_str(),"r");
    if (!nFile)
    {   
        printf("ERROR %s NOT FOUND\n",nameFile.c_str());
        exit(-1);
    }

	// Read number of stars in the Hipparcos catalog
    unsigned int catalogSize=0;
    fread((char*)&catalogSize,4,1,hipFile);
    LE_TO_CPU_INT32(catalogSize,catalogSize);

    StarArraySize = catalogSize;//120417;

    // Create the sequential array
    StarArray = new Hip_Star*[StarArraySize];
	for (int i=0;i<StarArraySize;++i)
	{
		StarArray[i] = NULL;
	}

	// Read common names & names catalog
	char ** commonNames = new char*[catalogSize];
	char ** names = new char*[catalogSize];

	for (unsigned int i=0; i<catalogSize; i++)
	{
		commonNames[i]=NULL;
		names[i]=NULL;
	}

	int tmp;
	char tmpName[20];

	while(!feof(cnFile))
	{
		fscanf(cnFile,"%d|%s\n",&tmp,tmpName);
		commonNames[tmp] = strdup(tmpName);
	}
	while(!feof(nFile))
	{
		fscanf(nFile,"%d|%s\n",&tmp,tmpName);
		names[tmp] = strdup(tmpName);
	}
	fclose(cnFile);
    fclose(nFile);

	// Read binary file Hipparcos catalog  
    Hip_Star * e = NULL;
    for(unsigned int i=0;i<catalogSize;i++)
    {
	    e = new Hip_Star;
	    e->HP=(unsigned int)i;
        if (!e->read(hipFile))
        {
        	delete e;
        	e=NULL;
        	continue;
        }
		else
		if (e->Mag>9)
 		{
        	delete e;
        	e=NULL;
        	continue;
        }

        // Set names if any
        if(commonNames[e->HP]) e->CommonName=commonNames[e->HP];
        if(names[e->HP]) e->Name=names[e->HP];

        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);
        StarArray[e->HP]=e;
    }

	delete commonNames;
	delete names;

    fclose(hipFile);

	starTexture = new s_texture("star16x16",TEX_LOAD_TYPE_PNG_SOLID);  // Load star texture

    starFont = new s_font(11.f,"spacefont", font_fileName); // load Font
    if (!starFont)
    {
	    printf("Can't create starFont\n");
        exit(-1);
    }

}


// Load common names from file 
void Hip_Star_mgr::load_common_names(const string& commonNameFile)
{
  //    printf("Loading star common names data...\n");
    FILE *cnFile;
    cnFile = NULL;

    // clear existing common names (would be faster if common names were in separate array
    // since relatively few are named)
    for (int i=0; i<StarArraySize; i++) {
      if(StarArray[i] != NULL) {
	StarArray[i]->CommonName = NULL;
      }
    }

    cnFile=fopen(commonNameFile.c_str(),"r");
    if (!cnFile)
    {   
        printf("WARNING %s NOT FOUND\n",commonNameFile.c_str());
        return;
    }


    int tmp;
    char tmpName[20];   // too small?
    Hip_Star *star;

    while(!feof(cnFile)) {
      fscanf(cnFile,"%d|%s\n",&tmp,tmpName);

      // update star common name
      star = search(tmp);

      if( star != NULL ) {
	star->CommonName = strdup(tmpName);
      }
    }
    fclose(cnFile);
    

}



// Draw all the stars
void Hip_Star_mgr::draw(float _star_scale, float _star_mag_scale, float _twinkle_amount, int name_ON,
						float maxMagStarName, Vec3f equ_vision,
						tone_reproductor* _eye, Projector* prj, bool _gravity_label)
{
	Hip_Star::twinkle_amount = _twinkle_amount;
	Hip_Star::star_scale = _star_scale;
	Hip_Star::star_mag_scale = _star_mag_scale;
	Hip_Star::eye = _eye;
	Hip_Star::proj = prj;
	Hip_Star::gravity_label = _gravity_label;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	glBlendFunc(GL_ONE, GL_ONE);

	// Find the star zones which are in the screen
	int nbZones=0;
	static int * zoneList;  // WARNING this is almost a memory leak...

	nbZones = HipGrid.Intersect(equ_vision, prj->get_fov()*M_PI/180.f*1.2f, zoneList);
	float maxMag = 5.5f+60.f/prj->get_fov();

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<Hip_Star *>::iterator end;
	static vector<Hip_Star *>::iterator iter;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
    	for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
    	{
			// If too small, skip and Compute the 2D position and check if in screen
			if ((*iter)->Mag>maxMag || !prj->project_earth_equ_check((*iter)->XYZ, (*iter)->XY)) continue;
			(*iter)->draw();
			if ((*iter)->CommonName && name_ON && (*iter)->Mag<maxMagStarName)
            {
		       	(*iter)->draw_name(starFont);
               	glBindTexture (GL_TEXTURE_2D, starTexture->getID());
            }
	    }
	}

    prj->reset_perspective_projection();
}

// Draw all the stars
void Hip_Star_mgr::draw_point(float _star_scale, float _star_mag_scale, float _twinkle_amount, int name_ON,
						float maxMagStarName, Vec3f equ_vision,
						tone_reproductor* _eye, Projector* prj, bool _gravity_label)
{
	Hip_Star::twinkle_amount = _twinkle_amount;
	Hip_Star::star_scale = _star_scale;
	Hip_Star::star_mag_scale = _star_mag_scale;
	Hip_Star::eye = _eye;
	Hip_Star::proj = prj;
	Hip_Star::gravity_label = _gravity_label;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	glBlendFunc(GL_ONE, GL_ONE);

	// Find the star zones which are in the screen
	int nbZones=0;
	static int * zoneList;  // WARNING this is almost a memory leak...

	nbZones = HipGrid.Intersect(equ_vision, prj->get_fov()*M_PI/180.f*1.2f, zoneList);
	float maxMag = 5.5f+60.f/prj->get_fov();

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<Hip_Star *>::iterator end;
	static vector<Hip_Star *>::iterator iter;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
    	for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
    	{
			// If too small, skip and Compute the 2D position and check if in screen
			if ((*iter)->Mag>maxMag || !prj->project_earth_equ_check((*iter)->XYZ, (*iter)->XY)) continue;
			(*iter)->draw_point();
			if ((*iter)->CommonName && name_ON && (*iter)->Mag<maxMagStarName)
            {
		       	(*iter)->draw_name(starFont);
               	glBindTexture (GL_TEXTURE_2D, starTexture->getID());
            }
	    }
	}

    prj->reset_perspective_projection();
}

// Look for a star by XYZ coords
Hip_Star * Hip_Star_mgr::search(Vec3f Pos)
{
    Pos.normalize();
    Hip_Star * nearest=NULL;
    float angleNearest=0.;

    for(int i=0; i<StarArraySize; i++)
    {
		if (!StarArray[i]) continue;
		if 	(StarArray[i]->XYZ[0]*Pos[0] + StarArray[i]->XYZ[1]*Pos[1] +
			StarArray[i]->XYZ[2]*Pos[2]>angleNearest)
    	{
			angleNearest = StarArray[i]->XYZ[0]*Pos[0] +
				StarArray[i]->XYZ[1]*Pos[1] + StarArray[i]->XYZ[2]*Pos[2];
        	nearest=StarArray[i];
    	}
    }
    if (angleNearest>RADIUS_STAR*0.9999)
    {   
	    return nearest;
    }
    else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<stel_object*> Hip_Star_mgr::search_around(Vec3d v, double lim_fov)
{
	vector<stel_object*> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	static Vec3d equPos;

	for(int i=0; i<StarArraySize; i++)
    {
		if (!StarArray[i]) continue;
		if (StarArray[i]->XYZ[0]*v[0] + StarArray[i]->XYZ[1]*v[1] + StarArray[i]->XYZ[2]*v[2]>=cos_lim_fov)
    	{
			result.push_back(StarArray[i]);
    	}
    }
	return result;
}

// Search the star by HP number
Hip_Star * Hip_Star_mgr::search(unsigned int _HP)
{
	if (StarArray[_HP] && StarArray[_HP]->HP == _HP)
		return StarArray[_HP];
    return NULL;
}

/*
void Hip_Star_mgr::Save(void)
{
	FILE * fic = fopen("cat.fab","wb");
    fwrite((char*)&StarArraySize,4,1,fic);

    for(int i=0;i<StarArraySize;i++)
    {
    	Hip_Star * s = StarArray[i];
    	float RAf=0, DEf=0;
    	unsigned short int mag=0;
    	unsigned short int type=0;

    	if (s)
    	{
    		RAf=s->r; DEf=s->d;
    		mag = s->magp;
    		type = s->typep;
    	}
    	fwrite((char*)&RAf,4,1,fic);
    	fwrite((char*)&DEf,4,1,fic);
    	fwrite((char*)&mag,2,1,fic);
    	fwrite((char*)&type,2,1,fic);
    }
   	fclose(fic);
}*/
