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
// Write frame by frame values in a file in XML format.
//
// Flags:
//
// -on/outputName <string>: Name of the output file. When not set, the
//		result goes into the standard output.
//
// -s/start <frameNumber>: Starting frame. When not set the start 
//		frame is read from the render globals or the animation slider.
//		
// -e/end <frameNumber>: End frame. When not set the end 
//		frame is read from the render globals or the animation slider.
//
// -by/byFrame <frameNumber>: Frame increment. When not set the 
//		frame increment is read from the render globals or defaults to 1.
//
// -cam/camera <name>: Name of the camera to be tracked. Multiple
//		such flags can be used, resulting in several camera being
//		tracked. When not set, all the renderable cameras are tracked.
//		
// -p/plug <name>: Additionnal values tracked per frame. Any plug 
//		name can be used.
//		
// Examples:
//      // Track two cameras, x translate and y rotation of object pCube1
//		animInfo -cam top -cam persp -p pCube1.tx -p pCube1.ry;
//
//		// Write renderable cameras into file /tmp/tstData, frame 1 to 10
//      animInfo -on "/tmp/tstData" -s 1 -e 10;
//
//      // Print renderable cameras into standard output. Frame range
//      // is read from the render global or the animation slider
//      // depending on user settings.
//      animInfo;
//
// The command may take a long time to run. If it is aborted with the
// ESC key, the the current frame data is completed (Current <TIME>
// </TIME> data, and an <INTERRUPT> tag is printed before closing the
// <SEQUENCE> data.
// 

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MFloatMatrix.h>
#include <maya/MTime.h>
#include <maya/MArgList.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnCamera.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MAnimControl.h>
#include <maya/MPxCommand.h>
#include <maya/MComputation.h>
#include <maya/MFnPlugin.h>

#define mCommandName "animInfo"				// Command name

#define INDENT_STEP  2						// Indentation steps

#define MAX_NB_PLUGS 100					// Max number of plugs
#define MAX_NB_CAM   32						// Max number of camera
#define FILE_TYPE    "Maya per frame data"	// File type
#define VERSION_ID   0						// Version number

static char * stIndentStr = "                                                ";

class animInfoCmd : public MPxCommand
{
public:
				 animInfoCmd();
    virtual		~animInfoCmd();

    virtual MStatus doIt ( const MArgList& args );

    static void* creator();

private:
	static MStatus nodeFromName(MString name, MObject & obj);
	const char * indent() const {return stIndentStr+strlen(stIndentStr)-fIndent;}

	MStatus parseArgs ( const MArgList& args );
	void readSceneStartEnd();
	MStatus writeFrameInfo( double frame );
	MStatus writeCameraInfo( const MFnCamera & cam );

	double		fStartFrame;
	double		fEndFrame;
	double		fByFrame;
	int			fIndent;
	FILE *		fOutput;

	int			fNbCameraPath;
	MDagPath	fCameraPath[MAX_NB_CAM];

	// We cannot use MPlugArray, as MPlugArray::append does not copy
	// the MPlug object...
	int			fNbPlugs;
	MPlug		fPlugs[MAX_NB_PLUGS];
};

animInfoCmd::animInfoCmd() :
	fStartFrame(-1),
	fEndFrame  (-2),
	fByFrame   ( 1),
	fIndent    (INDENT_STEP),
	fOutput    (stdout),
	fNbCameraPath(0),
	fNbPlugs   (0)
{}

animInfoCmd::~animInfoCmd() {}

//
//
///////////////////////////////////////////////////////////////////////////////

MStatus animInfoCmd::nodeFromName(MString name, MObject & obj)
{
	MSelectionList tempList;
    tempList.add( name );
    if ( tempList.length() > 0 ) 
	{
		tempList.getDependNode( 0, obj );
		return MS::kSuccess;
	}
	return MS::kFailure;
}



