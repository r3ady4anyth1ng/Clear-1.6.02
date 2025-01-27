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
// VRML2.0 Translator Maya specific source
//


#include <stdarg.h>
#include <stdlib.h>

#if defined (SGI) || defined (LINUX)
#include <sys/param.h>
#include <sys/stat.h>
#elif defined (OSMac_)
#	if defined(OSMac_CFM_)
#		include <types.h>
#	else
#		include <sys/types.h>
#	endif	
#endif

// #include <maya/NTDLL.h>

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MDistance.h>


#ifdef AW_NEW_IOSTREAMS
#include <iostream>
#include <fstream>
#else
#include <iostream.h>
#include <fstream.h>
#endif
#include <string.h>


#include <MDt.h>
#include <MDtExt.h>

#if defined (_WIN32) || defined (OSMac_)
#define True 1
#define False 0
#endif

#ifdef WIN32
#pragma warning(disable: 4800 4305)
#define MAXPATHLEN 512
#define Boolean bool
#endif // WIN32

extern char     VR_SceneName[MAXPATHLEN],
                VR_TexturePath[MAXPATHLEN];


extern int      VR_Precision,
                VR_HierarchyMode,
                VR_SelectionType;

extern int      VR_FrameStart,
                VR_FrameEnd,
                VR_FrameBy;
extern int      VR_AnimStartG,
                VR_AnimEndG,
                VR_AnimByG;
extern int		VR_KeyCurves;
extern int      VR_ViewerType[];

extern double   VR_FramesPerSec;
extern double   VR_NavigationSpeed;


extern Boolean  VR_LoopAnim,
                VR_AnimTrans,
                VR_AnimMaterials,
                VR_AnimLights,
				VR_AnimCameras,
                VR_AnimVertices,
                VR_MetaCycle,
                VR_AddTexturePath,
                VR_ViewFrames,
                VR_ExportNormals,
                VR_ColorPerVertex,
                VR_LongLines,
                VR_OppositeNormals;
extern Boolean  VR_Headlight;
extern Boolean  VR_AutoLaunchViewer;

extern Boolean  VR_EvalTextures,
                VR_ExportTextures;
extern Boolean	VR_OriginalTextures;

extern int      VR_Verbose,
                VR_MaxXTexRes,
                VR_MaxYTexRes,
                VR_XTexRes,
                VR_YTexRes;
extern int      VR_STextures;
extern int      VR_TimeSlider;

extern Boolean  VR_Compressed;

extern char *	vrml2Version;

extern void     VR_InitTransfer (int (*LPrintFunc) (const char *,...), Boolean LForceUpdate);

extern Boolean  VR_GetScene (char *, Boolean);
int             VR_PrintMessage (const char *LMessage,...);

//	Check for animation enabled

static int	AnimEnabled = false;

MString scriptToRun;
int		scriptAppend = 0;

/* ====================================== */
/* Get path name from a full path	 */
/* ====================================== */
char *
MDt_GetPathName (char *LFullStr, char *LRet, int LMaxLen)
{
	register int	LC;
	register int    LLen,
	                LLastSlashPos;

	LLastSlashPos = -1;
	for (LC = 0; LFullStr[LC] != '\0'; LC++)
	{
		if (LFullStr[LC] == '/')
			LLastSlashPos = LC;
	}

	LLen = LLastSlashPos + 1;

	if (LRet == NULL)
		return (NULL);

	LC = 0;
	if ((LRet != NULL) && ((LLen + 1) <= LMaxLen))
	{
		for (; LC < LLen; LC++)
			LRet[LC] = LFullStr[LC];
	}
	LRet[LC] = '\0';

	return (LRet);
}



