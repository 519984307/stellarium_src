/*
 * Stellarium
 * Copyright (C) 2014-2015 Marcos Cardinot
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

#include "Meteor.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"

#include <QtMath>

Meteor::Meteor(const StelCore* core, const StelTextureSP& bolideTexture)
	: m_core(core)
	, m_alive(false)
	, m_speed(72.)
	, m_initialZ(1.)
	, m_finalZ(1.)
	, m_absMag(.5)
	, m_aptMag(.5)
	, m_bolideTexture(bolideTexture)
	, m_segments(10)
{
	qsrand(QDateTime::currentMSecsSinceEpoch());
}

Meteor::~Meteor()
{
	m_bolideTexture.clear();
}

void Meteor::init(const float& radiantAlpha, const float& radiantDelta,
		  const float& speed, const QList<ColorPair> colors)
{
	// meteor velocity in km/s
	m_speed = speed;

	// find the radiant in horizontal coordinates
	Vec3d radiantAltAz;
	StelUtils::spheToRect(radiantAlpha, radiantDelta, radiantAltAz);
	radiantAltAz = m_core->j2000ToAltAz(radiantAltAz);
	float radiantAlt, radiantAz;
	// S is zero, E is 90 degrees (SDSS)
	StelUtils::rectToSphe(&radiantAz, &radiantAlt, radiantAltAz);

	// meteors won't be visible if radiant is below 0degrees
	if (radiantAlt < 0.f)
	{
		return;
	}

	// define the radiant coordinate system
	// rotation matrix to align z axis with radiant
	m_matAltAzToRadiant = Mat4d::zrotation(radiantAz) * Mat4d::yrotation(M_PI_2 - radiantAlt);

	// select a random initial meteor altitude in the horizontal system [MIN_ALTITUDE, MAX_ALTITUDE]
	float initialAlt = MIN_ALTITUDE + (MAX_ALTITUDE - MIN_ALTITUDE) * ((float) qrand() / ((float) RAND_MAX + 1));

	// calculates the max z-coordinate for the currrent radiant
	float maxZ = meteorZ(M_PI_2 - radiantAlt, initialAlt);

	// meteor trajectory
	// select a random xy position in polar coordinates (radiant system)

	float xyDist = maxZ * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, maxZ]
	float theta = 2 * M_PI * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, 2pi]

	// initial meteor coordinates (radiant system)
	m_position[0] = xyDist * qCos(theta);
	m_position[1] = xyDist * qSin(theta);
	m_position[2] = maxZ;
	m_posTrain = m_position;

	// store the initial z-component (radiant system)
	m_initialZ = m_position[2];

	// find the initial meteor coordinates in the horizontal system
	Vec3d positionAltAz = m_position;
	positionAltAz.transfo4d(m_matAltAzToRadiant);

	// find the angle from horizon to meteor
	float meteorAlt = qAsin(positionAltAz[2] / positionAltAz.length());

	// this meteor should not be visible if it is above the maximum altitude
	// or if it's below the horizon!
	if (positionAltAz[2] > MAX_ALTITUDE || meteorAlt <= 0.f)
	{
		return;
	}

	// determine the final z-component and the min distance between meteor and observer
	if (radiantAlt < 0.0262f) // (<1.5 degrees) earth grazing meteor ?
	{
		// earth-grazers are rare!
		// introduce a probabilistic factor just to make them a bit harder to occur
		float prob = ((float) qrand() / ((float) RAND_MAX + 1));
		if (prob > 0.3f) {
			return;
		}

		// limit lifetime to 12sec
		m_finalZ = -m_position[2];
		m_finalZ = qMax(m_position[2] - m_speed * 12.f, (double) m_finalZ);

		m_minDist = xyDist;
	}
	else
	{
		// limit lifetime to 12sec
		m_finalZ = meteorZ(M_PI_2 - meteorAlt, MIN_ALTITUDE);
		m_finalZ = qMax(m_position[2] - m_speed * 12.f, (double) m_finalZ);

		m_minDist = qSqrt(m_finalZ * m_finalZ + xyDist * xyDist);
	}

	// a meteor cannot hit the observer!
	if (m_minDist < MIN_ALTITUDE) {
		return;
	}

	// determine intensity [-3; 4.5]
	float Mag1 = (float) qrand() / ((float) RAND_MAX + 1) * 7.5f - 3;
	float Mag2 = (float) qrand() / ((float) RAND_MAX + 1) * 7.5f - 3;
	float Mag = (Mag1 + Mag2) / 2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	m_core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	m_absMag = rcMag.radius <= 1.2f ? 0.f : rcMag.luminance;
	if (m_absMag == 0.f) {
		return;
	}

	// most visible meteors are under about 184km distant
	// scale max mag down if outside this range
	float scale = qPow(184.0 / m_minDist, 2);
	m_absMag *= qMin(scale, 1.0f);

	// build the color vector
	buildColorVectors(colors);

	m_alive = true;
}

bool Meteor::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	if (m_position[2] < m_finalZ)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		m_absMag -= deltaTime * 2.f;
		if(m_absMag <= 0.f)
		{
			m_alive = false;
			return false;    // no longer visible
		}
	}

	m_position[2] -= m_speed * deltaTime;

	// train doesn't extend beyond start of burn
	if (m_position[2] + m_speed * 0.5f > m_initialZ)
	{
		m_posTrain[2] = m_initialZ;
	}
	else
	{
		m_posTrain[2] -= m_speed * deltaTime;
	}

	// update apparent magnitude based on distance to observer
	float scale = qPow(m_minDist / m_position.length(), 2);
	m_aptMag = m_absMag * qMin(scale, 1.f);
	m_aptMag = qMax(m_aptMag, 0.f);

	return true;
}

void Meteor::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	float thickness;
	float bolideSize;
	calculateThickness(core, thickness, bolideSize);

	drawTrain(sPainter, thickness);

	drawBolide(sPainter, bolideSize);
}

Vec4f Meteor::getColorFromName(QString colorName)
{
	int R, G, B; // 0-255
	if (colorName == "violet")
	{ // Calcium
		R = 176;
		G = 67;
		B = 172;
	}
	else if (colorName == "blueGreen")
	{ // Magnesium
		R = 0;
		G = 255;
		B = 152;
	}
	else if (colorName == "yellow")
	{ // Iron
		R = 255;
		G = 255;
		B = 0;
	}
	else if (colorName == "orangeYellow")
	{ // Sodium
		R = 255;
		G = 160;
		B = 0;
	}
	else if (colorName == "red")
	{ // atmospheric nitrogen and oxygen
		R = 255;
		G = 30;
		B = 0;
	}
	else
	{ // white
		R = 255;
		G = 255;
		B = 255;
	}

	return Vec4f(R/255.f, G/255.f, B/255.f, 1);
}

void Meteor::buildColorVectors(const QList<ColorPair> colors)
{
	// building color arrays (line and prism)
	QList<Vec4f> lineColor;
	QList<Vec4f> trainColor;
	foreach (ColorPair color, colors)
	{
		// segments to be painted with the current color
		int segs = qRound(m_segments * (color.second / 100.f)); // rounds to nearest integer
		for (int s = 0; s < segs; ++s)
		{
			Vec4f rgba = getColorFromName(color.first);
			lineColor.append(rgba);
			trainColor.append(rgba);
			trainColor.append(rgba);
		}
	}

	// make sure that all segments have been painted!
	const int segs = lineColor.size();
	if (segs < m_segments) {
		// use the last color to paint the last segments
		Vec4f rgba = getColorFromName(colors.last().first);
		for (int s = segs; s < m_segments; ++s) {
			lineColor.append(rgba);
			trainColor.append(rgba);
			trainColor.append(rgba);
		}
	} else if (segs > m_segments) {
		// remove the extra segments
		for (int s = segs; s > m_segments; --s) {
			lineColor.removeLast();
			trainColor.removeLast();
			trainColor.removeLast();
		}
	}

	// multi-color ?
	// select a random segment to be the first (to alternate colors)
	if (colors.size() > 1) {
		int firstSegment = (segs - 1) * ((float) qrand() / ((float) RAND_MAX + 1)); // [0, segments-1]
		QList<Vec4f> lineColor2 = lineColor.mid(0, firstSegment);
		QList<Vec4f> lineColor1 = lineColor.mid(firstSegment);
		lineColor.clear();
		lineColor.append(lineColor1);
		lineColor.append(lineColor2);

		firstSegment *= 2;
		QList<Vec4f> trainColor2 = trainColor.mid(0, firstSegment);
		QList<Vec4f> trainColor1 = trainColor.mid(firstSegment);
		trainColor.clear();
		trainColor.append(trainColor1);
		trainColor.append(trainColor2);
	}

	m_lineColorVector = lineColor.toVector();
	m_trainColorVector = trainColor.toVector();
}

float Meteor::meteorZ(float zenithAngle, float altitude)
{
	float distance;

	if (zenithAngle > 1.13446401f) // > 65 degrees?
	{
		float zcos = qCos(zenithAngle);
		distance = qSqrt(EARTH_RADIUS2 * qPow(zcos, 2)
				 + 2 * EARTH_RADIUS * altitude
				 + qPow(altitude, 2));
		distance -= EARTH_RADIUS * zcos;
	}
	else
	{
		// (first order approximation)
		distance = altitude / qCos(zenithAngle);
	}

	return distance;
}

Vec3d Meteor::altAzToRadiant(Vec3d position)
{
	position.transfo4d(m_matAltAzToRadiant.transpose());
	position *= 1242;
	return position;
}

Vec3d Meteor::radiantToAltAz(Vec3d position)
{
	position /= 1242.0; // 1242 to scale down under 1
	position.transfo4d(m_matAltAzToRadiant);
	return position;
}

void Meteor::calculateThickness(const StelCore* core, float& thickness, float& bolideSize)
{
	float maxFOV = core->getMovementMgr()->getMaxFov();
	float FOV = core->getMovementMgr()->getCurrentFov();
	thickness = 2*log(FOV + 0.25)/(1.2*maxFOV - (FOV + 0.25)) + 0.01;
	if (FOV <= 0.5)
	{
		thickness = 0.013 * FOV; // decreasing faster
	}
	else if (FOV > 100.0)
	{
		thickness = 0; // remove prism
	}

	bolideSize = thickness*3;
}

void Meteor::drawBolide(StelPainter& sPainter, const float& bolideSize)
{
	if (!bolideSize || !m_bolideTexture)
	{
		return;
	}

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1, 1, 1, m_aptMag);

	Vec3d topLeft = m_position;
	topLeft[1] -= bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(topLeft));
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = m_position;
	topRight[0] -= bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(topRight));
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = m_position;
	bottomRight[1] += bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(bottomRight));
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = m_position;
	bottomLeft[0] += bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(bottomLeft));
	colorArrayBolide.push_back(bolideColor);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	sPainter.enableClientStates(true, true, true);
	m_bolideTexture->bind();
	static const float texCoordData[] = {1.,0., 0.,0., 0.,1., 1.,1.};
	sPainter.setTexCoordPointer(2, GL_FLOAT, texCoordData);
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayBolide.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayBolide.constData());
	sPainter.drawFromArray(StelPainter::TriangleFan, vertexArrayBolide.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}

void Meteor::drawTrain(StelPainter& sPainter, const float& thickness)
{
	if (m_segments != m_lineColorVector.size() || 2*m_segments != m_trainColorVector.size())
	{
		qWarning() << "Meteor: color arrays have an inconsistent size!";
		return;
	}

	// train (triangular prism)
	//
	QVector<Vec3d> vertexArrayLine;
	QVector<Vec3d> vertexArrayL;
	QVector<Vec3d> vertexArrayR;
	QVector<Vec3d> vertexArrayTop;

	Vec3d posTrainB = m_posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = m_posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = m_posTrain;
	posTrainR[0] -= thickness;

	for (int i = 0; i < m_segments; ++i)
	{
		double height = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/(m_segments-1);
		Vec3d posi;

		posi = m_posTrain;
		posi[2] = height;
		vertexArrayLine.push_back(radiantToAltAz(posi));

		posi = posTrainB;
		posi[2] = height;
		vertexArrayL.push_back(radiantToAltAz(posi));
		vertexArrayR.push_back(radiantToAltAz(posi));

		posi = posTrainL;
		posi[2] = height;
		vertexArrayL.push_back(radiantToAltAz(posi));
		vertexArrayTop.push_back(radiantToAltAz(posi));

		posi = posTrainR;
		posi[2] = height;
		vertexArrayR.push_back(radiantToAltAz(posi));
		vertexArrayTop.push_back(radiantToAltAz(posi));

		float mag = m_aptMag * ((float) i / (float) (m_segments-1));
		m_lineColorVector[i][3] = mag;
		m_trainColorVector[i*2][3] = mag;
		m_trainColorVector[i*2+1][3] = mag;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	if (thickness)
	{
		sPainter.setColorPointer(4, GL_FLOAT, m_trainColorVector.constData());

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayL.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayL.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayR.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayR.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayTop.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayTop.size(), 0, true);
	}
	sPainter.setColorPointer(4, GL_FLOAT, m_lineColorVector.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayLine.constData());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArrayLine.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}