//
// Read the arguments, and make sure they are consistant
///////////////////////////////////////////////////////////////////////////////
#define MATCH(str, shortName, longName) \
		(((str)==(shortName))||((str)==(longName)))
static int stringArg(const MArgList& args, unsigned int &indx, MString & res)
{
	if (indx < args.length())
	{
		MStatus stat;
		indx++;
		res = args.asString( indx, &stat );
		if (stat == MS::kSuccess) 
			return 1;
	}
	return 0;
}

static int doubleArg(const MArgList& args, unsigned int &indx, double & res)
{
	if (indx < args.length())
	{
		MStatus stat;
		indx++;
		res = args.asDouble( indx, &stat );
		if (stat == MS::kSuccess) 
			return 1;
	}
	return 0;
}

// this method currently unused
//
// static int intArg(const MArgList& args, unsigned int &indx, int & res)
// {
// 	if (indx < args.length())
// 	{
// 		MStatus stat;
// 		indx++;
// 		res = args.asInt( indx, &stat );
// 		if (stat == MS::kSuccess) 
// 			return 1;
// 	}
// 	return 0;
// }

MStatus animInfoCmd::parseArgs( const MArgList& args )
{
	// Parse the arguments.
	MString arg;
	MStatus stat = MS::kSuccess;
	MString str;
	MObject cameraNode;
	unsigned int i;

	for ( i = 0; i < args.length(); i++ ) {
		arg = args.asString( i, &stat );
		if (stat != MS::kSuccess)
			continue;

		if (MATCH(arg, "-on", "-outputName") && stringArg(args, i, str))
		{
			FILE *f;
			if ( NULL != (f = fopen(str.asChar(), "w")))
				fOutput = f;
			else 
			{
				fprintf(stderr, "cannot open '%s'\n", str.asChar());
				fflush(stderr);
			}
		}
		else if (MATCH(arg, "-s", "-start"))
			doubleArg(args, i, fStartFrame);
		else if (MATCH(arg, "-e", "-end"))
			doubleArg(args, i, fEndFrame);
		else if (MATCH(arg, "-by", "-byFrame"))
			doubleArg(args, i, fByFrame);
		else if (MATCH(arg, "-cam", "-camera") && stringArg(args, i, str))
		{
			if (fNbCameraPath >= MAX_NB_CAM)
				break;

			nodeFromName(str, cameraNode);
			MStatus status;
			MDagPath::getAPathTo(cameraNode, fCameraPath[fNbCameraPath]);
			fCameraPath[fNbCameraPath].extendToShape();
			MFnCamera cam(fCameraPath[fNbCameraPath], &status);
			if (status == MS::kSuccess)
				fNbCameraPath++;
			
		}
		else if (MATCH(arg, "-p", "-plug") && stringArg(args, i, str))
		{
			MSelectionList plugList;
			plugList.add( str );
			// Find the plugs out of the names in the selectionList
			for (unsigned int k = 0 ; k<plugList.length() ; k++)
			{
				if (fNbPlugs >= MAX_NB_PLUGS)
					break;

				if (plugList.getPlug(k, fPlugs[fNbPlugs]) == MS::kSuccess)
					fNbPlugs++;
			}
		}
		else
		{
			fprintf(stderr, "Unknown argument '%s'\n", arg.asChar());
			fflush(stderr);
		}
	}

	if (fByFrame<=0) fByFrame = 1;

	if (fNbCameraPath == 0)
	{
		// User did not specify a valid camera. Find one.
		MItDag dagIterator( MItDag::kDepthFirst, MFn::kCamera);
        for (; !dagIterator.isDone(); dagIterator.next())
        {
			if (fNbCameraPath >= MAX_NB_CAM)
				break;

	    	bool		renderable;
	    	if ( !dagIterator.getPath(fCameraPath[fNbCameraPath]) )
	    		continue;
	
	    	renderable = false;
	    	MFnCamera fnCameraNode( fCameraPath[fNbCameraPath] );
	    	fnCameraNode.findPlug( "renderable" ).getValue( renderable );
	
	    	if (renderable)
				fNbCameraPath++;
        }
	}

	return MS::kSuccess;
}

