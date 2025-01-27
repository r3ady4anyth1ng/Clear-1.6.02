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

/*
  Application: wrl2ma

  Description:
      Converts VRML 97 spec world files to Maya ASCII format

  Inputs:	text file		VRML world file

  Outputs:	text file		Maya ASCII file

  Limitations:
	only handles some of the parameters of the VRML primitives and
	only the sphere, box, cone and cylinder primitives.

  Future Enhancements:

 */
const char	version[] = "3.0";

const char usage_leader[] =
"\nwrl2ma - convert VRML97 (wrl) file to Maya Ascii file - version %s\n";

const char usage_body[] =
"  Using LibVRML97 scene library by Chris Morley" \
"\n  \"Portions of this software are based in part on the VRML Parser" \
"\n  written by Silicon Graphics, Inc., Mountain View, California, USA.\"\n\n" \
"  usage: wrl2ma -i infile.wrl -o outfile.ma\n\n" \
"  options:  -h           \tthis help file\n" \
"            -c           \tforce color per vertex mode\n" \
"            -d           \tdump out new WRL file as parsed\n"
"            -e           \tuse Hard Edges on polygons\n" \
"            -i inputFile \tinput file to convert\n" \
"            -m           \tassume input file was output by Maya\n" \
"            -n           \tuse defined material names\n" \
"            -o outputFile\toutput file to save to\n" \
"            -v           \tverbose messages\n" \
"            -w           \tshow warnings\n\n" \
"  Default settings are: Maya7.0 format primitives, \n" \
"  do Not dump a WRL file as input is read, Use hard edges on polygons,\n" \
"  input is Not from Maya, will generate unique Material names,\n" \
"  No verbose output.\n\n" \
"  Extensions are needed on the file names.\n"
;


#ifdef WIN32
#include "windows.h"
#endif //

#include "MathUtils.h"
#include "VrmlScene.h"
#include "VrmlNodeShape.h"

#include "VrmlMFInt.h"
#include "VrmlNodeNormal.h"
#include "VrmlNodeTextureCoordinate.h"
#include "VrmlNodeCoordinate.h"
#include "VrmlNodeIFaceSet.h"
#include "VrmlNodeTransform.h"
#include "VrmlNodeAppearance.h"
#include "VrmlNodeMaterial.h"
#include "VrmlNodeGeometry.h"
#include "VrmlNodeTexture.h"
#include "VrmlNodeImageTexture.h"
#include "VrmlNodeTextureTransform.h"
#include "VrmlNodeColor.h"
#include "VrmlNodeSphere.h"
#include "VrmlNodeBox.h"
#include "VrmlNodeCone.h"
#include "VrmlNodeCylinder.h"
#include "VrmlNodeProto.h"
#include "VrmlNodeSwitch.h"
#include "VrmlNodeLOD.h"
#include "VrmlNodeILineSet.h"
#include "VrmlNodePointSet.h"
#include "VrmlNodeChild.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#if defined AW_NEW_IOSTREAMS
#  include <iostream>
#else
#  include <iostream.h>
#endif

#ifndef OSMac_
#include <malloc.h>
#endif //OSMac_

#include <string.h>

#ifdef _WIN32
#include <io.h>
#elif defined(OSMac_)
#ifndef atan2f
#define atan2f atan2
#endif // atan2f
#else
#include <unistd.h>
#include <getopt.h>
#endif //_WIN32

#include "AmEdge.h"
#include "AmEdgeList.h"

#ifdef DEBUG_ROTATIONS
#include "maya/mocapserver.h"
#endif

#define kFALSE false
#define kTRUE  true


// Column size of the hash table for polygon edges:
//
#define kHashSize 500

VrmlScene* g_pTheScene;

VrmlNode* root_node;
VrmlNode* node;


char	*defMatName = "Mat";

char	*LinearUnits = "centimeter";
char	*AngularUnits = "degree";
char	*TimeUnits = "film";

double	radToAngular = 180.0/3.14159265;
#if !defined (OSMac_)
double	pi = 3.14159265;
#endif
double	piBy2 = 3.14159265/2.0;

char	current_Mtl[512];
char	current_Name[512];
char	current_fullName[4096];
char	shadingGrp[512];

int		mayaSrc = 0;

bool	verbose = false;
bool	use_HardEdges = true;
bool	force_CPV = false;
bool	use_Names = false;
bool	showWarnings = false;

FILE	*fp;

extern "C" void axis_to_quat( float a[3], float phi, float q[4] );
extern "C" void build_rotmatrix( float m[4][4], float q[4] );

//
//	Define this for Windows
//
//	So that we can have some commonality for option processing
//

#if defined( _WIN32 )
char *  optarg = NULL;
int     optind = 1;
int     opterr = 0;
int     optlast = 0;
#define GETOPTHUH 0

int getopt(int argc, char **argv, char *pargs)
{
    if (optind >= argc) return EOF;

    if (optarg==NULL || optlast==':')
    {
        optarg = argv[optind];
        if (*optarg!='-' && *optarg!='/')
            return EOF;
    }

    if (*optarg=='-' || *optarg=='/') optarg++;
    pargs = strchr(pargs, *optarg);
    if (*optarg) optarg++;
    if (*optarg=='\0')
    {
        optind++;
        optarg = NULL;
    }
    if (pargs == NULL) return 0;  //error
    if (*(pargs+1)==':')
    {
        if (optarg==NULL)
        {
            if (optind >= argc) return EOF;
            // we want a second paramter
            optarg = argv[optind];
        }
        optind++;
    }
    optlast = *(pargs+1);

    return *pargs;
}
#endif  //_WIN32

#define MAXINDENTSPACE 60

int		tabs = 0; // current indent level

char *
indentSpace()
{
	static char spaces[MAXINDENTSPACE];

	int		useTabs = tabs;

	if ( useTabs > MAXINDENTSPACE )
		useTabs = MAXINDENTSPACE;

	for ( int i=0; i<useTabs; i++ )
		spaces[i] = ' ';

	spaces[useTabs] = 0;

	return( spaces );
}

void
verboseName( VrmlNode *node, char *title )
{
if( !node )
	cerr << "**" << title << " Node is NULL" << endl;
else if( verbose )
	{
	cerr << indentSpace() << title << " Node";
	const char * tempName = node->name();
	if( tempName && strlen( tempName ) > 0)
		cerr << " name " << tempName;
	}
}

// The ';' after endl will come from the instance of use
#define verboseNameNL( node, title )	\
	verboseName( node, title );			\
	if ( verbose )						\
		cerr << endl


// The ';' after endl will come from the instance of use
#define verboseMsg( MSG )	\
if ( verbose )				\
	cerr << indentSpace() << (MSG) << endl


int hashFindEdge( AmEdgeList *hashTable,
				  int        startIndex,
				  int        endIndex,
				  double     *startNormal,
				  double	 *endNormal,
				  int        &signEdge,
				  bool       &hardEdge )
//
// Description:
// 	    This function tries to find the edge with a certain
//      start vertex index and a centain end vertex index.
//      The edgeId is returned.
//
// Arguments:
//      hashTable   - edge hash table as describe in function
//                    "printPolyset"
//      startIndex  - index of the start vertex
//      endIndex    - index of the end vertex
//      signEdge    - indicating whether the edge found has
//                    the same direction of the give edge
//
// Notes:
//      Two edges are considered the same even if their signs
//      are opposite.
//
{
	int max = ( ( startIndex < endIndex ) ?
				endIndex : startIndex ) % kHashSize;

	int edgeId = hashTable[max].findEdge( startIndex,
										  endIndex,
										  startNormal,
										  endNormal,
										  signEdge,
										  hardEdge );
	return edgeId;
}

void hashAddEdge( AmEdgeList *hashTable,
				  int        startIndex,
				  int        endIndex,
				  double     *startNormal,
				  double     *endNormal,
				  int        edgeId )
//
// Description:
//      This function adds a new edge to the edge hash table
//      by recording its start vertex index, end vertex index,
//      and the edge Id.
//
// Arguments:
//      hashTable   - edge hash table as described in function
//                    "printPolyset"
//		startIndex  - index of the start vertex
//      endIndex    - index of the end vertex
//      edgeId      - identification of the edge, the order
//                    in which it is processed
//
{
	int max = ( ( startIndex < endIndex ) ?
				endIndex : startIndex ) % kHashSize;

	hashTable[max].addEdge( edgeId, startIndex, endIndex,
							startNormal, endNormal );
}


void printEdges( VrmlNodeIFaceSet *ifsNode, int polyIdx, int Idx,
				 AmEdgeList *hashTable,
				 int        &edgeId,
				 AmEdgeList *edgePrintList )
