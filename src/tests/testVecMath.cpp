/*
 * Stellarium 
 * Copyright (C) 2019 Alexander Wolf
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

#include <QObject>
#include <QtDebug>
#include <QVariantList>
#include <QtTest>

#include "tests/testVecMath.hpp"

QTEST_GUILESS_MAIN(TestVecMath)

#define ERROR_LIMIT 1e-5

void TestVecMath::testVec2Math()
{
	Vec2i vi;
	Vec2f vf;
	Vec2d vd;

	vi.set(0,0);
	QVERIFY(vi==Vec2i(0,0));
	QVERIFY(vi!=Vec2i(0,1));
	vi+=Vec2i(5,5);
	QVERIFY(vi==Vec2i(5,5));
	vi-=Vec2i(2,2);
	QVERIFY(vi==Vec2i(3,3));
	vi*=Vec2i(2,2);
	QVERIFY(vi==Vec2i(6,6));
	vi*=2;
	QVERIFY(vi==Vec2i(12,12));
	vi/=2;
	QVERIFY(vi==Vec2i(6,6));
	vi/=Vec2i(3,3);
	QVERIFY(vi==Vec2i(2,2));
	vi = Vec2i(1,1) + Vec2i(9,9);
	QVERIFY(vi==Vec2i(10,10));
	vi = Vec2i(5,5) - Vec2i(6,6);
	QVERIFY(vi==Vec2i(-1,-1));
	vi = Vec2i(10,10) * Vec2i(2,1);
	QVERIFY(vi==Vec2i(20,10));
	vi = Vec2i(10,10) / Vec2i(2,5);
	QVERIFY(vi==Vec2i(5,2));
	vi = Vec2i(5);
	QVERIFY(vi.min(Vec2i(3,3))==Vec2i(3,3));
	QVERIFY(vi.max(Vec2i(3,3))==Vec2i(5,5));
	QVERIFY(vi.clamp(Vec2i(1,1),Vec2i(10,10))==Vec2i(5,5));
	QVERIFY(vi.dot(Vec2i(5,5))==50);
	vi.set(2,2);
	QVERIFY(vi.length()==2);
	QVERIFY(vi.lengthSquared()==8);
	QVERIFY(vi.toString()==QString("[2, 2]"));
	Vec2i vt(1,1);
	QVERIFY(vt==Vec2i(1,1));
	vi = vt;
	QVERIFY(vi==vt);
	vi = *vt;
	QVERIFY(vi==vt);
	vi.set(10,10);
	QVERIFY(vi/2==Vec2i(5,5));

	vf.set(0.f,0.f);
	QVERIFY(vf==Vec2f(0.f,0.f));
	QVERIFY(vf!=Vec2f(0.f,1.f));
	vf+=Vec2f(5.f,5.f);
	QVERIFY(vf==Vec2f(5.f,5.f));
	vf-=Vec2f(2.f,2.f);
	QVERIFY(vf==Vec2f(3.f,3.f));
	vf*=Vec2f(2.f,2.f);
	QVERIFY(vf==Vec2f(6.f,6.f));
	vf/=Vec2f(3.f,3.f);
	QVERIFY(vf==Vec2f(2.f,2.f));
	vf = Vec2f(1.f,1.f) + Vec2f(9.f,9.f);
	QVERIFY(vf==Vec2f(10.f,10.f));
	vf = Vec2f(5.f,5.f) - Vec2f(6.f,6.f);
	QVERIFY(vf==Vec2f(-1.f,-1.f));
	vf = Vec2f(10.f,10.f) * Vec2f(2.f,1.f);
	QVERIFY(vf==Vec2f(20.f,10.f));
	vf = Vec2f(10.f,10.f) / Vec2f(2.f,5.f);
	QVERIFY(vf==Vec2f(5.f,2.f));
	vf = Vec2f(5.f);
	QVERIFY(vf.min(Vec2f(3.f,3.f))==Vec2f(3.f,3.f));
	QVERIFY(vf.max(Vec2f(3.f,3.f))==Vec2f(5.f,5.f));
	QVERIFY(vf.clamp(Vec2f(1.f,1.f),Vec2f(10.f,10.f))==Vec2f(5.f,5.f));
	QVERIFY(qAbs(vf.dot(Vec2f(5.f,5.f)) - 50.f) <= ERROR_LIMIT);
	vf.set(2.f,2.f);
	QVERIFY(qAbs(vf.length() - 2.82843f) <= ERROR_LIMIT);
	QVERIFY(qAbs(vf.lengthSquared() - 8.f) <= ERROR_LIMIT);
	Vec2f vtf(1.f,1.f);
	QVERIFY(vtf==Vec2f(1.f,1.f));
	vf = vtf;
	QVERIFY(vf==vtf);
	vf = *vtf;
	QVERIFY(vf==vtf);
	vf.set(10.f,10.f);
	QVERIFY(vf/2.f==Vec2f(5.f,5.f));

	vd.set(0.,0.);
	QVERIFY(vd==Vec2d(0.,0.));
	QVERIFY(vd!=Vec2d(0.,1.));
	vd+=Vec2d(5.,5.);
	QVERIFY(vd==Vec2d(5.,5.));
	vd-=Vec2d(2.,2.);
	QVERIFY(vd==Vec2d(3.,3.));
	vd*=Vec2d(2.,2.);
	QVERIFY(vd==Vec2d(6.,6.));
	vd/=Vec2d(3.,3.);
	QVERIFY(vd==Vec2d(2.,2.));
	vd = Vec2d(1.,1.) + Vec2d(9.,9.);
	QVERIFY(vd==Vec2d(10.,10.));
	vd = Vec2d(5.,5.) - Vec2d(6.,6.);
	QVERIFY(vd==Vec2d(-1.,-1.));
	vd = Vec2d(10.,10.) * Vec2d(2.,1.);
	QVERIFY(vd==Vec2d(20.,10.));
	vd = Vec2d(10.,10.) / Vec2d(2.,5.);
	QVERIFY(vd==Vec2d(5.,2.));
	vd = Vec2d(5.);
	QVERIFY(vd.min(Vec2d(3.,3.))==Vec2d(3.,3.));
	QVERIFY(vd.max(Vec2d(3.,3.))==Vec2d(5.,5.));
	QVERIFY(vd.clamp(Vec2d(1.,1.),Vec2d(10.,10.))==Vec2d(5.,5.));
	QVERIFY(qAbs(vd.dot(Vec2d(5.,5.)) - 50.) <= ERROR_LIMIT);
	vd.set(2.,2.);
	QVERIFY(qAbs(vd.length() - 2.82843) <= ERROR_LIMIT);
	QVERIFY(qAbs(vd.lengthSquared() - 8.) <= ERROR_LIMIT);
	Vec2d vtd(1.,1.);
	QVERIFY(vtd==Vec2d(1.,1.));
	vd = vtd;
	QVERIFY(vd==vtd);
	vd = *vtd;
	QVERIFY(vd==vtd);
	vd.set(10.,10.);
	QVERIFY(vd/2.==Vec2d(5.,5.));
}

void TestVecMath::testVec3Math()
{
	Vec3i vi;
	Vec3f vf;
	Vec3d vd;

	vi.set(0,0,0);
	QVERIFY(vi==Vec3i(0,0,0));
	QVERIFY(vi!=Vec3i(0,1,0));
	vi+=Vec3i(5,5,5);
	QVERIFY(vi==Vec3i(5,5,5));
	vi-=Vec3i(2,2,2);
	QVERIFY(vi==Vec3i(3,3,3));
	vi*=2;
	QVERIFY(vi==Vec3i(6,6,6));
	vi/=3;
	QVERIFY(vi==Vec3i(2,2,2));
	vi = Vec3i(1,1,1) + Vec3i(9,9,9);
	QVERIFY(vi==Vec3i(10,10,10));
	vi = Vec3i(5,5,5) - Vec3i(6,6,6);
	QVERIFY(vi==Vec3i(-1,-1,-1));
	vi = Vec3i(5,5,5)^Vec3i(6,6,6);
	QVERIFY(vi==Vec3i(0,0,0));
	QVERIFY(Vec3i(10,10,10) * Vec3i(2,1,5)==80);
	vi.set(5,5,5);
	QVERIFY(vi.dot(Vec3i(5,5,1))==55);
	vi.set(2,2,2);
	QVERIFY(vi.length()==3);
	QVERIFY(vi.lengthSquared()==12);
	QVERIFY(vi.toString()==QString("[2, 2, 2]"));
	vi = Vec3i(1);
	QVERIFY(vi.toVec3f()==Vec3f(1.f));
	QVERIFY(vi.toVec3d()==Vec3d(1.));
	vi = Vec3i(10);
	QVERIFY(vi/2==Vec3i(5));
	Vec3i vt(1,1,1);
	QVERIFY(vt==Vec3i(1,1,1));
	vi = vt;
	QVERIFY(vi==vt);
	vi = *vt;
	QVERIFY(vi==vt);

	vf.set(0.f,0.f,0.f);
	QVERIFY(vf==Vec3f(0.f,0.f,0.f));
	QVERIFY(vf!=Vec3f(0.f,1.f,0.f));
	vf+=Vec3f(5.f,5.f,5.f);
	QVERIFY(vf==Vec3f(5.f,5.f,5.f));
	vf-=Vec3f(2.f,2.f,2.f);
	QVERIFY(vf==Vec3f(3.f,3.f,3.f));
	vf*=2.f;
	QVERIFY(vf==Vec3f(6.f,6.f,6.f));
	vf/=3.f;
	QVERIFY(vf==Vec3f(2.f,2.f,2.f));
	vf = Vec3f(1.f,1.f,1.f) + Vec3f(9.f,9.f,9.f);
	QVERIFY(vf==Vec3f(10.f,10.f,10.f));
	vf = Vec3f(5.f,5.f,5.f) - Vec3f(6.f,6.f,6.f);
	QVERIFY(vf==Vec3f(-1.f,-1.f,-1.f));
	vf = Vec3f(5.f,5.f,5.f)^Vec3f(6.f,6.f,6.f);
	QVERIFY(vf==Vec3f(0.f,0.f,0.f));
	QVERIFY(Vec3f(10.f,10.f,10.f) * Vec3f(2.f,1.f,5.f)==80.f);
	vf.set(5.f,5.f,5.f);
	QVERIFY(qAbs(vf.dot(Vec3f(5.f,5.f,1.f)) - 55.f) <= ERROR_LIMIT);
	vf.set(2.f,2.f,2.f);
	QVERIFY(qAbs(vf.length() - 3.4641016f) <= ERROR_LIMIT);
	QVERIFY(qAbs(vf.lengthSquared() - 12.f) <= ERROR_LIMIT);
	vf = Vec3f(10.f);
	QVERIFY(vf/2.f==Vec3f(5.f));
	vf.set(3.f,3.f,3.f);
	QVERIFY(qAbs(vf.latitude() - 0.6154797f) <= ERROR_LIMIT);
	QVERIFY(qAbs(vf.longitude() - 0.7853982f) <= ERROR_LIMIT);
	Vec3f vtf(1.f,1.f,1.f);
	QVERIFY(vtf==Vec3f(1.f,1.f,1.f));
	vf = vtf;
	QVERIFY(vf==vtf);
	vf = *vtf;
	QVERIFY(vf==vtf);

	vd.set(0.,0.,0.);
	QVERIFY(vd==Vec3d(0.,0.,0.));
	QVERIFY(vd!=Vec3d(0.,1.,0.));
	vd+=Vec3d(5.,5.,5.);
	QVERIFY(vd==Vec3d(5.,5.,5.));
	vd-=Vec3d(2.,2.,2.);
	QVERIFY(vd==Vec3d(3.,3.,3.));
	vd*=2.;
	QVERIFY(vd==Vec3d(6.,6.,6.));
	vd/=3.;
	QVERIFY(vd==Vec3d(2.,2.,2.));
	vd = Vec3d(1.,1.,1.) + Vec3d(9.,9.,9.);
	QVERIFY(vd==Vec3d(10.,10.,10.));
	vd = Vec3d(5.,5.,5.) - Vec3d(6.,6.,6.);
	QVERIFY(vd==Vec3d(-1.,-1.,-1.));
	vd = Vec3d(5.,5.,5.)^Vec3d(6.,6.,6.);
	QVERIFY(vd==Vec3d(0.,0.,0.));
	QVERIFY(Vec3d(10.,10.,10.) * Vec3d(2.,1.,5.)==80.);
	vd.set(5.,5.,5.);
	QVERIFY(qAbs(vd.dot(Vec3d(5.,5.,1.)) - 55.) <= ERROR_LIMIT);
	vd.set(2.,2.,2.);
	QVERIFY(qAbs(vd.length() - 3.4641016) <= ERROR_LIMIT);
	QVERIFY(qAbs(vd.lengthSquared() - 12.) <= ERROR_LIMIT);
	vd = Vec3d(10.);
	QVERIFY(vd/2.==Vec3d(5.));
	vd.set(3.,3.,3.);
	QVERIFY(qAbs(vd.latitude() - 0.6154797) <= ERROR_LIMIT);
	QVERIFY(qAbs(vd.longitude() - 0.7853982) <= ERROR_LIMIT);
	Vec3d vtd(1.,1.,1.);
	QVERIFY(vtd==Vec3d(1.,1.,1.));
	vd = vtd;
	QVERIFY(vd==vtd);
	vd = *vtd;
	QVERIFY(vd==vtd);
}

void TestVecMath::testVec4Math()
{
	Vec4i vi;
	Vec4f vf;
	Vec4d vd;

	vi.set(0,0,0,0);
	QVERIFY(vi==Vec4i(0,0,0,0));
	QVERIFY(vi!=Vec4i(0,1,0,0));
	vi = Vec3i(0,0,0);
	QVERIFY(vi==Vec4i(0,0,0,1));
	vi+=Vec4i(2,2,2,1);
	QVERIFY(vi==Vec4i(2,2,2,2));
	vi-=Vec4i(1,1,1,1);
	QVERIFY(vi==Vec4i(1,1,1,1));
	vi*=6;
	QVERIFY(vi==Vec4i(6,6,6,6));
	vi/=3;
	QVERIFY(vi==Vec4i(2,2,2,2));
	vi = Vec4i(1,1,1,1) + Vec4i(9,9,9,9);
	QVERIFY(vi==Vec4i(10,10,10,10));
	vi = Vec4i(5,5,5,5) - Vec4i(6,6,6,6);
	QVERIFY(vi==Vec4i(-1,-1,-1,-1));
	vi.set(5,5,5,5);
	QVERIFY(vi.dot(Vec4i(5,5,1,1))==60);
	vi.set(2,2,2,2);
	QVERIFY(vi.length()==4);
	QVERIFY(vi.lengthSquared()==16);
	QVERIFY(vi.toString()==QString("[2, 2, 2, 2]"));
	Vec4i vt(1,1,1,1);
	QVERIFY(vt==Vec4i(1,1,1,1));
	vi = vt;
	QVERIFY(vi==vt);
	vi = *vt;
	QVERIFY(vi==vt);
	vi = Vec4i(Vec3i(10,5,2));
	QVERIFY(vi==Vec4i(10,5,2,1));

	vf.set(0.f,0.f,0.f,0.f);
	QVERIFY(vf==Vec4f(0.f,0.f,0.f,0.f));
	QVERIFY(vf!=Vec4f(0.f,1.f,0.f,0.f));
	vf = Vec3f(0.f,0.f,0.f);
	QVERIFY(vf==Vec4f(0.f,0.f,0.f,1.f));
	vf+=Vec4f(2.f,2.f,2.f,1.f);
	QVERIFY(vf==Vec4f(2.f,2.f,2.f,2.f));
	vf-=Vec4f(1.f,1.f,1.f,1.f);
	QVERIFY(vf==Vec4f(1.f,1.f,1.f,1.f));
	vf*=6.f;
	QVERIFY(vf==Vec4f(6.f,6.f,6.f,6.f));
	vf/=3.f;
	QVERIFY(vf==Vec4f(2.f,2.f,2.f,2.f));
	vf = Vec4f(1.f,1.f,1.f,1.f) + Vec4f(9.f,9.f,9.f,9.f);
	QVERIFY(vf==Vec4f(10.f,10.f,10.f,10.f));
	vf = Vec4f(5.f,5.f,5.f,5.f) - Vec4f(6.f,6.f,6.f,6.f);
	QVERIFY(vf==Vec4f(-1.f,-1.f,-1.f,-1.f));
	vf.set(5.f,5.f,5.f,5.f);
	QVERIFY(qAbs(vf.dot(Vec4f(5.f,5.f,1.f,1.f))-60.f)<=ERROR_LIMIT);
	vf.set(2.f,2.f,2.f,2.f);
	QVERIFY(qAbs(vf.length()-4.f) <= ERROR_LIMIT);
	QVERIFY(qAbs(vf.lengthSquared()-16.f) <= ERROR_LIMIT);

	vd.set(0.,0.,0.,0.);
	QVERIFY(vd==Vec4d(0.,0.,0.,0.));
	QVERIFY(vd!=Vec4d(0.,1.,0.,0.));
	vd = Vec3d(0.,0.,0.);
	QVERIFY(vd==Vec4d(0.,0.,0.,1.));
	vd+=Vec4d(2.,2.,2.,1.);
	QVERIFY(vd==Vec4d(2.,2.,2.,2.));
	vd-=Vec4d(1.,1.,1.,1.);
	QVERIFY(vd==Vec4d(1.,1.,1.,1.));
	vd*=6.;
	QVERIFY(vd==Vec4d(6.,6.,6.,6.));
	vd/=3.;
	QVERIFY(vd==Vec4d(2.,2.,2.,2.));
	vd = Vec4d(1.,1.,1.,1.) + Vec4d(9.,9.,9.,9.);
	QVERIFY(vd==Vec4d(10.,10.,10.,10.));
	vd = Vec4d(5.,5.,5.,5.) - Vec4d(6.,6.,6.,6.);
	QVERIFY(vd==Vec4d(-1.,-1.,-1.,-1.));
	vd.set(5.,5.,5.,5.);
	QVERIFY(qAbs(vd.dot(Vec4d(5.,5.,1.,1.))-60.)<=ERROR_LIMIT);
	vd.set(2.,2.,2.,2.);
	QVERIFY(qAbs(vd.length()-4.) <= ERROR_LIMIT);
	QVERIFY(qAbs(vd.lengthSquared()-16.) <= ERROR_LIMIT);
}


void TestVecMath::testMatrix3Math()
{
	Mat3f mf;
	Mat3d md;

	mf.set(0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f);
	QVERIFY(qAbs(mf.trace() - 0.f) <= ERROR_LIMIT);
	mf = mf + Mat3f(1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f);
	QVERIFY(qAbs(mf.trace() - 3.f) <= ERROR_LIMIT);
	mf = mf - Mat3f(2.f,2.f,2.f,2.f,2.f,2.f,2.f,2.f,2.f);
	QVERIFY(qAbs(mf.trace() + 3.f) <= ERROR_LIMIT);
	mf = Mat3f(1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f)*Mat3f(5.f,5.f,5.f,5.f,5.f,5.f,5.f,5.f,5.f);
	QVERIFY(qAbs(mf.trace() - 45.f) <= ERROR_LIMIT);

	md.set(0.,0.,0.,0.,0.,0.,0.,0.,0.);
	QVERIFY(qAbs(md.trace() - 0.) <= ERROR_LIMIT);
	md = md + Mat3d(1.,1.,1.,1.,1.,1.,1.,1.,1.);
	QVERIFY(qAbs(md.trace() - 3.) <= ERROR_LIMIT);
	md = md - Mat3d(2.,2.,2.,2.,2.,2.,2.,2.,2.);
	QVERIFY(qAbs(md.trace() + 3.) <= ERROR_LIMIT);
	md = Mat3d(1.,1.,1.,1.,1.,1.,1.,1.,1.)*Mat3d(5.,5.,5.,5.,5.,5.,5.,5.,5.);
	QVERIFY(qAbs(md.trace() - 45.) <= ERROR_LIMIT);
}
