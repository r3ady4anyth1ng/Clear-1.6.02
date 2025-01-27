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
// Description: 
//  A simple locator node that demonstrates the transparency flag on
//  locator nodes. Transparent objects must be drawn in a special draw
//  pass by Maya because the rendered output is dependent on the draw
//  order of objects.  The isTransparent() method tells the API that
//  this locator should be placed in this special transparency queue 
//  for rendering.  
//
//  API programmers can see the effects of this draw operation by
//  toggling the 'transparencySort' attribute on this node.  When the
//  attribute is set to true the locator will be drawn in sorted order
//  with all transparent objects in the scene.  When it is set to
//  false it will be drawn with all opaque objects in the scene.
//  
//  This also demonstrates the related drawLast() flag.  This flag allows
//  a locator to be specified as the last object to be drawn in a given
//  refresh pass.  This flag a very specialized purpose, but it can be
//  important when using the isTransparent flag in crucial situations.
//

#include <maya/MPxLocatorNode.h> 
#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnNumericAttribute.h>

#include <arrowData.h> 

class curvedArrows : public MPxLocatorNode
{
public:
	curvedArrows();
	virtual ~curvedArrows(); 

	virtual void			postConstructor(); 
	
    virtual MStatus   		compute( const MPlug& plug, MDataBlock& data );

	virtual void            draw( M3dView & view, const MDagPath & path, 
								  M3dView::DisplayStyle style,
								  M3dView::DisplayStatus status );

	virtual void			drawEdgeLoop( M3dView &, M3dView::DisplayStatus ); 
	
	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const; 
	virtual bool			isTransparent() const; 
	virtual bool			drawLast() const; 

	static  void *          creator();
	static  MStatus         initialize();

	static  MObject			aEnableTransparencySort; 
	static  MObject			aEnableDrawLast; 
	static  MObject			aTheColor; 
	static  MObject			aTransparency; 
	
public: 
	static	MTypeId		id;
};

MTypeId curvedArrows::id( 0x08102B );
MObject curvedArrows::aEnableTransparencySort; 
MObject curvedArrows::aEnableDrawLast;
MObject curvedArrows::aTheColor; 
MObject curvedArrows::aTransparency; 

curvedArrows::curvedArrows() {}
curvedArrows::~curvedArrows() {}

void
curvedArrows::postConstructor() 
{
}

MStatus curvedArrows::compute( const MPlug& /*plug*/, MDataBlock& /*data*/ )
{ 
	return MS::kUnknownParameter;
}

void curvedArrows::drawEdgeLoop( M3dView &view, M3dView::DisplayStatus status )
{
	glPushAttrib( GL_CURRENT_BIT ); 
	if ( status == M3dView::kActive ) {
		view.setDrawColor( 13, M3dView::kActiveColors );
	} else {
		view.setDrawColor( 13, M3dView::kDormantColors );
	}  
	glBegin( GL_LINE_LOOP ); 
	
	unsigned int i; 
	for ( i = 0; i < fsEdgeLoopSize; i ++ ) { 
		glVertex3d( fsVertexList[fsEdgeLoop[i]][0], 
					fsVertexList[fsEdgeLoop[i]][1],
					fsVertexList[fsEdgeLoop[i]][2]);
	}
	glEnd(); 
	glPopAttrib();
}