//
// Description:
//      This function goes through the vertices and
//      builds up the edge table based on the global
//      index of every vertex. The edge formed by a
//      pair of vertices is first checked for existance
//      in the edge table. New edges are added to the
//      edge table with a sign indicating which of the
//      two vertices is the start vertex.
//
// Arguments:
//      ifsNode       - a polygon in the polyset
//		polyIdx		  - current polygon id
//		Idx			  - current Index of vertices/pointers
//      hashTable     - the edge table
//      edgeId        - the identification of a particular edge
//      edgePrintList - a list of edges to be printed out in
//                      the order of processing
//
{
	//if( !polygon )
	//	return;

	VrmlMFInt polygon = ifsNode->getCoordIndex();

	VrmlNode *normals = ifsNode->getNormal();
	VrmlMFVec3f nrmValues;

	int nNorm = 0;
	if ( normals )
    {
		nrmValues = normals->toNormal()->normal();
		nNorm = nrmValues.size();
	}

	VrmlMFInt nrmIndices = ifsNode->getNormalIndex();
	int nNormIdx = nrmIndices.size();

	int startVertex = 0;
	int endVertex   = 0;

	int	numVertices = 0;
	for ( int vtxIdx = Idx; vtxIdx < polygon.size(); vtxIdx++ )
	{
		if ( polygon[vtxIdx] == -1 )
			break;
		else
		{
//			printf(" %d ", polygon[vtxIdx] );
			numVertices++;
		}
	}

//	printf( " number Vertices = %d\n", numVertices );

	int startIndex  = 0;
	int endIndex    = 0;

	double startNormal[3];
	double endNormal[3];
	double nx = 0.0;
	double ny = 0.0;
	double nz = 0.0;

	// Find out if there are normals per vertex or not

	bool nrmPerVertex = ifsNode->getNormalPerVertex();
	int	 useIdx = 0;

	for( int i = Idx; i < Idx+numVertices; i++ )
	{
		startVertex = polygon[i]; //polygon->vertex( i );

		if ( nNorm )
		{
			if ( nrmPerVertex )
			{
				useIdx = i;
			} else {
				useIdx = polyIdx;
			}

			// If we have indices into the normals then we will
			// use them.  else we use the normal values directly

			if ( nNormIdx )
			{
				if ( useIdx >= nNormIdx )
				{
					cerr << "start: Normal Index " << useIdx;
					cerr << " > size of Normal index table" << endl;
					useIdx = 0; // force use of 0th entry in table
				}

				useIdx = nrmIndices[useIdx];
			}

			// Ok, now we have the index into the actual normal table
			// Check that we are not going over its maximum index

			if ( useIdx >= nNorm )
			{
				cerr << "start: Normal index " << useIdx;
				cerr << " > size of Normal table" << endl;
				useIdx = 0;
			}

			startNormal[0] = nrmValues[useIdx][0];
			startNormal[1] = nrmValues[useIdx][1];
			startNormal[2] = nrmValues[useIdx][2];

		}

		startIndex = startVertex + 1;

//		printf( "start vertex = %d, Index = %d", startVertex, startIndex );

		if( i >= Idx+numVertices-1 )
		{
			endVertex = polygon[Idx];
			useIdx = Idx;
		} else {
			endVertex = polygon[i+1];
			useIdx = i+1;
		}

		// Check to see what type of normals we need to deal with
		// normal per vertex or faces.  Also need to check if we
		// are indexing into a table or using the normals directly

		if ( nNorm )
		{
			if ( nrmPerVertex )
			{
				if ( nNormIdx )
				{
					if ( useIdx >= nNormIdx )
					{
					cerr << "end: Normal Index " << useIdx;
					cerr << " > size of Normal index table" << endl;
					useIdx = 0; // force use of 0th entry in table
					}
					useIdx = nrmIndices[useIdx];
				}

				if ( useIdx >= nNorm )
				{
				cerr << "end: Normal index " << useIdx;
				cerr << " > size of Normal table" << endl;
				useIdx = 0;
				}

				if ( nNorm )
				{
					nx = nrmValues[useIdx][0];
					ny = nrmValues[useIdx][1];
					nz = nrmValues[useIdx][2];
				}

			} else {

				// We have normals per polygon, so lets just reuse the
				// original start value;

				nx = startNormal[0];
				ny = startNormal[1];
				nz = startNormal[2];
			}
		}

		endNormal[0] = nx;
		endNormal[1] = ny;
		endNormal[2] = nz;

		endIndex = endVertex + 1;

//		printf( " end Vertex = %d, Index = %d\n", endVertex, endIndex );

		int signEdge = 0;
		bool hardEdge = !nrmPerVertex;
		int idFound = hashFindEdge( hashTable, startIndex, endIndex,
									startNormal, endNormal, signEdge, hardEdge );
		if( -1 == idFound )
		{
//			printf( "add edge: start/end %d/%d, ID = %d\n",
//						startIndex, endIndex, edgeId );

			hashAddEdge( hashTable, startIndex, endIndex,
						 startNormal, endNormal, edgeId );
			signEdge = 1;

			edgePrintList->addEdge( edgeId, startIndex, endIndex,
									startNormal, endNormal );
			edgeId++;
		}
		else if( (hardEdge == true) && use_HardEdges )
			edgePrintList->hardenEdge( idFound );

	}
}

void printFacets( VrmlNodeIFaceSet *ifsNode, int polyIdx, int Idx,
				  AmEdgeList *hashTable,
				  int        &vtCount )
//
// Description:
//      This function prints out the facet information
//      of the polygon.
//
// Arguments:
//      polygon   - a polygon in the polyset
//      hashTable - the edge hash table
//      vtCount   - the index to the vetex texture list
//
{
//	if( NULL == polygon )
//		return;

	VrmlMFInt polygon = ifsNode->getCoordIndex();
	VrmlMFInt texIndices = ifsNode->getTexCoordIndex();
	VrmlMFInt clrIndices = ifsNode->getColorIndex();
	VrmlNodeColor *colors = ifsNode->color();

	int		nClrIndices = clrIndices.size();
	bool	clrPerVertex = ifsNode->getColorPerVertex();

	int startVertex = 0;
	int endVertex   = 0;
	int useIdx 		= 0;

	int	numVertices = 0;

	//printf( "clrPerVertex = %s, nClrIndices = %d\n",
	//	clrPerVertex ? "true":"false", nClrIndices );

	for ( int vtxIdx = Idx; vtxIdx < polygon.size(); vtxIdx++ )
	{
		if ( polygon[vtxIdx] == -1 )
			break;
		else
		{
//			printf(" %d ", polygon[vtxIdx] );
			numVertices++;
		}
	}

	int startIndex  = 0;
	int endIndex    = 0;
	int i           = 0;
	int j           = 0;


	fprintf( fp, "\t\tf\t%d", numVertices );

	int *colorIndex = new int [numVertices];
	//bool hasColor = clrPerVertex;

//	if ( clrIndices.size() > 0 )
//		hasColor = kTRUE;

	for( i = Idx, j = 0; i < Idx+numVertices; i++, j++ )
	{
		startVertex = polygon[i];
		//double r, g, b, a;
		//if( (kFALSE == hasColor) &&
		//	(sFailure != startVertex->color( r, g, b, a ) ) ) {
		//    hasColor = kTRUE;
		//}

		startIndex = startVertex;

		useIdx = i;

		if ( clrPerVertex )
		{
			if ( nClrIndices )
				useIdx = i;
			else
				useIdx = startIndex;		// was i
		} else {
			useIdx = polyIdx;
		}
        if ( nClrIndices )
		{
            useIdx = clrIndices[useIdx];
		}

		colorIndex[j] = useIdx;


		startIndex += 1;


		if( i >= Idx+numVertices-1)
		{
			endVertex = polygon[Idx];
		}
		else
		{
			endVertex = polygon[i+1];
		}

		endIndex = endVertex + 1;

		int signEdge = 0;
		bool hardEdge = kTRUE;
		int idFound = hashFindEdge( hashTable, startIndex,
									endIndex, NULL, NULL, signEdge, hardEdge );
		if( -1 == idFound )
		{
			cerr << "Error: edge of face not found!\n";
		}
		else
		{
			// Using 0-based edgeId, if the edge is reversed,
			// use -(edgeId+1) to avoid -0 edge.
			//
			if( 1 == signEdge )
			{
				fprintf( fp, " %d", (idFound-1)*signEdge );
			}
			else if( -1 == signEdge )
			{
				fprintf( fp, " %d", (idFound-1+1)*signEdge );
			}
		}

	}

	// This is for specifying the uv indices for each
	// vertex on the face.
	//
	if ( texIndices.size() > 0 )
		{
		fprintf( fp, "\tmf\t%d", numVertices );
		//for( i = 0; i < numVertices; i++ )
		for( i = Idx; i < Idx+numVertices; i++ )
			{
			fprintf( fp, " %d", texIndices[i] );
			vtCount++;
			}
	
		}

	else
		{
		VrmlNode *texCoords = ifsNode->getTexCoord();
		if ( texCoords )
			{
			// Here we are supposed to use the Coord Indices if there
			// are no texIndices
			if ( polygon.size() > 0 )
				{
				fprintf( fp, "\tmf\t%d", numVertices );
	        	for( i = Idx; i < Idx+numVertices; i++ )
		        	{
					fprintf( fp, " %d", polygon[i]);
					vtCount++;
					}
				}
			else
				{
				// as a fall back will output index values
				fprintf( fp, "\tmf\t%d", numVertices );
	    	    for( i = Idx; i < Idx+numVertices; i++ )
		    	    {
	    	        fprintf( fp, " %d", i);
					vtCount++;
					}
				}
			}
		}

	// There is 1 material node for each Indexed Face Set
	// But the Color Node and Material Node may both be defined
	// Here we want the diffuse color to be from the color node
	// and not the Material.  For now instead of somehow generating
	// multiple shaders will override the colour per vertex
	// This is a user option at the moment.

	// Also need to check that we have colours to output

	if( colors && (clrPerVertex || force_CPV) ) {
	    fprintf( fp, "\tfc\t%d", numVertices );
	    for( i = 0; i < numVertices; i++ )
		{
		    fprintf( fp, " %d", colorIndex[i] );
		}
	}
	fprintf( fp, "\n" );

	if( NULL != colorIndex ) {
	    delete [] colorIndex;
	}

#if 0
	printFile( "%s;\n", indentSpace() );
#endif
}


