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
// apiMeshShape.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include <math.h>           
#include <maya/MIOStream.h>           

#include <apiMeshShape.h>           
#include <apiMeshShapeUI.h>           
#include <apiMeshCreator.h>           
#include <apiMeshData.h>
#include <api_macros.h>           

#include <maya/MFnPlugin.h>
#include <maya/MFnPluginData.h>
#include <maya/MMatrix.h>
#include <maya/MAttributeSpecArray.h>
#include <maya/MAttributeSpec.h>
#include <maya/MAttributeIndex.h>
#include <maya/MObjectArray.h>           
#include <maya/MFnSingleIndexedComponent.h>           
#include <maya/MDagPath.h>           
#include <maya/MFnAttribute.h>           
#include <maya/MFnNumericAttribute.h>           
#include <maya/MFnTypedAttribute.h>
#include <maya/MPointArray.h>   

bool debug = false;

////////////////////////////////////////////////////////////////////////////////
//
// Shape implementation
//
////////////////////////////////////////////////////////////////////////////////

MObject apiMesh::inputSurface;
MObject apiMesh::outputSurface;
MObject apiMesh::cachedSurface;
MObject apiMesh::worldSurface;
MObject apiMesh::bboxCorner1;
MObject apiMesh::bboxCorner2;

MTypeId apiMesh::id( 0x80099 );

apiMesh::apiMesh() {}
apiMesh::~apiMesh() {}

///////////////////////////////////////////////////////////////////////////////
//
// Overrides
//
///////////////////////////////////////////////////////////////////////////////

/* override */
void apiMesh::postConstructor()
//
// Description
// 
//    When instances of this node are created internally, the MObject associated
//    with the instance is not created until after the constructor of this class
//    is called. This means that no member functions of MPxSurfaceShape can
//    be called in the constructor.
//    The postConstructor solves this problem. Maya will call this function
//    after the internal object has been created.
//    As a general rule do all of your initialization in the postConstructor.
//
{ 
	// This call allows the shape to have shading groups assigned
	//
	setRenderable( true );

	// Is there input history to this node
	//
	fHasHistoryOnCreate = false;
}

/* override */
MStatus apiMesh::compute( const MPlug& plug, MDataBlock& datablock )
//
// Description
//
//    When input attributes are dirty this method will be called to
//    recompute the output attributes.
//
// Arguments
//
//    plug      - the attribute that triggered the compute
//    datablock - the nodes data
//
// Returns
//
//    kSuccess          - this method could compute the dirty attribute,
//    kUnknownParameter - the dirty attribute can not be handled at this level
//
{ 
	if (debug)
		cerr << "apiMesh::compute : plug " << plug.info() << endl;

	if ( plug == outputSurface ) {
		return computeOutputSurface( plug, datablock );
	}
	else if ( plug == cachedSurface ) {
		return computeOutputSurface( plug, datablock );
	}
	else if ( plug == worldSurface ) {
		return computeWorldSurface( plug, datablock );
    }
    else {
        return MS::kUnknownParameter;
    }
}

/* override */
//
// Description
//
//    Handle internal attributes.
//
//    Attributes that require special storage, bounds checking,
//    or other non-standard behavior can be marked as "Internal" by
//    using the "MFnAttribute::setInternal" method.
//
//    The get/setInternalValue methods will get called for internal
//    attributes whenever the attribute values are stored or retrieved
//    using getAttr/setAttr or MPlug getValue/setValue.
//  
//    The inherited attribute mControlPoints is internal and we want
//    its values to get stored only if there is input history. Otherwise
//    any changes to the vertices are stored in the cachedMesh and outputMesh
//    directly.
//
//    If values are retrieved then we want the controlPoints value
//    returned if there is history, this will be the offset or tweak.
//    In the case of no history, the vertex position of the cached mesh
//    is returned.
//
bool apiMesh::getInternalValue( const MPlug& plug, MDataHandle& result )
{
	bool isOk = true;

	if( (plug == mControlPoints) ||
		(plug == mControlValueX) ||
		(plug == mControlValueY) ||
		(plug == mControlValueZ) )
	{
		// If there is input history then the control point value is
		// directly returned. This is the tweak or offset that
		// was applied to the vertex.
		//
		// If there is no input history then return the actual vertex
		// position and ignore the controlPoints attribute.
		//
		if ( hasHistory() )	{
			return MPxNode::getInternalValue( plug, result );
		}
		else {
			double val = 0.0;
			if ( (plug == mControlPoints) && !plug.isArray() ) {
				MPoint pnt;
				int index = plug.logicalIndex();
				value( index, pnt );
				result.set( pnt[0], pnt[1], pnt[2] );
			}
			else if ( plug == mControlValueX ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				value( index, 0, val );
				result.set( val );
			}
			else if ( plug == mControlValueY ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				value( index, 1, val );
				result.set( val );
			}
			else if ( plug == mControlValueZ ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				value( index, 2, val );
				result.set( val );
			}
		}
	}
	// This inherited attribute is used to specify whether or
	// not this shape has history. During a file read, the shape
	// is created before any input history can get connected.
	// This attribute, also called "tweaks", provides a way to
	// for the shape to determine if there is input history
	// during file reads.
	//
	else if ( plug == mHasHistoryOnCreate ) {
		result.set( fHasHistoryOnCreate );
	}
	else {
		isOk = MPxSurfaceShape::getInternalValue( plug, result );
	}

	return isOk;
}

/* override */
//
// Description
//
//    Handle internal attributes.
//
//    Attributes that require special storage, bounds checking,
//    or other non-standard behavior can be marked as "Internal" by
//    using the "MFnAttribute::setInternal" method.
//
//    The get/setInternalValue methods will get called for internal
//    attributes whenever the attribute values are stored or retrieved
//    using getAttr/setAttr or MPlug getValue/setValue.
//  
//    The inherited attribute mControlPoints is internal and we want
//    its values to get stored only if there is input history. Otherwise
//    any changes to the vertices are stored in the cachedMesh and outputMesh
//    directly.
//
//    If values are retrieved then we want the controlPoints value
//    returned if there is history, this will be the offset or tweak.
//    In the case of no history, the vertex position of the cached mesh
//    is returned.
//
bool apiMesh::setInternalValue( const MPlug& plug, const MDataHandle& handle )
{
	bool isOk = true;

	if( (plug == mControlPoints) ||
		(plug == mControlValueX) ||
		(plug == mControlValueY) ||
		(plug == mControlValueZ) )
	{
		// If there is input history then set the control points value
		// using the normal mechanism. In this case we are setting
		// the tweak or offset that will get applied to the input
		// history.
		//
		// If there is no input history then ignore the controlPoints
		// attribute and set the vertex position directly in the
		// cachedMesh.
		//
		if ( hasHistory() )	{
			verticesUpdated();
			return MPxNode::setInternalValue( plug, handle );
		}
		else {
			if( plug == mControlPoints && !plug.isArray()) {
				int index = plug.logicalIndex();
				MPoint point;
				double3& ptData = handle.asDouble3();
				point.x = ptData[0];
				point.y = ptData[1];
				point.z = ptData[2];
				setValue( index, point );
			}
			else if( plug == mControlValueX ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				setValue( index, 0, handle.asDouble() );
			}
			else if( plug == mControlValueY ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				setValue( index, 1, handle.asDouble() );
			}
			else if( plug == mControlValueZ ) {
				MPlug parentPlug = plug.parent();
				int index = parentPlug.logicalIndex();
				setValue( index, 2, handle.asDouble() );
			}
		}
	}
	// This inherited attribute is used to specify whether or
	// not this shape has history. During a file read, the shape
	// is created before any input history can get connected.
	// This attribute, also called "tweaks", provides a way to
	// for the shape to determine if there is input history
	// during file reads.
	//
	else if ( plug == mHasHistoryOnCreate ) {
		fHasHistoryOnCreate = handle.asBool();
	}
	else {
		isOk = MPxSurfaceShape::setInternalValue( plug, handle );
	}

	return isOk;
}

