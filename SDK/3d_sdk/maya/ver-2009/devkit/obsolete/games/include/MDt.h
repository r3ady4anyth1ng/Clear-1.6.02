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

//
// Original Author:    XCW
//

#ifndef __MDt_h__
#define __MDt_h__

#if defined (_WIN32)
#	include <windows.h>
#elif defined (OSMac_)
#	include <Carbon/Carbon.h>
#else
#	include <Xm/Xm.h>
#endif

#include <maya/MObject.h>
#include <maya/MIntArray.h>


#ifdef __cplusplus
extern "C"
{
#endif

// End of face/line set delimiter:
//
#define DtEND_OF_FACE -1

// File types:
//
#define DtInvalid     -1
#define DtStatic       0
#define DtAnimation    1
#define DtKinematic    2

#define DT_INVALID      DtInvalid
#define DT_STATIC       DtStatic
#define DT_ANIMATION    DtAnimation
#define DT_KINEMATIC    DtKinematic


// Table structure for DSO labels and entry functions:
//
typedef struct 
{
 char*	label;
 int	(*DtEntryFunc)(void);
 int	(*DtCmdFunc)(int argc,char** argv);
 int    (*DtExitFunc)(void);
 char*  defaultExtension;
 char*  magicString;
} DtEntryTable;

// General data types: 
//
typedef struct { float	vec[3]; } DtVec3f;
typedef struct { float	vec[2]; } DtVec2f;
typedef struct { unsigned char r,g,b,a; } DtRGBA;
typedef struct { float  r,g,b,a; } DtFltRGBA;

// Data types for DtPrintf
typedef enum {
      dStdout,
      dStderr,
      dPrompt,
      dErrlog,
      dPromptNoHistory
} DtOutputType;

// Data types for Character Network support:
//
typedef int DtStateTableEntry;

typedef struct
{
	int	storage;
	int	length;
	int	nextNode;
	int	entryFrame;
	char*	data1;
	char*	data2;
	char*	data3;
	char*	data4;
} DtTransition;

typedef struct
{
	int	flags;
	int	storage;
	int	length;
	char*	data1;
	char*	data2;
	char*	data3;
	char*	data4;
} DtSnippet;

typedef struct
{
	int			nodes;
	int			arcs;
	DtStateTableEntry*	adjacency;
	DtTransition*		arcData;
	DtSnippet*		nodeData;
	char*			data1;
	char*			data2;
	char*			data3;
	char*			data4;
} DtStateTable;

#define DT_ERROR                        (-1)
#define DT_CANCEL                       0
#define DT_SUCCESS                      1

// Bit mask values to use with DtIsValid(): 
//
#define DT_ENV_AMBIENT_INTENSITY	0x01000001
#define DT_ENV_AMBIENT_CLR		0x01000002
#define DT_ENV_FOG_TYPE			0x01000004
#define DT_ENV_FOG_CLR			0x01000008
#define DT_ENV_FOG_VISIBILITY		0x01000010

#define DT_CAMERA			0x02000001	/* DtCameraIsValid()	all camera functions		*/
#define DT_CAMERA_POSITION		0x02000002	/* DtCameraIsValid()	DtCameraGetPosition()		*/
#define DT_CAMERA_ORIENTATION		0x02000004	/* DtCameraIsValid()	DtCameraGetOrientation()	*/
#define DT_CAMERA_MATRIX		0x02000006	/* DtCameraIsValid()	DtCameraGetMatrix()		*/
#define DT_CAMERA_ASPECT		0x02000010	/* DtCameraIsValid()	DtCameraGetAspect()		*/
#define DT_CAMERA_NEAR_CLIP		0x02000020	/* DtCameraIsValid()	DtCameraGetNearClip()		*/
#define DT_CAMERA_FAR_CLIP		0x02000040	/* DtCameraIsValid()	DtCameraGetFarClip()		*/
#define DT_CAMERA_FOCAL_DISTANCE	0x02000080	/* DtCameraIsValid()	DtCameraGetFocalDistance()	*/
#define DT_CAMERA_HEIGHT_ANGLE		0x02000100	/* DtCameraIsValid()	DtCameraGetHeightAngle()	*/
#define DT_CAMERA_HEIGHT		0x02000200	/* DtCameraIsValid()	DtCameraGetHeight()		*/

#define DT_LIGHT			0x04000001	/* DtLightIsValid()     all light functions		*/
#define DT_LIGHT_ON			0x04000002	/* DtLightIsValid()	DtLightGetOn()			*/
#define DT_LIGHT_INTENSITY		0x04000004	/* DtLightIsValid()	DtLightGetIntensity()		*/
#define DT_LIGHT_COLOR			0x04000008	/* DtLightIsValid()	DtLightGetColor()		*/
#define DT_LIGHT_POSITION		0x04000010	/* DtLightIsValid()	DtLightGetPosition()		*/
#define DT_LIGHT_DIRECTION		0x04000020	/* DtLightIsValid()	DtLightGetDirection()		*/
#define DT_LIGHT_DROP_OFF_RATE		0x04000040	/* DtLightIsValid()	DtLightGetDropOffRate()		*/
#define DT_LIGHT_CUT_OFF_ANGLE		0x04000080	/* DtLightIsValid()	DtLightGetCutOffAngle()		*/

#define DT_SHAPE			0x08000001	/* DtShapeIsValid()	all shape functions		*/
#define DT_SHAPE_MATRIX			0x08000002	/* DtShapeIsValid()	DtShapeGetMatrix()		*/
#define DT_SHAPE_VISIBLE		0x08000004
#define DT_SHAPE_VERTICIES		0x08000008	/* DtShapeIsValid()	DtShapeGetVertices()		*/
#define DT_SHAPE_NORMALS		0x08000010	/* DtShapeIsValid()	DtShapeGetNormals()		*/
#define DT_SHAPE_TEXTURE_VERTICIES	0x08000020	/* DtShapeIsValid()	DtShapeGetTextureVertices()	*/

#define DT_FACE				0x08000040	/* DtGroupIsValid()	the next three face functions	*/
#define DT_FACE_VERTEX_INDEX		0x08000080	/* DtGroupIsValid()	DtFaceGetIndex() 		*/
#define DT_FACE_NORMAL_INDEX		0x08000100	/* DtGroupIsValid()	DtFaceGetNormalIndex()		*/
#define DT_FACE_TEXTURE_INDEX		0x08000200	/* DtGroupIsValid()	DtFaceGetTextureIndex()		*/

#define DT_LINE				0x08000400	/* DtGroupIsValid()	the next three line functions	*/
#define DT_LINE_VERTEX_INDEX		0x08000800	/* DtGroupIsValid()	DtLineGetIndex()		*/
#define DT_LINE_NORMAL_INDEX		0x08001000	/* DtGroupIsValid()	DtLineGetNormalIndex()		*/
#define DT_LINE_TEXTURE_INDEX		0x08002000	/* DtGroupIsValid()	DtLineGetTextureIndex()		*/

#define DT_POINT			0x08004000	/* DtGroupIsValid()	the next three point functions	*/
#define DT_POINT_VERTEX_INDEX		0x08008000	/* DtGroupIsValid()	DtPointGetIndex()		*/
#define DT_POINT_NORMAL_INDEX		0x08010000	/* DtGroupIsValid()	DtPointGetNormalIndex() 	*/
#define DT_POINT_TEXTURE_INDEX		0x08020000	/* DtGroupIsValid()	DtPointGetTextureIndex()	*/

#define DT_GROUP_MATRIX		        0x08040000	/* DtGroupIsValid()	DtGroupGetMatrix()		*/

#define DT_MATERIAL			0x10000001
#define DT_MTL_AMBIENT_CLR		0x10000002	/* DtMtlIsValid()	DtMtlGetAmbientClr()		*/
#define DT_MTL_DIFFUSE_CLR		0x10000004	/* DtMtlIsValid()	DtMtlGetDiffuseClr()		*/
#define DT_MTL_SPECULAR_CLR		0x10000008	/* DtMtlIsValid()	DtMtlGetSpecularClr()		*/
#define DT_MTL_EMISSIVE_CLR		0x10000010	/* DtMtlIsValid()	DtMtlGetEmissiveClr()		*/
#define DT_MTL_SHININESS		0x10000020	/* DtMtlIsValid()	DtMtlGetShininess()		*/
#define DT_MTL_TRANSPARENCY		0x10000040

#define DT_TEXTURE			0x10000080	/* DtTextureIsValid()	all texture functions		*/
#define DT_TEXTURE_BLEND_CLR		0x10000100	/* DtTextureIsValid()	DtTextureGetBlendClr()		*/
#define DT_TEXTURE_TRANSLATION		0x10000200	/* DtTextureIsValid()	DtTextureGetTranslation()	*/
#define DT_TEXTURE_ROTATION		0x10000400	/* DtTextureIsValid()	DtTextureGetRotation()		*/
#define DT_TEXTURE_SCALE		0x10000800	/* DtTextureIsValid()	DtTextureGetScale()		*/
#define DT_TEXTURE_CENTER		0x10001000	/* DtTextureIsValid()	DtTextureGetCenter()		*/


// Camera types, returned by DtCameraGetType(): 
//
#define DT_PERSPECTIVE_CAMERA	1
#define DT_ORTHOGRAPHIC_CAMERA	2

// Light types, returned by DtLightGetType():
//
#define DT_POINT_LIGHT		1
#define DT_DIRECTIONAL_LIGHT	2
#define DT_SPOT_LIGHT		3
#define DT_AMBIENT_LIGHT	4

// Texture defines:
//
#define DT_REPEAT	1
#define DT_CLAMP	2
#define DT_MODULATE	1
#define DT_DECAL	2
#define DT_BLEND	3


// GENERAL DT FUNCTIONS: 
//

int	DtOpenModel				(void);
int	DtGetFilename			(char** basename);
int	DtGetDirectory			(char** directory);

void DtPrintf 				(DtOutputType, const char*, ...);

#if defined (_WIN32)
int	DtGetParent				(int *widget);
#elif defined (OSMac_)
int DtGetParent				(ControlRef* widget);
#else
int	DtGetParent				(Widget* widget);
#endif

int 	DtSetFilename       	(char *name);
int 	DtSetDirectory      	(char *name);

void    DtSetUpdateShaders(int);
int		DtGetUpdateShaders();

int	DtMatrixGetTranslation	(float* matrix,float* xTrans,float* yTrans,float* zTrans);
int	DtMatrixGetQuaternion	(float* matrix,float* scalar,float* xAxis,float* yAxis,float* zAxis);
int	DtMatrixGetRotation		(float* matrix,float* xRot,float* yRot,float* zRot);
int	DtMatrixGetScale		(float* matrix,float* xScale,float* yScale,float* zScale);
int DtMatrixGetTransforms(float *matrix, float *translate, 
                          float *scale, float *quaternion, float *rotation);



#if 0
void	DtImageRead 	(char* name,int* width,int* height,
										int* components,unsigned char** image);
int	DtImageWrite 	(char* name,char* key,int width,int height,
										int components,unsigned char* image,
										IMF_CAP_SETTING** settings,IMF_LUT* lut);

#endif

// SCENE FUNCTIONS:
//

char *DtAPIVersion();

void	DtSceneGetName		(char** name);
int 	DtSceneGetType		(void);

int	DtFrameSet			(int frame);
int	DtFrameGet			(void);
int	DtFrameGetCount		(void);
void	DtFrameSetStart		(int start);
int	DtFrameGetStart  	(void);
void	DtFrameSetEnd		(int end);
int	DtFrameGetEnd   	(void);
void	DtFrameSetBy		(int by);
int	DtFrameGetBy		(void);
int	DtFrameGetRange		(int* start,int* end,int* by);
void	DtFrameGetKeyFrames	(int* count,int** frames);

void	DtEnvGetAttenuation	(float* a2,float* a1,float* a0);
int	DtEnvGetFogType		(int *type);
void	DtEnvGetFogColor	(float* red,float* green,float* blue);
int	DtEnvGetFogVisibility	(float *vis);

// CAMERA FUNCTIONS: 
//

int     DtCameraGetCount       (int* count);
int     DtCameraGetType        (int cameraID,int* type);
int		DtCameraGetName		   (int cameraID,char** name);
int     DtCameraGetPosition    (int cameraID, float* xTran,
											  float* yTran,float* zTran);
int		DtCameraGetInterest    (int cameraID, float* xTran,
											  float* yTran,float* zTran);
	
int     DtCameraGetUpPosition  (int cameraID, float* xTran, 
											  float* yTran,float* zTran); 
int     DtCameraGetOrientation (int cameraID, float* xRot,
											  float* yRot,float* zRot);
int     DtCameraGetOrientationQuaternion(int cameraID,
														float* xRot,
														float* yRot,
														float* zRot,
														float* wRot);
int     DtCameraGetMatrix       (int cameraID,float** matrix);
int     DtCameraGetAspect       (int cameraID,float* aspect);
int     DtCameraGetNearClip     (int cameraID,float* Lnear);
int     DtCameraGetFarClip      (int cameraID,float* Lfar);
int     DtCameraGetFocalDistance   (int cameraID,float* focal);
int     DtCameraGetHeightAngle  (int cameraID,float* angle);
int     DtCameraGetHeight       (int cameraID,float* height);

int     DtCameraIsValid         (int cameraID,int valid_bit);

// LIGHT FUNCTIONS:
//

int    DtEnvGetAmbientIntensity  (float *intensity);
void   DtEnvGetAmbientColor      (float* red, float* green,
																float* blue);
int     DtLightGetCount      (int* count);
int     DtLightGetType       (int lightID,int* type);
int     DtLightGetOn         (int lightID,int* on);
int     DtLightGetIntensity  (int lightID,float* intensity);
int     DtLightGetColor      (int lightID,
												float* red,float* green,
												float* blue);
int     DtLightGetPosition   (int lightID,
												float* xTrans,float* yTrans,
												float* zTrans);
int     DtLightGetDirection  (int lightID,
												float* xRot,float* yRot,
												float* zRot);
int     DtLightGetDropOffRate  (int lightID,float* rate);
int     DtLightGetCutOffAngle  (int lightID,float* angle);
int     DtLightIsValid         (int lightID,int valid_bit);


// SHAPE FUNCTIONS: 
//

int    DtShapeGetCount                 (void);

void   DtShapeUpdate		   			(void);
int	DtShapeGetName					(int shapeID,char** name);
int    DtShapeGetParentID				(int shapeID );
void   DtShapeGetParentName			(int shapeID,char** name);
int    DtShapeGetMatrix                (int shapeID,float** matrix);



int     DtShapeGetRotationPivot     (int shapeID,float *x, float *y, float *z);
int     DtShapeGetScalePivot        (int shapeID,float *x, float *y, float *z);
int     DtShapeGetTranslation       (int shapeID,float *x, float *y, float *z);
int     DtShapeGetRotation          (int shapeID,float *x, float *y, float *z);
int     DtShapeGetScale             (int shapeID,float *x, float *y, float *z);
int		DtShapeGetSetName			(int shapeID,char ** name);




int    DtShapeGetVertexCount           (int shapeID,int* count);
int    DtShapeGetVertex                (int shapeID,int index,DtVec3f* vertex);
int    DtShapeGetVertices              (int shapeID,int* count,DtVec3f** vertices);
int    DtShapeGetVerticesAnimated	   (int shapeID, int *count, int **vertices);
int    DtShapeGetTextureVertexCount    (int shapeID,int* count);
int    DtShapeGetTextureVertex  		(int shapeID,int index,DtVec2f* vertex);
int    DtShapeGetTextureVertices		(int shapeID,int* count,DtVec2f** vertices);
int    DtShapeGetNormalCount           (int shapeID,int* count);
int    DtShapeGetNormal                (int shapeID,int index,DtVec3f* normal);
int    DtShapeGetNormals               (int shapeID,int* count,DtVec3f** normals);
int    DtShapeGetPolygonNormalCount (int shapeID,int* count);
int    DtShapeGetPolygonNormal      (int shapeID,int index,DtVec3f* normal);
int    DtShapeGetPolygonNormals     (int shapeID,int* count,DtVec3f** normals);
int    DtShapeGetPolygonNormalIdx   (int shapeID,int groupID,int *count,int **indices);


int    DtShapeGetVerticesColor         (int shapeID,int* count,DtRGBA** colors);
int    DtShapeGetVertexColor           (int shapeID, int index, DtRGBA *color);


int	   DtShapeGetVerticesFaceColors	(int shapeID,int* count,DtFltRGBA** colors);

int	   DtShapeGetChildren				(int shapeID,int* count,int** children);

int    DtShapeIsDoubleSided 	       	(int shapeID);
int	   DtShapeIsOpposite				(int shapeID);
int    DtShapeIsFlatShaded     			(int shapeID);

int     DtGroupGetCount					(int shapeID);

int     DtGroupGetParentID				(int shapeID,int groupID);
void    DtGroupGetName					(int shapeID,int groupID,char** name);
void    DtGroupGetParentName			(int shapeID,int groupID,char** name);
int     DtGroupGetMatrix                (int shapeID,int groupID,float** matrix);
int     DtGroupGetVertexCount           (int shapeID,int groupID,int* count);
int     DtGroupGetVertex                (int shapeID,int groupID,int index,DtVec3f* vertex);
int     DtGroupGetVertices              (int shapeID,int groupID,int* count,DtVec3f** vertices);
int     DtGroupGetTextureVertexCount    (int shapeID,int groupID,int* count);
int     DtGroupGetTextureVertex  		(int shapeID,int groupID,int index,DtVec2f* vertex);
int     DtGroupGetTextureVertices		(int shapeID,int groupID,int* count,DtVec2f** vertices);
int     DtGroupGetNormalCount           (int shapeID,int groupID,int* count);
int     DtGroupGetNormal                (int shapeID,int groupID,int index,DtVec3f* normal);
int     DtGroupGetNormals               (int shapeID,int groupID,int* count,DtVec3f** normals);

int     DtPolygonGetCount               (int shapeID,int groupID,int* count);
int     DtPolygonGetIndices             (int index,int* vCount,int** vIdx,int** nIdx,int** tIdx);

int     DtFaceGetIndexByGroup           (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetNormalIndexByGroup     (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetTextureIndexByGroup    (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetIndexByShape           (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetNormalIndexByShape     (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetColorIndexByShape     (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetTextureIndexByShape    (int shapeID,int groupID,int* count,int** indices);
int     DtFaceGetMaterialIndex (int shapeID,int groupID,
												int *count,int **indices);


int	DtLineGetIndex			(int shapeID,int groupID,
												int* count,int** indices);
int	DtLineGetNormalIndex	(int shapeID,int groupID,
												int* count,int** indices);
int	DtLineGetTextureIndex	(int shapeID,int groupID,
												int* count,int** indices);
int	DtPointGetIndex			(int shapeID,int groupID,
												int* count,int** indices);
int	DtPointGetNormalIndex	(int shapeID,int groupID,
												int* count,int** indices);
int	DtPointGetTextureIndex	(int shapeID,int groupID,
												int* count,int** indices);

int	DtNurbsGetNumCtrlPoints	(int shapeID,int groupID,
												int* uNum,int* vNum);
int	DtNurbsGetIndex			(int shapeID,int groupID,
												int* count,int** indices);
int	DtNurbsGetUKnotVector	(int shapeID,int groupID,
												int* num,float** uKnotVector);
int	DtNurbsGetVKnotVector	(int shapeID,int groupID,
												int* num,float** vKnotVector);

int	DtNurbsGetTrimVertices	(int shapeID,int groupID,
												int* count,DtVec2f** vertices);
int	DtNurbsTrimGetCount	(int shapeID,int groupID,int* count);
int	DtNurbsTrimGetIndex		(int shapeID,int groupID,
									int trimID,int* count,int** indices);
int	DtNurbsTrimGetKnotVector	(int shapeID,int groupID,
									int trimID,int* num,float** knotVector);

int     DtShapeIsValid		(int shapeID,int valid_bit);
int     DtGroupIsValid		(int shapeID,int groupID,int valid_bit);

// MATERIAL FUNCTIONS:
//


int  DtPolygonMtlGetName (int shapeID,int groupID,int index,
																char** name);
int 	DtMtlGetSceneCount	(int* count);		

int 	DtMtlGetName		(int shapeID,int groupID,char** name);

int 	DtMtlGetNameByID	(int mtl_id,char** name);		
int 	DtMtlGetID			(int shapeID,int groupID,int* mtl_id);
int 	DtMtlGetAllClrbyID	(int mtl_id,int index,
							float* ared,float* agreen,float* ablue,
							float* dred,float* dgreen,float* dblue,
							float* sred,float* sgreen,float* sblue,
							float* ered,float* egreen,float* eblue,
							float* shininess,float* transparency);
int 	DtMtlGetAmbientClr	(char* name,int index,
										float* red,float* green,float* blue);
int 	DtMtlGetDiffuseClr	(char* name,int index,
										float* red,float* green,float* blue);
int 	DtMtlGetSpecularClr	(char* name,int index,
										float* red,float* green,float* blue);
int 	DtMtlGetEmissiveClr	(char* name,int index,
										float* red,float* green,float* blue);
int 	DtMtlGetShininess	(char* name,int index,float* shininess);
int 	DtMtlGetTransparency	(char* name,int index,float* transparency);
int 	DtMtlGetLightSource	(char* name,int index,int* lightsource);
int 	DtMtlIsRflMap		(char* name);
int		DtMtlHasAlpha		(char* name);
int		DtMtlHasAlphaById	(int mtl_id);
int 	DtTextureGetSceneCount	(int* count);
int 	DtTextureGetCount	(int shapeID,int* count);
int 	DtTextureGetName	(char* mtl_name,char** name);
int		DtTextureGetFileName(char *mtl_name, char **name);
int 	DtTextureGetID		(int mtl_id,int* textureID);
int		DtTextureGetNameID(int textureID,char** name);		
int 	DtTextureGetFileNameID	(int textureID,char** name);

int     DtTextureGetNameMulti (char* mtl_name, char *texType, char** name);
int     DtTextureGetIDMulti   (int mtl_id, char *texType, int* textureID);

int 	DtTextureGetWrap	(char* name,int* horizontal,int* vertical);
int 	DtTextureGetMode	(char* name,int* mode);
int 	DtTextureGetBlendClr	(char* name,float* red,float* green,float* blue);
int 	DtTextureGetImageSize	(char* name,int* width,int* height,int* components);
int 	DtTextureGetImageSizeByID	(int LID,int* width,int* height,int* components);
int 	DtTextureGetImage		(char* name,unsigned char** image);
int 	DtTextureGetImageByID	(int LID,unsigned char** LImage);		
int 	DtTextureGetTranslation	(char* name,float* s,float* t);
int 	DtTextureGetRotation	(char* name,float* angle);
int 	DtTextureGetScale		(char* name,float* s,float* t);
int 	DtTextureGetCenter		(char* name,float* x,float* y);

int 	DtMtlIsValid        	(char* name,int valid_bit);
int 	DtTextureIsValid    	(char* name,int valid_bit);

int 	DtFaceCount						(int indexCnt,int* indices);
int 	DtIndexCount					(int* indices);

// CHARACTER NETWORK STATE TABLE FUNCTIONS:
//

int 	DtCnetCharacterCount			(int* count);
int DtCnetCharacterGetShapes    (int cnetId, int *count, int ** shapelist);
int DtCnetCharacterGetFrameRange(int cnetId, int *start, int *end);

int 	DtCnetCharacterName				(int cnetId,char** name);
int	DtCnetGetStateTable				(int cnetId,DtStateTable** table);

// KINEMATION FUNCTIONS:
//
int	DtKinGetBodyCount	(int* count);
int	DtKinGetBodyName	(int bodyID,char** name);
int	DtKinGetSegmentCount	(int bodyID,int* count);
int	DtKinGetSegmentName	(int bodyID,int segID,char** name);
int	DtKinGetSegmentXfm	(int bodyID,int segID,int frame,float** xfm);
int	DtKinGetGroupCount	(int bodyID,int segID,int* count);
int	DtKinGetGroupName	(int bodyID,int segID,int groupID,char** name);

void    dtPolygonGetFaceList	(int shapeID,int groupID,int** list,int* count);


// SCENE INPUT SOURCE DEFINES (formally from Scene.h):

#define DN_ALL			-3
#define DN_NO_TEXTURE		-1

#define DN_CAMERA_EXT		"cam"		/* scene entity types */
#define DN_LIGHT_EXT		"lgt"
#define DN_OBJECT_EXT		"obj"
#define DN_REFLECT_EXT		"rfm"

#define DN_SWITCH		"Switch"	/* scene graph specific nodes */
#define DN_COMPLEX		"Complex"
#define DN_DRAWSTYLE		"DrawStyle"	
#define DN_MTL_BIND		"MtlBind"

#define DN_MATERIAL		"Material"
#define DN_TOPOLOGY		"Topology"
#define DN_FACES		"Faces"
#define DN_LINES		"Lines"
#define DN_POINTS		"Points"
#define DN_TRIM			"Trim"
#define DN_NURBS		"Nurbs"


#ifdef __cplusplus
};
#endif

//
//	Now for some cplusplus enabled functions
//

int DtShapeGetTRSAnimKeys( int shapeID, MIntArray *keyFrames );
int DtShapeGetVtxAnimKeys( int shapeID, MIntArray *keyFrames );

int DtLightGetTRSAnimKeys( int lightID, MIntArray *keyFrames );
int DtLightGetAnimKeys( int lightID, MIntArray *keyFrames );

int DtCameraGetTRSAnimKeys( int cameraID, MIntArray *keyFrames );
int DtCameraGetAnimKeys( int cameraID, MIntArray *keyFrames );

int DtMtlGetAnimKeys( int mtlID, MIntArray *keyFrames );


#endif	/* __MDt_h__ */