void outputHeader( char *fname )
{
	char	versionNum[32];
	
	time_t ltime;

    time( &ltime );

	strcpy( versionNum, "7.0" );
		
	fprintf( fp, "//Maya ASCII %s scene\n", versionNum);
	fprintf( fp, "//Name: %s\n", fname );
	fprintf( fp, "//Created by wrl2ma Version %s\n", version );
	fprintf( fp, "//Last modified: %s\n", ctime( &ltime ) );

	fprintf( fp, "requires maya \"%s\";\n", versionNum );
	fprintf( fp, "currentUnit -l %s -a %s -t %s;\n",
					LinearUnits, AngularUnits, TimeUnits );

	fprintf( fp, "\n" );

	fprintf( fp, "createNode lightLinker -n \"lightLinker1\";\n" );
	fprintf( fp,
"connectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[0].llnk\";\n" );
	fprintf( fp,
"connectAttr \":initialShadingGroup.msg\" \"lightLinker1.lnk[0].olnk\";\n" );
	fprintf( fp,
"connectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[1].llnk\";\n" );
	fprintf( fp,
"connectAttr \":initialParticleSE.msg\" \"lightLinker1.lnk[1].olnk\";\n" );

	fprintf( fp, "\n" );

}

int uniqueID()
{
	static int id = 0;

	id++;

	return id;
}

int	findNumFaces( VrmlMFInt &cindex )
{
	int nfaces = 0;

	for ( int i=0; i < cindex.size(); i++ )
	{
		if ( cindex[i] == -1 )
			nfaces++;
	}

	return nfaces;
}

//
//	Will need to change this to save the material info
//	and then check the actual values to see if they are the same
//	if they are then reuse the old name.
//
//	This current check of material pointers doesn't work.

struct matInfo {
	float dColor[3];
	float dEmissive[3];
	float dShininess;
	float dSpecular[3];
	float dTransparency;
	char  *mtlName;
	char  *texName;
} matInfo;

bool findMaterial( char *mtlName, VrmlNodeMaterial *mtl, char *texName )
{
	static int numMaterials = 0;
	static int maxMaterials = 0;
	static struct matInfo *known_MtlsInfo = 0;

	float *dColor;
	float *dEmissive;
	float dShininess;
	float *dSpecular;
	float dTransparency;

	if ( mtl )
	{
		dColor		= mtl->diffuseColor();
		dEmissive	= mtl->emissiveColor();
		dShininess	= mtl->shininess();
		dSpecular	= mtl->specularColor();
		dTransparency	= mtl->transparency();
	} else {
		cerr << "Material pointer is Null" << endl;
		return false;
	}

	//cerr << "checking material " << mtl << endl;

	bool found = false;

	for ( int i=0; i < numMaterials; i++ )
	{
		//cerr << "check material " << i << " " << mtlName << endl;

		if ( !strcmp( known_MtlsInfo[i].mtlName, mtlName ) )
		{
			return true;
		} else {

			if ( FPEQUAL(dColor[0],known_MtlsInfo[i].dColor[0]) &&
				 FPEQUAL(dColor[1],known_MtlsInfo[i].dColor[1]) &&
				 FPEQUAL(dColor[2],known_MtlsInfo[i].dColor[2]) &&
				 FPEQUAL(dEmissive[0],known_MtlsInfo[i].dEmissive[0]) &&
				 FPEQUAL(dEmissive[1],known_MtlsInfo[i].dEmissive[1]) &&
				 FPEQUAL(dEmissive[2],known_MtlsInfo[i].dEmissive[2]) &&
				 FPEQUAL(dSpecular[0],known_MtlsInfo[i].dSpecular[0]) &&
				 FPEQUAL(dSpecular[1],known_MtlsInfo[i].dSpecular[1]) &&
				 FPEQUAL(dSpecular[2],known_MtlsInfo[i].dSpecular[2]) &&
				 FPEQUAL(dShininess,known_MtlsInfo[i].dShininess) &&
				 FPEQUAL(dTransparency,known_MtlsInfo[i].dTransparency)  )
			{
				//cerr << "found same material node " << known_Mtls[i] << endl;
				
				found = false;
				if ( NULL == texName && NULL == known_MtlsInfo[i].texName )
					found = true;
				else if ( texName && known_MtlsInfo[i].texName )
				{
					if ( !strcmp( texName, known_MtlsInfo[i].texName ) )
						found = true;
					else
						found = false;
				} else
					found = false;

				if ( found )
				{
					strcpy( current_Mtl, known_MtlsInfo[i].mtlName );
					strcpy( shadingGrp, current_Mtl );
					strcat( shadingGrp, "SG" );
					return true;
				}
			}
		}	
	}


	if ( numMaterials+1 > maxMaterials )
	{
		known_MtlsInfo = (struct matInfo *)realloc( known_MtlsInfo, sizeof(struct matInfo ) * (numMaterials+10) );
		maxMaterials = numMaterials+10;
	}

	known_MtlsInfo[numMaterials].dColor[0] = dColor[0];
	known_MtlsInfo[numMaterials].dColor[1] = dColor[1];
	known_MtlsInfo[numMaterials].dColor[2] = dColor[2];
	known_MtlsInfo[numMaterials].dEmissive[0] = dEmissive[0];
	known_MtlsInfo[numMaterials].dEmissive[1] = dEmissive[1];
	known_MtlsInfo[numMaterials].dEmissive[2] = dEmissive[2];
	known_MtlsInfo[numMaterials].dSpecular[0] = dSpecular[0];
	known_MtlsInfo[numMaterials].dSpecular[1] = dSpecular[1];
	known_MtlsInfo[numMaterials].dSpecular[2] = dSpecular[2];
	known_MtlsInfo[numMaterials].dShininess = dShininess;
	known_MtlsInfo[numMaterials].dTransparency = dTransparency;
	
	if ( mtlName )
		known_MtlsInfo[numMaterials].mtlName = strdup( mtlName );
	else 
		known_MtlsInfo[numMaterials].mtlName = NULL;
	
	if ( texName )
		known_MtlsInfo[numMaterials].texName = strdup(texName);
	else
		known_MtlsInfo[numMaterials].texName = NULL;
	
	numMaterials++;

	return false;
}




char *outputTexture( VrmlNode *node, char *texXF )
{
#define MAXTEXTURENAME 1024
#define MAXCMDSTR 1024

	static char	texName[MAXTEXTURENAME];
	char		cmd[MAXCMDSTR] = "connectAttr -f ";

	VrmlNodeImageTexture *imageTex = node->toImageTexture();

	verboseNameNL( node, "Texture" );

	sprintf( texName, "file%d", uniqueID() );

	fprintf( fp, "createNode file -n %s;\n", texName );

	if( texXF && *texXF )
		{
		strncat( cmd, texXF, MAXCMDSTR );

		fprintf( fp, "%s.coverage %s.coverage;\n", cmd, texName );
		fprintf( fp, "%s.translateFrame %s.translateFrame;\n", cmd, texName );
		fprintf( fp, "%s.rotateFrame %s.rotateFrame;\n", cmd, texName );
		fprintf( fp, "%s.mirrorU %s.mirrorU;\n", cmd, texName );
		fprintf( fp, "%s.mirrorV %s.mirrorV;\n", cmd, texName );
		fprintf( fp, "%s.stagger %s.stagger;\n", cmd, texName );
		fprintf( fp, "%s.wrapU %s.wrapU;\n", cmd, texName );
		fprintf( fp, "%s.wrapV %s.wrapV;\n", cmd, texName );
		fprintf( fp, "%s.repeatUV %s.repeatUV;\n", cmd, texName );
		fprintf( fp, "%s.offset %s.offset;\n", cmd, texName );
		fprintf( fp, "%s.rotateUV %s.rotateUV;\n", cmd, texName );
		fprintf( fp, "%s.vertexUvOne %s.vertexUvOne;\n", cmd, texName );
		fprintf( fp, "%s.vertexUvTwo %s.vertexUvTwo;\n", cmd, texName );
		fprintf( fp, "%s.vertexUvThree %s.vertexUvThree;\n", cmd, texName );
		fprintf( fp, "%s.vertexCameraOne %s.vertexCameraOne;\n", cmd, texName );
		fprintf( fp, "%s.outUV %s.uv;\n", cmd, texName );
		fprintf( fp, "%s.outUvFilterSize %s.uvFilterSize;\n", cmd, texName );
		}

	if ( imageTex )
	{
		VrmlMFString url = imageTex->getUrl();
		if( verbose )
			cerr << indentSpace() << "  URL " << url << url.size() << endl;

		if ( url.size() > 0 )
		{
			char *urlString = url.get(0);
			if ( urlString )
				fprintf( fp,
"setAttr %s.fileTextureName -type \"string\" \"%s\";\n",
						texName, url.get(0) );
		}
	}


	// Connect up the attributes so that the textures are seen.

	fprintf( fp,
"%s.msg \":defaultRenderUtilityList1.u\" -na;\n",
			cmd );
	fprintf( fp,
"connectAttr -f \"%s.msg\" \":defaultTextureList1.tx\" -na;\n",
			texName);

	if( verbose )
		cerr << indentSpace() << "  returning name " << texName << endl;
	return texName;
}

