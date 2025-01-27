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
// VRML2 translator for Maya    
// 
//


#ifdef WIN32

#include <direct.h>
#include <process.h>

#define NO_ENV_REDEF

#endif

// math.h needs to be included before ilib.h

#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#if defined (OSMac_)
	extern "C" char * MayaGetEnv (const char *varname);
	extern "C" int ifftoany( char * inputFileName, char * outputFileName, char * outputKey);
	extern "C" int ifftoanyClose();
#else
#	include <sys/types.h>
#	include <sys/stat.h>
#endif
// Need to work around some file problems on the NT platform with Maya 1.0

#ifdef WIN32

// The include file flib.h needs to be modified on the NT
// platform so that HAWExport.h and NTDependencies.h not included.
//
// i.e. make changes such that flib looks like:
//
// #ifndef FCHECK
// //#include <HAWExport.h>
// #else
// #define FND_EXPORT
// #endif
// //#include <"NTDependencies.h"
// #endif
//

// The following include and typedef are sufficient to enable the 
// translator to be compiled.

typedef unsigned int uint;

#include <maya/MTypes.h>

#ifndef __uint32_t
typedef __int32 __uint32_t;
#endif

// End of the NT specific modifications (Maya NT 1.0)

#endif


// Currently need to have some of the Maya API functions declared before
// using the ilib.h file

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MStatus.h>
#include <maya/MPlug.h>
#include <maya/MIntArray.h>

#include <maya/ilib.h>
#include <iffwriter.h>


#if defined (SGI) || defined (LINUX)
#include <sys/param.h>
#include <unistd.h>
#endif

#if defined (_WIN32)
#define M_PI_2  (3.1415926/2.0)
#endif

#include "vrml2Export.h"

#ifdef WIN32
#define MAXPATHLEN 512
#pragma warning(disable: 4244 4305)
#endif // WIN32

//	=====================================================================
//
//	The following string is used to define the version number to the Maya
//	plug-ins Manager.  Change this whenever the translator is modified
//

char	*vrml2Version = "1.3";

//  =====================================================================





#include <MDt.h>
#include <MDtExt.h>

#define VDt_setWalkMode		DtExt_setWalkMode
#define VDt_SceneInit		DtExt_SceneInit
#define VDt_setOutputTransforms	DtExt_setOutputTransforms
#define VDt_setParents		DtExt_setParents

#define VDt_setSoftTextures	DtExt_setSoftTextures

#define VDt_setXTextureRes	DtExt_setXTextureRes
#define VDt_setYTextureRes	DtExt_setYTextureRes

#define VDt_setDebug		DtExt_setDebug

#define VDt_dbInit		DtExt_dbInit
#define VDt_CleanUp		DtExt_CleanUp

char    VR_HeaderString[] = "VRML2.0 output created by Alias Maya ";

//Declare dummy functions in Maya where there is no equivalent yet

int 
DtSceneSetRedraws (int)
{
	return (1);
}

int 
DtShapeGetBlindData (int LShapeID, int bdId, int *bdSize, char **bdPtr)
{
	return (1);
}


extern int      mvrInitTransfer (int (*) (const char *,...));
int             mvrGUI = 1;

extern int      VR_PrintMessage (const char *,...);
extern int      VR_SetMessage (const char *,...);
extern int      VR_SetMessageLine (int, const char *,...);


typedef float   Matrix[4][4];

unsigned int   VR_StrCpyL (char *, char *);
void            VR_FilterName (char *);

//Imports from vrml2Anim
//
extern int      VR_InitMetaCycle ();

extern int 
VR_MetaCycleCreateScript (int LAnimStart, int LAnimEnd,
		  VRShapeRec * LVRShapes, int LNumOfShapes, FILE * LOutFile,
	     char *LOutBuffer, unsigned int *LBufferPtrP, char *LIndentStr,
			  int *LCurrentIndentP, int *LActMsgLineNum);

extern Boolean 
VR_GetAnimation (int *LAnimStart, int *LAnimEnd, VRShapeRec * LVRShapes,
	int LNumOfShapes, VRMaterialRec * LVRMaterials, int LNumOfMaterials,
	     VRLightRec * LVRLights, int LNumOfLights, 
		 VRCameraRec *LVRCameras, int LNumOfCameras, int *LActMsgLineNum);

extern Boolean 
VR_SaveAnimation (FILE * LOutFile, char *LOutBuffer,
	 unsigned int *LBufferPtrP, char *LIndentStr, int *LCurrentIndentP,
		  int LAnimStart, int LAnimEnd, VRShapeRec * LVRShapes,
		  int LNumOfShapes, VRMaterialRec * LVRMaterials,
	     int LNumOfMaterials, VRLightRec * LVRLights, int LNumOfLights,
		 VRCameraRec * LVRCameras, int LNumOfCameras);


int             VR_ViewerType[5] = { 0,0,0,0,0 };

int             VR_TextureSaveMethod = VRTEXTURE_SGI_IMAGEFILE;
Boolean         VR_SceneInited = false,
                VR_SoftTextures = false,
                VR_AutoUpdate = true;
Boolean         VR_Headlight = true,
                VR_EmulateAmbientLight = true;
Boolean         VR_LoopAnim = false,
                VR_AnimTrans = true,
                VR_AnimMaterials = true,
                VR_AnimLights = true,
				VR_AnimCameras = true,
                VR_AnimVertices = false,
                VR_MetaCycle = false,
                VR_AddTexturePath = false,
                VR_ViewFrames = true,
                VR_ExportNormals = true,
				VR_OppositeNormals = false,
                VR_ColorPerVertex = false,
                VR_LongLines = true;
Boolean         VR_AutoLaunchViewer = false;
Boolean         VR_ExportTextures = true;
Boolean			VR_EvalTextures = true;
Boolean			VR_OriginalTextures = false;
Boolean			VR_Compressed = false;

Boolean         VR_ConversionRun,
                VR_MetaCycleOK;
int             VR_FrameStart = 0,
                VR_FrameEnd = 0,
                VR_FrameBy = 1;
int             VR_AnimStart = 0,
                VR_AnimEnd = 0,
                VR_AnimStep = 1;
int             VR_AnimStartG = 0,
                VR_AnimEndG = 0,
                VR_AnimStepG = 1;
int				VR_KeyCurves = 0;

double          VR_FramesPerSec = 30.0;
double          VR_NavigationSpeed = 1.0;
int             VR_UseMetaCycle = 0;
int             VR_Precision = 6,
                VR_HierarchyMode = VRHRC_FLAT,
                VR_SelectionType = VRSEL_ALL;
int             VR_STextures = 0;
char            VR_SceneName[MAXPATHLEN] = "",
                VR_TexturePath[MAXPATHLEN] = "";

int             VR_XTexRes = 256,
                VR_YTexRes = 256;
int				VR_MaxXTexRes = 4096;
int				VR_MaxYTexRes = 4096;
int             VR_Verbose = 0;
int				VR_TimeSlider = 0;


int             VR_UserSetFrame = 0;

int             VR_NumOfShapes = 0,
                VR_NumOfRootShapes = 0,
                VR_NumOfMaterials = 0,
                VR_NumOfCameras = 0,
				VR_NumOfLights = 0;
				

VRShapeRec     *VR_Shapes = NULL;
VRShapeRec    **VR_RootShapes = NULL;
VRMaterialRec  *VR_Materials = NULL;
VRLightRec     *VR_Lights = NULL;
VRCameraRec	   *VR_Cameras = NULL;

double			VR_normalSign = 1.0;

MIntArray		VRML2_ShapeIsInstanced;


/* ============================================================== */
/* Macro function to close Anchor, Collision or Billboard node	 */
/* ============================================================== */

#define _VRM_CloseVRML2Node()\
{\
 if(LVRShape->Anchor)\
 {\
  M_DecIndentStr(LIndentStr,LCurrentIndent);\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"}\n");\
  LVRShape->Anchor=false;\
 }\
 if(LVRShape->Collision)\
 {\
  M_DecIndentStr(LIndentStr,LCurrentIndent);\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"}\n");\
  LVRShape->Collision=false;\
 }\
 if(LVRShape->Billboard)\
 {\
  M_DecIndentStr(LIndentStr,LCurrentIndent);\
  M_AddLine(LOutBuffer,LBufferPtr,LIndentStr,"}\n");\
  LVRShape->Billboard=false;\
 }\
}



//
//	There is some conversions that need to be done between the ranges
//	of the Maya intensity values and that of VRML2
//
//	At the moment, I'm going to limit the Maya values by clamping
//	This can be changed if output matches are too far off
//

float
MToVrml2_mapFloatRange( double zeroToInfinity )
{

#ifdef INVENTOR_FORMAT
    if ( 0.00001 >= zeroToInfinity )
        return 0.0; 
        
    if ( 100000.0 <= zeroToInfinity )
        return 1.0;
        
    int ordMag = ( int ) log10( zeroToInfinity );
    
    float   zeroToOne = .1 * (( 5.0 + ( float ) ordMag ) +
            ( zeroToInfinity * powf( .1, ( ordMag +1 ))));

#else

	// i.e assume range is 0 to 100

	float zeroToOne = zeroToInfinity * 0.01;
	
	if ( zeroToOne > 1.0 )
	{
		zeroToOne = 1.0;
	}

#endif

    return  zeroToOne;
}   


//  Need to clamp the colour range for VRML2.
//  The spec colour range is [0.0 to 1.0]

void ClampColor( DtFltRGBA vfColors, float &red, float &green, float &blue)
{
    if ( vfColors.r < 0.0 )
        red = 0;
    else if ( vfColors.r > 1.0 )
        red = 1.0;
    else
        red = vfColors.r;

    if ( vfColors.g < 0.0 )
        green = 0;
    else if ( vfColors.g > 1.0 )
        green = 1.0;
    else
        green = vfColors.g;

    if ( vfColors.b < 0.0 )
        blue = 0;
    else if ( vfColors.b > 1.0 )
        blue = 1.0;
    else
        blue = vfColors.b;

}

//  Need to clamp the colour range for VRML2.
//  The spec colour range is [0.0 to 1.0]

void ClampColor( float &red, float &green, float &blue)
{
    if ( red < 0.0 )
        red = 0;
    else if ( red > 1.0 )
        red = 1.0;

    if ( green < 0.0 )
        green = 0;
    else if ( green > 1.0 )
        green = 1.0;

    if ( blue < 0.0 )
        blue = 0;
    else if ( blue > 1.0 )
        blue = 1.0;

}

//
//	Routine that will check for the existence of VRML2 tags as dynamic
//	Attributes.

int
vrml2GetTags( int shape, int *BDsize, char **BDPtr )
{
	MStatus stat;
	MObject object;

	static   VRML2BlindDataRec vrml2Tags;

	int	ret = 1;	
	int group = 0;
	
	DtExt_ShapeGetOriginal( shape, group, object );
	    
	// Take the first dag path.
	//
	MFnDagNode fnTransNode( object, &stat );
	MDagPath dagPath;
	stat = fnTransNode.getPath( dagPath );

	MFnDagNode fnDN( dagPath );
		
	// Check the vrml2Link attribute of the node
	//
	MPlug lPlug = fnDN.findPlug( "vrml2Link", &stat );
    MPlug dPlug = fnDN.findPlug( "vrml2Desc" );
	MPlug bPlug = fnDN.findPlug( "vrml2Billboard" );
	MPlug cPlug = fnDN.findPlug( "vrml2Collision" );
	MPlug pPlug = fnDN.findPlug( "vrml2Primitive" );
    MPlug sPlug = fnDN.findPlug( "vrml2Sensor" );

	
	MString vrml2Link;
    MString vrml2Desc;

	short vrml2Billboard;
	short vrml2Collision;
	short vrml2Primitive;
	short vrml2Sensor;

	lPlug.getValue( vrml2Link );
    dPlug.getValue( vrml2Desc );
	bPlug.getValue( vrml2Billboard );
	cPlug.getValue( vrml2Collision );
	pPlug.getValue( vrml2Primitive );	
    sPlug.getValue( vrml2Sensor );


	// Now if we don't have a Link here, lets try the transform node
	// just in case it is there.

	if ( stat == MS::kFailure || vrml2Link.length() == 0 )
	{
    	DtExt_ShapeGetTransform( shape, object );
        
    	// Take the first dag path.
    	//
    	MFnDagNode fnTransNode2( object, &stat );
    	stat = fnTransNode2.getPath( dagPath );
    
    	MFnDagNode fnDN2( dagPath );
        
    	// Check the vrml2Link attribute of the node
    	//
    	lPlug = fnDN2.findPlug( "vrml2Link", &stat );
    	lPlug.getValue( vrml2Link );

        dPlug = fnDN2.findPlug( "vrml2Desc" );
        dPlug.getValue( vrml2Desc );

	    bPlug = fnDN2.findPlug( "vrml2Billboard" );
	    bPlug.getValue( vrml2Billboard );

	    cPlug = fnDN2.findPlug( "vrml2Collision" );
	    cPlug.getValue( vrml2Collision );

	    pPlug = fnDN2.findPlug( "vrml2Primitive" );
	    pPlug.getValue( vrml2Primitive );

        sPlug = fnDN2.findPlug( "vrml2Sensor" );
        sPlug.getValue( vrml2Sensor );

	}

	if ( stat == MS::kSuccess )
	{
		//printf( "found vrml2 tags, %s\n", vrml2Link.asChar() );

		if ( vrml2Link.length() > 0 )
			strcpy( vrml2Tags.Link, vrml2Link.asChar() );
		else
			strcpy( vrml2Tags.Link, "" );

		if ( vrml2Desc.length() > 0 )
			strcpy( vrml2Tags.Description, vrml2Desc.asChar() );
		else 
			strcpy( vrml2Tags.Description, "" );

        vrml2Tags.PrimitiveType = vrml2Primitive;
        vrml2Tags.CollisionType = vrml2Collision;
        vrml2Tags.BillboardType = vrml2Billboard;
        vrml2Tags.SensorType = vrml2Sensor;

		*BDPtr = (char *)(&vrml2Tags);
		*BDsize = sizeof (VRML2BlindDataRec);
		ret = 0;
	} else {

		*BDPtr = NULL;
		ret = 1;
	}

	return ret;
}