/* override */
MStatus apiMesh::connectionMade( const MPlug& plug,
								 const MPlug& otherPlug,
								 bool asSrc )
//
// Description
//
//    Whenever a connection is made to this node, this method
//    will get called.
//
{
	if ( plug == inputSurface ) {
		MStatus stat;
		MObject thisObj = thisMObject();
		MPlug historyPlug( thisObj, mHasHistoryOnCreate );
		stat = historyPlug.setValue( true );
		MCHECKERROR( stat, "connectionMade: setValue(mHasHistoryOnCreate)" );
	}

	return MPxNode::connectionMade( plug, otherPlug, asSrc );
}

/* override */
MStatus apiMesh::connectionBroken( const MPlug& plug,
								   const MPlug& otherPlug,
								   bool asSrc )
//
// Description
//
//    Whenever a connection to this node is broken, this method
//    will get called.
//
{
	if ( plug == inputSurface ) {
		MStatus stat;
		MObject thisObj = thisMObject();
		MPlug historyPlug( thisObj, mHasHistoryOnCreate );
		stat = historyPlug.setValue( false );
		MCHECKERROR( stat, "connectionBroken: setValue(mHasHistoryOnCreate)" );
	}
	
	return MPxNode::connectionBroken( plug, otherPlug, asSrc );
}

/* override */
MStatus apiMesh::shouldSave( const MPlug& plug, bool& result )
//
// Description
//
//    During file save this method is called to determine which
//    attributes of this node should get written. The default behavior
//    is to only save attributes whose values differ from the default.
//
//
//
{
	MStatus status = MS::kSuccess;

	if( plug == mControlPoints || plug == mControlValueX ||
		plug == mControlValueY || plug == mControlValueZ )
	{
		if( hasHistory() ) {
			// Calling this will only write tweaks if they are
			// different than the default value.
			//
			status = MPxNode::shouldSave( plug, result );
		}
		else {
			result = false;
		}
	}
	else if ( plug == cachedSurface ) {
		if ( hasHistory() ) {
			result = false;
		}
		else {
			MObject data;
			status = plug.getValue( data );
			MCHECKERROR( status, "shouldSave: MPlug::getValue" );
			result = ( ! data.isNull() );
		}
	}
	else {
		status = MPxNode::shouldSave( plug, result );
	}

	return status;
}

/* override */
void apiMesh::componentToPlugs( MObject & component,
								MSelectionList & list ) const
//
// Description
//
//    Converts the given component values into a selection list of plugs.
//    This method is used to map components to attributes.
//
// Arguments
//
//    component - the component to be translated to a plug/attribute
//    list      - a list of plugs representing the passed in component
//
{
	if ( component.hasFn(MFn::kSingleIndexedComponent) ) {

		MFnSingleIndexedComponent fnVtxComp( component );
    	MObject thisNode = thisMObject();
		MPlug plug( thisNode, mControlPoints );
		// If this node is connected to a tweak node, reset the
		// plug to point at the tweak node.
		//
		convertToTweakNodePlug(plug);

		int len = fnVtxComp.elementCount();

		for ( int i = 0; i < len; i++ )
		{
			plug.selectAncestorLogicalIndex(fnVtxComp.element(i),
											plug.attribute());
			list.add(plug);
		}
	}
}

/* override */
MPxSurfaceShape::MatchResult
apiMesh::matchComponent( const MSelectionList& item,
					  const MAttributeSpecArray& spec,
					  MSelectionList& list )
//
// Description:
//
//    Component/attribute matching method.
//    This method validates component names and indices which are
//    specified as a string and adds the corresponding component
//    to the passed in selection list.
//
//    For instance, select commands such as "select shape1.vtx[0:7]"
//    are validated with this method and the corresponding component
//    is added to the selection list.
//
// Arguments
//
//    item - DAG selection item for the object being matched
//    spec - attribute specification object
//    list - list to add components to
//
// Returns
//
//    the result of the match
//
{
	MPxSurfaceShape::MatchResult result = MPxSurfaceShape::kMatchOk;
	MAttributeSpec attrSpec = spec[0];
	int dim = attrSpec.dimensions();

	// Look for attributes specifications of the form :
	//     vtx[ index ]
	//     vtx[ lower:upper ]
	//
	if ( (1 == spec.length()) && (dim > 0) && (attrSpec.name() == "vtx") ) {
		int numVertices = meshGeom()->vertices.length();
		MAttributeIndex attrIndex = attrSpec[0];

		int upper = 0;
		int lower = 0;
		if ( attrIndex.hasLowerBound() ) {
			attrIndex.getLower( lower );
		}
		if ( attrIndex.hasUpperBound() ) {
			attrIndex.getUpper( upper );
		}

		// Check the attribute index range is valid
		//
		if ( (lower > upper) || (upper >= numVertices) ) {
			result = MPxSurfaceShape::kMatchInvalidAttributeRange;	
		}
		else {
			MDagPath path;
			item.getDagPath( 0, path );
			MFnSingleIndexedComponent fnVtxComp;
			MObject vtxComp = fnVtxComp.create( MFn::kMeshVertComponent );

			for ( int i=lower; i<=upper; i++ )
			{
				fnVtxComp.addElement( i );
			}
			list.add( path, vtxComp );
		}
	}
	else {
		// Pass this to the parent class
		return MPxSurfaceShape::matchComponent( item, spec, list );
	}

	return result;
}

/* override */
bool apiMesh::match( const MSelectionMask & mask,
			  		 const MObjectArray& componentList ) const