void animInfoCmd::readSceneStartEnd()
{
	MTime startFrame;
	MTime endFrame;

    // Get the render globals node
    //
	int rangeIsSet = 0;
	MObject renderGlobNode;
	if (nodeFromName("defaultRenderGlobals", renderGlobNode) == MS::kSuccess)
	{
		MFnDependencyNode fnRenderGlobals( renderGlobNode );

		// Check if the time-slider or renderGlobals is used for 
		// the frame range
		//
		MPlug animPlug = fnRenderGlobals.findPlug( "animation" );
		short anim;
		animPlug.getValue( anim );

		if ( anim ) {
			float byFrame;
			fnRenderGlobals.findPlug( "startFrame"  ).getValue(startFrame);
			fnRenderGlobals.findPlug( "endFrame"    ).getValue(endFrame);
			fnRenderGlobals.findPlug( "byFrameStep" ).getValue(byFrame);
			fByFrame = (double) byFrame;
			rangeIsSet = 1;
		}
	}
	if (!rangeIsSet)	{
		// USE_TIMESLIDER
		startFrame = MAnimControl::minTime();
		endFrame   = MAnimControl::maxTime();
		fByFrame = 1;
	}
	fStartFrame = (int) startFrame.as( MTime::uiUnit() );
	fEndFrame   = (int)   endFrame.as( MTime::uiUnit() );
}

//
// Print info for a given time frame
///////////////////////////////////////////////////////////////////////////////
MStatus animInfoCmd::writeFrameInfo( double frame )
{
	int i;
	MStatus status;
	MGlobal::viewFrame (frame);				// Set the current frame

	fprintf(fOutput, "%s<TIME VAL=%g>\n", indent(), frame);
	fIndent += INDENT_STEP;

	for (i = 0 ; i<fNbCameraPath ; i++)
	{
		MFnCamera cam(fCameraPath[i], &status);
		if (status == MS::kSuccess)
			writeCameraInfo(cam);
	}

	double val;
	for (i = 0 ; i<fNbPlugs ; i++)
	{
		if (fPlugs[i].getValue(val) == MS::kSuccess)
			fprintf(fOutput, "%s<PARAM NAME=\"%s\" VAL=%lg>\n", 
					indent(), fPlugs[i].name().asChar(), val);
	}

	fIndent -= INDENT_STEP;
	fprintf(fOutput, "%s</TIME>\n", indent());
	return MS::kSuccess;
}

