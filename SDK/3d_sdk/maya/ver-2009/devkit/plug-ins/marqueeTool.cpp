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

//////////////////////////////////////////////////////
//
// Marquee selection within a user defined context.
// Draws the marquee using OpenGL.
// Selection is done through the API (MGlobal).
//
//////////////////////////////////////////////////////

#include <maya/MIOStream.h>
#include <math.h>
#include <stdlib.h>

#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MDagPath.h>
 
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>

#include <maya/MPxContextCommand.h>
#include <maya/MPxContext.h>
#include <maya/MEvent.h>

#if defined(OSMac_MachO_)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


//////////////////////////////////////////////
// The user Context
//////////////////////////////////////////////
const char helpString[] =
			"Click with left button or drag with middle button to select";

class marqueeContext : public MPxContext
{
public:
					marqueeContext();
	virtual void	toolOnSetup( MEvent & event );
	virtual MStatus	doPress( MEvent & event );
	virtual MStatus	doDrag( MEvent & event );
	virtual MStatus	doRelease( MEvent & event );
	virtual MStatus	doEnterRegion( MEvent & event );

private:
	short					start_x, start_y;
	short					last_x, last_y;

#ifdef USE_SOFTWARE_OVERLAYS
	short					p_last_x, p_last_y;
	bool					fsDrawn;
#endif

	MGlobal::ListAdjustment	listAdjustment;
	M3dView					view;
};

marqueeContext::marqueeContext()
{
	setTitleString ( "Marquee Tool" );

	// Tell the context which XPM to use so the tool can properly
	// be a candidate for the 6th position on the mini-bar.
	setImage("marqueeTool.xpm", MPxContext::kImage1 );
	
}

void marqueeContext::toolOnSetup ( MEvent & )
{
	setHelpString( helpString );
}

MStatus marqueeContext::doPress( MEvent & event )
//
// Begin marquee drawing (using OpenGL)
// Get the start position of the marquee 
//
{

		// Figure out which modifier keys were pressed, and set up the
	// listAdjustment parameter to reflect what to do with the selected points.
	if (event.isModifierShift() || event.isModifierControl() ) {
		if ( event.isModifierShift() ) {
			if ( event.isModifierControl() ) {
				// both shift and control pressed, merge new selections
				listAdjustment = MGlobal::kAddToList;
			} else {
				// shift only, xor new selections with previous ones
				listAdjustment = MGlobal::kXORWithList;
			}
		} else if ( event.isModifierControl() ) {
			// control only, remove new selections from the previous list
			listAdjustment = MGlobal::kRemoveFromList; 
		}
	} else {
		listAdjustment = MGlobal::kReplaceList;
	}

	// Extract the event information
	//
	event.getPosition( start_x, start_y );

	// Enable OpenGL drawing on viewport
	view = M3dView::active3dView();
	view.beginGL();

#ifdef USE_SOFTWARE_OVERLAYS
	p_last_x = start_x;
	p_last_y = start_y;

	fsDrawn = false;
#else
	// If HW overlays supported then initialize the overlay plane for drawing.
	view.beginOverlayDrawing();
#endif

	return MS::kSuccess;		
}