/* ============================================== */
/* Initialize the transfer for Dt		 */
/* ============================================== */
void 
VR_InitTransfer (int (*LPrintFunc) (const char *,...), Boolean LForceUpdate)
{
	if (LForceUpdate);

#ifdef WIN32
#pragma warning (disable:4390) // empty control statement. At this point don't change the code, hide the warning
#endif
	DtSceneSetRedraws ((int) VR_ViewFrames);

	VR_UserSetFrame = 1;

    //Set walk mode

	switch (VR_SelectionType)
	{
#ifdef MAYA_SRC
	case VRSEL_ALL:
		VDt_setWalkMode (ALL_Nodes);
		break;
	case VRSEL_ACTIVE:
		VDt_setWalkMode (ACTIVE_Nodes);
		break;
	case VRSEL_PICKED:
		VDt_setWalkMode (PICKED_Nodes);
		break;
#else
	case VRSEL_ALL:
		VDt_setWalkMode (ALL_Nodes);
		break;
	case VRSEL_PICKED:
		VDt_setWalkMode (ACTIVE_Nodes);
		break;
	case VRSEL_ACTIVE:
		VDt_setWalkMode (UNDER_Nodes);
		break;
#endif
	}


	VDt_SceneInit (VR_SceneName);

    //Set animation range

	if (VR_UserSetFrame)
	{
		DtFrameSetStart (VR_FrameStart);
		DtFrameSetEnd (VR_FrameEnd);
		DtFrameSetBy (VR_FrameBy);
	}


	//Set hierarchy conversion mode

	switch (VR_HierarchyMode)
	{
	case VRHRC_FULL:
		VDt_setOutputTransforms (kTRANSFORMALL);
		VDt_setParents (1);
		break;
	case VRHRC_FLAT:
		VDt_setOutputTransforms (kTRANSFORMMINIMAL);
		VDt_setParents (0);
		break;
	case VRHRC_WORLD:
		VDt_setOutputTransforms (kTRANSFORMNONE);
		VDt_setParents (0);
		break;
	}

    // Allow the user to specify if the textures should be sampled 
    // with the Texture Placement options

	VDt_setSoftTextures (VR_STextures);

	// Allow the user to specify if the textures should be evaluated with
	// convertSolidTx command or read in if it is a file texture.
	
	DtExt_setInlineTextures( VR_EvalTextures );

	//Set the size of the texture swatches to use  

	VDt_setXTextureRes (VR_XTexRes); 
	VDt_setYTextureRes (VR_YTexRes);

	DtExt_setMaxXTextureRes( VR_MaxXTexRes );
	DtExt_setMaxYTextureRes( VR_MaxYTexRes );
	
	// Say that we want to have camera info

	DtExt_setOutputCameras( 1 );

    //Now we can setup the database from the wire file geometry

	VDt_dbInit ();
}




/* ============================================================== */
/* Ken Shoemake's recommended algorithm is to convert the	 */
/* quaternion to a matrix and the matrix to Euler angles.	 */
/* We do this, of course, without generating unused matrix	 */
/* elements.							 */
/* ============================================================== */
void 
VR_MatrixToEuler (int LRotOrder, float m[3][3], float LRot[3])
{
	float           zr = 0.0f,
	                sxr,
	                cxr,
	                yr = 0.0f,
	                syr,
	                cyr,
	                xr = 0.0f,
	                szr,
	                czr;
	static float    epsilon = 1.0e-5;

	switch (LRotOrder)
	{
	case VRROT_ZYX:
		syr = -m[0][2];
		cyr = sqrt (1 - syr * syr);

        //Insufficient accuracy, assume that yr = PI / 2 && zr = 0

		if (cyr < epsilon)
		{
			xr = atan2 (-m[2][1], m[1][1]);
			yr = (syr > 0) ? M_PI_2 : -M_PI_2;		 //+/-90 degrees
				zr = 0.0;
		}
		else
		{
			xr = atan2 (m[1][2], m[2][2]);
			yr = atan2 (syr, cyr);
			zr = atan2 (m[0][1], m[0][0]);
		}
		break;

	case VRROT_YZX:
		szr = m[0][1];
		czr = sqrt (1 - szr * szr);
		if (czr < epsilon)
			//Insufficient accuracy, assume that yr = PI / 2 && zr = 0
		{
			xr = atan2 (m[1][2], m[2][2]);
			yr = 0.0;
			zr = (szr > 0) ? M_PI_2 : -M_PI_2;
		}
		else
		{
			xr = atan2 (-m[2][1], m[1][1]);
			yr = atan2 (-m[0][2], m[0][0]);
			zr = atan2 (szr, czr);
		}
		break;

	case VRROT_ZXY:
		sxr = m[1][2];
		cxr = sqrt (1 - sxr * sxr);

		if (cxr < epsilon)
			//Insufficient accuracy, assume that yr = PI / 2 && zr = 0
		{
			xr = (sxr > 0) ? M_PI_2 : -M_PI_2;
			yr = atan2 (m[2][0], m[0][0]);
			zr = 0.0;
		}
		else
		{
			xr = atan2 (sxr, cxr);
			yr = atan2 (-m[0][2], m[2][2]);
			zr = atan2 (-m[1][0], m[1][1]);
		}
		break;

	case VRROT_XZY:
		szr = -m[1][0];
		czr = sqrt (1 - szr * szr);
		if (czr < epsilon)	/* Insufficient accuracy, assume that
					 * yr=PI/2 && zr=0	 */
		{
			xr = 0.0;
			yr = atan2 (-m[0][2], m[2][2]);
			zr = (szr > 0) ? M_PI_2 : -M_PI_2;
		}
		else
		{
			xr = atan2 (m[0][2], m[1][1]);
			yr = atan2 (m[2][0], m[0][0]);
			zr = atan2 (szr, czr);
		}
		break;

	case VRROT_YXZ:
		sxr = -m[2][1];
		cxr = sqrt (1 - sxr * sxr);
		if (cxr < epsilon)	/* Insufficient accuracy, assume that
					 * yr=PI/2 && zr=0	 */
		{
			xr = (sxr > 0) ? M_PI_2 : -M_PI_2;
			yr = 0.0;
			zr = atan2 (-m[1][0], m[0][0]);
		}
		else
		{
			xr = atan2 (sxr, cxr);
			yr = atan2 (m[2][0], m[2][2]);
			zr = atan2 (m[0][1], m[1][1]);
		}
		break;

	case VRROT_XYZ:
		syr = m[2][0];
		cyr = sqrt (1 - syr * syr);
		if (cyr < epsilon)	/* Insufficient accuracy, assume that
					 * yr=PI/2 && zr=0	 */
		{
			xr = 0.0;
			yr = (syr > 0) ? M_PI_2 : -M_PI_2;
			zr = atan2 (m[0][1], m[1][1]);
		}
		else
		{
			xr = atan2 (-m[2][1], m[2][2]);
			yr = atan2 (syr, cyr);
			zr = atan2 (-m[1][1], m[0][0]);
		}
		break;
	}

	LRot[0] = xr;
	LRot[1] = yr;
	LRot[2] = zr;
}


int             VR_NXT[3] = {QY, QZ, QX};

/* ====================================================== */
/* Convert a matrix's rotation part to a quaternion	 */
/* based on the source in the book 'Advanced Animation	 */
/* and rendering techniques by Alan Watt & Mark Watt	 */
/* ====================================================== */
void 
VR_MatrixToQuaternion (float LRotMatrix[3][3], float LQ[4])
{
	double          LTr,
	                LS;
	register int    LI,
	                LJ,
	                LK;

	LTr = LRotMatrix[0][0] + LRotMatrix[1][1] + LRotMatrix[2][2];
	if (LTr > 0.0)
	{
		LS = sqrt (LTr + 1.0);
		LQ[QW] = LS * 0.5;
		LS = 0.5 / LS;
		LQ[QX] = (LRotMatrix[1][2] - LRotMatrix[2][1]) * LS;
		LQ[QY] = (LRotMatrix[2][0] - LRotMatrix[0][2]) * LS;
		LQ[QZ] = (LRotMatrix[0][1] - LRotMatrix[1][0]) * LS;
	}
	else
	{
		LI = QX;
		if (LRotMatrix[QY][QY] > LRotMatrix[QX][QX])
			LI = QY;
		if (LRotMatrix[QZ][QZ] > LRotMatrix[LI][LI])
			LI = QZ;
		LJ = VR_NXT[LI];
		LK = VR_NXT[LJ];
		LS = sqrt (LRotMatrix[LI][LI] - LRotMatrix[LJ][LJ] - 
											LRotMatrix[LK][LK] + 1.0);
		LQ[LI] = LS * 0.5;
		LS = 0.5 / LS;
		LQ[QW] = (LRotMatrix[LJ][LK] - LRotMatrix[LK][LJ]) * LS;
		LQ[LJ] = (LRotMatrix[LI][LJ] + LRotMatrix[LJ][LI]) * LS;
		LQ[LK] = (LRotMatrix[LI][LK] + LRotMatrix[LK][LI]) * LS;
	}
}


/* 
 * =============================================================
 * Decompose a matrix to translate, rotate(a quaternion) and scale
 * transformations
 * This is a bit optimized version of the one ine DtLayer.c++	
 * ============================================================
 */

void 
VR_DecomposeMatrix (const float *LMatrix, DECOMP_MAT * LDMatrix, int LRotOrder)
{
	float           LSXY,
	                LSXZ,
	                LRot[3],
	                LDet,
	                m[3][3],
	                LScaleX,
	                LScaleY,
	                LScaleZ,
	                LF;

	LDMatrix->Translate[0] = LMatrix[3 * 4 + 0];
	LDMatrix->Translate[1] = LMatrix[3 * 4 + 1];
	LDMatrix->Translate[2] = LMatrix[3 * 4 + 2];

	m[0][0] = LMatrix[0 * 4 + 0];
	m[0][1] = LMatrix[0 * 4 + 1];
	m[0][2] = LMatrix[0 * 4 + 2];

	LDMatrix->Scale[0] = LScaleX = sqrt(m[0][0] * m[0][0] + 
										m[0][1] * m[0][1] + 
										m[0][2] * m[0][2]);

	/* Normalize first row */
	LF = 1.0 / LScaleX;
	m[0][0] *= LF;
	m[0][1] *= LF;
	m[0][2] *= LF;

	/* Determine xy shear */
	LSXY =  LMatrix[0 * 4 + 0] * LMatrix[1 * 4 + 0] + 
			LMatrix[0 * 4 + 1] * LMatrix[1 * 4 + 1] + 
			LMatrix[0 * 4 + 2] * LMatrix[1 * 4 + 2];

	m[1][0] = LMatrix[1 * 4 + 0] - LSXY * LMatrix[0 * 4 + 0];
	m[1][1] = LMatrix[1 * 4 + 1] - LSXY * LMatrix[0 * 4 + 1];
	m[1][2] = LMatrix[1 * 4 + 2] - LSXY * LMatrix[0 * 4 + 2];

	LDMatrix->Scale[1] = LScaleY = sqrt(m[1][0] * m[1][0] + 
										m[1][1] * m[1][1] + 
										m[1][2] * m[1][2]);

	/* Normalize second row */
	LF = 1.0 / LScaleY;
	m[1][0] *= LF;
	m[1][1] *= LF;
	m[1][2] *= LF;

	/* Determine xz shear */
	LSXZ =  LMatrix[0 * 4 + 0] * LMatrix[2 * 4 + 0] + 
			LMatrix[0 * 4 + 1] * LMatrix[2 * 4 + 1] + 
			LMatrix[0 * 4 + 2] * LMatrix[2 * 4 + 2];

	m[2][0] = LMatrix[2 * 4 + 0] - LSXZ * LMatrix[0 * 4 + 0];
	m[2][1] = LMatrix[2 * 4 + 1] - LSXZ * LMatrix[0 * 4 + 1];
	m[2][2] = LMatrix[2 * 4 + 2] - LSXZ * LMatrix[0 * 4 + 2];

	LDMatrix->Scale[2] = LScaleZ = sqrt(m[2][0] * m[2][0] + 
										m[2][1] * m[2][1] + 
										m[2][2] * m[2][2]);

	/* Normalize third row */
	LF = 1.0 / LScaleZ;
	m[2][0] *= LF;
	m[2][1] *= LF;
	m[2][2] *= LF;

	LDet = (m[0][0] * m[1][1] * m[2][2]) + (m[0][1] * m[1][2] * m[2][0]) + (m[0][2] * m[1][0] * m[2][1]) - (m[0][2] * m[1][1] * m[2][0]) - (m[0][0] * m[1][2] * m[2][1]) - (m[0][1] * m[1][0] * m[2][2]);

	/*
	 * If the determinant of the rotation matrix is negative, negate the
	 * matrix and scale factors.
	 */
	if (LDet < 0.0)
	{
		m[0][0] *= -1.0;
		m[0][1] *= -1.0;
		m[0][2] *= -1.0;
		m[1][0] *= -1.0;
		m[1][1] *= -1.0;
		m[1][2] *= -1.0;
		m[2][0] *= -1.0;
		m[2][1] *= -1.0;
		m[2][2] *= -1.0;

		LDMatrix->Scale[0] *= -1.0;
		LDMatrix->Scale[1] *= -1.0;
		LDMatrix->Scale[2] *= -1.0;
	}

	//Copy the 3 x3 rotation matrix into the decomposition structure.
		memcpy (LDMatrix->RotMatrix, m, sizeof (float) * 9);

	LRot[1] = asin (-m[0][2]);
	if (fabs (cos (LRot[1])) > 0.0001)
	{
		LRot[0] = asin (m[1][2] / cos (LRot[1]));
		LRot[2] = asin (m[0][1] / cos (LRot[1]));
	}
	else
	{
		LRot[0] = acos (m[1][1]);
		LRot[2] = 0.0;
	}

	if (LRotOrder != VRROT_NO_EULER)
	{
		VR_MatrixToEuler (LRotOrder, m, LRot);
		LDMatrix->RotationAngles[0] = LRot[0];
		LDMatrix->RotationAngles[1] = LRot[1];
		LDMatrix->RotationAngles[2] = LRot[2];
	}
	else
	{
		LDMatrix->RotationAngles[0] = 0.0;
		LDMatrix->RotationAngles[1] = 0.0;
		LDMatrix->RotationAngles[2] = 0.0;
	}
	VR_MatrixToQuaternion (m, LDMatrix->Orientation);
}