//
// Description:
//
//		Check for matches between selection type / component list, and
//		the type of this shape / or it's components
//
//      This is used by sets and deformers to make sure that the selected
//      components fall into the "vertex only" category.
//
// Arguments
//
//		mask          - selection type mask
//		componentList - possible component list
//
// Returns
//		true if matched any
//
{
	bool result = false;

	if( componentList.length() == 0 ) {
		result = mask.intersects( MSelectionMask::kSelectMeshes );
	}
	else {
		for ( int i=0; i<(int)componentList.length(); i++ ) {
			if ( (componentList[i].apiType() == MFn::kMeshVertComponent) &&
				 (mask.intersects(MSelectionMask::kSelectMeshVerts))
			) {
				result = true;
				break;
			}
		}
	}
	return result;
}

/* override */
MObject apiMesh::createFullVertexGroup() const
//
// Description
//     This method is used by maya when it needs to create a component
//     containing every vertex (or control point) in the shape.
//     This will get called if you apply some deformer to the whole
//     shape, i.e. select the shape in object mode and add a deformer to it.
//
// Returns
//
//    A "complete" component representing all vertices in the shape.
//
{
	// Create a vertex component
	//
	MFnSingleIndexedComponent fnComponent;
	MObject fullComponent = fnComponent.create( MFn::kMeshVertComponent );		

	// Set the component to be complete, i.e. the elements in
	// the component will be [0:numVertices-1]
	//
	int numVertices = ((apiMesh*)this)->meshGeom()->vertices.length();
	fnComponent.setCompleteData( numVertices );

	return fullComponent;
}

/* override */
MObject apiMesh::localShapeInAttr() const
//
// Description
//
//    Returns the input attribute of the shape. This is used by
//    maya to establish input connections for deformers etc.
//    This attribute must be data of tye kGeometryData.
//
// Returns
//
//    input attribute for the shape
//
{
	return inputSurface;
}

/* override */
MObject apiMesh::localShapeOutAttr() const
//
// Description
//
//    Returns the output attribute of the shape. This is used by
//    maya to establish out connections for deformers etc.
//    This attribute must be data of tye kGeometryData.
//
// Returns
//
//    output attribute for the shape
//
//
{
	return outputSurface;
}

/* override */
MObject apiMesh::worldShapeOutAttr() const
//
// Description
//
//    Returns the world space output "array" attribute of the shape.
//    This is used by maya to establish out connections for deformers etc.
//    This attribute must be an array attribute, each element representing
//    a particular instance of the shape.
//    This attribute must be data of type kGeometryData.
//
// Returns
//
//    world space "array" attribute for the shape
//
{
	return worldSurface;
}

/* override */
MObject apiMesh::cachedShapeAttr() const
//
// Description
//
//    Returns the cached shape attribute of the shape.
//    This attribute must be data of type kGeometryData.
//
// Returns
//
//    cached shape attribute
//
{
	return cachedSurface;
}



/* override */
MObject apiMesh::geometryData() const
//
// Description
//
//    Returns the data object for the surface. This gets
//    called internally for grouping (set) information.
//
{
	apiMesh* nonConstThis = (apiMesh*)this;
	MDataBlock datablock = nonConstThis->forceCache();
	MDataHandle handle = datablock.inputValue( inputSurface );
	return handle.data();
}

/*override */
void apiMesh:: closestPoint ( const MPoint & toThisPoint, \
				MPoint & theClosestPoint, double tolerance ) const
//
// Description
//
//		Returns the closest point to the given point in space. 
//		Used for rigid bind of skin.  Currently returns wrong results;
//		override it by implementing a closest point calculation.
{
	// Iterate through the geometry to find the closest point within 
	// the given tolerance.
	//
	apiMeshGeom* geomPtr = ((apiMesh*)this)->meshGeom();
	int numVertices = geomPtr->vertices.length();
	for (int ii=0; ii<numVertices; ii++)
	{
		MPoint tryThisOne = geomPtr->vertices[ii];
	}
	
	// Set the output point to the result (hardcode for debug just now)
	//
	theClosestPoint = geomPtr->vertices[0];
}

/* override */
void apiMesh::transformUsing( const MMatrix & mat,
							  const MObjectArray & componentList )
//
// Description
//
//    Transforms by the matrix the given components, or the entire shape
//    if the componentList is empty. This method is used by the freezeTransforms command.
//
// Arguments
//
//    mat           - matrix to tranform the components by
//    componentList - list of components to be transformed,
//                    or an empty list to indicate the whole surface 	
//
{
	// Let the other version of transformUsing do the work for us.
	//
	transformUsing( mat,
					componentList,
					MPxSurfaceShape::kNoPointCaching,
					NULL);
}


