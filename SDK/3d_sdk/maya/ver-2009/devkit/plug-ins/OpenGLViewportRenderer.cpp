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

#include <stdio.h>

#include <OpenGLViewportRenderer.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MRenderingInfo.h>
#include <maya/MFnCamera.h>
#include <maya/MAngle.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>

#include <maya/MItDag.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MBoundingBox.h>


#include <maya/MHardwareRenderer.h>
#include <stdio.h>
#include <maya/MIOStream.h>

#include <maya/MDrawTraversal.h>
#include <maya/MGeometryManager.h>
#include <maya/MGeometry.h>
#include <maya/MGeometryData.h>
#include <maya/MGeometryPrimitive.h>
#include <maya/MHwTextureManager.h>
#include <maya/MImageFileInfo.h>

#include <maya/MFnSingleIndexedComponent.h>


#if MAYA_API_VERSION > 800
	#define _USE_MGL_FT_
	#include <maya/MGLFunctionTable.h>
	static MGLFunctionTable *gGLFT = NULL;
#else
	#if defined(OSMac_MachO_)
		#include <OpenGL/gl.h>
	#else
		#include <GL/gl.h>
	#endif
#endif

OpenGLViewportRenderer::OpenGLViewportRenderer()
:	MViewportRenderer("OpenGLViewportRenderer")
{
	// Set the ui name
	fUIName.set( "Plugin OpenGL Renderer");

	// This renderer overrides all drawing
	fRenderingOverride = MViewportRenderer::kOverrideAllDrawing;

	// Set API and version number
	m_API = MViewportRenderer::kOpenGL;
	m_Version = 2.0f;
}

/* virtual */
OpenGLViewportRenderer::~OpenGLViewportRenderer()
{
	uninitialize();
}

/* virtual */	
MStatus	
OpenGLViewportRenderer::initialize()
{
#if defined(_USE_MGL_FT)
	// Get a pointer to a GL function table
	Tboolean useMGLFT = false;
	MHardwareRenderer *rend = MHardwareRenderer::theRenderer();
	if (rend)
		gGLFT = rend->glFunctionTable();
	if (!gGLFT)
		return MStatus::kFailure;
#endif
	return MStatus::kSuccess;
}

/* virtual */	
MStatus	
OpenGLViewportRenderer::uninitialize()
{	
	gGLFT = NULL;
	return MStatus::kSuccess;
}

/* virtual */ 
MStatus	
OpenGLViewportRenderer::render(const MRenderingInfo &renderInfo)
{
	//printf("Render using (%s : %s) renderer\n", fName.asChar(), fUIName.asChar());

	// Support direct rendering to an OpenGL context only.
	//
	if (renderInfo.renderingAPI() == MViewportRenderer::kOpenGL)
	{
		// This is not required as the target has already been made current
		// before the render() call.
		// renderTarget.makeTargetCurrent();

		// Do some rendering...
		renderToTarget( renderInfo );
	}
	return MStatus::kSuccess;
}

/* virtual */ 
bool	
OpenGLViewportRenderer::nativelySupports( MViewportRenderer::RenderingAPI api, 
									   float version )
{
	// Do API check
	return (api == m_API);
}

