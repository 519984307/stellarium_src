/*
 * Copyright (C) 2013 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <cstdlib>
#include "MeteorShowers.hpp"
#include "MeteorStream.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include "StelToneReproducer.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"

MeteorStream::MeteorStream(const StelCore* core, double velocity, double radiantAlpha, double radiantDelta)
{
	speed = velocity;

	maxMag = 1; //start with the maximum mag

	//the meteor starts dead, after we'll calculate the position
	//if the position is within the bounds, this parameter will be changed to TRUE
	alive = false;

	double high_range = EARTH_RADIUS+HIGH_ALTITUDE;
	double low_range = EARTH_RADIUS+LOW_ALTITUDE;

	// view matrix of meteor model
	viewMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// find observer position in meteor coordinate system
	obs = core->altAzToJ2000(Vec3d(0,0,EARTH_RADIUS));
	obs.transfo4d(viewMatrix.transpose());

	// select random trajectory using polar coordinates in XY plane, centered on observer
	xydistance = (double)rand() / ((double)RAND_MAX+1)*(VISIBLE_RADIUS);
	double angle = (double)rand() / ((double)RAND_MAX+1)*2*M_PI;

	// set meteor start x,y
	position[0] = posInternal[0] = posTrain[0] = xydistance*cos(angle) + obs[0];
	position[1] = posInternal[1] = posTrain[1] = xydistance*sin(angle) + obs[1];

	// D is distance from center of earth
	double D = sqrt(position[0]*position[0] + position[1]*position[1]);

	if(D > high_range)     // won't be visible, meteor still dead
	{
		return;
	}

	posTrain[2] = position[2] = startH = sqrt(high_range*high_range - D*D);

	// determine end of burn point, and nearest point to observer for distance mag calculation
	// mag should be max at nearest point still burning
	endH = -startH;  // earth grazing
	minDist = xydistance;
	if(D <= low_range)
	{
		endH = sqrt(low_range*low_range - D*D);
		minDist = sqrt(xydistance*xydistance + pow(endH - obs[2], 2));
	}

	if(minDist > VISIBLE_RADIUS)
	{
		// on average, not visible (although if were zoomed ...)
		return; //meteor still dead
	}

	//If everything is ok until here,
	alive = true;  //the meteor is alive
	train = false; //and initially it is only a point

	// Determine drawing color given magnitude and eye
	// (won't be visible during daylight)

	// *** color varies somewhat based on speed, plus atmosphere reddening

	// determine intensity
	float Mag1 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
	float Mag2 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	mag = (5. + Mag) / 256.0;
	if(mag>250) mag = mag - 256;

	float term1 = std::exp(-0.92103f*(mag + 12.12331f)) * 108064.73f;

	float cmag=1.f;
	float rmag;

	// Compute the equivalent star luminance for a 5 arc min circle and convert it
	// in function of the eye adaptation
	const StelToneReproducer* eye = core->getToneReproducer();
	rmag = eye->adaptLuminanceScaled(term1);
	rmag = rmag/powf(core->getMovementMgr()->getCurrentFov(),0.85f)*500.f;

	// if size of star is too small (blink) we put its size to 1.2 --> no more blink
	// And we compensate the difference of brighteness with cmag
	if(rmag<1.2f)
	{
		cmag=rmag*rmag/1.44f;
	}

	mag = cmag;  // assumes white

	// most visible meteors are under about 180km distant
	// scale max mag down if outside this range
	float scale = 1;
	if(minDist!=0) scale = 180*180/(minDist*minDist);
	if(scale < 1) mag *= scale;
}

MeteorStream::~MeteorStream()
{
}

// returns true if alive
bool MeteorStream::update(double deltaTime)
{
	if(!alive)
		return(0);

	if(position[2] < endH)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out

		mag -= maxMag * deltaTime/500.0f;
		if(mag < 0)
			alive=0;    // no longer visible

	}

	// *** would need time direction multiplier to allow reverse time replay
	position[2] = position[2] - speed*deltaTime/1000.0f;

	// train doesn't extend beyond start of burn
	if(position[2] + speed*0.5f > startH)
	{
		posTrain[2] = startH ;
	}
	else
	{
		posTrain[2] -= speed*deltaTime/1000.0f;
	}

	// determine visual magnitude based on distance to observer
	double dist = sqrt(xydistance*xydistance + pow(position[2]-obs[2], 2));

	if(dist == 0) dist = .01;    // just to be cautious (meteor hits observer!)

	distMultiplier = minDist*minDist / (dist*dist);

	return(alive);
}


// returns true if visible
// Assumes that we are in local frame
void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
{
	if(!alive)
		return;

	const StelProjectorP proj = sPainter.getProjector();

	Vec3d spos = position;
	Vec3d epos = posTrain;

	// convert to equ
	spos.transfo4d(viewMatrix);
	epos.transfo4d(viewMatrix);

	// convert to local and correct for earth radius
	//[since equ and local coordinates in stellarium use same 0 point!]
	spos = core->j2000ToAltAz(spos);
	epos = core->j2000ToAltAz(epos);
	spos[2] -= EARTH_RADIUS;
	epos[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	spos/=1216;
	epos/=1216;

	if(train)
	{
		// connect this point with last drawn point
		double tmag = mag*distMultiplier;

		// compute an intermediate point so can curve slightly along projection distortions
		Vec3d posi = posInternal;
		posi[2] = position[2] + (posTrain[2] - position[2])/2;
		posi.transfo4d(viewMatrix);
		posi = core->j2000ToAltAz(posi);
		posi[2] -= EARTH_RADIUS;
		posi/=1216;

		// draw dark to light
		Vec4f colorArray[3];
		colorArray[0].set(0,0,0,0);
		colorArray[1].set(1,1,1,tmag*0.5);
		colorArray[2].set(1,1,1,tmag);
		Vec3d vertexArray[3];
		vertexArray[0]=epos;
		vertexArray[1]=posi;
		vertexArray[2]=spos;
		sPainter.setColorPointer(4, GL_FLOAT, colorArray);
		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArray);
		sPainter.enableClientStates(true, false, true);
		sPainter.drawFromArray(StelPainter::LineStrip, 3, 0, true);
		sPainter.enableClientStates(false);
	}
	else
	{
		Vec3d start;
		proj->project(spos, start);
		sPainter.drawPoint2d(start[0],start[1]);
	}

	train = 1;
}

bool MeteorStream::isAlive(void)
{
	return(alive);
}
