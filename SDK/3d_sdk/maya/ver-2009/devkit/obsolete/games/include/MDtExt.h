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
// Original Author:    XCW
//

#ifndef __DtExt_h__
#define __DtExt_h__

#include <maya/MObject.h>
#include <maya/MDagPath.h>

#ifdef __cplusplus
extern "C"
{
#endif


// Defined in DtLayer.c++:
//

int    DtExt_tesselate(); 
void   DtExt_setTesselate( int mode );
int    DtExt_materialInventory();
int    DtExt_outputTransforms();
void	DtExt_setOutputTransforms( int mode );
int    DtExt_xTextureRes();
int    DtExt_yTextureRes();
int     DtExt_MaxXTextureRes();
int     DtExt_MaxYTextureRes();
int		DtExt_outputCameras();
int		DtExt_Parents();
int		DtExt_Winding();
void	DtExt_setInlineTextures( int status );
int		DtExt_inlineTextures();
void	DtExt_setXTextureRes( int res );
void	DtExt_setYTextureRes( int res );
void    DtExt_setMaxXTextureRes( int res );
void    DtExt_setMaxYTextureRes( int res );
void	DtExt_setOutputCameras( int status );
int		DtExt_softTextures();
void	DtExt_setSoftTextures( int mode );
void	DtExt_setParents( int status );
void	DtExt_setWinding( int mode );
void	DtExt_addTextureSearchPath( char *path );
char *	DtExt_getTextureSearchPath( void );


void    DtExt_setRescaleRange( int mode );
int     DtExt_RescaleRange();
int		DtExt_VertexAnimation();
void	DtExt_setVertexAnimation( int mode );
int		DtExt_JointHierarchy();
void	DtExt_setJointHierarchy( int mode );
int		DtExt_MultiTexture();
void	DtExt_setMultiTexture( int mode );
int		DtExt_OriginalTexture();
void	DtExt_setOriginalTexture( int mode );


#if defined (_WIN32)
void	DtExt_SetParentWidget( int parent );
#elif defined (OSMac_)
void	DtExt_SetParentWidget( ControlRef parent );
#else
void	DtExt_SetParentWidget( Widget parent );
#endif

// Defined in MDtShape.cc:
//
int	DtExt_SetupWorldVertices( void );


// Walking of the Dag Tree methods:
//
#define	ALL_Nodes		 0
#define PICKED_Nodes     1 // corresponds to the ACTIVE_Nodes in PA DT
#define ACTIVE_Nodes	 2 // corresponds to the UNDER_Nodes in PA DT


#define DEBUG_GEOMAT	1
#define DEBUG_CAMERA	2
#define DEBUG_LIGHT		4

// Defined in MDtLayer.cc:
//
int		DtExt_WalkMode( void );
void	DtExt_setWalkMode( int mode );

// Shape extensions 

// MayaDT can not support these functions with the same arguments!
//

// Implemented now as
int DtExt_ShapeGetTransform(int shapeID, MObject &obj );
int DtExt_ShapeGetShapeNode(int shapeID, MObject &obj );
int DtExt_ShapeGetDagPath(int shapeID, MDagPath &dagPath);

//
// int  DtExt_ShapeGetAlObject(int shapeID, AlObject **obj);
// int  DtExt_ShapeGetAlObject(int shapeID, void **obj);

// Implemented now as 
int DtExt_ShapeGetShader( int shapeID, int groupID, MObject &obj );

int DtExt_MtlGetShader( int mtlID, MObject &obj );
//
// int  DtExt_ShapeGetAlShader(int shapeID, int groupID, AlShader **obj);
// int  DtExt_ShapeGetAlShader(int shapeID, int groupID, void **obj);

// Implemented now as
int  DtExt_ShapeGetOriginal(int shapeID, int groupID, MObject &obj);

//
// int  DtExt_ShapeGetAlOriginal(int shapeID, int groupID, AlObject **obj);
// int  DtExt_ShapeGetAlOriginal(int shapeID, int groupID, void **obj);

// Light extensions
int DtExt_LightGetTransform(int lightID, MObject &obj );
int DtExt_LightGetShapeNode(int lightID, MObject &obj );

// Camera extensions
int DtExt_CameraGetTransform(int cameraID, MObject &obj );
int DtExt_CameraGetShapeNode(int cameraID, MObject &obj );

// Defined in doShape.cc:
//
int	DtExt_ShapeIsAnim( int shapeID );

int	DtExt_ShapeGetTexCnt( int shapeID, int *cnt );
int	DtExt_ShapeIncTexCnt( int shapeID );

int	DtExt_ShapeIsInstanced( int shapeID );

// Defined in DtLayer.c++:
//
void   DtExt_setDebug( int );

int    DtExt_Debug();

void   DtExt_Msg( char *, ... );

void   DtExt_Err( char *, ... );

void   DtExt_SceneInit( char * );
void	DtExt_dbInit( void );
void	DtExt_CleanUp( void );

// const char* DtExt_nodeName( AlObject * );
// void DtExt_firstDagNode( AlObject** );

#ifdef __cplusplus
};
#endif

// General definitions for MDtExt functions:
//
#define kTESSNONE 0
#define kTESSTRI  3
#define kTESSQUAD 4

#define kTRANSFORMNONE    0
#define kTRANSFORMMINIMAL 1
#define kTRANSFORMALL     2

#define	 AP_PGM_NAME     "GameExport"
#define	 AP_PGM_VERSION    "2.1"

#endif




