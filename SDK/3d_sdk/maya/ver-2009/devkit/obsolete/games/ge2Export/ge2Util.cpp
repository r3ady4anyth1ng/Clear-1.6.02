/*
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
*/

//
// Description: 
//            
// ge2 utilities for Maya
// 
//

#include "ge2Util.h"
#include "ge2Wrapper.h"
#include <maya/MIOStream.h>

extern const char * objectName( MObject object );

// global functions:

// This function shows how you can implement a bounding box generator
// with MDt. We get the appropriate matrix from the dagPath, then
// transform all of the vertices by this matrix, and extract a min
// and max from it. Note that, for full hierarchy, you could get the bounding
// box attribute from the 
bool getBoundingBox( int shapeIdx, DtVec3f *boundingBox )
{
	int		numVertices;
	int		currVertex;

	MVector	min,
			max;

	DtVec3f *vertices;

	float	*mtx,
			xform[4][4];

	MPoint	currPoint;

	MObject	transformNode;	
	
	boundingBox[0].vec[0] = 0;
	boundingBox[0].vec[1] = 0;
	boundingBox[0].vec[2] = 0;
	boundingBox[1].vec[0] = 0;
	boundingBox[1].vec[1] = 0;
	boundingBox[1].vec[2] = 0;

	DtShapeGetMatrix( shapeIdx, &mtx );
	memcpy( xform, mtx, sizeof(float) * 16 );
	MMatrix	transformMatrix(xform);
	DtShapeGetVertices( shapeIdx, &numVertices, &vertices );

	if (numVertices == 0)
	{
		return false;
	}

	currPoint.x = vertices[0].vec[0];
	currPoint.y = vertices[0].vec[1];
	currPoint.z = vertices[0].vec[2];
	currPoint.w = 1;

	currPoint *= transformMatrix;

	min.x = max.x = currPoint.x;
	min.y = max.y = currPoint.y;
	min.z = max.z = currPoint.z;
	
	// Start at 1 because they are set to the 0th element directly above
	for ( currVertex = 1; currVertex < numVertices; currVertex++ )
	{
		currPoint.x = vertices[currVertex].vec[0];
		currPoint.y = vertices[currVertex].vec[1];
		currPoint.z = vertices[currVertex].vec[2];
		currPoint.w = 1;
		currPoint *= transformMatrix;

		if ( currPoint.x < min.x )
			min.x = currPoint.x;
		if ( currPoint.x > max.x )
			max.x = currPoint.x;
		if ( currPoint.y < min.y )
			min.y = currPoint.y;
		if ( currPoint.y > max.y )
			max.y = currPoint.y;
		if ( currPoint.z < min.z )
			min.z = currPoint.z;
		if ( currPoint.z > max.z )
			max.z = currPoint.z;
	}

	boundingBox[0].vec[0] = (float) min.x;
	boundingBox[0].vec[1] = (float) min.y;
	boundingBox[0].vec[2] = (float) min.z;
	boundingBox[1].vec[0] = (float) max.x;
	boundingBox[1].vec[1] = (float) max.y;
	boundingBox[1].vec[2] = (float) max.z;

	return true;
}

bool addElement( MIntArray& intArray, int newElem )
{
	unsigned int currIndex;

	for ( currIndex = 0; currIndex < intArray.length(); currIndex++ )
	{
		if ( newElem == intArray[currIndex] ) // Don't add if it's there already
			return false;

		if ( newElem < intArray[currIndex] )
		{
			intArray.insert( newElem, currIndex );
			return true;
		}
	}

	// If we made it here it should go at the end...
	intArray.append( newElem );
	return true;
}

