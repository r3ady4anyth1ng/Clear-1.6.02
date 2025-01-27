//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+
//
// cgfxAttrDef holds the "definition" of an attribute on a cgfxShader
// node.  This definition includes all the Maya attributes plus the
// CGeffect parameter index.
//

//

#include "cgfxEffectDef.h"
#include "cgfxShaderNode.h"

#ifdef _WIN32
#else
#	include <sys/timeb.h>
#	include <string.h>
#
#   define stricmp strcasecmp
#   define strnicmp strncasecmp
#endif



//
// A per-vertex attribute on a shader
//

cgfxVertexAttribute::cgfxVertexAttribute()
 : fNext( NULL), fSourceType( kUnknown), fSourceIndex( 0)
{
}


//
// A varying parameter to a pass
//

cgfxVaryingParameter::cgfxVaryingParameter( CGparameter parameter)
 :	fParameter( parameter),
	fVertexAttribute( NULL),
	fVertexStructure( NULL),
	fNext( NULL)
{
	if( parameter)
	{
		fName = cgGetParameterName( parameter);
//		fRegister = cgfxVaryingParameterManager::instance().findRegister( parameter);
	}
}


cgfxVaryingParameter::~cgfxVaryingParameter()
{
	if( fVertexStructure) delete fVertexStructure;
	if( fNext) delete fNext;
}


void cgfxVaryingParameter::setupAttributes( cgfxVertexAttribute*& vertexAttributes, CGprogram program)
{
	// Make sure our parameter name is acceptable is a Maya attribute name
	MString attrName = fName;
	int lastDot = attrName.rindex( '.');
	if( lastDot >= 0)
		attrName = attrName.substring( lastDot + 1, attrName.length() - 1);
	MString semantic = cgGetParameterSemantic( fParameter);
	semantic.toUpperCase();

	// Is this varying parameter packed or atomic?
	CGtype type = cgGetNamedUserType( program, attrName.asChar());
	if( type != CG_UNKNOWN_TYPE)
	{
		// It's packed: explode the inputs into the structure elements
		CGcontext context = cgGetProgramContext( program); 
		CGparameter packing = cgCreateParameter( context, type);
		fVertexStructure = new cgfxVaryingParameterStructure();
		fVertexStructure->fLength = 0;
		fVertexStructure->fSize = 0;
		CGparameter element = cgGetFirstStructParameter( packing);
		while( element)
		{
			MString elementName = cgGetParameterName( element);
			int lastDot = elementName.rindex( '.');
			if( lastDot >= 0)
				elementName = elementName.substring( lastDot + 1, elementName.length() - 1);
			cgfxVertexAttribute* attr = setupAttribute( elementName, semantic, element, vertexAttributes);
			fVertexStructure->fElements[ fVertexStructure->fLength].fVertexAttribute = attr;
			int size = cgGetParameterRows( element) * cgGetParameterColumns( element);
			CGtype type = cgGetParameterBaseType( element);
			if( type == CG_FLOAT) size *= sizeof( GLfloat);
			else if( type == CG_INT) size *= sizeof( GLint);
			fVertexStructure->fElements[ fVertexStructure->fLength].fSize = size;
			fVertexStructure->fLength++;
			fVertexStructure->fSize += size;
			element = cgGetNextParameter( element);
		}
		cgDestroyParameter( packing); 
	}
	else
	{
		// It's atomic - create a single, simple input
		fVertexAttribute = setupAttribute( attrName, semantic, fParameter, vertexAttributes);
	}

	// Now pull apart the semantic string to work out where to bind
	// this value in open GL (as the automagic binding through cgGL
	// didn't work so well when this was written)
	int radix = 1;
	fGLIndex = 0;
	unsigned int length = semantic.length();
	const char* str = semantic.asChar();
	for(;;)
	{
		char c = str[ length - 1];
		if( c < '0' || c > '9') break;
		fGLIndex += radix * (c - '0');
		radix *= 10;
		--length;
	}
	if( semantic.length() != length)
		semantic = semantic.substring( 0, length - 1);
	
	// Determine the semantic and setup the gl binding type we should use
	// to set this parameter. If there's a sensible default value, set that
	// while we're here.
	// Note there is no need to set the source type, this gets determined
	// when the vertex attribute sources are analysed
	if( semantic == "POSITION")
	{
		fGLType = glRegister::kPosition;
		fVertexAttribute->fSourceName = "position";
	}
	else if( semantic == "NORMAL")
	{
		fGLType = glRegister::kNormal;
		if( fVertexAttribute) 
			fVertexAttribute->fSourceName = "normal";
	}
	else if( semantic == "TEXCOORD")
	{
		fGLType = glRegister::kTexCoord;
		if( fVertexAttribute) 
		{
			if( attrName.toLowerCase() == "tangent")
				fVertexAttribute->fSourceName = "tangent:map1";
			else if( attrName.toLowerCase() == "binormal")
				fVertexAttribute->fSourceName = "binormal:map1";
			else
				fVertexAttribute->fSourceName = "uv:map1";
		}
	}
	else if( semantic == "TANGENT")
	{
		fGLType = glRegister::kTexCoord;
		fGLIndex += 6; // TANGENT is TEXCOORD6
		if( fVertexAttribute) 
			fVertexAttribute->fSourceName = "tangent:map1";
	}
	else if( semantic == "BINORMAL")
	{
		fGLType = glRegister::kTexCoord;
		fGLIndex += 7; // BINORMAL is TEXCOORD7
		if( fVertexAttribute) 
			fVertexAttribute->fSourceName = "binormal:map1";
	}
	else if( semantic == "COLOR")
	{
		fGLType = fGLIndex == 1 ? glRegister::kSecondaryColor : glRegister::kColor;
	}
	else if( semantic == "ATTR")
	{
		fGLType = glRegister::kVertexAttrib;
	}
	else if( semantic == "PSIZE")
	{
		fGLType = glRegister::kVertexAttrib;
		fGLIndex = 6;
	}
	else
	{
		fGLType = glRegister::kUnknown;
	}
}