//
// Print info for a given time frame
///////////////////////////////////////////////////////////////////////////////
MStatus animInfoCmd::writeCameraInfo( const MFnCamera & cam )
{
	MStatus status;
	fprintf(fOutput, "%s<CAMERA NAME=\"%s\">\n", 
			indent(), cam.name().asChar());
	fIndent += INDENT_STEP;

	MPoint pt = cam.eyePoint(MSpace::kWorld, &status);
	fprintf(fOutput, "%s<EYE X=%g Y=%g Z=%g>\n", indent(), pt.x, pt.y, pt.z);

	MVector v = cam.viewDirection(MSpace::kWorld);
	fprintf(fOutput, "%s<DIR X=%g Y=%g Z=%g>\n", indent(), v.x, v.y, v.z);

	v = cam.upDirection(MSpace::kWorld);
	fprintf(fOutput, "%s<UP X=%g Y=%g Z=%g>\n", indent(), v.x, v.y, v.z);

	v = cam.rightDirection(MSpace::kWorld);
	fprintf(fOutput, "%s<RIGHT X=%g Y=%g Z=%g>\n", indent(), v.x, v.y, v.z);

	pt = cam.centerOfInterestPoint(MSpace::kWorld, &status);
	if (status == MS::kSuccess)
		fprintf(fOutput, "%s<COI X=%g Y=%g Z=%g>\n", indent(), pt.x,pt.y,pt.z);
	else
		fprintf(fOutput, "%s<COI NONE>\n", indent());

	fprintf(fOutput, "%s<ASPECTRATIO VAL=%g>\n", indent(), cam.aspectRatio());
	fprintf(fOutput, "%s<FSTOP VAL=%g>\n", indent(), cam.fStop());
	fprintf(fOutput, "%s<FOCALLENGTH VAL=%g>\n", indent(), cam.focalLength());
	fprintf(fOutput, "%s<FOCUS VAL=%g NEAR=%g FAR=%g>\n", indent(),
			cam.focusDistance(), 
			cam.nearFocusDistance(), cam.farFocusDistance());

	MFloatMatrix mat = cam.projectionMatrix();
	fprintf(fOutput, "%s<MAT A00=%g A01=%g A02=%g A03=%g\n", indent(),
			mat(0,0), mat(0,1), mat(0,2), mat(0,3));
	fprintf(fOutput, "%s     A10=%g A11=%g A12=%g A13=%g\n", indent(),
			mat(1,0), mat(1,1), mat(1,2), mat(1,3));
	fprintf(fOutput, "%s     A20=%g A21=%g A22=%g A23=%g\n", indent(),
			mat(2,0), mat(2,1), mat(2,2), mat(2,3));
	fprintf(fOutput, "%s     A30=%g A31=%g A32=%g A33=%g>\n", indent(),
			mat(3,0), mat(3,1), mat(3,2), mat(3,3));

	fIndent -= INDENT_STEP;
	fprintf(fOutput, "%s</CAMERA>\n", indent());

	return MS::kSuccess;
}

//
// Main routine
///////////////////////////////////////////////////////////////////////////////
MStatus animInfoCmd::doIt( const MArgList& args )
{
	// parse the command arguments
	//
	MStatus stat = parseArgs(args);
	if (stat != MS::kSuccess) {
		if (fOutput != stdout) 
			fclose(fOutput);
		return stat;
	}

	if (fStartFrame > fEndFrame)
		readSceneStartEnd();

	// Remember the frame the scene was at so we can restore it later.
	MTime currentFrame = MAnimControl::currentTime();
	fprintf(fOutput, "<FILE TYPE=\"%s\" VERSION=%d>\n", FILE_TYPE, VERSION_ID);
	fprintf(fOutput, "\n%s<SEQUENCE START=%g END=%g STEP=%g>\n", 
			indent(), fStartFrame, fEndFrame, fByFrame);
	fIndent += INDENT_STEP;

	MComputation computation;
	computation.beginComputation();

	for (double frame = fStartFrame ; frame <= fEndFrame ; frame += fByFrame)
	{
		if (computation.isInterruptRequested())
		{
			fprintf(fOutput, "%s<INTERRUPTION>\n", indent());
			break ;
		}
		writeFrameInfo(frame);
	}
	computation.endComputation();

	fIndent -= INDENT_STEP;
	fprintf(fOutput, "%s</SEQUENCE>\n", indent());

	fflush(fOutput);
	if (fOutput != stdout) 
		fclose(fOutput);

	// Return to the frame we were at before we ran the animation
	MGlobal::viewFrame (currentFrame);

	return MS::kSuccess;
}

//
//
///////////////////////////////////////////////////////////////////////////////
void * animInfoCmd::creator() { return new animInfoCmd(); }

MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.0", "Any");

    MStatus status = plugin.registerCommand(mCommandName,
											animInfoCmd::creator );
	if (!status) status.perror("registerCommand");

    return status;
}

MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );

    MStatus status = plugin.deregisterCommand(mCommandName);
	if (!status) status.perror("deregisterCommand");

    return status;
}