char * outputTextureTransform( VrmlNode *node )
{
	//float	*dCenter;
	float	dRotation;
	float	*dScale;
	float	*dTranslation;

	float	defTranslate[2]= {0.,0.};
	float	defScale[2] = {1.,1.};
	float	defRotate = 0.;
	//float	defCenter[2] = {0.,0.};

	static char	texXFName[1024];

	verboseNameNL( node, "Texture Transform" );

	sprintf( texXFName, "place2dTexture%d", uniqueID() );
	fprintf( fp, "createNode place2dTexture -n %s;\n", texXFName );

	if ( node )
	{
		VrmlNodeTextureTransform *texXF = node->toTextureTransform();

		//dCenter = texXF->center();
		dRotation = texXF->rotation();
		dScale = texXF->scale();
		dTranslation = texXF->translation();
	} else {
		//dCenter = defCenter;
		dRotation = defRotate;
		dScale = defScale;
		dTranslation = defTranslate;
	}

	fprintf( fp, "setAttr %s.rotateUV %f;\n", texXFName, dRotation );
	fprintf( fp, "setAttr %s.offset %f %f;\n",
						texXFName, dTranslation[0], dTranslation[1] );
	fprintf( fp, "setAttr %s.repeatUV %f %f;\n",
						texXFName, dScale[0], dScale[1] );

	if( verbose )
		cerr << indentSpace() << "  ret name " << texXFName << endl;
	return texXFName;
}

void outputMaterial( VrmlNode *node, char *texName )
{
#define MAXMTLNAME 1024

	static int matLightId = 2;
	char	*ending;
	char	useMtl[MAXMTLNAME];
	int		useIndex;
	bool  matIsLambert = true;

	// take the materials node name 
	const char *mtlName = node->name();
	VrmlNodeMaterial *mtl = node->toMaterial();

	verboseNameNL( mtl, "Material" );


	if ( use_Names && mtlName && *mtlName )
		strncpy( useMtl, mtlName, MAXMTLNAME );
	else
		sprintf( useMtl, "Mat%d", uniqueID() );

	if ( mayaSrc )
		{
		// if it is a Maya generated name
		// strip off the _# suffixes if any
		ending = strrchr( useMtl, '_' );
		if ( ending )
		{
			int numFound = sscanf( ending, "_%d", &useIndex );
			if ( numFound == 1 )
			{
				*ending = 0;
				strcpy( current_Mtl, useMtl );	// copy to global
			}
		} else
			// no suffix, just copy
			strcpy( current_Mtl, useMtl );	// copy to global

		// check to see if the shader name is lambert1 this is the
		// default name of the initialShadingGroup material, don't want
		// to mess with this.

		strcpy( shadingGrp, current_Mtl );	// copy to global
		strcat( shadingGrp, "SG" );

		if ( !strcmp( current_Mtl, "lambert1" ) )
			strcpy( shadingGrp, "initialShadingGroup" ); // copy to global

	} else
		{
		// not from Maya so no worries on suffixes
		strcpy( shadingGrp, useMtl );	// copy to global
		strcat( shadingGrp, "SG" );		// add suffix to SG global
		strcpy( current_Mtl, useMtl );	// copy to global
	}

	// Need to check that the name used follows some of Maya's rules

	int z;
	for (z=0; z < (int)strlen(current_Mtl); z++ )
	{
		if ( current_Mtl[z] == '#' || current_Mtl[z] == ':' || current_Mtl[z] == '-' )
		{
			current_Mtl[z] = '_';
		}
	}

	for (z=0; z < (int)strlen(shadingGrp); z++ )
    {
        if ( shadingGrp[z] == '#' || shadingGrp[z] == ':' || shadingGrp[z] == '-' )
        {
            shadingGrp[z] = '_';
        }
    }

	//
	// Need to check that we haven't already created this material.
	// If we have then it is ok to just return;
	//

	if ( !findMaterial( current_Mtl, mtl, texName ) )	// test against global
	{
		// Create the material node
		// May have to check to see if there is specular colour defined
		// if so then set phong shader instead of lambert.
		//
		// diffuse colour is colour * diffuse coefficient
		// default diffuse coeffiecient is usually 0.8

		// these are only used in this scope
		//float dAmbientInt	= mtl->ambientIntensity();
		float *dColor		= mtl->diffuseColor();
		float *dEmissive	= mtl->emissiveColor();
		float dShininess	= mtl->shininess();
		float *dSpecular	= mtl->specularColor();
		float dTransparency	= mtl->transparency();

		if ( FPEQUAL(dSpecular[0],0.0) &&
			 FPEQUAL(dSpecular[1],0.0) &&
			 FPEQUAL(dSpecular[2],0.0) )
		{
			matIsLambert = true;
			if ( strcmp( shadingGrp, "initialShadingGroup" ) ) // sg global
			{
				fprintf( fp, "createNode lambert -n \"%s\";\n",
						current_Mtl );	// use global current_Mtl
			} else {
				fprintf( fp, "select \"%s\";\n", current_Mtl);
			}
		} else
			{
			// it is a Phong or Blinn
			matIsLambert = false;
			fprintf( fp, "createNode phong -n \"%s\";\n",
					current_Mtl );	// use global current_Mtl
		}

		fprintf( fp, "setAttr \".c\" -type \"float3\" %f %f %f;\n",
			dColor[0]/0.8, dColor[1]/0.8, dColor[2]/0.8 );

		fprintf( fp, "setAttr \".diffuse\" 0.8;\n" );

		fprintf( fp,
"setAttr \".incandescence\" -type \"float3\" %f %f %f;\n",
			dEmissive[0], dEmissive[1], dEmissive[2] );

		// dTransparency is a single float value.
		fprintf( fp,
"setAttr \".transparency\" -type \"float3\" %f %f %f;\n",
			dTransparency, dTransparency, dTransparency );

		if ( !matIsLambert )
		{
			fprintf( fp, "setAttr \".cosinePower\" %f;\n",
				dShininess*20 );

			fprintf( fp,
"setAttr \".specularColor\" -type \"float3\" %f %f %f;\n",
				dSpecular[0], dSpecular[1], dSpecular[2] );
		}

		//
		// if this isn't the initialShadingGroup them don't do this
		//

		if ( strcmp( shadingGrp, "initialShadingGroup" ) ) // sg global
		{
#if 0
			fprintf( fp,
"sets -renderable true -noSurfaceShader true -empty -name %sSG;\n",
					current_Mtl );	// use global current_Mtl
#else
			fprintf( fp, "createNode shadingEngine -n \"%s\";\n",
					shadingGrp );	// use global shadingGrp
			fprintf( fp, "\tsetAttr \".ihi\" 0;\n" );
			fprintf( fp, "\tsetAttr \".ro\" yes;\n" );
#endif

			fprintf( fp, "createNode \"materialInfo\" -n \"MI_%s\";\n",
					current_Mtl );	// use global current_Mtl

			fprintf( fp,
"connectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[%d].llnk\";\n",
					matLightId );

			fprintf( fp,
"connectAttr \"%s.msg\" \"lightLinker1.lnk[%d].olnk\";\n",
					shadingGrp, matLightId );	// use global shadingGrp
			matLightId++;

			//fprintf( fp, "connectAttr \"%s.oc\" \"%s.ss\";\n",
			//	current_Mtl, shadingGrp );	// use globals

			fprintf( fp, "connectAttr \"%s.msg\" \"MI_%s.sg\";\n",
					shadingGrp, current_Mtl );	// use globals

			fprintf( fp,
"connectAttr \"%s.pa\" \":renderPartition.st\" -na;\n",
				shadingGrp );	// use global shadingGrp
			fprintf( fp,
"connectAttr \"%s.msg\" \":defaultShaderList1.s\" -na;\n",
				current_Mtl );	// use global current_Mtl

			fprintf( fp, "connectAttr \":defaultLightList1.ltd\" \"%s.dl\";\n",
					shadingGrp );	// use global shadingGrp
			fprintf( fp, "connectAttr -f %s.outColor %s.surfaceShader;\n",
					current_Mtl, shadingGrp );	// use globals
		}

		// Now check to see if there was a texture to connect to the color

		if ( texName && *texName )
			fprintf( fp, "connectAttr -f %s.outColor %s.color;\n",
					texName, current_Mtl );	// use global current_Mtl
	} else {

		// We found the same material else where so should update
		// the name of the global material to use.  probably not this
		// same one, if the nodes are not all named.

		// strcpy( current_mtl, ... );
	}
}