cgfxVertexAttribute* cgfxVaryingParameter::setupAttribute( MString name, 
						const MString& semantic, 
						CGparameter parameter, 
						cgfxVertexAttribute*& vertexAttributes)
{
	// Does a varying parameter of this name already exist?
	cgfxVertexAttribute** attribute = &vertexAttributes;
	while( *attribute)
	{
		if( (*attribute)->fName == name)
		{
			return *attribute;
		}
		attribute = &(*attribute)->fNext;
	}

	// Add a new input for this parameter
	cgfxVertexAttribute* attr = new cgfxVertexAttribute();
	*attribute = attr;

	// Setup the varying parameter description
	attr->fName = name;
	attr->fType = cgGetTypeString( cgGetParameterType( parameter));
	attr->fSemantic = semantic;
	return attr;
}


void cgfxVaryingParameter::bind( const MDagPath& shape, cgfxStructureCache** cache,
					int vertexCount, const float * vertexArray,
					int normalsPerVertex, int normalCount, const float ** normalArrays,
					int colorCount, const float ** colorArrays,
					int texCoordCount, const float ** texCoordArrays)
{
	bool result = false;
	if( fVertexAttribute && fParameter)
	{
		switch( fVertexAttribute->fSourceType)
		{
			case cgfxVertexAttribute::kPosition:
				result = bind( vertexArray, 3);
				break;

			case cgfxVertexAttribute::kNormal:
				if( normalCount > 0 && normalArrays[ 0])
					result = bind( normalArrays[0], 3);
				break;

			case cgfxVertexAttribute::kUV:
				if( texCoordCount > fVertexAttribute->fSourceIndex && texCoordArrays[ fVertexAttribute->fSourceIndex])
					result = bind( texCoordArrays[ fVertexAttribute->fSourceIndex], 2);
				break;

			case cgfxVertexAttribute::kTangent:
				if( normalCount >= normalsPerVertex * fVertexAttribute->fSourceIndex + 1 && normalArrays[ normalsPerVertex * fVertexAttribute->fSourceIndex + 1])
					result = bind( normalArrays[ normalsPerVertex * fVertexAttribute->fSourceIndex + 1], 3);
				break;

			case cgfxVertexAttribute::kBinormal:
				if( normalCount >= normalsPerVertex * fVertexAttribute->fSourceIndex + 2 && normalArrays[ normalsPerVertex * fVertexAttribute->fSourceIndex + 2])
					result = bind( normalArrays[ normalsPerVertex * fVertexAttribute->fSourceIndex + 2], 3);
				break;

			case cgfxVertexAttribute::kColor:
				if( colorCount > fVertexAttribute->fSourceIndex && colorArrays[ fVertexAttribute->fSourceIndex])
					result = bind( colorArrays[ fVertexAttribute->fSourceIndex], 4);
				break;

			default:
				break;
		}
	}
	else if( fVertexStructure && fParameter && vertexCount)
	{
		// Build a unique name for the contents of this structure
		MString structureName;
		structureName += fVertexStructure->fSize;
		for( int i = 0; i < fVertexStructure->fLength; i++)
		{
				cgfxVertexAttribute* vertexAttribute = fVertexStructure->fElements[ i].fVertexAttribute;
				if( vertexAttribute) structureName += fVertexStructure->fElements[ i].fVertexAttribute->fSourceName;
				structureName += fVertexStructure->fElements[ i].fSize;
		}

		// See if this data already exists in the cache
		char* data = NULL;
		while( *cache)
		{
			if( !(*cache)->fShape.isValid() || !(*cache)->fShape.isAlive())
			{
				//printf( "Found stale cache data %s in the cache - deleting it\n", structureName.asChar());
				cgfxStructureCache* staleCache = *cache;
				*cache = staleCache->fNext;
				delete staleCache;
			}
			else 
			{
				if( (*cache)->fShape == shape.node() && (*cache)->fName == structureName)
				{
					//printf( "Found existing data in the cache for %s on %s\n", structureName.asChar(), shape.fullPathName().asChar());
					data = (*cache)->fData;
					break;
				}
				cache = &(*cache)->fNext;
			}
		}

		// If we couldn't find it, add it to the cache
		if( !data)
		{
			// Allocate storage for this structure
			//printf( "Added new cache entry for %s on %s\n", structureName.asChar(), shape.fullPathName().asChar());
			cgfxStructureCache* cacheEntry = new cgfxStructureCache( shape, structureName, fVertexStructure->fSize, vertexCount);
			cacheEntry->fNext = *cache;
			*cache = cacheEntry;
			data = cacheEntry->fData;
			char* dest = data;
			for( int i = 0; i < fVertexStructure->fLength; i++)
			{
				cgfxVertexAttribute* vertexAttribute = fVertexStructure->fElements[ i].fVertexAttribute;
				if( vertexAttribute)
				{
					const char* src = NULL;
					int size = 0;
					switch( vertexAttribute->fSourceType)
					{
						case cgfxVertexAttribute::kPosition:
							src = (const char*)vertexArray;
							size = 3 * sizeof( float);
							break;

						case cgfxVertexAttribute::kNormal:
							if( normalCount > 0 && normalArrays[ 0])
							{
								src = (const char*)normalArrays[0];
								size = 3 * sizeof( float);
							}
							break;

						case cgfxVertexAttribute::kUV:
							if( texCoordCount > vertexAttribute->fSourceIndex && texCoordArrays[ vertexAttribute->fSourceIndex])
							{
								src = (const char*)texCoordArrays[ vertexAttribute->fSourceIndex];
								size = 2 * sizeof( float);
							}
							break;

						case cgfxVertexAttribute::kTangent:
							if( normalCount >= normalsPerVertex * vertexAttribute->fSourceIndex + 1 && normalArrays[ normalsPerVertex * vertexAttribute->fSourceIndex + 1])
							{
								src = (const char*)normalArrays[ normalsPerVertex * vertexAttribute->fSourceIndex + 1];
								size = 3 * sizeof( float);
							}
							break;

						case cgfxVertexAttribute::kBinormal:
							if( normalCount >= normalsPerVertex * vertexAttribute->fSourceIndex + 2 && normalArrays[ normalsPerVertex * vertexAttribute->fSourceIndex + 2])
							{
								src = (const char*)normalArrays[ normalsPerVertex * vertexAttribute->fSourceIndex + 2];
								size = 3 * sizeof( float);
							}
							break;

						case cgfxVertexAttribute::kColor:
							if( colorCount > vertexAttribute->fSourceIndex && colorArrays[ vertexAttribute->fSourceIndex])
							{
								src = (const char*)colorArrays[ vertexAttribute->fSourceIndex];
								size = 4 * sizeof( float);
							}
							break;

						default:
							break;
					}

					// Do we have a valid input?
					if( src && size)
					{
						// Setup this element
						int srcSkip = 0;
						if( size > fVertexStructure->fElements[ i].fSize)
						{
							srcSkip = size - fVertexStructure->fElements[ i].fSize;
							size = fVertexStructure->fElements[ i].fSize;
						}
						int dstSkip = fVertexStructure->fSize - size;
						char* dst = dest;
						for( int v = 0; v < vertexCount; v++)
						{
							for( int b = 0; b < size; b++)
								*dst++ = *src++;
							src += srcSkip;
							dst += dstSkip;
						}
					}
					else
					{
						// NULL this element
						size = fVertexStructure->fElements[ i].fSize;
						int dstSkip = fVertexStructure->fSize - size;
						char* dst = dest;
						for( int v = 0; v < vertexCount; v++)
						{
							for( int b = 0; b < size; b++)
								*dst++ = 0;
							dst += dstSkip;
						}
					}
				}
				dest += fVertexStructure->fElements[ i].fSize;
			}
		}

		result = bind( (const float*)data, fVertexStructure->fSize / sizeof( float));
	}

	// If we were unable to bind a stream of data to this register, set a friendly NULL value
	if( !result)
		null();
}


