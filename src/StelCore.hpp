/*
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_

#include <string>

#include "Navigator.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"

class Observer;

//!  @brief Main class for stellarium core processing.
//!
//! Manage all the base modules which must be present in stellarium
class StelCore
{
public:
	// Inputs are the locale directory and root directory and callback function for recording actions
    StelCore();
    virtual ~StelCore();

	//! Init and load all main core components from the passed config file.
	void init(const InitParser& conf, LoadingBar& lb);

	//! Init projection temp TODO remove
	void initProj(const InitParser& conf);

	//! Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! Update core state before drawing modules
	//! @param delta_time the time increment in ms.
	void preDraw(int delta_time);
	void postDraw();
	
	//! Get the current projector used in the core
	Projector* getProjection() {return projection;}
	const Projector* getProjection() const {return projection;}
	
	//! Get the current navigation (manages frame transformation) used in the core
	Navigator* getNavigation() {return navigation;}
	const Navigator* getNavigation() const {return navigation;}

	//! Get the current tone converter used in the core
	ToneReproducer* getToneReproducer() {return tone_converter;}
	const ToneReproducer* getToneReproducer() const {return tone_converter;}

	//! Get the current observer description
	Observer* getObservatory() {return observatory;}
	//! Get the current observer description (as a const object)
	const Observer* getObservatory() const {return observatory;}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Planets flags
	bool setHomePlanet(string planet);

private:
	// Main elements of the program
	Navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observer * observatory;			// Manage observer position
	Projector * projection;				// Manage the projection mode and matrix
	ToneReproducer * tone_converter;	// Tones conversion between stellarium world and display device
	class MovementMgr* movementMgr;		// Manage vision movements
};

#endif // _STEL_CORE_H_
