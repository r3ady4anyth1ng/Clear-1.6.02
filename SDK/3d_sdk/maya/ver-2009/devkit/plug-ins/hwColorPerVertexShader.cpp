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

///////////////////////////////////////////////////////////////////
//
// NOTE: PLEASE READ THE README.TXT FILE FOR INSTRUCTIONS ON
// COMPILING AND USAGE REQUIREMENTS.
//
// DESCRIPTION: 
//		This is an example of drawing an object using
//		the stored color per vertex, or using false
//		coloring of either : the nornmals, the tangents,
//		or the binormals.
//
///////////////////////////////////////////////////////////////////

#ifdef WIN32
#pragma warning( disable : 4786 )		// Disable STL warnings.
#endif

#include <maya/MIOStream.h>
#include <math.h>

#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MStringArray.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxHwShaderNode.h>
#include <maya/MStringArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>

// For swatch rendering
#include <maya/MHardwareRenderer.h>
#include <maya/MGeometryData.h>


#include <maya/MHWShaderSwatchGenerator.h>

#if defined(OSMac_MachO_)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


class hwColorPerVertexShader : public MPxHwShaderNode
{
	public:
                    hwColorPerVertexShader();
    virtual         ~hwColorPerVertexShader();

    virtual MStatus compute( const MPlug&, MDataBlock& );
	virtual void    postConstructor();

	// Internally cached attribute handling routines
	virtual bool getInternalValueInContext( const MPlug&,
											  MDataHandle&,
											  MDGContext&);
    virtual bool setInternalValueInContext( const MPlug&,
											  const MDataHandle&,
											  MDGContext&);

	virtual MStatus		bind( const MDrawRequest& request,
							  M3dView& view );
	virtual MStatus		unbind( const MDrawRequest& request,
								M3dView& view );
	virtual MStatus		geometry( const MDrawRequest& request,
								M3dView& view,
								int prim,
								unsigned int writable,
								int indexCount,
								const unsigned int * indexArray,
								int vertexCount,
								const int * vertexIDs,
								const float * vertexArray,
								int normalCount,
								const float ** normalArrays,
								int colorCount,
								const float ** colorArrays,
								int texCoordCount,
								const float ** texCoordArrays);

	// Batch overrides
	virtual MStatus	glBind(const MDagPath& shapePath);
	virtual MStatus	glUnbind(const MDagPath& shapePath);
	virtual MStatus	glGeometry( const MDagPath&,
                              int prim,
							  unsigned int writable,
							  int indexCount,
							  const unsigned int * indexArray,
							  int vertexCount,
							  const int * vertexIDs,
							  const float * vertexArray,
							  int normalCount,
							  const float ** normalArrays,
							  int colorCount,
							  const float ** colorArrays,
							  int texCoordCount,
							  const float ** texCoordArrays);

	// Overridden to draw an image for swatch rendering.
	///
	virtual MStatus renderSwatchImage( MImage & image );
	void			drawTheSwatch( MGeometryData* pGeomData,
								   unsigned int* pIndexing,
								   unsigned int  numberOfData,
								   unsigned int  indexCount );
	MStatus	draw( int prim,
					unsigned int writable,
					int indexCount,
					const unsigned int * indexArray,
					int vertexCount,
					const float * vertexArray,
					int colorCount,
					const float ** colorArrays);


	virtual int		colorsPerVertex();
	virtual bool	hasTransparency();
	virtual int		normalsPerVertex();
	virtual int		texCoordsPerVertex();

    static  void *  creator();
    static  MStatus initialize();

    static  MTypeId id;

	private:
	void getFloat3(MObject attr, MFloatVector & val) const;
	void getFloat(MObject attr, float & val) const;

	// Attributes
	static MObject  aColorGain;
	static MObject  aColorBias;
	static MObject  aTranspGain;
	static MObject  aTranspBias;
	static MObject  aNormalsPerVertex;
	static MObject  aColorsPerVertex;
	static MObject  aColorSetName;
	static MObject  aTexRotateX;
	static MObject  aTexRotateY;
	static MObject  aTexRotateZ;

	// Cached internal values
	float3	mColorGain;
	float3	mColorBias;
	float	mTranspGain;
	float	mTranspBias;
	
	unsigned int mNormalsPerVertex;
	unsigned int mColorsPerVertex;
	MString mColorSetName;
	float	mTexRotateX;
	float	mTexRotateY;
	float	mTexRotateZ;

	MImage	*mSampleImage;
	GLuint	mSampleImageId;

	bool	mAttributesChanged;
};

MTypeId hwColorPerVertexShader::id( 0x00105450 );