/* ============================================== */
/* Copy a string and return the length of it	 */
/* ============================================== */
unsigned int 
VR_StrCpyL (register char *LStr0, register char *LStr1)
{
	register unsigned int LC;

	for (LC = 0; LStr1[LC] != '\0'; LC++)
		LStr0[LC] = LStr1[LC];
	return (LC);
}


/* ============================================== */
/* Get invalid characters out of a string	 */
/* ============================================== */
void 
VR_FilterName (register char *LStr)
{
	register unsigned int LC;

	for (LC = 0; LStr[LC] != '\0'; LC++)
	{
		switch (LStr[LC])
		{
		case '#':
		case '{':
		case '}':
		case '[':
		case ']':
			LStr[LC] = '_';
			break;
		}
	}
}


/* ============================================== */
/* Check if two memory areas are equal		 */
/* ============================================== */
Boolean 
VR_MemEqual (void *LB0, void *LB1, register unsigned int LLength)
{
	register unsigned char *LM0 = (unsigned char *) LB0;
	register unsigned char *LM1 = (unsigned char *) LB1;
	register unsigned int LCnt;

	LCnt = LLength;
	do
	{
		if (*(LM0++) != *(LM1++))
			return (false);
	} while (--LCnt);
	return (true);
}



/* ============================================================== */
/* Expand dir name (replace '.' with the current working dir)	 */
/* ============================================================== */
void 
VR_ExpandDir (register char *LDirN, register char *LBuf, int LLen)
{
	char            LCWD[MAXPATHLEN + 1];
	int             LL;

	if (LDirN[0] == '.')
	{
#ifdef WIN32
		_getcwd (LCWD, MAXPATHLEN);
#else
		getcwd (LCWD, MAXPATHLEN);
#endif
		if ((LL = (int)strlen (LCWD)) < LLen)
		{
			strcpy (LBuf, LCWD);
			LLen -= LL;
			if (strlen (LDirN + 1) < (unsigned int)LLen)
				strcat (LBuf, LDirN + 1);
		}
	}
	else
		strcpy (LBuf, LDirN);
}


