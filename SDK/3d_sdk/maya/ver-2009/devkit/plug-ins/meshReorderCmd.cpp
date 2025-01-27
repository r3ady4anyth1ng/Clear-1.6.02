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

#include "meshReorderCmd.h"
#include "meshMapUtils.h"

#include <maya/MItSelectionList.h>
#include <maya/MItMeshVertex.h>
#include <maya/MArgList.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MDagPathArray.h>


// CONSTRUCTOR DEFINITION:
meshReorderCommand::meshReorderCommand()
{
}


// DESTRUCTOR DEFINITION:
meshReorderCommand::~meshReorderCommand()
{
}


// METHOD FOR CREATING AN INSTANCE OF THIS COMMAND:
void* meshReorderCommand::creator()
{
   return new meshReorderCommand;
}


MStatus meshReorderCommand::parseArgs(const MArgList& args)
{
	MStatus stat;
	MString err;


	if( args.length() != 3 )
	{
		displayError("3 vertices must be specified");
		return MS::kFailure;
	}

	MObjectArray selectedComponent(3);
	MDagPathArray selectedPath;

	selectedPath.setLength(3);

	int argIdx = 0;
	for (unsigned int j = 0; j < 3; j++)
	{
		MString arg;

		if( ( stat = args.get( argIdx, arg )) != MStatus::kSuccess )
		{
			displayError( "Can't parse arg");
			return stat;
		}

		MSelectionList list;
		if (! list.add( arg ) )
		{
			err = arg + ": no such component";
			displayError(err);
			return MS::kFailure; // no such component
		}

		MItSelectionList selectionIt (list, MFn::kComponent);
		if (selectionIt.isDone ())
		{
			err = arg + ": not a component";
			displayError (err);
			return MS::kFailure;
		}

		if( selectionIt.getDagPath (selectedPath[j], selectedComponent[j]) != MStatus::kSuccess )
		{
			displayError( "Can't get path for");
			return stat;
		}


		if (!selectedPath[j].node().hasFn(MFn::kMesh) && !(selectedPath[j].node().hasFn(MFn::kTransform) && selectedPath[j].hasFn(MFn::kMesh)))
		{
			err = arg + ": Invalid type!  Only a mesh or its transform can be specified!";
			displayError (err);
			return MStatus::kFailure;
		}

		argIdx++;
	}

	if( ( stat = meshMapUtils::validateFaceSelection( selectedPath, selectedComponent, &fFaceIdxSrc, &fFaceVtxSrc ) ) != MStatus::kSuccess )
	{
		displayError("Selected vertices don't define a unique face on source mesh");
		return stat;
	}

	fDagPathSrc = selectedPath[0];

	return stat;
}

// FIRST INVOKED WHEN COMMAND IS CALLED, PARSING THE COMMAND ARGUMENTS, INITIALIZING DEFAULT PARAMETERS, THEN CALLING redoIt():
MStatus meshReorderCommand::doIt(const MArgList& args)
{
	MStatus  stat = MStatus::kSuccess;

	if ( ( stat = parseArgs( args ) ) != MStatus::kSuccess )
	{
		displayError ("Error parsing arguments");
		return stat;
	}

	return redoIt();
}

MStatus meshReorderCommand::redoIt()
{
	MStatus  stat = MStatus::kSuccess;

	MIntArray 			newPolygonCounts;
	MIntArray 			newPolygonConnects;
	MFloatPointArray 	origVertices;
	MFloatPointArray 	newVertices;


	MFnMesh theMesh( fDagPathSrc, &stat );
	if( stat != MStatus::kSuccess )
	{
		displayError(" MFnMesh creation");
		return stat;
	}

	stat = theMesh.getPoints (origVertices, MSpace::kWorld );
	if( stat != MStatus::kSuccess )
	{
		displayError(" MFnMesh getPoints");
		return stat;
	}

	// Initialize the traversal flags and CV mappings for this shape 
	MIntArray 			faceTraversal( theMesh.numPolygons(), false );
	MIntArray 			cvMapping( theMesh.numVertices(), -1 );
	MIntArray 			cvMappingInverse( theMesh.numVertices(), -1 ); 

	//
	//  Starting with the user selected face, recursively rebuild the entire mesh
	//

	stat = meshMapUtils::traverseFace( fDagPathSrc, fFaceIdxSrc, fFaceVtxSrc[0], fFaceVtxSrc[1], faceTraversal,
					cvMapping, cvMappingInverse, 
					newPolygonCounts, newPolygonConnects, 
					origVertices, newVertices );
	if ( stat != MStatus::kSuccess )
	{
		displayError(" could not process all the mesh faces.");
		return stat;
	}

	theMesh.createInPlace( newVertices.length(), newPolygonCounts.length(), newVertices, newPolygonCounts, newPolygonConnects );
	return stat;
}

MStatus meshReorderCommand::setSeedInfo( MDagPath& path, int face, int vtx0, int vtx1 )
{
	fFaceIdxSrc = face;
	fFaceVtxSrc.setLength(2);
	fFaceVtxSrc[0] = vtx0;
	fFaceVtxSrc[1] = vtx1;
	fDagPathSrc = path;

	return MStatus::kSuccess;
}