MObject  hwColorPerVertexShader::aColorGain;
MObject  hwColorPerVertexShader::aTranspGain;
MObject  hwColorPerVertexShader::aColorBias;
MObject  hwColorPerVertexShader::aTranspBias;
MObject  hwColorPerVertexShader::aNormalsPerVertex;
MObject  hwColorPerVertexShader::aColorsPerVertex;
MObject  hwColorPerVertexShader::aColorSetName;
MObject  hwColorPerVertexShader::aTexRotateX;
MObject  hwColorPerVertexShader::aTexRotateY;
MObject  hwColorPerVertexShader::aTexRotateZ;

void hwColorPerVertexShader::postConstructor( )
{
	setMPSafe(false);
}

hwColorPerVertexShader::hwColorPerVertexShader()
{
	mColorGain[0] = mColorGain[1] = mColorGain[2] = 1.0f;
	mColorBias[0] = mColorBias[1] = mColorBias[2] = 0.0f;
	mTranspGain = 1.0f;
	mTranspBias = 0.0f;
	mAttributesChanged = false;
	mNormalsPerVertex = 0;
	mColorsPerVertex = 0;
	mSampleImage = 0;
	mSampleImageId = 0;
	mTexRotateX = 0.0f;
	mTexRotateY = 0.0f;
	mTexRotateZ = 0.0f;
}

hwColorPerVertexShader::~hwColorPerVertexShader()
{
	if ( mSampleImageId > 0 )
		glDeleteTextures(1, &mSampleImageId );
}

// The creator method creates an instance of the
// hwColorPerVertexShader class and is the first method called by Maya
// when a hwColorPerVertexShader needs to be created.
//
void * hwColorPerVertexShader::creator()
{
    return new hwColorPerVertexShader();
}


/* virtual */
bool 
hwColorPerVertexShader::setInternalValueInContext( const MPlug& plug,
												  const MDataHandle& handle,
												  MDGContext&)
{
	bool handledAttribute = false;
	if (plug == aNormalsPerVertex)
	{
		handledAttribute = true;
		mNormalsPerVertex = (unsigned int) handle.asInt();
	}
	else if (plug == aColorsPerVertex)
	{
		handledAttribute = true;
		mColorsPerVertex = (unsigned int) handle.asInt();
	}
	else if (plug == aColorSetName)
	{
		handledAttribute = true;
		mColorSetName = (MString) handle.asString();
	}
	else if (plug == aTexRotateX)
	{
		handledAttribute = true;
		mTexRotateX = handle.asFloat();
	}
	else if (plug == aTexRotateY)
	{
		handledAttribute = true;
		mTexRotateY = handle.asFloat();
	}
	else if (plug == aTexRotateZ)
	{
		handledAttribute = true;
		mTexRotateZ = handle.asFloat();
	}

	else if (plug == aColorGain)
	{
		handledAttribute = true;
		float3 & val = handle.asFloat3();
		if (val[0] != mColorGain[0] || 
			val[1] != mColorGain[1] || 
			val[2] != mColorGain[2])
		{
			mColorGain[0] = val[0];
			mColorGain[1] = val[1];
			mColorGain[2] = val[2];
			mAttributesChanged = true;
		}
	}
	else if (plug == aColorBias)
	{
		handledAttribute = true;
		float3 &val = handle.asFloat3();
		if (val[0] != mColorBias[0] || 
			val[1] != mColorBias[1] || 
			val[2] != mColorBias[2])
		{
			mColorBias[0] = val[0];
			mColorBias[1] = val[1];
			mColorBias[2] = val[2];
			mAttributesChanged = true;
		}
	}
	else if (plug == aTranspGain)
	{
		handledAttribute = true;
		float val = handle.asFloat();
		if (val != mTranspGain)
		{
			mTranspGain = val;
			mAttributesChanged = true;
		}
	}
	else if (plug == aTranspBias)
	{
		handledAttribute = true;
		float val = handle.asFloat();
		if (val != mTranspBias)
		{
			mTranspBias = val;
			mAttributesChanged = true;
		}
	}

	return handledAttribute;
}

