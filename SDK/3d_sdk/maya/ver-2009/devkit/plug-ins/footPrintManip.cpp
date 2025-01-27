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

////////////////////////////////////////////////////////////////////////
// 
// footPrintManip.cpp
// 
// This plug-in demonstrates how to use the Show Manip Tool with
// a user-defined manipulator.
//
// This is the script for running this plug-in:
// 
// loadPlugin "footPrintManip.so";
// createNode footPrintLocator -n f1;
// 
// Now click on the showManipTool!
// 
////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>

#include <maya/MPxNode.h>   

#include <maya/MPxLocatorNode.h> 

#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MFnPlugin.h>
#include <maya/MDistance.h>
#include <maya/MFnUnitAttribute.h>

#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxManipContainer.h> 
#include <maya/MFnDistanceManip.h> 
#include <maya/MPxContext.h>
#include <maya/MPxSelectionContext.h>

#include <maya/MFnNumericData.h>
#include <maya/MManipData.h>

// Foot Data
//
static float sole[][3] = { {  0.00f, 0.0f, -0.70f},
						   {  0.04f, 0.0f, -0.69f },
						   {  0.09f, 0.0f, -0.65f },
						   {  0.13f, 0.0f, -0.61f }, 
						   {  0.16f, 0.0f, -0.54f },
						   {  0.17f, 0.0f, -0.46f },
						   {  0.17f, 0.0f, -0.35f },
						   {  0.16f, 0.0f, -0.25f },
						   {  0.15f, 0.0f, -0.14f },
						   {  0.13f, 0.0f,  0.00f },
						   {  0.00f, 0.0f,  0.00f },
						   { -0.13f, 0.0f,  0.00f },
						   { -0.15f, 0.0f, -0.14f },
						   { -0.16f, 0.0f, -0.25f },
						   { -0.17f, 0.0f, -0.35f },
						   { -0.17f, 0.0f, -0.46f }, 
						   { -0.16f, 0.0f, -0.54f },
						   { -0.13f, 0.0f, -0.61f },
						   { -0.09f, 0.0f, -0.65f },
						   { -0.04f, 0.0f, -0.69f }, 
						   { -0.00f, 0.0f, -0.70f } }; 
static float heel[][3] = { {  0.00f, 0.0f,  0.06f },
						   {  0.13f, 0.0f,  0.06f },
						   {  0.14f, 0.0f,  0.15f },
						   {  0.14f, 0.0f,  0.21f }, 
						   {  0.13f, 0.0f,  0.25f },
						   {  0.11f, 0.0f,  0.28f },
						   {  0.09f, 0.0f,  0.29f },
						   {  0.04f, 0.0f,  0.30f },
						   {  0.00f, 0.0f,  0.30f },
						   { -0.04f, 0.0f,  0.30f },
						   { -0.09f, 0.0f,  0.29f },
						   { -0.11f, 0.0f,  0.28f }, 
						   { -0.13f, 0.0f,  0.25f },
						   { -0.14f, 0.0f,  0.21f },
						   { -0.14f, 0.0f,  0.15f },
						   { -0.13f, 0.0f,  0.06f },  
						   { -0.00f, 0.0f,  0.06f } }; 
static int heelCount = 17;
static int soleCount = 21;
 

class footPrintLocatorManip : public MPxManipContainer
{
public:
    footPrintLocatorManip();
    virtual ~footPrintLocatorManip();
    
    static void * creator();
    static MStatus initialize();
    virtual MStatus createChildren();
    virtual MStatus connectToDependNode(const MObject & node);

    virtual void draw(M3dView & view, 
					  const MDagPath & path, 
					  M3dView::DisplayStyle style,
					  M3dView::DisplayStatus status);
	MManipData startPointCallback(unsigned index) const;
	MVector nodeTranslation() const;

    MDagPath fDistanceManip;
	MDagPath fNodePath;

public:
    static MTypeId id;
};