MStatus marqueeContext::doDrag( MEvent & event )
//
// Drag out the marquee (using OpenGL)
//
{
	
	event.getPosition( last_x, last_y );


#ifdef USE_SOFTWARE_OVERLAYS

	GLboolean depthTest[1];
	GLboolean colorLogicOp[1];
	GLboolean lineStipple[1];

	// Save the state of these 3 attribtes and restore them later.
	glGetBooleanv (GL_DEPTH_TEST, depthTest);
	glGetBooleanv (GL_COLOR_LOGIC_OP, colorLogicOp);
	glGetBooleanv (GL_LINE_STIPPLE, lineStipple);
#endif

	// Turn Line stippling on.
	glLineStipple( 1, 0x5555 );
	glLineWidth( 1.0 );
	glEnable( GL_LINE_STIPPLE );

	// Save the state of the matrix on stack
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();

	// Setup the Orthographic projection Matrix.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
    gluOrtho2D(
    			0.0, (GLdouble) view.portWidth(),
    			0.0, (GLdouble) view.portHeight()
    );
    glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
    glTranslatef(0.375, 0.375, 0.0);

	// Set the draw color
	glIndexi (2);

	// If we are using software overlays then we need to draw the marquee
	// in XOR mode

#ifdef USE_SOFTWARE_OVERLAYS
	
	glDisable (GL_DEPTH_TEST);

	// Enable XOR mode.
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp (GL_XOR);

	// We erase the previously drawn rubber band on the screen by 
	// redrawing it in XOR OpenGL mode.

	if (fsDrawn)
	{
		
		glBegin( GL_LINE_LOOP );
			glVertex2i( start_x, start_y );
			glVertex2i( p_last_x, start_y );
			glVertex2i( p_last_x, p_last_y );
			glVertex2i( start_x, p_last_y );
		glEnd();
		
	}

	fsDrawn = true;
#else
	// If HW overlays enabled then we will clear the overlay plane 
	// so that the previously drawn marquee does not appear on the screen
	// anymore
	view.clearOverlayPlane();

#endif

	// Draw the rectangular marquee
	//
	glBegin( GL_LINE_LOOP );
		glVertex2i( start_x, start_y );
		glVertex2i( last_x, start_y );
		glVertex2i( last_x, last_y );
		glVertex2i( start_x, last_y );
	glEnd();

#ifdef _WIN32
	SwapBuffers( view.deviceContext() );
#elif defined (OSMac_)
	::aglSwapBuffers(view.display()); 
#else
	glXSwapBuffers( view.display(), view.window() );
#endif

	// Restore the state of the matrix from stack
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();


#ifdef USE_SOFTWARE_OVERLAYS
	// Store the current x and y coordinates such that in the next iteration
	// we can erase this marquee by redrawing it in XOR mode.
	p_last_x = last_x;
	p_last_y = last_y;

	// Restore the previous state of these attributes
	if (colorLogicOp[0])
		glEnable (GL_COLOR_LOGIC_OP);
	else
		glDisable (GL_COLOR_LOGIC_OP);

	if (depthTest[0])
		glEnable (GL_DEPTH_TEST);
	else
		glDisable (GL_DEPTH_TEST);

	if (lineStipple[0])
		glEnable( GL_LINE_STIPPLE );
	else
		glDisable( GL_LINE_STIPPLE );
#endif

	return MS::kSuccess;		

}