/* virtual */
bool
hwColorPerVertexShader::getInternalValueInContext( const MPlug& plug,
											  MDataHandle& handle,
											  MDGContext&)
{
	bool handledAttribute = false;
	if (plug == aColorGain)
	{
		handledAttribute = true;
		handle.set( mColorGain[0], mColorGain[1], mColorGain[2] );
	}
	else if (plug == aColorBias)
	{
		handledAttribute = true;
		handle.set( mColorBias[0], mColorBias[1], mColorBias[2] );
	}
	else if (plug == aTranspGain)
	{
		handledAttribute = true;
		handle.set( mTranspGain );
	}
	else if (plug == aTranspBias)
	{
		handledAttribute = true;
		handle.set( mTranspBias );
	}
	else if (plug == aNormalsPerVertex)
	{
		handledAttribute = true;
		handle.set( (int)mNormalsPerVertex );
	}
	else if (plug == aColorsPerVertex)
	{
		handledAttribute = true;
		handle.set( (int)mColorsPerVertex );
	}
	else if (plug == aColorSetName)
	{
		handledAttribute = true;
		handle.set( mColorSetName );
	}
	else if (plug == aTexRotateX)
	{
		handledAttribute = true;
		handle.set( mTexRotateX ); 
	}
	else if (plug == aTexRotateY)
	{
		handledAttribute = true;
		handle.set( mTexRotateY ); 
	}
	else if (plug == aTexRotateZ)
	{
		handledAttribute = true;
		handle.set( mTexRotateZ ); 
	}

	return handledAttribute;
}