MManipData footPrintLocatorManip::startPointCallback(unsigned index) 
const
{
	MFnNumericData numData;
	MObject numDataObj = numData.create(MFnNumericData::k3Double);
	MVector vec = nodeTranslation();
	numData.setData(vec.x, vec.y, vec.z);
	return MManipData(numDataObj);
}


MVector footPrintLocatorManip::nodeTranslation() const
{
	MFnDagNode dagFn(fNodePath);
	MDagPath path;
	dagFn.getPath(path);
	path.pop();  // pop from the shape to the transform
	MFnTransform transformFn(path);
	return transformFn.translation(MSpace::kWorld);
}


MTypeId footPrintLocatorManip::id( 0x8001b );


footPrintLocatorManip::footPrintLocatorManip() 
{ 
    // Do not call createChildren from here 
}


footPrintLocatorManip::~footPrintLocatorManip() 
{
}


void* footPrintLocatorManip::creator()
{
     return new footPrintLocatorManip();
}


MStatus footPrintLocatorManip::initialize()
{ 
    MStatus stat;
    stat = MPxManipContainer::initialize();
    return stat;
}


MStatus footPrintLocatorManip::createChildren()
{
    MStatus stat = MStatus::kSuccess;

    MString manipName("distanceManip");
    MString distanceName("distance");

    MPoint startPoint(0.0, 0.0, 0.0);
    MVector direction(0.0, 1.0, 0.0);
    fDistanceManip = addDistanceManip(manipName,
									  distanceName);
	MFnDistanceManip distanceManipFn(fDistanceManip);
	distanceManipFn.setStartPoint(startPoint);
	distanceManipFn.setDirection(direction);
	
    return stat;
}


MStatus footPrintLocatorManip::connectToDependNode(const MObject &node)
{
    MStatus stat;

	// Get the DAG path
	//
	MFnDagNode dagNodeFn(node);
	dagNodeFn.getPath(fNodePath);

    // Connect the plugs
    //    

    MFnDistanceManip distanceManipFn(fDistanceManip);
    MFnDependencyNode nodeFn(node);    

	MPlug sizePlug = nodeFn.findPlug("size", &stat);
    if (MStatus::kFailure != stat) {
	    distanceManipFn.connectToDistancePlug(sizePlug);
		unsigned startPointIndex = distanceManipFn.startPointIndex();
	    addPlugToManipConversionCallback(startPointIndex, 
										 (plugToManipConversionCallback) 
										 &footPrintLocatorManip::startPointCallback);
		finishAddingManips();
	    MPxManipContainer::connectToDependNode(node);
	}

    return stat;
}


void footPrintLocatorManip::draw(M3dView & view, 
								 const MDagPath &path, 
								 M3dView::DisplayStyle style,
								 M3dView::DisplayStatus status)
{ 
    MPxManipContainer::draw(view, path, style, status);
    view.beginGL(); 

    MPoint textPos = nodeTranslation();
    char str[100];
    sprintf(str, "Stretch Me!"); 
    MString distanceText(str);
    view.drawText(distanceText, textPos, M3dView::kLeft);

    view.endGL();
}


class footPrintLocator : public MPxLocatorNode
{
public:
	footPrintLocator();
	virtual ~footPrintLocator(); 

    virtual MStatus   		compute(const MPlug& plug, MDataBlock &data);

	virtual void            draw(M3dView &view, const MDagPath &path, 
								 M3dView::DisplayStyle style,
								 M3dView::DisplayStatus status);

	virtual bool            isBounded() const;
	virtual MBoundingBox    boundingBox() const; 

	static  void *          creator();
	static  MStatus         initialize();

	static  MObject         size;         // The size of the foot

public: 
	static	MTypeId		id;
};


MTypeId footPrintLocator::id( 0x8001c );
MObject footPrintLocator::size;


footPrintLocator::footPrintLocator() 
{}


footPrintLocator::~footPrintLocator() 
{}


MStatus footPrintLocator::compute(const MPlug &plug, MDataBlock &data)
{ 
	return MS::kUnknownParameter;
}


