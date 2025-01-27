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

///////////////////////////////////////////////////////////////////////////////
//
// apiSimpleShapeUI.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h> 

#include <apiSimpleShapeUI.h>

#include <maya/MMaterial.h>
#include <maya/MSelectionList.h>
#include <maya/MSelectionMask.h>
#include <maya/MDrawData.h>
#include <maya/MMatrix.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MObjectArray.h>
#include <maya/MDagPath.h>

// Object and component color defines
//
#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue
#define DORMANT_VERTEX_COLOR	8	// purple
#define ACTIVE_VERTEX_COLOR		16	// yellow

// Vertex point size
//
#define POINT_SIZE				2.0	

////////////////////////////////////////////////////////////////////////////////
//
// UI implementation
//
////////////////////////////////////////////////////////////////////////////////

apiSimpleShapeUI::apiSimpleShapeUI() {}
apiSimpleShapeUI::~apiSimpleShapeUI() {}

void* apiSimpleShapeUI::creator()
{
	return new apiSimpleShapeUI();
}

///////////////////////////////////////////////////////////////////////////////
//
// Overrides
//
///////////////////////////////////////////////////////////////////////////////

/* override */
void apiSimpleShapeUI::getDrawRequests( const MDrawInfo & info,
							 bool objectAndActiveOnly,
							 MDrawRequestQueue & queue )
//
// Description:
//
//     Add draw requests to the draw queue
//
// Arguments:
//
//     info                 - current drawing state
//     objectsAndActiveOnly - no components if true
//     queue                - queue of draw requests to add to
//
{
	// Get the data necessary to draw the shape
	//
	MDrawData data;
	apiSimpleShape* shape = (apiSimpleShape*) surfaceShape();
	MVectorArray* geomPtr = shape->getControlPoints();

	// This call creates a prototype draw request that we can fill
	// in and then add to the draw queue.
	//
	MDrawRequest request = info.getPrototype( *this );

	// Stuff our data into the draw request, it'll be used when the drawing
	// actually happens
	getDrawData( geomPtr, data );

	request.setDrawData( data );


	// Decode the draw info and determine what needs to be drawn
	//

	M3dView::DisplayStyle  appearance    = info.displayStyle();
	M3dView::DisplayStatus displayStatus = info.displayStatus();

	switch ( appearance )
	{
		case M3dView::kWireFrame :
		{
			request.setToken( kDrawWireframe );

			M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
			M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;

			switch ( displayStatus )
			{
				case M3dView::kLead :
					request.setColor( LEAD_COLOR, activeColorTable );
					break;
				case M3dView::kActive :
					request.setColor( ACTIVE_COLOR, activeColorTable );
					break;
				case M3dView::kActiveAffected :
					request.setColor( ACTIVE_AFFECTED_COLOR, activeColorTable );
					break;
				case M3dView::kDormant :
					request.setColor( DORMANT_COLOR, dormantColorTable );
					break;
				case M3dView::kHilite :
					request.setColor( HILITE_COLOR, activeColorTable );
					break;

				default:
						break;
			}

			queue.add( request );

			break;
		}

		case M3dView::kGouraudShaded :
		{
			// Create the smooth shaded draw request
			//
			request.setToken( kDrawSmoothShaded );

			// Need to get the material info
			//
			MDagPath path = info.multiPath();	// path to your dag object 
			M3dView view = info.view();; 		// view to draw to
			MMaterial material = MPxSurfaceShapeUI::material( path );

			// Evaluate the material and if necessary, the texture.
			//
			if ( ! material.evaluateMaterial( view, path ) ) {
				cerr << "Couldnt evaluate\n";
			}

			bool drawTexture = true;
			if ( drawTexture && material.materialIsTextured() ) {
				material.evaluateTexture( data );
			}

			request.setMaterial( material );

			// request.setDisplayStyle( appearance );

			bool materialTransparent = false;
			material.getHasTransparency( materialTransparent );
			if ( materialTransparent ) {
				request.setIsTransparent( true );
			}

			queue.add( request );

			// create a draw request for wireframe on shaded if
			// necessary.
			//
			if ( (displayStatus == M3dView::kActive) ||
				 (displayStatus == M3dView::kLead) ||
				 (displayStatus == M3dView::kHilite) )
			{
				MDrawRequest wireRequest = info.getPrototype( *this );
				wireRequest.setDrawData( data );
				wireRequest.setToken( kDrawWireframeOnShaded );
				wireRequest.setDisplayStyle( M3dView::kWireFrame );

				M3dView::ColorTable activeColorTable = M3dView::kActiveColors;

				switch ( displayStatus )
				{
					case M3dView::kLead :
						wireRequest.setColor( LEAD_COLOR, activeColorTable );
						break;
					case M3dView::kActive :
						wireRequest.setColor( ACTIVE_COLOR, activeColorTable );
						break;
					case M3dView::kHilite :
						wireRequest.setColor( HILITE_COLOR, activeColorTable );
						break;

					default :	
						break;
				}

				queue.add( wireRequest );
			}

			break;
		}

		case M3dView::kFlatShaded :
			request.setToken( kDrawFlatShaded );
			break;

		default:
			break;

	}

	// Add draw requests for components
	//
	if ( !objectAndActiveOnly ) {

		// Inactive components
		//
		if ( (appearance == M3dView::kPoints) ||
			 (displayStatus == M3dView::kHilite) )
		{
			MDrawRequest vertexRequest = info.getPrototype( *this ); 
			vertexRequest.setDrawData( data );
			vertexRequest.setToken( kDrawVertices );
			vertexRequest.setColor( DORMANT_VERTEX_COLOR,
									M3dView::kActiveColors );

			queue.add( vertexRequest );
		}

		// Active components
		//
		if ( surfaceShape()->hasActiveComponents() ) {

			MDrawRequest activeVertexRequest = info.getPrototype( *this ); 
			activeVertexRequest.setDrawData( data );
		    activeVertexRequest.setToken( kDrawVertices );
		    activeVertexRequest.setColor( ACTIVE_VERTEX_COLOR,
										  M3dView::kActiveColors );

		    MObjectArray clist = surfaceShape()->activeComponents();
		    MObject vertexComponent = clist[0]; // Should filter list
		    activeVertexRequest.setComponent( vertexComponent );

			queue.add( activeVertexRequest );
		}
	}
}

