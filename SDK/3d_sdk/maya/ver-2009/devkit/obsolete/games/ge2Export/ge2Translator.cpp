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
// GE2.0 Translator Maya specific source
//

#include "ge2Translator.h"

// Initialize our magic "number"
MString ge2Translator::magic ("filetype gx");
// A GE2 file is an ascii file where the 1st line contains 
// the string "filetype gx"

MString ge2Translator::version( "1.6" );

ge2Translator::ge2Translator()
{
}

ge2Translator::~ge2Translator()
{
}

void* ge2Translator::creator()
{
	return new ge2Translator();
}

//
//
// The reader is not implemented yet.
//
MStatus ge2Translator::reader (const MFileObject & file,
								const MString & options,
								MPxFileTranslator::FileAccessMode mode)
{
	MStatus rval (MS::kSuccess);

	return rval;
}

// Write method of the GE2.0 translator / file exporter
MStatus ge2Translator::writer ( const MFileObject & fileObject,
								  const MString & options,
								  MPxFileTranslator::FileAccessMode mode)
{
	char            LTmpStr[MAXPATHLEN];
	unsigned int	i,
					LN;

	// const MString   fname = fileObject.fullName ();
	MString         extension;
	MString         baseFileName;


	// Lets strip off the known extension of .grp if it is there.
	extension.set (".grp");
	int  extLocation = fileObject.name ().rindex ('.');

	if ( (extLocation != -1) && // no '.' in name
		 (extLocation != 0) && // name was ".grp" -- that's ok??
		 (fileObject.name ().substring (extLocation, fileObject.name ().length () - 1) == extension) )
	{
		baseFileName = fileObject.name ().substring (0, extLocation - 1);
	} else
	{
		baseFileName = fileObject.name ();
		extension.clear ();
	}

	geWrapper.setBaseFileName( baseFileName );
	geWrapper.setPathName( fileObject.fullName() );

	geWrapper.pluginVersion = version;

	// Set the directory at the Dt level
	strncpy( LTmpStr, geWrapper.getPathName().asChar(), MAXPATHLEN );
	LN = (int)strlen( LTmpStr );
	if ( LTmpStr[LN - 1] == '/' )
		LTmpStr[LN - 1] = '\0';
	DtSetDirectory( LTmpStr );	

	// in an ideal world, everything in setDefaults() should be overridden
	// with the option parsing. If the mel script doesn't get run for whatever
	// reason, or neglects to return some values, hopefully setDefaults will
	// enable the export to go through anyway
	geWrapper.setDefaults();

// Turn off this pesky warning on NT - performance warning
// for int -> bool conversion.
#ifdef WIN32
#pragma warning ( disable : 4800 )
#endif

	// Lets now do all of the option processing	
	if ( options.length () > 0 )
	{
		//Start parsing.

		MStringArray optionList;
		MStringArray    theOption;

		options.split(';', optionList);

		//break out all the options.

		for ( i = 0; i < optionList.length (); ++i )
		{
			theOption.clear ();
			optionList[i].split( '=', theOption );
			if ( theOption.length () > 1 )
			{
				if ( theOption[0] == MString( "enableAnim" ) )
				{
					geWrapper.enableAnim = (bool) ( theOption[1].asInt() );
				}  else if ( theOption[0] == MString( "animStart" ) )
				{
					geWrapper.frameStart = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animEnd" ) )
				{
					geWrapper.frameEnd = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animStep" ) )
				{
					geWrapper.frameStep = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animVertices" ) )
				{
					geWrapper.animVertices = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animDisplacement" ) )
				{
					if ( theOption[1].asInt() == 1 )
						geWrapper.vertexDisplacement = ge2Wrapper::kVDRelative;
					else
						geWrapper.vertexDisplacement = ge2Wrapper::kVDAbsolute;
				} else if ( theOption[0] == MString( "animTransforms" ) )
				{
					geWrapper.animTransforms = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animShaders" ) )
				{
					geWrapper.animShaders = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animLights" ) )
				{
					geWrapper.animLights = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "animCamera" ) )
				{
					geWrapper.animCamera = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "keyCurves" ) )
				{
					geWrapper.keyCurves = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "keySample" ) )
				{
					geWrapper.keySample = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "sampRate" ) )
				{
					geWrapper.sampleRate = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "sampTol" ) )
				{
					geWrapper.sampleTolerance = (float) ( theOption[1].asFloat() );
				} else if ( theOption[0] == MString( "useGL" ) )
				{
					geWrapper.useDomainGL = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "usePSX" ) )
				{
					geWrapper.useDomainPSX = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "useN64" ) )
				{
					geWrapper.useDomainN64 = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "useCustom" ) )
				{
					geWrapper.useDomainCustom = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "hrcType" ) )
				{
					geWrapper.hrcMode = static_cast <ge2Wrapper::GEHrcMode> ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportSel" ) )
				{					
					geWrapper.selType = static_cast <ge2Wrapper::GESelType> ( theOption[1].asInt() );
					if ( (mode == MPxFileTranslator::kExportActiveAccessMode) &&
						 (geWrapper.selType == ge2Wrapper::kSelAll) )
					{
						geWrapper.selType = ge2Wrapper::kSelActive;
					}
				} else if ( theOption[0] == MString( "exportLights" ) )
				{
					geWrapper.outputLights = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportCamera" ) )
				{
					geWrapper.outputCamera = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportJoints" ) )
				{
					geWrapper.outputJoints = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportNormals" ) )
				{
					geWrapper.outputNormals = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "opposite" ) )
				{
					geWrapper.oppositeNormals = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportGeometry" ) )
				{ 
					geWrapper.outputGeometry = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "reverse" ) )
				{
					geWrapper.reverseWinding = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "exportTextures" ) )
				{
					geWrapper.outputTextures = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "tesselation" ) )
				{
					if ( theOption[1].asInt() == 2 )
						DtExt_setTesselate( kTESSQUAD );
					else
						DtExt_setTesselate( kTESSTRI );
				} else if ( theOption[0] == MString( "texsample" ) )
				{
					geWrapper.sampleTextures = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "texevaluate" ) )
				{
					geWrapper.evalTextures = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "texOriginal" ) )
				{
					geWrapper.useOriginalFileTextures = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "Xtexres" ) )
				{
					geWrapper.xTexRes = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "Ytexres" ) )
				{
					geWrapper.yTexRes = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "MaxXtexres" ) )
				{
					geWrapper.xMaxTexRes = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "MaxYtexres" ) )
				{
					geWrapper.yMaxTexRes = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "texType" ) )
				{
					geWrapper.texType = theOption[1];
				} else if ( theOption[0] == MString( "precision" ) )
				{
					geWrapper.precision = (int) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "format" ) )
				{
					geWrapper.useTabs = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "comments" ) )
				{
					geWrapper.writeComments = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "verboseGeom" ) )
				{
					geWrapper.verboseGeom = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "verboseLgt" ) )
				{
					geWrapper.verboseLgt = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "verboseCam" ) )
				{
					geWrapper.verboseCam = (bool) ( theOption[1].asInt() );
				} else if ( theOption[0] == MString( "script" ) )
				{
					geWrapper.userScript = theOption[1];
				} else if ( theOption[0] == MString( "scriptAppend" ) )
				{
					geWrapper.scriptAppendFileName = (int) ( theOption[1].asInt() );
				}
			}
		}
	}

