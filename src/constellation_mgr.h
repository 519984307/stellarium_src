/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�eau
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

#ifndef _CONSTELLATION_MGR_H_
#define _CONSTELLATION_MGR_H_

#include <vector>

#include "stel_object.h"
#include "fader.h"
#include "loadingbar.h"
#include "translator.h"
#include "stelobjectmgr.h"

class ToneReproductor;
class HipStarMgr;
class Constellation;
class Projector;
class Navigator;

class ConstellationMgr : public StelObjectMgr
{
public:
    ConstellationMgr(HipStarMgr *_hip_stars);
	virtual ~ConstellationMgr();
    
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "constellations";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproductor *eye); //! Draw constellation lines, art, names and boundaries
	virtual void update(double deltaTime);
	virtual void updateI18n();
	virtual void updateSkyCulture(LoadingBar& lb);	
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	virtual vector<StelObject> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	
	//! Return the matching constellation object's pointer if exists or NULL
	//! @param nameI18n The case sensistive constellation name
	virtual StelObject searchByNameI18n(const wstring& nameI18n) const;
	
	//! @brief Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters	
	
	//! @brief Read constellation names from the given file
	//! @param namesFile Name of the file containing the constellation names in english
	void loadNames(const string& names_file);
	
	//! @brief Load constellation line shapes, art textures and boundaries shapes from data files
	void loadLinesAndArt(const string& lines_file, const string& art_file, LoadingBar& lb);
	
	//! Set constellation art fade duration
	void setArtFadeDuration(float duration);
	//! Get constellation art fade duration
	float getArtFadeDuration() const {return artFadeDuration;}
		
	//! Set constellation maximum art intensity
	void setArtIntensity(float f);
	//! Set constellation maximum art intensity
	float getArtIntensity() const {return artMaxIntensity;}
	
	//! Set whether constellation art will be displayed
	void setFlagArt(bool b);
	//! Get whether constellation art is displayed
	bool getFlagArt(void) const {return flagArt;}
	
	//! Set whether constellation path lines will be displayed
	void setFlagLines(bool b);
	//! Get whether constellation path lines are displayed
	bool getFlagLines(void) const {return flagLines;}
	
	//! Set whether constellation boundaries lines will be displayed
	void setFlagBoundaries(bool b);
	//! Get whether constellation boundaries lines are displayed
	bool getFlagBoundaries(void) const {return flagBoundaries;}
	
	//! Set whether constellation names will be displayed
	void setFlagNames(bool b);
	//! Set whether constellation names are displayed
	bool getFlagNames(void) const {return flagNames;}
	
	//! Set whether selected constellation must be displayed alone
	void setFlagIsolateSelected(bool s) { isolateSelected = s; setSelectedConst(selected);}
	//! Get whether selected constellation is displayed alone
	bool getFlagIsolateSelected(void) const { return isolateSelected;}
	
	//! Define line color
	void setLinesColor(const Vec3f& c);
	//! Get line color
	Vec3f getLinesColor() const;
	
	//! Define boundary color
	void setBoundariesColor(const Vec3f& c);
	//! Get current boundary color
	Vec3f getBoundariesColor() const;
		
	//! Set label color for names
	void setNamesColor(const Vec3f& c);
	//! Get label color for names
	Vec3f getNamesColor() const;
	
	//! Define font size to use for constellation names display
	void setFontSize(double newFontSize);
	
	//! Define which constellation is selected from its abbreviation
	void setSelected(const string& abbreviation) {setSelectedConst(findFromAbbreviation(abbreviation));}
	
	//! Define which constellation is selected from a star number
	void setSelected(const StelObject &s) {if (!s) setSelectedConst(NULL); else setSelectedConst(is_star_in(s));}
	
	StelObject getSelected(void) const;	

private:
	bool loadBoundaries(const string& conCatFile);
	void draw_lines(Projector * prj) const;
	void draw_art(Projector * prj, const Navigator * nav) const;
	void draw_names(Projector * prj) const;
	void drawBoundaries(Projector* prj) const;	
	void setSelectedConst(Constellation* c);

    Constellation* is_star_in(const StelObject &s) const;
    Constellation* findFromAbbreviation(const string& abbreviation) const;		
    vector<Constellation*> asterisms;
	double fontSize;
    s_font* asterFont;
    HipStarMgr* hipStarMgr;
	Constellation* selected;
	bool isolateSelected;
	vector<vector<Vec3f> *> allBoundarySegments;

	string lastLoadedSkyCulture;	// Store the last loaded sky culture directory name
	
	// These are THE master settings - individual constellation settings can vary based on selection status
	bool flagNames;
	bool flagLines;
	bool flagArt;
	bool flagBoundaries;
	float artFadeDuration;
	float artMaxIntensity;
};

#endif // _CONSTELLATION_MGR_H_
