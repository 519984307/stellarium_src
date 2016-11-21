/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _STELTEXTUREMGR_HPP_
#define _STELTEXTUREMGR_HPP_

#include "StelTexture.hpp"
#include <QObject>

class QNetworkReply;
class QThread;


//! @class StelTextureMgr
//! Manage textures loading.
//! It provides method for loading images in a separate thread.
class StelTextureMgr : QObject
{
public:
	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked for in Stellarium's standard textures directories.
	//! @param params the texture creation parameters.
	StelTextureSP createTexture(const QString& filename, const StelTexture::StelTextureParams& params=StelTexture::StelTextureParams());

	//! Load an image from a file and create a new texture from it in a new thread.
	//! @param url the texture file name or URL, can be absolute path if starts with '/' otherwise
	//!    the file will be looked for in Stellarium's standard textures directories.
	//! @param params the texture creation parameters.
	//! @param lazyLoading define whether the texture should be actually loaded only when needed, i.e. when bind() is called the first time.
	StelTextureSP createTextureThread(const QString& url, const StelTexture::StelTextureParams& params=StelTexture::StelTextureParams(), bool lazyLoading=true);

	//! Returns the estimated memory usage of all textures currently loaded through StelTexture
	int getGLMemoryUsage();

private:
	friend class StelTexture;
	friend class ImageLoader;
	friend class StelApp;

	//! Private constructor, use StelApp::getTextureManager for the correct instance
	StelTextureMgr();

	//! Initialize some variable from the openGL context.
	//! Must be called after the creation of the GLContext.
	void init();

	int glMemoryUsage;
};


#endif // _STELTEXTUREMGR_HPP_