/* virtual */ bool	
OpenGLViewportRenderer::override( MViewportRenderer::RenderingOverride override )
{
	// Check override
	return (override == fRenderingOverride);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering methods
////////////////////////////////////////////////////////////////////////////////////////////////////
bool
OpenGLViewportRenderer::drawSurface( const MDagPath &dagPath, bool active, bool templated )
{

	bool drewSurface = false;
	if ( dagPath.hasFn( MFn::kMesh ))
	{
		MObject object = dagPath.node();
		MFnMesh mesh(object);

		// Figure out texturing
		//
		bool haveTexture = false;
		int	numUVsets = mesh.numUVSets();
		MString uvSetName;
		MObjectArray textures;
		if (numUVsets > 0)
		{
			mesh.getCurrentUVSetName( uvSetName );
			MStatus status = mesh.getAssociatedUVSetTextures(uvSetName, textures);
			if (status == MS::kSuccess && textures.length())
			{
				haveTexture = true;
			}
		}

		bool haveColors = false;
		int	numColors = mesh.numColorSets();
		MString colorSetName;
		if (numColors > 0)
		{
			haveColors = true;
			mesh.getCurrentColorSetName(colorSetName);
		}

		bool useNormals = false;

		// Setup our requirements needs.
		MGeometryRequirements requirements;
		requirements.addPosition();
		if (useNormals)
			requirements.addNormal();
		if (haveTexture)
			requirements.addTexCoord( uvSetName );
		if (haveColors)
			requirements.addColor( colorSetName );

		// Test for tangents and binormals
		bool testBinormal = false;
		if (testBinormal)
			requirements.addBinormal( uvSetName );
		bool testTangent= false;
		if (testTangent)
			requirements.addTangent( uvSetName );

		// Cheesy test for filtering out 3 components for drawing.
#if defined(_TEST_OGL_RENDERER_COMPONENT_FILTER_)
		MFnSingleIndexedComponent components;
		MObject comp = components.create( MFn::kMeshPolygonComponent );
		components.addElement( 1 );
		components.addElement( 200 );
		components.addElement( 5000 );
		MGeometry geom = MGeometryManager::getGeometry( dagPath, requirements, &comp );
#else
		MGeometry geom = MGeometryManager::getGeometry( dagPath, requirements, NULL );
#endif
		unsigned int numPrims = geom.primitiveArrayCount();
		if (numPrims)
		{
			const MGeometryPrimitive prim = geom.primitiveArray(0);

			unsigned int numElem = prim.elementCount();
			if (numElem)
			{
				//MGeometryData::ElementType primType = prim.dataType();
				unsigned int *idx = (unsigned int *) prim.data();

				// Get the position data
				const MGeometryData pos = geom.position();
				float * posPtr = (float * )pos.data();

				bool haveData = idx && posPtr; 

				// Get the normals data
				float * normPtr = NULL;
				if (useNormals)
				{
					const MGeometryData norm = geom.normal();				
					normPtr = (float * )norm.data();
				}

				// Get the texture coordinate data
				float *uvPtr = NULL;
				if (haveTexture)
				{
					const MGeometryData uvs = geom.texCoord( uvSetName );
					uvPtr = (float *)uvs.data();
				}

				unsigned int numColorComponents = 4;
				float *clrPtr = NULL;
				if (haveColors)
				{
					const MGeometryData clrs = geom.color( colorSetName );
					clrPtr = (float *)clrs.data();
				}
				else if (testBinormal)
				{
					const MGeometryData binorm = geom.binormal( uvSetName );
					clrPtr = (float *)binorm.data();
					numColorComponents = 3;
				}
				else if (testTangent)
				{
					const MGeometryData tang = geom.tangent( uvSetName );
					clrPtr = (float *)tang.data();
					numColorComponents = 3;
				}


				if (haveData)
				{
					drewSurface = true;

					bool drawWire = false;

					MMatrix  matrix = dagPath.inclusiveMatrix();
					#if defined(_USE_MGL_FT_)
						gGLFT->glMatrixMode( MGL_MODELVIEW );
						gGLFT->glPushMatrix();
						gGLFT->glMultMatrixd( &(matrix.matrix[0][0]) );
					#else
						::glMatrixMode( GL_MODELVIEW );
						::glPushMatrix();
						::glMultMatrixd( &(matrix.matrix[0][0]) );
					#endif

					// Setup state, and routing.
					//
					bool boundTexture = false;
					if (uvPtr)
					{
						MImageFileInfo::MHwTextureType hwType;
						if (MS::kSuccess == MHwTextureManager::glBind( textures[0], hwType ))
						{
							boundTexture = true;

							#if defined(_USE_MGL_FT_)
							gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_WRAP_S, MGL_REPEAT);
							gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_WRAP_T, MGL_REPEAT);
							gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MIN_FILTER, MGL_LINEAR);
							gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MAG_FILTER, MGL_LINEAR);

							if (!clrPtr)
								gGLFT->glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
							gGLFT->glEnableClientState( MGL_TEXTURE_COORD_ARRAY );
							gGLFT->glTexCoordPointer( 2, MGL_FLOAT, 0, uvPtr );

							#else
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

							if (!clrPtr)
								glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
							glEnableClientState( GL_TEXTURE_COORD_ARRAY );
							glTexCoordPointer( 2, GL_FLOAT, 0, uvPtr );

							#endif

							drawWire = false;
						}
					}

					if (clrPtr)
					{
						#if defined(_USE_MGL_FT_)
						gGLFT->glEnableClientState(MGL_COLOR_ARRAY);
						gGLFT->glColorPointer( numColorComponents, MGL_FLOAT, 0, clrPtr );
						#else
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer( numColorComponents, GL_FLOAT, 0, clrPtr );
						#endif
					}

					#if defined(_USE_MGL_FT_)
					gGLFT->glEnableClientState( MGL_VERTEX_ARRAY );
					gGLFT->glVertexPointer ( 3, MGL_FLOAT, 0, posPtr );
					#else
					glEnableClientState( GL_VERTEX_ARRAY );
					glVertexPointer ( 3, GL_FLOAT, 0, posPtr );
					#endif

					if (normPtr)
					{
						#if defined(_USE_MGL_FT_)
						gGLFT->glEnableClientState( MGL_NORMAL_ARRAY );
						gGLFT->glNormalPointer (    MGL_FLOAT, 0, normPtr );		
						#else
						glEnableClientState( GL_NORMAL_ARRAY );
						glNormalPointer (    GL_FLOAT, 0, normPtr );		
						#endif
						drawWire = false;
					}

					// Draw
					if (drawWire)
					{
						#if defined(_USE_MGL_FT_)
						gGLFT->glPolygonMode(MGL_FRONT_AND_BACK, MGL_LINE);
						gGLFT->glDrawElements ( MGL_TRIANGLES, numElem, MGL_UNSIGNED_INT, idx );		
						gGLFT->glPolygonMode(MGL_FRONT_AND_BACK, MGL_FILL);

						if (clrPtr)
						{
							gGLFT->glDisableClientState(MGL_COLOR_ARRAY);
						}	
						#else
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						glDrawElements ( GL_TRIANGLES, numElem, GL_UNSIGNED_INT, idx );		
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

						if (clrPtr)
						{
							glDisableClientState(GL_COLOR_ARRAY);
						}	
						#endif
					}
					else
					{
						#if defined(_USE_MGL_FT_)
						if (active)
						{
							gGLFT->glEnable(MGL_POLYGON_OFFSET_FILL);
							gGLFT->glPolygonOffset( 0.95f, 1.0f ); 
						}
						gGLFT->glDrawElements ( MGL_TRIANGLES, numElem, MGL_UNSIGNED_INT, idx );		
						if (active)
							gGLFT->glDisable(MGL_POLYGON_OFFSET_FILL);

						if (normPtr)
							gGLFT->glDisableClientState( MGL_NORMAL_ARRAY );
						if (boundTexture)
						{
							gGLFT->glDisableClientState( MGL_TEXTURE_COORD_ARRAY );
							gGLFT->glDisable(MGL_TEXTURE_2D);
							gGLFT->glBindTexture(MGL_TEXTURE_2D, 0);
						}
						if (clrPtr)
						{
							gGLFT->glDisableClientState(MGL_COLOR_ARRAY);
						}	

						if (active)
						{
							gGLFT->glColor3f( 1.0f, 1.0f, 1.0f );
							gGLFT->glPolygonMode(MGL_FRONT_AND_BACK, MGL_LINE);
							gGLFT->glDrawElements ( MGL_TRIANGLES, numElem, MGL_UNSIGNED_INT, idx );		
							gGLFT->glPolygonMode(MGL_FRONT_AND_BACK, MGL_FILL);
						}
						#else
						if (active)
						{
							glEnable(GL_POLYGON_OFFSET_FILL);
							glPolygonOffset( 0.95f, 1.0f ); 
						}
						glDrawElements ( GL_TRIANGLES, numElem, GL_UNSIGNED_INT, idx );		
						if (active)
							glDisable(GL_POLYGON_OFFSET_FILL);

						if (normPtr)
							glDisableClientState( GL_NORMAL_ARRAY );
						if (boundTexture)
						{
							glDisableClientState( GL_TEXTURE_COORD_ARRAY );
							glDisable(GL_TEXTURE_2D);
							glBindTexture(GL_TEXTURE_2D, 0);
						}
						if (clrPtr)
						{
							glDisableClientState(GL_COLOR_ARRAY);
						}	

						if (active)
						{
							::glColor3f( 1.0f, 1.0f, 1.0f );
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
							glDrawElements ( GL_TRIANGLES, numElem, GL_UNSIGNED_INT, idx );		
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}
						#endif
					}


					// Reset state, and routing.
					#if defined(_USE_MGL_FT_)
					gGLFT->glDisableClientState( MGL_VERTEX_ARRAY );
					gGLFT->glPopMatrix();
					#else
					glDisableClientState( GL_VERTEX_ARRAY );
					::glPopMatrix();
					#endif
				}
			}
		}
	}
	return drewSurface;
}