// The initialize routine is called after the node has been created.
// It sets up the input and output attributes and adds them to the node.
// Finally the dependencies are arranged so that when the inputs
// change Maya knowns to call compute to recalculate the output values.
//
MStatus hwColorPerVertexShader::initialize()
{
    MFnNumericAttribute nAttr; 
    MFnTypedAttribute tAttr; 
	MStatus status;

    // Create input attributes.
	// All attributes are cached internal
	//
    aColorGain = nAttr.createColor( "colorGain", "cg", &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(1.f, 1.f, 1.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

    aTranspGain = nAttr.create("transparencyGain", "tg",
						  MFnNumericData::kFloat, 1.f, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(1.f));
    CHECK_MSTATUS( nAttr.setSoftMin(0.f));
    CHECK_MSTATUS( nAttr.setSoftMax(2.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

    aColorBias = nAttr.createColor( "colorBias", "cb", &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(0.f, 0.f, 0.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

    aTranspBias = nAttr.create( "transparencyBias", "tb",
						   MFnNumericData::kFloat, 0.f, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(0.f));
    CHECK_MSTATUS( nAttr.setSoftMin(-1.f));
    CHECK_MSTATUS( nAttr.setSoftMax(1.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

	aNormalsPerVertex = nAttr.create("normalsPerVertex", "nv",
		MFnNumericData::kInt, 0, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(false));
    CHECK_MSTATUS( nAttr.setDefault(0));
    CHECK_MSTATUS( nAttr.setSoftMin(0));
    CHECK_MSTATUS( nAttr.setSoftMax(3));
	nAttr.setCached( true );
	nAttr.setInternal( true );

	aColorsPerVertex = nAttr.create("colorsPerVertex", "cv",
		MFnNumericData::kInt, 0, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(false));
    CHECK_MSTATUS( nAttr.setDefault(0));
    CHECK_MSTATUS( nAttr.setSoftMin(0));
    CHECK_MSTATUS( nAttr.setSoftMax(5));
	nAttr.setCached( true );
	nAttr.setInternal( true );

	aColorSetName = tAttr.create("colorSetName", "cs",
		MFnData::kString, MObject::kNullObj, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( tAttr.setStorable(true));
    CHECK_MSTATUS( tAttr.setKeyable(false));
	tAttr.setCached( true );
	tAttr.setInternal( true );

    aTexRotateX = nAttr.create( "texRotateX", "tx",
						   MFnNumericData::kFloat, 0.f, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(0.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );
	
    aTexRotateY = nAttr.create( "texRotateY", "ty",
						   MFnNumericData::kFloat, 0.f, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(0.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

    aTexRotateZ = nAttr.create( "texRotateZ", "tz",
						   MFnNumericData::kFloat, 0.f, &status);
    CHECK_MSTATUS( status );
    CHECK_MSTATUS( nAttr.setStorable(true));
    CHECK_MSTATUS( nAttr.setKeyable(true));
    CHECK_MSTATUS( nAttr.setDefault(0.f));
	nAttr.setCached( true );
	nAttr.setInternal( true );

	// create output attributes here
	// outColor is the only output attribute and it is inherited
	// so we do not need to create or add it.
	//

	// Add the attributes here

    CHECK_MSTATUS( addAttribute(aColorGain));
    CHECK_MSTATUS( addAttribute(aTranspGain));
    CHECK_MSTATUS( addAttribute(aColorBias));
    CHECK_MSTATUS( addAttribute(aTranspBias));
	CHECK_MSTATUS( addAttribute(aNormalsPerVertex));
	CHECK_MSTATUS( addAttribute(aColorsPerVertex));
	CHECK_MSTATUS( addAttribute(aColorSetName));
	CHECK_MSTATUS( addAttribute(aTexRotateX));
	CHECK_MSTATUS( addAttribute(aTexRotateY));
	CHECK_MSTATUS( addAttribute(aTexRotateZ));

    CHECK_MSTATUS( attributeAffects (aColorGain,  outColor));
    CHECK_MSTATUS( attributeAffects (aTranspGain, outColor));
    CHECK_MSTATUS( attributeAffects (aColorBias,  outColor));
    CHECK_MSTATUS( attributeAffects (aTranspBias, outColor));
    CHECK_MSTATUS( attributeAffects (aNormalsPerVertex, outColor));
    CHECK_MSTATUS( attributeAffects (aColorsPerVertex, outColor));
    CHECK_MSTATUS( attributeAffects (aColorSetName, outColor));
    CHECK_MSTATUS( attributeAffects (aTexRotateX, outColor));
    CHECK_MSTATUS( attributeAffects (aTexRotateY, outColor));
    CHECK_MSTATUS( attributeAffects (aTexRotateZ, outColor));

    return MS::kSuccess;
}

/* virtual */
MStatus		
hwColorPerVertexShader::bind( const MDrawRequest& request,
							  M3dView& view )
{
    return MS::kSuccess;
}
	
/* virtual */ 
MStatus		
hwColorPerVertexShader::unbind( const MDrawRequest& request,
								M3dView& view )
{
    return MS::kSuccess;
}


/* virtual */
MStatus		
hwColorPerVertexShader::geometry( const MDrawRequest& request,
							M3dView& view,
							int prim,
							unsigned int writable,
							int indexCount,
							const unsigned int * indexArray,
							int vertexCount,
							const int * vertexIDs,
							const float * vertexArray,
							int normalCount,
							const float ** normalArrays,
							int colorCount,
							const float ** colorArrays,
							int texCoordCount,
							const float ** texCoordArrays)
{
	// If we're received a color, that takes priority
	//
	if( colorCount > 0 && colorArrays[ colorCount - 1] != NULL )
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS); 
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glDisable(GL_LIGHTING);

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer( 4, GL_FLOAT, 0, &colorArrays[colorCount - 1][0]);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer ( 3, GL_FLOAT, 0, &vertexArray[0] );
		glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );

		glEnableClientState(GL_COLOR_ARRAY);

		glPopClientAttrib();
		glPopAttrib();

		return MS::kSuccess;
	}
	// If this attribute is enabled, normals, tangents,
	// and binormals can be visualized using false coloring.
	// Negative values will clamp to black however.
	if ((unsigned int)normalCount > mNormalsPerVertex) {
		normalCount = mNormalsPerVertex;
		return MS::kSuccess;
	}
	else if (normalCount > 0)
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS); 
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glDisable(GL_LIGHTING);

		if (normalCount > 1)
		{
			if (normalCount == 2)
			{
	#ifdef _TANGENT_DEBUG
				const float *tangents = (const float *)&normalArrays[1][0];
				for (unsigned int  i=0; i< vertexCount; i++)
				{
					cout<<"tangent["<<i
						<<"] = "<<tangnets[i*3]
						<<","<<tangents[i*3+1]<<","<<tangents[i*3+2]<<endl;
				}
	#endif
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, &normalArrays[1][0]);
			}
			else
			{
	#ifdef _BINORMAL_DEBUG
				const float *binormals = (const float *)&normalArrays[2][0];
				for (unsigned int  i=0; i< vertexCount; i++)
				{
					cout<<"binormals["<<i<<"] = "<<binormals[i*3]
						<<","<<binormals[i*3+1]<<","<<binormals[i*3+2]<<endl;
				}
	#endif
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, &normalArrays[2][0]);
			}
		}
		else if (normalCount)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(3, GL_FLOAT, 0, &normalArrays[0][0]);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer ( 3, GL_FLOAT, 0, &vertexArray[0] );
		glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );

		glDisableClientState(GL_COLOR_ARRAY);

		glPopClientAttrib();
		glPopAttrib();

		return MS::kSuccess;
	}
	else
	{
		return draw( prim, writable, indexCount, indexArray,
			vertexCount, vertexArray, colorCount, colorArrays );
	}
}


// Batch overrides
/* virtual */
MStatus	
hwColorPerVertexShader::glBind(const MDagPath& shapePath)
{
    return MS::kSuccess;
}

/* virtual */
MStatus	
hwColorPerVertexShader::glUnbind(const MDagPath& shapePath)
{
    return MS::kSuccess;
}

/* virtual */
MStatus	hwColorPerVertexShader::glGeometry( const MDagPath& path,
                                int prim,
								unsigned int writable,
								int indexCount,
								const unsigned int * indexArray,
								int vertexCount,
								const int * vertexIDs,
								const float * vertexArray,
								int normalCount,
								const float ** normalArrays,
								int colorCount,
								const float ** colorArrays,
								int texCoordCount,
								const float ** texCoordArrays)
{
	// If this attribute is enabled, normals, tangents,
	// and binormals can be visualized using false coloring.
	// Negative values will clamp to black however.

#ifdef _TEST_FILE_PATH_DURING_DRAW_
	if (path.hasFn( MFn::kMesh ) )
	{
		// Check the # of uvsets. If no uvsets
		// then can't return tangent or binormals
		//
		MGlobal::displayInfo( path.fullPathName() );

		MFnMesh fnMesh( path.node() );
		numUVSets = fnMesh.numUVSets();
	}
#endif
	if ((unsigned int)normalCount > mNormalsPerVertex)
		normalCount = mNormalsPerVertex;
	if (normalCount > 0)
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS); 
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);

		if (normalCount > 1)
		{
			if (normalCount == 2)
			{
	//#define _TANGENT_DEBUG
	#ifdef _TANGENT_DEBUG
				const float *tangents = (const float *)&normalArrays[1][0];
				for (int  i=0; i< vertexCount; i++)
				{
					cout<<"tangent["<<i<<"] = "<<tangents[i*3]
						<<","<<tangents[i*3+1]<<","<<tangents[i*3+2]<<endl;
				}
	#endif
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, &normalArrays[1][0]);
			}
			else
			{
	#ifdef _BINORMAL_DEBUG
				const float *binormals = (const float *)&normalArrays[2][0];
				for (unsigned int  i=0; i< vertexCount; i++)
				{
					cout<<"binormals["<<i<<"] = "<<binormals[i*3]
						<<","<<binormals[i*3+1]<<","<<binormals[i*3+2]<<endl;
				}
	#endif
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, &normalArrays[2][0]);
			}
		}
		else if (normalCount)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(3, GL_FLOAT, 0, &normalArrays[0][0]);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer ( 3, GL_FLOAT, 0, &vertexArray[0] );
		glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );

		glDisableClientState(GL_COLOR_ARRAY);

		glPopClientAttrib();
		glPopAttrib();

		return MS::kSuccess;
	}
	else
	{
		return draw( prim, writable, indexCount, indexArray,
			vertexCount, vertexArray, colorCount, colorArrays );
	}
}