//
// Description
//
//    Transforms the given components. This method is used by
//    the move, rotate, and scale tools in component mode.
//    The bounding box has to be updated here, so do the normals and
//    any other attributes that depend on vertex positions.
//
// Arguments
//    mat           - matrix to tranform the components by
//    componentList - list of components to be transformed,
//                    or an empty list to indicate the whole surface 	
//    cachingMode   - how to use the supplied pointCache
//    pointCache    - if non-null, save or restore points from this list base
//					  on the cachingMode
//
void apiMesh::transformUsing( const MMatrix & mat,
							  const MObjectArray & componentList,
							  MVertexCachingMode cachingMode,
							  MPointArray* pointCache)
{

    MStatus stat;
	apiMeshGeom* geomPtr = meshGeom();

	bool savePoints    = (cachingMode == MPxSurfaceShape::kSavePoints);
	unsigned int i=0,j=0;
	unsigned int len = componentList.length();
	
	if (cachingMode == MPxSurfaceShape::kRestorePoints) {
		// restore the points based on the data provided in the pointCache attribute
		//
		unsigned int cacheLen = pointCache->length();
		if (len > 0) {
			// traverse the component list
			//
			for ( i = 0; i < len && j < cacheLen; i++ )
			{
				MObject comp = componentList[i];
				MFnSingleIndexedComponent fnComp( comp );
				int elemCount = fnComp.elementCount();
				for ( int idx=0; idx<elemCount && j < cacheLen; idx++, ++j ) {
					int elemIndex = fnComp.element( idx );
					geomPtr->vertices[elemIndex] = (*pointCache)[j];
				}
			}
		} else {
			// if the component list is of zero-length, it indicates that we
			// should transform the entire surface
			//
			len = geomPtr->vertices.length();
			for ( unsigned int idx = 0; idx < len && j < cacheLen; ++idx, ++j ) {
				geomPtr->vertices[idx] = (*pointCache)[j];
			}
		}
	} else {	
		// Transform the surface vertices with the matrix.
		// If savePoints is true, save the points to the pointCache.
		//
		if (len > 0) {
			// Traverse the componentList 
			//
			for ( i=0; i<len; i++ )
			{
				MObject comp = componentList[i];
				MFnSingleIndexedComponent fnComp( comp );
				int elemCount = fnComp.elementCount();

				if (savePoints && 0 == i) {
					pointCache->setSizeIncrement(elemCount);
				}
				for ( int idx=0; idx<elemCount; idx++ )
				{
					int elemIndex = fnComp.element( idx );
					if (savePoints) {
						pointCache->append(geomPtr->vertices[elemIndex]);
					}
					geomPtr->vertices[elemIndex] *= mat;
					geomPtr->normals[idx] =
						geomPtr->normals[idx].transformAsNormal( mat );
				}
			}
		} else {
			// If the component list is of zero-length, it indicates that we
			// should transform the entire surface
			//
			len = geomPtr->vertices.length();
			if (savePoints) {
				pointCache->setSizeIncrement(len);
			}
			for ( unsigned int idx = 0; idx < len; ++idx ) {
				if (savePoints) {
					pointCache->append(geomPtr->vertices[idx]);
				}
				geomPtr->vertices[idx] *= mat;
				geomPtr->normals[idx] =
					geomPtr->normals[idx].transformAsNormal( mat );
				
			}
		}
	}
	// Retrieve the value of the cached surface attribute.
	// We will set the new geometry data into the cached surface attribute
	//
	// Access the datablock directly. This code has to be efficient
	// and so we bypass the compute mechanism completely.
	// NOTE: In general we should always go though compute for getting
	// and setting attributes.
	//
	MDataBlock datablock = forceCache();

	MDataHandle cachedHandle = datablock.outputValue( cachedSurface, &stat );
	MCHECKERRORNORET( stat, "computeInputSurface error getting cachedSurface")
	apiMeshData* cached = (apiMeshData*) cachedHandle.asPluginData();

	MDataHandle dHandle = datablock.outputValue( mControlPoints, &stat );
	MCHECKERRORNORET( stat, "transformUsing get dHandle" )		
	
	// If there is history then calculate the tweaks necessary for
	// setting the final positions of the vertices.
	// 
	if ( hasHistory() && (NULL != cached) ) {
		// Since the shape has history, we need to store the tweaks (deltas)
		// between the input shape and the tweaked shape in the control points
		// attribute.
		//
		stat = buildControlPoints( datablock, geomPtr->vertices.length() );
		MCHECKERRORNORET( stat, "transformUsing buildControlPoints" )

		MArrayDataHandle cpHandle( dHandle, &stat );
		MCHECKERRORNORET( stat, "transformUsing get cpHandle" )

		// Loop through the component list and transform each vertex.
		//
		for ( i=0; i<len; i++ )
		{
			MObject comp = componentList[i];
			MFnSingleIndexedComponent fnComp( comp );
			int elemCount = fnComp.elementCount();
			for ( int idx=0; idx<elemCount; idx++ )
			{
				int elemIndex = fnComp.element( idx );
				cpHandle.jumpToElement( elemIndex );
				MDataHandle pntHandle = cpHandle.outputValue();	
				double3& pnt = pntHandle.asDouble3();		

				MPoint oldPnt = cached->fGeometry->vertices[elemIndex];
				MPoint newPnt = geomPtr->vertices[elemIndex];
				MPoint offset = newPnt - oldPnt;

				pnt[0] += offset[0];
				pnt[1] += offset[1];
				pnt[2] += offset[2];				
			}
		}
	}

	// Copy outputSurface to cachedSurface
	//
	if ( NULL == cached ) {
		cerr << "NULL cachedSurface data found\n";
	}
	else {
		*(cached->fGeometry) = *geomPtr;
	}

	MPlug pCPs(thisMObject(),mControlPoints);
	pCPs.setValue(dHandle);

	// Moving vertices will likely change the bounding box.
	//
	computeBoundingBox( datablock );

	// Tell maya the bounding box for this object has changed
	// and thus "boundingBox()" needs to be called.
	//
	childChanged( MPxSurfaceShape::kBoundingBoxChanged );
}

//
// Description
//
//    Transforms the given components. This method is used by
//    the move, rotate, and scale tools in component mode when the
//    tweaks for the shape are stored on a separate tweak node.
//    The bounding box has to be updated here, so do the normals and
//    any other attributes that depend on vertex positions.
//
// Arguments
//    mat           - matrix to tranform the components by
//    componentList - list of components to be transformed,
//                    or an empty list to indicate the whole surface 	
//    cachingMode   - how to use the supplied pointCache
//    pointCache    - if non-null, save or restore points from this list base
//					  on the cachingMode
//    handle	    - handle to the attribute on the tweak node where the
//					  tweaks should be stored
//
/* override */
void
apiMesh::tweakUsing( const MMatrix & mat,
					 const MObjectArray & componentList,
					 MVertexCachingMode cachingMode,
					 MPointArray* pointCache,
					 MArrayDataHandle& handle )
{
	apiMeshGeom* geomPtr = meshGeom();	

	bool savePoints    = (cachingMode == MPxSurfaceShape::kSavePoints);
	bool updatePoints  = (cachingMode == MPxSurfaceShape::kUpdatePoints);

	MArrayDataBuilder builder = handle.builder();

	MPoint delta, currPt, newPt;
	unsigned int i=0;
	unsigned int len = componentList.length();
	unsigned int cacheIndex = 0;
	unsigned int cacheLen = (NULL != pointCache) ? pointCache->length() : 0;

	if (cachingMode == MPxSurfaceShape::kRestorePoints) {
		// restore points from the pointCache
		//
		if (len > 0) {
			// traverse the component list
			//
			for ( i=0; i<len; i++ )
			{
				MObject comp = componentList[i];
				MFnSingleIndexedComponent fnComp( comp );
				int elemCount = fnComp.elementCount();
				for ( int idx=0; idx<elemCount && cacheIndex < cacheLen; idx++, cacheIndex++) {
					int elemIndex = fnComp.element( idx );
					double3 & pt = builder.addElement( elemIndex ).asDouble3();
					MPoint& cachePt = (*pointCache)[cacheIndex];
					pt[0] += cachePt.x;
					pt[1] += cachePt.y;
					pt[2] += cachePt.z;
				}
			}
		} else {
			// if the component list is of zero-length, it indicates that we
			// should transform the entire surface
			//
			len = geomPtr->vertices.length();
			for ( unsigned int idx = 0; idx < len && idx < cacheLen; ++idx ) {
				double3 & pt = builder.addElement( idx ).asDouble3();
				MPoint& cachePt = (*pointCache)[cacheIndex];
				pt[0] += cachePt.x;
				pt[1] += cachePt.y;
				pt[2] += cachePt.z;
			}
		}
	} else {
		// Tweak the points. If savePoints is true, also save the tweaks in the
		// pointCache. If updatePoints is true, add the new tweaks to the existing
		// data in the pointCache.
		//
		if (len > 0) {
			for ( i=0; i<len; i++ )
			{
				MObject comp = componentList[i];
				MFnSingleIndexedComponent fnComp( comp );
				int elemCount = fnComp.elementCount();
				if (savePoints) {
					pointCache->setSizeIncrement(elemCount);
				}
				for ( int idx=0; idx<elemCount; idx++ )
				{
					int elemIndex = fnComp.element( idx );
					double3 & pt = builder.addElement( elemIndex ).asDouble3();
					currPt = newPt = geomPtr->vertices[elemIndex];
					newPt *= mat;
					delta.x = newPt.x - currPt.x;
					delta.y = newPt.y - currPt.y;
					delta.z = newPt.z - currPt.z;
					pt[0] += delta.x;
					pt[1] += delta.y;
					pt[2] += delta.z;
					if (savePoints) {
						// store the points in the pointCache for undo
						//
						pointCache->append(delta*(-1.0));
					} else if (updatePoints && cacheIndex < cacheLen) {
						MPoint& cachePt = (*pointCache)[cacheIndex];
						cachePt[0] -= delta.x;
						cachePt[1] -= delta.y;
						cachePt[2] -= delta.z;
						cacheIndex++;
					}
				}
			}
		} else {
			// if the component list is of zero-length, it indicates that we
			// should transform the entire surface
			//
			len = geomPtr->vertices.length();
			if (savePoints) {
				pointCache->setSizeIncrement(len);
			}
			for ( unsigned int idx = 0; idx < len; ++idx ) {
				double3 & pt = builder.addElement( idx ).asDouble3();
				currPt = newPt = geomPtr->vertices[idx];
				newPt *= mat;
				delta.x = newPt.x - currPt.x;
				delta.y = newPt.y - currPt.y;
				delta.z = newPt.z - currPt.z;
				pt[0] += delta.x;
				pt[1] += delta.y;
				pt[2] += delta.z;
				if (savePoints) {
					// store the points in the pointCache for undo
					//
					pointCache->append(delta*-1.0);
				} else if (updatePoints && idx < cacheLen) {
					MPoint& cachePt = (*pointCache)[idx];
					cachePt[0] -= delta.x;
					cachePt[1] -= delta.y;
					cachePt[2] -= delta.z;
				}
			}
		}
	}
	// Set the builder into the handle.
	//
	handle.set(builder);

	// Tell maya the bounding box for this object has changed
	// and thus "boundingBox()" needs to be called.
	//
	childChanged( MPxSurfaceShape::kBoundingBoxChanged );
}