/* ====================================== */
/* Create ROUTEs for animations		 */
/* ====================================== */
void 
VR_CreateROUTEs (FILE * LOutFile, char *LOutBuffer, unsigned int *LBufferPtrP, 
				 char *LIndentStr, int *LCurrentIndentP, 
				 VRShapeRec * LVRShapes, int LNumOfShapes, 
				 VRMaterialRec * LVRMaterials, int LNumOfMaterials, 
				 VRLightRec * LVRLights, int LNumOfLights,
				 VRCameraRec * LVRCameras, int LNumOfCameras)
{
	register int	LC;
	unsigned int   LBufferPtr = *LBufferPtrP;
	int             LCurrentIndent = *LCurrentIndentP;
	char           *LName;
	char            LNameF[256];
	char            LTmpStr[512];
	char           *LTmpStrC;

	//Create ROUTEs for transformations
	//
	if (VR_AnimTrans)
	{
		for (LC = 0; LC < LNumOfShapes; LC++)
		{
			strcpy (LNameF, LVRShapes[LC].Name);

			if (LVRShapes[LC].NumOfAnimatedVertices != 0)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sVtxAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sVtxAnim.value_changed TO %sGeoPoints.set_point\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}

			if (LVRShapes[LC].TransAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sPosAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sPosAnim.value_changed TO %s.set_translation\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRShapes[LC].OriAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sRotAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sRotAnim.value_changed TO %s.set_rotation\n", LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRShapes[LC].ScaleAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sSclAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sSclAnim.value_changed TO %s.set_scale\n", LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
			{
				fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
				LBufferPtr = 0;
			}
		}
	}

	//Create ROUTEs for shader animations
	//
	if (VR_AnimMaterials)
	{
		for (LC = 0; LC < LNumOfMaterials; LC++)
		{
			DtMtlGetNameByID (LC, &LName);
			sprintf (LNameF, "%s_%d", LName, LC);
			VR_FilterName (LNameF);
			if (LVRMaterials[LC].DiffuseColAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sMatDifAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sMatDifAnim.value_changed TO %s.set_diffuseColor\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRMaterials[LC].SpecularColAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sMatSpcAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sMatSpcAnim.value_changed TO %s.set_specularColor\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRMaterials[LC].EmissiveColAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sMatEmsAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sMatEmsAnim.value_changed TO %s.set_emissiveColor\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
			if (LVRMaterials[LC].ShininessAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sMatShnAnim.set_fraction\n", 
																LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sMatShnAnim.value_changed TO %s.set_shininess\n", 
														LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
			if (LVRMaterials[LC].TransparencyAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sMatTrsAnim.set_fraction\n", 
																LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sMatTrsAnim.value_changed TO %s.set_transparency\n", 
														LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
		}
	}

	//Create ROUTEs for light animations
	//
	if (VR_AnimLights)
	{
		for (LC = 0; LC < LNumOfLights; LC++)
		{
			sprintf (LNameF, "Light_%d", LC);
			if (LVRLights[LC].LocationAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sLightLocAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sLightLocAnim.value_changed TO %s.set_location\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRLights[LC].DirectionAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sLightDirAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sLightDirAnim.value_changed TO %s.set_direction\n",
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRLights[LC].CutOffAngleAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sLightCOAAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sLightCOAAnim.value_changed TO %s.set_cutOffAngle\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRLights[LC].ColorAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sLightColAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sLightColAnim.value_changed TO %s.set_color\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
            if (LVRLights[LC].IntensityAnimated)
            {
                sprintf (LTmpStr,
        "ROUTE TimeSource.fraction_changed TO %sLightIntAnim.set_fraction\n",
                                                                    LNameF);
                M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
                sprintf (LTmpStr, 
        "ROUTE %sLightIntAnim.value_changed TO %s.set_intensity\n",
                                                            LNameF, LNameF);
                M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
            }   

		}
	}

	//Create ROUTEs for camera animations
	//
	if (VR_AnimCameras)
	{
		for (LC = 0; LC < LNumOfCameras; LC++)
		{
			if (DtCameraGetName (LC, &LTmpStrC))
				strcpy (LNameF, LTmpStrC);
			else
				sprintf (LNameF, "Camera_%d", LC);

			if (LVRCameras[LC].LocationAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sCameraLocAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sCameraLocAnim.value_changed TO %s.set_position\n", 
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}
			if (LVRCameras[LC].DirectionAnimated)
			{
				sprintf (LTmpStr, 
		"ROUTE TimeSource.fraction_changed TO %sCameraDirAnim.set_fraction\n", 
																	LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
				sprintf (LTmpStr, 
		"ROUTE %sCameraDirAnim.value_changed TO %s.set_orientation\n",
															LNameF, LNameF);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr);
			}

		}
	}

	fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
	LBufferPtr = 0;

	*LBufferPtrP = LBufferPtr;
	*LCurrentIndentP = LCurrentIndent;
}



/* ============================== */
/* Define geometries		 */
/* ============================== */
void 
VR_DefineGeometries (FILE * LOutFile, register VRShapeRec * LVRShapes, int LNumOfShapes, char *LIndentStr, int *LCurrentIndentPtr)
{
	char            LTmpStrCN[256],
	                LTmpStrCNE[256];
	char            LTmpStr0[512];
	char            LOutBuffer[VR_OUTBUFFER_SIZE];
	unsigned int   LBufferPtr;
	VRML2BlindDataRec LBDRec;
	VRML2BlindDataRec *LBDPtr;
	register int	LC0,
	                LC1,
	                LShapeN,
	                LNumOfGroups,
	                LGroupN;
	int             LShapeID,
	                LCurrentIndent;
	int             LPIdxN,
	                LVIdxN,
	                LTexNumOnShape;
	int            LBDSize;
	int           *LVIdx;
	int           *LNIdx;
	int           *LTIdx;
	DtVec2f        *LVertices2D;
	DtVec3f        *LVertices;
	DtFltRGBA	   *vfColors;
	int			    vfColorCnt;
	Boolean         LNormalsOnShape,
	                LShapeDoubleSided,
	                LShapeCPV,
	                LPolygonsGo,
	                LPolyOk;

	float			red,
					green,
					blue;
					
	double nrmSign = 1.0;
	
	LCurrentIndent = *LCurrentIndentPtr;
	LBufferPtr = 0;

	for (LShapeN = 0; LShapeN < LNumOfShapes; LShapeN++)
	{
		LBDPtr = NULL;
		LBDSize = 0;
		//fprintf (stderr, "DtShapeGetBlindDataG\n");
		fflush (stderr);

		// Instead of asking for Blind data here, we can check for 
		// vrml2 dynamical attributes.
		
	//	if (DtShapeGetBlindData (LShapeN, (int) VRML2_BLINDDATA_ID, 
	//									&LBDSize, (char **) (&LBDPtr)) != 0)
		if ( vrml2GetTags( LShapeN, &LBDSize,(char **) &LBDPtr ) )
		{
			LBDRec.PrimitiveType = VRPRIM_NONE;
			LBDRec.CollisionType = VRCOLL_OBJECT;
			LBDRec.BillboardType = VRBB_NONE;
			LBDRec.SensorType = VRSENS_NONE;
			strcpy (LBDRec.Link, "");
			strcpy (LBDRec.Description, "");
		}
		if (LBDPtr == NULL)
			LBDPtr = &LBDRec;

		//Only get groups if this shape is neither an instance nor a primitive
		//
		if (((LShapeID = VRML2_ShapeIsInstanced[LShapeN]) == -1)
			  && (LBDPtr->PrimitiveType == VRPRIM_NONE))
		{
			LShapeID = LShapeN;
			DtShapeGetVertices (LShapeID, &LVIdxN, &LVertices);
            DtShapeGetVerticesFaceColors( LShapeID, &vfColorCnt, &vfColors );


			if ((LVertices != NULL) && (LVIdxN > 0))
			{
				//Define groups
				//
				LNumOfGroups = DtGroupGetCount (LShapeID);
				LShapeDoubleSided = (0 != DtShapeIsDoubleSided (LShapeID));
				for (LGroupN = 0; LGroupN < LNumOfGroups; LGroupN++)
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "Shape\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"geometry DEF ");


					//Vertices
					//
					sprintf (LTmpStr0, "%s_%dGeo", 
									LVRShapes[LShapeN].Name, LGroupN);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
														" IndexedFaceSet\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"convex FALSE\n");
					if (LShapeDoubleSided)
					{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"solid FALSE\n");
					}
					else
					{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"solid TRUE\n");
					}

					//Define the vertex array of the Dt shape only once, 
					// at the 0 th group
					// the rest of the groups will only reference it with USE
					if (LGroupN == 0)
					{
						sprintf (LTmpStr0, 
									"coord DEF %sGeoPoints Coordinate\n", 
													LVRShapes[LShapeN].Name);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
						M_IncIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"point [\n");
						M_IncIndentStr (LIndentStr, LCurrentIndent);
						strcpy (LTmpStrCN, "%.1f %.1f %.1f,\n");
						LTmpStrCN[2] = LTmpStrCN[7] = LTmpStrCN[12] = 
													(char) VR_Precision + '0';
						strcpy (LTmpStrCNE, "%.1f %.1f %.1f\n");
						LTmpStrCNE[2] = LTmpStrCNE[7] = LTmpStrCNE[12] = 
													(char) VR_Precision + '0';
						for (LC0 = 0; LC0 < LVIdxN; LC0++)
						{
							if (LC0 < (LVIdxN - 1))
								sprintf (LTmpStr0, LTmpStrCN, 
									LVertices[LC0].vec[0], 
									LVertices[LC0].vec[1], 
									LVertices[LC0].vec[2]);
							else
								sprintf (LTmpStr0, LTmpStrCNE, 
									LVertices[LC0].vec[0], 
									LVertices[LC0].vec[1], 
									LVertices[LC0].vec[2]);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);
							if (LBufferPtr >= 
									(VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
							{
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
							}
						}
						M_DecIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
						M_DecIndentStr (LIndentStr, LCurrentIndent);

						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
						fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
						LBufferPtr = 0;
					}
					else
					{
						sprintf (LTmpStr0, "coord USE %sGeoPoints\n", 
													LVRShapes[LShapeN].Name);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);
					}


					//Polygons (vertex indices)
					//
					DtPolygonGetCount (LShapeID, LGroupN, &LPIdxN);

					//Check if there is at least one 'good' polygon
					//
					LPolygonsGo = false;
					for (LC0 = 0; LC0 < LPIdxN; LC0++)
					{
						DtPolygonGetIndices (LC0, &LVIdxN, &LVIdx, 
															&LNIdx, &LTIdx);
						if (LVIdxN > 2)
							LPolygonsGo = true;
					}

					if (LPolygonsGo)
					{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"coordIndex [\n");
						M_IncIndentStr (LIndentStr, LCurrentIndent);
						fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
						LBufferPtr = 0;
						if (LPIdxN == 1)
						{
							DtPolygonGetIndices (0, &LVIdxN, &LVIdx, 
															&LNIdx, &LTIdx);

							if (LVIdxN > 2) //Is it a 'good' polygon ?
							{
								fprintf (LOutFile, LIndentStr);
								for (LC1 = 0; LC1 < LVIdxN; LC1++)
								{
									fprintf (LOutFile, "%d ", LVIdx[LC1]);
								}
								fprintf (LOutFile, "-1\n");
							}
						}
						else
						{
							fprintf (LOutFile, LIndentStr);
							if (VR_LongLines)
							{
								for (LC0 = 0; LC0 < (LPIdxN - 1); LC0++)
								{
									DtPolygonGetIndices (LC0, &LVIdxN, 
													&LVIdx, &LNIdx, &LTIdx);

									if (LVIdxN > 2) //Is it a 'good' polygon ?
									{
										for (LC1 = 0; LC1 < LVIdxN; LC1++)
										{
											fprintf (LOutFile, "%d ", 
																LVIdx[LC1]);
										}
										fprintf (LOutFile, "-1, ");
									}
								}
							}
							else
							{
								for (LC0 = 0; LC0 < (LPIdxN - 1); LC0++)
								{
									DtPolygonGetIndices (LC0, &LVIdxN, &LVIdx, 
															&LNIdx, &LTIdx);

									if (LVIdxN > 2) //Is it a 'good' polygon ?
									{
										for (LC1 = 0; LC1 < LVIdxN; LC1++)
										{
											fprintf (LOutFile, "%d ", 
																LVIdx[LC1]);
										}
										fprintf (LOutFile, "-1, \n");
										fprintf (LOutFile, LIndentStr);
									}
								}
							}
							DtPolygonGetIndices (LC0, &LVIdxN, &LVIdx, 
															&LNIdx, &LTIdx);

							if (LVIdxN > 2) //Is it a 'good' polygon ?
							{
								for (LC1 = 0; LC1 < LVIdxN; LC1++)
								{
									fprintf (LOutFile, "%d ", LVIdx[LC1]);
								}
								fprintf (LOutFile, "-1\n");
							}
						}
						M_DecIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
						fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
						LBufferPtr = 0;
					}


					//Color per vertex
					//
					// #03{

					for (LC0 = 0, LShapeCPV = false; LC0 < vfColorCnt; LC0++)
					{
						if (vfColors[LC0].a != 0)
						{
							LShapeCPV = true;
							break;
						}
					}
		
					if (VR_ColorPerVertex && LShapeCPV)
					{
						if ( LGroupN == 0 )
						{
							sprintf (LTmpStr0,
                                    "color DEF %sVtxColors Color\n",
                                                    LVRShapes[LShapeN].Name);

							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															LTmpStr0);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
							M_IncIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"color [\n");
							M_IncIndentStr (LIndentStr, LCurrentIndent);
							if ( vfColorCnt == 1)
							{
							//	DtShapeGetVertexColor (LShapeID, 0, &LDtColor);

								ClampColor( vfColors[0], red, green, blue);

								sprintf (LTmpStr0, "%f %f %f\n", 
														red, 
														green,
														blue );
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);
							}
							else
							{
								for (LC0 = 0; LC0 < vfColorCnt; LC0++)
								{
									//DtShapeGetVertexColor (LShapeID, LC0, &LDtColor);
		                            ClampColor( vfColors[LC0], red,green,blue);

									sprintf (LTmpStr0, "%f %f %f\n", 
                        		        			red, 
													green, 
													blue );

									M_AddLine (LOutBuffer, LBufferPtr, 
														LIndentStr, LTmpStr0);
									if (LBufferPtr >= 
										(VR_OUTBUFFER_SIZE-VR_MAX_LINE_SIZE))
									{
										fwrite (LOutBuffer, 1, LBufferPtr, 
																	LOutFile);
										LBufferPtr = 0;
									}
								}
							}

							M_DecIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
							M_DecIndentStr (LIndentStr, LCurrentIndent);
							// #03}
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
							fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
							LBufferPtr = 0;
						}
						else 
						{
							sprintf( LTmpStr0, "color USE %sVtxColors\n",
										LVRShapes[LShapeN].Name );
							M_AddLine( LOutBuffer, LBufferPtr, LIndentStr,
										LTmpStr0 );
						}


						// Now output the Vertex Color Indices
						
                        DtFaceGetColorIndexByShape (LShapeID, LGroupN,
                                                            &LVIdxN, &LNIdx);

                        if ((vfColorCnt > 0) && LPolygonsGo)
                        {
                            M_AddLine (LOutBuffer, LBufferPtr, LIndentStr,
                                                            "colorIndex [\n");
                            M_IncIndentStr (LIndentStr, LCurrentIndent);
                            fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
                            LBufferPtr = 0;
                            fprintf (LOutFile, LIndentStr);
                            if (VR_LongLines)
                            {
                                for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
                                {
                                    // Check if this polygon is going
                                    // to have at least 3 vertices
                                    //
                                    if (LC1 == 0)
                                    {
                                        LPolyOk = true;
                                        if (LNIdx[LC0] == -1)
                                        {
                                            LC0++;
                                            LPolyOk = false;
                                        }
                                        else if (LNIdx[LC0 + 1] == -1)
                                        {
                                            LC0 += 2;
                                            LPolyOk = false;
                                        }
                                        else if (LNIdx[LC0 + 2] == -1)
                                        {
                                            LC0 += 3;
                                            LPolyOk = false;
                                        }

                                        if (LPolyOk)
                                        {
                                            fprintf (LOutFile, "%d ",
                                                            LNIdx[LC0]);
                                            LC0++;
                                            LC1++;
                                        }
                                    }
                                    else
                                    {
                                        if (LNIdx[LC0] == -1)
                                        {
                                            if (LC0 < (LVIdxN - 1))
                                                fprintf (LOutFile, "%d, ",
                                                            LNIdx[LC0]);
                                            else
                                                 fprintf (LOutFile, "%d\n",
                                                             LNIdx[LC0]);
                                            LC1 = 0;
                                        }
                                        else
                                        {
                                            fprintf (LOutFile, "%d ",
                                                            LNIdx[LC0]);
                                            LC1++;
                                        }
                                        LC0++;
                                    }
                                }
                            }
                            else
                            {
                                for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
                                {
                                    // Check if this polygon is going
                                    // to have at least 3 vertices
                                    //
                                    if (LC1 == 0)
                                    {
                                        LPolyOk = true;
                                        if (LNIdx[LC0] == -1)
                                        {
                                            LC0++;
                                            LPolyOk = false;
                                        }
                                        else if (LNIdx[LC0 + 1] == -1)
                                        {
                                            LC0 += 2;
                                            LPolyOk = false;
                                        }
                                        else if (LNIdx[LC0 + 2] == -1)
                                        {
                                            LC0 += 3;
                                            LPolyOk = false;
                                        }

                                        if (LPolyOk)
                                        {
                                            fprintf (LOutFile, "%d ",
                                                            LNIdx[LC0]);
                                            LC0++;
                                            LC1++;
                                        }
                                    }
                                    else
                                    {
                                        if (LNIdx[LC0] == -1)
                                        {
                                            if (LC0 < (LVIdxN - 1))
                                            {
											    fprintf (LOutFile, "%d,\n",
                                                            LNIdx[LC0]);
					                            fprintf (LOutFile, LIndentStr);
											}
                                            else
                                                fprintf (LOutFile, "%d\n",
                                                            LNIdx[LC0]);

                                            LC1 = 0;
                                        }
                                        else
                                        {
                                            fprintf (LOutFile, "%d ",
                                                            LNIdx[LC0]);
                                            LC1++;
                                        }
                                        LC0++;
                                    }
                                }
                            }

                            M_DecIndentStr (LIndentStr, LCurrentIndent);
                            M_AddLine (LOutBuffer, LBufferPtr, LIndentStr,
                                                                        "]\n");
                            fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
                            LBufferPtr = 0;
                        }
                    }


					//Normals
					//
					LNormalsOnShape = false;
					if (VR_ExportNormals && LPolygonsGo)
					{

						// Here we check to see if the user wants to flip
						// the normals for some reason, or we have check
						// for reversal for single sided objects

					    if ( VR_OppositeNormals || 
								(DtExt_Winding() &&
								 !DtShapeIsDoubleSided( LShapeID ) &&
								  DtShapeIsOpposite( LShapeID ) ) )
						        nrmSign = -1.0;
						else 
								nrmSign = 1.0;

						DtShapeGetNormals (LShapeID, &LVIdxN, &LVertices);
						if ((LVertices != NULL) && (LVIdxN > 0))
						{
							LNormalsOnShape = true;

						} else {

							DtShapeGetPolygonNormals( LShapeID, &LVIdxN,
																&LVertices);

							if ((LVertices != NULL) && (LVIdxN > 0))
							{
								LNormalsOnShape = false;
							}
						}
						if ( (LVertices != NULL) && (LVIdxN > 0))
						{
							if ( LNormalsOnShape )
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
												"normalPerVertex TRUE\n" );
							else 
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr,
							                     "normalPerVertex FALSE\n" );
							
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"normal Normal\n");
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"{\n");
							M_IncIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"vector [\n");
							M_IncIndentStr (LIndentStr, LCurrentIndent);
							strcpy (LTmpStrCN, "%.1f %.1f %.1f,\n");
							LTmpStrCN[2] = LTmpStrCN[7] = LTmpStrCN[12] = 
													(char) VR_Precision + '0';
							strcpy (LTmpStrCNE, "%.1f %.1f %.1f\n");
							LTmpStrCNE[2] = LTmpStrCNE[7] = LTmpStrCNE[12] = 
													(char) VR_Precision + '0';

							// Lets see if we are doing Polygon normals or not

						if ( LNormalsOnShape )
						{
							if (LVIdxN == 1)
							{
								sprintf (LTmpStr0, LTmpStrCNE, 
									LVertices[0].vec[0] * nrmSign, 
									LVertices[0].vec[1] * nrmSign, 
									LVertices[0].vec[2] * nrmSign);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);
							}
							else
							{
								for (LC0 = 0; LC0 < LVIdxN; LC0++)
								{
									if (LC0 < (LVIdxN - 1))
										sprintf (LTmpStr0, LTmpStrCN, 
											LVertices[LC0].vec[0] * nrmSign, 
											LVertices[LC0].vec[1] * nrmSign, 
											LVertices[LC0].vec[2] * nrmSign);
									else
										sprintf (LTmpStr0, LTmpStrCNE, 
											LVertices[LC0].vec[0] * nrmSign, 
											LVertices[LC0].vec[1] * nrmSign, 
											LVertices[LC0].vec[2] * nrmSign);
									M_AddLine (LOutBuffer, LBufferPtr, 
														LIndentStr, LTmpStr0);
									if (LBufferPtr >= 
										(VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
									{
										fwrite (LOutBuffer, 1, LBufferPtr, 
																	LOutFile);
										LBufferPtr = 0;
									}
								}
							}
							
						} else {

							// now lets do the Polygon normals

							DtShapeGetPolygonNormalIdx(LShapeID,
							                    LGroupN, &LVIdxN, &LNIdx);
							int NC0, NC1;

							if (LVIdxN > 0)
							{
								for ( NC0 = 0; NC0 < LVIdxN; NC0++ )
								{
									if ( LNIdx[ NC0 ] == -1 )
									{
										NC1 = LNIdx[ NC0 - 1];

										if (NC0 < (LVIdxN - 1))
										{
                                        sprintf (LTmpStr0, LTmpStrCN,
                                            LVertices[NC1].vec[0] * nrmSign,
                                            LVertices[NC1].vec[1] * nrmSign,
                                            LVertices[NC1].vec[2] * nrmSign);
										} else {
                                		sprintf (LTmpStr0, LTmpStrCNE,
                                	    	LVertices[NC1].vec[0] * nrmSign,
                                	    	LVertices[NC1].vec[1] * nrmSign,
                                	    	LVertices[NC1].vec[2] * nrmSign);
										}
                                		M_AddLine (LOutBuffer, 
											LBufferPtr, LIndentStr, LTmpStr0);

                                    	if (LBufferPtr >= 
                                    	    (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
                                    	{   
                                        	fwrite (LOutBuffer, 1, LBufferPtr, 
                                                                    LOutFile);
                                        	LBufferPtr = 0;
                                    	}
									}

								}

							}
								
						}
							M_DecIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"]\n");
							M_DecIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"}\n");
							fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
							LBufferPtr = 0;
						}

						//Normal indices
						//
						if (LNormalsOnShape)
						{
							DtFaceGetNormalIndexByShape (LShapeID, LGroupN, 
															&LVIdxN, &LNIdx);

							if ((LVIdxN > 0) && LPolygonsGo)
							{
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"normalIndex [\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
								fprintf (LOutFile, LIndentStr);
								if (VR_LongLines)
								{
									for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
									{
										// Check if this polygon is going 
										// to have at least 3 vertices
										//
										if (LC1 == 0)
										{
											LPolyOk = true;
											if (LNIdx[LC0] == -1)
											{
												LC0++;
												LPolyOk = false;
											}
											else if (LNIdx[LC0 + 1] == -1)
											{
												LC0 += 2;
												LPolyOk = false;
											}
											else if (LNIdx[LC0 + 2] == -1)
											{
												LC0 += 3;
												LPolyOk = false;
											}

											if (LPolyOk)
											{
												fprintf (LOutFile, "%d ", 
																LNIdx[LC0]);
												LC0++;
												LC1++;
											}
										}
										else
										{
											if (LNIdx[LC0] == -1)
											{
												if (LC0 < (LVIdxN - 1))
													fprintf (LOutFile, "%d, ", 
																LNIdx[LC0]);
												else
													fprintf (LOutFile, "%d\n", 
																LNIdx[LC0]);
												LC1 = 0;
											}
											else
											{
												fprintf (LOutFile, "%d ", 
																LNIdx[LC0]);
												LC1++;
											}
											LC0++;
										}
									}
								}
								else
								{
									for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
									{
										// Check if this polygon is going 
										// to have at least 3 vertices
										//
										if (LC1 == 0)
										{
											LPolyOk = true;
											if (LNIdx[LC0] == -1)
											{
												LC0++;
												LPolyOk = false;
											}
											else if (LNIdx[LC0 + 1] == -1)
											{
												LC0 += 2;
												LPolyOk = false;
											}
											else if (LNIdx[LC0 + 2] == -1)
											{
												LC0 += 3;
												LPolyOk = false;
											}

											if (LPolyOk)
											{
												fprintf (LOutFile, "%d ", 
																LNIdx[LC0]);
												LC0++;
												LC1++;
											}
										}
										else
										{
											if (LNIdx[LC0] == -1)
											{
												if (LC0 < (LVIdxN - 1))
												{
													fprintf (LOutFile, "%d,\n",
																LNIdx[LC0]);
						                            fprintf (LOutFile, LIndentStr);
												}
												else
													fprintf (LOutFile, "%d\n", 
																LNIdx[LC0]);
												LC1 = 0;
											}
											else
											{
												fprintf (LOutFile, "%d ", 
																LNIdx[LC0]);
												LC1++;
											}
											LC0++;
										}
									}
								}

								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"]\n");
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
							}
						}
					}

					//Textures
					//
					if (VR_ExportTextures && LPolygonsGo)
					{
						DtTextureGetCount (LShapeID, &LTexNumOnShape);
						if (LTexNumOnShape > 0)
							//Are there textures on this shape ?
						{
							//Texture coordinates
							//
							DtGroupGetTextureVertices (LShapeID, LGroupN, 
													&LVIdxN, &LVertices2D);
							if (LVIdxN > 0)
							{
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
											"texCoord TextureCoordinate\n");
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"{\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"point [\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
								if (LVIdxN == 1)
								{
									sprintf (LTmpStr0, "%f %f\n", 
											LVertices2D[0].vec[0], 
											LVertices2D[0].vec[1]);
									M_AddLine (LOutBuffer, LBufferPtr, 
													LIndentStr, LTmpStr0);
								}
								else
								{
									for (LC0 = 0; LC0 < LVIdxN; LC0++)
									{
										sprintf (LTmpStr0, "%f %f\n", 
											LVertices2D[LC0].vec[0], 
											LVertices2D[LC0].vec[1]);
										M_AddLine (LOutBuffer, LBufferPtr, 
														LIndentStr, LTmpStr0);
										if (LBufferPtr >= 
										(VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
										{
											fwrite (LOutBuffer, 1, LBufferPtr, 
																	LOutFile);
											LBufferPtr = 0;
										}
									}
								}
								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"]\n");
								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"}\n");
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
#ifdef MAYA_SRC
								free ( LVertices2D );
#else
								free ((char *) LVertices2D);
#endif
							}

							//Texture indices
							//
							DtFaceGetTextureIndexByGroup (LShapeID, LGroupN, 
															&LVIdxN, &LVIdx);
							if (LVIdxN > 0)
							{
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr,
											   "texCoordIndex [\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
								if (VR_LongLines)
								{
									for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
									{
										// Check if this polygon is going 
										// to have at least 3 vertices
										//
										if (LC1 == 0)
										{
											LPolyOk = true;
											if (LVIdx[LC0] == -1)
											{
												LC0++;
												LPolyOk = false;
											}
											else if (LVIdx[LC0 + 1] == -1)
											{
												LC0 += 2;
												LPolyOk = false;
											}
											else if (LVIdx[LC0 + 2] == -1)
											{
												LC0 += 3;
												LPolyOk = false;
											}

											if (LPolyOk)
											{
				                                fprintf (LOutFile, LIndentStr);
												fprintf (LOutFile, "%d ", 
																LVIdx[LC0]);
												LC0++;
												LC1++;
											}
										}
										else
										{
											if (LVIdx[LC0] == -1)
											{
												if (LC0 < (LVIdxN - 1))
													fprintf (LOutFile, "%d, ", 
																LVIdx[LC0]);
												else
													fprintf (LOutFile, "%d\n", 
																LVIdx[LC0]);
												LC1 = 0;
											}
											else
											{
												fprintf (LOutFile, "%d ", 
																LVIdx[LC0]);
												LC1++;
											}
											LC0++;
										}
									}
								}
								else
								{
									for (LC0 = 0, LC1 = 0; LC0 < LVIdxN;)
									{
										//Check if this polygon is going 
										// to have at least 3 vertices
										//
										if (LC1 == 0)
										{
											LPolyOk = true;
											if (LVIdx[LC0] == -1)
											{
												LC0++;
												LPolyOk = false;
											}
											else if (LVIdx[LC0 + 1] == -1)
											{
												LC0 += 2;
												LPolyOk = false;
											}
											else if (LVIdx[LC0 + 2] == -1)
											{
												LC0 += 3;
												LPolyOk = false;
											}

											if (LPolyOk)
											{
				                                fprintf (LOutFile, LIndentStr);
												fprintf (LOutFile, "%d ", 
																LVIdx[LC0]);
												LC0++;
												LC1++;
											}
										}
										else
										{
											if (LVIdx[LC0] == -1)
											{
												if (LC0 < (LVIdxN - 1))
													fprintf (LOutFile, "%d,\n",
																LVIdx[LC0]);
												else
													fprintf (LOutFile, "%d\n", 
																LVIdx[LC0]);
												LC1 = 0;
											}
											else
											{
												fprintf (LOutFile, "%d ", 
																LVIdx[LC0]);
												LC1++;
											}
											LC0++;
										}
									}
								}

								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"]\n");
								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
							}
						}
					}

					// #04} close the DtGroup
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}
		}
	}

	//Close the Switch statement
	//
	M_DecIndentStr (LIndentStr, LCurrentIndent);
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
	M_DecIndentStr (LIndentStr, LCurrentIndent);
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");


	// #01{
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "Group\n{\n");
	M_IncIndentStr (LIndentStr, LCurrentIndent);
	// #02[
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "children\n");
	M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
	M_IncIndentStr (LIndentStr, LCurrentIndent);
	fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
	LBufferPtr = 0;

	*LCurrentIndentPtr = LCurrentIndent;
}


