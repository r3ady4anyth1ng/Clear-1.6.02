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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined (SGI) || defined (LINUX)
#include <GL/glx.h>
#include <GL/gl.h>
#endif

#include <maya/MIOStream.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>


#include <maya/ilib.h>
#include <iffwriter.h>

#if defined (_WIN32)
#include <process.h>
#endif

#if defined (SGI) || defined (LINUX)
#include <X11/keysym.h>
#endif

extern "C" {

#if defined(OSWin_) || defined(OSUnix_) || defined(OSMac_)
#	include <sys/types.h>	/* For all unix types & values		      */
#else
	error "__FILE__:__LINE__ Unknown O/S"
#endif

#if (defined (SGI) || defined (LINUX) || defined (OSMac_))
#include <unistd.h>
#endif

#include "MDt.h"
#include "MDtExt.h"
}

#if defined (_WIN32)
#pragma warning (disable: 4244 4305 )
#endif

//  =====================================================================
//
//  The following string is used to define the version number to the Maya
//  plug-ins Manager.  Change this whenever the translator is modified
//

char    *rtgVersion = "4.04";

//  =====================================================================



//
//	variables for keeping track of the UI interface options
//
int		rtg_v18_compatible = false;

int		rtg_output_image_format = 1;
int		rtg_output_file_format = 1;

int		rtg_output_vert_norms  = true;
int		rtg_output_vert_colors = true;
int		rtg_output_tex_coords = true;
int		rtg_output_poly_norms = true;
int		rtg_output_hierarchy  = true;
int		rtg_show_index_counters = true;
int		rtg_output_pivots     = false;
int     rtg_output_transforms = true;
int     rtg_output_materials  = true;
int		rtg_output_animation  = true;
int		rtg_output_local      = true;
int     rtg_output_all_nodes  = true;
int     rtg_output_degrees    = true;
int     rtg_output_decomp     = true;

//
//
//

char     rtg_title[] = "Alias Rtg Output";
int 	globalpolys;

#define D2R (3.1415927 / 180.)

FILE *outputFile    = NULL;

#define GEOMETRY    1
#define	HEIRARCHY   2

#define WORLD 0
#define FLAT  1
#define FULL  2
#define NONE    0
#define TRI     1
#define QUAD    2

typedef float   Matrix[4][4];

static int      tabs = 0;


DtStateTable *stateTable = NULL;

static int		counter;


void doCR( void )
{
	// Will need to add to this to do unix/DOS style

	if ( rtg_output_file_format == 1)
	    fprintf( outputFile, "\n" );			// unix
	else
		fprintf( outputFile, "\r\n" );	// dos
}

int getTopLevelCount( void )
{
	int count = 0;
	int shapeNo = 0;
	int numShapes;

	numShapes = DtShapeGetCount();

	for ( shapeNo = 0; shapeNo < numShapes; shapeNo++ ) {

		if ( DtShapeGetParentID( shapeNo ) < 0 ) {

			count++;
		}
	}

	return count;
}


void doHeader( void )
{
	int numShapes;

	fprintf( outputFile, "HEADER_TITLE %s", rtg_title );
	doCR();
	fprintf( outputFile, "HEADER_VERSION %s", rtgVersion );
	doCR();

    numShapes = DtShapeGetCount();

	doCR();
    fprintf(outputFile, "NUMBER_OF_OBJECTS %d",  numShapes );
	doCR();
	doCR();

	fprintf(outputFile, "OUTPUT_VERT_NORMS   %s", rtg_output_vert_norms ?
		    "on" : "off" );
	doCR();
	fprintf(outputFile, "OUTPUT_VERT_COLORS  %s", rtg_output_vert_colors ?
			"on" : "off" );
	doCR();
	fprintf(outputFile, "OUTPUT_TEX_COORDS   %s", rtg_output_tex_coords ?
		    "on" : "off" );
	doCR();
	fprintf(outputFile, "OUTPUT_POLY_NORMS   %s", rtg_output_poly_norms ?
		    "on" : "off" );
	doCR();
	fprintf(outputFile, "OUTPUT_HIERARCHY    %s", rtg_output_hierarchy ?
		    "on" : "off" );
    doCR();
    fprintf(outputFile, "OUTPUT_LOCAL        %s", rtg_output_local ?
            "on" : "off" );
	doCR();
	fprintf(outputFile, "SHOW_INDEX_COUNTERS %s", rtg_show_index_counters ?
		    "on" : "off" );
	doCR();

	fprintf(outputFile, "OUTPUT_MATERIALS    %s", rtg_output_materials ?
			"on" : "off" );
	doCR();

	fprintf(outputFile,  "OUTPUT_ANIMATION   %s",  rtg_output_animation ?
			"on" : "off" );
	doCR();

    fprintf(outputFile,  "OUTPUT_ALL_NODES   %s",  rtg_output_all_nodes ?
            "on" : "off" );
    doCR();

    fprintf(outputFile,  "OUTPUT_DECOMP      %s",  rtg_output_decomp ?
            "on" : "off" );
    doCR();

    fprintf(outputFile,  "OUTPUT_DEGREES     %s",  rtg_output_degrees ?
            "on" : "off" );
    doCR();

	doCR();

}