void footPrintLocator::draw(M3dView &view, const MDagPath &path, 
							M3dView::DisplayStyle style,
							M3dView::DisplayStatus status)
{ 
	// Get the size
	//
	MObject thisNode = thisMObject();
	MPlug plug(thisNode, size);
	MDistance sizeVal;
	plug.getValue(sizeVal);

	float multiplier = (float) sizeVal.asCentimeters();

	view.beginGL(); 


	if ((style == M3dView::kFlatShaded) || 
		(style == M3dView::kGouraudShaded)) 
	{  
		// Push the color settings
		// 
		glPushAttrib(GL_CURRENT_BIT);

		if (status == M3dView::kActive) {
			view.setDrawColor(13, M3dView::kActiveColors);
		} else {
			view.setDrawColor(13, M3dView::kDormantColors);
		}  

		glBegin(GL_TRIANGLE_FAN);
	        int i;
			int last = soleCount - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(sole[i][0] * multiplier,
						   sole[i][1] * multiplier,
						   sole[i][2] * multiplier);
			} 
		glEnd();
		glBegin(GL_TRIANGLE_FAN);
			last = heelCount - 1;
			for (i = 0; i < last; ++i) {
				glVertex3f(heel[i][0] * multiplier,
						   heel[i][1] * multiplier,
						   heel[i][2] * multiplier);
			}
		glEnd();

		glPopAttrib();
	}

	// Draw the outline of the foot
	//
	glBegin(GL_LINES);
	    int i;
	    int last = soleCount - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(sole[i][0] * multiplier, 
					   sole[i][1] * multiplier, 
					   sole[i][2] * multiplier);
			glVertex3f(sole[i+1][0] * multiplier, 
					   sole[i+1][1] * multiplier, 
					   sole[i+1][2] * multiplier);
		} 
		last = heelCount - 1;
	    for (i = 0; i < last; ++i) { 
			glVertex3f(heel[i][0] * multiplier, 
					   heel[i][1] * multiplier, 
					   heel[i][2] * multiplier);
			glVertex3f(heel[i+1][0] * multiplier, 
					   heel[i+1][1] * multiplier, 
					   heel[i+1][2] * multiplier);
		}
	glEnd();

	view.endGL();
}


bool footPrintLocator::isBounded() const
{ 
	return true;
}


MBoundingBox footPrintLocator::boundingBox() const
{   
	// Get the size
	//
	MObject thisNode = thisMObject();
	MPlug plug(thisNode, size);
	MDistance sizeVal;
	plug.getValue(sizeVal);

	double multiplier = sizeVal.asCentimeters();
 
	MPoint corner1(-0.17, 0.0, -0.7);
	MPoint corner2(0.17, 0.0, 0.3);

	corner1 = corner1 * multiplier;
	corner2 = corner2 * multiplier;

	return MBoundingBox(corner1, corner2);
}


void* footPrintLocator::creator()
{
	return new footPrintLocator();
}


MStatus footPrintLocator::initialize()
{ 
	MFnUnitAttribute unitFn;
	MStatus			 stat;

	size = unitFn.create("size", "sz", MFnUnitAttribute::kDistance);
	unitFn.setDefault(10.0);
	unitFn.setStorable(true);
	unitFn.setWritable(true);

	stat = addAttribute(size);
	if (!stat) {
		stat.perror("addAttribute");
		return stat;
	}
	
	MPxManipContainer::addToManipConnectTable(id);

	return MS::kSuccess;
}


MStatus initializePlugin(MObject obj)
{ 
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode("footPrintLocator", 
								 footPrintLocator::id, 
								 &footPrintLocator::creator, 
								 &footPrintLocator::initialize,
								 MPxNode::kLocatorNode);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

    status = plugin.registerNode("footPrintLocatorManip", 
								 footPrintLocatorManip::id, 
								 &footPrintLocatorManip::creator, 
								 &footPrintLocatorManip::initialize,
								 MPxNode::kManipContainer);
    if (!status) {
        status.perror("registerNode");
        return status;
    }

	return status;
}


MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(footPrintLocator::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	status = plugin.deregisterNode(footPrintLocatorManip::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