/* ============================================== */
/* Free allocated shapes, materials and lights	 */
/* ============================================== */
void 
VR_FreeScene ()
{
	int    LC;

	if (VR_RootShapes != NULL)
	{
		free ((char *) VR_RootShapes);
		VR_RootShapes = NULL;
		VR_NumOfRootShapes = 0;
	}
	if (VR_Shapes != NULL)
	{
		for (LC = 0; LC < VR_NumOfShapes; LC++)
		{
			if (VR_Shapes[LC].AnimatedVertices != NULL)
				free ((char *) (VR_Shapes[LC].AnimatedVertices));
			if (VR_Shapes[LC].VertexAnimation != NULL)
				free ((char *) (VR_Shapes[LC].VertexAnimation));
			if (VR_Shapes[LC].TransformAnimation != NULL)
				free ((char *) (VR_Shapes[LC].TransformAnimation));
			if (VR_Shapes[LC].trsAnimKeyFrames )
				delete VR_Shapes[LC].trsAnimKeyFrames;
            if (VR_Shapes[LC].vtxAnimKeyFrames )
                delete VR_Shapes[LC].vtxAnimKeyFrames;
		}
		free ((char *) VR_Shapes);
		VR_Shapes = NULL;
		VR_NumOfShapes = 0;
	}
	if (VR_Materials != NULL)
	{
		for (LC = 0; LC < VR_NumOfMaterials; LC++)
		{
			if (VR_Materials[LC].Animation != NULL)
				free ((char *) (VR_Materials[LC].Animation));
            if (VR_Materials[LC].AnimKeyFrames )
                delete VR_Materials[LC].AnimKeyFrames;
		}
		free ((char *) VR_Materials);
		VR_Materials = NULL;
	}
	if (VR_Lights != NULL)
	{
		for (LC = 0; LC < VR_NumOfLights; LC++)
		{
			if (VR_Lights[LC].Animation != NULL)
				free ((char *) (VR_Lights[LC].Animation));
            if (VR_Lights[LC].trsAnimKeyFrames )
                delete VR_Lights[LC].trsAnimKeyFrames;
            if (VR_Lights[LC].AnimKeyFrames )
                delete VR_Lights[LC].AnimKeyFrames;
		}
		free ((char *) VR_Lights);
		VR_Lights = NULL;
	}

	if (VR_Cameras != NULL)
	{
		for (LC = 0; LC < VR_NumOfCameras; LC++)
		{
			if (VR_Cameras[LC].Animation != NULL)
				free ((char *) (VR_Cameras[LC].Animation));
            if (VR_Cameras[LC].trsAnimKeyFrames )
                delete VR_Cameras[LC].trsAnimKeyFrames;
            if (VR_Cameras[LC].AnimKeyFrames )
                delete VR_Cameras[LC].AnimKeyFrames;
		}
		free ((char *) VR_Cameras);
		VR_Cameras = NULL;
	}
}