#ifdef WIN32
#pragma warning ( default : 4800 )
#endif

	geWrapper.initScene(); // do some initialization
	geWrapper.writeScene(); // write it to the appropriate files
	geWrapper.killScene(); // clean-up

	return MS::kSuccess;
}

MPxFileTranslator::MFileKind ge2Translator::identifyFile (
	      const MFileObject & fileName,
	      const char *buffer,
	      short size) const
{
	//Check the buffer for the "GE2" magic number, the
	// string "filetype gx"
	MFileKind rval = kNotMyFileType;

	if ((size >= (short) magic.length ()) &&
	    (0 == strncmp (buffer, magic.asChar (), magic.length ())))
	{
		rval = kIsMyFileType;
	}
	return rval;
}


// Shouldn't be any C functions trying to call us, should there?
/*extern "C" */MStatus initializePlugin ( MObject obj )
{
	MStatus         status;
	MFnPlugin       plugin( obj, "Alias", ge2Translator::getVersion().asChar(), "Any" );

	/*
	// Get Maya-set values and initialize them --
	// if user wants to override them then that's ok
	MTime startFrame( MAnimControl::minTime().value(), MTime::uiUnit() );
	MTime endFrame( MAnimControl::maxTime().value(), MTime::uiUnit() );
	MTime currFrame( MAnimControl::currentTime().value(), MTime::uiUnit() );
	MAnimControl::PlaybackMode playbackMode = MAnimControl::playbackMode();
	MAnimControl::PlaybackViewMode playbackViewMode = MAnimControl::viewMode();
*/

	//Register the translator with the system (NAME, image, creator func. mel script
	status = plugin.registerFileTranslator (
							"GE2",
							"ge2Translator.rgb",
							ge2Translator::creator,
							"ge2TranslatorOpts", 
							"",
							true );
	if (!status)
	{
		status.perror ("registerFileTranslator");
		return status;
	}

	return status;
}

// Shouldn't be any C functions trying to call us, should there?
/*extern "C" */MStatus uninitializePlugin ( MObject obj )
{
	MStatus         status;
	MFnPlugin       plugin (obj);

	status = plugin.deregisterFileTranslator ("GE2");
	if (!status)
	{
		status.perror ("deregisterFileTranslator");
		return status;
	}

	return status;
}