static void
normalize_quat(float q[4])
{
    int i;
    float mag;

    mag = (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    for (i = 0; i < 4; i++) q[i] /= mag;
}

// Something to add for the Windows side

#ifdef WIN32
double copysign( double x, double y)
{
	if ( y < 0.0 )
		return( -fabs(x) );
	else
		return( fabs(x) );
}
#endif

void outputTransform(
		VrmlNode	*node,
		char		*name,
		char		*parent,
		char		*fullParent )
{
	VrmlNodeTransform *tnode = node->toTransform();
	VrmlSFVec3f tmpValue;
	VrmlSFRotation tmpRot;

	float *T;
	float *R;
	float *S;

	verboseNameNL( node, "Transform" );

	fprintf( fp, "createNode transform" );
	if ( name && *name )
		fprintf( fp, " -n \"%s\"", name );
	if ( /*tabs > 0 &&*/ fullParent && *fullParent )
		fprintf( fp, " -p \"%s\"", fullParent );
	fprintf( fp, ";\n" );


	// Now output the TRS values if needed

	tmpValue = tnode->getTranslation();
	T = tmpValue.get();

    fprintf( fp, "\tsetAttr \".t\" -type \"double3\" %f %f %f;\n",
                T[0], T[1], T[2] );

	tmpRot = tnode->getRotation();
	R = tmpRot.get();



	float Rsign = 1.;
	
	// Check for special case.  All zeros

	if ( FPEQUAL(R[0], 0.) && FPEQUAL(R[1], 0.) && FPEQUAL(R[2], 0.) )
	{
        fprintf( fp, "\tsetAttr \".r\" -type \"double3\" %f %f %f;\n",
                R[0] * radToAngular,
                R[1] * radToAngular,
                R[2] * radToAngular );
	
	} else if ( FPEQUAL(R[1], 0.) && FPEQUAL(R[2], 0.) ) {
	
		if ( R[0] < 0. ) Rsign = -1.;
		fprintf( fp, "\tsetAttr \".r\" -type \"double3\" %f %f %f;\t//$$$$\n",
				Rsign*R[3] * radToAngular, 0., 0. );

	} else if ( FPEQUAL(R[0], 0.) && FPEQUAL(R[2], 0.) ) {
		
		if ( R[1] < 0.) Rsign = -1.;
		fprintf( fp, "\tsetAttr \".r\" -type \"double3\" %f %f %f;\t//$$$$\n",
				0., Rsign*R[3] * radToAngular, 0. );

	} else if ( FPEQUAL(R[0], 0.) && FPEQUAL(R[1], 0.) ) {

		if ( R[2] < 0.) Rsign = -1.;
		fprintf( fp, "\tsetAttr \".r\" -type \"double3\" %f %f %f;\t//$$$$\n",
				0., 0., Rsign*R[3] * radToAngular );

	} else {

		// Convert the VRML axis, rotation to Euler angles for Maya

        float q[4];
        float m[4][4];
		float rot[3];

		axis_to_quat( R, R[3], q );
		normalize_quat( q );
		build_rotmatrix( m, q );

#ifdef OLD_ROT_METHOD

		float r[3][3];
				
		// Make this into what I would normally use

        r[0][0] = m[0][0];
        r[1][0] = m[0][1];
        r[2][0] = m[0][2];
        r[0][1] = m[1][0];
        r[1][1] = m[1][1];
        r[2][1] = m[1][2];
        r[0][2] = m[2][0];
        r[1][2] = m[2][1];
        r[2][2] = m[2][2];

		// extract the rotation angles

	    rot[1] = asin( -r[0][2]);
    	if ( fabs( cos( rot[1])) > 0.0001) {
    	    rot[0] = asin( r[1][2]/cos( rot[1]));
    	    rot[2] = asin( r[0][1]/cos( rot[1]));
    	} else {
    	    rot[0] = acos( r[1][1]);
    	    rot[2] = 0.0f;
    	}
#else
		// New improved method

		float		m11, m12, m13;
		float		m22, m23;
		float		m32, m33;
		float		zr;
		//float		sxr;
		//float 	cxr;
		float 		yr;
		//float		syr, cyr;
		float 		xr,  szr,  czr;
		static float epsilon = 1.0e-5;

		m11 = m[0][0];
		m12 = m[1][0];
		m13 = m[2][0];

		m22 = m[1][1];
		m23 = m[2][1];

		m32 = m[1][2];
		m33 = m[2][2];

		szr = m12;
		if (fabs(double(szr)) > 1.0) szr = (float)copysign(1.0, szr);
		czr = sqrt(1 - szr * szr);
		if (czr < epsilon)
		{
			// Insufficient accuracy, assume that zr = +/- PI/2 && yr = 0 
			xr = atan2f(m23, m33);
			yr = 0.0;
			zr = (szr > 0.0f) ? (float)M_PI_2 : (float)-M_PI_2;
		} else {
			xr = atan2f(-m32, m22);
			yr = atan2f(-m13, m11);
			zr = atan2f(szr,  czr);
		}

        rot[0] = xr;
        rot[1] = yr;
        rot[2] = zr;
#endif

		// Lets do a check to see if the rotation that we got is 
		// a valid floating point number.  else will output as 0xnan10000
		// or -1#IND (on NT), If that happens force a zero value.

		char  nanCheck[64];
		bool  nanFound = false;

    	fprintf( fp, "\tsetAttr \".r\" -type \"double3\" " );
				
	    sprintf( nanCheck, "%f", rot[0]*radToAngular );
		if ( NULL != strstr( nanCheck, "IND" ) || 
			 NULL != strstr( nanCheck, "nan") )
		{
			fprintf( fp, "0 " );
			nanFound = true;
		}
		else
			fprintf( fp, " %s ", nanCheck );

        sprintf( nanCheck, "%f", rot[1]*radToAngular );
        if ( NULL != strstr( nanCheck, "IND" ) || 
			 NULL != strstr( nanCheck, "nan") )
        {
			fprintf( fp, "0 " );
			nanFound = true;
		}
        else
            fprintf( fp, " %s ", nanCheck );

        sprintf( nanCheck, "%f", rot[2]*radToAngular );
        if ( NULL != strstr( nanCheck, "IND" ) || 
			 NULL != strstr( nanCheck, "nan") )
        {
			fprintf( fp, "0 " );
			nanFound = true;
		}
        else
            fprintf( fp, " %s ", nanCheck );

		if ( nanFound ) {
			fprintf( fp, ";\t// nan generated\n" );
		}
		else
			fprintf( fp, ";\n" );


	}

	tmpValue = tnode->getScale();
	S = tmpValue.get();

	fprintf( fp, "\tsetAttr \".s\" -type \"double3\" %f %f %f;\n",
				S[0], S[1], S[2] );

}

//	Some routines that we haven't converted, 
//	either there is no Maya equivalent, or 
//	haven't gotten around to doing this yet.

void outputILineSetGeo( VrmlNodeGeometry *node )
{
	static bool outFlag = false;
	
	if ( showWarnings )
	{
		if ( !outFlag )
		{
			cerr << "Sorry, Indexed Line Sets are not supported" << endl;
			outFlag = true;
		}
	}
}

void outputPointSetGeo( VrmlNodeGeometry *node )
{
	static bool outFlag = false;
	
	if ( showWarnings )
	{
		if ( !outFlag )
		{
			cerr << "Sorry, Point Sets are not supported" << endl;
			outFlag = true;
		}
	}
}



//	Some routines that we have converted over to Maya

void outputShapeNode( VrmlNode *node, int shapeDone, char *name,
									char *parent, char *fullParent )
{
	char	useName[256];

	verboseNameNL( node, "Geometry" );

	// name is the Shape name (jjh I wonder if this is dangerous?)
	if ( name && *name )
		strcpy( useName, name );

	else if ( parent && *parent )
		//if ( shapeDone )
		//	sprintf( useName, "%sShape%d", parent, uniqueID() );
		//else
		//	{
			// Why did I have this "shapeDone" flag ??
			// I could still generate identical nodes with
			// instances and things

			//sprintf( useName, "%sShape", parent );
			sprintf( useName, "%sShape%d", parent, uniqueID() );
		//	}
	else
		// no Shape nor parent name .. make up a name
		sprintf( useName, "shape%d", uniqueID() );

	strcpy( current_Name, useName ); // copy to global

	// prepare to create the transform
	fprintf( fp, "createNode transform" );

	int numID;
	numID = uniqueID();
	fprintf( fp, " -n \"%sXX%d\"", useName, numID );

	if ( fullParent && *fullParent )
		fprintf( fp, " -p \"%s\"", fullParent );
	fprintf( fp, ";\n" );

	// and create the Maya mesh Shape underneath it
	fprintf( fp, "createNode mesh -n \"%s\"", useName );
	fprintf( fp, " -p \"%sXX%d\"", useName, numID );
	fprintf( fp, ";\n" );

	sprintf( current_fullName, "%s|%sXX%d|%s", 
				fullParent,useName, numID, useName ); // set global

}

void outputGroup( VrmlNode *node, char *name, char *parent, char *fullParent )
{
    fprintf( fp, "createNode transform" );
    if ( name && *name )
        fprintf( fp, " -n \"%s\"", name );
    if ( /*tabs > 0 && */ fullParent && *fullParent )
		fprintf( fp, " -p \"%s\"", fullParent );
	fprintf( fp, ";\n" );

    // Now output the TRS values if needed

    fprintf( fp, "\tsetAttr \".t\" -type \"double3\" %f %f %f;\n",
                0.0, 0.0, 0.0 );

    fprintf( fp, "\tsetAttr \".r\" -type \"double3\" %f %f %f;\n",
                0.0 * radToAngular,
                0.0 * radToAngular,
                0.0 * radToAngular );

    fprintf( fp, "\tsetAttr \".s\" -type \"double3\" %f %f %f;\n",
                1.0, 1.0, 1.0 );

}

void outputShapeGeo( VrmlNodeIFaceSet *ifsNode )
{
	int		nverts;

	// VrmlNodeIFaceSet *ifsNode = node->toIFaceSet();

	VrmlNode *coords = ifsNode->getCoordinate();

	verboseNameNL( ifsNode, "Index Face Set" );

	if ( coords )
    {
		VrmlMFVec3f &coord = coords->toCoordinate()->coordinate();
		nverts = coord.size();

		fprintf( fp, "\tsetAttr -s %d \".vt[0:%d]\"", nverts, nverts - 1 );
		for ( int i=0; i < nverts; i++ )
			fprintf( fp, "\n\t\t%f %f %f",
					coord[i][0], coord[i][1], coord[i][2] );
		fprintf( fp, ";\n" );
	}

	// Output Vertex colors

	VrmlNodeColor *colors = ifsNode->color();
	if ( colors )
	{
		VrmlMFColor Vtx_Colors = colors->color();
		nverts = Vtx_Colors.size();

		if ( nverts > 0 )
		{
			fprintf( fp, "\tsetAttr -s %d \".clr[0:%d]\"", nverts, nverts - 1 );
			for ( int i=0; i < nverts; i++ )
			{
				fprintf( fp, "\n\t\t%f %f %f 1",
						Vtx_Colors[i][0], Vtx_Colors[i][1], Vtx_Colors[i][2] );
			}
			fprintf( fp, ";\n" );
		}
	}

	// Output texture coordinates

	VrmlNode *texCoords = ifsNode->getTexCoord();
	if ( texCoords )
    {
		VrmlMFVec2f &coord = texCoords->toTextureCoordinate()->coordinate();
		nverts = coord.size();

		fprintf( fp, "\tsetAttr -s %d \".uv[0:%d]\"", nverts, nverts - 1 );
		for ( int i=0; i < nverts; i++ )
			fprintf( fp, "\n\t\t%f %f",
					coord[i][0], coord[i][1] );
		fprintf( fp, ";\n" );
	}


	// Print out the edges.
	//

	int numPolygons = 0;

	VrmlMFInt cindex = ifsNode->getCoordIndex();

	AmEdgeList *hashTable = new AmEdgeList [kHashSize];
	AmEdgeList *edgePrintList = new AmEdgeList;

	if ( cindex.size() )
    {
		numPolygons = findNumFaces( cindex );

		int	curIdx = 0;
		int edgeId = 1;

//		printf( "numPolygons = %d\n", numPolygons );

		for( int i = 0; i < numPolygons; i++ )
		{
			printEdges( ifsNode, i, curIdx, hashTable, edgeId, edgePrintList );

			for ( ; curIdx < cindex.size(); curIdx++ )
			{
				if ( cindex[curIdx] == -1 )
				{
					curIdx++;
					break;
				}
			}
		}

		fprintf( fp, "\tsetAttr -s %d \".ed[0:%d]\" -type \"long3\"\n",
					(edgeId - 1), (edgeId-2) );

		edgePrintList->printEdges();
		delete edgePrintList;
		edgePrintList = NULL;

		fprintf( fp, "\t\t;\n" );

	}

	// Output faces

	//VrmlMFInt cindex = ifsNode->getCoordIndex();

	// Print out the facets.
	//
	int vtCount = 1;
	if ( numPolygons > 0 )
	{
		fprintf( fp, "\tsetAttr -s %d \".fc[0:%d]\" -type \"polyFaces\"\n",
			   numPolygons, numPolygons-1 );
	}
	
	int	curIdx = 0;

//	printf( "numPolygons = %d\n", numPolygons );

	for( int i = 0; i < numPolygons; i++ )
	{
		printFacets( ifsNode, i, curIdx, hashTable, vtCount );
		for ( ; curIdx < cindex.size(); curIdx++ )
		{
			if ( cindex[curIdx] == -1 )
			{
				curIdx++;
				break;
			}
		}
	}

	fprintf( fp, "\t\t;\n" );


#if 0
	// Output Render Info for this shape
	// to be completed.
	// Check SOLID field for doubleSided
	// Check if normals given, & number of them for smooth_shading
	// By default VRML does flat shading, unless Normals/vertex is set

	if( renderInfo.doubleSided )
		printFile( "%s%s \".ds\" yes;\n", indentSpace(), kSetAttrCommand );
	else
		printFile( "%s%s \".ds\" no;\n", indentSpace(), kSetAttrCommand );



	if( renderInfo.smooth_shading )
		printFile( "%s%s \".smo\" yes;\n", indentSpace(), kSetAttrCommand );
	else
		printFile( "%s%s \".smo\" no;\n", indentSpace(), kSetAttrCommand );
#endif

	//
	// Now assign the current Material to the current Mesh
	//

	if ( numPolygons > 0 )
	{
		fprintf( fp, "sets -e -forceElement %s %s;\n",
				shadingGrp, current_fullName ); // use globals
	}
}


//
//	Routines for outputting the VRML primitives.
//	Some of the VRML options are implemented as delete components (faces) for those
//	that can be done.
//

int uniquePrimID()
{
    static int id = 0;

    id++;

    return id;
}

void outputPrimSphere( VrmlNodeSphere *sphere )
{
    int id;

    // Lets get this id

    id = uniquePrimID();

	verboseNameNL( sphere, "Sphere" );

	fprintf( fp, "createNode polySphere -n \"polySphere%d\";\n", id );

	// Wrl is in metres, we are in cm, so *100 would normally be required
	// this is not consistent now, so I'm just going to leave it alone

	fprintf( fp, "\tsetAttr \".r\" %f;\n", sphere->getRadius() );

	//
	// Now assign the current Material to the current Mesh
	//

	fprintf( fp, "sets -e -forceElement %s %s;\n",
			shadingGrp, current_Name ); // use globals

	// Now connect it up

	fprintf( fp, "connectAttr \"polySphere%d.out\" \"%s.i\";\n",
			id, current_Name ); // use global current_Name

}

void outputPrimBox( VrmlNodeBox *box )
{
    int id;

    // Lets get this id

    id = uniquePrimID();

	verboseNameNL( box, "Box" );

	fprintf( fp, "createNode polyCube -n \"polyCube%d\";\n", id );

	// Wrl is in metres, we are in cm, so *100 would normally be required
	// this is not consistent now, so I'm just going to leave it alone

	VrmlSFVec3f tmpValue;
	float *S;

	tmpValue = box->getSize();
	S = tmpValue.get();

	fprintf( fp, "\tsetAttr \".w\" %f;\n", S[0] );
	fprintf( fp, "\tsetAttr \".h\" %f;\n", S[1] );
	fprintf( fp, "\tsetAttr \".d\" %f;\n", S[2] );

	//
	// Now assign the current Material to the current Mesh
	// ?? should this use the current_Mesh global?

	fprintf( fp, "sets -e -forceElement %s %s;\n",
			shadingGrp, current_Name ); // use globals

	// Now connect it up

	fprintf( fp, "connectAttr \"polyCube%d.out\" \"%s.i\";\n",
			id, current_Name ); // use global current_Name

}

#define N_CONE_SIDES 20
#define CONE_BOTTOM   0

void outputPrimCone( VrmlNodeCone *cone )
{
	int id;
    int     flag = 0;
    int     compNum;
    char    compList[512];

	// Lets get this id

	id = uniquePrimID();
    compNum = 0;

	verboseNameNL( cone, "Cone" );

    fprintf( fp, "createNode polyCone -n \"polyCone%d\";\n", id );

    // Wrl is in metres, we are in cm, so *100 would normally be required
    // this is not consistent now, so I'm just going to leave it alone

    fprintf( fp, "\tsetAttr \".r\" %f;\n", cone->getBottomRadius() );
    fprintf( fp, "\tsetAttr \".h\" %f;\n", cone->getHeight() );

    fprintf( fp, "\tsetAttr \".sa\" %d;\n", N_CONE_SIDES );
    fprintf( fp, "\tsetAttr \".sh\" 1;\n" );
    fprintf( fp, "\tsetAttr \".sc\" 0;\n" );

	// Check to see if both the sides and bottom are turned off.
	// If so then it is not
	// visible, so lets turn the object off for now

	if ( !cone->getBottom() && !cone->getSide() )
    {
		fprintf( fp,
"// cone set invisible as sides & cap are off in VRML input\n" );
		fprintf( fp, "setAttr \"%s.visibility\" 0;\n",
				current_Name ); // use global current_Name
	}

	// If the bottom or sides are turned off

	else if( !cone->getBottom() || !cone->getSide() )
	{
        flag = 1;

		fprintf( fp,
"createNode deleteComponent -n\"deleteComponent%d\";\n", id );

        // Now lets see what needs to be deleted

        strcpy( compList, "" );

        if ( !cone->getBottom() )
        {
            strcat( compList, "\"f[0]\" " );
            compNum++;
        }

        if ( !cone->getSide() )
        {
			strcat( compList, "\"f[1:20]\" " );
            compNum++;
        }

        fprintf( fp, "\tsetAttr \".dc\" -type \"componentList\" %d %s;\n",
                        compNum, compList );
	}

    //
    // Now assign the current Material to the current Mesh
    // ?? should this use the current_Mesh global?

    fprintf( fp, "sets -e -forceElement %s %s;\n",
                shadingGrp, current_Name ); // use globals

    // Now connect it up

	if ( flag == 0 )
	{
    	fprintf( fp, "connectAttr \"polyCone%d.out\" \"%s.i\";\n",
				id, current_Name ); // use global current_Name
	} else {
		fprintf( fp, "connectAttr \"deleteComponent%d.og\" \"%s.i\";\n",
				id, current_Name ); // use global current_Name
		fprintf( fp,
"connectAttr \"polyCone%d.out\" \"deleteComponent%d.ig\";\n",
				id, id );
	}

}

#define N_CYLINDER_SIDES 20
#define CYLINDER_BOTTOM  (N_CYLINDER_SIDES + 1)
#define CYLINDER_TOP     (N_CYLINDER_SIDES + 2)

void outputPrimCylinder( VrmlNodeCylinder *cylinder )
{
	int		id;
	int		flag = 0;
	int		compNum;
	char	compList[512];

	id  = uniquePrimID();
	compNum = 0;

	verboseNameNL( cylinder, "Cylinder" );

    fprintf( fp, "createNode polyCylinder -n \"polyCylinder%d\";\n", id );

    // Wrl is in metres, we are in cm, so *100 would normally be required
    // this is not consistent now, so I'm just going to leave it alone

    fprintf( fp, "\tsetAttr \".r\" %f;\n", cylinder->getRadius() );
    fprintf( fp, "\tsetAttr \".h\" %f;\n", cylinder->getHeight() );
    fprintf( fp, "\tsetAttr \".tx\" 1;\n" );

	fprintf( fp, "\tsetAttr \".sa\" %d;\n", N_CYLINDER_SIDES );
	fprintf( fp, "\tsetAttr \".sh\" 1;\n" );
	fprintf( fp, "\tsetAttr \".sc\" 0;\n" );

	// To see if we need the caps or not, we put them in in the primitive
	// and then we delete the faces that we know of.  This relies on the fact
	// of knowing which faces are the top and bottom, bottom = 20, top = 21.

    // Check to see if both the sides and bottom are turned off.
	// If so then it is not
    // visible, so lets turn the object off for now

    if( !cylinder->getBottom()	&&
		!cylinder->getTop()		&&
		!cylinder->getSide()
	  )
    {
	    fprintf( fp,
"// Cylinder set invisible as sides & caps are off in VRML input\n" );
		fprintf( fp, "setAttr \"%s.visibility\" 0;\n",
				current_Name ); // use global current_Name
    }

	// going to set both top and bottom on.

    else if ( !cylinder->getBottom() || !cylinder->getTop() || !cylinder->getSide())
    {
		flag = 1;

        fprintf( fp,
"createNode deleteComponent -n \"deleteComponent%d\";\n", id );

		// Now lets see what needs to be deleted

		strcpy( compList, "" );

		// The order is important as the faces will be renumbered
		// as they are deleted

        if ( !cylinder->getSide() )
        {
            strcat( compList, "\"f[0:19]\" " );
            compNum++;
        }

        if ( !cylinder->getBottom() )
        {
            strcat( compList, "\"f[20]\" " );
            compNum++;
        }

        if ( !cylinder->getTop() )
        {
           	strcat( compList, "\"f[21]\" " );
            compNum++;
        }

		// Now we can create the componentList

        fprintf( fp, "\tsetAttr \".dc\" -type \"componentList\" %d %s;\n",
						compNum, compList );
	}


    //
    // Now assign the current Material to the current Mesh
    // ?? should this use the current_Mesh global?

    fprintf( fp, "sets -e -forceElement %s %s;\n",
                shadingGrp, current_Name ); // use global current_Name

    // Now connect it up

    if ( flag == 0 )
    {
        fprintf( fp,
"connectAttr \"polyCylinder%d.out\" \"%s.i\";\n",
                id, current_Name ); // use global current_Name
    } else {
        fprintf( fp,
"connectAttr \"deleteComponent%d.og\" \"%s.i\";\n",
                id, current_Name ); // use global current_Name
        fprintf( fp,
"connectAttr \"polyCylinder%d.out\" \"deleteComponent%d.ig\";\n",
                id, id );
    }

}

void checkShape( VrmlNode *node,
	char *parentName,
	char *fullParent,
	int shapeDone )
{
	char *texName;
	char *nodeName;
	char useName[1024];

	if ( !node->toShape() )
		return;

	verboseNameNL( node, "Shape" );

	VrmlNodeShape *shape = node->toShape();

	if ( shape->getAppearance() )
	{
		VrmlNodeAppearance *appear = shape->getAppearance()->toAppearance();

		verboseNameNL( appear, "Appearance" );

		if ( appear->texture() )
			{
			char *texXF;
		
			if ( appear->textureTransform() )
				texXF = outputTextureTransform( appear->textureTransform() );
			else
				texXF = (char *) 0;

			texName = outputTexture( appear->texture(), texXF );
			}
		else
			texName = (char *) 0;

		VrmlNode *material = appear->material();

		if ( appear->material() )
			outputMaterial( material, texName );
	}

	VrmlNode* geometry = shape->getGeometry();

	if ( geometry )
	{
		nodeName = (char *)node->name();
		if ( nodeName && *nodeName )
		{
			strcpy( useName, nodeName);	

	    	// Need to check that the name used follows some of Maya's rules

		for (int z=0; z < (int)strlen(useName); z++ )
    		{
    		    if ( useName[z] == '#' || useName[z] == ':' || useName[z] == '-' )
    		    {   
    		        useName[z] = '_';
    		    }
    		}
		} else {
			useName[0] = 0;
		}

		outputShapeNode( node, shapeDone, useName,
							parentName, fullParent );

		VrmlNodeGeometry *geo = geometry->toGeometry();

		// VrmlNodeIFaceSet *IFaceSet = geo->toIFaceSet();
		if ( geo->toIFaceSet() )
			outputShapeGeo( geo->toIFaceSet() );
		else if ( geo->toSphere() )
			outputPrimSphere( geo->toSphere() );
		else if ( geo->toBox() )
			outputPrimBox( geo->toBox() );
		else if ( geo->toCone() )
			outputPrimCone( geo->toCone() );
		else if ( geo->toCylinder() )
			outputPrimCylinder( geo->toCylinder() );
		
		// Now for those geometry nodes that can't or don't convert

		else if ( !strcmp( "IndexedLineSet", geo->nodeType()->getName() ) )
			outputILineSetGeo( geo );
		else if ( !strcmp("PointSet",geo->nodeType()->getName() ) )
			outputPointSetGeo( geo );			// toPointSet doesn't exist


	}
}

static int clevel = 0;

void checkScene( VrmlNode *node, char *parent )
{
	int		numToCheck = 0;
	int		shapeDone = 0;

	char	useName[1024];
	char	fullName[4096];
	char	parentName[256];
	// bool	group = false;
	// bool	trans = false;

	clevel++;

	if ( node->toGroup() )
		numToCheck = node->toGroup()->size();
	else if ( node->toTransform() )
		numToCheck = node->toTransform()->size();
	else if ( node->toProto() )
		numToCheck = node->toProto()->size();
	else if ( node->toLOD() )
		numToCheck = node->toLOD()->getLevel()->size();
	else
		return;

	char *sceneNode = (char *) node->name();

	if ( sceneNode && *sceneNode )
		strcpy( parentName, sceneNode );
	else {
		//parentName[0] = 0;
		sprintf( parentName, "t%d", uniqueID() );
		node->setName( parentName, NULL );
	}


    // Need to check that the name used follows some of Maya's rules

    for (int z=0; z < (int)strlen(parentName); z++ )
    {
        if ( parentName[z] == '#' || parentName[z] == ':' || parentName[z] == '-' )
        {   
            parentName[z] = '_';
        }
    }

	// now check the name of the parent as given us
	if ( parent && *parent )
		sprintf( fullName, "%s|%s", parent, parentName ); //sceneNode );
	else
		// no parent means we are at the top
		sprintf( fullName, "|%s", parentName ); //sceneNode );

	if ( verbose )
		{
		cerr << indentSpace()
			 << "Node " << fullName << " has " << numToCheck;
		if( numToCheck > 1 )
			cerr << " children.";
		else
			cerr << " child.";
		cerr << endl;
		}


	for ( int i=0; i < numToCheck; i++ )
	{
		// loop to process each child node expected
		VrmlNode *current=NULL;

		if ( node->toGroup() )
			current = node->toGroup()->child(i);
		else if ( node->toTransform() )
			current = node->toTransform()->child(i);
		else if ( node->toLOD() )
			current = node->toLOD()->getLevel()->get(i);
		else 
		{
			cerr << "child node not group/transform" << endl;
			// In which case we don't have a valid pointer, so lets skip the 
			// rest of this child, go onto the next
			continue;
		}

		if( verbose )
			{
			cerr << indentSpace() << "child " << i;
			if ( ! current->name() )
				cerr << " with name " << current->name();
			cerr << endl;

			cerr << indentSpace()
				 << "Node is " << current->nodeType()->getName() << endl;

			}

		if ( current->toNavigationInfo() )
			verboseMsg( "found Navigation Info - no action taken" );

		if ( current->toViewpoint() )
			verboseMsg( "Found Viewpoint - no action taken" );


		if ( current->toShape() )
		{
			checkShape( current, parentName, fullName, shapeDone );
			shapeDone = 1;
		}

		if ( current->toTransform() )
		{
			if( current->name() && strlen( current->name()) > 0)
			{
			    // Need to check that the name used follows some of Maya's rules

				strcpy( useName, current->name() );

			for (int z=0; z < (int)strlen(useName); z++ )
    			{
        			if ( useName[z] == '#' || useName[z] == ':' || useName[z] == '-' )
        			{   
            			useName[z] = '_';
        			}
    			}

				outputTransform( current, useName, parentName, fullName );
			}
			else
				{
				char	currentName[256];

				sprintf( currentName, "t%d", uniqueID() );
				current->setName( currentName, NULL );
				outputTransform( current,
					currentName, parentName, fullName );
				}

			tabs += 2;
			checkScene( current, fullName );
			tabs -= 2;
		}

		else if ( current->toGroup() )
		{
			if ( verbose )
				cerr << indentSpace()
					 << "found Group Node clevel = " << clevel << endl;

			if ( current->name() && strlen(current->name()) > 0 )
			{
                // Need to check that the name used follows some of Maya's rules
                strcpy( useName, current->name() );

                for (int z=0; z < (int)strlen(useName); z++ )
                {
                    if ( useName[z] == '#' || useName[z] == ':' || useName[z] == '-' )
                    {
                        useName[z] = '_';
                    }
                }

				outputGroup( current, useName, parentName, fullName );
			}
			else {
				char	currentName[256];

				sprintf( currentName, "t%d", uniqueID() );
				current->setName( currentName, NULL );
				outputGroup( current, currentName, parentName, fullName );
			}
			tabs += 2;
			checkScene( current, fullName );
			tabs -= 2;
		}

        else if ( current->toSwitch() )
        {
            if ( verbose )
                cerr << indentSpace()
                     << "found Switch Node clevel = " << clevel << endl;

			VrmlMFNode *choiceNodes = current->toSwitch()->getChoiceNodes();
			int whichChoice = current->toSwitch()->getWhichChoice();

			if ( verbose )
				cerr << indentSpace()
					 << "Choice is = " << whichChoice << endl;
			
			if ( whichChoice >= 0 )
			{		
				VrmlNode *useNode = choiceNodes->get(whichChoice);

				if ( verbose )
					cerr << indentSpace()
						 << "using Node " << useNode->nodeType()->getName() << endl;
			

	            if ( useNode->name() && strlen(useNode->name()) > 0 )
    	        {
        	        // Need to check that the name used follows some of i
					// Maya's rules

	               strcpy( useName, useNode->name() );

		       for (int z=0; z < (int)strlen(useName); z++ )
        	        {
            	        if ( useName[z] == '#' || useName[z] == ':' || useName[z] == '-' )
                	    {
                    	    useName[z] = '_';
                    	}
                	}

                	outputGroup( useNode, useName, parentName, fullName );
            	}
            	else {
                	char    currentName[256];

                	sprintf( currentName, "t%d", uniqueID() );
                	useNode->setName( currentName, NULL );
                	outputGroup( useNode, currentName, parentName, fullName );
            	}
         
				if ( useNode->toShape() )
				{
					checkShape( useNode, parentName, fullName, shapeDone );
					shapeDone = 1;
				} else {
					
					//cerr << "switch checking " << useNode->nodeType()->getName() << endl;

					tabs += 2;
            		checkScene( useNode, fullName );
            		tabs -= 2;
				}
			}
        } else if ( current->toProto() )
		{
            if ( verbose )
                cerr << indentSpace()
                     << "found proto Node clevel = " << clevel << endl;

            if ( current->name() && strlen(current->name()) > 0 )
            {
                // Need to check that the name used follows some of i
                // Maya's rules
               
                strcpy( useName, current->name() );

                for (int z=0; z < (int)strlen(useName); z++ )
                {
                    if ( useName[z] == '#' || useName[z] == ':' || useName[z] == '-' )
                    {
                        useName[z] = '_';
                    }
                }

                outputGroup( current, useName, parentName, fullName );
            }
            else {
                char    currentName[256];

                sprintf( currentName, "t%d", uniqueID() );
                current->setName( currentName, NULL );
                outputGroup( current, currentName, parentName, fullName );
            }
                
            tabs += 2;
            checkScene( current, fullName );
            tabs -= 2;
                
		}

	}
}

void showHelp()
{
	char usage[1024];

	sprintf( usage, usage_leader, version );
	puts( usage );
	puts( usage_body );
}

const char linearUnitsList[] =
"inches feet yards miles millimeters centimeters kilometers meters";
const char angularUnitsList[] = "radian degree angMinute angSecond";
const char timeUnitsList[] =
"hours minutes seconds milliseconds games palframe ntscframe showscan palfield ntscfield";

int
main( int argc, char **argv)
{
	char *inputFile = "test.wrl";
	char *outputFile = "test.ma";
	int	  opt;
	bool  dumpWRL = false;

	//
	// Check that we have something
	//

	if ( argc < 2 )
	{
		showHelp();
		return 0;
	}

	//
	// Parse the options
	//
	
	while ((opt = getopt(argc, argv, "a:A:cCdDeEhHmMnNI:i:L:l:O:o:vVt:T:Ww")) != -1)
	{
		switch (opt) {
		case 'H':
		case 'h':
			showHelp();
			return 0;

		case 'C':
		case 'c':
			force_CPV = true;
			break;

		case 'D':
		case 'd':
			dumpWRL = true;
			break;

		case 'E':
		case 'e':
			use_HardEdges = false;
			break;

		case 'M':
		case 'm':
			mayaSrc = 1;
			break;

		case 'N':
		case 'n':
			use_Names = true;
			break;

		case 'I' :
		case 'i' :
			inputFile = strdup( optarg );			
			break;

		case 'O' :
		case 'o' :
			outputFile = strdup( optarg );
			break;

		case 'V' :
		case 'v' :
			verbose = true;
			break;

		case 'L' :
		case 'l' :
			LinearUnits = strdup( optarg );
			if( ! strstr( linearUnitsList, LinearUnits ) )
				{
				fprintf( stderr, "invalid linear unit argument - %s\n", LinearUnits);
				fprintf( stderr, " use one of %s\n", linearUnitsList );
				}
			break;

		case 'A' :
		case 'a' :
			AngularUnits = strdup( optarg );
			if( ! strstr( angularUnitsList, AngularUnits ) )
				{
				fprintf( stderr, "invalid angular unit argument - %s\n", AngularUnits);
				fprintf( stderr, " use one of %s\n", angularUnitsList );
				}
			else
			if ( !strcmp( AngularUnits, "radian" ) )
				radToAngular = 1.0;
			else if ( !strcmp( AngularUnits, "degree" ) )
				radToAngular = 180. / 3.14159258;
			break;

		case 'T' :
		case 't' :
			TimeUnits = strdup( optarg );
			if( ! strstr( timeUnitsList, TimeUnits ) )
				{
				fprintf( stderr, "invalid time unit argument - %s\n", TimeUnits);
				fprintf( stderr, " use one of %s\n", timeUnitsList );
				}
			break;

		case 'W' :
		case 'w' :
			showWarnings = true;
			break;
		}
	}
	
	g_pTheScene = new VrmlScene( inputFile );

	root_node = g_pTheScene->getRoot();

	if ( root_node )
	{
		if ( verbose )
			cerr << "root" << root_node->name() << endl;

		fp = fopen( outputFile, "wb" );

		if ( fp != NULL )
		{
			outputHeader( outputFile );

			node = root_node;

			if ( strlen(node->name() ) == 0 )
				node->setName("root");

			// For the 1st node we will output a transform node for it.
			// sort of like a placeholder, just in case its needed.

			fprintf( fp, "createNode transform -n \"%s\";\n\n", node->name() );

			// Now go on to do the rest of the scene

			checkScene( root_node, "" );

			fprintf( fp, "connectAttr \"lightLinker1.msg\" \":lightList1.ln\" -na;\n" );

			fclose( fp );

		} else {

			perror( "wrl2ma: Could not create output file " );
		}

		if ( dumpWRL )
			g_pTheScene->save("test_out.wrl");
	}

	return 0;
}