/* virtual */
MStatus hwColorPerVertexShader::renderSwatchImage( MImage& outImage )
{
	MStatus status = MStatus::kFailure;

	// Get the hardware renderer utility class
	MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
	if (pRenderer)
	{
		const MString& backEndStr = pRenderer->backEndString();

		// Get geometry
		// ============
		unsigned int* pIndexing = 0;
		unsigned int  numberOfData = 0;
		unsigned int  indexCount = 0;

		MHardwareRenderer::GeometricShape gshape = MHardwareRenderer::kDefaultSphere;
		//MHardwareRenderer::GeometricShape gshape = MHardwareRenderer::kDefaultPlane;
		MGeometryData* pGeomData =
			pRenderer->referenceDefaultGeometry( gshape, numberOfData, pIndexing, indexCount );
		if( !pGeomData )
		{
			return MStatus::kFailure;
		}

		// Make the swatch context current
		// ===============================
		//
		unsigned int width, height;
		outImage.getSize( width, height );
		unsigned int origWidth = width;
		unsigned int origHeight = height;

		MStatus status2 = pRenderer->makeSwatchContextCurrent( backEndStr, width, height );

		if( status2 != MS::kSuccess )
		{
			pRenderer->dereferenceGeometry( pGeomData, numberOfData );
			return MStatus::kFailure;
		}

	    glPushAttrib(GL_ALL_ATTRIB_BITS);

		// Get the light direction from the API, and use it
		// =============================================
		{
			float light_pos[4];
			pRenderer->getSwatchLightDirection( light_pos[0], light_pos[1], light_pos[2], light_pos[3] );

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glLightfv(GL_LIGHT0, GL_POSITION, light_pos );
			glPopMatrix();

			float light_ambt[4] = {1.0f, 1.0f, 1.0f, 1.0};
			float light_diff[4] = {1.0f, 1.0f, 1.0f, 1.0};
			float light_spec[4] = {1.0f, 1.0f, 1.0f, 1.0};

			glLightfv( GL_LIGHT0, GL_AMBIENT,  light_ambt );
			glLightfv( GL_LIGHT0, GL_DIFFUSE,  light_diff );
			glLightfv( GL_LIGHT0, GL_SPECULAR, light_spec );

			glEnable( GL_LIGHTING );
			glEnable( GL_LIGHT0 );
		}

		// Get camera
		// ==========
		{
			// Get the camera frustum from the API
			double l, r, b, t, n, f;
			pRenderer->getSwatchOrthoCameraSetting( l, r, b, t, n, f );

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho( l, r, b, t, n, f );

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}

		// Draw The Swatch
		// ===============
		drawTheSwatch( pGeomData, pIndexing, numberOfData, indexCount );

		// Read pixels back from swatch context to MImage
		// ==============================================
		pRenderer->readSwatchContextPixels( backEndStr, outImage );

		// Double check the outing going image size as image resizing
		// was required to properly read from the swatch context
		outImage.getSize( width, height );
		if (width != origWidth || height != origHeight)
		{
			status = MStatus::kFailure;
		}
		else
		{
			status = MStatus::kSuccess;
		}

		glPopAttrib();
	}

	return status;
}