/* ============================================================== */
/* Get the scene from Dt and save it to the VRML2.0 file	 */
/* ============================================================== */
Boolean 
VR_GetScene (char *LFileNameRet, Boolean LUpdateScene)
{
	float           LRF,
	                LGF,
	                LBF,
	                LRF1,
	                LGF1,
	                LBF1,
	                LRF2,
	                LGF2,
	                LBF2,
	                LRF3,
	                LGF3,
	                LBF3,
	                LWF,
	                LF1;
	register float  LF;
	float          *LMatrix;
	VRShapeRec     *LVRShape;
	VRShapeRec     *LVRShape1;
	VRShapeRec     *LVRShape2;
	VRShapeRec     *LVRShapeParent;
	float          *LQ;
	int             LPIdxN,
	                LVIdxN,
	                LXSize,
	                LYSize,
	                LTexNumOnShape;
	int             LMatID,
	                LOn;
	FILE           *LOutFile;
	register int	LC0,
	                LC1,
	                LC2,
	                LShapeN,
	                LNumOfGroups,
	                LGroupN;
	int             LShapeID,
	                LParentID,
	                LCurrentIndent = 0,
	                LActMsgLineNum;
	int			LRootShapeCnt;
	unsigned int	LBufferPtr;
	DECOMP_MAT      LDecomposedMatrix;
	char           *LName;
	char           *LMatName;
	char            LMatNameF[256];
	char           *LTxtName;
	char		   *LTexFName;
	char           *LTextureDir;
	unsigned char  *LImage;
	char            LOutBuffer[VR_OUTBUFFER_SIZE];
	char			LOutTmpBuf[VR_OUTBUFFER_SIZE];
	char            LExportDir[MAXPATHLEN + 1];
	char            LTextureExportDir[MAXPATHLEN + 1];
	char            LSceneName[MAXPATHLEN + 1];
	char            LIndentStr[256];
	char            LTmpStr0[512];
	char            LShapeName[256];
	char            LLightName[256];
	char			cmd[2048];
	char           *LTmpStrC;
	VRML2BlindDataRec LBDRec;
	VRML2BlindDataRec *LBDPtr;
	int            LBDSize;
	Boolean         LGoTxt,
	                LLampOk,
	                LAnimated,
	                LMultShp;

	LTmpStr0[0] = '\0';
	memset (LIndentStr, (int) ' ', 256);
	LFileNameRet[0] = '\0';

	strcpy (LSceneName, VR_SceneName);

	LAnimated = false;
	if (DtGetDirectory (&LName) > 0)
	{
		if (LUpdateScene)
		{
			if ((VR_NumOfShapes = DtShapeGetCount ()) == 0)
			{
				VR_SetMessage ("No shapes in the scene!\n");
				VDt_CleanUp ();
				return (false);
			}
		}
		VR_ConversionRun = true;
		VR_ExpandDir (LName, LExportDir, MAXPATHLEN);

		//If no texture path is given, default it to the 'ExportDir'
		//
		if (VR_TexturePath[0] == '\0')
			LTextureDir = LExportDir;
		else
			LTextureDir = VR_TexturePath;

		sprintf (LOutBuffer, "%s/%s.wrl", LExportDir, LSceneName);


		//
		//	Check that the instances that the MDt library defined are 
		//	actually real instances for VRML2.  Maya can have instanced
		//	geometry, but have different shaders on that geometry.
		//	So need to determine if in fact the shaders are all the same
		//	for those shapes that MDt say are instances.

		int curInstance;
		
		for (LC0 = 0; LC0 < VR_NumOfShapes; LC0++)
        {
			VRML2_ShapeIsInstanced.append( DtExt_ShapeIsInstanced( LC0 ) );
			
			curInstance = VRML2_ShapeIsInstanced[LC0];
			
			if (curInstance	>= 0 )
			{
				LNumOfGroups = DtGroupGetCount(curInstance);

				if ( LNumOfGroups != DtGroupGetCount( LC0 ) )
				{
					// Here the number of groups is different so we know
					// that they are different shader assignments
					
					VRML2_ShapeIsInstanced[LC0] = -1;
				} else {
				
					// Here we should then check each of the group definitions
					// to see if they are the same.  If there are no groups
					// then everything is ok.
			
					for ( LC1 = 0; LC1 < LNumOfGroups; LC1++ )
					{
						// Would have to check each of the groups to see that
						// the defining data is the same.  Going to skip this 
						// part for now, to make it complete should really
						// complete this.

					}
				}
			}
		}

		if ((LOutFile = fopen (LOutBuffer, "wb")) != NULL)
		{
			strcpy (LFileNameRet, LOutBuffer);
			LActMsgLineNum = 1;
			VR_SetMessageLine (LActMsgLineNum, "Saving data\n");
			LActMsgLineNum++;
			if (LUpdateScene)
			{
				DtMtlGetSceneCount (&VR_NumOfMaterials);
				DtLightGetCount (&VR_NumOfLights);
				DtCameraGetCount (&VR_NumOfCameras);

				VR_AnimStart = DtFrameGetStart ();
				VR_AnimEnd = DtFrameGetEnd ();
				VR_AnimStep = DtFrameGetBy();

				VR_Shapes = (VRShapeRec *) 
							calloc (VR_NumOfShapes, sizeof (VRShapeRec)); 
				if ( VR_Shapes == NULL )
				{
					VDt_CleanUp ();
					VR_FreeScene ();

					return (false);
				}
				for (LC0 = 0; LC0 < VR_NumOfShapes; LC0++)
				{
					VR_Shapes[LC0].NumOfAnimatedVertices = 0;
					VR_Shapes[LC0].AnimatedVertices = NULL;
					VR_Shapes[LC0].VertexAnimation = NULL;
					DtShapeGetName (LC0, &LName);
					VR_FilterName (LName);
					strcpy (VR_Shapes[LC0].Name, LName);
					VR_Shapes[LC0].TransformAnimation = NULL;
					VR_Shapes[LC0].TransAnimated = false;
					VR_Shapes[LC0].OriAnimated = false;
					VR_Shapes[LC0].ScaleAnimated = false;
				}
				for (LC0 = 0; LC0 < VR_NumOfShapes; LC0++)
				{
					LC2 = 0;
					strcpy (LName, VR_Shapes[LC0].Name);
					for (LC1 = LC0 + 1; LC1 < VR_NumOfShapes; LC1++)
					{
						if (strcmp (VR_Shapes[LC1].Name, LName) == 0)
						{
							sprintf (LTmpStr0, "_%d", LC2);
							LC2++;
							strcat (VR_Shapes[LC1].Name, LTmpStr0);
						}
					}
				}
			}

			if (VR_AnimEnd > VR_AnimStart)
			{
				if (LUpdateScene)
				{
					//Allocate Material and Light array
					//
					if ((VR_Materials = (VRMaterialRec *) 
						calloc(1, sizeof( VRMaterialRec)*VR_NumOfMaterials)) != NULL)
					{
						for (LC0 = 0; LC0 < VR_NumOfMaterials; LC0++)
							VR_Materials[LC0].Animation = NULL;
					}
					if ((VR_Lights = (VRLightRec *) 
						calloc (1, sizeof (VRLightRec) * VR_NumOfLights)) != NULL)
					{
						for (LC0 = 0; LC0 < VR_NumOfLights; LC0++)
							VR_Lights[LC0].Animation = NULL;
					}
					if ((VR_Cameras = (VRCameraRec *) 
						calloc (1, sizeof (VRCameraRec) * VR_NumOfCameras)) != NULL)
					{
						for (LC0 = 0; LC0 < VR_NumOfCameras; LC0++)
							VR_Cameras[LC0].Animation = NULL;
					}
				}
				LAnimated = true;
			}

			//Save texture images
			//
			if (VR_ExportTextures)
			{
				DtTextureGetSceneCount (&LPIdxN);
				if (LPIdxN > 0)
				{
					VR_SetMessageLine (LActMsgLineNum, "Saving texture images:\n");
					LActMsgLineNum++;
					VR_ExpandDir (LTextureDir, LTextureExportDir, MAXPATHLEN);
					for (LC0 = 0, LGoTxt = true; (LC0 < LPIdxN) && LGoTxt; LC0++)
					{
						DtTextureGetNameID (LC0, &LTxtName);
						VR_FilterName (LTxtName);
						DtTextureGetImageSizeByID (LC0, &LXSize, &LYSize, &LVIdxN);
						if ((LXSize > 0) && (LYSize > 0) && (LVIdxN > 0))
						{
							DtTextureGetImageByID (LC0, &LImage);

							if (LImage != NULL)
							{
								switch (VR_TextureSaveMethod)
								{
								case VRTEXTURE_SGI_IMAGEFILE:

									switch (LVIdxN)
									{
									case 4:
										sprintf (LOutBuffer, "%s/%s_%s.rgb", 
													LTextureExportDir, 
													LSceneName, LTxtName);
#if defined(OSMac_)
										sprintf( LOutTmpBuf, 
												"%s/MDt_tmp%d.iff", 
													MayaGetEnv("TMPDIR"), getpid() );
#else
										sprintf( LOutTmpBuf, 
												"%s/MDt_tmp%d.iff", 
													getenv("TMPDIR"), getpid() );

#endif
										
										IFFimageWriter writer;
										MStatus Wstat;
										float zBuffer[1024];
										
										Wstat = writer.open( LOutTmpBuf );
										if ( Wstat != MS::kSuccess )
										{
											cerr << "error opening write file "
											<< LOutBuffer << endl;
                                            VR_SetMessageLine (LActMsgLineNum,
                                               " Failed to save texture file %s:\n", LOutBuffer);                           
                                            LGoTxt = false;
											
										} else {
												
											writer.setSize( LXSize, LYSize);
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
											
#if defined(OSMac_)
											ifftoany(LOutTmpBuf, LOutBuffer, "sgi");
											ifftoanyClose();
#endif
											//
											// Probably want to change this to convert to some
											// user defined type in the future as some players
											// don't like SGI format texture files
											//
											
											// Need to convert to SGI format
#if defined(OSMac_)
											unlink( LOutTmpBuf );
#else
											sprintf(cmd, "imgcvt -t sgi %s %s",
													LOutTmpBuf, LOutBuffer );
											system( cmd );

											// Now lets remove the tmp file

											unlink( LOutTmpBuf );
#endif

										}
										break;
									}
									break;
								}

								VR_SetMessageLine (LActMsgLineNum, 
									" %3d%%	%s:	%dx%dx%d\n",
										   ((LC0 + 1) * 100) / LPIdxN, 
										   LTxtName, LXSize, LYSize, LVIdxN);
							}
						}
					}
					LActMsgLineNum++;
				}
			}

			M_SetIndentStr (LIndentStr, LCurrentIndent, 0);
			LBufferPtr = 0;
			LBufferPtr += VR_StrCpyL (LOutBuffer, 
					"#VRML V2.0 utf8 CosmoWorlds V1.0\n\n");

			// #01{
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "WorldInfo\n");
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "title	");
			LOutBuffer[LBufferPtr++] = '"';
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LSceneName);
			LOutBuffer[LBufferPtr++] = '"';
			LOutBuffer[LBufferPtr++] = '\n';


			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "info	");
			LOutBuffer[LBufferPtr++] = '"';
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr,
						"VRML2.0 created with Version " );
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, vrml2Version );
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
						", from Alias Maya " );
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
							(char *)(MGlobal::mayaVersion().asChar()) );
			
			LOutBuffer[LBufferPtr++] = '"';
			LOutBuffer[LBufferPtr++] = '\n';
			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "NavigationInfo\n");
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);

			// Now lets take a look at the viewer types to output

			if ( VR_ViewerType[0] || VR_ViewerType[1] || VR_ViewerType[2] ||
				 VR_ViewerType[3] || VR_ViewerType[4] ) 
			{
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "type [");

				if ( VR_ViewerType[0] )
				{
					LBufferPtr += VR_StrCpyL(LOutBuffer+LBufferPtr, 
															" \"WALK\" ");
				}
				if ( VR_ViewerType[1] )
                {
					LBufferPtr += VR_StrCpyL(LOutBuffer+LBufferPtr, 
														" \"EXAMINE\" ");
				}
				if ( VR_ViewerType[2] )
                {
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
														" \"FLY\" ");
				}
                if ( VR_ViewerType[3] )
                {
                    LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
                                                        " \"ANY\" ");
                }   
                if ( VR_ViewerType[4] )
                {
                    LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
                                                        " \"NONE\" ");
                }   

				LOutBuffer[LBufferPtr++] = ']';
				LOutBuffer[LBufferPtr++] = '\n';
			}

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "headlight	");
			if (VR_Headlight)
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, "TRUE\n");
			else
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, "FALSE\n");

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "speed		");
			sprintf (LTmpStr0, "%f\n", VR_NavigationSpeed);
			LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

			// #01}

			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;
			VR_MetaCycleOK = false;

			/*
			 * if ( VR_MetaCycle ) { VR_InitMetaCycle( );
			 * 
			 * if (
			 * LAnimated=VR_GetAnimation(&VR_AnimStart,&VR_AnimEnd
			 * ,VR_Shapes,LNumOfShapes,VR_Materials,VR_NumOfMateri
			 * als,VR_Lights,VR_NumOfLights,&LActMsgLineNum)) {
			 * VR_MetaCycleCreateScript(VR_AnimStart,VR_AnimEnd,VR
			 * _Shapes,LNumOfShapes,LOutFile,LOutBuffer,&LBufferPt
			 * r,LIndentStr,&LCurrentIndent,&LActMsgLineNum );
			 * VR_MetaCycleOK=true; } }
			 */

			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "Switch\n");
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "whichChoice	-1\n");
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "choice\n");
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
			M_IncIndentStr (LIndentStr, LCurrentIndent);
			
			//Define materials
			//
			VR_SetMessageLine (LActMsgLineNum, "Saving materials:\n");
			LActMsgLineNum++;
			for (LC0 = 0; LC0 < VR_NumOfMaterials; LC0++)
			{
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "Shape\n");
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
				M_IncIndentStr (LIndentStr, LCurrentIndent);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
												"appearance Appearance\n");
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
				M_IncIndentStr (LIndentStr, LCurrentIndent);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"material DEF ");

				DtMtlGetNameByID (LC0, &LMatName);
				strcpy (LMatNameF, LMatName);
				VR_SetMessageLine (LActMsgLineNum, " %s\n", LMatNameF);
				VR_FilterName (LMatNameF);
				sprintf (LMatNameF, "%s_%d", LMatName, LC0);
				VR_FilterName (LMatNameF);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LMatNameF);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															" Material\n");
				// #03{
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
				M_IncIndentStr (LIndentStr, LCurrentIndent);

				DtMtlGetAllClrbyID (LC0, 0, &LRF, &LGF, &LBF, 
											&LRF1, &LGF1, &LBF1, 
											&LRF2, &LGF2, &LBF2, 
											&LRF3, &LGF3, &LBF3, &LWF, &LF1);

				/*
				 * M_AddLine (
				 * LOutBuffer,LBufferPtr,LIndentStr,"ambientCo
				 * lor	" ); sprintf ( LTmpStr0,"%f %f
				 * %f\n",LRF,LGF,LBF );LBufferPtr +=
				 * VR_StrCpyL (
				 * LOutBuffer+LBufferPtr,LTmpStr0 );
				 */
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"diffuseColor	");
				sprintf (LTmpStr0, "%f %f %f\n", LRF1, LGF1, LBF1);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"specularColor	");
				sprintf (LTmpStr0, "%f %f %f\n", LRF2, LGF2, LBF2);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"emissiveColor	");
				sprintf (LTmpStr0, "%f %f %f\n", LRF3, LGF3, LBF3);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

				// Material values have to between 0 - 1 for VRML2
				// Then limit to 1.0 
				
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"shininess		");
				if ( LWF > 1.0 ) LWF = 1.0;

				sprintf (LTmpStr0, "%f\n", LWF );
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);

				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"transparency	");
				sprintf (LTmpStr0, "%f\n", LF1);
				LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, LTmpStr0);
				if (LBufferPtr >= (VR_OUTBUFFER_SIZE - VR_MAX_LINE_SIZE))
				{
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
				// #03}
				M_DecIndentStr (LIndentStr, LCurrentIndent);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				M_DecIndentStr (LIndentStr, LCurrentIndent);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
				M_DecIndentStr (LIndentStr, LCurrentIndent);
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
			}
			LActMsgLineNum++;
			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;

			//Define geometries first in case we need instancing
			//
			VR_DefineGeometries (LOutFile, VR_Shapes, VR_NumOfShapes, 
												LIndentStr, &LCurrentIndent);


			//Define lights
			//
			if (!VR_EmulateAmbientLight)
			{
				DtEnvGetAmbientIntensity (&LRF);
				if (LRF != 0.0)
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"DEF Lampa ");
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															"PointLight\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"ambientIntensity	");
					sprintf (LTmpStr0, "%f\n", LRF);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LTmpStr0);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
												"location	0.0 0.0 0.0\n");
					DtEnvGetAmbientColor (&LRF, &LGF, &LBF);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "color	");

					// Clamp color, just in case
					ClampColor( LRF, LGF, LBF );

					sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}

			for (LC0 = 0; LC0 < VR_NumOfLights; LC0++)
			{
				DtLightGetType (LC0, &LVIdxN);
				sprintf (LLightName, "Light_%d", LC0);
				switch (LVIdxN)
				{
				case DT_AMBIENT_LIGHT:
					if (VR_EmulateAmbientLight)
					{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF ");
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LLightName);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															" PointLight\n");
						LLampOk = true;
					}
					else
						LLampOk = false;
					break;

				case DT_POINT_LIGHT:
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF ");
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LLightName);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															" PointLight\n");
					LLampOk = true;
					break;

				case DT_DIRECTIONAL_LIGHT:
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF ");
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LLightName);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
													" DirectionalLight\n");
					LLampOk = true;
					break;

				case DT_SPOT_LIGHT:
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "DEF ");
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LLightName);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															" SpotLight\n");
					LLampOk = true;
					break;

				default:
					LLampOk = false;
					break;
				}
				if (LLampOk)
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);

					switch (LVIdxN)
					{
					case DT_AMBIENT_LIGHT:
						if (VR_EmulateAmbientLight)
						{
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"ambientIntensity	1.0\n");
							DtLightGetPosition (LC0, &LRF, &LGF, &LBF);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"location	");
							sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
							LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
						}
						break;

					case DT_POINT_LIGHT:
						DtLightGetPosition (LC0, &LRF, &LGF, &LBF);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"location	");
						sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
						break;

					case DT_SPOT_LIGHT:
						DtLightGetPosition (LC0, &LRF, &LGF, &LBF);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"location	");
						sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LTmpStr0);

						DtLightGetDirection (LC0, &LRF, &LGF, &LBF);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"direction	");
						sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LTmpStr0);

						DtLightGetCutOffAngle (LC0, &LRF);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"cutOffAngle	");

						// cutOffAngle in Maya is measured from side to side
						// of the light.  In vrml2 measured from the center
						// so have to divide by 2
						
						sprintf (LTmpStr0, "%f\n", LRF*0.5 );
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
						break;

					case DT_DIRECTIONAL_LIGHT:
						DtLightGetDirection (LC0, &LRF, &LGF, &LBF);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"direction	");
						sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
						break;
					}

					DtLightGetColor (LC0, &LRF, &LGF, &LBF);
					DtLightGetIntensity (LC0, &LRF1);

					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "color	");

                    // Clamp color, just in case
                    ClampColor( LRF, LGF, LBF );

					sprintf (LTmpStr0, "%f %f %f\n", 
										LRF, LGF, LBF);
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LTmpStr0);

                    M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "intensity ");
                    sprintf (LTmpStr0, "%f\n", 
                                    MToVrml2_mapFloatRange( LRF1 ) );
                    LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
                                                                LTmpStr0);



					DtLightGetOn (LC0, &LOn);
					if (LOn == 0)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"on		FALSE\n");

					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}
			}

