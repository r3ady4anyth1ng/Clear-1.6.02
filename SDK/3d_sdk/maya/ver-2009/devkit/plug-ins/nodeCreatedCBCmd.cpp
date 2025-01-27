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

// MEL Command: nodeCreatedCB
//
// Description:
//		A simple plugin to register a mel proc to be called whenever a node is 
//		created. Registered Mel procedures should be declared to take a single
//		string argument (the name of the added node).
//
//		Warning: mel procedures registered with this method will be called 
//				 whenever a node is added to the DG. This may cause problems
//				 in certain cases. For example: each time a reference is 
//				 reloaded, each of its nodes is re-added to the DG.
//
// Flags:
//		-register/-r string: registers a mel callback. There is no limit
//							 to the number of callbacks that can be registered.
//
//		-unregister/-u string: unregister the specified callback.
//
//		-filter/-f string: by default all registered callbacks will be called
//						   whenever any node is created. Using the filter flag
//						   you can indicate that callbacks should only be
//						   called when certain node types are created.
//						   To determine the type of a particular node see the
//						   'objectType' and 'nodeType' commands.
//
//						   Note: only one filter can be in affect at a time,
//						   and it will be applied to all registered callbacks.
//
//		-fullDagPath/-fdp: if this flag is specified when registering a 
//						   callback, any dag node names passed into the mel
//						   procedure will include the full dag path.
//
// Note:
//		The -register, -unregister, and -filter flags are mutually exclusive, 
//		only one should be used per command invocation.
//
// Important:
//		Do not delete the node in your callback.
//
// Example Usage:
//
// // Appends the suffix '_ply' to all created mesh nodes
// //
// global proc myCB( string $node )
// {
//		print("calling polyCB " + $node + "\n");
//		string $type = `objectType $node`;
//		if ( $type == "mesh" ) {
//			rename $node ($node+"_ply");
//		}	
// }
//
// nodeCreatedCB -register "myCB";
//
#include <nodeCreatedCBCmd.h>
#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MDGMessage.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>

#define kRegisterFlagLong		"-register"
#define kRegisterFlag			"-r"
#define kUnregisterFlagLong		"-unregister"
#define kUnregisterFlag			"-u"
#define kFilterFlagLong			"-filter"
#define kFilterFlag				"-f"
#define kFullDagPathFlagLong	"-fullDagPath"
#define kFullDagPathFlag		"-fdp"


// The id of the API callback. The API callback must be removed when this
// plug-in is unloaded.
//
MCallbackId		nodeCreatedCB::sId;
// The array of all registered Mel procedures.
//
MStringArray	nodeCreatedCB::sMelProcs;
// Flags to indicate whether or not a mel procedure should be passed the
// short or long names of dag nodes.
//
MIntArray		nodeCreatedCB::sFullDagPath;


MStatus nodeCreatedCB::doIt( const MArgList& args )
//
//	Description:
//		implements the MEL nodeCreatedCB command.
//
{
	MStatus stat = MS::kSuccess;

	MArgDatabase argData( syntax(), args );

	// Parse command flags.
	//
	if ( argData.isFlagSet( kRegisterFlag ) ) {
		// Register a new procedure.
		//
		MString proc;
		argData.getFlagArgument( kRegisterFlag, 0, proc );
		stat = registerMelProc( proc, argData.isFlagSet( kFullDagPathFlag ) );
	} else if ( argData.isFlagSet( kUnregisterFlag ) ) {
		// Unregister a procedure.
		//
		MString proc;
		argData.getFlagArgument( kUnregisterFlag, 0, proc );
		stat = unregisterMelProc( proc );
	} else if ( argData.isFlagSet( kFilterFlag ) ) {
		// Change the filter being applied.
		//
		MString filter;
		argData.getFlagArgument( kFilterFlag, 0, filter );
		stat = changeFilter( filter );
	}
	
	if ( stat.error() ) {
		MGlobal::displayError( stat.errorString() );
	}

	return stat;
}

MStatus nodeCreatedCB::registerMelProc( MString melProc, bool fullDagPath )
//
// Register a Mel procedure to be called whenever a node is added.
//
{
	MStatus stat = MS::kSuccess;
	if ( melProc.length() == 0 ) {
		// Basic error checking. An empty string is not a valid mel procedure
		// name
		//
		stat = MS::kFailure;
		stat.perror("invalid mel callback: " + melProc);
		return stat;
	}
	nodeCreatedCB::sMelProcs.append( melProc );
	nodeCreatedCB::sFullDagPath.append( fullDagPath );
	return stat;
}