/* ============================================== */
/* Launch Netscape with the exported file   */
/* ============================================== */
void 
VR_LaunchViewer (char *LFileName)
{

#if defined (_WIN32)

	char dpath[512];
	
	MDt_GetPathName( LFileName, dpath, 256);
	
	ShellExecute(NULL, NULL, LFileName, NULL, NULL, SW_SHOWNORMAL);


#elif defined (SGI) || defined (LINUX)
	char            LTmpStr[MAXPATHLEN + 1];
	struct stat     LST;

	strcpy (LTmpStr, getenv ("HOME"));
	strcat (LTmpStr, "/.netscape/lock");

	//Is Netscape already running ?
		if (lstat (LTmpStr, &LST) == 0)
	{
		VR_PrintMessage ("\nTransferring file to Netscape...\n");
		sprintf (LTmpStr, "netscape -remote \"openURL(file:%s)\"&", LFileName);

		//printf ("system command = %s\n", LTmpStr);

		system (LTmpStr);
	} else
	{
		VR_PrintMessage ("\nLaunching Netscape...\n");
		sprintf (LTmpStr, "netscape file:%s&", LFileName);

		//printf ("system command = %s\n", LTmpStr);

		system (LTmpStr);
	}

#endif

}



class vrml2Translator : public MPxFileTranslator {

public:

	vrml2Translator () { };
	virtual ~vrml2Translator () { };
	static void    *creator ();

	MStatus         reader ( const MFileObject& file,
			                 const MString& optionsString,
			                 MPxFileTranslator::FileAccessMode mode);
	MStatus         writer ( const MFileObject& file,
		                     const MString& optionsString,
		                     MPxFileTranslator::FileAccessMode mode);
	bool            haveReadMethod () const;
	bool            haveWriteMethod () const;
	MString         defaultExtension () const;
	bool            canBeOpened () const;
	MFileKind       identifyFile ( const MFileObject& fileName,
				                   const char *buffer,
				                   short size) const;

private:
	static MString  magic;
};

void* vrml2Translator::creator()
{
	return new vrml2Translator();
}




//Print a message into the TextWidget
//
int 
VR_SetMessage (const char *LMessage,...)
{
	char            LTmpStr[1024];
	va_list         LAL;

	va_start (LAL, LMessage);
	vsprintf (LTmpStr, LMessage, LAL);
	va_end (LAL);

#if 0
	printf ("%s\n", LTmpStr);
	fflush (stdout);
#endif


	return (0);
}


//Append a message into the TextWidget
//
int 
VR_PrintMessage (const char *LMessage,...)
{
	char            LTmpStr[1024];
	va_list         LAL;

	va_start (LAL, LMessage);
	vsprintf (LTmpStr, LMessage, LAL);
	va_end (LAL);

#if 0
	printf ("%s\n", LTmpStr);
	fflush (stdout);
#endif
	return (0);
}


//Print a message into the TextWidget 's given line
//
int 
VR_SetMessageLine (int LLineNum, const char *LMessage,...)
{
	char            LTmpStr[1024];
	va_list         LAL;

	va_start (LAL, LMessage);
	vsprintf (LTmpStr, LMessage, LAL);
	va_end (LAL);

#if 0
	printf ("%s\n", LTmpStr);
	fflush (stdout);
#endif

	return (0);
}

// Initialize our magic "number"
MString vrml2Translator::magic ("#VRML2 V2.0 utf8");

// An VRML2 file is an ascii file where the 1st line contains 
// the string #VRML V2.0 utf8.
//
//
// The reader is not implemented yet.
//
MStatus vrml2Translator::reader (const MFileObject & file,
								const MString & options,
								MPxFileTranslator::FileAccessMode mode)
{
	MStatus rval (MS::kSuccess);

	return rval;
}

// Write method of the VRML2.0 translator / file exporter
//