//#ifndef MAYA_SRC
			//Define cameras
			//

		    char *CameraName = NULL;
		    char fullCameraName[1024];

			if (DtCameraGetCount (&LPIdxN))
			{
				for (LC0 = 0; LC0 < LPIdxN; LC0++)
				{
					if (DtCameraGetType (LC0, &LVIdxN))
					{
						if (LVIdxN == DT_PERSPECTIVE_CAMERA)
						{
							// #03{

					        // Lets get the camera's name, 
							// if no name use index number
        					//
        					if ( DtCameraGetName( LC0, &CameraName ) )
        					{
            					sprintf( fullCameraName, 
										"DEF %s Viewpoint\n", CameraName );

					            // Check that we don't have any of those 
								// silly illegal VRML2 chars

            					VR_FilterName ( fullCameraName );
        					}
        					else
        					    sprintf( fullCameraName, 
										"DEF Camera_%d Viewpoint\n", LC0 );

							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															fullCameraName);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"{\n");
							M_IncIndentStr (LIndentStr, LCurrentIndent);

							if (DtCameraGetPosition (LC0, &LRF, &LGF, &LBF))
							{
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LIndentStr);
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, "position	");
								sprintf (LTmpStr0, "%f %f %f\n", LRF, LGF, LBF);
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
														LBufferPtr, LTmpStr0);
							}

							if (DtCameraGetOrientationQuaternion (LC0, 
													&LRF, &LGF, &LBF, &LWF))
							{

								//Convert standard quaternion to 
								// 'VRML rotation' (nx, ny, nz, angle (rad))
								// Normalized

								LF = LWF;
								LWF = acos (LF) * 2.0;
								LF = sqrt (LRF * LRF + LGF * LGF + LBF * LBF);
								if (LF > 0.0)
								{
									LF = 1.0 / LF;
									LRF *= LF;
									LGF *= LF;
									LBF *= LF;
								}
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LIndentStr);

								LBufferPtr += VR_StrCpyL (LOutBuffer + 
												LBufferPtr, "orientation	");
								sprintf (LTmpStr0, "%f %f %f %f\n", 
														LRF, LGF, LBF, LWF);
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
														LBufferPtr, LTmpStr0);

							}


							if (DtCameraGetHeightAngle (LC0, &LRF))
							{
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LIndentStr);
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
												LBufferPtr, "fieldOfView	");
								sprintf (LTmpStr0, "%f\n", LRF / VRDegToRad);
								LBufferPtr += VR_StrCpyL (LOutBuffer + 
														LBufferPtr, LTmpStr0);
							}
							LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LIndentStr);
							LBufferPtr += VR_StrCpyL (LOutBuffer + 
												LBufferPtr, "description	");

							if (DtCameraGetName (LC0, &LTmpStrC))
								strcpy (LTmpStr0, LTmpStrC);
							else
								sprintf (LTmpStr0, "Camera_%d", LC0);
							LOutBuffer[LBufferPtr++] = '"';
							LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LTmpStr0);
							LOutBuffer[LBufferPtr++] = '"';
							LOutBuffer[LBufferPtr++] = '\n';
							// #03}

							M_DecIndentStr (LIndentStr, LCurrentIndent);
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"}\n");
						}
					}
				}
			}
//#endif // MAYA_SRC

			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;

			// Create hierarchy links for the shapes and Collect root shapes 
			// (shapes with NULL parents)
			//
			if (LUpdateScene)
			{
				// Only in full hierarchy do we need to have any of this hierarchy
				// links, and this flag is set at the beginning of the
				// translation. (Wayne Arnold 4/9/98)
				
				//VDt_setParents (TRUE);
				
				if (VR_RootShapes != NULL)
				{
					free ((char *) VR_RootShapes);
					VR_RootShapes = NULL;
					VR_NumOfRootShapes = 0;
				}

				for (LShapeN = 0, LVRShape = VR_Shapes; 
							LShapeN < VR_NumOfShapes; LVRShape++, LShapeN++)
				{
					LVRShape->ID = LShapeN;
					if ((LParentID = DtShapeGetParentID (LShapeN)) != -1)
					{
						LVRShape->Parent = VR_Shapes + LParentID;
					}
					else
					{
						LVRShape->Parent = NULL;
						VR_NumOfRootShapes++;
					}
					LVRShape->Child = NULL;
					LVRShape->RightSibling = NULL;
				}

				if ((VR_RootShapes = (VRShapeRec **) malloc (sizeof (VRShapeRec *) * VR_NumOfRootShapes)) != NULL)
				{
					for (LShapeN = 0, LVRShape = VR_Shapes, LC0 = 0; 
						LShapeN < VR_NumOfShapes; LVRShape++, LShapeN++)
					{
						if (LVRShape->Parent == NULL)
						{
							VR_RootShapes[LC0] = LVRShape;
							LC0++;
						}
					}
				}


				//Set Child and RightSibling links
				//
				for (LShapeN = 0, LVRShape = VR_Shapes; 
						LShapeN < VR_NumOfShapes; LVRShape++, LShapeN++)
				{
					if ((LVRShapeParent = LVRShape->Parent) != NULL)
					{
						if (LVRShapeParent->Child == NULL)
						{
							LVRShapeParent->Child = LVRShape;
							for (LC0 = LShapeN, LVRShape1 = LVRShape + 1, 
								LVRShape2 = LVRShape; 
										LC0 < (VR_NumOfShapes - 1); 
														LVRShape1++, LC0++)
							{
								if (LVRShape1->Parent == LVRShapeParent)
								{
									LVRShape2->RightSibling = LVRShape1;
									LVRShape2 = LVRShape1;
								}
							}
						}
					}
				}
			}

			//Define the shapes
			//
			for (LShapeN = 0, LVRShape = VR_Shapes, LRootShapeCnt = 1; 
														LVRShape != NULL;)
			{
				LVRShape->Billboard = false;
				LVRShape->Anchor = false;
				LVRShape->Collision = false;

				LBDPtr = NULL;
				LBDSize = 0;
				//fprintf (stderr, "DtShapeGetBlindDataM\n");
				//fflush (stderr);
		//		if (DtShapeGetBlindData (LShapeN, (int) VRML2_BLINDDATA_ID, 
		//								&LBDSize, (char **) (&LBDPtr)) != 0)
				if ( vrml2GetTags( LVRShape->ID, &LBDSize,(char **) &LBDPtr ) )
				{
					LBDRec.PrimitiveType = VRPRIM_NONE;
					LBDRec.CollisionType = VRCOLL_OBJECT;
					LBDRec.BillboardType = VRBB_NONE;
					LBDRec.SensorType = VRSENS_NONE;
					strcpy (LBDRec.Link, "");
					strcpy (LBDRec.Description, "");
				}
				if (LBDPtr == NULL)
					LBDPtr = &LBDRec;

				/*
				 * printf("Sensortype: %d\n",LBD.SensorType
				 * );fflush(stdout );
				 */

				strcpy (LShapeName, LVRShape->Name);
				if (DtShapeGetMatrix (LVRShape->ID, &LMatrix) == 1)
				{
					// #03{
					sprintf (LTmpStr0, "DEF %s Transform\n", LShapeName);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);

						VR_DecomposeMatrix (LMatrix, 
									&LDecomposedMatrix, VRROT_NO_EULER);
					sprintf (LTmpStr0, "translation		%f %f %f\n", 
								LDecomposedMatrix.Translate[0], 
								LDecomposedMatrix.Translate[1], 
								LDecomposedMatrix.Translate[2]);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);

					M_QuaternionToVRML (LDecomposedMatrix.Orientation, LQ);
					sprintf (LTmpStr0, "rotation		%f %f %f %f\n", 
											LQ[QX], LQ[QY], LQ[QZ], LQ[QW]);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);

					sprintf (LTmpStr0, "scale		%f %f %f\n", 
								LDecomposedMatrix.Scale[0], 
								LDecomposedMatrix.Scale[1], 
								LDecomposedMatrix.Scale[2]);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, LTmpStr0);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
								"scaleOrientation	0.0  0.0  1.0  0.0\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}

				if (LBDPtr->BillboardType != VRBB_NONE)
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"children Billboard\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"axisOfRotation	");
					LVRShape->Billboard = true;
					switch (LBDPtr->BillboardType)
					{
					case VRBB_XROT:
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	"1 0 0\n");
						break;
					case VRBB_YROT:
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	"0 1 0\n");
						break;
					case VRBB_SCRALIGN:
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	"0 0 0\n");
						break;
					default:
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	"0 0 0\n");
						break;
					}
				}

				if (LBDPtr->CollisionType == VRCOLL_NONE)
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"children Collision\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"collide         FALSE\n");
					LVRShape->Collision = true;
				}

				//Is there link info (Anchor) on this shape ?
				//
				if (LBDPtr->Link[0] != '\0')
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"children Anchor\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "url ");
					LOutBuffer[LBufferPtr++] = '"';
					LVRShape->Anchor = true;

					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																LBDPtr->Link);
					LOutBuffer[LBufferPtr++] = '"';
					LOutBuffer[LBufferPtr++] = '\n';

					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"description ");
					LOutBuffer[LBufferPtr++] = '"';
					LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
														LBDPtr->Description);
					LOutBuffer[LBufferPtr++] = '"';
					LOutBuffer[LBufferPtr++] = '\n';
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}

				if (((LShapeID = VRML2_ShapeIsInstanced[LVRShape->ID]) != -1) 
												&& (LShapeID != LVRShape->ID))
				{
					;
					// Don't beleive that this should be here.
					// the children will be output below
#if 0
					for (LPIdxN = 0, LVRShape1 = NULL; 
									LPIdxN < VR_NumOfShapes; LPIdxN++)
						if (VR_Shapes[LPIdxN].ID == LShapeID)
						{
							LVRShape1 = VR_Shapes + LPIdxN;
							break;
						}
					LVRShape1 = LVRShape1->Child;
					if (LVRShape1 != NULL)
					{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"children\n");
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
						M_IncIndentStr (LIndentStr, LCurrentIndent);
						for (; LVRShape1 != NULL; 
										LVRShape1 = LVRShape1->RightSibling)
						{
							M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"USE ");
							LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
															LVRShape1->Name);
							LOutBuffer[LBufferPtr++] = '\n';
						}
						M_DecIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
					}
