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
//

//polyRawWriter.cpp

//General Includes
//
#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MPlug.h>
#include <maya/MIOStream.h>
#include <time.h>

//Iterator Includes
//
#include <maya/MItMeshPolygon.h>


//Header File
//
#include "polyRawWriter.h"

//Macros
//
#define DELIMITER "\t"
#define SHAPE_DIVIDER "*******************************************************************************\n"
#define HEADER_LINE "===============================================================================\n"
#define LINE "-------------------------------------------------------------------------------\n"


polyRawWriter::polyRawWriter(const MDagPath& dagPath, MStatus& status):
polyWriter(dagPath, status),
fHeadUVSet(NULL)
//Summary:	creates and initializes an object of this class
//Args   :	dagPath - the DAG path of the current node
//			status - will be set to MStatus::kSuccess if the constructor was
//					 successful;  MStatus::kFailure otherwise
{
}


polyRawWriter::~polyRawWriter() 
//Summary:  deletes the objects created by this class
{
	if (NULL != fHeadUVSet) delete fHeadUVSet;
}


MStatus polyRawWriter::extractGeometry() 
//Summary:	extracts main geometry as well as all UV sets and each set's
//			coordinates
{

	if (MStatus::kFailure == polyWriter::extractGeometry()) {
		return MStatus::kFailure;
	}

	MStringArray uvSetNames;
	if (MStatus::kFailure == fMesh->getUVSetNames(uvSetNames)) {
		MGlobal::displayError("MFnMesh::getUVSetNames"); 
		return MStatus::kFailure;
	}

	unsigned int uvSetCount = uvSetNames.length();
	unsigned int i;

	UVSet* currUVSet = NULL;

	for (i = 0; i < uvSetCount; i++ ) {
		if (0 == i) {
			currUVSet = new UVSet;
			fHeadUVSet = currUVSet;
		} else {
			currUVSet->next = new UVSet;
			currUVSet = currUVSet->next;
		}

		currUVSet->name = uvSetNames[i];
		currUVSet->next = NULL;

		// Retrieve the UV values
		//
		if (MStatus::kFailure == fMesh->getUVs(currUVSet->uArray, currUVSet->vArray, &currUVSet->name)) {
			return MStatus::kFailure;
		}
	}

	return MStatus::kSuccess;
}