MIntArray shapeGetTRSAnimKeys( int shapeID )
{
	MStatus			status;
	MObject			transformNode;

	MObject			anim;
	MFnDependencyNode dgNode;
	MDagPath		dagPath;

	int				currKey,
					numKeys,
					keyTime,
					stat;

	MIntArray		keyFrames;

#ifdef MAYA1X
	return keyFrames;
#else
	MItDependencyGraph::Direction direction = MItDependencyGraph::kUpstream;
	MItDependencyGraph::Traversal traversalType = MItDependencyGraph::kBreadthFirst;
	MItDependencyGraph::Level level = MItDependencyGraph::kNodeLevel;

	MFn::Type filter = MFn::kAnimCurve;
	
	stat = DtExt_ShapeGetTransform( shapeID, transformNode );
	if ( 1 != stat )
	{
		cerr << "DtExt_ShapeGetTransform problems" << endl;
		return keyFrames;
	}
	
	MItDependencyGraph dgIter( transformNode, filter, direction, 
							traversalType, level, &status );

	for ( ; !dgIter.isDone(); dgIter.next() )
    {        
		anim = dgIter.thisNode( &status );
		MFnAnimCurve animCurve( anim, &status );
		if ( MS::kSuccess == status )
		{
			numKeys = animCurve.numKeyframes( &status );
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				// Truncating values here...
				keyTime = (int) animCurve.time( currKey, &status ).value();
				addElement( keyFrames, keyTime );
			}
		}
    }

	return keyFrames;
#endif
}

MIntArray shapeGetVertAnimKeys( int shapeID )
{
	MStatus			status;
	MObject			shapeNode;

	MObject			anim;
	MFnDependencyNode dgNode;
	MDagPath		dagPath;

	int				currKey,
					numKeys,
					keyTime,
					stat;

	MIntArray		keyFrames;

#ifdef MAYA1X
	return keyFrames;
#else
	MDagPath		shapeDagPath;
	MItDependencyGraph::Direction direction = MItDependencyGraph::kUpstream;
	MItDependencyGraph::Traversal traversalType = MItDependencyGraph::kBreadthFirst;
	MItDependencyGraph::Level level = MItDependencyGraph::kNodeLevel;
	MFn::Type filter = MFn::kAnimCurve;
	
	stat = DtExt_ShapeGetShapeNode( shapeID, shapeNode );
	if ( 1 != stat )
	{
		cerr << "Problems in shapeGetShapeNode" << endl;
	}

	MItDependencyGraph dgIter( shapeNode, filter, direction, 
							traversalType, level, &status );

	for ( ; !dgIter.isDone(); dgIter.next() )
	{
		anim = dgIter.thisNode( &status );
		MFnAnimCurve animCurve( anim, &status );
		if ( MS::kSuccess == status )
		{
			numKeys = animCurve.numKeyframes( &status );
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				// Truncating values here; may need more control
				keyTime = (int) animCurve.time( currKey, &status ).value();
				addElement( keyFrames, keyTime );
			}
		}
	}

	return keyFrames;
#endif
}

int shapeGetNumEdges( int shapeID, int& numEdges )
{
	int		status;

	MObject shapeNode;
	MStatus returnStatus;

	numEdges = 0;

	status = DtExt_ShapeGetShapeNode( shapeID, shapeNode );
	if ( 1 != status )
		return 0;

	MFnMesh fnMesh( shapeNode, &returnStatus );

	numEdges = fnMesh.numEdges( &returnStatus );
	if ( MS::kSuccess != returnStatus )
	{
		numEdges = 0;
		return 0;
	}

	return 1;
}

int shapeGetEdge( int shapeID, int edgeID, long2& edge )
{
	int		status;

	long2	edgeVertices;

	MObject shapeNode;
	MStatus returnStatus;

	edge[0] = -1;
	edge[1] = -1;

	status = DtExt_ShapeGetShapeNode( shapeID, shapeNode );
	if ( 1 != status )
		return 0;

	MFnMesh fnMesh( shapeNode, &returnStatus );

	returnStatus = fnMesh.getEdgeVertices( edgeID, edgeVertices );
	if ( MS::kSuccess != returnStatus )
		return 0;

	edge[0] = edgeVertices[0];
	edge[1] = edgeVertices[1];

	return 1;
}