void resetCounter( void )
{
	counter = 0;
}

void doCounter( void )
{
	if ( rtg_show_index_counters )
		fprintf( outputFile, "%d ", counter );
	counter++;
}

void doTabs( void )
{
    int i;
    for ( i =0; i < (tabs*4); i++ ){
	fprintf(outputFile, " ");
    }
}



void
outputTransforms( int shapeNo,  int output_transforms )
{
	MObject  object;
	MTransformationMatrix::RotationOrder rotOrder;

	MStatus	 stat;

	MVector	 Tx;

	double	   sx, sy, sz;
	double	   tx, ty, tz;
	double	   rx, ry, rz;
    double     prx, pry, prz;
    double     psx, psy, psz;

#ifdef NEEDTHIS
	double	   prix=0, priy=0, priz=0;
    double     prox=0, proy=0, proz=0;
	double	   psix=0, psiy=0, psiz=0;
    double     psox=0, psoy=0, psoz=0;
#endif

	float	   ltm[4][4];

	float     *mat;
	float 	   fsx, fsy, fsz;
	float	   ftx, fty, ftz;
	float	   frx, fry, frz;

	if ( output_transforms ) {

		if  ( rtg_output_pivots ) {

			DtExt_ShapeGetTransform( shapeNo, object );
			
   			// Take the first dag path.
		    //
			MFnDagNode fnTransNode( object, &stat );
			MDagPath dagPath;
			stat = fnTransNode.getPath( dagPath );

			MFnTransform dag( dagPath, &stat );

    		if( MS::kSuccess == stat )
    		{
				Tx = dag.translation( MSpace::kTransform, &stat );
				tx = Tx.x;
				ty = Tx.y;
				tz = Tx.z;
				
				double rot[3];
				rotOrder = MTransformationMatrix::kXYZ;
				stat = dag.getRotation( rot, rotOrder, MSpace::kObject );
				rx = rot[0];
				ry = rot[1];
				rz = rot[2];

                if ( rtg_output_degrees )
                {
                    rx *= (180./3.1415927);
                    ry *= (180./3.1415927);
                    rz *= (180./3.1415927);
                }   

				double scale[3];
				stat = dag.getScale( scale );
				sx = scale[0];
				sy = scale[1];
				sz = scale[2];

                MPoint RPivot = dag.rotatePivot( MSpace::kWorld, &stat );
                prx = RPivot.x; 
                pry = RPivot.y;
                prz = RPivot.z;

				MPoint SPivot = dag.scalePivot( MSpace::kWorld, &stat );
				if ( stat != MS::kSuccess )
				{
				    cerr << "error getting scalePivot " << endl;
				}

				psx = SPivot.x;
				psy = SPivot.y;
				psz = SPivot.z;
	
				MTransformationMatrix localTM = dag.transformation();
				MMatrix localMatrix = localTM.asMatrix();
				localMatrix.get( ltm );

#ifdef NEEDTHIS
				dag->rotatePivotIn( prix, priy, priz );
				dag->rotatePivotOut( prox, proy, proz ); 
				dag->scalePivotIn( psix, psiy, psiz );
				dag->scalePivotOut( psox, psoy, psoz );

				dag->localTransformationMatrix( ltm );
#endif

				doTabs();
				fprintf( outputFile, "tran: %f %f %f", tx, ty, tz );
				doCR();

				doTabs();
				fprintf( outputFile, "rot: %f %f %f", rx, ry, rz );
				doCR();

				doTabs();
				fprintf( outputFile, "scal: %f %f %f", sx, sy, sz );
				doCR();

				// Need to decide if this is really needed.
				
#ifdef NEEDTHIS
				doTabs();
				fprintf( outputFile, "sPvI: %f %f %f", psix, psiy, psiz );
				doCR();

                doTabs();
                fprintf( outputFile, "sPvO: %f %f %f", psox, psoy, psoz );
                doCR();

				doTabs();
				fprintf( outputFile, "rPvI: %f %f %f", prix, priy, priz );
				doCR();

                doTabs();
                fprintf( outputFile, "rPvO: %f %f %f", prox, proy, proz );
                doCR();
#endif

                doTabs();
                fprintf( outputFile, "sPiv: %f %f %f", psx, psy, psz );
                doCR();

                doTabs();
                fprintf( outputFile, "rPiv: %f %f %f", prx, pry, prz );
                doCR();

				if ( rtg_output_local ) {
					doCR();
					doTabs();
					fprintf( outputFile, "ltm0: %f %f %f %f", 
						ltm[0][0], ltm[0][1], ltm[0][2], ltm[0][3] );
                	doCR();
                	doTabs();
                	fprintf( outputFile, "ltm1: %f %f %f %f", 
                        ltm[1][0], ltm[1][1], ltm[1][2], ltm[1][3] );
                	doCR();
                	doTabs();
                	fprintf( outputFile, "ltm2: %f %f %f %f", 
                        ltm[2][0], ltm[2][1], ltm[2][2], ltm[2][3] );
                	doCR();
                	doTabs();
                	fprintf( outputFile, "ltm3: %f %f %f %f", 
                        ltm[3][0], ltm[3][1], ltm[3][2], ltm[3][3] );
					doCR();
				}

				doCR();

			}

		}
		else {

            DtShapeGetMatrix( shapeNo, &mat );

			if ( rtg_output_decomp )
			{
				DtMatrixGetTranslation( mat, &ftx, &fty, &ftz );
				DtMatrixGetRotation( mat, &frx, &fry, &frz );
				DtMatrixGetScale( mat, &fsx, &fsy, &fsz );

			} else {

	            DtExt_ShapeGetTransform( shapeNo, object );
            
            	MFnTransform dag( object, &stat );
            
            	if( MS::kSuccess == stat )
            	{
             
                	Tx = dag.translation( MSpace::kTransform, &stat );
                	ftx = Tx.x;
                	fty = Tx.y;
                	ftz = Tx.z;
                
                	double rot[3];
				    rotOrder = MTransformationMatrix::kXYZ;
                	stat = dag.getRotation( rot, rotOrder, MSpace::kObject );
                	frx = rot[0];
                	fry = rot[1];
                	frz = rot[2];
					

                	double scale[3];
                	stat = dag.getScale( scale );
                	fsx = scale[0];
                	fsy = scale[1];
                	fsz = scale[2];
				}

			}

            if ( rtg_output_degrees )
            {
                frx *= (180./3.1415927);
                fry *= (180./3.1415927);
                frz *= (180./3.1415927);
            }   


			doTabs();
			fprintf( outputFile, "tran: %f %f %f", ftx, fty, ftz );
			doCR();

			doTabs();
			fprintf( outputFile, "rot: %f %f %f", frx, fry, frz );
			doCR();

			doTabs();
			fprintf( outputFile, "scal: %f %f %f", fsx, fsy, fsz );
			doCR();


			if ( rtg_output_local ) {
            	doCR();
            	doTabs();
            	fprintf( outputFile, "ltm0: %f %f %f %f",
                    mat[0], mat[1], mat[2], mat[3] );
            	doCR();
            	doTabs();
            	fprintf( outputFile, "ltm1: %f %f %f %f",
                    mat[4], mat[5], mat[6], mat[7] );
            	doCR();
            	doTabs();
            	fprintf( outputFile, "ltm2: %f %f %f %f",
                        mat[8], mat[9], mat[10], mat[11] );
            	doCR();
            	doTabs();
            	fprintf( outputFile, "ltm3: %f %f %f %f",
                    mat[12], mat[13], mat[14], mat[15] );
            	doCR();
			}

           	doCR();
			
		}
	}
		
}


