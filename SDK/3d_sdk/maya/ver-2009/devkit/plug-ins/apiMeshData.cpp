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
// apiMeshData.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>
#include <apiMeshData.h>
#include <apiMeshIterator.h>
#include <api_macros.h>

#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MArgList.h>

//////////////////////////////////////////////////////////////////////

// Ascii file IO defines
//
#define kDblQteChar				"\""
#define kSpaceChar				"	"
#define kWrapString				"\n\t\t"
#define kVertexKeyword			"v"
#define kNormalKeyword			"vn"
#define kTextureKeyword			"vt"
#define kFaceKeyword			"face"
#define kUVKeyword				"uv" 

//////////////////////////////////////////////////////////////////////

const MTypeId apiMeshData::id( 0x80777 );
const MString apiMeshData::typeName( "apiMeshData" );

apiMeshData::apiMeshData() : fGeometry( NULL )
{
	fGeometry = new apiMeshGeom;
}

apiMeshData::~apiMeshData()
{
	if ( NULL != fGeometry ) {
		delete fGeometry;
		fGeometry = NULL;
	}
}

/* override */
MStatus apiMeshData::readASCII( const MArgList& argList, unsigned& index )
//
// Description
//    ASCII file input method.
//
{
	if ( ! readVerticesASCII(argList,index) ) {
		return MS::kFailure;
	}
	else if ( ! readNormalsASCII(argList,++index) ) {
		return MS::kFailure;
	}
	else if ( ! readFacesASCII(argList,++index) ) {
		return MS::kFailure;
	} 
	else if ( ! readUVASCII(argList,index) ) { 
		return MS::kFailure; 
	}
	
	
	return MS::kSuccess;
}

/* override */
MStatus apiMeshData::readBinary( istream& /*in*/, unsigned /*length*/ )
//
// Description
//     NOT IMPLEMENTED
//
{
	return MS::kSuccess;
}

/* override */
MStatus apiMeshData::writeASCII( ostream& out )
//
// Description
//    ASCII file output method.
//
{
	if ( ! writeVerticesASCII(out) ) {
		return MS::kFailure;
	}
	else if ( ! writeNormalsASCII(out) ) {
		return MS::kFailure;
	}
	else if ( ! writeFacesASCII(out) ) {
		return MS::kFailure;
	}

	writeUVASCII(out); 
	return MS::kSuccess;
}

/* override */
MStatus apiMeshData::writeBinary( ostream& /*out*/ )
//
// Description
//    NOT IMPLEMENTED
//
{
	return MS::kSuccess;
}

/* override */
void apiMeshData::copy ( const MPxData& other )
{
	*fGeometry = *(((const apiMeshData &)other).fGeometry);
}

/* override */
MTypeId apiMeshData::typeId() const
//
// Description
//    Binary tag used to identify this kind of data
//
{
	return apiMeshData::id;
}

/* override */
MString apiMeshData::name() const
//
// Description
//    String name used to identify this kind of data
//
{
	return apiMeshData::typeName;
}

void * apiMeshData::creator()
{
	return new apiMeshData;
}

/* override */
MPxGeometryIterator* apiMeshData::iterator( MObjectArray & componentList,
											MObject & component,
											bool useComponents)
//
// Description
//
{
	apiMeshGeomIterator * result = NULL;
	if ( useComponents ) {
		result = new apiMeshGeomIterator( fGeometry, componentList );
	}
	else {
		result = new apiMeshGeomIterator( fGeometry, component );
	}
	return result;
}

/* override */
MPxGeometryIterator* apiMeshData::iterator( MObjectArray & componentList,
											MObject & component,
											bool useComponents,
											bool /*world*/) const
//
// Description
//
{
	apiMeshGeomIterator * result = NULL;
	if ( useComponents ) {
		result = new apiMeshGeomIterator( fGeometry, componentList );
	}
	else {
		result = new apiMeshGeomIterator( fGeometry, component );
	}
	return result;
}