/* override */
//
// Description
//
//    Returns offsets for the given components to be used my the
//    move tool in normal/u/v mode.
//
// Arguments
//
//    component - components to calculate offsets for
//    direction - array of offsets to be filled
//    mode      - the type of offset to be calculated
//    normalize - specifies whether the offsets should be normalized
//
// Returns
//
//    true if the offsets could be calculated, false otherwise
//
bool apiMesh::vertexOffsetDirection( MObject & component,
									 MVectorArray & direction,
									 MVertexOffsetMode mode,
									 bool normalize )
{
	MStatus stat;
	bool offsetOkay = false ;

	MFnSingleIndexedComponent fnComp( component, &stat );
	if ( !stat || (component.apiType() != MFn::kMeshVertComponent) ) {
		return false;
	}

	offsetOkay = true ;

	apiMeshGeom * geomPtr = meshGeom();
	if ( NULL == geomPtr ) {
		return false;
	}

	// For each vertex add the appropriate offset
	//
	int count = fnComp.elementCount();
	for ( int idx=0; idx<count; idx++ )
	{
		MVector normal = geomPtr->normals[ fnComp.element(idx) ];

		if( mode == MPxSurfaceShape::kNormal ) {
			if( normalize ) normal.normalize() ;
			direction.append( normal );
		}
		else {
			// Construct an orthonormal basis from the normal
			// uAxis, and vAxis are the new vectors.
			//
			MVector uAxis, vAxis ;
			int    i, j, k;
			double a;
			normal.normalize();

			i = 0;  a = fabs( normal[0] );
			if ( a < fabs(normal[1]) ) { i = 1; a = fabs(normal[1]); }
			if ( a < fabs(normal[2]) ) i = 2;
			j = (i+1)%3;  k = (j+1)%3;
			a = sqrt(normal[i]*normal[i] + normal[j]*normal[j]);
			uAxis[i] = -normal[j]/a; uAxis[j] = normal[i]/a; uAxis[k] = 0.0;
			vAxis = normal^uAxis;

			if ( mode == MPxSurfaceShape::kUTangent ||
				 mode == MPxSurfaceShape::kUVNTriad )
			{
				if( normalize ) uAxis.normalize() ;
				direction.append( uAxis );
			}

			if ( mode == MPxSurfaceShape::kVTangent ||
				 mode == MPxSurfaceShape::kUVNTriad )
			{
				if( normalize ) vAxis.normalize() ;
				direction.append( vAxis );
			}

			if ( mode == MPxSurfaceShape::kUVNTriad ) {
				if( normalize ) normal.normalize() ;
				direction.append( normal );
			}
		}
	}

	return offsetOkay ;
}

/* override */
bool apiMesh::isBounded() const
//
// Description
//
//    Specifies that this object has a boundingBox.
//
{ 
	return true;
}

/* override */
MBoundingBox apiMesh::boundingBox() const
//
// Description
//
//    Returns the bounding box for this object.
//    It is a good idea not to recompute here as this funcion is called often.
//
{
    MObject thisNode = thisMObject();
    MPlug   c1Plug( thisNode, bboxCorner1 );
    MPlug   c2Plug( thisNode, bboxCorner2 );
    MObject corner1Object;
    MObject corner2Object;
    c1Plug.getValue( corner1Object );
    c2Plug.getValue( corner2Object );
    
    double3 corner1, corner2;
    
    MFnNumericData fnData;
    fnData.setObject( corner1Object );
    fnData.getData( corner1[0], corner1[1], corner1[2] );
    fnData.setObject( corner2Object );
    fnData.getData( corner2[0], corner2[1], corner2[2] );
    
    MPoint corner1Point( corner1[0], corner1[1], corner1[2] );
    MPoint corner2Point( corner2[0], corner2[1], corner2[2] );
    
    return MBoundingBox( corner1Point, corner2Point );
}    

/* override */
MPxGeometryIterator* apiMesh::geometryIteratorSetup(MObjectArray& componentList,
													MObject& components,
													bool forReadOnly )
//
// Description
//
//    Creates a geometry iterator compatible with his shape.
//
// Arguments
//
//    componentList - list of components to be iterated
//    components    - component to be iterator
//    forReadOnly   -
//
// Returns
//
//    An iterator for the components
//
{
	apiMeshGeomIterator * result = NULL;
	if ( components.isNull() ) {
		result = new apiMeshGeomIterator( meshGeom(), componentList );
	}
	else {
		result = new apiMeshGeomIterator( meshGeom(), components );
	}
	return result;
}

/* override */
bool apiMesh::acceptsGeometryIterator( bool writeable )
//
// Description
//
//    Specifies that this shape can provide an iterator for getting/setting
//    control point values.
//
// Arguments
//
//    writable - maya asks for an iterator that can set points if this is true
//
{
	return true;
}

/* override */
bool apiMesh::acceptsGeometryIterator( MObject&, bool writeable,
									   bool forReadOnly )