MStatus vrml2Translator::writer ( const MFileObject & fileObject,
								  const MString & options,
								  MPxFileTranslator::FileAccessMode mode)
{
	char            LTmpStr[MAXPATHLEN];
	unsigned int    i;
	int             LN;

	Boolean         retFlag = false;

	const MString   fname = fileObject.fullName ();
	MString         extension;
	MString         baseFileName;


	// Lets strip off the known extension of .wrl if it is there.
	
	extension.set (".wrl");
	int  extLocation = fileObject.name ().rindex ('.');

	if (extLocation > 0 && fileObject.name ().substring (extLocation,
			     fileObject.name ().length () - 1) == extension)
	{
		baseFileName = fileObject.name ().substring (0, extLocation - 1);
	} else
	{
		baseFileName = fileObject.name ();
		extension.clear ();
	}

	strcpy( VR_SceneName, baseFileName.asChar() );


	// Lets now do all of the option processing
	
	if (options.length () > 0)
	{
		//Start parsing.

		MStringArray optionList;
		MStringArray theOption;

		options.split (';', optionList);

		//break out all the options.

		for (i = 0; i < optionList.length (); ++i)
		{
			theOption.clear ();
			optionList[i].split ('=', theOption);
			if (theOption.length () > 1)
			{
				if (theOption[0] == MString ("loop"))
				{
					VR_LoopAnim = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("timeslider"))
				{
					VR_TimeSlider = (int) (theOption[1].asInt ());

                } else if (theOption[0] == MString ("animEnabled"))
                { 
                    AnimEnabled = (int) (theOption[1].asInt ());

				} else if (theOption[0] == MString ("animStart"))
				{
					VR_FrameStart = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("animEnd"))
				{
					VR_FrameEnd = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("animStep"))
				{
					VR_FrameBy = (int) (theOption[1].asInt ());

                } else if (theOption[0] == MString ("animFramesPerSec"))
                {
                    VR_FramesPerSec = (double) (theOption[1].asDouble ());

				} else if (theOption[0] == MString ("keyCurves"))
				{
					VR_KeyCurves = (int) (theOption[1].asInt() );

				} else if (theOption[0] == MString ("animVertices"))
				{
					VR_AnimVertices = (int) (theOption[1].asInt ());
					DtExt_setVertexAnimation( VR_AnimVertices );
					
				} else if (theOption[0] == MString ("animTransforms"))
				{
					VR_AnimTrans = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("animShaders"))
				{
					VR_AnimMaterials = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("animLights"))
				{
					VR_AnimLights = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("animCameras"))
				{
					VR_AnimCameras = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("hrcType"))
				{
					VR_HierarchyMode = theOption[1].asInt () - 1;
                } else if (theOption[0] == MString ("joints"))
                {
                    // Allow user to specify if the hierarchy should include
                    // NULL geometry nodes - usually joints 

                    DtExt_setJointHierarchy( theOption[1].asInt() );

				} else if (theOption[0] == MString ("exportSel"))
				{
					VR_SelectionType = theOption[1].asInt () - 1;

				} 
			
				// Check for various viewer modes
				// can now have more than 1 selected

				else if (theOption[0] == MString ("vwalk"))
				{
					VR_ViewerType[0] = theOption[1].asInt();
                } else if (theOption[0] == MString ("vexamine"))
                {
                    VR_ViewerType[1] = theOption[1].asInt();
                } else if (theOption[0] == MString ("vfly"))
                {
                    VR_ViewerType[2] = theOption[1].asInt();
                } else if (theOption[0] == MString ("vany"))
                {
                    VR_ViewerType[3] = theOption[1].asInt();
                } else if (theOption[0] == MString ("vnone"))
                {
                    VR_ViewerType[4] = theOption[1].asInt();
				} 


				else if (theOption[0] == MString ("navigationSpeed"))
				{
					VR_NavigationSpeed = theOption[1].asFloat ();
				} else if (theOption[0] == MString ("headlight"))
				{
					VR_Headlight = (Boolean) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("texsample"))
				{
					VR_STextures = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("texevaluate"))
				{
					VR_EvalTextures = (Boolean) (theOption[1].asInt ());
                } else if (theOption[0] == MString ("texoriginal"))
                {
                    VR_OriginalTextures = (Boolean) (theOption[1].asInt ());

				} else if (theOption[0] == MString ("Xtexres"))
				{
					VR_XTexRes = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("Ytexres"))
				{
					VR_YTexRes = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("MaxXtexres"))
				{
					VR_MaxXTexRes = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("MaxYtexres"))
				{
					VR_MaxYTexRes = (int) (theOption[1].asInt ());

                } else if (theOption[0] == MString ("searchPath"))
                {
                    DtExt_addTextureSearchPath( (char *)theOption[1].asChar());

				} else if (theOption[0] == MString ("precision"))
				{
					VR_Precision = theOption[1].asInt ();
				} else if (theOption[0] == MString ("exportNormals"))
				{
					VR_ExportNormals = (Boolean) (theOption[1].asInt ());

                } else if (theOption[0] == MString ("exportCPV"))
                {
                    VR_ColorPerVertex = (Boolean) (theOption[1].asInt ());
                    
				} else if (theOption[0] == MString ("oppositeNormals"))
				{
					VR_OppositeNormals = (Boolean) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("exportTextures"))
				{
					VR_ExportTextures = (Boolean) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("longLines"))
				{
					VR_LongLines = (Boolean) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("launchViewer"))
				{
					VR_AutoLaunchViewer = (Boolean) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("verbose"))
				{
					VR_Verbose = (int) (theOption[1].asInt ());
                } else if (theOption[0] == MString ("debug"))
                {
                    int levelG = DtExt_Debug();
                    if ( (int) (theOption[1].asInt () ) )
                        levelG |= DEBUG_GEOMAT;
                    else 
                        levelG &= ~DEBUG_GEOMAT;
                        
                    DtExt_setDebug( levelG );

                } else if (theOption[0] == MString ("debugC"))
                {
                    int levelC = DtExt_Debug();
                    if ( (int) (theOption[1].asInt () ) )
                        levelC |= DEBUG_CAMERA;
                    else 
                        levelC &= ~DEBUG_CAMERA;
                        
                    DtExt_setDebug( levelC );

                } else if (theOption[0] == MString ("debugL"))
                {
					int levelL = DtExt_Debug();
					if ( (int) (theOption[1].asInt () ) )
						levelL |= DEBUG_LIGHT;
					else 
	                    levelL &= ~DEBUG_LIGHT;

                    DtExt_setDebug( levelL );

				} else if (theOption[0] == MString ("compressed"))
				{
					VR_Compressed = (int) (theOption[1].asInt ());
				} else if (theOption[0] == MString ("reversed"))
				{
					DtExt_setWinding( theOption[1].asInt() );
				} else if (theOption[0] == MString ("tesselation"))
				{
					if ( theOption[1].asInt() == 2 )
					{
						DtExt_setTesselate( kTESSQUAD );
					} else {
						DtExt_setTesselate( kTESSTRI );
					}

                } else if (theOption[0] == MString ("script"))
                {
                    scriptToRun = theOption[1];

				} else if (theOption[0] == MString ("scriptAppend"))
				{
					scriptAppend = (int)(theOption[1].asInt() );
				}
				
			}
		}
	}

	// Lets see how we entered this plug-in, either with the export all
	// or export selection flag set.

    if ( mode == MPxFileTranslator::kExportActiveAccessMode )
	{ 
		VR_SelectionType = 2;
	}


	// Lets check the TimeSlider control now:

	if ( VR_TimeSlider )
	{
		MTime start( MAnimControl::minTime().value(), MTime::uiUnit() );
        VR_FrameStart = (int) start.value();

		MTime end( MAnimControl::maxTime().value(), MTime::uiUnit() );
		VR_FrameEnd = (int) end.value();
	}

	// Now see if the animation is really enabled.
	// Else we will set the end frame to the beginning frame automatically

	if ( !AnimEnabled )
	{
		VR_FrameEnd = VR_FrameStart;
	}

	// Find out where the file is supposed to end up.
	
	MDt_GetPathName ((char *) (fname.asChar ()), LTmpStr, MAXPATHLEN);

	LN = (int)strlen (LTmpStr);
	if (LTmpStr[LN - 1] == '/')
		LTmpStr[LN - 1] = '\0';

	DtSetDirectory (LTmpStr);

	// Now lets setup some paths to do basic texture file searching
	// for those textures with relative paths

	MStringArray wSpacePaths;
	MStringArray rPaths;
	MString 	 usePath;
	MString		 separator;
	MString		 path_symbol;
	char		 *chkPath;
	
	MGlobal::executeCommand( "workspace -q -rd", wSpacePaths );
	MGlobal::executeCommand( "workspace -q -rtl", rPaths );

	if ( DtExt_getTextureSearchPath() )
		separator.set( "|" );
	else
		separator.set( "" );

    for (i = 0; i < wSpacePaths.length (); ++i)
	{
		// Check that the SpacePaths end in / or \
		// if so then don't add in another one
		
		chkPath = (char *)wSpacePaths[i].asChar();
		if ( chkPath[wSpacePaths[i].length()-1] == '/' ||
			 chkPath[wSpacePaths[i].length()-1] == '\\' )
	        path_symbol.set( "" );
	    else
	        path_symbol.set( "/" );
			
		
		for ( unsigned int j = 0; j < rPaths.length(); j++ )
		{
			usePath = usePath + separator + wSpacePaths[i] + path_symbol + rPaths[j];
			separator.set( "|" );

			if ( rPaths[j] == MString( "sourceImages" ) )
				usePath = usePath + separator + wSpacePaths[i] + path_symbol
				+ MString( "sourceimages" );

		}
	}

    DtExt_addTextureSearchPath( (char *)usePath.asChar() );

	// Need to do some initialization stuff
	
	VR_InitTransfer (NULL, False);

	// Now do the export
	
	retFlag = VR_GetScene (LTmpStr, True);

	// Do we want to launch a viewer to see what we did ?
	
	if (retFlag && VR_AutoLaunchViewer)
	{
		VR_LaunchViewer (LTmpStr);
	}

	// Now lets see if the user wants something else to be done

	if ( 0 < scriptToRun.length() )
	{
		if ( scriptAppend )
		{
			scriptToRun += MString( " " ) + MString( LTmpStr );
		}

		system( scriptToRun.asChar() );
	}

	return MS::kSuccess;
}

