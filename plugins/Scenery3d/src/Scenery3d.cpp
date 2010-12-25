/*
 * TestPlugin
 * 
 * Copyright (C) 2010 Simon Parzer, Gustav Oberwandling, Peter Neubauer
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

#include "Scenery3d.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"
#include "Scenery3dMgr.hpp"

#include <QAction>
#include <QString>
#include <QDebug>
#include <QSettings>
#include <stdexcept>
#include <cmath>


Scenery3d::Scenery3d()
    :rotation(0.0f),
    objModel(NULL),
    vertices(NULL), verticesP(NULL),
    texcoords(NULL), normals(NULL)
{
    objModel = new OBJ();
}

Scenery3d::~Scenery3d()
{
    delete objModel;
}

void Scenery3d::load(const QSettings& scenery3dIni, const QString& scenery3dID)
{
    id = scenery3dID;
    name = scenery3dIni.value("model/name").toString();
    authorName = scenery3dIni.value("model/author").toString();
    description = scenery3dIni.value("model/description").toString();
    landscapeName = scenery3dIni.value("model/landscape").toString();
    modelSceneryFile = scenery3dIni.value("model/scenery").toString();
    modelGroundFile = scenery3dIni.value("model/ground").toString();

    QString modelFile = StelFileMgr::findFile(Scenery3dMgr::MODULE_PATH + id + "/" + modelSceneryFile);
    qDebug() << "Trying to load OBJ model: " << modelFile;
    //objModel->load(modelFile.toAscii());
}

void Scenery3d::update(double deltaTime)
{
    rotation += 8.0f * deltaTime;
}

void Scenery3d::drawCubeTestScene(StelCore* core)
{
    const Vec3d cube_vertice[] = {
        Vec3d( 1.0, 1.0, 1.0),
        Vec3d(-1.0, 1.0, 1.0),
        Vec3d( 1.0,-1.0, 1.0),
        Vec3d(-1.0,-1.0, 1.0),

        Vec3d(-1.0,-1.0,-1.0),

        Vec3d(-1.0, 1.0, 1.0),
        Vec3d(-1.0, 1.0,-1.0),

        Vec3d( 1.0, 1.0, 1.0),
        Vec3d( 1.0, 1.0,-1.0),

        Vec3d( 1.0,-1.0, 1.0),
        Vec3d( 1.0,-1.0,-1.0),

        Vec3d(-1.0,-1.0,-1.0),

        Vec3d( 1.0, 1.0,-1.0),
        Vec3d(-1.0, 1.0,-1.0),
    };

    const Vec3f cube_colors[] = {
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f),

        Vec3f(1.0f, 0.5f, 0.0f),

        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f),

        Vec3f(1.0f, 1.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 0.0f),

        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(0.0f, 0.0f, 1.0f),

        Vec3f(1.0f, 0.5f, 0.0f),

        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
    };

    Vec3d cube_vertice_triangles[36];
    Vec3f cube_colors_triangles[36];

    for (int i=2; i<14; i++) {
        int idx = i-2;
        int tri = idx*3;
        if ((i%2) == 0) {
            cube_vertice_triangles[tri] = cube_vertice[idx];
            cube_vertice_triangles[tri+1] = cube_vertice[idx+1];
            cube_vertice_triangles[tri+2] = cube_vertice[idx+2];
        } else {
            cube_vertice_triangles[tri] = cube_vertice[idx+1];
            cube_vertice_triangles[tri+1] = cube_vertice[idx];
            cube_vertice_triangles[tri+2] = cube_vertice[idx+2];
        }
        cube_colors_triangles[tri] = cube_colors_triangles[tri+1] =
                cube_colors_triangles[tri+2] = cube_colors[i];
    }

    const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, core->getCurrentProjectionType());
    StelPainter painter(prj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glClear(GL_DEPTH_BUFFER_BIT);

    Vec3d* vertice = new Vec3d[36 * 100];
    Vec3f* colors = new Vec3f[36 * 100];

    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            double xPos = 15.0 + x * 3.0;
            double yPos = 15.0 + y * 3.0;
            for (int i = 0; i < 36; i++) {
                int idx = (x*10 + y) * 36 + i;
                Vec3d cv = cube_vertice_triangles[i];
                vertice[idx][0] = cv[0] + xPos;
                vertice[idx][1] = cv[1] + yPos;
                vertice[idx][2] = cv[2];
                colors[idx] = cube_colors_triangles[i];
            }
        }
    }

    //Vec3d nullv(0, 0, 0);
    //float fov = prj->getFov();
    float fov = prj->getFov();
    float aspect = (float)prj->getViewportWidth() / (float)prj->getViewportHeight();
    float zNear = 1.0f;
    float zFar = 10000.0f;
    float f = 2.0 / tan(fov * M_PI / 360.0);
    Mat4d projMatd(f / aspect, 0, 0, 0,
                   0, f, 0, 0,
                   0, 0, (zFar + zNear) / (zNear - zFar), 2.0 * zFar * zNear / (zNear - zFar),
                   0, 0, -1, 0);
    /*Mat4d mvpMat = projMatd * prj->getModelViewMatrix();
    const int num_triangles = 100*36 / 3;
    for (int i=0; i<num_triangles; i++) {
        int tri = i*3;
        bool valid = true;
        valid &= prj->projectInPlace(vertice[tri]);
        valid &= prj->projectInPlace(vertice[tri+1]);
        valid &= prj->projectInPlace(vertice[tri+2]);
        if (!valid) {
            vertice[tri] = vertice[tri+1] = vertice[tri+2] = nullv;
        }
        vertice[tri].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri]);
        vertice[tri+1].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri+1]);
        vertice[tri+2].transfo4d(mvpMat);
        prj->viewportTransform(vertice[tri+2]);
        //qDebug() << vertice[tri].toString();
    }*/


    /*Mat4f glViewMatrix;
    Mat4f glProjectionMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, glViewMatrix.r);
    glGetFloatv(GL_PROJECTION_MATRIX, glProjectionMatrix.r);
    qDebug() << "view";
    glViewMatrix.print();
    qDebug() << "projection";
    glProjectionMatrix.print();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();*/

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(projMatd);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(prj->getModelViewMatrix());

    painter.setArrays(vertice, NULL, colors);
    for (int i = 0; i < 100; i++) {
        painter.drawFromArray(StelPainter::Triangles, 36, i * 36, false);
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    /*painter.setColor(1.f, 1.f, 1.f);
    QVector<Vec3d> pv;
    for (int i = 0; i < 100; i++) {
        pv.clear();
        for (int j = 0; j < 14; j++) {
            pv << vVertice[i*14 + j];
        }
        StelVertexArray va(pv, StelVertexArray::TriangleStrip);
        painter.drawSphericalTriangles(va, false, NULL, true, 1000.);
    }*/

    delete[] vertice;
    delete[] colors;

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
	
void Scenery3d::draw(StelCore* core)
{
    // for debug purposes
    drawCubeTestScene(core);
}