MStatus marqueeContext::doRelease( MEvent & event )
//
// Selects objects within the marquee box.
{

	MSelectionList			incomingList, marqueeList;

	// Clear the marquee when you release the mouse button

#ifdef USE_SOFTWARE_OVERLAYS

	GLboolean depthTest[1];
	GLboolean colorLogicOp[1];
	GLboolean lineStipple[1];

	event.getPosition( last_x, last_y );

	// Save the state of these 3 attribtes and restore them later.
	glGetBooleanv (GL_DEPTH_TEST, depthTest);
	glGetBooleanv (GL_COLOR_LOGIC_OP, colorLogicOp);
	glGetBooleanv (GL_LINE_STIPPLE, lineStipple);

	// Turn Line stippling on.
	glLineStipple( 1, 0x5555 );
	glLineWidth( 1.0 );
	glEnable( GL_LINE_STIPPLE );

	// Disable GL_DEPTH_TEST
	glDisable (GL_DEPTH_TEST);

	// Enable XOR mode.
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp (GL_INVERT);

	// Save the current Matrix onto the stack
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();

	// Setup the Orthographic projection Matrix.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
    gluOrtho2D(
    			0.0, (GLdouble) view.portWidth(),
    			0.0, (GLdouble) view.portHeight()
    );
    glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
    glTranslatef(0.375, 0.375, 0.0);

	// Set the draw color
	glIndexi (2);

	// Redraw the marquee so that it will be cleared from the screen
	// when the mouse is released.
	glBegin( GL_LINE_LOOP );
		glVertex2i( start_x, start_y );
		glVertex2i( p_last_x, start_y );
		glVertex2i( p_last_x, p_last_y );
		glVertex2i( start_x, p_last_y );
	glEnd();
		

#ifndef _WIN32
    glXSwapBuffers( view.display(), view.window() );
#else
	SwapBuffers( view.deviceContext() );
#endif

	// Restore saved Matrix from stack
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glDisable(GL_COLOR_LOGIC_OP);
	
	// Restore the previous state of these attributes
	if (colorLogicOp[0])
		glEnable (GL_COLOR_LOGIC_OP);
	else
		glDisable (GL_COLOR_LOGIC_OP);

	if (depthTest[0])
		glEnable (GL_DEPTH_TEST);
	else
		glDisable (GL_DEPTH_TEST);

	if (lineStipple[0])
		glEnable( GL_LINE_STIPPLE );
	else
		glDisable( GL_LINE_STIPPLE );

#else
	// If HW overlays enabled, then clear the overlay plane
	// such that the marquee is no longer drawn on screen.
	view.clearOverlayPlane();
	view.endOverlayDrawing();
#endif

	view.endGL();

	// Get the end position of the marquee
	event.getPosition( last_x, last_y );

	// Save the state of the current selections.  The "selectFromSceen"
	// below will alter the active list, and we have to be able to put
	// it back.
	MGlobal::getActiveSelectionList(incomingList);

	// If we have a zero dimension box, just do a point pick
	//
	if ( abs(start_x - last_x) < 2 && abs(start_y - last_y) < 2 ) {
		// This will check to see if the active view is in wireframe or not.
		MGlobal::SelectionMethod selectionMethod = MGlobal::selectionMethod();

		MGlobal::selectFromScreen( start_x, start_y, MGlobal::kReplaceList, selectionMethod );
	} else {
		// The Maya select tool goes to wireframe select when doing a marquee, so
		// we will copy that behaviour.
		// Select all the objects or components within the marquee.
		MGlobal::selectFromScreen( start_x, start_y, last_x, last_y,
								   MGlobal::kReplaceList, 
								   MGlobal::kWireframeSelectMethod );
	}

	// Get the list of selected items
	MGlobal::getActiveSelectionList(marqueeList);

	// Restore the active selection list to what it was before
	// the "selectFromScreen"
	MGlobal::setActiveSelectionList(incomingList, MGlobal::kReplaceList);

	// Update the selection list as indicated by the modifier keys.
	MGlobal::selectCommand(marqueeList, listAdjustment);
	return MS::kSuccess;		
}
MStatus marqueeContext::doEnterRegion( MEvent & )
{
	return setHelpString( helpString );
}

//////////////////////////////////////////////
// Command to create contexts
//////////////////////////////////////////////

class marqueeContextCmd : public MPxContextCommand
{
public:	
						marqueeContextCmd();
	virtual MPxContext*	makeObj();
	static	void*		creator();
};

marqueeContextCmd::marqueeContextCmd() {}

MPxContext* marqueeContextCmd::makeObj()
{
	return new marqueeContext();
}

void* marqueeContextCmd::creator()
{
	return new marqueeContextCmd;
}

//////////////////////////////////////////////
// plugin initialization
//////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
	MStatus		status;
	MFnPlugin	plugin( obj, PLUGIN_COMPANY, "4.5", "Any");

	status = plugin.registerContextCommand( "marqueeToolContext",
										    marqueeContextCmd::creator );
	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus		status;
	MFnPlugin	plugin( obj );

	status = plugin.deregisterContextCommand( "marqueeToolContext" );

	return status;
}