#endif
				}
				else
					LShapeID = LVRShape->ID;

					
				LNumOfGroups = DtGroupGetCount (LShapeID);

				if ((LVRShape->Child != NULL) || (LNumOfGroups > 1))
				{
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"children\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "[\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);
					LMultShp = true;
				}
				else
				{
					if (LNumOfGroups > 0)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"children Shape\n");
					LMultShp = false;
				}

				for (LGroupN = 0; LGroupN < LNumOfGroups; LGroupN++)
				{
					// #04{
					if (LMultShp)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																"Shape\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);

					// #05{
					// There is something wrong here
					//
					// Need to have something for an appearance node
					// for now lets just output the appearance node
					// all of the time.  The material portion was output
					// anyways.
					
					//if (LVRShape->Child == NULL)
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
												"appearance Appearance\n");
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
					M_IncIndentStr (LIndentStr, LCurrentIndent);


					// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
					// Check that this is correct
					// For instances should be using the current shapes
					// material.
					// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
					//
					if (DtMtlGetID (LShapeID, LGroupN, &LMatID))
					{
						DtMtlGetName (LShapeID, LGroupN, &LMatName);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"material USE ");
						sprintf (LMatNameF, "%s_%d", LMatName, LMatID);
						VR_FilterName (LMatNameF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LMatNameF);
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																		"\n");

						DtTextureGetCount (LShapeID, &LTexNumOnShape);
						if (VR_ExportTextures && (LTexNumOnShape > 0))
							//Are there textures on this shape ?
						{
							DtTextureGetName (LMatName, &LTxtName);

							if (LTxtName != NULL)
							{
								VR_FilterName (LTxtName);
								// #06{
									M_AddLine (LOutBuffer, LBufferPtr, 
										LIndentStr, "texture ImageTexture\n");
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"{\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);

								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"url  ");
								LOutBuffer[LBufferPtr++] = '"';


								int useFname;

								DtTextureGetFileName(LMatName,&LTexFName);
								if ( *LTexFName && VR_OriginalTextures )
									useFname = true;
								else
									useFname = false;
									
								if (VR_AddTexturePath)
								{
									LBufferPtr += VR_StrCpyL (LOutBuffer + 
														LBufferPtr, "file:");
									if ( !useFname )
									{
										LBufferPtr += VR_StrCpyL (LOutBuffer + 
												LBufferPtr, LTextureExportDir);
										LOutBuffer[LBufferPtr++] = '/';
									}
								}

								if ( useFname )
								{
									LBufferPtr += VR_StrCpyL (LOutBuffer +
                                                    LBufferPtr, LTexFName);
								}
								else
								{
									LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LSceneName);
									LOutBuffer[LBufferPtr++] = '_';
									LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, LTxtName);
									LBufferPtr += VR_StrCpyL (LOutBuffer + 
													LBufferPtr, ".rgb");
								}

								LOutBuffer[LBufferPtr++] = '"';
								LOutBuffer[LBufferPtr++] = '\n';

								// Lets check out if the texture is suppose
								// to wrap

								int LHorzWrap, LVertWrap;

								DtTextureGetWrap( LTxtName, &LHorzWrap,
															&LVertWrap );
								if ( LHorzWrap == DT_REPEAT )
									M_AddLine (LOutBuffer, LBufferPtr, 
												LIndentStr, "repeatS TRUE\n");
							 	else
									M_AddLine (LOutBuffer, LBufferPtr,
												LIndentStr, "repeatS FALSE\n");

								if ( LVertWrap == DT_REPEAT )
									M_AddLine (LOutBuffer, LBufferPtr, 
												LIndentStr, "repeatT TRUE\n");
								else
									M_AddLine (LOutBuffer, LBufferPtr,
												LIndentStr, "repeatT FALSE\n");


								// #06}
								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"}\n");

								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
									"textureTransform TextureTransform\n");
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"{\n");
								M_IncIndentStr (LIndentStr, LCurrentIndent);

								if ( VR_STextures ||
		                             !getenv( "MDT_USE_TEXTURE_TRANSFORM" ) )
									LRF2 = 0.0;
								else
								{
                                	DtTextureGetRotation (LTxtName, &LRF2);
									LRF2 /= 57.29577951;
								}
								sprintf (LTmpStr0, "rotation %f\n", LRF2);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);

								if ( VR_STextures )
								{
									LRF = 1.0;
									LGF = 1.0;
								} else {
								    DtTextureGetScale (LTxtName, &LRF, &LGF);
								}

								if ( LHorzWrap != DT_REPEAT )
									LRF = 1.0;
								if ( LVertWrap != DT_REPEAT )
									LGF = 1.0;

								sprintf (LTmpStr0, "scale %f %f\n", LRF, LGF);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);

							
								float	offsU, offsV;

                                DtTextureGetTranslation (LTxtName, &offsU,
																	&offsV);

								if ( VR_STextures || 
									!getenv( "MDT_USE_TEXTURE_TRANSFORM" ) )
								{
									LRF = 0.0;
									LGF = 0.0;
									LRF1 = 0.0;
									LGF1 = 0.0;
									offsU = 0.0;
									offsV = 0.0;
								} else {

									// for maya is the center of rotation at the
									// origin of the texture.
									// for VRML the center is the lower left 
									// corner.
#ifdef MAYA_SRC
								// This isn't quite correct for Maya and VRML

									LRF2 = -LRF2;
									LGF1 = (offsU*cos(LRF2) + offsV*sin(LRF2));
									LRF1 = (offsU*sin(LRF2) + offsV*cos(LRF2));

									offsU = 0.;
									offsV = 0.;
#else
									LRF2 *= -1.;
									LRF1 = -0.5*(cos(LRF2)+sin(LRF2)-1.0)*LRF;
									LGF1 = 0.5*(1.0-cos(LRF2+1.570796327) -
												sin(LRF2 + 1.570796327)) * LGF;
#endif
									sprintf (LTmpStr0, "center %f %f\n", 
														0.5, 0.5 );
                               		M_AddLine (LOutBuffer, LBufferPtr, 
													LIndentStr, LTmpStr0);
														
								}
								sprintf (LTmpStr0, "translation %f %f\n", 
                                					offsU+LRF1, offsV+LGF1);

								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	LTmpStr0);

								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"}\n");

								fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
								LBufferPtr = 0;
							}
						}
					}
					else
					{
						// #06{
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
										"material DEF _DefMat Material\n");
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "{\n");
						M_IncIndentStr (LIndentStr, LCurrentIndent);
						// #06}
							M_DecIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					}
					// #05} Close 'Appearance'
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
					
					int instanceS = -1;
					
					switch (LBDPtr->PrimitiveType)
					{
					case VRPRIM_NONE:
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
															"geometry USE ");
						instanceS = VRML2_ShapeIsInstanced[LVRShape->ID];
						if (instanceS < 0 )
							sprintf (LTmpStr0, "%s_%dGeo\n", LVRShape->Name, 
																	LGroupN);
						else 
						{
		                    for (LPIdxN = 0, LVRShape1 = NULL; 
                                    LPIdxN < VR_NumOfShapes; LPIdxN++)
        		                if (VR_Shapes[LPIdxN].ID == instanceS)
            		            {   
                		            LVRShape1 = VR_Shapes + LPIdxN;
                    		        break;
                        		}
							if ( LVRShape1 )
								sprintf (LTmpStr0, "%s_%dGeo\n", 
											LVRShape1->Name, LGroupN);
						}
						LBufferPtr += VR_StrCpyL (LOutBuffer + LBufferPtr, 
																	LTmpStr0);
						break;

					case VRPRIM_BOX:
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
														"geometry Box {}\n");
						break;

					case VRPRIM_CONE:
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
							"geometry Cone { bottomRadius 0.5 height 1.0 }\n");
						break;

					case VRPRIM_CYLINDER:
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
							"geometry Cylinder { radius 0.5 height 1.0 }\n");
						break;

					case VRPRIM_SPHERE:
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
													"geometry Sphere { }\n");
						break;
					}

#ifdef OLD_WAY
					//	Close 'Anchor', 'Collision' or 'Billboard' node 
					//	if HRCMODE is either WORLD or FLAT
						switch (VR_HierarchyMode)
						{
						case VRHRC_WORLD:
						case VRHRC_FLAT:
							_VRM_CloseVRML2Node ();
							break;
						}
#endif

					// #04}
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
				}

				// If there were more than 1 groups, 
				// but the DtShape has no children, close the Children group
				if ((LVRShape->Child == NULL) && (LNumOfGroups > 1))
				{
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
				}

                //  Close 'Anchor', 'Collision' or 'Billboard' node 
                //  if HRCMODE is either WORLD or FLAT
                switch (VR_HierarchyMode)
                {
                    case VRHRC_WORLD:
                    case VRHRC_FLAT:
                        _VRM_CloseVRML2Node ();
                        break;
                }      

				//Get next shape depending on the hierarchy mode
				//
				switch (VR_HierarchyMode)
				{
				case VRHRC_WORLD:
				case VRHRC_FLAT:
					LVRShape++;
					LShapeN++;
					if (LShapeN >= VR_NumOfShapes)
						LVRShape = NULL;
					// #03}			// Close transformation node
					M_DecIndentStr (LIndentStr, LCurrentIndent);
					M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
					fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
					LBufferPtr = 0;
					break;

				case VRHRC_FULL:
					if (LVRShape->Child != NULL)
						LVRShape = LVRShape->Child;
					else
					{
						_VRM_CloseVRML2Node ();

						//Close Transform node
						//
						M_DecIndentStr (LIndentStr, LCurrentIndent);
						M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");

						//Go on in the hierarchy
						//
						if (LVRShape->RightSibling != NULL)
							LVRShape = LVRShape->RightSibling;
						else
						{
							while ((LVRShape->RightSibling == NULL) && 
												(LVRShape->Parent != NULL))
							{
								LVRShape = LVRShape->Parent;
								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																	"]\n");

								_VRM_CloseVRML2Node ();


								//Close Transform node
								//
								M_DecIndentStr (LIndentStr, LCurrentIndent);
								M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
																		"}\n");
							}
							if (LVRShape != NULL)
								LVRShape = LVRShape->RightSibling;

							//Get next root shape
							//
							if (LVRShape == NULL)
							{
								if (LRootShapeCnt < VR_NumOfRootShapes)
								{
									LVRShape = VR_RootShapes[LRootShapeCnt];
									LRootShapeCnt++;
								}
								else
									LVRShape = NULL;
								// Hierarchy completed, 
								// no more root shapes, terminate
							}
						}
					}
					LShapeN++;
					if (LShapeN >= VR_NumOfShapes)
						LVRShape = NULL;


					/*
					 * if ( LVRShape->Child != NULL)
					 * LVRShape = LVRShape->Child; else {
					 * _VRM_CloseSpecNode();
					 * 
					 * // Close Transform node //
					 * M_DecIndentStr ( LIndentStr,
					 * LCurrentIndent );
					 * M_AddLine ( LOutBuffer,
					 * LBufferPtr, LIndentStr, "}\n" );
					 * 
					 * // Go on in the hierarchy // if (
					 * LVRShape->RightSibling != NULL)
					 * LVRShape = LVRShape->RightSibling;
					 * else { _VRM_CloseSpecNode(); //
					 * Close transform node //
					 * M_DecIndentStr(LIndentStr,LCurrentI
					 * ndent);
					 * M_AddLine(LOutBuffer,LBuffe
					 * rPtr,LIndentStr,"}\n");
					 * while((LVRShape->RightSibling==NULL
					 * )&&(LVRShape->Parent!=NULL)) {
					 * LVRShape=LVRShape->Parent;
					 * 
					 * // If there were more than 1 groups,
					 * but the DtShape has no children,
					 * close the Children group if (
					 * LVRShape->MultipleChld ) {
					 * M_DecIndentStr(LIndentStr,LCurrentI
					 * ndent);
					 * M_AddLine(LOutBuffer,LBuffe
					 * rPtr,LIndentStr,"]\n");
					 * LVRShape->MultipleChld = false; }
					 * 
					 * _VRM_CloseSpecNode();
					 * 
					 * // Close transform node //
					 * M_DecIndentStr ( LIndentStr,
					 * LCurrentIndent );
					 * M_AddLine ( LOutBuffer,
					 * LBufferPtr, LIndentStr, "}\n" ); }
					 * if ( LVRShape != NULL ) LVRShape =
					 * LVRShape->RightSibling; else { if
					 * ( LShapeN < ( VR_NumOfShapes - 1 )
					 * ) LVRShape++; } } } LShapeN++;if (
					 * LShapeN >= VR_NumOfShapes )
					 * LVRShape = NULL;
					 */
					break;
				}
			}

			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;
			fflush (LOutFile);

			if (LAnimated && VR_ConversionRun)
			{
				if (VR_ConversionRun &&
				    (LAnimated = VR_GetAnimation (&VR_AnimStart, &VR_AnimEnd,
				    	VR_Shapes, VR_NumOfShapes, VR_Materials,
						VR_NumOfMaterials, VR_Lights, VR_NumOfLights,
						VR_Cameras, VR_NumOfCameras, &LActMsgLineNum)))
				{
					VR_SaveAnimation (LOutFile, LOutBuffer, 
						&LBufferPtr, LIndentStr, &LCurrentIndent,
						VR_AnimStart, VR_AnimEnd, VR_Shapes, 
						VR_NumOfShapes, VR_Materials,
						  VR_NumOfMaterials, VR_Lights, VR_NumOfLights,
						  VR_Cameras, VR_NumOfCameras);
				}
			}
			else
				LAnimated = false;

			// #02]
			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "]\n");
			// #01}
			M_DecIndentStr (LIndentStr, LCurrentIndent);
			M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, "}\n");
			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);
			LBufferPtr = 0;

			if (LAnimated)
			{
				M_AddLine (LOutBuffer, LBufferPtr, LIndentStr, 
					"ROUTE TOUCH.touchTime TO TimeSource.set_startTime\n");
				VR_CreateROUTEs (LOutFile, LOutBuffer, &LBufferPtr, 
						LIndentStr, &LCurrentIndent,
						VR_Shapes, VR_NumOfShapes, 
						VR_Materials, VR_NumOfMaterials,
						 VR_Lights, VR_NumOfLights,
						 VR_Cameras, VR_NumOfCameras);
			}

			fwrite (LOutBuffer, 1, LBufferPtr, LOutFile);

			fclose (LOutFile);

			// Lets now see if we are supposed to compress the file
			
			if ( VR_Compressed )
			{
				char cmdStr[2048];
				
				sprintf( cmdStr, "gzip -f %s", LFileNameRet );
				system( cmdStr );

				// There a couple of differences for SGI/NT that we
				// can take care of here.
				// Also want to check to see if the gzip program actually
				// produced a .gz file.  If not then lets not move it over
				// top of the original file.

				struct stat sbuf;
				sprintf( cmdStr, "%s.gz", LFileNameRet );
				if ( STAT( cmdStr, &sbuf ) == 0 )
				{
#ifdef WIN32
					sprintf( cmdStr, "move \"%s.gz\" \"%s\"", LFileNameRet, LFileNameRet);
#else
					sprintf( cmdStr, "mv %s.gz %s", LFileNameRet, LFileNameRet);
#endif
					system( cmdStr );
				}
					
			}


			// Time to finish off

			if ( VR_ConversionRun )
			{
				VR_PrintMessage ("Conversion completed");

				VDt_CleanUp ();

				VR_FreeScene ();

				return (true);
			}
		}
		else
			VR_PrintMessage ("Error: Could not open output file. Dir: %s\n",
					 LExportDir);
	}

	VDt_CleanUp ();
	
	VR_FreeScene ();

	return (false);
}

#ifdef WIN32
#pragma warning(default: 4244 4305)
#endif // WIN32
