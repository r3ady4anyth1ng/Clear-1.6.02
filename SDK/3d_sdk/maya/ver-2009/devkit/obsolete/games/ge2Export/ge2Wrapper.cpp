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
// ge2 translator for Maya    
// 
//

#ifdef WIN32
#include <process.h>
#endif

#ifdef WIN32
#define MAXPATHLEN 1024
#endif

#include "ge2Wrapper.h"
#include <maya/MIOStream.h>

extern const char * objectName( MObject object );

MString ge2Wrapper::trueString( "t" );
MString ge2Wrapper::falseString( "f" );

char	ge2Wrapper::gofTemplate[] = "gof-2-02.tpl";
char	ge2Wrapper::gmfTemplate[] = "gmf-2-02.tpl";
char	ge2Wrapper::gafTemplate[] = "gaf-2-02.tpl";
char	ge2Wrapper::gsfTemplate[] = "gsf-2-02.tpl";

// Default light values
float	ge2Wrapper::defaultAmbientLightBrightness =				0.5;
float	ge2Wrapper::defaultAmbientLightColor[] =				{ 1.0, 1.0, 1.0 };
float	ge2Wrapper::defaultInfiniteLightBrightness =			0.5;
float	ge2Wrapper::defaultInfiniteLightColor[] =				{ 1.0, 1.0, 1.0 };
float	ge2Wrapper::defaultInfiniteLightDirection[] =			{ 1.0, 1.0, 1.0 };
float	ge2Wrapper::defaultPointLightBrightness =				0.5;
float	ge2Wrapper::defaultPointLightColor[] =					{ 1.0, 1.0, 1.0 };
float	ge2Wrapper::defaultPointLightPosition[] =				{ 0.0, 10.0, 0.0 };
float	ge2Wrapper::defaultPointLightRadius =					10.0;
float	ge2Wrapper::defaultSpotLightBrightness =				0.5;
float	ge2Wrapper::defaultSpotLightColor[] =					{ 1.0, 1.0, 1.0 };
float	ge2Wrapper::defaultSpotLightPosition[] =				{ 0.0, 10.0, 0.0 };
float	ge2Wrapper::defaultSpotLightAimPoint[] =				{ 0.0, 0.0, 0.0 };
float	ge2Wrapper::defaultSpotLightRadius =					10.0;
float	ge2Wrapper::defaultSpotLightFalloffAngle =				45.0;
float	ge2Wrapper::defaultSpotLightAspectRatio[] =				{ (float) 1.0, (float) 1.33 };
// Default camera values
float	ge2Wrapper::defaultCameraViewAngle =					45.0;
float	ge2Wrapper::defaultCameraHitherDistance =				(float) 0.1;
float	ge2Wrapper::defaultCameraYonDistance =					9999.0;
float	ge2Wrapper::defaultCameraLocation[] =					{ 10.0, 10.0, 10.0 };
float	ge2Wrapper::defaultCameraAimPoint[] =					{ 0.0, 0.0, 0.0 };
float	ge2Wrapper::defaultCameraLookupVector[] =				{ 0.0, 1.0, 0.0 };

// geWrapper class methods:
ge2Wrapper::ge2Wrapper()
:	pluginVersion		("unknown"),
	geVersion			("2.02"),
	shapeInfo			(NULL),
	numShapes			(0),
	shaders				(NULL),
	numShaders			(0),
	enableAnim			(true),
	animVertices		(true),
	vertexDisplacement	(kVDInvalid),
	animTransforms		(true),
	animShaders			(false),
	animLights			(false),
	animCamera			(false),
	keyCurves			(false),
	keySample			(false),
	sampleRate			(1),
	sampleTolerance		(0.0),
	useDomainGL			(false),
	useDomainPSX		(false),
	useDomainN64		(false),
	useDomainCustom		(false),
	useOriginalFileTextures (false),
	verboseGeom			(false),
	verboseLgt			(false),
	verboseCam			(false),
	outputNormals		(true),
	oppositeNormals		(false),
	outputTextures		(true),
	outputLights		(true),
	outputCamera		(true),
	outputGeometry		(true),
	outputJoints		(true),
	reverseWinding		(false),
	saveFrame			(-1),
	frameStart			(1),
	frameEnd			(1),
	frameStep			(1),
	numFrames			(1),
	hrcMode				(kHrcFlat),
	selType				(kSelAll),
	sampleTextures		(1),
	evalTextures		(0),
	xTexRes				(256),
	yTexRes				(256),
	xMaxTexRes			(4096),
	yMaxTexRes			(4096),
	precision			(5),
	useTabs				(true),
	writeComments		(false),
	scriptAppendFileName (false)
{
}

ge2Wrapper::~ge2Wrapper()
{
	if ( shapeInfo )
	{
		delete [] shapeInfo;
		shapeInfo = NULL;
	}

	if ( shaders )
	{
		delete [] shaders;
		shaders = NULL;
	}
}

//
// setDefaults() -- these values will be overridden by return of the
//		mel script. If the script isn't called for any reason, or
//		it fails to provide options for any methods, hopefully
//		this method will do enough initialization for the export to proceed...
void ge2Wrapper::setDefaults()
{	
	if ( shapeInfo )
	{
		delete [] shapeInfo;
		shapeInfo = NULL;
	}

	enableAnim = true;
	animVertices = true;
	vertexDisplacement = kVDAbsolute;
	animTransforms = true;
	animShaders = true;
	animLights = false;
	animCamera = false;
	keyCurves = true;
	keySample = true;
	sampleRate = 1;
	sampleTolerance = 0.5;

	useDomainGL = false;
	useDomainPSX = false;
	useDomainN64 = false;
	useDomainCustom = false;
	useOriginalFileTextures = false;
	verboseGeom = false;
	verboseLgt = false;
	verboseCam = false;
	outputNormals = true;
	oppositeNormals = false;
	outputTextures = true;
	outputLights = true;
	outputCamera = true;
	outputGeometry = true;
	outputJoints = true;
	reverseWinding = false;

	hrcMode = kHrcFlat;
	selType = kSelAll;
	sampleTextures = 1;
	evalTextures = 1;
	xTexRes = 256;
	yTexRes = 256;
	xMaxTexRes = 4096;
	yMaxTexRes = 4096;
	texType = MString( "sgi" );
	precision = 5;
	useTabs = true;
	writeComments = false;
	userScript.clear();
	scriptAppendFileName = false;
}

void ge2Wrapper::setPathName( const MString& path )
{
	int  lastSlashLocation = path.rindex ('/');

	if (lastSlashLocation != -1)
	{
		pathName = path.substring (0, lastSlashLocation - 1);
	} else
	{
		pathName = path;
	}

	strcpy( charPathName, pathName.asChar () );
}

void ge2Wrapper::setBaseFileName( const MString& baseName )
{
	baseFileName = baseName;

	strcpy( charBaseFileName, baseFileName.asChar () );
}

void ge2Wrapper::ntify( char* str )
{
	unsigned int i;
	
	for ( i = 0; i < strlen( str ); i++ )
	{
		if ( str[i] == '/' )
			str[i] = '\\';
	}
}

void ge2Wrapper::outputHeader( MTextFileWriter &fileWriter )
{
	fileWriter.print( " filetype gx;\n" );
	fileWriter.print( " version %s;\n", geVersion.asChar() );	
	fileWriter.print( " #  created from Maya::ge2Export plugin - version %s\n", pluginVersion.asChar() );
}

