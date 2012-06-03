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

#ifndef _STELTEXTURE_HPP_
#define _STELTEXTURE_HPP_

#include <QObject>
#include <QImage>
#include <QtOpenGL>

#include "StelRenderer.hpp"
#include "StelTextureBackend.hpp"
#include "StelTextureParams.hpp"
#include "StelTextureTypes.hpp"

//! @class StelTexture
//! Texture class. For creating an instance, use StelTextureMgr::createTexture() and StelTextureMgr::createTextureThread()
//! @sa StelTextureSP
class StelTexture : public QObject
{
	Q_OBJECT

public:
	//! Destructor
	~StelTexture();

	//! Bind the texture so that it can be used for drawing.
	//!
	//! If the texture is lazily loaded and has not been loaded before,
	//! it will start loading, it will not be bound and false will be returned.
	//! @param textureUnit Texture unit to use (default is 0)
	//! @return true if the binding successfully occured, false if the texture is not yet loaded.
	
	//minor change by Eleni Maria Stea:
	//added texture unit (useful when multiple textures are used)
	//this change doesn't affect the previous calls of the function!
	bool bind(int textureUnit = 0)
	{
		if(textureBackend->getStatus() != TextureStatus_Loaded){return false;}
		renderer->bindTexture(textureBackend, textureUnit);
		return true;
	}

	//! Return whether the texture can be bound, i.e. it is fully loaded
	bool canBind() const {return textureBackend->getStatus() == TextureStatus_Loaded;}

	//! Return the width and heigth of the texture in pixels
	bool getDimensions(int &width, int &height)
	{
		if(textureBackend->getStatus() != TextureStatus_Loaded){return false;}

		const QSize dimensions = textureBackend->getDimensions();
		width  = dimensions.width();
		height = dimensions.height();

		return true;
	}

	//! Get the error message which caused the texture loading to fail
	//! @return human friendly error message or empty string if no errors occured
	const QString& getErrorMessage() const {return textureBackend->getErrorMessage();}

	//! Return whether the image is currently being loaded
	bool isLoading() const {return textureBackend->getStatus() == TextureStatus_Loading;}

private:
	//! Texture backend at Renderer side.
	StelTextureBackend* textureBackend;

	//! Renderer that constructed the texture backend.
	StelRenderer* renderer;

	//! Constructs a StelTextyre.
	friend class StelTextureMgr;

	//! Private constructor (so only StelTextureMgr can construct this).
	StelTexture(StelTextureBackend* backend, StelRenderer* renderer);
};


#endif // _STELTEXTURE_HPP_