bool
OpenGLViewportRenderer::drawBounds( const MDagPath &dagPath,
								    const MBoundingBox &box)
{
	MMatrix  matrix = dagPath.inclusiveMatrix();
	{
		MPoint	minPt = box.min();
		MPoint	maxPt = box.max();

		double bottomLeftFront[3] =		{ minPt.x, minPt.y, minPt.z };
		double topLeftFront[3] = 		{ minPt.x, maxPt.y, minPt.z };
		double bottomRightFront[3] =	{ maxPt.x, minPt.y, minPt.z };
		double topRightFront[3] =		{ maxPt.x, maxPt.y, minPt.z };
		double bottomLeftBack[3] =		{ minPt.x, minPt.y, maxPt.z };
		double topLeftBack[3] =			{ minPt.x, maxPt.y, maxPt.z };
		double bottomRightBack[3] =		{ maxPt.x, minPt.y, maxPt.z };
		double topRightBack[3] =		{ maxPt.x, maxPt.y, maxPt.z };

		#if defined(_USE_MGL_FT_)
		gGLFT->glMatrixMode( MGL_MODELVIEW );
		gGLFT->glPushMatrix();
		gGLFT->glMultMatrixd( &(matrix.matrix[0][0]) );

		gGLFT->glBegin(MGL_LINE_STRIP);
		gGLFT->glVertex3dv( bottomLeftFront );
		gGLFT->glVertex3dv( bottomLeftBack );
		gGLFT->glVertex3dv( topLeftBack );
		gGLFT->glVertex3dv( topLeftFront );
		gGLFT->glVertex3dv( bottomLeftFront );
		gGLFT->glVertex3dv( bottomRightFront );
		gGLFT->glVertex3dv( bottomRightBack);
		gGLFT->glVertex3dv( topRightBack );
		gGLFT->glVertex3dv( topRightFront );
		gGLFT->glVertex3dv( bottomRightFront );
		gGLFT->glEnd();

		gGLFT->glBegin(MGL_LINES);
		gGLFT->glVertex3dv(bottomLeftBack);
		gGLFT->glVertex3dv(bottomRightBack);

		gGLFT->glVertex3dv(topLeftBack);
		gGLFT->glVertex3dv(topRightBack);

		gGLFT->glVertex3dv(topLeftFront);
		gGLFT->glVertex3dv(topRightFront);
		gGLFT->glEnd();	

		gGLFT->glPopMatrix();
		#else
		::glMatrixMode( GL_MODELVIEW );
		::glPushMatrix();
		::glMultMatrixd( &(matrix.matrix[0][0]) );

		::glBegin(GL_LINE_STRIP);
		::glVertex3dv( bottomLeftFront );
		::glVertex3dv( bottomLeftBack );
		::glVertex3dv( topLeftBack );
		::glVertex3dv( topLeftFront );
		::glVertex3dv( bottomLeftFront );
		::glVertex3dv( bottomRightFront );
		::glVertex3dv( bottomRightBack);
		::glVertex3dv( topRightBack );
		::glVertex3dv( topRightFront );
		::glVertex3dv( bottomRightFront );
		::glEnd();

		::glBegin(GL_LINES);
		::glVertex3dv(bottomLeftBack);
		::glVertex3dv(bottomRightBack);

		::glVertex3dv(topLeftBack);
		::glVertex3dv(topRightBack);

		::glVertex3dv(topLeftFront);
		::glVertex3dv(topRightFront);
		::glEnd();	

		::glPopMatrix();
		#endif
	}
	return true;
}