//
// Bind data to GL
//
bool cgfxVaryingParameter::bind( const float* data, int stride)
{
	bool result = false;
	switch( fGLType)
	{
		case glRegister::kPosition:
			glStateCache::instance().enablePosition();
			glVertexPointer( stride, GL_FLOAT, 0, data);
			result = true;
			break;

		case glRegister::kNormal:
			if( stride == 3)
			{
				glStateCache::instance().enableNormal();
				glNormalPointer( GL_FLOAT, 0, data);
				result = true;
			}
			break;

		case glRegister::kTexCoord:
			if( fGLIndex < glStateCache::sMaxTextureUnits)
			{
				glStateCache::instance().enableAndActivateTexCoord( fGLIndex);
				glTexCoordPointer( stride, GL_FLOAT, 0, data);
				result = true;
			}
			break;

		case glRegister::kColor:
			if( stride > 2)
			{
				glStateCache::instance().enableColor();
				glColorPointer( stride, GL_FLOAT, 0, data);
				result = true;
			}
			break;

		case glRegister::kSecondaryColor:
			if( stride > 2)
			{
				glStateCache::instance().enableSecondaryColor();
				if( glStateCache::glVertexAttribPointer) 
					glStateCache::glSecondaryColorPointer( stride, GL_FLOAT, 0, (GLvoid*)data);
				result = true;
			}
			break;

		case glRegister::kVertexAttrib:
			glStateCache::instance().enableVertexAttrib( fGLIndex);
			if( glStateCache::glVertexAttribPointer) 
				glStateCache::glVertexAttribPointer( fGLIndex, stride, GL_FLOAT, GL_FALSE, 0, data);
			result = true;
			break;

		/// TODO add secondaryColor, vertexWeight, vertexAttrib, fog
		default:
			break;
	}
	return result;
}