void
defineAnim(void)
{
	int numShapes;
	int shapeNo;
	int start, end;
	int frame;
	char *name;

	if ( rtg_output_animation ) {	
	
		doTabs();
		fprintf( outputFile,  "ANIMATION_LIST" );
		doCR();
		
		start = DtFrameGetStart();
		end   = DtFrameGetEnd();

		numShapes = DtShapeGetCount();

		tabs++;
		
		for ( frame = start; frame <= end; frame++ ) {
			
			DtFrameSet( frame );
			
			doTabs();
			fprintf( outputFile,  "FRAME %d",  frame );
			doCR();
			
			tabs++;
			
			for ( shapeNo = 0; shapeNo < numShapes; shapeNo++ ) {

				if ( rtg_output_all_nodes || DtExt_ShapeIsAnim( shapeNo ) ) 
				{
					DtShapeGetName( shapeNo, &name );
	
					doTabs();
					fprintf(outputFile,  "OBJECT %s",  name );
					doCR();
					
					tabs++;
					outputTransforms( shapeNo,  TRUE );
					tabs--;
				}
			}
			tabs--;
		}
		
		tabs--;

		doTabs();
		fprintf( outputFile,  "END_ANIMATION_LIST" );
		doCR();
		doCR();
	}
}

bool isNurbSurface( int shapeID )
{
    MObject surfaceNode;
    MStatus stat;

    DtExt_ShapeGetShapeNode( shapeID, surfaceNode ); 

    MFnNurbsSurface fnNurb( surfaceNode, &stat );

    bool givenNurb = true;

    if( MS::kSuccess == stat )
    {
        givenNurb = true;
    } else {
        givenNurb = false;
    }

    return givenNurb;
}