void ge2Wrapper::outputLGTTemplates( MTextFileWriter &fileWriter )
{
	// Ambient light
	fileWriter.print( " template ambient-light (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "float brightness %6.3f;\n", defaultAmbientLightBrightness );
	fileWriter.print( "vec3f rgb %6.3f %6.3f %6.3f;\n",
		defaultAmbientLightColor[0], defaultAmbientLightColor[1], defaultAmbientLightColor[2] );
	fileWriter.removeTab();
	fileWriter.print( " )\n" );

	fileWriter.print( "\n" );

	// infinite light
	fileWriter.print( " template infinite-light (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "float brightness %6.3f;\n", defaultInfiniteLightBrightness );
	fileWriter.print( "vec3f rgb %6.3f %6.3f %6.3f;\n",
		defaultInfiniteLightColor[0], defaultInfiniteLightColor[1], defaultInfiniteLightColor[2] );
	fileWriter.print( "vec3f direction %6.3f %6.3f %6.3f;\n",
		defaultInfiniteLightDirection[0], defaultInfiniteLightDirection[1], defaultInfiniteLightDirection[2] );
	fileWriter.removeTab();
	fileWriter.print( " )\n" );

	fileWriter.print( "\n" );

	// point-light
	fileWriter.print( " template point-light (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "float brightness %6.3f;\n", defaultPointLightBrightness );
	fileWriter.print( "vec3f rgb %6.3f %6.3f %6.3f;\n",
		defaultPointLightColor[0], defaultPointLightColor[1], defaultPointLightColor[2] );
	fileWriter.print( "vec3f position %6.3f %6.3f %6.3f;\n",
		defaultPointLightPosition[0], defaultPointLightPosition[1], defaultPointLightPosition[2] );
	fileWriter.print( "boolean use-attenuation TRUE;\n" ); // Just hard-coded it, OK??
	fileWriter.print( "float radius %6.3f;\n", defaultPointLightRadius );
	fileWriter.removeTab();
	fileWriter.print( " )\n" );

	fileWriter.print( "\n" );

	// spot-light
	fileWriter.print( " template spot-light (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "float brightness %6.3f;\n", defaultSpotLightBrightness );
	fileWriter.print( "vec3f rgb %6.3f %6.3f %6.3f;\n",
		defaultSpotLightColor[0], defaultSpotLightColor[1], defaultSpotLightColor[2] );
	fileWriter.print( "vec3f position %6.3f %6.3f %6.3f;\n",
		defaultSpotLightPosition[0], defaultSpotLightPosition[1], defaultSpotLightPosition[2] );
	fileWriter.print( "vec3f aim-point %6.3f %6.3f %6.3f;\n",
		defaultSpotLightAimPoint[0], defaultSpotLightAimPoint[1], defaultSpotLightAimPoint[2] );
	fileWriter.print( "boolean use-attenuation TRUE;\n" ); // Just hard-coded it, OK??
	fileWriter.print( "float radius %6.3f;\n", defaultSpotLightRadius );
	fileWriter.print( "string pattern;\n" );
	fileWriter.print( "float falloff-angle %6.3f;\n", defaultSpotLightFalloffAngle );
	fileWriter.print( "vec2f aspect-ratio %6.3f %6.3f;\n",
		defaultSpotLightAspectRatio[0], defaultSpotLightAspectRatio[1] );
	fileWriter.removeTab();
	fileWriter.print( " )\n" );

	fileWriter.print( "\n" );

	// light template
	fileWriter.print( " template light (\n" );
	fileWriter.addTab();
	fileWriter.print( "ambient-light ambient-lights[];\n" );
	fileWriter.print( "infinite-light infinite-lights[];\n" );
	fileWriter.print( "point-light point-lights[];\n" );
	fileWriter.print( "spot-light spot-lights[];\n" );
	fileWriter.removeTab();
	fileWriter.print( " )\n" );

	fileWriter.print( "\n" );
}

void ge2Wrapper::outputCAMTemplates( MTextFileWriter &fileWriter )
{
	fileWriter.print( "template camera (\n" );
	fileWriter.addTab();
	fileWriter.print( "float view-angle %6.3f;\n", defaultCameraViewAngle );
	fileWriter.print( "float hither %6.3f;\n", defaultCameraHitherDistance );
	fileWriter.print( "float yon %6.3f;\n", defaultCameraYonDistance );
	fileWriter.print( "vec3f location %6.3f %6.3f %6.3f;\n",
			defaultCameraLocation[0], defaultCameraLocation[1], defaultCameraLocation[2] );
	fileWriter.print( "vec3f aim %6.3f %6.3f %6.3f;\n",
			defaultCameraAimPoint[0], defaultCameraAimPoint[1], defaultCameraAimPoint[2] );
	fileWriter.print( "vec3f lookup-vector %6.3f %6.3f %6.3f;\n",
			defaultCameraLookupVector[0], defaultCameraLookupVector[1], defaultCameraLookupVector[2] );
	fileWriter.removeTab();
	fileWriter.print( ")\n" );
}

void ge2Wrapper::outputGOFTemplates( MTextFileWriter &fileWriter )
{
	fileWriter.print( "template vertex (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f coord 0.0 0.0 0.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template segment (\n" );
	fileWriter.addTab();
	fileWriter.print( "integer vertex-indices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template uv-array (\n" );
	fileWriter.addTab();
	fileWriter.print( "float uv[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template face (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f normal;\n" );
	fileWriter.print( "integer vertex-indices[];\n" );
	fileWriter.print( "integer vertex-normal-indices[];\n" );
	fileWriter.print( "integer vertex-color-indices[];\n" );
	fileWriter.print( "integer uv-indices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template face-part (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "string material;\n" );
	fileWriter.print( "integer face-indices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template vertex-part (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "integer vertex-indices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template vertex-displacement (\n" );
	fileWriter.addTab();
	fileWriter.print( "integer vertex-index;\n" );
	fileWriter.print( "vec3f coord 0.0 0.0 0.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template absolute-displacement (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vertex-displacement vertex-displacements[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template linear-displacement (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vertex-displacement vertices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template rotation-axis (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f head 0.0 0.0 0.0;\n" );
	fileWriter.print( "vec3f tail 0.0 0.0 0.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template cylindrical-displacement (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vertex-displacement vertices[];\n" );
	fileWriter.print( "rotation-axis rotation-axis;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template body (\n" );
	fileWriter.addTab();
	fileWriter.print( "vertex vertices[];\n" );
	fileWriter.print( "segment segments[];\n" );
	fileWriter.print( "face faces[];\n" );
	fileWriter.print( "vec3f normals[];\n" );
	fileWriter.print( "uv-array uv-array[];\n" );
	fileWriter.print( "vec3f vertex-color[];\n" );
	fileWriter.print( "face-part face-parts[];\n" );
	fileWriter.print( "string material;\n" );
	fileWriter.print( "vertex-part vertex-parts[];\n" );
	fileWriter.print( "absolute-displacement absolute-displacement[];\n" );
	fileWriter.print( "linear-displacement linear-displacement[];\n" );
	fileWriter.print( "cylindrical-displacement cylindrical-displacement[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template gobject (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vec4f matrix[] <1.0 0.0 0.0 0.0; 0.0 1.0 0.0 0.0; 0.0 0.0 1.0 0.0; 0.0 0.0 0.0 1.0;>\n" );
	fileWriter.print( "vec3f bounding-box[];\n" );
	fileWriter.print( "string body;\n" );
	fileWriter.print( "gobject children[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );
}

void ge2Wrapper::outputGMFTemplates( MTextFileWriter& fileWriter )
{
	fileWriter.print( "template map (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "integer width; integer height;\n" );
	fileWriter.print( "integer number-of-channels;\n" );
	fileWriter.print( "integer bits-per-channel;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f DIFFUSE-COLOR 0.8 0.8 0.8;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "vec3f SPECULAR-COLOR 0.0 0.0 0.0;\n" );
	fileWriter.print( "float SPECULAR-EXPONENT 1.0;\n" );
	fileWriter.print( "vec3f EMISSION-COLOR 0.0 0.0 0.0;\n" );
	fileWriter.print( "float OPACITY 1.0;\n" );
	fileWriter.print( "vec3f AMBIENT-COLOR 0.2 0.2 0.2;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template Lightscape-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f DIFFUSE-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "vec3f GLOW-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "integer GLOW-POWER 0;\n" );
	fileWriter.print( "float REFRACTION-INDEX 1.0;\n" );
	fileWriter.print( "boolean METAL-P FALSE;\n" );
	fileWriter.print( "float SMOOTHNESS 0.0;\n" );
	fileWriter.print( "float OPACITY 0.0;\n" );
	fileWriter.print( "float MESH-SCALE 1.0;\n" );
	fileWriter.print( "boolean MESHING-P TRUE;\n" );
	fileWriter.print( "boolean OCCLUDING-P TRUE;\n" );
	fileWriter.print( "boolean RECEIVING-P TRUE;\n" );
	fileWriter.print( "boolean REFLECTING-P TRUE;\n" );
	fileWriter.print( "boolean WINDOW-P FALSE;\n" );
	fileWriter.print( "boolean OPENING-P FALSE;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "string TEXTURE-MIN-FILTER \"POINT\";\n" );
	fileWriter.print( "string TEXTURE-MAX-FILTER \"POINT\";\n" );
	fileWriter.print( "boolean TEXTURE-MODULATE-P TRUE;\n" );
	fileWriter.print( "float TEXTURE-MAP-BRIGHTNESS 1.0;\n" );
	fileWriter.print( "boolean BUMP-P FALSE;\n" );
	fileWriter.print( "float BUMP-HEIGHT 0.5;\n" );
	fileWriter.print( "float BUMP-WIDTH 0.082021;\n" );
	fileWriter.print( "float BUMP-CLAMP 1.0;\n" );
	fileWriter.print( "boolean NOISE-P FALSE;\n" );
	fileWriter.print( "float NOISE-MAGNITUDE 1.0;\n" );
	fileWriter.print( "float NOISE-FREQUENCY 0.082021;\n" );
	fileWriter.print( "integer NOISE-LAYERS 1;\n" );
	fileWriter.print( "string FACING-CONTROL \"FRONT\";\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template Sega-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f DIFFUSE-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "string PLANE \"SINGLE\";\n" );
	fileWriter.print( "string SORT \"SORT_CEN\";\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "boolean REUSE-TEXTURE-MAP FALSE;\n" );
	fileWriter.print( "string DIR \"POLYGON\";\n" );
	fileWriter.print( "string TEXTURE-DIR \"NOFLIP\";\n" );
	fileWriter.print( "boolean USE-LIGHT-OPTION FALSE;\n" );
	fileWriter.print( "boolean USE-CLIP-OPTION FALSE;\n" );
	fileWriter.print( "boolean USE-PALETTE-OPTION FALSE;\n" );
	fileWriter.print( "string DRAWING-MODE \"OVERWRITE\";\n" );
	fileWriter.print( "boolean MESH FALSE;\n" );
	fileWriter.print( "boolean SEND-FACE-TO-SEGA FALSE;\n" );
	fileWriter.print( "boolean PRESHADE TRUE;\n" );
	fileWriter.print( "float TEXTURE-SCALE 1.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template N64-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "boolean USE-TEXTURE-MAP FALSE;\n" );
	fileWriter.print( "vec3f DIFFUSE-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "string FACING-CONTROL \"FRONT\";\n" );
	fileWriter.print( "boolean OMIT-FACE FALSE;\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.print( "boolean PRESHADE FALSE;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "string TEXTURE-BIT-DEPTH \"16\";\n" );
	fileWriter.print( "float FACE-ALPHA 1.0;\n" );
	fileWriter.print( "boolean USE-LIGHT-OPTION FALSE;\n" );
	fileWriter.print( "string TEXTURE-TYPE \"N64-RGBA\";\n" );
	fileWriter.print( "string TEXTURE-SIZE \"16\";\n" );
	fileWriter.print( "boolean BLEND-COLOR TRUE;\n" );

/*	 not sure about these:
	fileWriter.print( "boolean USE-ZBUFFER FALSE;\n" );
	fileWriter.print( "boolean USE-FOG FALSE;\n" );
	fileWriter.print( "boolean USE-REFLECTION-MAPPING FALSE;\n" );
*/

	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template SonyPSX-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f DIFFUSE-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "boolean USE-TEXTURE-MAP FALSE;\n" );
	fileWriter.print( "boolean OMIT-FACE FALSE;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "string FACING-CONTROL \"FRONT\";\n" );
	fileWriter.print( "boolean ABE FALSE;\n" );
	fileWriter.print( "boolean PRESHADE FALSE;\n" );
	fileWriter.print( "boolean LIGHT-CALCULATION FALSE;\n" );
	fileWriter.print( "string ABR \"0\";\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.print( "boolean COLOR-BY-VERTEX FALSE;\n" );

/*	not sure about these
	fileWriter.print( "boolean PERFORM-SUBDIVISION FALSE;\n" );
	fileWriter.print( "boolean PERFORM-ACTIVE-SUBDIVISION FALSE;\n" );
	fileWriter.print( "boolean CLIP FALSE;\n" );
	fileWriter.print( "boolean PERFORM-MIPMAPPING TRUE;\n" );
*/

	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template Render-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "float COLOR-OPACITY[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "vec3f SPECULAR-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "float PHONG-SPECULAR-EXPONENT[] <40.0; 40.0; 40.0; >\n" );
	fileWriter.print( "float COOK-SPECULAR-EXPONENT[] <0.05; 0.05; 0.05; >\n" );
	fileWriter.print( "float BLINN-SPECULAR-EXPONENT[] <0.15; 0.15; 0.15; >\n" );
	fileWriter.print( "vec3f AMBIENT-LIGHT-FILTER 1.0 1.0 1.0;\n" );
	fileWriter.print( "vec3f LIGHT-FILTER-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "float METAL 0.0;\n" );
	fileWriter.print( "vec3f FRESNEL 0.755 0.49 0.095;\n" );
	fileWriter.print( "float VISIBILITY 1.0;\n" );
	fileWriter.print( "float HIGHLIGHT-OPACITY[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.print( "boolean USE-ROUND-CONTROL FALSE;\n" );
	fileWriter.print( "float ROUND-DIFFUSE[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string ROUND-DIFFUSE-CURVE \"KAY\";\n" );
	fileWriter.print( "float ROUND-OPACITY[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string ROUND-OPACITY-CURVE \"KAY\";\n" );
	fileWriter.print( "float ROUND-SPECULAR[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string ROUND-SPECULAR-CURVE \"KAY\";\n" );
	fileWriter.print( "float ROUND-REFLECT[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string ROUND-REFLECT-CURVE \"KAY\";\n" );
	fileWriter.print( "float ROUND-REFRACT[] <1.0; 1.0; 1.0; >\n" );
	fileWriter.print( "string ROUND-REFRACT-CURVE \"KAY\";\n" );
	fileWriter.print( "boolean USE-FOG-CONTROL FALSE;\n" );
	fileWriter.print( "integer FOG-LIMIT[] <75; 250; >\n" );
	fileWriter.print( "boolean USE-DEFAULT-FOG-COLOR TRUE;\n" );
	fileWriter.print( "vec3f FOG-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "boolean USE-3D-WIPE-CONTROL FALSE;\n" );
	fileWriter.print( "float FOG-OPACITY 1.0;\n" );
	fileWriter.print( "float 3D-WIPE-LIMIT[] <-1.0; -1.0; >\n" );
	fileWriter.print( "string FACING-CONTROL \"FRONT\";\n" );
	fileWriter.print( "float EXPONENT-UV[] <0.15; 0.15; >\n" );
	fileWriter.print( "string HAIRLINE-AXIS \"X\";\n" );
	fileWriter.print( "float HAIRLINE-CENTER[] <0.0; 0.0; 0.0; >\n" );
	fileWriter.print( "boolean USE-BUMP-MAP FALSE;\n" );
	fileWriter.print( "map BUMP-MAP;\n" );
	fileWriter.print( "float BUMP-UV-SCALE[] <1.0; 1.0; >\n" );
	fileWriter.print( "float BUMP-UV-OFFSET[] <0.0; 0.0; >\n" );
	fileWriter.print( "float BUMP-UV-ROTATE 0.0;\n" );
	fileWriter.print( "boolean BUMP-USE-ST FALSE;\n" );
	fileWriter.print( "integer BUMP-ST-UPPER-LEFT[] <0; 0; >\n" );
	fileWriter.print( "integer BUMP-ST-LOWER-RIGHT[] <0; 0; >\n" );
	fileWriter.print( "integer BUMP-ST-DIFFUSION-RANGE[] <0; 0; >\n" );
	fileWriter.print( "string BUMP-MAP-SHADER \"JITTER\";\n" );
	fileWriter.print( "integer BUMP-MAP-SAMPLING-LIMIT[] <32; 256; >\n" );
	fileWriter.print( "integer BUMP-MAP-JITTER-SAMPLING-LIMIT[] <1; 3; >\n" );
	fileWriter.print( "string USE-BUMP-MAP-CONTRIBUTE \"NO\";\n" );
	fileWriter.print( "float BUMP-MAP-CONTRIBUTE[] <0.01; 10000.0; >\n" );
	fileWriter.print( "string USE-BUMP-MAP-ALPHA \"NO\";\n" );
	fileWriter.print( "float BUMP-MAP-ALPHA 1.0;\n" );
	fileWriter.print( "string BUMP-MAP-BOUNDARY \"STD\";\n" );
	fileWriter.print( "float BUMP-MAP-DEPTH[] <1.0; 1.0; >\n" );
	fileWriter.print( "boolean USE-TEXTURE-MAP FALSE;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "float TEXTURE-UV-SCALE[] <1.0; 1.0; >\n" );
	fileWriter.print( "float TEXTURE-UV-OFFSET[] <0.0; 0.0; >\n" );
	fileWriter.print( "float TEXTURE-UV-ROTATE 0.0;\n" );
	fileWriter.print( "boolean TEXTURE-USE-ST FALSE;\n" );
	fileWriter.print( "integer TEXTURE-ST-UPPER-LEFT[] <0; 0; >\n" );
	fileWriter.print( "integer TEXTURE-ST-LOWER-RIGHT[] <0; 0; >\n" );
	fileWriter.print( "integer TEXTURE-ST-DIFFUSION-RANGE[] <0; 0; >\n" );
	fileWriter.print( "string TEXTURE-MAP-SHADER \"JITTER\";\n" );
	fileWriter.print( "integer TEXTURE-MAP-SAMPLING-LIMIT[] <32; 256; >\n" );
	fileWriter.print( "integer TEXTURE-MAP-JITTER-SAMPLING-LIMIT[] <1; 3; >\n" );
	fileWriter.print( "string USE-TEXTURE-MAP-CONTRIBUTE \"NO\";\n" );
	fileWriter.print( "float TEXTURE-MAP-CONTRIBUTE[] <0.01; 10000.0; >\n" );
	fileWriter.print( "float TEXTURE-MAP-ALPHA 1.0;\n" );
	fileWriter.print( "string USE-TEXTURE-MAP-ALPHA \"NO\";\n" );
	fileWriter.print( "string TEXTURE-MAP-BOUNDARY \"STD\";\n" );
	fileWriter.print( "float TEXTURE-OPAQUE 1.0;\n" );
	fileWriter.print( "boolean USE-OPACITY-MAP FALSE;\n" );
	fileWriter.print( "map OPACITY-MAP;\n" );
	fileWriter.print( "float OPACITY-UV-SCALE[] <1.0; 1.0; >\n" );
	fileWriter.print( "float OPACITY-UV-OFFSET[] <0.0; 0.0; >\n" );
	fileWriter.print( "float OPACITY-UV-ROTATE 0.0;\n" );
	fileWriter.print( "boolean OPACITY-USE-ST FALSE;\n" );
	fileWriter.print( "integer OPACITY-ST-UPPER-LEFT[] <0; 0; >\n" );
	fileWriter.print( "integer OPACITY-ST-LOWER-RIGHT[] <0; 0; >\n" );
	fileWriter.print( "integer OPACITY-ST-DIFFUSION-RANGE[] <0; 0; >\n" );
	fileWriter.print( "string OPACITY-MAP-SHADER \"JITTER\";\n" );
	fileWriter.print( "integer OPACITY-MAP-SAMPLING-LIMIT[] <32; 256; >\n" );
	fileWriter.print( "integer OPACITY-MAP-JITTER-SAMPLING-LIMIT[] <1; 3; >\n" );
	fileWriter.print( "string USE-OPACITY-MAP-CONTRIBUTE \"NO\";\n" );
	fileWriter.print( "float OPACITY-MAP-CONTRIBUTE[] <0.01; 10000.0; >\n" );
	fileWriter.print( "float OPACITY-MAP-ALPHA 1.0;\n" );
	fileWriter.print( "string USE-OPACITY-MAP-ALPHA \"NO\";\n" );
	fileWriter.print( "string OPACITY-MAP-BOUNDARY \"STD\";\n" );
	fileWriter.print( "float OPACITY-OPAQUE 1.0;\n" );
	fileWriter.print( "boolean USE-REFLECT-MAP FALSE;\n" );
	fileWriter.print( "map REFLECT-MAP;\n" );
	fileWriter.print( "float REFLECT-UV-SCALE[] <1.0; 1.0; >\n" );
	fileWriter.print( "boolean REFLECT-USE-ST FALSE;\n" );
	fileWriter.print( "integer REFLECT-ST-UPPER-LEFT[] <0; 0; >\n" );
	fileWriter.print( "integer REFLECT-ST-LOWER-RIGHT[] <0; 0; >\n" );
	fileWriter.print( "integer REFLECT-ST-DIFFUSION-RANGE[] <0; 0; >\n" );
	fileWriter.print( "string REFLECT-MAP-SHADER \"JITTER\";\n" );
	fileWriter.print( "integer REFLECT-MAP-SAMPLING-LIMIT[] <32; 256; >\n" );
	fileWriter.print( "integer REFLECT-MAP-JITTER-SAMPLING-LIMIT[] <1; 3; >\n" );
	fileWriter.print( "string USE-REFLECT-MAP-CONTRIBUTE \"NO\";\n" );
	fileWriter.print( "float REFLECT-MAP-CONTRIBUTE[] <0.01; 10000.0; >\n" );
	fileWriter.print( "string USE-REFLECT-MAP-ALPHA \"NO\";\n" );
	fileWriter.print( "float REFLECT-MAP-ALPHA 1.0;\n" );
	fileWriter.print( "string REFLECT-MAP-BOUNDARY \"STD\";\n" );
	fileWriter.print( "vec3f REFLECT-MAP-COLOR-SCALE 1.0 1.0 1.0;\n" );
	fileWriter.print( "vec3f REFLECT-MAP-BG-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "float REFLECT-OPAQUE 1.0;\n" );
	fileWriter.print( "boolean USE-REFRACT-MAP FALSE;\n" );
	fileWriter.print( "map REFRACTION-MAP;\n" );
	fileWriter.print( "float REFRACT-UV-SCALE[] <1.0; 1.0; >\n" );
	fileWriter.print( "boolean REFRACT-USE-ST FALSE;\n" );
	fileWriter.print( "integer REFRACT-ST-UPPER-LEFT[] <0; 0; >\n" );
	fileWriter.print( "integer REFRACT-ST-LOWER-RIGHT[] <0; 0; >\n" );
	fileWriter.print( "integer REFRACT-ST-DIFFUSION-RANGE[] <0; 0; >\n" );
	fileWriter.print( "string REFRACT-MAP-SHADER \"JITTER\";\n" );
	fileWriter.print( "integer REFRACT-MAP-SAMPLING-LIMIT[] <32; 256; >\n" );
	fileWriter.print( "integer REFRACT-MAP-JITTER-SAMPLING-LIMIT[] <1; 3; >\n" );
	fileWriter.print( "string USE-REFRACT-MAP-CONTRIBUTE \"NO\";\n" );
	fileWriter.print( "float REFRACT-MAP-CONTRIBUTE[] <0.01; 10000.0; >\n" );
	fileWriter.print( "string USE-REFRACT-MAP-ALPHA \"NO\";\n" );
	fileWriter.print( "float REFRACT-MAP-ALPHA 1.0;\n" );
	fileWriter.print( "string REFRACT-MAP-BOUNDARY \"STD\";\n" );
	fileWriter.print( "vec3f REFRACT-MAP-COLOR-SCALE 1.0 1.0 1.0;\n" );
	fileWriter.print( "vec3f REFRACT-MAP-BG-COLOR 1.0 1.0 1.0;\n" );
	fileWriter.print( "float REFRACT-OPAQUE 1.0;\n" );
	fileWriter.print( "boolean RAY-TRACE FALSE;\n" );
	fileWriter.print( "float REFLECTANCE 0.0;\n" );
	fileWriter.print( "float REFRACTANCE 0.0;\n" );
	fileWriter.print( "float REFRACTION-INDEX[] <1.0; 1.33; >\n" );
	fileWriter.print( "float BUMP-DISSOLVE 1.0;\n" );
	fileWriter.print( "float TEXTURE-DISSOLVE 1.0;\n" );
	fileWriter.print( "float TEXTURE-DARKNESS 1.0;\n" );
	fileWriter.print( "vec3f TEXTURE-MAP-COLOR-SCALE 1.0 1.0 1.0;\n" );
	fileWriter.print( "float OPACITY-DISSOLVE 1.0;\n" );
	fileWriter.print( "float OPACITY-DARKNESS 1.0;\n" );
	fileWriter.print( "vec3f OPACITY-MAP-COLOR-SCALE 1.0 1.0 1.0;\n" );
	fileWriter.print( "float REFLECT-DISSOLVE 1.0;\n" );
	fileWriter.print( "float REFLECT-DARKNESS 1.0;\n" );
	fileWriter.print( "float REFRACT-DISSOLVE 1.0;\n" );
	fileWriter.print( "float REFRACT-DARKNESS 1.0;\n" );
	fileWriter.print( "float REFRACT-INDEX 1.333;\n" );
	fileWriter.print( "string SHADING-MODEL \"LAMBERT\";\n" );
	fileWriter.print( "vec3f AMBIENT-VOLUME-FILTER 0.15 0.15 0.15;\n" );
	fileWriter.print( "vec3f DIFFUSE-VOLUME-FILTER 1.0 1.0 1.0;\n" );
	fileWriter.print( "float DENSITY 1.0;\n" );
	fileWriter.print( "vec3f DENSITY-LIMIT 1.0 1.0 1.0;\n" );
	fileWriter.print( "boolean USE-NOISE FALSE;\n" );
	fileWriter.print( "string FAST-NOISE \"OFF\";\n" );
	fileWriter.print( "float NOISE-FREQUENCY 1.0;\n" );
	fileWriter.print( "integer NOISE-DETAIL 1;\n" );
	fileWriter.print( "float NOISE-OFFSET 0.0;\n" );
	fileWriter.print( "float NOISE-CLIP[] <0.0; 1.0; >\n" );
	fileWriter.print( "float NOISE-SCALE 1.0;\n" );
	fileWriter.print( "string NOISE-REVERSE \"OFF\";\n" );
	fileWriter.print( "string NOISE-TURBULENCE \"OFF\";\n" );
	fileWriter.print( "boolean USE-NOISE-SEED FALSE;\n" );
	fileWriter.print( "float NOISE-SEED 1.0;\n" );
	fileWriter.print( "float VOLUME-DARKNESS-PERCENTAGE 0.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template GL_Shade-domain extends Base-domain (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f DIFFUSE-COLOR 0.8 0.8 0.8;\n" );
	fileWriter.print( "float SHININESS 0.0;\n" );
	fileWriter.print( "vec3f SPECULAR-COLOR 0.0 0.0 0.0;\n" );
	fileWriter.print( "vec3f EMISSION-COLOR 0.0 0.0 0.0;\n" );
	fileWriter.print( "string VERTEX-NORMAL-SOURCE \"FACE\";\n" );
	fileWriter.print( "boolean OUTLINING FALSE;\n" );
	fileWriter.print( "map TEXTURE-MAP;\n" );
	fileWriter.print( "float OPACITY 1.0;\n" );
	fileWriter.print( "boolean COLOR-BY-VERTEX FALSE;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template material (\n" );
	fileWriter.addTab();
	fileWriter.print( "Base-domain Base;\n" );
	fileWriter.print( "Lightscape-domain Lightscape;\n" );
	fileWriter.print( "Sega-domain Sega;\n" );
	fileWriter.print( "N64-domain N64;\n" );
	fileWriter.print( "SonyPSX-domain SonyPSX;\n" );
	fileWriter.print( "Render-domain Render;\n" );
	fileWriter.print( "GL_Shade-domain GL_Shade;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );
}

void ge2Wrapper::outputGAFTemplates( MTextFileWriter &fileWriter )
{
	fileWriter.print( "template animation (\n" );
	fileWriter.addTab();
	fileWriter.print( "integer number-of-frames 0;\n" );
	fileWriter.print( "string displacement-base-name;\n" );
	fileWriter.print( "animation-frame frames[];\n" );
	fileWriter.print( "integer keyframe-indices[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template animation-frame (\n" );
	fileWriter.addTab();
	fileWriter.print( "integer frame-number 0;\n" );
	fileWriter.print( "pobject-animation pobjects[];\n" );
	fileWriter.print( "skeleton-animation skeletons[];\n" );
	fileWriter.print( "light lights;\n" );
	fileWriter.print( "camera camera;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template pobject-animation (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "boolean visible TRUE;\n" );
	fileWriter.print( "vec4f matrix[];\n" );
	fileWriter.print( "vec3f rst[];\n" );
	fileWriter.print( "displacement-animation displacements[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template skeleton-animation (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vec3f root-rst[];\n" );
	fileWriter.print( "bone-data bone-data[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template bone-data (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vec4f dofs;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template displacement-animation (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "float factor 0.0;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );
}

void ge2Wrapper::outputGSFTemplates( MTextFileWriter &fileWriter )
{
	fileWriter.print( "template skeleton (\n" );
	fileWriter.addTab();
	fileWriter.print( "joint joints[];\n" );
	fileWriter.print( "bone bones[];\n" );
	fileWriter.print( "base bases[];\n" );
	fileWriter.print( "pose poses[];\n" );
	fileWriter.print( "string skins[];\n" );
	fileWriter.print( "string base;\n" );
	fileWriter.print( "string root-joint;\n" );
	fileWriter.print( "string rotation-order;\n" );
	fileWriter.print( "string translation-order;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template joint (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "vec3f coordinate;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );
	fileWriter.print( "template bone (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "string rotation-order \"xyz\";\n" );
	fileWriter.print( "dof-limit dof-limit;\n" );
	fileWriter.print( "float length 1.0;\n" );
	fileWriter.print( "axis-orientation axis-orientation;\n" );
	fileWriter.print( "string locked-to;\n" );
	fileWriter.print( "string parent-joint;\n" );
	fileWriter.print( "string child-joint;\n" );
	fileWriter.print( "string attached-objects[];\n" );
	fileWriter.print( "skin-part hard-parts[];\n" );
	fileWriter.print( "skin-part soft-parts[];\n" );
	fileWriter.print( "string softness-function \":sinusoid\";\n" );
	fileWriter.print( "vec4f softenss-exponent 0.6 0.6 0.6 0.6;\n" );
	fileWriter.print( "skin-displacement skin-displacements[];\n" );
	fileWriter.print( "bone children[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template dof-limit (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec4f min;\n" );
	fileWriter.print( "vec4f max;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template axis-orientation (\n" );
	fileWriter.addTab();
	fileWriter.print( "boolean rotate-axis FALSE;\n" );
	fileWriter.print( "vec3f axis-orientation;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template skin-part (\n" );
	fileWriter.addTab();
	fileWriter.print( "string part;\n" );
	fileWriter.print( "string type;\n" );
	fileWriter.print( "integer polyhedron-index;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template skin-displacement (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "string disp-object;\n" );
	fileWriter.print( "string type;\n" );
	fileWriter.print( "float angle;\n" );
	fileWriter.print( "float exp 1.0;\n" );
	fileWriter.print( "string limit-action;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template base (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "pose-root-array pose-root-array;\n" );
	fileWriter.print( "bone-frame-axis bone-frame-axes[];\n" );
	fileWriter.print( "base-bone-axis bone-axes[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template pose-root-array (\n" );
	fileWriter.addTab();
	fileWriter.print( "vec3f translation;\n" );
	fileWriter.print( "vec3f rotation;\n" );
	fileWriter.print( "vec3f scale;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template bone-frame-axis (\n" );
	fileWriter.addTab();
	fileWriter.print( "string bone;\n" );
	fileWriter.print( "vec3f frame-dx;\n" );
	fileWriter.print( "vec3f frame-dy;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template base-bone-axis (\n" );
	fileWriter.addTab();
	fileWriter.print( "string bone;\n" );
	fileWriter.print( "vec3f bone-rotation;\n" );
	fileWriter.print( "float scale;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template pose (\n" );
	fileWriter.addTab();
	fileWriter.print( "string name;\n" );
	fileWriter.print( "pose-root-array pose-root-array;\n" );
	fileWriter.print( "pose-bone-axis bone-aes[];\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );

	fileWriter.print( "template pose-bone-axis (\n" );
	fileWriter.addTab();
	fileWriter.print( "string bone;\n" );
	fileWriter.print( "vec4f bone-displacement-dofs;\n" );
	fileWriter.removeTab();
	fileWriter.print( ")\n\n" );
}

void ge2Wrapper::initScene()
{
	int		currShape;

	char	tmpSceneName[128];

	switch (selType)
	{
	case kSelAll:
		DtExt_setWalkMode( ALL_Nodes );
		break;
	case kSelPicked:
		DtExt_setWalkMode( PICKED_Nodes );
		break;
	case kSelActive:
		DtExt_setWalkMode( ACTIVE_Nodes );
		break;
	case kSelInvalid:
		DtExt_setWalkMode( ALL_Nodes );
		break;
	}

	strcpy( tmpSceneName, baseFileName.asChar() );
	DtExt_SceneInit( tmpSceneName );

#ifdef MAYA1X
	saveFrame = 1;
#else
	// So we can return to this later, use as a basis, etc.
	MTime userFrame( MAnimControl::currentTime().value(), MTime::uiUnit() );
	saveFrame = (int) userFrame.value();
#endif

	if ( enableAnim )
	{
		if(frameStart > frameEnd) {
			// if the frame range is backwards, swap it around (or crash :-)
			int tmpFrame = frameEnd;
			frameEnd = frameStart;
			frameStart = tmpFrame;
		}
		DtFrameSetStart( frameStart );
		DtFrameSetEnd( frameEnd );
		DtFrameSetBy( frameStep );
		int rawDiff = frameEnd - frameStart;
		int remainder = rawDiff % frameStep;
		numFrames = (rawDiff / frameStep) + 1;
		if ( 0 < remainder )
			numFrames += 1;
		if ( saveFrame < frameStart )
			saveFrame = frameStart;
		if ( saveFrame > frameEnd )
			saveFrame = frameEnd;
	}
	else
	{
		DtFrameSetStart( saveFrame );
		DtFrameSetEnd( saveFrame );
		DtFrameSetBy( 1 );
		numFrames = 1;
	}

	//Set hierarchy conversion mode
	switch ( hrcMode )
	{
	case kHrcFull:
		DtExt_setOutputTransforms (kTRANSFORMALL);
		DtExt_setParents (1);
		break;
	case kHrcFlat:
		DtExt_setOutputTransforms (kTRANSFORMMINIMAL);
		DtExt_setParents (0);
		break;
	case kHrcWorld:
		DtExt_setOutputTransforms (kTRANSFORMNONE);
		DtExt_setParents (0);
		break;
	default:
        DtExt_setOutputTransforms (kTRANSFORMMINIMAL);
        DtExt_setParents (0);
		break;
	}

	DtExt_setJointHierarchy( outputJoints );

    // Allow the user to specify if the textures should be sampled 
    // with the Texture Placement options
	DtExt_setSoftTextures( sampleTextures );

	// Allow the user to specify if the textures should be evaluated with
	// convertSolidTx command or read in if it is a file texture.	
	DtExt_setInlineTextures( evalTextures );

	//Set the size of the texture swatches to use  
	DtExt_setXTextureRes (xTexRes); 
	DtExt_setYTextureRes (yTexRes);

	DtExt_setMaxXTextureRes( xMaxTexRes );
	DtExt_setMaxYTextureRes( yMaxTexRes );

#if 0
	uint debug = 0;
	if ( verboseGeom )
		debug |= DEBUG_GEOMETRY;
	if ( verboseLgt )
		debug |= DEBUG_LIGHTS;
	if ( verboseCam )
		debug |= DEBUG_CAMERA;

	DtExt_setDebug( debug );
#else
	//See if user wants some verbose messages
	if ( verboseGeom || verboseLgt || verboseCam )
	{
		DtExt_setDebug( true );
	}
#endif

	// Say that we want to have camera info
	DtExt_setOutputCameras( outputCamera );
	// DtExt_setOutputLights( outputLights );
	// DtExt_setOutputGeometry( outputGeometry );

	writer.setUseTabs( useTabs );

    //Now we can setup the database from the wire file geometry
	DtExt_dbInit();

	numShapes = DtShapeGetCount();
	shapeInfo = new MShapeInfo[numShapes];

	for ( currShape = 0; currShape < numShapes; currShape++ )
	{
		shapeInfo[currShape].index = currShape;
		shapeInfo[currShape].allocFrameInfo( numFrames );
	}

	if ( outputGeometry )
	{
		collectGeometry();

		if ( enableAnim && ( animVertices || animTransforms ) )
		{
			if ( !keyCurves && !keySample )
			{
				setAllKeyFrames();
			}
			else 
			{
				if ( keyCurves )
				{
					setAnimCurveKeyFrames();
				}

				if ( keySample )
				{
					setSampledKeyFrames();
				}
			}
		}
	}

	collectMaterials();
}

void ge2Wrapper::collectMaterials()
{
	int		currShader;

	if ( shaders )
	{
		delete shaders;
		shaders = NULL;
	}

	DtMtlGetSceneCount( &numShaders );

	if ( 0 == numShaders )
		return;

	shaders = new MShader[numShaders];

	for ( currShader = 0; currShader < numShaders; currShader++ )
	{
		shaders[currShader].fill( currShader, *this );
	}
}

int ge2Wrapper::frameConvertLocalToDt( int localFrame )
{
	int		dtFrame;

	if ( DtFrameGetEnd() < (localFrame * DtFrameGetBy()) )
		dtFrame = DtFrameGetEnd();
	else
		dtFrame = DtFrameGetStart() + (localFrame*DtFrameGetBy());

	return dtFrame;
}

int ge2Wrapper::frameConvertDtToLocal( int dtFrame )
{
	int		localFrame;

	localFrame = (dtFrame - DtFrameGetStart()) / DtFrameGetBy();

	if ( localFrame >= numFrames )
	{
		if ( localFrame > numFrames )
		{
			cerr << "Requested frame range smaller that actual frame range" << endl
				 << "Please check your animation export settings." << endl;
		}
		localFrame = numFrames - 1;
	}
	else
	{
		if ( ((dtFrame - DtFrameGetStart()) % DtFrameGetBy()) != 0 )
		{
			cerr << "#### Error: Something wrong in frameConvertDtToLocal()!" << endl
				<< "Frame passed in was not on step?" << endl;
		}
	}

	return localFrame;
}

void ge2Wrapper::collectGeometry()
{
	int		currDtFrame,
			currLocalFrame,
			currShape;

	// shapeInfo[shape].frameInfo is a 0 based array with numFrames elements.
	// we access these by currFrame, which runs from 0 to numFrames;
	// dtFrame is set to be the corresponding frame in Maya terms
	for ( currLocalFrame = 0; currLocalFrame < numFrames; currLocalFrame++ )
	{
		currDtFrame = frameConvertLocalToDt( currLocalFrame );
		DtFrameSet( currDtFrame );

		for ( currShape = 0; currShape < numShapes; currShape++ )
		{
			shapeInfo[currShape].fill( currLocalFrame, animTransforms, animVertices );
		}
	}

	DtFrameSet( saveFrame );
}

void ge2Wrapper::setAllKeyFrames()
{
	int			currShape,
				currFrame;

	for ( currShape = 0; currShape < numShapes; currShape++ )
	{
		for ( currFrame = 0; currFrame < numFrames; currFrame++ )
		{
			shapeInfo[currShape].addKeyFrame( currFrame );
		}
	}
}

void ge2Wrapper::setAnimCurveKeyFrames()
{
	int			currShape,
				currKey,
				dtKey,
				localKey,
				numKeys;

	MIntArray	dgKeyArray;

	for ( currShape = 0; currShape < numShapes; currShape++ )
	{
		// We're going to get the transform vertices even if the user
		// has just set animVertices. This will catch transform keyframes,
		// which you may not want...
		if ( animTransforms || animVertices )
		{
			dgKeyArray = shapeGetTRSAnimKeys( currShape );
			numKeys = dgKeyArray.length();
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				dtKey = dgKeyArray[currKey];
				localKey = frameConvertDtToLocal( dtKey );
				shapeInfo[currShape].addKeyFrame( localKey );
			}
		}

		if ( animVertices )
		{
			dgKeyArray = shapeGetVertAnimKeys( currShape );
			numKeys = dgKeyArray.length();
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				dtKey = dgKeyArray[currKey];
				localKey = frameConvertDtToLocal( dtKey );
				shapeInfo[currShape].addKeyFrame( localKey );
			}
		}
	}
}

// This function uses a MShapeInfo method called getKeyBetween...
// getKeyBetween is essentially using keyframes found on animCurves.
// If there are such keyframes, we assume that they are at points we're outputting
// (i.e. they're at integral frames and not between 'steps'.)
// if they're not they should be reassigned to the closest one (if anim
// curves are used, step is forced to be 1)
void ge2Wrapper::setSampledKeyFrames()
{
	int				prevFrame,
					nextFrame,
					currShape;

	int				currKey,
					nextKey,
					lastKey,
					tempKey;

	int				currVertex;

	MFloatVector	diff;

	MComputation	computation;

	computation.beginComputation();

	for ( currShape = 0; currShape < numShapes; currShape++ )
	{		
		// Always treat first and last as keys...
		shapeInfo[currShape].addKeyFrame( 0 );
		shapeInfo[currShape].addKeyFrame( numFrames-1 );
		lastKey = 0;		

		while ( (currKey = getNextKeyToSample( lastKey )) < (numFrames - 1) )
		{				
			if ( computation.isInterruptRequested() )
			{
				computation.endComputation();
				return;
			}

			tempKey = shapeInfo[currShape].getLastKeyBetween( lastKey, currKey );
			if ( -1 != tempKey )
				lastKey = tempKey;
			// else there's one later...
			
			nextKey = getNextKeyToSample( currKey );
			tempKey = shapeInfo[currShape].getFirstKeyBetween( currKey, nextKey );
			if ( -1 != tempKey ) // this means there's one already set before the next sample
				nextKey = tempKey;

			// may want to change currKey to be midpoint between last and next keys
			// for more uniform sampling. If this is done, getNextKeyToSample() should be
			// modified to add rate and not do an int divide to find it...

			if ( shapeInfo[currShape].isKeyFrame( currKey ) )
			{
				lastKey = currKey;
				continue;
			}

			if ( animTransforms )
			{
				MFloatVectorArray	diffTranslationPrev,
									diffTranslationNext,
									diffRotationPrev,
									diffRotationNext,
									diffScalePrev,
									diffScaleNext;

				for ( prevFrame = lastKey + 1; prevFrame <= currKey; prevFrame++ )
				{
					diff = shapeInfo[currShape].frameInfo[prevFrame].translation -
							shapeInfo[currShape].frameInfo[prevFrame-1].translation;
					diffTranslationPrev.append( diff );
					diff = shapeInfo[currShape].frameInfo[prevFrame].rotation -
							shapeInfo[currShape].frameInfo[prevFrame-1].rotation;
					diffRotationPrev.append( diff );
					diff = shapeInfo[currShape].frameInfo[prevFrame].scale - 
							shapeInfo[currShape].frameInfo[prevFrame-1].scale;
					diffScalePrev.append( diff );
				}

				for ( nextFrame = currKey + 1; nextFrame <= nextKey; nextFrame++ )
				{
					diff = shapeInfo[currShape].frameInfo[nextFrame].translation -
							shapeInfo[currShape].frameInfo[nextFrame-1].translation;
					diffTranslationNext.append( diff );
					diff = shapeInfo[currShape].frameInfo[nextFrame].rotation -
							shapeInfo[currShape].frameInfo[nextFrame-1].rotation;
					diffRotationNext.append( diff );
					diff = shapeInfo[currShape].frameInfo[nextFrame].scale - 
							shapeInfo[currShape].frameInfo[nextFrame-1].scale;
					diffScaleNext.append( diff );
				}
				
				bool foundKey = false;
				if ( areDifferent( diffTranslationPrev, diffTranslationNext, kDiffTranslate ) )
					foundKey = true;
				else
					if ( areDifferent( diffRotationPrev, diffRotationNext, kDiffRotate ) )
						foundKey = true;
					else
						if ( areDifferent( diffScalePrev, diffScaleNext, kDiffScale ) )
							foundKey = true;

				if ( foundKey )
				{
					shapeInfo[currShape].addKeyFrame( currKey );
					lastKey = currKey;
					continue;
				}
			}

			if ( animVertices )
			{
				bool foundKey = false;
				for ( currVertex = 0; currVertex < shapeInfo[currShape].frameInfo[currKey].numVertices; currVertex++ )
				{
					MFloatVectorArray	diffVertexPrev,
										diffVertexNext;

					for ( prevFrame = lastKey + 1; prevFrame <= currKey; prevFrame++ )
					{
						diff = shapeInfo[currShape].frameInfo[prevFrame].vertexPosition[currVertex] -
								shapeInfo[currShape].frameInfo[prevFrame-1].vertexPosition[currVertex];
						diffVertexPrev.append( diff );
					}

					for ( nextFrame = currKey + 1; nextFrame <= nextKey; nextFrame++ )
					{
						diff = shapeInfo[currShape].frameInfo[nextFrame].vertexPosition[currVertex] -
								shapeInfo[currShape].frameInfo[nextFrame-1].vertexPosition[currVertex];
						diffVertexNext.append( diff );
					}

					if ( areDifferent( diffVertexPrev, diffVertexNext, kDiffVertex ) )
					{
						foundKey = true;
						break;
					}
				}

				if ( foundKey )
				{
					shapeInfo[currShape].addKeyFrame( currKey );
					lastKey = currKey;
					continue;
				}
			}

			lastKey = currKey;
		}
	}

	computation.endComputation();
}

bool ge2Wrapper::areDifferent( const MFloatVectorArray& first, const MFloatVectorArray& second, GEDiffType diffType )
{
	int				index,
					numVectors;

	MFloatVector	avgFirst( MFloatVector::zero ),
					avgSecond( MFloatVector::zero ),
					maxDiff( MFloatVector::zero ),
					avgDiff;

	float			x, y, z;

	// Will have to do some special processing for rotations

	numVectors = first.length();
	if ( numVectors )
	{
		for ( index = 0; index < numVectors; index++ )
			avgFirst += first[index];
		avgFirst /= (float) numVectors;
	}

	numVectors = second.length();
	if ( numVectors )
	{
		for ( index = 0; index < numVectors; index++ )
			avgSecond += second[index];
		avgSecond /= (float) numVectors;
		if ( numVectors > 1 )
		{
		// If any of the vectors in the second set differs significantly
		// from the avg, we want to flag that case as well
			for ( index = 0; index < numVectors; index++ )
				if ( second[index].length() > maxDiff.length() )
					maxDiff = second[index];
		}
	}

	avgDiff = avgSecond - avgFirst;
	x = (float) fabs( avgDiff.x );
	y = (float) fabs( avgDiff.y );
	z = (float) fabs( avgDiff.z );

	switch (diffType)
	{
	case kDiffTranslate:
		if ( ( translateTolAvg() < x ) ||
			 ( translateTolAvg() < y ) ||
			 ( translateTolAvg() < z ) )
			 return true;
		if ( translateTolSingle( second.length() ) < maxDiff.length() )
			return true;
		break;
	case kDiffVertex:
		if ( ( vertexTolAvg() < x ) ||
			 ( vertexTolAvg() < y ) ||
			 ( vertexTolAvg() < z ) )
			 return true;
		if ( vertexTolSingle( second.length() ) < maxDiff.length() )
			return true;
		break;
	case kDiffScale:
		if ( ( scaleTolAvg() < x ) ||
			 ( scaleTolAvg() < y ) ||
			 ( scaleTolAvg() < z ) )
			 return true;
		if ( scaleTolSingle( second.length() ) < maxDiff.length() )
			return true;
		break;
	case kDiffRotate:
		if ( ( rotateTolAvg() < x ) ||
			 ( rotateTolAvg() < y ) ||
			 ( rotateTolAvg() < z ) )
			 return true;
		if ( rotateTolSingle( second.length() ) < maxDiff.length() )
			return true;
		break;
	}

	// If we made it this far we:
	return false;
}

// These are pretty rudimentary, as you can see. Change them
// to suit your needs...
float ge2Wrapper::translateTolAvg()
{
	return sampleTolerance;
}

float ge2Wrapper::translateTolSingle( int numElements )
{
	return 5*numElements*sampleTolerance;
}

float ge2Wrapper::rotateTolAvg()
{
	return sampleTolerance;
}

float ge2Wrapper::rotateTolSingle( int numElements )
{
	return 5*numElements*sampleTolerance;
}

float ge2Wrapper::scaleTolAvg()
{
	return sampleTolerance * sampleTolerance;
}

float ge2Wrapper::scaleTolSingle( int numElements )
{
	return 5*numElements*sampleTolerance;
}

float ge2Wrapper::vertexTolAvg()
{
	return sampleTolerance * sampleTolerance;
}

float ge2Wrapper::vertexTolSingle( int numElements )
{
	return 5*numElements*sampleTolerance;
}

int ge2Wrapper::getNextKeyToSample( int lastKey )
{
	int	numSamplesToLast = lastKey / sampleRate; // and we ignore a remainder
	int	lastSample = numSamplesToLast * sampleRate;
	int nextSample = lastSample + sampleRate;
	return nextSample;
}

void ge2Wrapper::killScene()
{
	DtExt_CleanUp();

	saveFrame = -1;

	if ( shapeInfo )
	{
		delete [] shapeInfo;
		shapeInfo = NULL;
	}

	if ( shaders )
	{
		delete [] shaders;
		shaders = NULL;
	}
}

int ge2Wrapper::writeScene()
{
	bool	wroteGOF = false,
			wroteGMF = false,
			wroteLGT = false,
			wroteCAM = false,
			wroteGSF = false,
			wroteGAF = false;

	int		fileStatus;

	MString	path;

	writer.clearFile();

	writer.setUseTabs( useTabs );

	writer.setPrecision( precision );

	if ( outputGeometry )
	{
		fileStatus = writeGOF();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteGOF = true;
		}

		fileStatus = writeGMF();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteGMF = true;
		}
	}

	if ( outputLights )
	{
		fileStatus = writeLGT();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteLGT = true;
		}
	}

	if ( outputCamera )
	{
		fileStatus = writeCAM();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteCAM = true;
		}
	}

	/*  GSF isn't ready yet	
	if ( exportSkeletons )
	{
		fileStatus = writeGSF();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteGSF = true;
		}
	}
*/
	
	if ( enableAnim )
	{
		fileStatus = writeGAF();
		if ( fileStatus == GE_WRITE_OK )
		{
			wroteGAF = true;
		}
	}

	// Now create .grp file with all of bool's in mind...

	if ( !(wroteGOF || wroteGMF || wroteLGT || wroteCAM || wroteGAF) )
		return GE_WRITE_NO_DATA;

	path = pathName + "/" + baseFileName + ".grp";
	writer.setFile( path );
	
	outputHeader( writer );
	if ( wroteGOF )
		outputIncludeFile( writer, "gof" );
	if ( wroteGMF )
		outputIncludeFile( writer, "gmf" );
	if ( wroteGSF )
		outputIncludeFile( writer, "gsf" );
	if ( wroteLGT )
		outputIncludeFile( writer, "lgt" );
	if ( wroteCAM )
		outputIncludeFile( writer, "cam" );
	// GAF contains this GRP file...

	writer.writeFile();

	// if user has specified a script to run, do it now...
	if ( 0 < userScript.length() )
	{
		MString cmd = userScript;
		if ( scriptAppendFileName )
		{
			// forward slash works on both dos and unix...
			MString separator = "/";

			if ( wroteGAF )
			{
				cmd += " " + pathName + separator + baseFileName + ".gaf";
			}
			else
			{
				cmd += " " + pathName + separator + baseFileName + ".grp";
			}
		}
		
//		cerr << "would execute: " << cmd << endl;
		system( cmd.asChar() );
	}
	
	return GE_WRITE_OK;
}

void ge2Wrapper::outputIncludeFile( MTextFileWriter &fileWriter, char * ext )
{
	MString path;

	path = baseFileName + "." + ext;

	fileWriter.print( " include \"%s\";\n", path.asChar() );
}

int ge2Wrapper::outputBodyToFile( const int currShape )
{
	int		numVertices,
			numEdges,
			numGroups,
			numFaces,
			numVerticesInFace,
			numVertexNormals,
			numPolygonNormals,
//			numColors,
			numStrings,
			numUVs;

	int		polygonNormalListCounter,
			polygonNormalIndex;

	int		currString,			
			currVertex,
			currEdge,
			currGroup,
			currFace,
			currNormal,
			currUV,
//			currColor,			
			outputCounter,
			counter,
			i;

	bool	smoothShaded,
			wroteOne,
			wroteComment;

	char	shapeName[256],
			*materialName;

	DtVec3f *vertices,
			*normals,
			normal,
			tmpNormal;

    DtVec2f *uv;
//	DtRGBA	*color;

//	float	r, g, b;

	long2	edge;	

	int     *polygonNormalList,
			*vArr, 
			*nArr, 
			*tArr;

	shapeGetShapeName( currShape, shapeName );

	numStrings = bodyName.length();
	for (currString = 0; currString < numStrings; currString++)
	{
		// FIXIT:
		// this means we already printed this one.
		// or (!) it's got the same name and it's on a different DAG path
		// check to see if it's an instanced one, if so, then we should
		// return. If not, better find a new (unique) name
		if (bodyName[currString] == shapeName)
		{
			return GE_WRITE_OK;
		}
	}

	// FIXIT
	// doing this here because don't want to output if it's empty (just a group??)
	DtShapeGetVertices( currShape, &numVertices, &vertices ); 
	if (!numVertices)
		return GE_WRITE_NO_DATA;
		
	writer.print( "body %s (\n", shapeName );
	writer.addTab();

	// VERTICES:
	writer.print( "vertices[] <\n" );
	writer.addTab();
	for (currVertex = 0; currVertex < numVertices; currVertex++)
	{		
		writer.print( "(coord %10.xf %10.xf %10.xf;)", 
							vertices[currVertex].vec[0], 
							vertices[currVertex].vec[1], 
							vertices[currVertex].vec[2],
							currVertex );
		if ( writeComments )
			writer.print( "  # vertex %d", currVertex );

		writer.print( "\n" );
	}
	writer.removeTab();
	writer.print( ">\n");

	// SEGMENTS:
	// This function gets the number of edges from the API method 
	// MFnMesh::numEdges... This will have to be modified to handle
	// tesselated nurbs objects (which won't respond to MFnMesh calls)
	shapeGetNumEdges( currShape, numEdges );
	if ( 0 < numEdges )
	{
		writer.print( "segments[] <\n" );
		writer.addTab();
		for ( currEdge = 0; currEdge < numEdges; currEdge++ )
		{		
			// This too uses an MFnMesh method...
			shapeGetEdge( currShape, currEdge, edge );
			writer.print( "(vertex-indices[] < %d; %d;>)",
					edge[0], edge[1] );

			if ( writeComments )
				writer.print( "  # segment %d", currEdge );

			writer.print( "\n" );
		}
		writer.removeTab();
		writer.print( ">\n" );
	}

	// FACES:
	counter = 0;
	numGroups = DtGroupGetCount( currShape );
	assert(numGroups > 0); // why wouldn't this be!!
	MIntArray faceArray;

	writer.print( "faces[] <\n" );
	writer.addTab();

	smoothShaded = !DtShapeIsFlatShaded( currShape );

	for ( currGroup = 0; currGroup < numGroups; currGroup++ )
	{
// not only does this call get the number of polys (faces)
// in the given shape and group, it also sets up tables for that shape and 
// group so that the later DtPolygonGetIndices call can access the info
// quickly and easily
		DtPolygonGetCount( currShape, currGroup, &numFaces);

		DtShapeGetPolygonNormalIdx( currShape, currGroup, &numPolygonNormals, &polygonNormalList );
		polygonNormalListCounter = 0;
		polygonNormalIndex = 0; // A default value

		for ( currFace = 0; currFace < numFaces; currFace++ )
		{
			for ( i = polygonNormalListCounter; i < numPolygonNormals; i++ )
			{
				if ( polygonNormalList[i] == -1 ) // We're at the end of a face.
				{
					if ( 0 != i ) // This should only happen if there's a -1 at the start
					// of the polygonNormalList, which shouldn't happen (but we check anyway)
					{
						polygonNormalIndex = polygonNormalList[i-1];
						polygonNormalListCounter = i+1;
						break;
					}
				}
			}

			faceArray.append( counter );
			counter++;

			wroteComment = false;
			wroteOne = false;
			writer.print( "(" );

			// Normal:						

			if ( !smoothShaded && outputNormals )
			{
				wroteOne = true;
				DtShapeGetPolygonNormal( currShape, polygonNormalIndex, &normal );
				for ( i = 0; i < 3; i++ )
				{				
					if ( oppositeNormals )
						tmpNormal.vec[i] = -normal.vec[i];
					else
						tmpNormal.vec[i] = normal.vec[i];
				}

				writer.print( "normal %.xf %.xf %.xf;",
							tmpNormal.vec[0], tmpNormal.vec[1], tmpNormal.vec[2] );
				if ( writeComments )
				{
					writer.print( "  # face %d", counter);
					wroteComment = true;
				}
			}
			
			DtPolygonGetIndices(currFace, &numVerticesInFace, &vArr, &nArr, &tArr);

			// vertex-indices:
			if ( vArr )
			{			
				assert( numVerticesInFace > 0 ); // why would this be 0?
				if ( wroteOne )
				{
					writer.print( "\n" );
					writer.print( " vertex-indices[] <" );
				}
				else
				{
					wroteOne = true;
					writer.print( "vertex-indices[] <" );
				}

				outputCounter = 0;
				writer.addTab();
				
				if ( reverseWinding )
				{
					for ( currVertex = 0; currVertex < numVerticesInFace; currVertex++ )
					{
						writer.print( " %d;", vArr[currVertex] );
						if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
						{
							writer.print( "\n" );
							outputCounter = 0;
						}
					}
				}
				else {
					writer.print( " %d;", vArr[0] );
					for ( currVertex = numVerticesInFace-1; currVertex > 0; currVertex-- )
					{
						writer.print( " %d;", vArr[currVertex] );
						if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
						{
							writer.print( "\n" );
							outputCounter = 0;
						}
					}
				}
				writer.print( ">" );
				if ( writeComments && !wroteComment )
				{
					writer.print( "  # face %d", counter );
					wroteComment = true;
				}
				writer.removeTab();
			}

			if ( outputNormals && smoothShaded && nArr )
			{
				writer.print( "\n" );
				// vertex-normal-indices				
				writer.print( " vertex-normal-indices[] <" );
				writer.addTab();
				outputCounter = 0;
				for ( currVertex = 0; currVertex < numVerticesInFace; currVertex++ )
				{
					writer.print( " %d;", nArr[currVertex] );
					if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
					{
						writer.print( "\n" );
						outputCounter = 0;
					}
				}
				writer.print( ">" );
				if ( writeComments && !wroteComment )
				{
					writer.print( "  # face %d", counter );
					wroteComment = true;
				}
				writer.removeTab();
			}
			
			if ( outputTextures && tArr )
			{
				writer.print( "\n" );
				// vertex-texture-indices
				writer.print( " uv-indices[] <" );
				writer.addTab();
				outputCounter = 0;
				for ( currVertex = 0; currVertex < numVerticesInFace; currVertex++ )
				{
					writer.print( " %d;", tArr[currVertex] );
					if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
					{
						writer.print( "\n" );
						outputCounter = 0;
					}
				}
				writer.print( ">" );
				if ( writeComments && !wroteComment )
				{
					writer.print( "  # face %d", counter );
					wroteComment = true;
				}
				writer.removeTab();
			}
			writer.print( ")\n" ); // End of face
		}
		faceArray.append( -1 );
	}
	writer.removeTab();
	writer.print( ">\n" ); // end of all faces

	// normals
	if ( outputNormals && smoothShaded )
	{
		DtShapeGetNormals( currShape, &numVertexNormals, &normals );
		if ( numVertexNormals > 0 )
		{
			writer.print( "normals[] <\n" );
			writer.addTab();
			for ( currNormal = 0; currNormal < numVertexNormals; currNormal++ )
			{
				writer.print( "%6.xf %6.xf %6.xf;",
								normals[currNormal].vec[0], 
								normals[currNormal].vec[1],
								normals[currNormal].vec[2] );

				if ( writeComments )
					writer.print( "  # normal %d", currNormal );
				writer.print( "\n" );
			}
			writer.removeTab();
			writer.print( ">\n" );
		}
	}

	// uv-array
	if ( outputTextures )
	{
		DtShapeGetTextureVertices( currShape, &numUVs, &uv );
		if (numUVs > 0)
		{
			writer.print( "uv-array[] <\n" );
			writer.addTab();
			for ( currUV = 0; currUV < numUVs; currUV++ )
			{
				writer.print( "(uv[] <%.xf; %.xf;>)",
					uv[currUV].vec[0], uv[currUV].vec[1] );
				if ( writeComments )
					writer.print( "  # uv %d", currUV );
				writer.print( "\n" );
			}
			writer.removeTab();
			writer.print( ">\n" );
		}
	}

/*
	// vertex-colors	
	DtShapeGetVerticesColor( currShape, &numColors, &color );
	if (numColors > 0)
	{
		writer.print( "vertex-color[] <\n" );
		writer.addTab();
		for ( currColor = 0; currColor < numColors; currColor++ )
		{
			r = color[currColor].r / 256;
			g = color[currColor].g / 256;
			b = color[currColor].b / 256;
			// Also have access to Alpha here
			writer.print( "%6.xf %6.xf %6.xf;", r, g, b );
			if ( writeComments )
				writer.print( "  # cpv %d", currColor );
			writer.print( "\n" );
		}
		writer.removeTab();
		writer.print( ">\n" );
	}
*/

	// face-parts:
	numGroups = DtGroupGetCount( currShape );
	assert( numGroups > 0 ); // why wouldn't this be!!
	if ( numGroups > 1 )
	{
		int faceArrayMarker = 0, 
			arrayIndex;

		writer.print( "face-parts[] <\n" );
		writer.addTab();
		for ( currGroup = 0; currGroup < numGroups; currGroup++ )
		{	
			DtMtlGetName( currShape, currGroup, &materialName);
			writer.print( "(name \"%s\";\n", materialName );
			writer.print( " material \"%s\";\n", materialName );
			writer.print( " face-indices[] <" );
			writer.addTab();
			outputCounter = 0;
			for ( arrayIndex = faceArrayMarker; faceArray[arrayIndex] != -1; arrayIndex++ )
			{
				writer.print( " %d;", faceArray[arrayIndex] );
				if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
				{
					writer.print( "\n" );
					outputCounter = 0;
				}
			}
			faceArrayMarker = arrayIndex + 1;
			writer.print( " >)\n" );
			writer.removeTab();
		}

		writer.print( ">\n" );
		writer.removeTab();
	}
	else // there's only one group -- assign it to the whole body
	{
		// following three lines should work, but Nichimen's xwriter doesn't
		// handle it. Instead, treat all faces as one face-part
#if 0
		DtMtlGetName( currShape, 0, &materialName);
		assert(materialName);
		writer.print( "material \"%s\";\n", materialName);
#else
		int faceArrayMarker = 0, 
			arrayIndex;

		writer.print( "face-parts[] <\n" );
		writer.addTab();
		for ( currGroup = 0; currGroup < numGroups; currGroup++ )
		{	
			DtMtlGetName( currShape, currGroup, &materialName);
			writer.print( "(name \"%s\";\n", materialName );
			// FIXIT: named twice?
			writer.print( " material \"%s\";\n", materialName );
			writer.print( " face-indices[] <" );
			writer.addTab();
			outputCounter = 0;
			for ( arrayIndex = faceArrayMarker; faceArray[arrayIndex] != -1; arrayIndex++ )
			{
				writer.print( " %d;", faceArray[arrayIndex] );
				if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
				{
					writer.print( "\n" );
					outputCounter = 0;
				}
			}
			faceArrayMarker = arrayIndex + 1;
			writer.print( " >)\n" );
			writer.removeTab();		
		}
		
		writer.removeTab();
		writer.print( ">\n" );
#endif
	}

	// absolute-displacements
	if ( enableAnim && animVertices )
	{
		outputDisplacement( currShape );
	}
	
	writer.removeTab();
	writer.print( ")\n" ); // end of this body

	return GE_WRITE_OK;
}

void ge2Wrapper::outputDisplacement( int shapeID )
{
	int		keyIndex,
			localKeyFrame,
			dtKeyFrame,
			localBaseFrame,
			localBaseFrameIndex,
			numKeys,
			currVertex,
			numVertices;

	bool	wroteVertexDisplacement,
			wroteDisplacementLine,
			writeCurrVertex;

	char	shapeName[256],
			displacementName[256];

	MFloatVector	thisVertex,
					baseVertex;

	shapeGetShapeName( shapeID, shapeName );

	numKeys = shapeInfo[shapeID].keyFrames.length();

	localBaseFrame = frameConvertDtToLocal( saveFrame );
	localBaseFrameIndex = -1;
 
	wroteDisplacementLine = false;

	// Don't know if we have entries for each output frame,
	// or just the key times at this point, depending on how
	// we're sampling the animation
	for ( keyIndex = 0; keyIndex < numKeys; keyIndex++ )
	{
		if(shapeInfo[shapeID].keyFrames[keyIndex] == localBaseFrame)
		{
			localBaseFrameIndex = keyIndex;
			break;
		}
	}

	for ( keyIndex = 0; keyIndex < numKeys; keyIndex++ )
	{
		wroteVertexDisplacement = false;		

		localKeyFrame = shapeInfo[shapeID].keyFrames[keyIndex];
		dtKeyFrame = frameConvertLocalToDt( localKeyFrame );
		// don't output this as a displacement: it's the base
		if ( localBaseFrame == localKeyFrame )
			continue;

		numVertices = shapeInfo[shapeID].frameInfo[keyIndex].numVertices;

		for ( currVertex = 0; currVertex < numVertices; currVertex++ )
		{
			writeCurrVertex = false;
			thisVertex = shapeInfo[shapeID].frameInfo[keyIndex].vertexPosition[currVertex];
			if ( kVDAbsolute == vertexDisplacement )
				writeCurrVertex = true;
			else // Must be relative -- check if it's different from base
			{
				// If we're lucky, the base frame was at a key time, we saved the
				// values here somewhere, and we can conveniently check if it's
				// the same as the base and then skip it, if not we just write it anyway.
				if(localBaseFrameIndex >= 0){		
					baseVertex = shapeInfo[shapeID].frameInfo[localBaseFrameIndex].vertexPosition[currVertex];
	
					// This isEquivalent function has an optional tolerance argument --
					// it defaults to 1.0e-5F last time i checked. If this doesn't suit your
					// needs then pass one in...
					if ( !baseVertex.isEquivalent( thisVertex ) )
						writeCurrVertex = true;
				}
				else {
					// the base frame didn't happen at a key time, so we don't
					// have the data on hand, just write it. It would be better
					// if the baseFrame were always saved, but that's a large
					// code change for just before beta2.
					writeCurrVertex = true;
				}
			}

			if ( writeCurrVertex )
			{
				if ( !wroteDisplacementLine )
				{
					if ( kVDAbsolute == vertexDisplacement )
						writer.print( "absolute-displacement[] <\n" );
					else
						writer.print( "linear-displacement[] <\n" );
					writer.addTab();
					wroteDisplacementLine = true;
				}

				if ( !wroteVertexDisplacement )
				{
					sprintf( displacementName, "%s_FRAME_%d", shapeName, dtKeyFrame );
					writer.print( "(name \"%s\";\n", displacementName );
					writer.print( " vertices[] <\n" );
					writer.addTab();
					wroteVertexDisplacement = true;
				}

				writer.print( "(vertex-index %d;\n", currVertex );
				writer.print( " coord %.xf %.xf %.xf;)\n", 
					thisVertex.x, thisVertex.y, thisVertex.z );
			}
		}

		if ( wroteVertexDisplacement )
		{
			writer.removeTab();
			writer.print( " > )\n" );
		}
	}
	
	if ( wroteDisplacementLine )
	{
		writer.removeTab();
		writer.print( ">\n" );
	}
}

int ge2Wrapper::outputGObjectToFile( int shapeIdx, bool topLevelCall )
{
	int		row,
			column;
	int		numVertices, // to determine if this has a body or not
			numChildren,
			childIdx,
			*children;

	char	*shapeTransformName,
			shapeShapeName[256];

	float	*mtx,
			mat[4][4];

	DtVec3f	boundingBox[2];

	if ( processedDtShape[shapeIdx] == trueString )
		return GE_WRITE_OK;

	processedDtShape.set( trueString, shapeIdx );

	DtShapeGetName( shapeIdx, &shapeTransformName );
	if ( topLevelCall )
	{
		writer.print( "gobject %s (\n", shapeTransformName );
		writer.addTab();
		writer.print( " " );
	}
	else
	{
		writer.print( "(" );
	}
	
	writer.print( "name \"%s\";\n", shapeTransformName );

	DtShapeGetMatrix( shapeIdx, &mtx );
	memcpy( mat, mtx, sizeof(float) * 16 );
	writer.print( " matrix[] <\n" );
	writer.addTab();
	for ( row = 0; row < 4; row++ )
	{
		for ( column = 0; column < 4; column++ )
		{
			writer.print( "%10.xf", mat[row][column] );
			if ( column != 4 - 1 )
				writer.print( " " );
		}
		if ( row != 4 - 1 )
			writer.print( ";\n" );
		else
			writer.print( "; >\n" );
	}
	writer.removeTab();

	getBoundingBox( shapeIdx, &boundingBox[0] );
	writer.print( " bounding-box[] <");
	writer.print( " %.xf %.xf %.xf; %.xf %.xf %.xf; >\n",
			boundingBox[0].vec[0], boundingBox[0].vec[1], boundingBox[0].vec[2],
			boundingBox[1].vec[0], boundingBox[1].vec[1], boundingBox[1].vec[2] );

	DtShapeGetVertexCount( shapeIdx, &numVertices );
	// Should we include points and lines??
	if (numVertices > 0) 
	{
		shapeGetShapeName( shapeIdx, shapeShapeName );
		writer.print( " body \"%s\";\n", shapeShapeName );
	}

	if ( kTRANSFORMALL == DtExt_outputTransforms() )
	{
		DtShapeGetChildren( shapeIdx, &numChildren, &children );
		if ( (numChildren > 0) && (children != 0) )
		{		
			writer.print( "children[] <\n" );
			writer.addTab();
			for ( childIdx = 0; childIdx < numChildren; childIdx++ )
			{
				outputGObjectToFile( children[childIdx], false );
			}
			writer.removeTab();
			free( children ); // this sounds a little new-agey and peace-loving,
			// so don't get me wrong...

			writer.print( ">\n" ); // end of children
		}
	}

	if ( topLevelCall )
	{
		writer.removeTab();
	}
	
	writer.print( ")\n" );		
	// the end of this gobject

	return GE_WRITE_OK;
}

/*
int ge2Wrapper::outputBoneToFile( MFnIkJoint &joint )
{
	MStatus	stat;
	MPlug	plug;

	MFnIkJoint	child;

	MTransformationMatrix::RotationOrder rotOrder;
	double	rot[3];
	char	rotationOrder[4];

	bool	limitEnable;

	float	minRotXLimit,
			minRotYLimit,
			minRotZLimit,
			maxRotXLimit,
			maxRotYLimit,
			maxRotZLimit,
			minScaleLimit,
			maxScaleLimit;

	const float noMinLimit = -999999.99;
	const float noMaxLimit = 999999.99;

	float	boneLength;
	writer.print( "    (name \"%s\";\n", joint.name().asChar() );

	stat = joint.getRotation( rot, rotOrder );
	
	switch ( rotOrder )
	{
	case MTransformationMatrix::kXYZ:
		strcpy( rotationOrder, "xyz" );
		break;
	case MTransformationMatrix::kZXY:
		strcpy( rotationOrder, "zxy" );
		break;
	case MTransformationMatrix::kXZY:
		strcpy( rotationOrder, "xzy" );
		break;
	case MTransformationMatrix::kYXZ:
		strcpy( rotationOrder, "yxz" );
		break;
	case MTransformationMatrix::kZYX:
		strcpy( rotationOrder, "zyx" );
		break;
	default:
		// Should have been one of those!!
		assert( 0 ); 
		break;
	}

	writer.print( "     rotation-order \"%s\";\n", rotationOrder );

	// Get min rotation-limit values
	// Should get "dofMask" plug here to check -- if that dof is disabled,
	// get current rot and set limits to that...
	plug = joint.findPlug( "minRotRotXLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "minRotRotXLimit", &stat );
		stat = plug.getValue( minRotXLimit );
	}
	else
	{
		minRotXLimit = noMinLimit;  // FIXIT a defined val? check with DaveA at Nichimen
	}

	plug = joint.findPlug( "minRotRotYLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "minRotRotYLimit", &stat );
		stat = plug.getValue( minRotYLimit );
	}
	else
	{
		minRotYLimit = noMinLimit;  // FIXIT
	}

	plug = joint.findPlug( "minRotRotZLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "minRotRotZLimit", &stat );
		stat = plug.getValue( minRotZLimit );
	}
	else
	{
		minRotZLimit = noMinLimit;  // FIXIT
	}

	// Get max rotation-limit values
	plug = joint.findPlug( "maxRotRotXLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "maxRotRotXLimit", &stat );
		stat = plug.getValue( maxRotXLimit );
	}
	else
	{
		maxRotXLimit = noMaxLimit;  // FIXIT a defined val? check with DaveA at Nichimen
	}

	plug = joint.findPlug( "maxRotRotYLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "maxRotRotYLimit", &stat );
		stat = plug.getValue( maxRotYLimit );
	}
	else
	{
		maxRotYLimit = noMaxLimit;  // FIXIT
	}

	plug = joint.findPlug( "maxRotRotZLimitEnable", &stat );
	stat = plug.getValue( limitEnable );
	if ( true == limitEnable )
	{
		plug = joint.findPlug( "maxRotRotZLimit", &stat );
		stat = plug.getValue( maxRotZLimit );
	}
	else
	{
		maxRotZLimit = noMaxLimit;  // FIXIT
	}

	// scale -- they only use one value?!
	minScaleLimit = noMinLimit;
	maxScaleLimit = noMaxLimit;

	writer.print( "     dof-limit (\n" );
	writer.print( "       min %.6f %.6f %.6f %.6f;\n",
		minRotXLimit, minRotYLimit, minRotZLimit, minScaleLimit );
	writer.print( "       max %.6f %.6f %.6f %.6f;)\n",
		maxRotXLimit, maxRotYLimit, maxRotZLimit, maxScaleLimit );
	
	// Length -- tracked in Maya?
	boneLength = 1.0;
	writer.print( "     length %.6f;\n", boneLength );

	writer.print( "     axis-orientation (\n" );
	writer.print( "       rotate-axis TRUE;\n" ); // FIXIT: locked or not?
	writer.print( "       axis-orientation 0.0 0.0 0.0;)\n" ); // What does this mean?

	//writer.print( "     locked-to %s;\n", lockedToBone );
	writer.print( "     parent-joint \"%s\";\n", joint.name().asChar() );
	// What about multiple bones stemmed from this root??
	//writer.print( "     child-joint %s;\n", child.name().asChar() );

	writer.print( "     attached-objects[] <\n" );
	writer.print( "     >\n" );
	// go through dep. graph downstream, find any kSkins?
	// and output. Don't know if GE expects GObjects (transforms) 
	// or bodies (shapes) -- dep graph looks like it tracks shape,
	// but i'm not sure

	writer.print( "     hard-parts[] <\n" );
	writer.print( "     >\n" );
	writer.print( "     soft-parts[] <\n" );
	writer.print( "     >\n" );

	writer.print( "     softness-function\":sinusoid\";\n" );
	writer.print( "     softness-exponent 0.6 0.6 0.6;\n" );
	writer.print( "     skin-displacements[] <\n" );
	writer.print( "     >\n" );

	writer.print( "   )\n" );

	return 1;
}
*/
int ge2Wrapper::writeGAF()
{
	int		currDtFrame,
			currLocalFrame,
			currShape,
			sceneKey,
			numKeys,
			numLights,
			numAnimatedVertices,
			row,
			column,
			outputCounter;

	float	*matrix,
			mtx[4][4],
			displacementFactor;

	bool	wrotePobjectsLine,
			wroteFrame;

	char	*shapeName,
			shapeNodeName[128];

	char	visible[8],
			displacementName[256];

	MIntArray	sceneKeyFrames;

	MString path,
			templatePath,
			include;

	// Do i really want to perform this check??
	// Maybe the gaf file is still desired
	// if so, comment out these two compares
	if ( DtFrameGetStart() == DtFrameGetEnd() )
		return GE_WRITE_NO_DATA;
	if ( 1 >= DtFrameGetCount() )
		return GE_WRITE_NO_DATA;

	path = pathName + "/" + baseFileName + ".gaf";
	writer.setFile( path );

	outputHeader( writer );

	writer.print( " include \"%s\";\n\n", gafTemplate );

	templatePath = pathName + "/" + gafTemplate;
	if ( !fileExists( templatePath ) )
	{
		MTextFileWriter fileWriter;
		fileWriter.setFile( templatePath );
		outputHeader( fileWriter );
		fileWriter.print( "\n" );
		outputGAFTemplates( fileWriter );
		fileWriter.writeFile();
	}

	outputIncludeFile( writer, "grp" );
	if ( useTabs )
		writer.print( "\n" );
	
	// animation template
	writer.print( "animation %s (\n", baseFileName.asChar() ); // Don't really know what name
	writer.addTab();
	writer.print( "number-of-frames %d;\n", numFrames );
//	writer.print( "displacement-base-name %s", ??? );
	writer.print( "frames[] <\n" );
	writer.addTab();

	for ( currDtFrame = DtFrameGetStart(); currDtFrame <= DtFrameGetEnd(); currDtFrame += DtFrameGetBy() )
	{
		DtFrameSet( currDtFrame );
		currLocalFrame = frameConvertDtToLocal( currDtFrame );

		wrotePobjectsLine = false;
		wroteFrame = false;
		

		for ( currShape = 0; currShape < numShapes; currShape++ )
		{
			if ( shapeInfo[currShape].isKeyFrame( currLocalFrame ) )
			{
				addElement( sceneKeyFrames, currDtFrame );

				if ( !wroteFrame )
				{
					writer.print( "(frame-number %d;\n", currDtFrame );
					wroteFrame = true;
				}

				if ( !wrotePobjectsLine )
				{
					writer.print( " pobjects[] <\n" );
					writer.addTab();
					wrotePobjectsLine = true;
				}

				DtShapeGetName( currShape, &shapeName );
				writer.print( "(name \"%s\";\n", shapeName );
			
				// FIXIT: get visibility status from Transform node...
				// Find animCurve which channels visibility. Get value at this frame...
				strcpy( visible, "TRUE" );
				writer.print( " visible %s;\n", visible );

				if ( animTransforms )
				{
					DtShapeGetMatrix( currShape, &matrix );
					memcpy( mtx, matrix, sizeof(float) * 16 );
					writer.print( " matrix[] <\n" );
					writer.addTab();
					for ( row = 0; row < 4; row++ )
					{
						for ( column = 0; column < 4; column++ )
						{
							writer.print( "%10.xf", mtx[row][column] );
							if ( column != 4 - 1 )
								writer.print( " " );
						}
						writer.print( ";\n" );
					}
					writer.removeTab();
					writer.print( " >\n" );

					writer.print( " rst[] <\n" );
					writer.addTab();
					writer.print( "%.xf %.xf %.xf;\n", 
						shapeInfo[currShape].frameInfo[currLocalFrame].rotation.x,
						shapeInfo[currShape].frameInfo[currLocalFrame].rotation.y,
						shapeInfo[currShape].frameInfo[currLocalFrame].rotation.z );
					writer.print( "%.xf %.xf %.xf;\n", 
						shapeInfo[currShape].frameInfo[currLocalFrame].scale.x,
						shapeInfo[currShape].frameInfo[currLocalFrame].scale.y,
						shapeInfo[currShape].frameInfo[currLocalFrame].scale.z );
					writer.print( "%.xf %.xf %.xf; >\n", 
						shapeInfo[currShape].frameInfo[currLocalFrame].translation.x,
						shapeInfo[currShape].frameInfo[currLocalFrame].translation.y,
						shapeInfo[currShape].frameInfo[currLocalFrame].translation.z );
					writer.removeTab();
				}

				if ( animVertices )
				{
					numAnimatedVertices = shapeInfo[currShape].frameInfo[currLocalFrame].vertexPosition.length() > 0;

					if ( numAnimatedVertices > 0 )
					{
						shapeGetShapeName( currShape, shapeNodeName );
						writer.print( " displacements[] <\n" );
						writer.addTab();
						displacementFactor = (float) 1.0;
						sprintf( displacementName, "%s_FRAME_%d", shapeNodeName, currDtFrame );
						writer.print( "(name \"%s\";\n", displacementName );
						writer.print( " factor %.xf;)>\n", displacementFactor );
						writer.removeTab();
					}
				}

				writer.print( ")\n" );
			}
		} // End of curr shape iteration

		if ( wrotePobjectsLine )
		{
			writer.removeTab();
			writer.print( " >\n" ); // end of pobjects[] for this frame
		}

//		writer.print( "     skeletons[] <\n" );
//		writer.print( "     >\n" );

		// The DtFrameSet sets lights and cameras to be in the right spot...

		if ( animLights )
		{
			if ( !wroteFrame )
			{
				writer.print( "(frame-number %d;\n", currDtFrame );
				wroteFrame = true;
			}

			// may want to keyframe lights too...
			// this can be done similarly to the way
			// shapes are keyed. Will be a fair chunk of code to add
			addElement( sceneKeyFrames, currDtFrame );

			DtLightGetCount( &numLights );
			if ( numLights > 0 )
			{
				writer.print( " lights (\n" );
				outputLightsToFile();
				writer.print( " )\n" );
			}
		}

		if ( animCamera )
		{
			if ( !wroteFrame )
			{
				writer.print( "(frame-number %d;\n", currDtFrame );
				wroteFrame = true;
			}

			// may want to keyframe the camera too...
			// this can be done similarly to the way
			// shapes are keyed. Not too tough to do...
			addElement( sceneKeyFrames, currDtFrame );

			int	cameraForOutput = getCameraForOutput();
			if ( -1 != cameraForOutput )
			{
				writer.print( " camera (\n" );
				outputCameraToFile( cameraForOutput );
				writer.print( " )\n" );
			}
		}

		if ( wroteFrame )
		{
			writer.print( ")" );
			if ( writeComments )
				writer.print( " # end of animation-frame %d", currDtFrame );

			writer.print( "\n" );
		}
	}
	
	writer.removeTab();
	writer.print( ">\n" ); // end of frames[]

	numKeys = sceneKeyFrames.length();

	if ( 0 < numKeys )
	{
		outputCounter = 0;
		writer.print( "keyframe-indices[] < " );
		writer.addTab();
		
		for ( sceneKey = 0; sceneKey < numKeys; sceneKey++ )
		{
			writer.print( "%d; ", sceneKeyFrames[sceneKey] );
			if ( useTabs && (++outputCounter > GE2_MAX_INDICES_IN_LINE) )
			{
				writer.print( "\n" );
				outputCounter = 0;
			}
		}

		writer.removeTab();
		writer.print( " >\n" );
	}

	writer.removeTab();
	writer.print( ")\n" );

	writer.writeFile();

	DtFrameSet( saveFrame ); // So user gets it back like they left it

	return GE_WRITE_OK;
}

int ge2Wrapper::writeGOF()
{
	int		currShapeIdx;

//	int		status;

	MString path,
			templatePath;
	
	if ( numShapes > 0 )
	{
		// There must be a better way to do this
		processedDtShape.clear();
		// so we don't allocate new memory each time, only once
		processedDtShape.setSizeIncrement( numShapes * sizeof(falseString) );
		for ( currShapeIdx = 0; currShapeIdx < numShapes; currShapeIdx++ )
		{			
			processedDtShape.append( falseString );
		}
	}
	else 
	{
		// No shapes ?? 
		return GE_WRITE_NO_DATA;
	}

	path = pathName + "/" + baseFileName + ".gof";
	writer.setFile( path );

	outputHeader( writer );
	writer.print( " include \"%s\";\n", gofTemplate );

	templatePath = pathName + "/" + gofTemplate;
	if ( !fileExists( templatePath ) )
	{
		MTextFileWriter templateWriter;
		templateWriter.setFile( templatePath );
		outputHeader( templateWriter );
		templateWriter.print( "\n" );
		outputGOFTemplates( templateWriter );
		templateWriter.writeFile();
	}

	if ( useTabs )
		writer.print( "\n" );

	if ( writeComments )
	{
		writer.print( "# bodies:\n\n" );
	}

	for ( currShapeIdx = 0; currShapeIdx < numShapes; currShapeIdx++ )
	{
		outputBodyToFile( currShapeIdx );
	}

	writer.print( "\n" );

	if ( writeComments )
		writer.print( "# gobjects:\n" );

	for ( currShapeIdx = 0; currShapeIdx < numShapes; currShapeIdx++ )
	{
//		status = outputGObjectToFile( currShapeIdx, true );
		outputGObjectToFile( currShapeIdx, true );
	}

	writer.writeFile();

	return GE_WRITE_OK;
}

int ge2Wrapper::writeGMF()
{
	int		currMaterial;

	MString			path,
					templatePath;
					
	if ( 0 == numShaders )
	{
		// No materials 
		return GE_WRITE_NO_DATA;
	}

	path = pathName + "/" + baseFileName + ".gmf";
	writer.setFile( path );
	
	outputHeader( writer );
	writer.print( " include \"%s\";\n\n", gmfTemplate );

	templatePath = pathName + "/" + gmfTemplate;
	if ( !fileExists( templatePath ) )
	{
		MTextFileWriter fileWriter;
		fileWriter.setFile( templatePath );
		outputHeader( fileWriter );
		fileWriter.print( "\n" );
		outputGMFTemplates( fileWriter );
		fileWriter.writeFile();
	}

	for ( currMaterial = 0; currMaterial < numShaders; currMaterial++ )
	{
		shaders[currMaterial].output( writer );
	}

	writer.writeFile();
	return GE_WRITE_OK;
}

int ge2Wrapper::writeLGT()
{
	int		numLights;

	DtLightGetCount( &numLights );

	if ( 0 == numLights )
	{
		return GE_WRITE_NO_DATA;
	}

	MString	path;

	path = pathName + "/" + baseFileName + ".lgt";

	writer.setFile( path );
	
	outputHeader( writer );
	writer.print( "\n" );
	outputLGTTemplates( writer );

	writer.print( "light lights (\n" );	
	outputLightsToFile();
	writer.print( ")\n" );

	writer.writeFile();
	return GE_WRITE_OK;
}

int ge2Wrapper::outputLightsToFile()
{
	int		i,
			numLights,
			lightType,
			currType,
			lightTypeToCheck,
			lightOn,
			currLight,
			lightCount,
			numLightsOfType[GE_NUM_LIGHT_TYPES];

	float   position[3],
			color[3],
			direction[3],
			aim[3],
			intensity,
			cutOffAngle,
			distance,
			radius;

	bool	useDecay;

	char	lightName[256];

	DtLightGetCount( &numLights );

	for ( i = 0; i < GE_NUM_LIGHT_TYPES; i++ )
	{
		numLightsOfType[i] = 0;
	}

	for ( currLight = 0; currLight < numLights; currLight++ )
	{
		DtLightGetType( currLight, &lightType );
		// use lightType-1 because DT_*_LIGHT starts at 1
		numLightsOfType[lightType-1]++;
	}

	lightCount = 0;
	for ( i = 0; i < GE_NUM_LIGHT_TYPES; i++ )
	{
		lightCount += numLightsOfType[i];
	}

	writer.addTab();

	for ( currType = 0; currType < GE_NUM_LIGHT_TYPES; currType++ )
	{
		if ( 0 == numLightsOfType[currType] )
		{
			continue; // there's none of this type... don't write it...
		}

		// currType+1 because DT_*_LIGHT starts at 1 (MDt.h)
		switch ( currType+1 )
		{
		case DT_AMBIENT_LIGHT:
			lightTypeToCheck = DT_AMBIENT_LIGHT;
			writer.print( "ambient-lights[] <\n" );
			break;
		case DT_DIRECTIONAL_LIGHT:
			lightTypeToCheck = DT_DIRECTIONAL_LIGHT;
			writer.print( "infinite-lights[] <\n" );
			break;
		case DT_POINT_LIGHT:
			lightTypeToCheck = DT_POINT_LIGHT;
			writer.print( "point-lights[] <\n" );
			break;
		case DT_SPOT_LIGHT:
			lightTypeToCheck = DT_SPOT_LIGHT;
			writer.print( "spot-lights[] <\n" );
			break;
		default: // Shouldn't get here, but in case we did:
			lightTypeToCheck = DT_AMBIENT_LIGHT;
			writer.print( "ambient-lights[] <\n" );			
			break;
		}

		writer.addTab();

		for ( currLight = 0; currLight < numLights; currLight++ )
		{			
			DtLightGetOn(currLight, &lightOn);
			DtLightGetType( currLight, &lightType );
			if ( (lightType == lightTypeToCheck) && (lightOn == 1) )
			{
				lightGetName( currLight, lightName );
				writer.print( "(name \"%s\";\n", lightName );

				// All the lights have 'brightness'
				DtLightGetIntensity( currLight, &intensity );
				writer.print( " brightness %.xf;\n", intensity );

				// They all have colors too
				DtLightGetColor( currLight, &color[0], &color[1], &color[2] );
				writer.print( " rgb %.xf %.xf %.xf;\n",
						color[0], color[1], color[2] );
				
				// Now for type-specific stuff:
				if ( lightType == DT_DIRECTIONAL_LIGHT )
				{
					DtLightGetDirection( currLight, 
							&direction[0], &direction[1], &direction[2] );
					writer.print( " direction %.xf %.xf %.xf;\n",
							direction[0], direction[1], direction[2] );
				}
				else if ( lightType == DT_POINT_LIGHT )
				{
					DtLightGetPosition( currLight, 
							&position[0], &position[1], &position[2] );
					writer.print( " position %.xf %.xf %.xf;\n",
							position[0], position[1], position[2] );
					writer.print( " use-attenuation " );
					writer.print( "FALSE;\n" );
				}
				else if ( lightType == DT_SPOT_LIGHT )
				{
					// Need to have this as the direction is used for the aim-point
					DtLightGetDirection( currLight, 
							&direction[0], &direction[1], &direction[2] );
					DtLightGetPosition( currLight, 
							&position[0], &position[1], &position[2] );
					writer.print( " position %.xf %.xf %.xf;\n",
							position[0], position[1], position[2] );

					lightGetDistanceToCenter( currLight, distance );

					for ( i = 0; i < 3; i++ )
					{
						aim[i] = position[i] + direction[i]*distance;
					}
					writer.print( " aim-point %.xf %.xf %.xf;\n",
											aim[0], aim[1], aim[2] );

					lightGetUseDecayRegions( currLight, useDecay );
					writer.print( " use-attenuation " );
					if ( useDecay )
						writer.print( "TRUE;\n" );
					else
						writer.print( "FALSE;\n" );

					DtLightGetCutOffAngle( currLight, &cutOffAngle );					
					// Dt documentation says cutOffAngle is from one edge to another
					// MFnSpotLight says this is just from 'direction' to edge
					// (MFnSpotLight doesn't really support direction, though!)
					radius = (float) (distance * tan( cutOffAngle ));
					writer.print( " radius %.xf;\n", radius );					
						
					cutOffAngle = (cutOffAngle * 180) / (float) PI;
					writer.print( " falloff-angle %.xf;\n", cutOffAngle );
				}
				
				writer.print( ")\n" ); // End of this light
			}

		}
		writer.removeTab();
		writer.print( ">\n" ); // End of this type
	}

	writer.removeTab();

	return GE_WRITE_OK;
}

int ge2Wrapper::getCameraForOutput()
{
	int		numCameras,
			currCamera,
			cameraForOutput = -1,
			cameraType;

	DtCameraGetCount( &numCameras );
	// No cameras:
	if ( 0 == numCameras )
	{
		return -1;
	}

	for ( currCamera = 0; currCamera < numCameras; currCamera++ )
	{
		DtCameraGetType( currCamera, &cameraType );
		// We'll take the last perspective camera
		if ( DT_PERSPECTIVE_CAMERA == cameraType )
		{
			cameraForOutput = currCamera;
		}
	}

	// No perspective cameras:
	if ( cameraForOutput == -1 )
	{
		if ( numCameras > 0 )
			cameraForOutput = 0; // Must exist, and must be an ortho view if we're here
	}

	return cameraForOutput;
}

int ge2Wrapper::writeCAM()
{
	MString	path;

	int cameraForOutput = getCameraForOutput();

	if ( -1 == cameraForOutput )
		return GE_WRITE_NO_DATA;
	
	path = pathName + "/" + baseFileName + ".cam";
	writer.setFile( path );
	
	outputHeader( writer );
	writer.print( "\n" );
	outputCAMTemplates( writer );
	writer.print( "\n" );

	// even though it might make more sense to use the camera's name here
	// this is how Nichimen does it...
	writer.print( "camera camera (\n" );
	outputCameraToFile( cameraForOutput );
	writer.print( ")\n" );

	writer.writeFile();
	
	return GE_WRITE_OK;
}

int ge2Wrapper::outputCameraToFile( int cameraIndex )
{
	float	hither,
			yon,
			position[3],
			interest[3],
			lookup[3],
			viewAngle;

	char	*cameraName;

	DtCameraGetName( cameraIndex, &cameraName );

	writer.addTab();
	DtCameraGetHeightAngle( cameraIndex, &viewAngle );
	cameraGetWidthAngle( cameraIndex, viewAngle );
//	viewAngle = (viewAngle * 180) / PI; // _NOT_ in rads
	writer.print( "view-angle %.xf;\n", viewAngle );

	DtCameraGetNearClip( cameraIndex, &hither );
	writer.print( "hither %.xf;\n", hither );
	DtCameraGetFarClip( cameraIndex, &yon );
	writer.print( "yon %.xf;\n", yon );

	DtCameraGetPosition( cameraIndex, &position[0], &position[1], &position[2] );
	writer.print( "location %.xf %.xf %.xf;\n",
						position[0], position[1], position[2] );
	DtCameraGetInterest( cameraIndex, &interest[0], &interest[1], &interest[2] );
	writer.print( "aim %.xf %.xf %.xf;\n",
						interest[0], interest[1], interest[2] );

	cameraGetUpDir( cameraIndex, 
				lookup[0], lookup[1], lookup[2] );
	writer.print( "lookup-vector %.xf %.xf %.xf;\n",
				lookup[0], lookup[1], lookup[2] );

	writer.removeTab();

	return GE_WRITE_OK;
}


int ge2Wrapper::writeGSF()
{
#ifdef MAYA1X
	return GE_WRITE_NO_DATA;
#else
	float		xPos, 
				yPos, 
				zPos;

	MString		path,
				templatePath;

	MPlug		plug;
	MPlugArray	plugArray;
	
	MStatus		status;
	MObject		joint;

	MDagPath	dagPath;
	
	MItDag::TraversalType traversalType = MItDag::kDepthFirst;

	MFn::Type	filter = MFn::kJoint;
	
	path = pathName + "/" + baseFileName + ".gsf";
	writer.setFile( path );
	
	outputHeader( writer );

	writer.print( " include \"%s\";\n\n", gsfTemplate );

	templatePath = pathName + "/" + gsfTemplate;
	if ( !fileExists( templatePath ) )
	{
		MTextFileWriter fileWriter;
		fileWriter.setFile( templatePath );
		outputHeader( fileWriter );
		fileWriter.print( "\n" );
		outputGSFTemplates( fileWriter );
		fileWriter.writeFile();
	}

	MItDag dagIter( traversalType, filter, &status );
	if ( MS::kSuccess != status )
	{
		cerr << "Iterator constructor problems!" << endl;
		return GE_WRITE_FILE_ERROR;
	}

	writer.print( " skeleton skeleton (\n" );
	writer.addTab();
	writer.print( "joints[] <\n" );
	writer.addTab();
	for ( ; !dagIter.isDone(); dagIter.next() )
    {
		status = dagIter.getPath( dagPath );
		if ( MS::kSuccess != status )
		{
			cerr << "DagPath construction problems in joint..." << endl;
			return GE_WRITE_FILE_ERROR;
		}

		joint = dagPath.node( &status );

		MFnIkJoint fnJoint( joint, &status );
		if ( MS::kSuccess != status )
		{
			cerr << "MFnIkJoint construction problems in joint..." << endl;
			return GE_WRITE_FILE_ERROR;
		}		

		writer.print( "(name \"%s\";\n", fnJoint.name().asChar() );
		plug = fnJoint.findPlug( "translateX", &status );
		status = plug.getValue( xPos );
		plug = fnJoint.findPlug( "translateY", &status );
		status = plug.getValue( yPos );
		plug = fnJoint.findPlug( "translateZ", &status );
		status = plug.getValue( zPos );
		writer.print( " coordinate %.xf %.xf %.xf;)\n",
			xPos, yPos, zPos );
	}
	writer.removeTab();
	writer.print( ">\n" ); // end of joints...

	writer.removeTab();

	// iterate again and do bones...
	MItDag dagIter2( traversalType, filter, &status );
	if ( MS::kSuccess != status )
	{
		cerr << "Iterator constructor problems in joint (second pass)..." << endl;
		return GE_WRITE_FILE_ERROR;
	}
	
	writer.print( "bones[] <\n" );
	writer.addTab();
	for ( ; !dagIter2.isDone(); dagIter2.next() )
	{
		status = dagIter2.getPath( dagPath );
		if ( MS::kSuccess != status )
		{
			cerr << "DagPath construction problems in joint (second pass)..." << endl;
			return GE_WRITE_FILE_ERROR;
		}

		joint = dagPath.node( &status );

		MFnIkJoint fnJoint( joint, &status );
		if ( MS::kSuccess != status )
		{
			cerr << "MFnIkJoint construction problems in joint (second pass)..." << endl;
			return GE_WRITE_FILE_ERROR;
		}

//		outputBoneToFile( fnJoint );
	}
	writer.removeTab();
	writer.print( ">\n" );
/*
	writer.print( "   bases[] <\n" );
	writer.print( "   >\n" );
	writer.print( "   skins[] <\n" );
	writer.print( "   >\n" );
	writer.print( "   base \"%s\";\n", baseName );
	writer.print( "   root-joint \"%s\";\n", rootJointName ); // can get this at least
	writer.print( "   rotation-order \"\"" );
	writer.print( "   translation-order \"\"" );
*/
	writer.print( " )\n" );
	writer.writeFile();

	return GE_WRITE_OK;
#endif
}

ge2Wrapper::MShapeInfo::MShapeInfo()
:	frameInfo		(NULL),
	index			(-1)
{
}

ge2Wrapper::MShapeInfo::~MShapeInfo()
{
	index = -1;
	keyFrames.clear();

	if ( frameInfo )
	{
		delete [] frameInfo;
		frameInfo = NULL;
	}
}

void ge2Wrapper::MShapeInfo::allocFrameInfo( int numFrames )
{
	if ( frameInfo )
	{
		delete [] frameInfo;
		frameInfo = NULL;
	}

	frameInfo = new MFrameInfo[numFrames];
}

bool ge2Wrapper::MShapeInfo::isKeyFrame( int frame )
{
	unsigned int currKey;

	for ( currKey = 0; currKey < keyFrames.length(); currKey++ )
	{
		if ( keyFrames[currKey] == frame ) 
			return true;
		if ( keyFrames[currKey] > frame )
			break;
	}

	return false;
}

void ge2Wrapper::MShapeInfo::addKeyFrame( int frame )
{
	addElement( keyFrames, frame );
}

void ge2Wrapper::MShapeInfo::addKeyFrames( MIntArray& keyArray )
{
	unsigned int currIndex; 

	for ( currIndex = 0; currIndex < keyArray.length(); currIndex++ )
		addKeyFrame( keyArray[currIndex] );
}

int	ge2Wrapper::MShapeInfo::getFirstKeyBetween( int start, int end )
{
	int		currIndex,
			numKeys,
			key;

	numKeys = keyFrames.length();

	for ( currIndex = 0; currIndex < numKeys; currIndex++ )
	{
		key = keyFrames[currIndex];
		if ( key <= start )
			continue;
		if ( key >= end )
			return -1;
		
		// that we got here means this is the first we've found
		return key;
	}

	return -1;
}

int ge2Wrapper::MShapeInfo::getLastKeyBetween( int start, int end )
{
	int		currIndex,
			key;

	for ( currIndex = keyFrames.length() - 1; currIndex >= 0; currIndex-- )
	{
		key = keyFrames[currIndex];
		if ( key >= end )
			continue;
		if ( key <= start )
			return -1;
		
		// that we got here means this is the first we've found
		return key;
	}

	return -1;
}

ge2Wrapper::MShapeInfo::MFrameInfo::MFrameInfo()
{
}

ge2Wrapper::MShapeInfo::MFrameInfo::~MFrameInfo()
{
}

// This function assumes that you are set at the given frame
void ge2Wrapper::MShapeInfo::fill( int frame, bool transforms, bool verticesBool )
{
	if ( verticesBool )
	{
		int			currVertex,
					numVertices;
	
		DtVec3f		*vertices;
		
		DtShapeGetVertices( index, &numVertices, &vertices ); 
		frameInfo[frame].numVertices = numVertices;

		if ( frameInfo[frame].numVertices > 2 )
		{		
			frameInfo[frame].vertexPosition.clear();
			frameInfo[frame].vertexPosition.setSizeIncrement( frameInfo[frame].numVertices );
			for ( currVertex = 0; currVertex < frameInfo[frame].numVertices; currVertex++ )
			{
				MFloatVector pos( vertices[currVertex].vec );
				frameInfo[frame].vertexPosition.append( pos );
			}
		}
	}

	if ( transforms )
	{
		float	x, y, z;

		DtShapeGetTranslation( index, &x, &y, &z );
		MFloatVector trans( x, y, z );
		frameInfo[frame].translation = trans;
		DtShapeGetRotation( index, &x, &y, &z );
		MFloatVector rot( x, y, z );
		frameInfo[frame].rotation = rot;
		DtShapeGetScale( index, &x, &y, &z );
		MFloatVector scale( x, y, z );
		frameInfo[frame].scale = scale;
	}
}

ge2Wrapper::MShader::MShader()
:	map				(NULL),
	N64Domain		(NULL),
	PSXDomain		(NULL),
	GLDomain		(NULL)
{
}

ge2Wrapper::MShader::~MShader()
{
	if ( map )
	{
		delete map;
		map = NULL;
	}

	if ( N64Domain )
	{
		delete N64Domain;
		N64Domain = NULL;
	}

	if ( PSXDomain )
	{
		delete PSXDomain;
		PSXDomain = NULL;
	}

	if ( GLDomain )
	{
		delete GLDomain;
		GLDomain = NULL;
	}
}

ge2Wrapper::MShader::MTextureMap::MTextureMap()
{
}

ge2Wrapper::MShader::MTextureMap::~MTextureMap()
{
}

ge2Wrapper::MShader::MN64::MN64()
:	useTextureMap	(kDomainBoolInvalid),
	faceControl		(kDomainFaceInvalid),
	omitFace		(kDomainBoolInvalid),
	normalSource	(kDomainNormalInvalid),
	preShade		(kDomainBoolInvalid),
	useLightOption	(kDomainBoolInvalid),
	blendColor		(kDomainBoolInvalid),
	useZBuffer		(kDomainBoolInvalid),
	useFog			(kDomainBoolInvalid),
	useReflectionMapping	(kDomainBoolInvalid)
{
}

ge2Wrapper::MShader::MN64::~MN64()
{
}

ge2Wrapper::MShader::MPSX::MPSX()
:	useTextureMap		(kDomainBoolInvalid),
	omitFace			(kDomainBoolInvalid),
	lightCalculation	(kDomainBoolInvalid),
	faceControl			(kDomainFaceInvalid),
	normalSource		(kDomainNormalInvalid),
	preShade			(kDomainBoolInvalid),
	subdiv				(kDomainBoolInvalid),
	activeSubdiv		(kDomainBoolInvalid),
	clip				(kDomainBoolInvalid),
	useMipmap			(kDomainBoolInvalid)
{
}

ge2Wrapper::MShader::MPSX::~MPSX()
{
}

ge2Wrapper::MShader::MGL::MGL()
:	normalSource		(kDomainNormalInvalid),
	colorByVertex		(kDomainBoolInvalid)
{
}

ge2Wrapper::MShader::MGL::~MGL()
{
}

void ge2Wrapper::MShader::getFaceControl( GEDomainFaceControl &domainFaceControl )
{
	int		stat,
			matID,
			currShape,
			currGroup,
			numShapes,
			numGroups;

	MObject shader;

	GEDomainFaceControl tempControl,
						saveControl = kDomainFaceInvalid;

	numShapes = DtShapeGetCount();

	for ( currShape = 0; currShape < numShapes; currShape++ )
	{
		numGroups = DtGroupGetCount( currShape );

		for ( currGroup = 0; currGroup < numGroups; currGroup++ )
		{
			stat = DtMtlGetID( currShape, currGroup, &matID );
			if ( (1 != stat) || ( matID != materialID ) )
				continue;
			
			// If we got here, then this shader as applied to the currShape
			// check for facing control...
			if ( DtShapeIsDoubleSided( currShape ) )
				tempControl = kDomainFaceBoth;
			else
			{
				if ( DtShapeIsOpposite( currShape ) )
					tempControl = kDomainFaceBack;
				else
					tempControl = kDomainFaceFront;
			}
			
			if ( kDomainFaceInvalid == saveControl )
				saveControl = tempControl;
			else
			{
				if ( saveControl != tempControl )
				{
					domainFaceControl = kDomainFaceInvalid;
					return;
				}
			}
		}
	}

	domainFaceControl = saveControl;
}

void ge2Wrapper::MShader::fill( int matID, ge2Wrapper& caller )
{
	int			stat,
				textureID;

	char		*matName,
				*textureName,
				*textureFileName;

	char		outTmpFile[256];
	char		cmd[512];

	// What will hold the texture image
	unsigned char *image;

	// use this variable later on to get values from MDt that we don't store 
	materialID = matID;

	DtMtlGetNameByID( matID, &matName );
	MString name( matName );
	materialName = name;

	if ( map )
	{
		delete map;
		map = NULL;
	}

	if ( caller.outputTextures )
	{
		DtTextureGetID( matID, &textureID );
		if ( -1 != textureID )
		{
			map = new MTextureMap;

			if ( !map )
			{
				cerr << "Memory allocation problems...leaving MShader::fill()" << endl;
				return;
			}

			DtTextureGetNameID( textureID, &textureName );
			DtTextureGetFileNameID( textureID, &textureFileName );
			DtTextureGetImageSizeByID( textureID, &map->width, 
					&map->height, &map->channels );
				
			// Check to see if this is a file texture and if the user
			// wants to just use this original file
			if ( ( strcmp( textureFileName, "" ) != 0 ) &&
				 caller.useOriginalFileTextures )
			{
				map->name = MString( textureFileName );
			} else
			{				
				DtTextureGetImageByID( textureID, &image);

				if ( NULL != image )
				{
					// This name is a path to the file
					// May want textures in one location; if so,
					// should modify this accordingly...
					map->name = caller.getPathName() + "/" + 
						caller.getBaseFileName() + "_" +
						textureName + "." + caller.texType;

					sprintf( outTmpFile, "%s/MDt_tmp%d.iff", 
						getenv("TMPDIR"), matID );

					IFFimageWriter iffWriter;
					MStatus Wstat;
					float zBuffer[1024];
												
					Wstat = iffWriter.open( outTmpFile );
					if ( Wstat != MS::kSuccess )
					{
						cerr << "error opening write file "
  							 << outTmpFile << endl;
						map->name = MString( "" );
					} else 
					{							
						iffWriter.setSize( map->width, map->height );
						iffWriter.setBytesPerChannel( 1 );
						iffWriter.setType( ILH_RGBA );

						Wstat = iffWriter.writeImage( (byte *) image, zBuffer, 0 );
						if ( Wstat != MS::kSuccess )
						{
							cerr << "error on writing " 
								<< iffWriter.errorString() << endl;
						}

						iffWriter.close();
							
						// convert to user-specified format
						sprintf(cmd, "imgcvt -t %s %s %s",
								caller.texType.asChar(), 
								outTmpFile,
								map->name.asChar() );
						system( cmd );

						// Now lets remove the tmp file
						unlink( outTmpFile );
					}
				}
				else
				{
					delete map;
					map = NULL;
				}
			}
		}
	}

	if ( caller.useDomainGL )
	{
		GLDomain = new MGL;
	}

	if ( caller.useDomainN64 )
	{
		N64Domain = new MN64;
	}

	if ( caller.useDomainPSX )
	{
		PSXDomain = new MPSX;
	}

	// now get all of the extras...
	MObject		shader;
	MStatus		status;
	MPlug		vPlug;
	bool		plugValueBool;
	short		plugValueInt;
//	MString		plugValueString;

	stat = DtExt_MtlGetShader( matID, shader );
	if ( stat != 1 )
	{
		cerr << "Problems getting shader in MShader::fill()" << endl;
		return;
	}
	MFnDependencyNode fnShader( shader, &status );

	if ( N64Domain )
	{
		vPlug = fnShader.findPlug( "ge2N64UseTextureMap", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->useTextureMap = kDomainBoolTrue;
			else
				N64Domain->useTextureMap = kDomainBoolFalse;
		}

		// getFaceControl actually checks the attributes of the shape -- uses
		// DtShapeIsDoubleSided, etc. Want to be able to specify different
		// face Control for different domains, however, so we use the dynamic 
		// shader attribute, faceControl
//		getFaceControl( N64Domain->faceControl );
		vPlug = fnShader.findPlug( "ge2N64FacingControl", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueInt );
			switch ( plugValueInt )
			{
			case 0:
				N64Domain->faceControl = kDomainFaceBoth;
				break;
			case 1:
				N64Domain->faceControl = kDomainFaceFront;
				break;
			case 2:
				N64Domain->faceControl = kDomainFaceBack;
				break;
			}
		}
		

		vPlug = fnShader.findPlug( "ge2N64OmitFace", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->omitFace = kDomainBoolTrue;
			else
				N64Domain->omitFace = kDomainBoolFalse;
		}

		// If you want normal source, can do it the same way as getFaceControl()

		vPlug = fnShader.findPlug( "ge2N64Preshade", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->preShade = kDomainBoolTrue;
			else
				N64Domain->preShade = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2N64UseLightOption", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->useLightOption = kDomainBoolTrue;
			else
				N64Domain->useLightOption = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2N64BlendColor", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->blendColor = kDomainBoolTrue;
			else
				N64Domain->blendColor = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2N64UseZBuffer", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->useZBuffer = kDomainBoolTrue;
			else
				N64Domain->useZBuffer = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2N64UseFog", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->useFog = kDomainBoolTrue;
			else
				N64Domain->useFog = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2N64UseReflectionMapping", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				N64Domain->useReflectionMapping = kDomainBoolTrue;
			else
				N64Domain->useReflectionMapping = kDomainBoolFalse;
		}
	}

	if ( PSXDomain )
	{
		vPlug = fnShader.findPlug( "ge2PsxUseTextureMap", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->useTextureMap = kDomainBoolTrue;
			else
				PSXDomain->useTextureMap = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxOmitFace", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->omitFace = kDomainBoolTrue;
			else
				PSXDomain->omitFace = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxLightCalculation", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->lightCalculation = kDomainBoolTrue;
			else
				PSXDomain->lightCalculation = kDomainBoolFalse;
		}

		// See notes on N64 for faceControl comments
//		getFaceControl( PSXDomain->faceControl );

		vPlug = fnShader.findPlug( "ge2PsxFacingControl", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueInt );
			switch ( plugValueInt )
			{
			case 0:
				PSXDomain->faceControl = kDomainFaceBoth;
				break;
			case 1:
				PSXDomain->faceControl = kDomainFaceFront;
				break;
			case 2:
				PSXDomain->faceControl = kDomainFaceBack;
				break;
			}
		}

		vPlug = fnShader.findPlug( "ge2PsxABE", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->abe = kDomainBoolTrue;
			else
				PSXDomain->abe = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxABR", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueInt );
			PSXDomain->abr = plugValueInt;
		}

		// If you want normal source, can do it the same way as getFaceControl()

		vPlug = fnShader.findPlug( "ge2PsxPreshade", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->preShade = kDomainBoolTrue;
			else
				PSXDomain->preShade = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxPerformSubdivision", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->subdiv = kDomainBoolTrue;
			else
				PSXDomain->subdiv = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxPerformActiveSubdivision", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->activeSubdiv = kDomainBoolTrue;
			else
				PSXDomain->activeSubdiv = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxClip", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->clip = kDomainBoolTrue;
			else
				PSXDomain->clip = kDomainBoolFalse;
		}

		vPlug = fnShader.findPlug( "ge2PsxUseMipmap", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				PSXDomain->useMipmap = kDomainBoolTrue;
			else
				PSXDomain->useMipmap = kDomainBoolFalse;
		}
	}

	if ( GLDomain )
	{
		// If you want normal source, can do it the same way as getFaceControl()

/*		vPlug = fnShader.findPlug( "ge2Gl_ShadeColorByVertex", &status );
		if ( MS::kSuccess == status )
		{
			vPlug.getValue( plugValueBool );
			if ( true == plugValueBool )
				GLDomain->colorByVertex = kDomainBoolTrue;
			else
				GLDomain->colorByVertex = kDomainBoolFalse;
		}
*/
	}
}

void ge2Wrapper::MShader::output( MTextFileWriter& writer )
{
	int				unused = 0;

	float			diffuseR, diffuseG, diffuseB;
	float			specR, specG, specB;
	float			specExp;
	float			emissiveR, emissiveG, emissiveB;
	float			transparency, opacity;
	float			ambientR, ambientG, ambientB;

	DtMtlGetAllClrbyID( materialID, unused, 
						&ambientR, &ambientG, &ambientB,
						&diffuseR, &diffuseG, &diffuseB,
						&specR, &specG, &specB,
						&emissiveR, &emissiveG, &emissiveB,
						&specExp, &transparency );

	opacity = 1 - transparency;

	writer.print( "material %s (\n", materialName.asChar() );
	writer.addTab();

	writer.print( "Base (\n" );
	writer.addTab();

	writer.print( "DIFFUSE-COLOR %6.3f %6.3f %6.3f;\n", 
				diffuseR, diffuseG, diffuseB );

	if ( map )
	{
		writer.print( "TEXTURE-MAP (\n" );
		writer.addTab();		
		writeTexture( writer, map->name.asChar(), map->width, map->height, 
			map->channels, map->bitsPerChannel );
		writer.removeTab();
		writer.print( ")\n" );
	}

	writer.print( "SPECULAR-COLOR %.xf %.xf %.xf;\n",
				specR, specG, specB );	
	writer.print( "EMISSION-COLOR %.xf %.xf %.xf;\n",
				emissiveR, emissiveG, emissiveB );
	writer.print( "OPACITY %.xf;\n", opacity );
	writer.print( "AMBIENT-COLOR %.xf %.xf %.xf;\n",
				ambientR, ambientG, ambientB );

	writer.removeTab();
	writer.print( ")\n" );

	writer.print( "Lightscape ()\n" );
	writer.print( "Sega ()\n" );

	if ( N64Domain )
	{
		writer.print( "N64 (\n" );
		writer.addTab();

		outputBoolIfValid( writer, "USE-TEXTURE-MAP", N64Domain->useTextureMap );
		
		// This is in the base. If it's needed to be output to N64,
		// uncomment it out
/*		writer.print( "DIFFUSE-COLOR %.xf %.xf %.xf;\n",
			diffuseR, diffuseG, diffuseB );*/

		if ( N64Domain->faceControl != kDomainFaceInvalid )
		{
			writer.print( "FACING-CONTROL " );
			if ( N64Domain->faceControl == kDomainFaceFront )
				writer.print( "\"FRONT\";\n" );
			else if ( N64Domain->faceControl == kDomainFaceBack )
					writer.print( "\"BACK\";\n" );
				else if ( N64Domain->faceControl == kDomainFaceBoth )
					writer.print( "\"BOTH\";\n" );
		}

		outputBoolIfValid( writer, "OMIT-FACE", N64Domain->omitFace );

/*		if ( N64Domain->normalSource != kDomainNormalInvalid )
		{
			writer.print( "VERTEX-NORMAL-SOURCE " );
			if ( N64Domain->normalSource == kDomainNormalFace )
				writer.print( "\"FACE\";\n" );
			else
				writer.print( "\"VERTEX\";\n" );
		}*/

		outputBoolIfValid( writer, "PRESHADE", N64Domain->preShade );
		outputBoolIfValid( writer, "USE-LIGHT-OPTION", N64Domain->useLightOption );
		outputBoolIfValid( writer, "BLEND-COLOR", N64Domain->blendColor );

/*		outputBoolIfValid( writer, "USE-ZBUFFER", N64Domain->useZBuffer );
		outputBoolIfValid( writer, "USE-FOG", N64Domain->useFog );
		outputBoolIfValid( writer, "USE-REFLECTION-MAPPING", N64Domain->useReflectionMapping );*/

		if ( map )
		{
			int textureID, horizWrap, vertWrap;
			char* textureName;

			DtTextureGetID( materialID, &textureID );
			DtTextureGetNameID( textureID, &textureName );

			// we should have textureName from above
			DtTextureGetWrap( textureName, &horizWrap, &vertWrap );
			writer.print( "TEXTURE-TILE-S " );
			// ge also has a 'MIRROR' val, but this is inaccessible through
			// Dt right now. It's the texturePlacement's attr, and it's called "mirror"
			if ( DT_REPEAT == horizWrap )	
				writer.print( "\"WRAP\";\n" );
			else
				writer.print( "\"CLAMP\";\n" );
			writer.print( "TEXTURE-TILE-T " );
			if ( DT_REPEAT == vertWrap )
				writer.print( "\"WRAP\";\n" );
			else
				writer.print( "\"CLAMP\";\n" );
		}

		writer.removeTab();
		writer.print( ")\n" );
	}
	else
	{
		writer.print( "N64 ()\n" );
	}

	if ( PSXDomain )
	{
		writer.print( "SonyPSX (\n" );
		writer.addTab();
		// Diffuse color is in the base. Uncomment the following out if needed
/*		writer.print( "DIFFUSE-COLOR %.xf %.xf %.xf;\n",
			diffuseR, diffuseG, diffuseB ); */

		outputBoolIfValid( writer, "USE-TEXTURE-MAP", PSXDomain->useTextureMap );
		outputBoolIfValid( writer, "OMIT-FACE", PSXDomain->omitFace );		

		if ( PSXDomain->faceControl != kDomainFaceInvalid )
		{
			writer.print( "FACING-CONTROL " );
			if ( PSXDomain->faceControl == kDomainFaceFront )
				writer.print( "\"FRONT\";\n" );
			else if ( PSXDomain->faceControl == kDomainFaceBack )
					writer.print( "\"BACK\";\n" );
				else if ( PSXDomain->faceControl == kDomainFaceBoth )
					writer.print( "\"BOTH\";\n" );
		}		

		outputBoolIfValid( writer, "ABE", PSXDomain->abe );
		outputBoolIfValid( writer, "PRESHADE", PSXDomain->preShade );		
		outputBoolIfValid( writer, "LIGHT-CALCULATION", PSXDomain->lightCalculation );

		if ( -1 != PSXDomain->abr )
		{
			writer.print( "ABR \"" );
			switch ( PSXDomain->abr )
			{
				case 0:
					writer.print( "0" );
					break;
				case 1:
					writer.print( "1" );
					break;
				case 2:
					writer.print( "2" );
					break;
				case 3:
					writer.print( "3" );
					break;
			}
			writer.print( "\";\n" );
			
		}

		if ( PSXDomain->normalSource != kDomainNormalInvalid )
		{
			writer.print( "VERTEX-NORMAL-SOURCE " );
			if ( PSXDomain->normalSource == kDomainNormalFace )
				writer.print( "\"FACE\";\n" );
			else
				writer.print( "\"VERTEX\";\n" );
		}		

		outputBoolIfValid( writer, "PERFORM-SUBDIVISION", PSXDomain->subdiv );
		outputBoolIfValid( writer, "PERFORM-ACTIVE-SUBDIVISION", PSXDomain->activeSubdiv );
		outputBoolIfValid( writer, "CLIP", PSXDomain->clip );
		outputBoolIfValid( writer, "PERFORM-MIPMAPPING", PSXDomain->useMipmap );

		writer.removeTab();
		writer.print( ")\n" );
	}
	else
	{
		writer.print( "SonyPSX ()\n" );
	}

	writer.print( "Render ()\n" );

	if ( GLDomain )
	{
		writer.print( "GL_Shade (\n" );
		writer.addTab();
		// All of the following are in the base, and shouldn't need
		// to be output here, but the xwriter doesn't handle this too well...
		writer.print( "DIFFUSE-COLOR %.xf %.xf %.xf;\n", 
					diffuseR, diffuseG, diffuseB );
		writer.print( "SHININESS %.xf;\n", specExp );

		if ( map )
		{
			writer.print( "TEXTURE-MAP (\n" );
			writer.addTab();		
			writeTexture( writer, map->name.asChar(), map->width, map->height, 
				map->channels, map->bitsPerChannel );
			writer.removeTab();
			writer.print( ")\n" );
		}

		writer.print( "SPECULAR-COLOR %6.3f %6.3f %6.3f;\n",
					specR, specG, specB );
		writer.print( "EMISSION-COLOR %6.3f %6.3f %6.3f;\n",
					emissiveR, emissiveG, emissiveB );
		
		if ( GLDomain->normalSource != kDomainNormalInvalid )
		{
			writer.print( "VERTEX-NORMAL-SOURCE " );
			if ( GLDomain->normalSource == kDomainNormalFace )
				writer.print( "\"FACE\";\n" );
			else
				writer.print( "\"VERTEX\";\n" );
		}

//		outputBoolIfValid( writer, "OUTLINING", GLDomain->outlining );

		// Is this opacity the same as the base opacity??
		writer.print( "OPACITY %.xf;\n", opacity );

		outputBoolIfValid( writer, "COLOR-BY-VERTEX", GLDomain->colorByVertex );

		writer.removeTab();
		writer.print( ")\n" );
	}
	else
	{
		writer.print( "GL_Shade ()\n" );
	}
	
	writer.removeTab();
	writer.print( ")\n" );
}

void ge2Wrapper::MShader::writeTexture( MTextFileWriter& writer,
		const char* name, 
		int width, 
		int height, 
		int channels, 
		int bitsPerChannel )
{
	writer.print( "name \"%s\";\n", name );
	writer.print( "width %d; height %d;\n", width, height );
	writer.print( "number-of-channels %d;\n", channels );
	writer.print( "bits-per-channel %d;\n", bitsPerChannel );
}

void ge2Wrapper::MShader::outputBoolIfValid( MTextFileWriter& writer, char* name, GEDomainBool boolVal )
{
	if ( kDomainBoolInvalid != boolVal )
	{
		writer.print( "%s ", name );
		if ( kDomainBoolTrue == boolVal )
			writer.print( "TRUE;\n" );
		else
			writer.print( "FALSE;\n" );
	}
}