//
// Send null data to GL
//
void cgfxVaryingParameter::null()
{
	switch( fGLType)
	{
		case glRegister::kPosition:
			glVertex4f( 0.0f, 0.0f, 0.0f, 1.0f);
			break;

		case glRegister::kNormal:
			glNormal3f( 0.0f, 0.0f, 1.0f);
			break;

		case glRegister::kTexCoord:
			glStateCache::instance().activeTexture( fGLIndex);
			glStateCache::glMultiTexCoord4fARB( GL_TEXTURE0 + fGLIndex, 0.0f, 0.0f, 0.0f, 0.0f );
			break;

		case glRegister::kColor:
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f);
			break;

		case glRegister::kSecondaryColor:
			if( glStateCache::glSecondaryColor3f) 
				glStateCache::glSecondaryColor3f( 1.0f, 1.0f, 1.0f);
			break;

		case glRegister::kVertexAttrib:
			if( glStateCache::glVertexAttrib4f) 
				glStateCache::glVertexAttrib4f( fGLIndex, 0.0f, 0.0f, 0.0f, 0.0f);
			break;

		/// TODO add secondaryColor, vertexWeight, vertexAttrib, fog
		default:
			break;
	}
}



inline void addVaryingParametersRecursive( CGparameter parameter, cgfxVaryingParameter**& nextParameter)
{
	if( cgGetParameterVariability( parameter) == CG_VARYING)
	{
		if( cgGetParameterType( parameter) == CG_STRUCT)
		{
			CGparameter input = cgGetFirstStructParameter( parameter);
			while( input)
			{
				addVaryingParametersRecursive( input, nextParameter);
				input = cgGetNextParameter( input);
			}
		}
		else if( cgIsParameterReferenced( parameter))
		{
			*nextParameter = new cgfxVaryingParameter( parameter);
			nextParameter = &(*nextParameter)->fNext;
		}
	}
}