void
defineShape( int shapeNo )
{
    int numGroups;
    int groupNo;
    int numPolys;
	int gPolys = 0;			
    int polyNo;
    int vCount,  tCount,  nCount,  cCount;
	int gvCount, gtCount, gnCount;
    int *vIdx;
    int *nIdx;
    int *tIdx;
    int   i;
	int      matNo;
	char    *matName;
    char    *name,  ext[10];
    DtVec3f *vertex;
    DtVec3f *normal;
    DtVec2f *texture;
    DtRGBA  *colors;
    int     *color;

	int		doing_OBJECT = false;

	char     s1[32], s2[32];

    numGroups = DtGroupGetCount( shapeNo );

    DtShapeGetName( shapeNo, &name );
 
    doTabs();

	numPolys = 0;

	if ( rtg_v18_compatible )
	{
        DtShapeGetVertices( shapeNo,  &vCount, &vertex );
        DtShapeGetNormals( shapeNo,   &nCount,  &normal );
        DtShapeGetTextureVertices( shapeNo,  &tCount,  &texture );
		DtShapeGetVerticesColor( shapeNo,  &cCount,  &colors );  

		for ( groupNo = 0; groupNo < numGroups; groupNo++ )
		{
			DtFaceGetIndexByShape( shapeNo, groupNo, &gvCount, &vIdx );
            gPolys = DtFaceCount( gvCount, vIdx);

			numPolys += gPolys;
		}

	}

            
    for ( groupNo = 0; groupNo < numGroups; groupNo++ ) {

		if ( rtg_v18_compatible )
		{
	        DtFaceGetIndexByShape( shapeNo, groupNo, &gvCount, &vIdx );
	        DtFaceGetNormalIndexByShape( shapeNo, groupNo, &gnCount, &nIdx );
	        DtFaceGetTextureIndexByShape( shapeNo, groupNo, &gtCount, &tIdx );
			DtFaceGetIndexByShape( shapeNo,  groupNo,  &cCount, &color );

            gPolys = DtFaceCount( gvCount, vIdx);

			
		} else {
		
			DtGroupGetVertices( shapeNo,  groupNo,  &vCount, &vertex );
			DtGroupGetNormals( shapeNo,  groupNo,  &nCount,  &normal );  
			DtGroupGetTextureVertices( shapeNo,  groupNo,  &tCount,  &texture );
			DtShapeGetVerticesColor( shapeNo,  &cCount,  &colors );
			DtFaceGetIndexByShape( shapeNo,  groupNo,  &cCount,  &color );

			// DtPolygonGetCount( shapeNo, groupNo, &numPolys );

			DtFaceGetIndexByGroup( shapeNo, groupNo, &gvCount, &vIdx );
       		DtFaceGetNormalIndexByGroup( shapeNo, groupNo, &gnCount, &nIdx );
       		DtFaceGetTextureIndexByGroup( shapeNo, groupNo, &gtCount, &tIdx );

			numPolys = DtFaceCount( gvCount, vIdx);
		}	

		if ( !rtg_v18_compatible || groupNo == 0 )
		{
			if ( !rtg_v18_compatible && numGroups != 1 ) {
				sprintf( ext,  "_%d",  groupNo );
			}
			else {
				strcpy( ext,  "" );
			}		

			if ( rtg_output_vert_norms ) {
				sprintf( s1, "n%d", nCount );
			}
			else {
				strcpy( s1, "" );
			}

			if ( rtg_output_tex_coords ) {
				sprintf( s2, "t%d", tCount );
			}
			else {
				strcpy( s2, "" );
			}

			doing_OBJECT = true;

			fprintf(outputFile, "OBJECT_START %s%s v%d %s %s p%d", 
					name, ext,  vCount,  s1,  s2,  numPolys );		
			doCR();

			if ( rtg_v18_compatible && rtg_output_materials )
			{
				for ( int g = 0; g < numGroups; g++ )
				{
        	       	doTabs();
        	       	DtMtlGetID( shapeNo, g, &matNo );
        	       	DtMtlGetName( shapeNo, g, &matName );
        	       	fprintf( outputFile, "USES_MATERIAL %d %s", matNo, matName );
        	        doCR();
        	    }

			} else {
				if ( rtg_output_materials ) {
					doTabs();
					DtMtlGetID( shapeNo, groupNo, &matNo );
					DtMtlGetName( shapeNo, groupNo, &matName );
					fprintf( outputFile, "USES_MATERIAL %d %s", matNo, matName );
					doCR();
				}
			}

			doCR();
			doTabs();
			if ( rtg_output_hierarchy ) {
				fprintf(outputFile, "VERTEX local");
			}
			else {
				fprintf(outputFile, "VERTEX world");
			}

			doCR();

			resetCounter();
			for ( i = 0; i < vCount; i++ ) {

				doTabs();
				doCounter();
				fprintf(outputFile, "  %f %f %f",  
					vertex[i].vec[0],  vertex[i].vec[1], vertex[i].vec[2] );
				doCR();
			}   
	
			if( rtg_output_vert_colors ) {
				doCR();
				doTabs();
				resetCounter();
				if ( rtg_v18_compatible )
				{
					fprintf(outputFile, "COLORS" );
				} else {
					fprintf(outputFile, "COLOR");
				}
				doCR();
				for ( i = 0; i < cCount; i++ ) {
					if ( color[i] != -1 ) {
						doTabs();
						doCounter();
						fprintf(outputFile,  "  %d %d %d %d",  colors[color[i]].r, 
							colors[color[i]].g,  colors[color[i]].b, 
							colors[color[i]].a );
						doCR();
					}
				}
			}

			if ( rtg_output_vert_norms ) {
				doCR();
				doTabs();
				resetCounter();
				fprintf(outputFile, "NORMAL");
				doCR();
				for ( i = 0; i < nCount; i++ ) {
			
					doTabs();    
					doCounter();
					fprintf(outputFile, "  %f %f %f",  
						normal[i].vec[0],  normal[i].vec[1],  normal[i].vec[2] );
					doCR();
				}
			}

			if (rtg_output_tex_coords) {
				doCR();
				doTabs();
				resetCounter();
				fprintf(outputFile, "TEXCOORD");
				doCR();
				for ( i = 0; i < tCount; i++ ) {
					doTabs();		    
					doCounter();
					fprintf(outputFile, "  %f %f",  
						texture[i].vec[0],  texture[i].vec[1] );
					doCR();
				}
			}

	
			doCR();
			doTabs();
			fprintf(outputFile, "POLYGON" );
			doCR();
			resetCounter();
		}


		int localPolyCnt = 0;

		if ( rtg_v18_compatible )
		{
			numPolys = gPolys;
			localPolyCnt = 0;
		}

		int iv = 0, in = 0, it = 0;

		for ( polyNo = 0; polyNo < numPolys; polyNo++ ) {

			// DtPolygonGetIndices( polyNo, &vCount, &vIdx, &nIdx, &tIdx );

			doTabs();
			doCounter();
	    	
			fprintf(outputFile, "v ");
			while( vIdx[iv] != -1 ) {
				fprintf(outputFile, "%d ",  vIdx[iv] );
				iv++;
			}
			iv++;
	    
			if ( rtg_output_vert_norms ) {
				if ( nIdx ) {
					fprintf(outputFile, "n ");
					while ( nIdx[in] != -1 ) {
						fprintf(outputFile, "%d ",  nIdx[in] );
						in++;
					}
					in++;
				}
			}
	    
			if ( rtg_output_tex_coords ) {
				if ( tIdx ) {
					fprintf(outputFile, "t ");
					while ( tIdx[it] != -1 ) {
						fprintf(outputFile, "%d ",  tIdx[it]);
						it++;
					}
					it++;
				}
			}
	    

			// Now for some of the v1.8 options.  
			// polygon normals,materials,color

			if ( rtg_v18_compatible )
			{
				float red, green, blue;
				int		tex_id;
				int		ired, igreen, iblue;
				int		pcount;
				DtVec3f	polyNrm;

                DtMtlGetID( shapeNo, groupNo, &matNo );
                DtMtlGetName( shapeNo, groupNo, &matName );
				DtMtlGetAmbientClr( matName, 0, &red, &green, &blue );
				DtTextureGetID(matNo, &tex_id);
			
				//
				// This needs to be redone, as the order of the polygons
				// may not match the order being given here.
				//
				DtShapeGetPolygonNormalCount( shapeNo, &pcount);
				DtShapeGetPolygonNormal(shapeNo, localPolyCnt, &polyNrm );

				if ( pcount )
				{
					fprintf( outputFile, "-N %f %f %f",
						polyNrm.vec[0], polyNrm.vec[1], polyNrm.vec[2] );
				}
				
				if ( tex_id > -1 )
				{
					fprintf( outputFile, " T %d", tex_id );
				} else {
					ired = int(red * 256.0);
					if ( ired > 255 ) ired = 255;
					igreen = int(green * 256.0);
					if ( igreen > 255 ) igreen = 255; 
					iblue = int(blue * 256.0);
					if ( iblue > 255 ) iblue = 255;
					
					fprintf( outputFile, " RGB %d %d %d", ired, igreen, iblue);
				}
	
			}

			localPolyCnt++;

			doCR();
			    
		}

		if ( !rtg_v18_compatible )
		{
        	if ( vIdx )
        	    free( vIdx );
        	if ( nIdx )
        	    free( nIdx );
        	if ( tIdx )
        	    free( tIdx );
		}

		if ( !rtg_v18_compatible )
		{
			doTabs();
			fprintf(outputFile, "OBJECT_END");
			doCR();
			doCR();
    	}

	}

	if ( rtg_v18_compatible  && doing_OBJECT )
	{
        doTabs();
        fprintf(outputFile, "OBJECT_END");
        doCR();
        doCR();
	}

}