//
// Description
//
//    Specifies that this shape can provide an iterator for getting/setting
//    control point values.
//
// Arguments
//
//    writable   - maya asks for an iterator that can set points if this is true
//    forReadOnly - maya asking for an iterator for querying only
//
{
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//
///////////////////////////////////////////////////////////////////////////////

bool apiMesh::hasHistory()
//
// Description
//
//    Returns true if the shape has input history, false otherwise.
//
{
	return fHasHistoryOnCreate;
}

MStatus apiMesh::computeBoundingBox( MDataBlock& datablock )
//
// Description
//
//    Use the larges/smallest vertex positions to set the corners 
//    of the bounding box.
//
{
	MStatus stat = MS::kSuccess;

	// Update bounding box
	//
	MDataHandle lowerHandle = datablock.outputValue( bboxCorner1 );
	MDataHandle upperHandle = datablock.outputValue( bboxCorner2 );
	double3 &lower = lowerHandle.asDouble3();
	double3 &upper = upperHandle.asDouble3();

	apiMeshGeom* geomPtr = meshGeom();
	int cnt = geomPtr->vertices.length();
	if ( cnt == 0 ) return stat;

	// This clears any old bbox values
	//
	MPoint tmppnt = geomPtr->vertices[0];
	lower[0] = tmppnt[0]; lower[1] = tmppnt[1]; lower[2] = tmppnt[2];
	upper[0] = tmppnt[0]; upper[1] = tmppnt[1]; upper[2] = tmppnt[2];


	for ( int i=0; i<cnt; i++ )
	{
		MPoint pnt = geomPtr->vertices[i];

		if ( pnt[0] < lower[0] ) lower[0] = pnt[0];
		if ( pnt[1] < lower[1] ) lower[1] = pnt[1];
		if ( pnt[2] > lower[2] ) lower[2] = pnt[2];
		if ( pnt[0] > upper[0] ) upper[0] = pnt[0];
		if ( pnt[1] > upper[1] ) upper[1] = pnt[1];
		if ( pnt[2] < upper[2] ) upper[2] = pnt[2];
	}
	
	lowerHandle.setClean();
	upperHandle.setClean();

	// Signal that the bounding box has changed.
	//
	childChanged( MPxSurfaceShape::kBoundingBoxChanged );

	return stat;
}

MStatus apiMesh::computeInputSurface( const MPlug& plug, MDataBlock& datablock )
//
// Description
//
//    If there is input history, evaluate the input attribute
//
{
	MStatus stat = MS::kSuccess;

	// Get the input surface if there is history
	//
	if ( hasHistory() ) {	
		MDataHandle inputHandle = datablock.inputValue( inputSurface, &stat );
		MCHECKERROR( stat, "computeInputSurface error getting inputSurface")
		
			apiMeshData* surf = (apiMeshData*) inputHandle.asPluginData();
		if ( NULL == surf ) {
			cerr << "NULL inputSurface data found\n";
			return stat;
		}
	
		apiMeshGeom* geomPtr = surf->fGeometry;
	
		// Create the cachedSurface and copy the input surface into it
		//
		MFnPluginData fnDataCreator;
		MTypeId tmpid( apiMeshData::id );
		fnDataCreator.create( tmpid, &stat );
		MCHECKERROR( stat, "compute : error creating Cached apiMeshData")
			apiMeshData * newCachedData = (apiMeshData*)fnDataCreator.data( &stat );
		MCHECKERROR( stat, " error gettin proxy cached apiMeshData object")
			*(newCachedData->fGeometry) = *geomPtr;
	
		MDataHandle cachedHandle = datablock.outputValue( cachedSurface,&stat );
		MCHECKERROR( stat, "computeInputSurface error getting cachedSurface")
			cachedHandle.set( newCachedData );
	}
	return stat;
}

MStatus apiMesh::computeOutputSurface( const MPlug& plug,
									   MDataBlock& datablock )
//
// Description
//
//    Compute the outputSurface attribute.
//
//    If there is no history, use cachedSurface as the
//    input surface. All tweaks will get written directly
//    to it. Output is just a copy of the cached surface
//    that can be connected etc.
//
{
	MStatus stat;

	// Check for an input surface. The input surface, if it
	// exists, is copied to the cached surface.
	//
	if ( ! computeInputSurface( plug, datablock ) ) {
		return MS::kFailure;
	}

	// Get a handle to the cached data
	//
	MDataHandle cachedHandle = datablock.outputValue( cachedSurface, &stat );
	MCHECKERROR( stat, "computeInputSurface error getting cachedSurface")
	apiMeshData* cached = (apiMeshData*) cachedHandle.asPluginData();
	if ( NULL == cached ) {
		cerr << "NULL cachedSurface data found\n";
	}

	datablock.setClean( plug );

	// Apply any vertex offsets.
	//
	if ( hasHistory() ) {
		applyTweaks( datablock, cached->fGeometry );
	}
	else {
	    MArrayDataHandle cpHandle = datablock.inputArrayValue( mControlPoints,
														   &stat );
		cpHandle.setAllClean();
	}

	// Create some output data

	//
	MFnPluginData fnDataCreator;
	MTypeId tmpid( apiMeshData::id );
	fnDataCreator.create( tmpid, &stat );
	MCHECKERROR( stat, "compute : error creating apiMeshData")
	apiMeshData * newData = (apiMeshData*)fnDataCreator.data( &stat );
	MCHECKERROR( stat, "compute : error gettin at proxy apiMeshData object")

	// Copy the data
	//
	if ( NULL != cached ) {
		*(newData->fGeometry) = *(cached->fGeometry);
	}
	else {
		cerr << "computeOutputSurface: NULL cachedSurface data\n";
	}

	// Assign the new data to the outputSurface handle
	//
	MDataHandle outHandle = datablock.outputValue( outputSurface );
	outHandle.set( newData );

	// Update the bounding box attributes
	//
	stat = computeBoundingBox( datablock );
    MCHECKERROR( stat, "computeBoundingBox" )
	
	return stat;
}

MStatus apiMesh::computeWorldSurface( const MPlug& plug, MDataBlock& datablock )
//
// Description
//
//    Compute the worldSurface attribute.
//
{
	MStatus stat;

	computeOutputSurface( plug, datablock );
	MDataHandle inHandle = datablock.outputValue( outputSurface );
	apiMeshData* outSurf = (apiMeshData*)inHandle.asPluginData();
	if ( NULL == outSurf ) {
		cerr << "computeWorldSurface: outSurf NULL\n";
		return MS::kFailure;
	}

	// Create some output data
	//
	MFnPluginData fnDataCreator;
	MTypeId tmpid( apiMeshData::id );

	fnDataCreator.create( tmpid, &stat );
	MCHECKERROR( stat, "compute : error creating apiMeshData")

	apiMeshData * newData = (apiMeshData*)fnDataCreator.data( &stat );
	MCHECKERROR( stat, "compute : error gettin at proxy apiMeshData object")

	// Get worldMatrix from MPxSurfaceShape and set it to MPxGeometryData
	MMatrix worldMat = getWorldMatrix(datablock, 0);
	newData->setMatrix( worldMat );

	// Copy the data
	//
	*(newData->fGeometry) = *(outSurf->fGeometry);

	// Assign the new data to the outputSurface handle
	//
	int arrayIndex = plug.logicalIndex( &stat );
	MCHECKERROR( stat, "computWorldSurface : logicalIndex" );

	MArrayDataHandle worldHandle = datablock.outputArrayValue( worldSurface,
															   &stat );
 	MCHECKERROR( stat, "computWorldSurface : outputArrayValue" );

	MArrayDataBuilder builder = worldHandle.builder( &stat );
 	MCHECKERROR( stat, "computWorldSurface : builder" );

	MDataHandle outHandle = builder.addElement( arrayIndex, &stat );
	MCHECKERROR( stat, "computWorldSurface : addElement" );
	
	outHandle.set( newData );

	return stat;
}



MStatus apiMesh::applyTweaks( MDataBlock& datablock, apiMeshGeom* geomPtr )
//
// Description
//
//    If the shape has history, apply any tweaks (offsets) made
//    to the control points.
//
{
	MStatus stat;

    MArrayDataHandle cpHandle = datablock.inputArrayValue( mControlPoints,
														   &stat );
    MCHECKERROR( stat, "applyTweaks get cpHandle" )

	// Loop through the component list and transform each vertex.
	//
	int elemCount = cpHandle.elementCount();
	for ( int idx=0; idx<elemCount; idx++ )
	{
		int elemIndex = cpHandle.elementIndex();
		MDataHandle pntHandle = cpHandle.outputValue();	
		double3& pnt = pntHandle.asDouble3();
		MPoint offset( pnt[0], pnt[1], pnt[2] );

		// Apply the tweaks to the output surface
		//
		MPoint& oldPnt = geomPtr->vertices[elemIndex];
		oldPnt = oldPnt + offset;

		cpHandle.next();
	}

	return stat;
}

bool apiMesh::value( int pntInd, int vlInd, double & val ) const
//
// Description
//
//	  Helper function to return the value of a given vertex
//    from the cachedMesh.
//
{
	bool result = false;

	apiMesh* nonConstThis = (apiMesh*)this;
	apiMeshGeom* geomPtr = nonConstThis->cachedGeom();
	if ( NULL != geomPtr ) {
		MPoint point = geomPtr->vertices[ pntInd ];
		val = point[ vlInd ];
		result = true;
	}

	return result;
}

bool apiMesh::value( int pntInd, MPoint & val ) const
//
// Description
//
//	  Helper function to return the value of a given vertex
//    from the cachedMesh.
//
{
	bool result = false;

	apiMesh* nonConstThis = (apiMesh*)this;
	apiMeshGeom* geomPtr = nonConstThis->cachedGeom();
	if ( NULL != geomPtr ) {
		MPoint point = geomPtr->vertices[ pntInd ];
		val = point;
		result = true;
	}

	return result;
}

bool apiMesh::setValue( int pntInd, int vlInd, double val )
//
// Description
//
//	  Helper function to set the value of a given vertex
//    in the cachedMesh.
//
{
	bool result = false;

	apiMesh* nonConstThis = (apiMesh*)this;
	apiMeshGeom* geomPtr = nonConstThis->cachedGeom();
	if ( NULL != geomPtr ) {
		MPoint& point = geomPtr->vertices[ pntInd ];
		point[ vlInd ] = val;
		result = true;
	}

	verticesUpdated();

	return result;
}

bool apiMesh::setValue( int pntInd, const MPoint & val )
//
// Description
//
//	  Helper function to set the value of a given vertex
//    in the cachedMesh.
//
{
	bool result = false;

	apiMesh* nonConstThis = (apiMesh*)this;
	apiMeshGeom* geomPtr = nonConstThis->cachedGeom();
	if ( NULL != geomPtr ) {
		geomPtr->vertices[ pntInd ] = val;
		result = true;
	}

	verticesUpdated();

	return result;
}

MObject apiMesh::meshDataRef()
//
// Description
//
//    Get a reference to the mesh data (outputSurface)
//    from the datablock. If dirty then an evaluation is
//    triggered.
//
{
	// Get the datablock for this node
	//
	MDataBlock datablock = forceCache();

	// Calling inputValue will force a recompute if the
	// connection is dirty. This means the most up-to-date
	// mesh data will be returned by this method.
	//
	MDataHandle handle = datablock.inputValue( outputSurface );
	return handle.data();
}

apiMeshGeom* apiMesh::meshGeom()
//
// Description
//
//    Returns a pointer to the apiMeshGeom underlying the shape.
//
{
	MStatus stat;
	apiMeshGeom * result = NULL;

	MObject tmpObj = meshDataRef();
	MFnPluginData fnData( tmpObj );
	apiMeshData * data = (apiMeshData*)fnData.data( &stat );
	MCHECKERRORNORET( stat, "meshGeom : Failed to get apiMeshData");

	if ( NULL != data ) {
		result = data->fGeometry;
	}
		
	return result;
}

MObject apiMesh::cachedDataRef()
//
// Description
//
//    Get a reference to the mesh data (cachedSurface)
//    from the datablock. No evaluation is triggered.
//
{
	// Get the datablock for this node
	//
	MDataBlock datablock = forceCache();
	MDataHandle handle = datablock.outputValue( cachedSurface );
	return handle.data();
}

apiMeshGeom* apiMesh::cachedGeom()
//
// Description
//
//    Returns a pointer to the apiMeshGeom underlying the shape.
//
{
	MStatus stat;
	apiMeshGeom * result = NULL;

	MObject tmpObj = cachedDataRef();
	MFnPluginData fnData( tmpObj );
	apiMeshData * data = (apiMeshData*)fnData.data( &stat );
	MCHECKERRORNORET( stat, "cachedGeom : Failed to get apiMeshData");

	if ( NULL != data ) {
		result = data->fGeometry;
	}
		
	return result;
}

MStatus apiMesh::buildControlPoints( MDataBlock& datablock, int count )
//
// Description
//
//    Check the controlPoints array. If there is input history
//    then we will use this array to store tweaks (vertex movements).
//
{
	MStatus stat;

	MArrayDataHandle cpH = datablock.outputArrayValue( mControlPoints, &stat );
	MCHECKERROR( stat, "compute get cpH" )
		
	MArrayDataBuilder oldBuilder = cpH.builder();
	if ( count != (int)oldBuilder.elementCount() ) 
	{
		// Make and set the new builder based on the
		// info from the old builder.
		MArrayDataBuilder builder( oldBuilder );
		MCHECKERROR( stat, "compute - create builder" )

		for ( int vtx=0; vtx<count; vtx++ )
		{
		  /* double3 & pt = */ builder.addElement( vtx ).asDouble3();
		}

		cpH.set( builder );
	}
		
	cpH.setAllClean();

	return stat;
}

void apiMesh::verticesUpdated()
//
// Description
//
//    Helper function to tell maya that this shape's
//    vertices have updated and that the bbox needs
//    to be recalculated and the shape redrawn.
//
{
	childChanged( MPxSurfaceShape::kBoundingBoxChanged );
	childChanged( MPxSurfaceShape::kObjectChanged );
}

void* apiMesh::creator()
//
// Description
//
//    Called internally to create a new instance of the users MPx node.
//
{
	return new apiMesh();
}

MStatus apiMesh::initialize()
//
// Description
//
//    Attribute (static) initialization.
//    See api_macros.h.
//
{ 
	MStatus				stat;
    MFnTypedAttribute	typedAttr;
  
	// ----------------------- INPUTS --------------------------
    inputSurface = typedAttr.create( "inputSurface", "is",
									  apiMeshData::id,
									  MObject::kNullObj, &stat );
    MCHECKERROR( stat, "create inputSurface attribute" )
	typedAttr.setStorable( false );
    ADD_ATTRIBUTE( inputSurface );

	// ----------------------- OUTPUTS -------------------------

    // bbox attributes
    //
	MAKE_NUMERIC_ATTR(	bboxCorner1, "bboxCorner1", "bb1",
						MFnNumericData::k3Double, 0,
						false, false, false );
	MAKE_NUMERIC_ATTR(	bboxCorner2, "bboxCorner2", "bb2",
						MFnNumericData::k3Double, 0,
						false, false, false );

	// local/world output surface attributes
	//
    outputSurface = typedAttr.create( "outputSurface", "os",
									  apiMeshData::id,
									  MObject::kNullObj, &stat );
    MCHECKERROR( stat, "create outputSurface attribute" )
    ADD_ATTRIBUTE( outputSurface );
	typedAttr.setWritable( false );

    worldSurface = typedAttr.create( "worldSurface", "ws",
									  apiMeshData::id,
									  MObject::kNullObj, &stat );
    MCHECKERROR( stat, "create worldSurface attribute" );

	typedAttr.setCached( false );

	typedAttr.setWritable( false );

	stat = typedAttr.setArray( true );
    MCHECKERROR( stat, "set array" );

 	stat = typedAttr.setUsesArrayDataBuilder( true );
    MCHECKERROR( stat, "set uses array data builder" );

	stat = typedAttr.setDisconnectBehavior( MFnAttribute::kDelete );
    MCHECKERROR( stat, "set disconnect behavior data builder" );

 	stat = typedAttr.setWorldSpace( true );
    MCHECKERROR( stat, "set world space" );

    ADD_ATTRIBUTE( worldSurface );

	// Cached surface used for file IO
	//
    cachedSurface = typedAttr.create( "cachedSurface", "cs",
									  apiMeshData::id,
									  MObject::kNullObj, &stat );
    MCHECKERROR( stat, "create cachedSurface attribute" )
	typedAttr.setReadable( true );
	typedAttr.setWritable( true );
	typedAttr.setStorable( true );
    ADD_ATTRIBUTE( cachedSurface );

	// ---------- Specify what inputs affect the outputs ----------
	//
	ATTRIBUTE_AFFECTS( inputSurface, outputSurface );
	ATTRIBUTE_AFFECTS( inputSurface, worldSurface );
	ATTRIBUTE_AFFECTS( outputSurface, worldSurface );
    ATTRIBUTE_AFFECTS( inputSurface, bboxCorner1 );
    ATTRIBUTE_AFFECTS( inputSurface, bboxCorner2 );    
	ATTRIBUTE_AFFECTS( cachedSurface, outputSurface );
	ATTRIBUTE_AFFECTS( cachedSurface, worldSurface );

	
    ATTRIBUTE_AFFECTS( mControlPoints, outputSurface );
    ATTRIBUTE_AFFECTS( mControlValueX, outputSurface );
    ATTRIBUTE_AFFECTS( mControlValueY, outputSurface );
    ATTRIBUTE_AFFECTS( mControlValueZ, outputSurface );
    ATTRIBUTE_AFFECTS( mControlPoints, cachedSurface );
    ATTRIBUTE_AFFECTS( mControlValueX, cachedSurface );
    ATTRIBUTE_AFFECTS( mControlValueY, cachedSurface );
    ATTRIBUTE_AFFECTS( mControlValueZ, cachedSurface );
    ATTRIBUTE_AFFECTS( mControlPoints, worldSurface );
    ATTRIBUTE_AFFECTS( mControlValueX, worldSurface );
    ATTRIBUTE_AFFECTS( mControlValueY, worldSurface );
    ATTRIBUTE_AFFECTS( mControlValueZ, worldSurface );
	

	return MS::kSuccess;
}

////////////////////////////////////////////////////////////////////////////////
//
// Node registry
//
// Registers/Deregisters apiMeshData geometry data,
// apiMeshCreator DG node, and apiMeshShape user defined shape.
//
////////////////////////////////////////////////////////////////////////////////

MStatus initializePlugin( MObject obj )
{ 
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");
	MStatus stat1, stat2, stat3;

	stat1 = plugin.registerData( "apiMeshData", apiMeshData::id,
								  &apiMeshData::creator,
								  MPxData::kGeometryData );
	if ( ! stat1 ) {
		cerr << "Failed to register geometry data : apiMeshData \n";
		return stat1;
	}

	stat2 = plugin.registerShape( "apiMesh", apiMesh::id,
								   &apiMesh::creator,
								   &apiMesh::initialize,
								   &apiMeshUI::creator  );
	if ( ! stat2 ) {
		cerr << "Failed to register shape\n";
		if ( stat1) plugin.deregisterData( apiMeshData::id );
		return stat2;
	}
	
	stat3 = plugin.registerNode( "apiMeshCreator", apiMeshCreator::id,
								 &apiMeshCreator::creator,
								 &apiMeshCreator::initialize  );
	if ( ! stat3 ) {
		cerr << "Failed to register creator\n";
		if ( stat2 ) {
			plugin.deregisterNode( apiMesh::id );
			plugin.deregisterData( apiMeshData::id );
		}
	}

	return stat3;

}

MStatus uninitializePlugin( MObject obj)
{
	MFnPlugin plugin( obj );
	MStatus stat;

	stat = plugin.deregisterNode( apiMesh::id );
	if ( ! stat ) {
		cerr << "Failed to deregister shape : apiMeshShape \n";
	}

	stat = plugin.deregisterData( apiMeshData::id );
	if ( ! stat ) {
		cerr << "Failed to deregister geometry data : apiMeshData \n";
	}
 
	stat = plugin.deregisterNode( apiMeshCreator::id );
	if ( ! stat ) {
		cerr << "Failed to deregister node : apiMeshCreator \n";
	}

	return stat;
}