//
// A pass in a technique
//

cgfxPass::cgfxPass( CGpass pass, cgfxEffect* effect)
 :	fPass( pass),
	fEffect( effect),
	fProgram( NULL),
	fParameters( NULL),
	fNext( NULL)
{
	if( pass)
	{
		fName = cgGetPassName( pass);
		CGstateassignment stateAssignment = cgGetFirstStateAssignment( pass);
		cgfxVaryingParameter** nextParameter = &fParameters;
		while( stateAssignment )
		{
			CGstate state = cgGetStateAssignmentState( stateAssignment);
			if( cgGetStateType( state) == CG_PROGRAM_TYPE && 
					( stricmp( cgGetStateName( state), "vertexProgram") == 0 ||
					  stricmp( cgGetStateName( state), "vertexShader") == 0))
			{
				fProgram = cgGetProgramStateAssignmentValue( stateAssignment);
				if( fProgram)
				{
					CGparameter parameter = cgGetFirstParameter( fProgram, CG_PROGRAM);
					while( parameter)
					{
						addVaryingParametersRecursive( parameter, nextParameter);
						parameter = cgGetNextParameter( parameter);
					}
				}
			}
			stateAssignment = cgGetNextStateAssignment( stateAssignment);
		}
	}
}


cgfxPass::~cgfxPass()
{
	if( fNext) delete fNext;
	if( fParameters) delete fParameters;
}


void cgfxPass::setupAttributes( cgfxVertexAttribute*& vertexAttributes)
{
	cgfxVaryingParameter* parameter = fParameters;
	while( parameter)
	{
		parameter->setupAttributes( vertexAttributes, fProgram);
		parameter = parameter->fNext;
	}
}


void cgfxPass::bind( const MDagPath& shape, int vertexCount, const float * vertexArray,
					int normalsPerVertex, int normalCount, const float ** normalArrays,
					int colorCount, const float ** colorArrays,
					int texCoordCount, const float ** texCoordArrays)
{
	cgfxVaryingParameter* parameter = fParameters;
	while( parameter)
	{
		parameter->bind( shape, &fEffect->fCache, 
						vertexCount, vertexArray, 
						 normalsPerVertex, normalCount, normalArrays, 
						 colorCount, colorArrays, 
						 texCoordCount, texCoordArrays);
		parameter = parameter->fNext;
	}
}



//
// A technique in an effect
//