void
defineChildren( int pass, int shapeNo,  int level )
{
    char *name, ext[10];
    int  numChildren;
    int  numGroups;
    int  groupNo;
    int *children = NULL;
    int  childNo;
    
    DtShapeGetChildren( shapeNo,  &numChildren,  &children );
    
    /*
     * The first bit of code here is used if we are processing
     * the geometry of the model.
     */
    if ( pass == GEOMETRY ) {
	
		if (! numChildren ) {
			/*
			 * This must be a top level node,  so process it.
			 */
			defineShape( shapeNo );
		}
		else {
			/*
			 * This must be a group node,  so process each of its
			 * children. Note that we don't actually output any
			 * group information at this point (that is done in the
			 * heirarchy pass).
			 */

			numGroups = DtGroupGetCount( shapeNo );
			if ( numGroups > 0 )
			{
				defineShape( shapeNo );
			}

			for ( childNo = 0; childNo < numChildren; childNo++ ) {
				defineChildren( pass,  children[childNo], level+1 );
			}
	
			
		}
    }
    /*
     * The next bit of code is used if we are processing the
     * heirarchy of the model.
     */
    else {
		if (! numChildren ) {
			/*
			 * This must be a top level node,  so process it.
			 */
			numGroups = DtGroupGetCount( shapeNo );
			DtShapeGetName( shapeNo,  &name );
	
			tabs++;

			// Lets do a little bit of checking to see if all of this
			// is really necessary.

			if ( 0 == numGroups && DtExt_JointHierarchy() )
			{

	            DtShapeGetName( shapeNo, &name );

        	    doTabs();
            	fprintf(outputFile,  "%d G %s",  level,  name );
            	doCR();
            	tabs++;
            	outputTransforms( shapeNo,  rtg_output_transforms );
            	tabs--;
				tabs--;

			} else {
				if ( rtg_v18_compatible )
				{
					if ( numGroups > 1 )
							numGroups = 1;
				}

				for ( groupNo = 0; groupNo < numGroups; groupNo++ ) {
					doTabs();
	    
					if ( numGroups != 1 ) {
						sprintf( ext,  "_%d",  groupNo );
					}
					else {
						strcpy( ext,  "" );
					}

					fprintf(outputFile,  "%d P %s%s",  level,  name, ext );
					doCR();
					tabs++;
					outputTransforms( shapeNo,  rtg_output_transforms );
					tabs--;
				}
				tabs--;
			}
		}
		else {
			/*
			 * This must be a group node,  so process each of its
			 * children.
			 */
			numGroups = DtGroupGetCount( shapeNo );
			DtShapeGetName( shapeNo, &name );
	     
			tabs++;

			if ( numGroups > 0 )
			{
				if ( rtg_v18_compatible )
				{
					if ( numGroups > 1 )
							numGroups = 1;
				}

				for ( groupNo = 0; groupNo < numGroups; groupNo++ ) {
					doTabs();
	    
					if ( numGroups != 1 ) {
						sprintf( ext,  "_%d",  groupNo );
					}
					else {
						strcpy( ext,  "" );
					}

					fprintf(outputFile,  "%d P %s%s",  level,  name, ext );
					doCR();
					tabs++;
					outputTransforms( shapeNo,  rtg_output_transforms );
					tabs--;
				}
			
			} else {

				doTabs();
				fprintf(outputFile,  "%d G %s",  level,  name );
				doCR();
				tabs++;
				outputTransforms( shapeNo,  rtg_output_transforms );
				tabs--;
			}

			for ( childNo = 0; childNo < numChildren; childNo++ ) {
				defineChildren( pass,  children[childNo],  level+1 );
			}
	
			tabs--;
	     	     
		}

    }

	// Free the memory allocated.

  	if ( children )
		free( children );
	
}