// This function _copies_ the name into the passed char**-
// assumes the caller has allocated enough space (and the caller
// is responsible for disposing of it)
int shapeGetShapeName( int shapeID, char* name )
{
	int		status;
    MObject	shapeNode;

    char	*cp = NULL;	

	// Otherwise have to assume it's a valid char array...
	if ( NULL == name )
		return 0;

	name[0] = '\0';

	status = DtExt_ShapeGetShapeNode( shapeID, shapeNode );
	if ( 1 != status )
		return 0;

	// Get the name from the node.
    //
    cp = (char *) objectName( shapeNode );
    if( NULL == cp ) 
	{
		return 0;
	}

    // Copy over the name. No changes to the Maya name.
    //
    strcpy( name, cp );

    return 1;

}  // shapeGetShapeName //

// This function _copies_ the name into the passed char**-
// assumes the caller has allocated enough space (and the caller
// is responsible for disposing of it)
int lightGetName( int lightID, char* name )
{
	int		status;
    MObject	lightTransform;

    char	*cp = NULL;

	// Otherwise have to assume it's a valid char array...
	if ( NULL == name )
		return 0;

	name[0] = '\0';

	status = DtExt_LightGetTransform( lightID, lightTransform );
	if ( 1 != status )
		return 0;

	// Get the name from the node.
    //
    cp = (char *) objectName( lightTransform );
    if( NULL == cp ) 
	{
		return 0;
	}

    // Copy over the name. No changes to the Maya name.
    //
    strcpy( name, cp );

    return 1;

}  // lightGetName //

int lightGetUseDecayRegions( int lightID, bool& useDecay )
{
	int		lightType;
	int		status;
	bool	decay;

	MStatus returnStatus = MS::kSuccess;
	MObject lightShape;

	useDecay = false;

	status = DtLightGetType( lightID, &lightType );
	if ( 1 != status )
		return 0;

	if ( DT_SPOT_LIGHT != lightType )
		return 0; // Only spotlights have decay regions

	status = DtExt_LightGetShapeNode( lightID, lightShape );
	if ( 1 != status )
		return 0;

	MFnSpotLight fnSpotLight( lightShape, &returnStatus );
	if ( MS::kSuccess != returnStatus )
		return 0;

	decay = fnSpotLight.useDecayRegions( &returnStatus );

	if ( MS::kSuccess != returnStatus )
		return 0;
	
	// if we got here data is OK
	useDecay = decay;
	return 1;
}

int lightGetDistanceToCenter( int lightID, float& distance )
{
	int		lightType;
	int		status;
	double	tmpDistance;

	MStatus returnStatus = MS::kSuccess;
	MObject lightShape;

	distance = 0;

	status = DtLightGetType( lightID, &lightType );
	if ( 1 != status )
		return 0;

	if ( DT_SPOT_LIGHT != lightType )
		return 0; // Only spotlights have centers?

	status = DtExt_LightGetShapeNode( lightID, lightShape );
	if ( 1 != status )
		return 0;

	MFnSpotLight fnSpotLight( lightShape, &returnStatus );
	if ( MS::kSuccess != returnStatus )
		return 0;

	tmpDistance = fnSpotLight.centerOfIllumination( &returnStatus );

	if ( MS::kSuccess != returnStatus )
		return 0;
	
	// if we got here data is OK
	distance = (float) tmpDistance;
	return 1;
}

int cameraGetUpDir( int cameraID, float& xUp, float& yUp, float& zUp )
{
	int		status;
	MObject	cameraShape;
	MStatus returnStatus = MS::kSuccess;

	xUp = 0;
	yUp = 0;
	zUp = 0;

	status = DtExt_CameraGetShapeNode( cameraID, cameraShape );
	if ( 1 != status)
		return 0; // bad cameraID, probably

	MFnCamera fnCamera( cameraShape, &returnStatus );

	if( MS::kSuccess == returnStatus )
	{
		MVector upDir = fnCamera.upDirection( MSpace::kObject, &returnStatus );

		if ( MS::kSuccess == returnStatus )
		{
			xUp = (float) upDir.x;
			yUp = (float) upDir.y;
			zUp = (float) upDir.z;
			return 1;
		}
	}

	return 0;
}

