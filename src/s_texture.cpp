/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
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

#include "s_texture.h"
#include "stdlib.h"
#include "glpng.h"

char s_texture::texDir[255] = "./";
char s_texture::suffix[10] = "";

s_texture::s_texture(char * _textureName) : loadType(PNG_BLEND3)
{
    if (!_textureName) exit(-1);
    texID=0;
    textureName=strdup(_textureName);
    load();
}

s_texture::s_texture(char * _textureName, int _loadType)
{
    if (!_textureName) exit(-1);
	loadType2=GL_CLAMP;
    switch (_loadType)
    {
        case TEX_LOAD_TYPE_PNG_ALPHA : loadType=PNG_ALPHA; break;
        case TEX_LOAD_TYPE_PNG_SOLID : loadType=PNG_SOLID; break;
        case TEX_LOAD_TYPE_PNG_BLEND3: loadType=PNG_BLEND1; break;
        case TEX_LOAD_TYPE_PNG_REPEAT: loadType=PNG_BLEND3; loadType2=GL_REPEAT; break;
        default : loadType=PNG_BLEND3;
    }
    texID=0;
    textureName=strdup(_textureName);
    load();
}

s_texture::~s_texture()
{   
    unload();
    if (textureName) free(textureName);
}

int s_texture::load()
{
    char * fullName = (char*)malloc( sizeof(char) * ( strlen("./"/*texDir*/) + strlen(textureName) + strlen(".png"/*suffix*/) + 1 ) );
    sprintf(fullName,"%s%s%s","./"/*texDir*/,textureName,".png"/*suffix*/);
    FILE * tempFile = fopen(fullName,"r");
    if (!tempFile) printf("WARNING : Can't load texture %s!\n",fullName);
    pngInfo info;
    pngSetStandardOrientation(1);
    texID = pngBind(fullName, PNG_BUILDMIPMAPS, loadType, &info, loadType2, GL_LINEAR, GL_LINEAR);
    return (texID!=0);
}

void s_texture::unload()
{   
    glDeleteTextures(1, (GLuint*)&texID);						// Delete The Texture
}

int s_texture::reload()
{	
    unload();
    return load();
}