void
defineTextureTable()
{
	int count;
	int	XSize, YSize, Components;
	char	*texName;
	char	*texFName;
	char	*outName;
	char	*outDir;
	char	OutBuffer[1024];
	char	OutTmpBuf[512];
	char	cmd[2048];
	char	imageExt[64] = "rgb";
	char	imageType[64] = "SGI";
	char	imageFormat[64] = "SGI";

    unsigned char  *LImage;

	// Need to add in option for user to specify the output format wanted
	// SGI, Alias PIX, ... lets keep them to those that imgcvt can handle

	if ( rtg_v18_compatible )
	{
        DtGetDirectory( &outDir );
        DtSceneGetName( &outName );

		DtTextureGetSceneCount( &count );

        switch ( rtg_output_image_format )
        {
           case 1:
				strcpy( imageExt, "rgb" );
				strcpy( imageType, "SGI" );
				strcpy( imageFormat, "SGI" );
               break;
                        
           case 2:
				strcpy( imageExt, "pix" );
				strcpy( imageType, "ALIAS_PIX" );
				strcpy( imageFormat, "PIX" );
               break;
        }       

		doTabs();
		fprintf( outputFile, "TEXTURE_TABLE %s %d entries ( %s )",
				outName, count, imageType );
		doCR();

		for ( int i = 0; i < count; i++ )
		{
			DtTextureGetImageSizeByID( i, &XSize, &YSize, &Components);
			DtTextureGetNameID( i, &texName);
			DtTextureGetImageByID ( i, &LImage);
			DtTextureGetFileNameID( i, &texFName );
			
			// Be careful here.  Even if there is an image, it may not be the actual
			// image if the DtExt_setOriginalTexture( true ) has been called.

			if ( !DtExt_OriginalTexture() && LImage )
			{
				sprintf (OutBuffer, "%s/%s_%s.%s",
                                 outDir, outName, texName, imageExt);
                sprintf( OutTmpBuf, "%s/MDt_tmp%d.iff",
                                 getenv("TMPDIR"), getpid() ); 

                IFFimageWriter writer;
                MStatus Wstat;
                float zBuffer[1024];
                                        
                Wstat = writer.open( OutTmpBuf );
                if ( Wstat != MS::kSuccess )
                {
                	cerr << "error opening write file "
                         << OutBuffer << endl;
                                            
                } else {
                                           
                    writer.setSize( XSize, YSize);
                    writer.setBytesPerChannel( 1 );
                    writer.setType( ILH_RGBA );
                    //writer.outFilter( "imgcvt -t sgi");                                           
                                            
                    Wstat = writer.writeImage( (byte *)LImage, zBuffer, 0 );                        
                    if ( Wstat != MS::kSuccess )
                    {
                        cerr << "error on writing "
                             << writer.errorString() << endl;
                    }
                                               
                    writer.close();

                    //
                    // Probably want to change this to convert to some 
                    // user defined type in the future
                    //
                    // Need to convert to SGI or PIX format
                                           
					switch ( rtg_output_image_format ) {
					case 1:
	                    sprintf(cmd, "imgcvt -t sgi %s %s",
                                         OutTmpBuf, OutBuffer );
						break;
					case 2:
                    	sprintf(cmd, "imgcvt -t als %s %s",
                                         OutTmpBuf, OutBuffer );
					}

                    system( cmd );
                                            
                    // Now lets remove the tmp file
                                            
                    unlink( OutTmpBuf );
				}
                                                

			}
		
			// in the following the 1st "texName" is actually supposed to
			// be the material name that uses this texture.  Too late at
			// the moment to work it out.

			doTabs();

			if ( DtExt_OriginalTexture() )
			{
				fprintf( outputFile, "%d %s %d %d %s %s",
									i, "user", XSize, YSize, 
									texName, texFName ? texFName : "(Null)" );
			} else {
				fprintf( outputFile, "%d %s %d %d %s %s_%s.%s",
									i, imageFormat, XSize, YSize, 
										texName, outName, texName, imageExt );
			}
			doCR();
		}

		doTabs();
		fprintf( outputFile, "END_TEXTURE_TABLE %s",
					outName );
		doCR();

	}
}