int cameraGetWidthAngle( int cameraID, float& angle )
{
	int		status,
			type;
	
	MObject	cameraShape;
	MStatus	returnStatus;	

	// Initialize return values.
    //
	angle = 0.0;

	status = DtCameraGetType( cameraID, &type );
	if ( 1 != status )
		return 0;
	if ( DT_PERSPECTIVE_CAMERA != type )
		return 0;

	status = DtExt_CameraGetShapeNode( cameraID, cameraShape );

	if ( 1 != status )
		return 0;

	MFnCamera fnCamera( cameraShape, &returnStatus );

	if ( MS::kSuccess != returnStatus )
		return 0;
	
	angle = (float) fnCamera.horizontalFieldOfView( &returnStatus );
	angle = (angle * 180) / (float) PI;

	if ( MS::kSuccess != returnStatus )
	{
		angle = 0.0;
		return 0;
	}

	return 1;
}

bool fileExists( MString fullPath )
{
	
#if defined (OSMac_)
	char fname[MAXPATHLEN];
	strcpy (fname, fullPath.asChar());
	FILE* fp = fopen( fname, "ra" );
#else
	FILE* fp = fopen( fullPath.asChar(), "r" );
#endif
	if ( fp )
	{
		fclose( fp );
		return true;
	}
	else
		return false;
}

// To determine some characteristics of shapes that ge tracks in materials
int getFaceType( int shapeID )
{
	int retVal;

	if ( DtShapeIsDoubleSided( shapeID ) )
		retVal = ge2Wrapper::kFaceBoth;
	else
	{
		if ( DtShapeIsOpposite( shapeID ) )
			retVal = ge2Wrapper::kFaceBack;
		else
			retVal = ge2Wrapper::kFaceFront;
	}

	return retVal;
}

int isFlatShaded( int shapeID )
{
	return (int) DtShapeIsFlatShaded( shapeID );
}

int isColoredPerVertex( int shapeID )
{
	int count;
	DtRGBA *colors;

	DtShapeGetVerticesColor( shapeID, &count, &colors );

	if ( 0 >= count )
		return 0;
	else
		return 1;
}

// returns -1 if some faces have one value and others have different value
int isVertexNormalPerFace( int shapeID, int groupID )
{
	int		currFace,
			numFaces,
			numVerticesInFace,
			newVal,
			retVal = -1;

	int		*vArr,
			*nArr,
			*tArr;

	DtPolygonGetCount( shapeID, groupID, &numFaces );
	for ( currFace = 0; currFace < numFaces; currFace++ )
	{
		DtPolygonGetIndices(currFace, &numVerticesInFace, &vArr, &nArr, &tArr);
		if ( -1 == retVal ) 
			retVal = ( nArr ) ? 1 : 0;
		else {
			newVal = ( nArr ) ? 1 : 0;
			if ( newVal != retVal )
				return -1; // Indeterminate!!
		}
	}

	return retVal;
}

int isPreLighted( int shapeID )
{
	return 0;
}

//
// MTextFileWriter methods
//
MTextFileWriter::MTextFileWriter()
{
	reset();
}

MTextFileWriter::~MTextFileWriter()
{
	reset();
}


void MTextFileWriter::reset()
{
	resetFileAndBuffer();

	numTabStops = TEXT_FILE_WRITER_DEFAULT_NUM_TAB_STOPS;
	precision = TEXT_FILE_WRITER_DEFAULT_PRECISION;

	useTabs = true;
}

void MTextFileWriter::resetFileAndBuffer()
{
	buffer.clear();
	filePath.clear();
}