void
hwColorPerVertexShader::drawTheSwatch( MGeometryData* pGeomData,
									   unsigned int* pIndexing,
									   unsigned int  numberOfData,
									   unsigned int  indexCount )

{
	MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
	if( !pRenderer )	return;

	// Set the default background color
	{
		float r, g, b, a;
		MHWShaderSwatchGenerator::getSwatchBackgroundColor( r, g, b, a );
		glClearColor( r, g, b, a );
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	{
		// Enable the blending to get the transparency to work
		//
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// Load in a sample background image
	if (mSampleImageId == 0)
	{
		mSampleImage = new MImage;
		MStatus rstatus = mSampleImage->readFromFile("C:\\temp\\maya.gif");
		if (rstatus == MStatus::kSuccess)			
		{
			unsigned int w, h;
			mSampleImage->getSize( w, h );
			if (w > 2 && h > 2 )
			{
				glGenTextures( 1, &mSampleImageId );
				if (mSampleImageId > 0)
				{
					glEnable(GL_TEXTURE_2D);
					glBindTexture ( GL_TEXTURE_2D, mSampleImageId );
					glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, 
						GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *) mSampleImage->pixels() );
				}
			}
		}
		if (mSampleImage)
		{
			delete mSampleImage;
		}
	}

	// Overlay the background checker board
	//
	bool drawBackGround = ( mTranspBias > 0.0f );
	bool drawBackGroundTexture = (mSampleImageId != 0);
	if (drawBackGround)
	{
		if (drawBackGroundTexture)
		{
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);

			glBindTexture(GL_TEXTURE_2D, mSampleImageId );

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glEnable(GL_TEXTURE_2D);
		}

		unsigned int numberOfRepeats = 8;
		MColor quadColor( 0.5f, 0.5f, 0.5f, 1.0f );
		pRenderer->drawSwatchBackGroundQuads( quadColor, 
							drawBackGroundTexture, numberOfRepeats );
		
		if (drawBackGroundTexture)
			glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}

	{
		// Set some example material
		//
		float ambient[4]  = { 0.1f, 0.1f, 0.1f, 1.0f };
		float diffuse[4]  = { 0.7f, 0.7f, 0.7f, 1.0f };
		float specular[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		float emission[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission );
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular );
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);

		// Track diffuse color
		float swatchColor[4];
		float biasT = 1.0f - mTranspGain - mTranspBias;
		swatchColor[0] = (diffuse[0] * mColorGain[0]) + mColorBias[0];
		swatchColor[1] = (diffuse[1] * mColorGain[1]) + mColorBias[1];
		swatchColor[2] = (diffuse[2] * mColorGain[2]) + mColorBias[2];
		swatchColor[3] = (diffuse[3] * mTranspGain) + biasT;

		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, swatchColor );			
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		glEnable(GL_COLOR_MATERIAL);
		glColor4fv( swatchColor );
	}

	if (pGeomData)
	{
		glPushClientAttrib ( GL_CLIENT_VERTEX_ARRAY_BIT );

		if (mNormalsPerVertex >= 1)
		{
			glDisable(GL_LIGHTING);
			float *normalData = (float *)( pGeomData[1].data() );
			float * tangentData = (float *)( pGeomData[3].data() );
			float * binormalData = (float *)( pGeomData[4].data() );
			if (normalData && mNormalsPerVertex  == 1)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, normalData);
			}
			else if (tangentData && mNormalsPerVertex  == 2)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, tangentData);
			}
			else if (binormalData)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(3, GL_FLOAT, 0, binormalData);
			}
		}

		float *vertexData = (float *)( pGeomData[0].data() );
		if (vertexData)
		{
			glEnableClientState( GL_VERTEX_ARRAY );
			glVertexPointer ( 3, GL_FLOAT, 0, vertexData );
		}

		float *normalData = (float *)( pGeomData[1].data() );
		if (normalData)
		{
			glEnableClientState( GL_NORMAL_ARRAY );
			glNormalPointer (    GL_FLOAT, 0, normalData );
		}

		if (mSampleImageId > 0)
		{
			float *uvData = (float *) (pGeomData[2].data() );
			if (uvData)
			{
				glBindTexture ( GL_TEXTURE_2D, mSampleImageId );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				glEnable(GL_TEXTURE_2D);
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 2, GL_FLOAT, 0, uvData );

				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();
				glScalef( 0.5f, 0.5f, 1 );
				//glTranslatef(0.5f, 0.5f, 0.0f);
				glRotatef( mTexRotateX, 1.0f, 0.0f, 0.0f);
				glRotatef( mTexRotateY, 0.0, 1.0f, 0.0f);
				glRotatef( mTexRotateZ, 0.0, 0.0f, 1.0f);
				glMatrixMode(GL_MODELVIEW);
			}
		}

		if (vertexData && normalData && pIndexing )
			glDrawElements ( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, pIndexing );

		glPopClientAttrib();

		// Release data references
		pRenderer->dereferenceGeometry( pGeomData, numberOfData );

	}
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_LIGHTING );
}