/* override */
bool apiMeshData::updateCompleteVertexGroup( MObject & component ) const
//
// Description
//     Make sure complete vertex group data is up-to-date.
//     Returns true if the component was updated, false if it was already ok.
//
//     This is used by deformers when deforming the "whole" object and
//     not just selected components.
//
{
	MStatus stat;
	MFnSingleIndexedComponent fnComponent( component, &stat );

	// Make sure there is non-null geometry and that the component
	// is "complete". A "complete" component represents every 
	// vertex in the shape.
	//
	if ( stat && (NULL != fGeometry) && (fnComponent.isComplete()) ) {
	
		int maxVerts ;
		fnComponent.getCompleteData( maxVerts );
		int numVertices = fGeometry->vertices.length();

		if ( (numVertices > 0) && (maxVerts != numVertices) ) {
			// Set the component to be complete, i.e. the elements in
			// the component will be [0:numVertices-1]
			//
			fnComponent.setCompleteData( numVertices );
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////

MStatus apiMeshData::writeUVASCII( ostream &out )
{
	int uvCount = fGeometry->uvcoords.uvcount(); 
	int faceVertexCount = fGeometry->uvcoords.faceVertexIndex.length();

	if ( uvCount > 0 ) { 
		out << "\n"; 
		out << kWrapString; 
		out << kDblQteChar << kUVKeyword << kDblQteChar
			<< kSpaceChar << uvCount << kSpaceChar << faceVertexCount; 

		int i; 
		float u, v; 
		for ( i = 0; i < uvCount; i ++ ) { 
			fGeometry->uvcoords.getUV( i, u, v ); 
			out << kWrapString; 
			out << u << kSpaceChar; 
			out << v << kSpaceChar; 
		}

		const MIntArray &fvl = fGeometry->uvcoords.faceVertexIndex; 
		for ( i = 0; i < faceVertexCount; i ++ ) { 
			out << kWrapString; 
			out << fvl[i] << kSpaceChar; 
		}
	}

	return MS::kSuccess; 
}

MStatus apiMeshData::writeVerticesASCII( ostream& out )
{
	MPoint vertex;
	int vertexCount = fGeometry->vertices.length();

	out << "\n";
	out << kWrapString;
	out << kDblQteChar << kVertexKeyword << kDblQteChar
		<< kSpaceChar << vertexCount;

	for ( int i=0; i<vertexCount; i++ ) {
		vertex = fGeometry->vertices[i];
		out << kWrapString;
		out << vertex[0] << kSpaceChar;
		out << vertex[1] << kSpaceChar;
		out << vertex[2];
	}

	return MS::kSuccess;
}

MStatus apiMeshData::writeNormalsASCII( ostream& out )
{
	MVector normal;
	int normalCount = fGeometry->normals.length();

	out << "\n";
	out << kWrapString;
	out << kDblQteChar << kNormalKeyword << kDblQteChar
		<< kSpaceChar << normalCount;

	for ( int i=0; i<normalCount; i++ ) {
		normal = fGeometry->normals[i];
		out << kWrapString;
		out << normal[0] << kSpaceChar;
		out << normal[1] << kSpaceChar;
		out << normal[2];
	}

	return MS::kSuccess;
}

MStatus apiMeshData::writeFacesASCII( ostream& out )
{
	int numFaces = fGeometry->face_counts.length();
	int vid = 0;

	for ( int f=0; f<numFaces; f++ )
	{
		int faceVertexCount = fGeometry->face_counts[f];
		out << "\n";
		out << kWrapString;
		out << kDblQteChar << kFaceKeyword << kDblQteChar
			<< kSpaceChar << faceVertexCount;

		out << kWrapString;
		for ( int v=0; v<faceVertexCount; v++ )
		{
			out << fGeometry->face_connects[vid] << kSpaceChar;
			vid++;
		}
	}

	return MS::kSuccess;
}

MStatus apiMeshData::readVerticesASCII( const MArgList& argList,
										unsigned& index )
{
	MStatus result;
	MString geomStr;
	MPoint vertex;
	int vertexCount = 0;

	result = argList.get( index, geomStr );

	if ( result && (geomStr == kVertexKeyword) ) {
		result = argList.get( ++index, vertexCount );
		
		for ( int i=0; i<vertexCount; i++ )
		{
			if ( argList.get( ++index, vertex ) ) {
				fGeometry->vertices.append( vertex );
			}
			else {
				result = MS::kFailure;
			}
		}
	}

	return result;
}

MStatus apiMeshData::readNormalsASCII( const MArgList& argList,
									   unsigned& index )
{
	MStatus result;
	MString geomStr;
	MPoint normal;
	int normalCount = 0;

	result = argList.get( index, geomStr );

	if ( result && (geomStr == kNormalKeyword) ) {
		result = argList.get( ++index, normalCount );
		for ( int i=0; i<normalCount; i++ )
		{
			if ( argList.get( ++index, normal ) ) {
				fGeometry->normals.append( normal );
			}
			else {
				result = MS::kFailure;
			}
		}
	}

	return result;
}

MStatus apiMeshData::readUVASCII( const MArgList &argList, 
								  unsigned &index )
{
	MStatus result = MS::kSuccess; 
	MString uvStr; 
	double u, v; 
	int fvi; 
	int faceVertexListCount = 0; 
	int uvCount; 
	
	fGeometry->uvcoords.reset(); 
	if ( argList.get(index,uvStr) && (uvStr == kUVKeyword) ) { 
		result = argList.get( ++index, uvCount ); 
		if ( result ) { 
			result = argList.get( ++index, faceVertexListCount ); 
		}
		int i; 
		for ( i = 0; i < uvCount && result; i ++ ) { 
			if ( argList.get( ++index, u ) && argList.get( ++index, v ) ) { 
				fGeometry->uvcoords.append_uv( (float)u, (float)v );
			} else { 
				result = MS::kFailure; 
			}
		}
		
		for ( i = 0; i < faceVertexListCount && result; i++ ) { 
			if ( argList.get( ++index, fvi ) ) { 
				fGeometry->uvcoords.faceVertexIndex.append( fvi ); 
			} else { 
				result = MS::kFailure; 
			}
		}
	}
	
	return result; 
}

MStatus apiMeshData::readFacesASCII( const MArgList& argList,
									 unsigned& index )
{
	MStatus result = MS::kSuccess;
	MString geomStr;
	int faceCount = 0;
	int vid;

	while( argList.get(index,geomStr) && (geomStr == kFaceKeyword) )
	{
		result = argList.get( ++index, faceCount );
		fGeometry->face_counts.append( faceCount );

		for ( int i=0; i<faceCount; i++ )
		{
			if ( argList.get( ++index, vid ) ) {
				fGeometry->face_connects.append( vid );
			}
			else {
				result = MS::kFailure;
			}
		}
		index++;
	}

	fGeometry->faceCount = fGeometry->face_counts.length();
	return result;
}