bool
OpenGLViewportRenderer::setupLighting()
{
	//glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHTING);
	return false;
}

// Comment out #define to see how pruning works in this example
//
// #define _DEBUG_TRAVERSAL_PRUNING

// Use a custom traverser class.
class MsurfaceDrawTraversal : public MDrawTraversal
{
	virtual bool		filterNode( const MDagPath &traversalItem )
	{
		bool prune = false;

		// Check to only prune shapes, not transforms.
		//
#if defined(_DEBUG_TRAVERSAL_PRUNING)
		MString pname = traversalItem.fullPathName() ;
#endif
		if ( traversalItem.childCount() == 0)
		{
			if ( !traversalItem.hasFn( MFn::kMesh) &&
				!traversalItem.hasFn( MFn::kNurbsSurface) &&
				!traversalItem.hasFn( MFn::kSubdiv) &&
				!traversalItem.hasFn( MFn::kSketchPlane ) &&
				!traversalItem.hasFn( MFn::kGroundPlane )
				)
			{
#if defined(_DEBUG_TRAVERSAL_PRUNING)
				printf("Prune path [ %s ]\n", pname.asChar());
#endif
				prune = true;
			}
		}
		else
		{
#if defined(_DEBUG_TRAVERSAL_PRUNING)
			printf("Don't prune path [ %s ]\n", pname.asChar());
#endif
		}
		return prune;
	}

};