void MTextFileWriter::printTabs( char* storage )
{
	int			i;
	char		tab[16] = "   ";

	if ( 0 == numTabStops )
		return;

//	assert( numTabStops > 0 ); // shouldn't ever be negative!
	if ( numTabStops < 0 )
	{
		cerr << "###!!! numTabStops: " << numTabStops << endl;
		numTabStops = 0;
	}

	for ( i = 0; i < numTabStops; i++ )
	{
		strcat( storage, tab );
	}
}

void MTextFileWriter::clearFile()
{
	resetFileAndBuffer();
}

void MTextFileWriter::setFile( const MString newFilePath )
{
	assert( (0 == filePath.length()) && (0 == buffer.length()) );

	// buffer should be cleared already...
//	buffer.clear();
	filePath = newFilePath;
}

void MTextFileWriter::writeFile()
{
	FILE*			file;
	unsigned int	i;

	assert( 0 != filePath.length() );

#if defined (OSMac_)
	char fname[MAXPATHLEN];
	strcpy (fname, filePath.asChar());
	file = fopen( fname, "wt" );
#else
	file = fopen( filePath.asChar(), "wt" );
#endif
	assert( file );

	for ( i = 0; i < buffer.length(); i++ )
	{
		fprintf( file, "%s", buffer[i].asChar() );
	}

	fclose( file );

	resetFileAndBuffer();
}

void MTextFileWriter::print( const char* format, ... )
{
	unsigned int	formatCounter;
	int				bufferStringIndex;

	char			newFormat[TEXT_FILE_WRITER_MAX_FORMAT_LENGTH],
					precisionStr[32],
					temp[2],
					storageBuf[TEXT_FILE_WRITER_MAX_SINGLE_CALL_LENGTH],
					precisionChar;

	bool			keepLooking,
					foundDot;

	newFormat[0] = '\0';
	storageBuf[0] = '\0';
	precisionChar = '0' + precision;

	// going to have troubles with \%'s
	for ( formatCounter = 0; formatCounter < strlen(format); formatCounter++ )
	{
		if ( '%' == format[formatCounter] )
		{
			foundDot = false;
			temp[0] = format[formatCounter];
			temp[1] = '\0';
			strcpy( precisionStr, temp );
			keepLooking = true;
			while (keepLooking)
			{
				switch ( format[++formatCounter] )
				{
				case 'x':
					// Don't want to precision hex values!
					if ( foundDot )
					{
						temp[0] = precisionChar;
					}
					else
					{
						temp[0] = format[formatCounter];
						keepLooking = false;
					}
					break;

				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
					temp[0] = format[formatCounter];
					keepLooking = false;
					break;
				case '0':					
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':				
					temp[0] = format[formatCounter];
					break;
				case '.':
					foundDot = true;
					temp[0] = format[formatCounter];
					break;

				// Anything else we quit checking...
				default:
					temp[0] = format[formatCounter];
					keepLooking = false;
					break;
				}
				strcat( precisionStr, temp );
			}
			strcat( newFormat, precisionStr );
		}
		else
		{
			temp[0] = format[formatCounter];
			temp[1] = '\0';
			strcat( newFormat, temp );
		}
	}

	va_list argList;
	va_start ( argList, format );

	vsprintf( storageBuf, newFormat, argList );
	
	va_end( argList );

	bufferStringIndex = 0;
	if ( buffer.length() > 0 )
	{
		bufferStringIndex = buffer[buffer.length()-1].rindex( '\n' );
	}

	if ( -1 != bufferStringIndex ) // add a new string [with tabs] to buffer array
	{
		MString newLine = "";
		if ( useTabs )
		{
			char tabBuf[TEXT_FILE_WRITER_MAX_TAB_LENGTH];
			tabBuf[0] = '\0';
			printTabs( tabBuf );
			newLine += tabBuf;
		}

		newLine += storageBuf;			

		buffer.append( newLine );
	}
	else
	{
		buffer[buffer.length() - 1] += storageBuf;
	}
}