MStatus	hwColorPerVertexShader::draw( 
                                int prim,
								unsigned int writable,
								int indexCount,
								const unsigned int * indexArray,
								int vertexCount,
								const float * vertexArray,
								int colorCount,
								const float ** colorArrays)
{
	// Should check this value to allow caching
	// of color values.
	mAttributesChanged = false;

	// We assume triangles here.
	//
    if (!vertexCount || 
		!((prim == GL_TRIANGLES) || (prim == GL_TRIANGLE_STRIP)))
	{
        return MS::kFailure;
    }
 
    //glPushAttrib(GL_ALL_ATTRIB_BITS); // This is really overkill
	glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glDisable(GL_LIGHTING);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer ( 3, GL_FLOAT, 0, &vertexArray[0] );

	bool localCopyOfColors = false;
	float * colors = NULL;

	// Do "cheesy" multi-pass here for more than one color set
	//
	bool blendSet = false;

	if( colorCount <= 1 )
	{
		glDisable(GL_BLEND);
		if( colorCount > 0 && colorArrays[0] != NULL)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, colorArrays[0]);
		}
		else
		{
			glColor4f(1.0, 0.5, 1.0, 1.0);
		}
		glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );
	}
	else 
	{
		// Do a 1:1 Blend if we have more than on color set available.
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnableClientState(GL_COLOR_ARRAY);
		blendSet = true;

		for (int i=0; i<colorCount; i++)		
		{
			if (colorArrays[i] != NULL)
			{
				// Apply gain and bias
				//
				bool haveTransparency = false;

				if (mColorGain[0] != 1.0f ||
					mColorGain[1] != 1.0f ||
					mColorGain[2] != 1.0f ||
					mColorBias[0] != 0.0f ||
					mColorBias[1] != 0.0f ||
					mColorBias[2] != 0.0f ||
					mTranspGain != 1.0f ||
					mTranspBias != 0.0f)
				{
					// This sample code is a CPU bottlneck. It could be
					// replaced with a vertex program or color matrix
					// operator.
					//

					// We really want to scale 1-transp.
					// T = 1 - ((1-T)*gain + bias) = 
					//   = T * gain + 1 - gain - bias
					float biasT = 1 - mTranspGain - mTranspBias;

					// Either make a copy or read directly colors.
					//
					if (!(writable & kWriteColorArrays)) 
					{
						unsigned int numFloats = 4 * vertexCount;
						colors = new float[numFloats];
						localCopyOfColors = true;
					}
					else
					{
						colors = (float *)colorArrays[i];
					}

					float *origColors = (float *)colorArrays[i];
					float *colorPtr = colors;
					for (int ii = vertexCount ; ii-- ; )
					{
						colorPtr[0] = (origColors[0] * mColorGain[0]) + mColorBias[0];
						colorPtr[1] = (origColors[1] * mColorGain[1]) + mColorBias[1];
						colorPtr[2] = (origColors[2] * mColorGain[2]) + mColorBias[2];
						colorPtr[3] = (origColors[3] * mTranspGain) + biasT;
						if (colorPtr[3] != 1.0f)
							haveTransparency = true;

						colorPtr += 4;
						origColors += 4;
					}
				}
				else
				{
					// Do a quick test for transparency.
					// This attribute is currently not
					// being passed through to the plugin.
					// so must be recomputed per refresh.
					//
					colors = (float *)colorArrays[i];
					float *colorPtr = colors;
					for (int ii = vertexCount ; ii-- ; )
					{
						if (colorPtr[3] != 1.0f )
						{
							haveTransparency = true;
							break;
						}
						else
							colorPtr += 4;
					}
				}

				// Blending when there are alpha values
				//
				if (!blendSet)
				{
					if (haveTransparency)
					{
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					}
				}
				else {
					glDisable(GL_BLEND);
				}
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_FLOAT, 0, colors);
			}
			else {
				glDisable(GL_BLEND);
				glDisableClientState(GL_COLOR_ARRAY);
				glColor4f(1.0, 1.0, 1.0, 1.0);
			}

			glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );
		}
	}

	glDisable(GL_BLEND);
	glPopClientAttrib();
	glPopAttrib();

	// Delete any local storage if we are passed 
	// non-writable data.
	if (localCopyOfColors)
	{
		delete[] colors;
		colors = NULL;
	}

	return MS::kSuccess;
}