bool OpenGLViewportRenderer::renderToTarget( const MRenderingInfo &renderInfo )
//
// Description:
//		Render directly to current OpenGL target.
//
{

	const MRenderTarget &renderTarget = renderInfo.renderTarget();

	// Draw the world space axis 
	//
	#if defined(_USE_MGL_FT_)
	gGLFT->glDisable(MGL_LIGHTING);
	gGLFT->glBegin(MGL_LINES);
	gGLFT->glColor3f( 1.0f, 0.0f, 0.0f );
	gGLFT->glVertex3f( 0.0f, 0.0f, 0.0f );
	gGLFT->glVertex3f( 3.0f, 0.0f, 0.0f );

	gGLFT->glColor3f( 0.0f, 1.0f, 0.0f );
	gGLFT->glVertex3f( 0.0f, 0.0f, 0.0f );
	gGLFT->glVertex3f( 0.0f, 3.0f, 0.0f );

	gGLFT->glColor3f( 0.0f, 0.0f, 1.0f );
	gGLFT->glVertex3f( 0.0f, 0.0f, 0.0f );
	gGLFT->glVertex3f( 0.0f, 0.0f, 3.0f );
	gGLFT->glEnd();	
	gGLFT->glEnable(MGL_DEPTH_TEST);
	#else
	::glDisable(GL_LIGHTING);
	::glBegin(GL_LINES);
	::glColor3f( 1.0f, 0.0f, 0.0f );
	::glVertex3f( 0.0f, 0.0f, 0.0f );
	::glVertex3f( 3.0f, 0.0f, 0.0f );

	::glColor3f( 0.0f, 1.0f, 0.0f );
	::glVertex3f( 0.0f, 0.0f, 0.0f );
	::glVertex3f( 0.0f, 3.0f, 0.0f );

	::glColor3f( 0.0f, 0.0f, 1.0f );
	::glVertex3f( 0.0f, 0.0f, 0.0f );
	::glVertex3f( 0.0f, 0.0f, 3.0f );
	::glEnd();	
	glEnable(GL_DEPTH_TEST);
	#endif


	// Draw some surfaces...
	//
	bool useDrawTraversal = true;
	if (useDrawTraversal)
	{
#if defined(_DEBUG_TRAVERSAL_PRUNING)
		printf("==========================\n");
#endif
		const MDagPath &cameraPath = renderInfo.cameraPath();
		if (cameraPath.isValid())
		{
			bool pruneDuringTraversal = true;

			// You can actually keep the traverser classes around
			// if desired. Here we just create temporary traversers
			// on the fly.
			//
			MDrawTraversal *trav = NULL;
			if (pruneDuringTraversal)
			{
				trav = new MsurfaceDrawTraversal;
				trav->enableFiltering( true );
			}
			else
			{
				trav = new MDrawTraversal;
				trav->enableFiltering( false );
			}
			if (!trav)
			{
				printf("Warning: failed to create a traversal class !\n");
				return true;
			}

			trav->setFrustum( cameraPath, renderTarget.width(), 
				renderTarget.height() );

			if (!trav->frustumValid())
				printf("Warning : Frustum is invalid !\n");

			trav->traverse();

			unsigned int numItems = trav->numberOfItems();
#if defined(_DEBUG_TRAVERSAL_PRUNING)
			printf("There are %d items on the traversal list\n", numItems );
#endif
			if (numItems)
			{
				setupLighting();
			}

			unsigned int i;
			for (i=0; i<numItems; i++)
			{
				MDagPath path;
				trav->itemPath(i, path);

				if (path.isValid())
				{
					bool drawIt = false;

					// Default traverer may have view manips showing up.
					// This is currently a known Maya bug.
					if (!pruneDuringTraversal)
						if ( path.hasFn( MFn::kViewManip ))
							continue;

#if defined(_DEBUG_TRAVERSAL_PRUNING)
					MString pname = path.fullPathName() ;					
					printf("Draw path [%d][ %s ]\n", i, pname.asChar());
#endif

					//
					// Draw surfaces (polys, nurbs, subdivs)
					//
					bool active = false;
					bool templated = false;
					if ( path.hasFn( MFn::kMesh) || 
						 path.hasFn( MFn::kNurbsSurface) || 
						 path.hasFn( MFn::kSubdiv) )
					{
						drawIt = true;
						if (trav->itemHasStatus( i, MDrawTraversal::kActiveItem ))
						{
							active = true;
							#if defined(_USE_MGL_FT_)
							gGLFT->glColor3f( 1.0f, 1.0f, 1.0f );
							#else
							::glColor3f( 1.0f, 1.0f, 1.0f );
							#endif
						}
						else if (trav->itemHasStatus( i, MDrawTraversal::kTemplateItem ))
						{
							#if defined(_USE_MGL_FT_)
							gGLFT->glColor3f( 0.2f, 0.2f, 0.2f );
							#else
							::glColor3f( 0.2f, 0.2f, 0.2f );
							#endif
							templated = true;
						}
						else
						{
							#if defined(_USE_MGL_FT_)
							if (path.hasFn( MFn::kMesh ))
								gGLFT->glColor3f( 0.286f, 0.706f, 1.0f );
							else if (path.hasFn( MFn::kNurbsSurface))
								gGLFT->glColor3f( 0.486f, 0.306f, 1.0f );
							else
								gGLFT->glColor3f( 0.886f, 0.206f, 1.0f );
							#else
							if (path.hasFn( MFn::kMesh ))
								::glColor3f( 0.286f, 0.706f, 1.0f );
							else if (path.hasFn( MFn::kNurbsSurface))
								::glColor3f( 0.486f, 0.306f, 1.0f );
							else
								::glColor3f( 0.886f, 0.206f, 1.0f );
							#endif
						}
					}

					//
					// Draw the ground plane
					//
					else if (path.hasFn( MFn::kSketchPlane ) ||
							 path.hasFn( MFn::kGroundPlane ))
					{
						drawIt = true;
						#if defined(_USE_MGL_FT_)
						gGLFT->glColor3f( 0.0f, 1.0f, 0.0f );
						#else
						::glColor3f( 0.0f, 1.0f, 0.0f );
						#endif
					}

					if (drawIt)
					{
						MFnDagNode dagNode(path);
						if (!drawSurface( path, active, templated ))
						{
							MBoundingBox box = dagNode.boundingBox();
							drawBounds( path, box );
						}
					}
				}
			}

			if (trav)
				delete trav;
		}
	}
	// Draw everything in the scene, without any culling or visibility
	// testing. This is just comparison code, and should never be
	// enabled, unless you want to do post-filtering...
	else 
	{
		MItDag::TraversalType traversalType = MItDag::kDepthFirst;
		MFn::Type filter = MFn::kMesh;
		MStatus status;

		MItDag dagIterator( traversalType, filter, &status);

		for ( ; !dagIterator.isDone(); dagIterator.next() ) 
		{

			MDagPath dagPath;

			status = dagIterator.getPath(dagPath);
			if ( !status ) {
				status.perror("MItDag::getPath");
				continue;
			}

			MFnDagNode dagNode(dagPath, &status);
			if ( !status ) {
				status.perror("MFnDagNode constructor");
				continue;
			}

			MBoundingBox box = dagNode.boundingBox();
			#if defined(_USE_MGL_FT_)
			gGLFT->glColor3f( 1.0f, 0.0f, 0.0f );
			#else
			::glColor3f( 1.0f, 0.0f, 0.0f );
			#endif
			drawBounds( dagPath, box );
		}
	}

	return true;
}