/* override */
void apiSimpleShapeUI::draw( const MDrawRequest & request, M3dView & view ) const
//
// Description:
//
//     Main (OpenGL) draw routine
//
// Arguments:
//
//     request - request to be drawn
//     view    - view to draw into
//
{ 
	// Get the token from the draw request.
	// The token specifies what needs to be drawn.
	//
	int token = request.token();
	switch( token )
	{
		case kDrawWireframe :
		case kDrawWireframeOnShaded :
		case kDrawVertices :
			drawVertices( request, view );
			break;

		case kDrawSmoothShaded :
			break;			// Not implemented, left as exercise

		case kDrawFlatShaded : // Not implemented, left as exercise
			break;
	}
}

/* override */
bool apiSimpleShapeUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList,
					MPointArray &worldSpaceSelectPts ) const
//
// Description:
//
//     Main selection routine
//
// Arguments:
//
//     selectInfo           - the selection state information
//     selectionList        - the list of selected items to add to
//     worldSpaceSelectPts  -
//
{
	bool selected = false;
	bool componentSelected = false;
	bool hilited = false;

	hilited = (selectInfo.displayStatus() == M3dView::kHilite);
	if ( hilited ) {
		componentSelected = selectVertices( selectInfo, selectionList, worldSpaceSelectPts );
		selected = selected || componentSelected;
	}

	if ( !selected ) 
	{
		// NOTE: If the geometry has an intersect routine it should
		// be called here with the selection ray to determine if the
		// the object was selected.

		selected = true;
		MSelectionMask priorityMask( MSelectionMask::kSelectNurbsSurfaces );
		MSelectionList item;
		item.add( selectInfo.selectPath() );
		MPoint xformedPt;
		if ( selectInfo.singleSelection() ) {
			MPoint center = surfaceShape()->boundingBox().center();
			xformedPt = center;
			xformedPt *= selectInfo.selectPath().inclusiveMatrix();
		}

		selectInfo.addSelection( item, xformedPt, selectionList,
								 worldSpaceSelectPts, priorityMask, false );
	}

	return selected;
}

///////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
///////////////////////////////////////////////////////////////////////////////


void apiSimpleShapeUI::drawVertices( const MDrawRequest & request,
							  M3dView & view ) const
