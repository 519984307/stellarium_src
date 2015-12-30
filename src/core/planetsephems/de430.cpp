/*
Copyright (c) 2015 Holger Niessner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "de430.hpp"
#include "StelUtils.hpp"
#include "StelCore.hpp"
#include "StelApp.hpp"

#ifdef __cplusplus
  extern "C" {
#endif

static void * ephem;

static Vec3d tempECL = Vec3d(0,0,0);
static Vec3d tempICRF = Vec3d(0,0,0);
static char nams[JPL_MAX_N_CONSTANTS][6];
static double vals[JPL_MAX_N_CONSTANTS];
static double tempXYZ[6];

static bool initDone = false;

void InitDE430(const char* filepath)
{
	ephem = jpl_init_ephemeris(filepath, nams, vals);

	if(jpl_init_error_code() != 0)
	{
		StelApp::getInstance().getCore()->setDe430Active(false);
		qDebug() << "Error "<< jpl_init_error_code() << "at DE430 init:" << jpl_init_error_message();
	}
	else
	{
		initDone = true;
		double jd1, jd2;
		jd1=jpl_get_double(ephem, JPL_EPHEM_START_JD);
		jd2=jpl_get_double(ephem, JPL_EPHEM_END_JD);
		qDebug() << "DE430 init successful. startJD=" << QString::number(jd1, 'f', 4) << "endJD=" << QString::number(jd2, 'f', 4);
	}
}

void TerminateDE430()
{
  jpl_close_ephemeris(ephem);
}

void GetDe430Coor(const double jde, const int planet_id, double * xyz, const int centralBody_id)
{
    if(initDone)
    {
	jpl_pleph(ephem, jde, planet_id, centralBody_id, tempXYZ, 0);

        tempICRF = Vec3d(tempXYZ[0], tempXYZ[1], tempXYZ[2]);
        tempECL = StelCore::matJ2000ToVsop87 * tempICRF;

        xyz[0] = tempECL[0];
        xyz[1] = tempECL[1];
        xyz[2] = tempECL[2];
    }
}

//void GetDe430OsculatingCoor(double jd0, double jd, int planet_id, double *xyz, const int centralBody_id)
//{
	
//}

#ifdef __cplusplus
  }
#endif