void
dumpAllTextures( void /*AlTexture*/ *texture )
{
    // FIXTHIS was an AlTexture

#ifdef FIXTHIS

	void        *subTexture;
	const char      *object;
	const char      *filename;
	const char      *channel;
	const char      *textype;

	channel = texture->fieldType();
	textype = texture->textureType();

	if ( subTexture = texture->firstTexture() ) {

		do {

			dumpAllTextures( subTexture );

		} while ( texture->nextTextureD( texture ) == sSuccess );
	}

	if ( object = texture->firstPerObjectPixEntry() ) {

		do {
			
			filename = texture->getPerObjectPixFilename( object );

			doTabs();
			fprintf( outputFile, "FILENAME %s %s %s %s", 
					 channel, textype, filename ? filename : "", 
					 object ? object: "" );
			doCR();

		} while ( object = texture->nextPerObjectPixEntry( object ) );
	}
	else if ( filename = texture->filename() ){
		
		doTabs();
		fprintf( outputFile, "FILENAME %s %s %s", 
				 channel, textype, filename ? filename : "" );
		doCR();
	}

#endif

}

void
getTextureFiles( char *matName, char *textureName )
{

#ifdef FIXTHIS

	AlShader  *material;
	AlTexture *texture;

	if ( material = AlUniverse::firstShader() ) {

		do {
			if (! strcmp( material->name(), matName ) ) {

				if ( texture = material->firstTexture() ) {

					do {

						dumpAllTextures( texture );

					} while ( material->nextTextureD( texture ) == sSuccess );
				}
			}
		} while ( AlUniverse::nextShaderD( material ) == sSuccess );
	}

#endif

} 

void
defineMultiTextures( int matNum )
{
	char * MaterialName;
    char * TextureName;
	char * FileName;
	int	   texId;

    DtMtlGetNameByID( matNum, &MaterialName );

	DtTextureGetNameMulti( MaterialName, "ambient", &TextureName );
	if ( TextureName )
	{
		doTabs();
    	fprintf(outputFile, "AMBIENT_NAME %s", TextureName );
    	doCR();

		DtTextureGetIDMulti( matNum, "ambient", &texId);

		if ( texId > -1 )
		{
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "AMBIENT_FILENAME %s", FileName );
				doCR();
			} else {

				printf ( "ambient filename not found\n" );
				}
		}
		
	}

    DtTextureGetNameMulti( MaterialName, "incandescence", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "INCANDESCENCE_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "incandescence", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "INCANDESCENCE_FILENAME %s", FileName );
        		doCR();
			}
		}

    }

    DtTextureGetNameMulti( MaterialName, "bump", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "BUMP_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "bump", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "BUMP_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "diffuse", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "DIFFUSE_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "diffuse", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "DIFFUSE_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "translucence", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "TRANSLUCENCE_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "translucence", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "TRANSLUCENCE_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "shininess", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "SHININESS_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "SHININESS", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "SHININESS_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "specular", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "SPECULAR_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "specular", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "SPECULAR_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "reflectivity", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "REFLECTIVITY_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "reflectivity", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "REFLECTIVITY_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

    DtTextureGetNameMulti( MaterialName, "reflected", &TextureName );
    if ( TextureName )
    {
        doTabs();
        fprintf(outputFile, "REFLECTED_NAME %s", TextureName );
        doCR();

        DtTextureGetIDMulti( matNum, "reflected", &texId);
        if ( texId > -1 )
        {
            DtTextureGetFileNameID( texId, &FileName );
            if ( FileName && FileName[0] )
			{
				doTabs();
                fprintf(outputFile, "REFLECTED_FILENAME %s", FileName );
        		doCR();
			}
		}
    }

}