// virtual
// Tells maya that color per vertex will be needed.
int	hwColorPerVertexShader::normalsPerVertex()
{
	unsigned int numNormals = mNormalsPerVertex;

	MStringArray setNames;
	const MDagPath & path = currentPath();	
	if (path.hasFn( MFn::kMesh ) )
	{
		// Check the # of uvsets. If no uvsets
		// then can't return tangent or binormals
		//
		// MGlobal::displayInfo( path.fullPathName() );

		MFnMesh fnMesh( path.node() );
		if (fnMesh.numUVSets() == 0)
		{
			// Put out a warnnig if we're asking for too many uvsets.
			MString dispWarn("Asking for more uvsets then available for shape: ");
			MString pathName = path.fullPathName();
			dispWarn = dispWarn + pathName;
			MGlobal::displayWarning( dispWarn  );
			numNormals = (mNormalsPerVertex > 1 ? 1 : 0);
		}
	}
	return numNormals;
}

// virtual
// Tells maya that texCoords per vertex will be needed.
int	hwColorPerVertexShader::texCoordsPerVertex()
{
	return 0;
}

// virtual
// Tells maya that color per vertex will be needed.
int	hwColorPerVertexShader::colorsPerVertex()
{
	int returnVal = 0;

	// Going to be displaying false coloring,
	// so skip getting internal colors.
	if (mNormalsPerVertex)
		return 0;
	else {
		const MDagPath & path = currentPath();	
		if (path.hasFn( MFn::kMesh ) )
		{
			MFnMesh fnMesh( path.node() );
			unsigned int numColorSets = fnMesh.numColorSets();
			if (numColorSets < 2)
				returnVal = numColorSets;
			else
				returnVal  = 2;
		}
	}
	return returnVal;
}

// virtual
// Tells maya that color per vertex wiill be needed.
bool hwColorPerVertexShader::hasTransparency()
{
	return true;
}

// The compute method is called by Maya when the input values
// change and the output values need to be recomputed.
//
// In this case this is only used for software shading, when the to
// compute the rendering swatches.
MStatus hwColorPerVertexShader::compute (const MPlug& plug, MDataBlock& data)
{
	MStatus returnStatus;

	// Check that the requested recompute is one of the output values
	//
	if ((plug != outColor) && (plug.parent() != outColor))
		return MS::kUnknownParameter;

	MDataHandle inputData = data.inputValue (aColorGain, &returnStatus);
	CHECK_MSTATUS( returnStatus );

	const MFloatVector & color = inputData.asFloatVector();

	MDataHandle outColorHandle = data.outputValue( outColor, &returnStatus );
	CHECK_MSTATUS( returnStatus );
    MFloatVector & outColor = outColorHandle.asFloatVector();
	outColor = color;

	CHECK_MSTATUS( data.setClean( plug ) );

	return MS::kSuccess;
}

// The initializePlugin method is called by Maya when the
// plugin is loaded. It registers the hwColorPerVertexShader node
// which provides Maya with the creator and initialize methods to be
// called when a hwColorPerVertexShader is created.
//
MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;

	const MString& swatchName =	MHWShaderSwatchGenerator::initialize();
	const MString UserClassify( "shader/surface/utility/:swatch/"+swatchName );

	MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");

	status = plugin.registerNode("hwColorPerVertexShader",
								 hwColorPerVertexShader::id, 
								 hwColorPerVertexShader::creator,
								 hwColorPerVertexShader::initialize,
								 MPxNode::kHwShaderNode, &UserClassify );
	CHECK_MSTATUS(status);

	return status;
}

// The unitializePlugin is called when Maya needs to unload the plugin.
// It basically does the opposite of initialize by calling
// the deregisterNode to remove it.
//
MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj );

	// Unregister all chamelion shader nodes
	status = plugin.deregisterNode( hwColorPerVertexShader::id );
	CHECK_MSTATUS(status);

	return status;
}