MStatus nodeCreatedCB::unregisterMelProc( MString melProc )
//
// Unregister a Mel procedure.
//
{
	MStatus stat = MS::kFailure;
	int numProcs = nodeCreatedCB::sMelProcs.length();
	for ( int i = 0; i < numProcs; i++ ) {
		// Search the array of registered callbacks for melProc and
		// remove it. If melProc exists more than once, only one instance
		// of it will be removed.
		//
		if ( nodeCreatedCB::sMelProcs[i] == melProc ) {
			nodeCreatedCB::sMelProcs.remove(i);
			nodeCreatedCB::sFullDagPath.remove(i);
			stat = MS::kSuccess;
			break;
		}
	}

	if ( !stat ) {
		// Report an error if melProc was not found.
		//
		stat.perror(melProc + " is not a registered callback.");
	}

	return stat;
}

MStatus nodeCreatedCB::changeFilter( MString filter )
//
// Change the node type filter.
//
{
	MStatus stat = MS::kSuccess;
	// We don't want to add sCallbackFunc more than once as that will cause
	// all registered Mel procs to be called multiple times. So we first
	// remove, then re-add the callback.
	//
	MDGMessage::removeCallback( nodeCreatedCB::sId );
	nodeCreatedCB::sId = MDGMessage::addNodeAddedCallback( nodeCreatedCB::sCallbackFunc, filter );
	return stat;
}

void nodeCreatedCB::sCallbackFunc( MObject& node, void* clientData )
//
// The API callback called whenever a node is added. This function handles
// calling all registered MEL callbacks and passing them the appropriate
// node name argument.
//
{
	int numProcs = nodeCreatedCB::sMelProcs.length();
	for ( int i = 0; i < numProcs; i++ ) {
		MString melCmd = nodeCreatedCB::sMelProcs[i];
		MString nodeName;
		if ( nodeCreatedCB::sFullDagPath[i] && 
			 node.hasFn( MFn::kDagNode ) ) {
			MFnDagNode dagObj( node );
			nodeName = dagObj.fullPathName();
		} else {
			MFnDependencyNode dn( node );
			nodeName = dn.name();
		}
		melCmd += " \"" + nodeName + "\"";
		MGlobal::executeCommand( melCmd );
	}
}

MSyntax	nodeCreatedCB::newSyntax()
//
// Create the syntax object for the nodeCreateCB command.
//
{
	MSyntax syntax;

	syntax.addFlag( kRegisterFlag, kRegisterFlagLong, MSyntax::kString );
	syntax.addFlag( kUnregisterFlag, kUnregisterFlagLong, MSyntax::kString );
	syntax.addFlag( kFilterFlag, kFilterFlagLong, MSyntax::kString );
	syntax.addFlag( kFullDagPathFlag, kFullDagPathFlagLong );

	return syntax;
}
														
void* nodeCreatedCB::creator()										
{																
	return new nodeCreatedCB;										
}
															
MStatus initializePlugin( MObject obj )						
{																
	MFnPlugin	plugin( obj, PLUGIN_COMPANY, "6.0" );				
	MStatus		stat;											
	stat = plugin.registerCommand( "nodeCreatedCB",					
	                                nodeCreatedCB::creator,
									nodeCreatedCB::newSyntax );	    
	if ( !stat )												
		stat.perror("registerCommand");	
	
	// add the API callback.
	//
	nodeCreatedCB::sId = MDGMessage::addNodeAddedCallback( nodeCreatedCB::sCallbackFunc );

	return stat;												
}																
MStatus uninitializePlugin( MObject obj )						
{																
	MFnPlugin	plugin( obj );									
	MStatus		stat;											
	stat = plugin.deregisterCommand( "nodeCreatedCB" );				
	if ( !stat )												
		stat.perror("deregisterCommand");						

	// Remove the API callback, necessary to prevent crashes.
	//
	MDGMessage::removeCallback( nodeCreatedCB::sId );

	nodeCreatedCB::sMelProcs.clear();
	nodeCreatedCB::sFullDagPath.clear();
	
	return stat;												
}