MStatus polyRawWriter::writeToFile(ostream& os) 
//Summary:	outputs the geometry of this polygonal mesh in raw text format
//Args   :	os - an output stream to write to
//Returns:  MStatus::kSuccess if the method succeeds
//			MStatus::kFailure if the method fails
{
	MGlobal::displayInfo("Exporting " + fMesh->partialPathName());

	os << SHAPE_DIVIDER;
	os << "Shape:  " << fMesh->partialPathName() << "\n";
	os << SHAPE_DIVIDER;
	os << "\n";

	if (MStatus::kFailure == outputFaces(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputVertices(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputVertexInfo(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputNormals(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputTangents(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputBinormals(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputColors(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputUVs(os)) {
		return MStatus::kFailure;
	}

	if (MStatus::kFailure == outputSets(os)) {
		return MStatus::kFailure;
	}
	os << "\n\n";
	
	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputFaces(ostream& os) 
//Summary:	outputs the vertex indices that comprise each face
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all faces were outputted
//			MStatus::kFailure otherwise
{
	unsigned int faceCount = fMesh->numPolygons();
	if (0 == faceCount) {
		return MStatus::kFailure;
	}

	MStatus status;
	MIntArray indexArray;

	os << "Faces:  " << faceCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Index|Vertex Indices\n";
	os << LINE;

	unsigned int i;
	for (i = 0; i < faceCount; i++) {
		os << i << DELIMITER;

		unsigned int indexCount = fMesh->polygonVertexCount(i, &status);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MFnMesh::polygonVertexCount");
			return MStatus::kFailure;
		}

		status = fMesh->getPolygonVertices (i, indexArray);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MFnMesh::getPolygonVertices");
			return MStatus::kFailure;
		}

		unsigned int j;
		for (j = 0; j < indexCount; j++) {
			os << indexArray[j] << " ";
		}

		os << "\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputVertices(ostream& os) 
//Summary:	outputs all vertex coordinates
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all vertex coordinates were outputted
//			MStatus::kFailure otherwise
{
	unsigned int vertexCount = fVertexArray.length();
	unsigned i;

	if (0 == vertexCount) {
		return MStatus::kFailure;
	}

	os << "Vertices:  " << vertexCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Vertex|(x, y, z)\n";
	os << LINE;
	for (i = 0; i < vertexCount; i++) {
		os << i << DELIMITER << "(" 
		   << fVertexArray[i].x << ", " 
		   << fVertexArray[i].y << ", " 
		   << fVertexArray[i].z << ")\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputVertexInfo(ostream& os) 
//Summary:	outputs the per face per vertex information such as normal, color, and uv set
//			indices
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all per face per vertex information was outputted
//			MStatus::kFailure otherwise
{
	unsigned int faceCount = fMesh->numPolygons();
	unsigned i, j, indexCount;

	MStatus status;
	MIntArray indexArray;

	//output the header
	os << "Vertex Info:\n";
	os << HEADER_LINE;
	os << "Format:  Face|faceVertexIndex|vertexIndex|normalIndex|colorIndex|";
	
	//Add each uv set to the header
	UVSet* currUVSet;
	for (currUVSet = fHeadUVSet; currUVSet != NULL; currUVSet = currUVSet->next) {
		os << "| UV_" << currUVSet->name;
	}
	os << "\n";

	os << LINE;

	MIntArray normalIndexArray;
	int colorIndex, uvID;

	for (i = 0; i < faceCount; i++) {

		indexCount = fMesh->polygonVertexCount(i, &status);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MFnMesh::polygonVertexCount");
			return MStatus::kFailure;
		}

		status = fMesh->getPolygonVertices (i, indexArray);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MFnMesh::getPolygonVertices");
			return MStatus::kFailure;
		}

		status == fMesh->getFaceNormalIds (i, normalIndexArray);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MFnMesh::getFaceNormalIds");
			return MStatus::kFailure;
		}

		for (j = 0; j < indexCount; j++) {
			status = fMesh->getFaceVertexColorIndex(i, j, colorIndex);

			//output the face, face vertex index, vertex index, normal index, color index
			//for the current vertex on the current face
			os << i << DELIMITER << j << DELIMITER << indexArray[j] << DELIMITER
			   << normalIndexArray[j] << DELIMITER << colorIndex << DELIMITER;

			//output each uv set index for the current vertex on the current face
			for (currUVSet = fHeadUVSet; currUVSet != NULL; currUVSet = currUVSet->next) {
				status = fMesh->getPolygonUVid(i, j, uvID, &currUVSet->name);
				if (MStatus::kFailure == status) {
					MGlobal::displayError("MFnMesh::getPolygonUVid");
					return MStatus::kFailure;
				}
				os << DELIMITER << uvID;
			}
			os << "\n";
		}

		os << "\n";
	}
	os << "\n";

	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputNormals(ostream& os) 
//Summary:	outputs the normals for this polygonal mesh
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all normals were outputted
//			MStatus::kFailure otherwise
{
	unsigned int normalCount = fNormalArray.length();
	if (0 == normalCount) {
		return MStatus::kFailure;
	}

	os << "Normals:  " << normalCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Index|[x, y, z]\n";
	os << LINE;

	unsigned int i;
	for (i = 0; i < normalCount; i++) {
		os << i << DELIMITER << "["
		   << fNormalArray[i].x << ", "
		   << fNormalArray[i].y << ", "
		   << fNormalArray[i].z << "]\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}

MStatus polyRawWriter::outputTangents(ostream& os) 
//Summary:	outputs the normals for this polygonal mesh
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all normals were outputted
//			MStatus::kFailure otherwise
{
	unsigned int tangentCount = fTangentArray.length();
	if (0 == tangentCount) {
		return MStatus::kFailure;
	}

	os << "Tangents:  " << tangentCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Index|[x, y, z]\n";
	os << LINE;

	unsigned int i;
	for (i = 0; i < tangentCount; i++) {
		os << i << DELIMITER << "["
		   << fTangentArray[i].x << ", "
		   << fTangentArray[i].y << ", "
		   << fTangentArray[i].z << "]\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}

MStatus polyRawWriter::outputBinormals(ostream& os) 
//Summary:	outputs the normals for this polygonal mesh
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all normals were outputted
//			MStatus::kFailure otherwise
{
	unsigned int binormalCount = fBinormalArray.length();
	if (0 == binormalCount) {
		return MStatus::kFailure;
	}

	os << "Binormals:  " << binormalCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Index|[x, y, z]\n";
	os << LINE;

	unsigned int i;
	for (i = 0; i < binormalCount; i++) {
		os << i << DELIMITER << "["
		   << fBinormalArray[i].x << ", "
		   << fBinormalArray[i].y << ", "
		   << fBinormalArray[i].z << "]\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}

MStatus polyRawWriter::outputColors(ostream& os) 
//Summary:	outputs the colors for this polygonal mesh
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if all colors were outputted
//			MStatus::kFailure otherwise
{
	unsigned int colorCount = fColorArray.length();
	if (0 == colorCount) {
		return MStatus::kFailure;
	}

	os << "Colors:  " << colorCount << "\n";
	os << HEADER_LINE;
	os << "Format:  Index|R G B A\n";
	os << LINE;
	
	unsigned int i;
	for (i = 0; i < colorCount; i++) {
		os << i << DELIMITER
		   << fColorArray[i].r << " "
		   << fColorArray[i].g << " "
		   << fColorArray[i].b << " "
		   << fColorArray[i].a << "\n";
	}
	os << "\n\n";

	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputUVs(ostream& os) 
//Summary:	for each UV Set, outputs all uv coordinates
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if UV coordinates for all UV sets were outputted
//			MStatus::kFailure otherwise
{
	UVSet* currUVSet;
	unsigned int i, uvCount;
	for (currUVSet = fHeadUVSet; currUVSet != NULL; currUVSet = currUVSet->next) {
		if (currUVSet->name == fCurrentUVSetName) {
			os << "Current ";
		}

		os << "UV Set:  " << currUVSet->name << "\n";
		uvCount = currUVSet->uArray.length();
		os << "UV Count:  " << uvCount << "\n";
		os << HEADER_LINE;
		os << "Format:  Index|(u, v)\n";
		os << LINE;
		for (i = 0; i < uvCount; i++) {
			os << i << DELIMITER << "(" << currUVSet->uArray[i] << ", " << currUVSet->vArray[i] << ")\n";
		}
		os << "\n";
	}
	os << "\n";
	return MStatus::kSuccess;
}


MStatus polyRawWriter::outputSingleSet(ostream& os, MString setName, MIntArray faces, MString textureName)
//Summary:	outputs this mesh's sets and each sets face components, and any 
//			associated texture
//Args   :	os - an output stream to write to
//Returns:	MStatus::kSuccess if set information was outputted
//			MStatus::kFailure otherwise
{
	unsigned int i;
	unsigned int faceCount = faces.length();

	os << "Set:  " << setName << "\n";
	os << HEADER_LINE;
	os << "Faces:  ";
	for (i = 0; i < faceCount; i++) { 
		os << faces[i] << " ";
	}
	os << "\n";
	if (textureName == "") {
		textureName = "none";
	} 
	os << "Texture File: " << textureName << "\n";
	os << "\n\n";
	return MStatus::kSuccess;
}
