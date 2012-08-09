
#include "StelQGL1Renderer.hpp"
#include "StelQGL1ArrayVertexBufferBackend.hpp"


StelQGL1ArrayVertexBufferBackend::
StelQGL1ArrayVertexBufferBackend(const PrimitiveType type,
                                 const QVector<StelVertexAttribute>& attributes)
	: StelQGLArrayVertexBufferBackend(type, attributes)
{
}

//! Helper function that enables a vertex attribute and provides attribute data to GL.
//!
//! @param attribEnum GL enumeration corresponding to the attribute interpretation will be
//!                   written here.
//! @param attribute  Defines the attribute to enable.
//! @param data       Attribute data (e.g. positions, texcoords, normals, etc.)
void enableAttribute(GLenum& attribEnum, const StelVertexAttribute& attribute, 
                     const void* data)
{
	attribEnum = gl1AttributeEnum(attribute.interpretation);
	glEnableClientState(attribEnum);
	switch(attribute.interpretation)
	{
		case AttributeInterpretation_Position:
			glVertexPointer(attributeDimensions(attribute.type),
			                glAttributeType(attribute.type), 0, data);
			break;
		case AttributeInterpretation_TexCoord:
			glTexCoordPointer(attributeDimensions(attribute.type),
			                  glAttributeType(attribute.type), 0, data);
			break;
		case AttributeInterpretation_Color:
			glColorPointer(attributeDimensions(attribute.type),
			               glAttributeType(attribute.type), 0, data);
			break;
		case AttributeInterpretation_Normal:
			glNormalPointer(glAttributeType(attribute.type), 0, data);
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown attribute interpretation");
	}
}

void StelQGL1ArrayVertexBufferBackend::
	draw(StelQGL1Renderer& renderer, const Mat4f& projectionMatrix,
	     StelQGLIndexBuffer* indexBuffer)
{
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to draw a vertex buffer that is not locked.");

	GLenum enabledAttributes [MAX_VERTEX_ATTRIBUTES];

	bool usingVertexColors = false;
	bool usingTexturing = false;
	// Provide all vertex attributes' arrays to GL.
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		Q_ASSERT_X(attrib < MAX_VERTEX_ATTRIBUTES, Q_FUNC_INFO,
		           "enabledAttributes array is too small to handle all vertex attributes.");

		const StelVertexAttribute& attribute(attributes.attributes[attrib]);
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			usingVertexColors = true;
		}

		if(attribute.interpretation == AttributeInterpretation_TexCoord)
		{
			glEnable(GL_TEXTURE_2D);
			usingTexturing = true;
		}
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			usingVertexColors = true;
		}
		if(attribute.interpretation == AttributeInterpretation_Position &&
		   usingProjectedPositions)
		{
			// Using projected positions, use projectedPositions vertex array.
			enableAttribute(enabledAttributes[attrib], 
			                attribute, projectedPositions.constData());

			// Projected positions are used within a single renderer drawVertexBufferBackend
			// call - we set this so any further calls with this buffer won't accidentally 
			// use projected data from before (we don't destroy the buffer so we can 
			// reuse it).
			usingProjectedPositions = false;
			continue;
		}

		// Not a position attribute, or not using projected positions, 
		// so use the normal vertex array.
		enableAttribute(enabledAttributes[attrib], attribute, 
		                buffers[attrib]->constData());
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Set the real openGL projection and modelview matrix to 2d orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMultMatrixf(projectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!usingVertexColors)
	{
		const Vec4f& color = renderer.getGlobalColor();
		glColor4f(color[0], color[1], color[2], color[3]);
	}

	// Draw the vertices.
	if(NULL != indexBuffer)
	{
		glDrawElements(glPrimitiveType(primitiveType), indexBuffer->length(),
		               glIndexType(indexBuffer->indexType()), indexBuffer->indices());
	}
	else
	{
		glDrawArrays(glPrimitiveType(primitiveType), 0, vertexCount);
	}

	for(int attribute = 0; attribute < attributes.count; attribute++) 
	{
		glDisableClientState(enabledAttributes[attribute]);
	}

	if(usingTexturing)
	{
		glDisable(GL_TEXTURE_2D);
	}
}