void curvedArrows::draw( M3dView & view, const MDagPath & /*path*/, 
							 M3dView::DisplayStyle style,
							 M3dView::DisplayStatus status )
{ 
	// Get the size
	//
	MObject thisNode = thisMObject();
	MPlug tPlug = MPlug( thisNode, aTransparency ); 
	MPlug cPlug = MPlug( thisNode, aTheColor ); 

	float r, g, b, a; 
	MObject color; 

	cPlug.getValue( color ); 
	MFnNumericData data( color ); 
	data.getData( r, g, b ); 
	tPlug.getValue( a ); 

	view.beginGL(); 

	if( (style == M3dView::kFlatShaded) ||
	    (style == M3dView::kGouraudShaded) ) {
		// Push the color settings
		// 
		glPushAttrib( GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | 
					  GL_PIXEL_MODE_BIT ); 
	
		if ( a < 1.0f ) { 
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		}
		
		glColor4f( r, g, b, a );			
	
		glBegin( GL_TRIANGLES ); 
		unsigned int i;
		for ( i = 0; i < fsFaceListSize; i ++ ) { 
			unsigned int *vid = fsFaceList[i];
			unsigned int *nid = fsFaceVertexNormalList[i]; 
			for ( unsigned int j = 0; j < 3; j ++ ) { 
				glNormal3d( fsNormalList[nid[j]-1][0], 
							fsNormalList[nid[j]-1][1], 
							fsNormalList[nid[j]-1][2] );
				glVertex3d( fsVertexList[vid[j]-1][0], 
							fsVertexList[vid[j]-1][1],
							fsVertexList[vid[j]-1][2] ); 
			}
		}
		glEnd(); 
	
		glPopAttrib(); 

		drawEdgeLoop( view, status ); 
	} else { 
		drawEdgeLoop( view, status ); 
	}

	view.endGL(); 
}

bool curvedArrows::isTransparent( ) const
{
	MObject thisNode = thisMObject(); 
	MPlug plug( thisNode, aEnableTransparencySort ); 
	bool value; 
	plug.getValue( value ); 
	return value; 
}

bool curvedArrows::drawLast() const
{
    MObject thisNode = thisMObject();
    MPlug plug( thisNode, aEnableDrawLast );
    bool value;
    plug.getValue( value );
    return value;
}

bool curvedArrows::isBounded() const
{ 
	return true;
}

MBoundingBox curvedArrows::boundingBox() const
{   
	MBoundingBox bbox; 
	
	unsigned int i;
	for ( i = 0; i < fsVertexListSize; i ++ ) { 
		double *pt = fsVertexList[i]; 
		bbox.expand( MPoint( pt[0], pt[1], pt[2] ) ); 
	}
	return bbox; 
}

void* curvedArrows::creator()
{
	return new curvedArrows();
}

MStatus curvedArrows::initialize()
{ 
	MFnNumericAttribute nAttr; 

	aTheColor = nAttr.createColor( "theColor", "tc" ); 
	nAttr.setDefault( 0.1, 0.1, 0.8 ); 
	nAttr.setKeyable( true ); 
	nAttr.setReadable( true ); 
	nAttr.setWritable( true ); 
	nAttr.setStorable( true ); 
	
	aTransparency = nAttr.create( "transparency", "t", MFnNumericData::kFloat );
	nAttr.setDefault( 0.5 ); 
	nAttr.setKeyable( true ); 
	nAttr.setReadable( true ); 
	nAttr.setWritable( true ); 
	nAttr.setStorable( true ); 

	aEnableTransparencySort = 
		nAttr.create( "transparencySort", "ts", MFnNumericData::kBoolean ); 
	nAttr.setDefault( true ); 
	nAttr.setKeyable( true ); 
	nAttr.setReadable( true ); 
	nAttr.setWritable( true ); 
	nAttr.setStorable( true ); 

    aEnableDrawLast =
        nAttr.create( "drawLast", "dL", MFnNumericData::kBoolean );
    nAttr.setDefault( false );
    nAttr.setKeyable( true );
    nAttr.setReadable( true );
    nAttr.setWritable( true );
    nAttr.setStorable( true );
                                                                                                                                                    
    MStatus stat1, stat2, stat3, stat4;
    stat1 = addAttribute( aTheColor );
    stat2 = addAttribute( aEnableTransparencySort );
    stat3 = addAttribute( aEnableDrawLast );
    stat4 = addAttribute( aTransparency );

    if ( !stat1 || !stat2 || !stat3  || !stat4) {
        stat1.perror("addAttribute");
        stat2.perror("addAttribute");
        stat3.perror("addAttribute");
        stat4.perror("addAttribute");
        return MS::kFailure;
    }

	return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "6.0", "Any");

	status = plugin.registerNode( "curvedArrows", curvedArrows::id, 
						 &curvedArrows::creator, &curvedArrows::initialize,
						 MPxNode::kLocatorNode );
	if (!status) {
		status.perror("registerNode");
		return status;
	}
	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( curvedArrows::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