bool vrml2Translator::haveReadMethod () const
{
	return false;
}

bool vrml2Translator::haveWriteMethod () const
{
	return true;
}

// Whenever Maya needs to know the preferred extension of this file format,
// it calls this method.  For example, if the user tries to save a file called
// "test" using the Save As dialog, Maya will call this method and actually
// save it as "test.wrl2".  Note that the period should * not * be included in
// the extension.

MString vrml2Translator::defaultExtension () const
{
	return "wrl";
}

// This method tells Maya whether the translator can open and import files
// (returns true) or only import  files (returns false)

bool vrml2Translator::canBeOpened () const
{
	return true;
}

MPxFileTranslator::MFileKind vrml2Translator::identifyFile (
	      const MFileObject & fileName,
	      const char *buffer,
	      short size) const
{
	//Check the buffer for the "VRML2" magic number, the
	// string "#VRML2 V2.0 utf8"

	MFileKind rval = kNotMyFileType;

	if ((unsigned(size) >= magic.length ()) &&
	    (0 == strncmp (buffer, magic.asChar (), magic.length ())))
	{
		rval = kIsMyFileType;
	}
	return rval;
}

MStatus 
initializePlugin (MObject obj)
{
	MStatus         status;

    char            version[256];

    strcpy( version, vrml2Version );
    strcat( version, "-" );
    strcat( version, DtAPIVersion() );

	MFnPlugin       plugin (obj, "VRML2.0 Translator for Maya", version, "Any");

	//Register the translator with the system

	status = plugin.registerFileTranslator ("vrml2",
							"vrml2Translator.rgb",
							vrml2Translator::creator,
							"vrml2TranslatorOpts", "",
							true);
	if (!status)
	{
		status.perror ("registerFileTranslator");
	}

	return status;
}

MStatus 
uninitializePlugin (MObject obj)
{
	MStatus         status;
	MFnPlugin       plugin (obj);

	status = plugin.deregisterFileTranslator ("vrml2");
	if (!status)
	{
		status.perror ("deregisterFileTranslator");
	}

	return status;
}

#ifdef WIN32
#pragma warning(default: 4800 4305)
#endif // WIN32