void
defineMaterials(void)
{
	int	  count;
	int   matNum;
	char *name;
	float ared, agreen, ablue;
	float dred, dgreen, dblue;
	float sred, sgreen, sblue;
	float ered, egreen, eblue;
	float shininess;
	float transparency;
	char *texName;

	if ( rtg_output_materials ) {

		DtMtlGetSceneCount( &count );

		doTabs();
		fprintf(outputFile, "MATERIAL_LIST %d", count);
		doCR();

		for ( matNum = 0; matNum < count; matNum++ ) {

			tabs++;

			doTabs();
			fprintf(outputFile, "MATERIAL %d", matNum);
			doCR();

			DtMtlGetNameByID( matNum, &name );

			DtMtlGetAllClrbyID( matNum,0, 
							    &ared, &agreen,&ablue,
							    &dred, &dgreen, &dblue,
							    &sred, &sgreen, &sblue,
							    &ered, &egreen, &eblue,
							    &shininess, &transparency );


			DtTextureGetName( name, &texName );


			doTabs();
			fprintf(outputFile, "NAME %s", name );
			doCR();

			doTabs();
			fprintf(outputFile, "AMBIENT %f %f %f", ared, agreen, ablue );
			doCR();

			doTabs();
			fprintf(outputFile, "DIFFUSE %f %f %f", dred, dgreen, dblue );
			doCR();

			doTabs();
			fprintf(outputFile, "SPECULAR %f %f %f", sred, sgreen, sblue );
			doCR();

			doTabs();
			fprintf(outputFile, "EMMISION %f %f %f", ered, egreen, eblue );
			doCR();

			doTabs();
			fprintf(outputFile, "SHININESS %f", shininess );
			doCR();

			doTabs();
			fprintf(outputFile, "TRANSPARENCY %f", transparency );
			doCR();

			if ( texName ) {
				doTabs();
				fprintf(outputFile, "TEXTURE_NAME %s", texName );
				doCR();

				//getTextureFiles( name, texName );
				char *fName = NULL;
				DtTextureGetFileName( name, &fName );
				if ( fName && fName[0] )
				{
					doTabs();
					fprintf(outputFile, "TEXTURE_FILENAME %s", fName );
					doCR();
				}
					
			}

			if ( DtExt_MultiTexture() )
			{
				defineMultiTextures( matNum );
			}

			doCR();

			tabs--;
		}
		doTabs();
		fprintf(outputFile, "END_MATERIAL_LIST");
		doCR();
		doCR();

	}
}

void
defineFigure(void)
{
    int numShapes;
    int shapeNo;

    numShapes = DtShapeGetCount();

    tabs = -1;    
	doTabs();
	fprintf( outputFile, "HIERARCHY_LIST %s %d top_level",
			 rtg_output_hierarchy ? (
				 rtg_output_pivots ? "HXP" : "HX") : "H",
			 getTopLevelCount() );
	doCR();
    for ( shapeNo = 0; shapeNo < numShapes; shapeNo++ ) {
	
	/*
	 * Process all shapes with no parent,  as they are the
	 * roots of the heirarchy. defineChildren will process
	 * all the geometry in this or lower level nodes.
	 */
		if ( DtShapeGetParentID( shapeNo ) < 0 ) {
	
			defineChildren( HEIRARCHY,  shapeNo,  0 );	
		}
    }
	doTabs();
	fprintf( outputFile, "END_HIERARCHY_LIST" );
	doCR();
	doCR();


    tabs = 0;
    for ( shapeNo = 0; shapeNo < numShapes; shapeNo++ ) {
	
		/*
		 * Process all shapes with no parent,  as they are the
		 * roots of the heirarchy. defineChildren will process
		 * all the geometry in this or lower level nodes.
		 */
		if ( DtShapeGetParentID( shapeNo ) < 0 ) {
	
			defineChildren( GEOMETRY,  shapeNo,  0 );	
		}
    }


}
	

int
rtgExport()
{
	char				 *outName;
	char				  buffer[1024];

	if ( (DtExt_outputTransforms() == kTRANSFORMMINIMAL) ||
		(DtExt_outputTransforms() == kTRANSFORMALL) ) {
		rtg_output_hierarchy = TRUE;
	}
	else {
		rtg_output_hierarchy = FALSE;
	}

	DtGetDirectory( &outName );
	strcpy( buffer, outName );

	DtSceneGetName( &outName );
	strcat( buffer, "/" );
	strcat( buffer, outName );
	strcat( buffer, ".rtg" );

	if ( ( outputFile = fopen( buffer, "w" ) ) ) {
		doHeader();

		defineMaterials();

		defineFigure();

		defineTextureTable();

		defineAnim();

		fclose( outputFile );
	}


	return 0;
}

#if defined (_WIN32)
#pragma warning(default: 4244 4305)
#endif // WIN32