cgfxTechnique::cgfxTechnique( CGtechnique technique, cgfxEffect* effect)
 :	fTechnique( technique),
	fEffect( effect),
	fPasses( NULL),
	fNext( NULL)
{
	if( technique)
	{
		fName = cgGetTechniqueName( technique);
		CGpass pass = cgGetFirstPass( technique);
		cgfxPass** nextPass = &fPasses;
		while( pass)
		{
			// If there's no state assigned in a pass, it's either a no-op, or
			// it failed to compile (i.e. no vertex program). Either way, skip it
			if( cgGetFirstStateAssignment( pass))
			{
				*nextPass = new cgfxPass( pass, fEffect);
				nextPass = &(*nextPass)->fNext;
			}
			pass = cgGetNextPass( pass);
		}
	}
	fMultiPass = fPasses && fPasses->fNext;
}


cgfxTechnique::~cgfxTechnique()
{
	if( fNext) delete fNext;
	if( fPasses) delete fPasses;
}


void cgfxTechnique::setupAttributes( cgfxVertexAttribute*& vertexAttributes)
{
	cgfxPass* pass = fPasses;
	while( pass)
	{
		pass->setupAttributes( vertexAttributes);
		pass = pass->fNext;
	}
}



//
// An effect
//

cgfxEffect::cgfxEffect()
:	fEffect( NULL),
	fTechniques( NULL),
	fTechnique( NULL),
	fCache( NULL)
{
}


cgfxEffect::~cgfxEffect()
{
	setEffect( NULL);
}


void cgfxEffect::setEffect( CGeffect effect)
{
	if( effect != fEffect)
	{
//		if( fEffect) delete fEffect;
		if( fTechniques) delete fTechniques;
		fTechniques = NULL;
		fTechnique = NULL;
		flushCache();
		fEffect = effect;
		if( fEffect)
		{
			CGtechnique technique = cgGetFirstTechnique( effect);
			cgfxTechnique** nextTechnique = &fTechniques;
			while( technique)
			{
				*nextTechnique = new cgfxTechnique( technique, this);
				nextTechnique = &(*nextTechnique)->fNext;
				technique = cgGetNextTechnique( technique);
			}
		}
	}
}


void cgfxEffect::setupAttributes( cgfxShaderNode* shader)
{
	cgfxVertexAttribute* vertexAttributes = NULL;
	cgfxTechnique* technique = fTechniques;
	MString currentTechnique = shader->getTechnique();
	while( technique)
	{
		if( technique->fName == currentTechnique)
		{
			fTechnique = technique;
			technique->setupAttributes( vertexAttributes);
			break;
		}
		technique = technique->fNext;
	}
	shader->setVertexAttributes( vertexAttributes);

	// We've changed technique, so flush any cached data we had as the
	// structure definitions may have changed
	flushCache();
}


void cgfxEffect::flushCache()
{
	//printf( "Flushing all cached data\n");
	if( fCache) 
	{
		delete fCache;
		fCache = NULL;
	}
}


void cgfxEffect::flushCache( const MDagPath& shape)
{
	//printf( "Flushing cached data for shape %s\n", shape.fullPathName().asChar());
	cgfxStructureCache** cacheEntry = &fCache;
	while( *cacheEntry)
	{
		if( !(*cacheEntry)->fShape.isValid() || !(*cacheEntry)->fShape.isAlive() || (*cacheEntry)->fShape == shape.node())
		{
			//printf( "Found stale cache data %s in the cache - deleting it\n", (*cacheEntry)->fName.asChar());
			cgfxStructureCache* staleEntry = (*cacheEntry);
			*cacheEntry = staleEntry->fNext;
			staleEntry->fNext = NULL;
			delete staleEntry;
		}
		else
		{
			cacheEntry = &(*cacheEntry)->fNext;
		}
	}
}


cgfxStructureCache::cgfxStructureCache( const MDagPath& shape, const MString& name, int stride, int count)
: fShape( shape.node()), fName( name), fElementSize( stride), fVertexCount( count), fData( new char[ stride * count])
{
}


cgfxStructureCache::~cgfxStructureCache()
{
	if( fData) delete[] fData;
}
