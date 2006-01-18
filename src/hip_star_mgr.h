/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _HIP_STAR_MGR_H_
#define _HIP_STAR_MGR_H_

#include <vector>
#include <string>
#include "hip_star.h"
#include "grid.h"
#include "fader.h"
#include "loadingbar.h"

using namespace std ;

class HipStarMgr  
{
public:
    HipStarMgr();
    virtual ~HipStarMgr();
	void init(float font_size, const string& font_name, const string& hipCatFile, const string& commonNameFile, const string& sciNameFile, LoadingBar& lb);
	void update(int delta_time) {names_fader.update(delta_time); starsFader.update(delta_time);}
	void set_names_fade_duration(float duration) {names_fader.set_duration((int) (duration * 1000.f));}
	int load_common_names(const string& commonNameFile);
	void load_sci_names(const string& sciNameFile);
    void draw(Vec3f equ_vision, ToneReproductor* _eye, Projector* prj);	// Draw all the stars
   
    void save(void);                    	// Debug function
    HipStar *search(Vec3f Pos);  		// Search the star by position
	HipStar *search(const string&);	// Search the star by string (incl catalog prefix)
	HipStar *searchHP(unsigned int);	// Search the star by HP number
	HipStar *searchSAO(unsigned int);	// Search the star by SAO number
	HipStar *searchHD(unsigned int);	// Search the star by HD number
	// Return a stl vector containing the stars located inside the lim_fov circle around position v
	vector<StelObject*> search_around(Vec3d v, double lim_fov);
	vector<wstring> getNames(void) { return lstCommonNames; }
	unsigned int getCommonNameHP(wstring _commonname);
	void set_label_color(const Vec3f& c) {HipStar::label_color = c;}
	void set_circle_color(const Vec3f& c) {HipStar::circle_color = c;}
	
	/** Set display flag for Stars */
	void setFlagStars(bool b) {starsFader=b;}
	/** Get display flag for Stars */
	bool getFlagStars(void) const {return starsFader==true;}	
	
	/** Set display flag for Star names */
	void setFlagStarName(bool b) {names_fader=b;}
	/** Get display flag for Star names */
	bool getFlagStarName(void) const {return names_fader==true;}
	
	/** Set display flag for Star Scientific names */
	void setFlagStarSciName(bool b) {flagStarSciName=b;}
	/** Get display flag for Star Scientific names */
	bool getFlagStarSciName(void) const {return flagStarSciName;}	
	
	/** Set flag for Star twinkling */
	void setFlagStarTwinkle(bool b) {flagStarTwinkle=b;}
	/** Get flag for Star twinkling */
	bool getFlagStarTwinkle(void) const {return flagStarTwinkle;}	
		
	/** Set flag for displaying Star as GLpoints (faster but not so nice) */
	void setFlagPointStar(bool b) {flagPointStar=b;}
	/** Get flag for displaying Star as GLpoints (faster but not so nice) */
	bool getFlagPointStar(void) const {return flagPointStar;}	
	
	/** Set maximum magnitude at which stars names are displayed */
	void setMaxMagStarName(float b) {maxMagStarName=b;}
	/** Get maximum magnitude at which stars names are displayed */
	float getMaxMagStarName(void) const {return maxMagStarName;}	
	
	/** Set maximum magnitude at which stars scientific names are displayed */
	void setMaxMagStarSciName(float b) {maxMagStarSciName=b;}
	/** Get maximum magnitude at which stars scientific names are displayed */
	float getMaxMagStarSciName(void) const {return maxMagStarSciName;}	
		
	/** Set base stars display scaling factor */
	void setStarScale(float b) {starScale=b;}
	/** Get base stars display scaling factor */
	float getStarScale(void) const {return starScale;}			
	
	/** Set stars display scaling factor wrt magnitude */
	void setStarMagScale(float b) {starMagScale=b;}
	/** Get base stars display scaling factor wrt magnitude */
	float getStarMagScale(void) const {return starMagScale;}

	/** Set stars twinkle amount */
	void setStarTwinkleAmount(float b) {twinkleAmount=b;}
	/** Get stars twinkle amount */
	float getStarTwinkleAmount(void) const {return twinkleAmount;}
	
	/** Set stars limiting display magnitude */
	void setStarLimitingMag(float f) {limitingMag = f;}
	/** Get stars limiting display magnitude */
	float getStarLimitingMag(void) const {return limitingMag;}
		
private:
	void saveData(void);

	/** Draw the stars rendered as GLpoint. This may be faster but it is not so nice */
	void drawPoint(Vec3f equ_vision, ToneReproductor* _eye, Projector* prj);	// Draw all the stars as points
 
	// Load all the stars from the files
	bool load_SAO_HIP_NAME_data(const string& hipCatFile, LoadingBar& lb, float maxmag);
	bool load_combined(const string& hipCatFile, LoadingBar& lb);
	bool load_combined_bin(const string& hipCatFile, LoadingBar& lb);

	bool load_doubleraw(const string& hipCatFile);
	bool load_double(const string& hipCatFile);
	bool load_variable(const string& hipCatFile);
	bool save_double(void);
	bool save_variable(void);

	bool save_combined(void);
	bool save_combined_bin(void);
	bool save_record(FILE *dataFile, HipStar *e);
	bool save_name_record(FILE *dataFile, unsigned int HP, string name);
	bool save_record_bin(FILE *dataFile, HipStar *e);

	void load_data(const string& hipCatFile, LoadingBar& lb);

	LinearFader names_fader;
	LinearFader starsFader;

	float starScale;
	float starMagScale;
	bool flagStarName;
	bool flagStarSciName;
	float maxMagStarName;
	float maxMagStarSciName;
	bool flagStarTwinkle;
	float twinkleAmount;
	bool flagPointStar;
	bool gravityLabel;
	float limitingMag;                  // limiting magnitude at 60 degree fov
	
	vector<HipStar*>* starZones;       // array of star vector with the grid id as array rank
	Grid HipGrid;                       // Grid for opimisation
	HipStar * StarArray;  				// Sequential Array of the star for memory allocation optimization
	HipStar ** StarFlatArray; 			// The simple array of the star for sequential research
	int starArraySize;                  // Number of star in the array

	int CombinedTotal;
	// TEMP for building data
	HipStar *SAOStarArray;  			// Sequential Array of the star for memory allocation optimization

	HipStar **HPStarFlatArray; 		// The simple array of the star for sequential research
	int HPStarArraySize;               // Number of star in the array

	HipStar **HDStarFlatArray; 		// The simple array of the star for sequential research
	int HDStarArraySize;               // Number of star in the array

	HipStar **SAOStarFlatArray; 		// The simple array of the star for sequential research
	int SAOStarArraySize;                  // Number of star in the array

	s_texture *starTexture;			// star texture
	s_texture *starcTexture;			// charted interior disc
	
	vector<wstring> lstCommonNames;
	vector<unsigned int> lstCommonNamesHP;
};


#endif // _STAR_MGR_H_