//
// Description:
//
//     Component (vertex) drawing routine
//
// Arguments:
//
//     request - request to be drawn
//     view    - view to draw into
//
{
	MDrawData data = request.drawData();
	MVectorArray * geom = (MVectorArray*)data.geometry();

	view.beginGL(); 

	// Query current state so it can be restored
	//
	bool lightingWasOn = glIsEnabled( GL_LIGHTING ) ? true : false;
	if ( lightingWasOn ) {
		glDisable( GL_LIGHTING );
	}
	float lastPointSize;
	glGetFloatv( GL_POINT_SIZE, &lastPointSize );

	// Set the point size of the vertices
	//
	glPointSize( POINT_SIZE );

	// If there is a component specified by the draw request
	// then loop over comp (using an MFnComponent class) and draw the
	// active vertices, otherwise draw all vertices.
	//
	MObject comp = request.component();
	if ( ! comp.isNull() ) {
		MFnSingleIndexedComponent fnComponent( comp );
		for ( int i=0; i<fnComponent.elementCount(); i++ )
		{
			int index = fnComponent.element( i );
			glBegin( GL_POINTS );
			MVector& point = (*geom)[index];
			glVertex3f( (float)point[0], 
						(float)point[1], 
						(float)point[2] );
			glEnd();

			char annotation[32];
			sprintf( annotation, "%d", index );
			view.drawText( annotation, point );
		}
	}
	else {
		for ( unsigned int i=0; i<geom->length(); i++ )
		{
			glBegin( GL_POINTS );
			MVector point = (*geom)[ i ];
			glVertex3f( (float)point[0], (float)point[1], (float)point[2] );
			glEnd();
		}
	}

	// Restore the state
	//
	if ( lightingWasOn ) {
		glEnable( GL_LIGHTING );
	}
	glPointSize( lastPointSize );

	view.endGL(); 
}

bool apiSimpleShapeUI::selectVertices( MSelectInfo &selectInfo,
							 MSelectionList &selectionList,
							 MPointArray &worldSpaceSelectPts ) const
//
// Description:
//
//     Vertex selection.
//
// Arguments:
//
//     selectInfo           - the selection state information
//     selectionList        - the list of selected items to add to
//     worldSpaceSelectPts  -
//
{
	bool selected = false;
	M3dView view = selectInfo.view();

	MPoint 		xformedPoint;
	MPoint 		currentPoint;
	MPoint 		selectionPoint;
	double		z,previousZ = 0.0;
 	int			closestPointVertexIndex = -1;

	const MDagPath & path = selectInfo.multiPath();

	// Create a component that will store the selected vertices
	//
	MFnSingleIndexedComponent fnComponent;
	MObject surfaceComponent = fnComponent.create( MFn::kMeshVertComponent );
	int vertexIndex;

	// if the user did a single mouse click and we find > 1 selection
	// we will use the alignmentMatrix to find out which is the closest
	//
	MMatrix	alignmentMatrix;
	MPoint singlePoint; 
	bool singleSelection = selectInfo.singleSelection();
	if( singleSelection ) {
		alignmentMatrix = selectInfo.getAlignmentMatrix();
	}

	// Get the geometry information
	//
	apiSimpleShape* shape = (apiSimpleShape*) surfaceShape();
	MVectorArray* geomPtr = shape->getControlPoints();
	MVectorArray& geom = *geomPtr;


	// Loop through all vertices of the mesh and
	// see if they lie withing the selection area
	//
	int numVertices = geom.length();
	for ( vertexIndex=0; vertexIndex<numVertices; vertexIndex++ )
	{
		const MVector& point = geom[ vertexIndex ];

		// Sets OpenGL's render mode to select and stores
		// selected items in a pick buffer
		//
		view.beginSelect();

		glBegin( GL_POINTS );
		glVertex3f( (float)point[0], 
					(float)point[1], 
					(float)point[2] );
		glEnd();

		if ( view.endSelect() > 0 )	// Hit count > 0
		{
			selected = true;

			if ( singleSelection ) {
				xformedPoint = currentPoint;
				xformedPoint.homogenize();
				xformedPoint*= alignmentMatrix;
				z = xformedPoint.z;
				if ( closestPointVertexIndex < 0 || z > previousZ ) {
					closestPointVertexIndex = vertexIndex;
					singlePoint = currentPoint;
					previousZ = z;
				}
			} else {
				// multiple selection, store all elements
				//
				fnComponent.addElement( vertexIndex );
			}
		}
	}

	// If single selection, insert the closest point into the array
	//
	if ( selected && selectInfo.singleSelection() ) {
		fnComponent.addElement(closestPointVertexIndex);

		// need to get world space position for this vertex
		//
		selectionPoint = singlePoint;
		selectionPoint *= path.inclusiveMatrix();
	}

	// Add the selected component to the selection list
	//
	if ( selected ) {
		MSelectionList selectionItem;
		selectionItem.add( path, surfaceComponent );

		MSelectionMask mask( MSelectionMask::kSelectComponentsMask );
		selectInfo.addSelection(
			selectionItem, selectionPoint,
			selectionList, worldSpaceSelectPts,
			mask, true );
	}

	return selected;
}

