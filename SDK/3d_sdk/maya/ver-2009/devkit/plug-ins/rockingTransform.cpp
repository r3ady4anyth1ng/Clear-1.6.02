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
// Example custom transform:
//	This plug-in implements an example custom transform that
//	can be used to perform a rocking motion around the X axix.
//	Geometry of any rotation can be made a child of this transform
//	to demonstrate the effect.
//	The plug-in contains two pieces:
//	1. The custom transform node -- rockingTransformNode
//	2. The custom transformation matrix -- rockingTransformMatrix
//	These classes are used together in order to implement the
//	rocking motion.  Note that the rock attribute is stored outside
//	of the regular transform attributes.
//
// MEL usage:
/*
	// Create a rocking transform and make a rotated plane
	// its child.
	loadPlugin rockingTransform;
	file -f -new;
	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -tx 1 -ch 1;
	select -r pPlane1 ;
	rotate -r -ws -15 -15 -15 ;
	createNode rockingTransform;
	parent pPlane1 rockingTransform1;
	setAttr rockingTransform1.rockx 55;
*/
//

#include <maya/MPxTransform.h>
#include <maya/MPxTransformationMatrix.h>
#include <maya/MGlobal.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MIOStream.h>

#include "rockingTransform.h"

#ifndef M_PI
#include <math.h>
#endif

//
// Initialize our static class variables
//
MObject rockingTransformNode::aRockInX;
MTypeId rockingTransformNode::id(kRockingTransformNodeID);
MTypeId rockingTransformMatrix::id(kRockingTransformMatrixID);

//
// Implementation of our custom transformation matrix
//

//
//	Constructor for matrix
//
rockingTransformMatrix::rockingTransformMatrix()
{
	rockXValue = 0.0;
}

//
// Creator for matrix
//
void *rockingTransformMatrix::creator()
{
	return new rockingTransformMatrix();
}

//
//	Utility method for getting the rock
//	motion in the X axis
//
double rockingTransformMatrix::getRockInX() const
{
	return rockXValue;
}

//
//	Utility method for setting the rcok 
//	motion in the X axis
//
void rockingTransformMatrix::setRockInX( double rock )
{
	rockXValue = rock;
}

//
// This method will be used to return information to
// Maya.  Use the attributes which are outside of
// the regular transform attributes to build a new
// matrix.  This new matrix will be passed back to
// Maya.
//
MMatrix rockingTransformMatrix::asMatrix() const
{
	// Get the current transform matrix
	MMatrix m = ParentClass::asMatrix();
	// Initialize the new matrix we will calculate
	MTransformationMatrix tm( m );
	// Find the current rotation as a quaternion
	MQuaternion quat = rotation();
	// Convert the rocking value in degrees to radians
	DegreeRadianConverter conv;
	double newTheta = conv.degreesToRadians( getRockInX() );
	quat.setToXAxis( newTheta );
	// Apply the rocking rotation to the existing rotation
	tm.addRotationQuaternion( quat.x, quat.y, quat.z, quat.w, MSpace::kTransform );
	// Let Maya know what the matrix should be
	return tm.asMatrix();
}

MMatrix rockingTransformMatrix::asMatrix(double percent) const
{
	MPxTransformationMatrix m(*this);

	//	Apply the percentage to the matrix components
	MVector trans = m.translation();
	trans *= percent;
	m.translateTo( trans );
	MPoint rotatePivotTrans = m.rotatePivot();
	rotatePivotTrans = rotatePivotTrans * percent;
	m.setRotatePivot( rotatePivotTrans );
	MPoint scalePivotTrans = m.scalePivotTranslation();
	scalePivotTrans = scalePivotTrans * percent;
	m.setScalePivotTranslation( scalePivotTrans );

	//	Apply the percentage to the rotate value.  Same
	// as above + the percentage gets applied
	MQuaternion quat = rotation();
	DegreeRadianConverter conv;
	double newTheta = conv.degreesToRadians( getRockInX() );
	quat.setToXAxis( newTheta );
	m.rotateBy( quat );
	MEulerRotation eulRotate = m.eulerRotation();
	m.rotateTo(  eulRotate * percent, MSpace::kTransform);

	//	Apply the percentage to the scale
	MVector s(scale(MSpace::kTransform));
	s.x = 1.0 + (s.x - 1.0)*percent;
	s.y = 1.0 + (s.y - 1.0)*percent;
	s.z = 1.0 + (s.z - 1.0)*percent;
	m.scaleTo(s, MSpace::kTransform);

	return m.asMatrix();
}

MMatrix	rockingTransformMatrix::asRotateMatrix() const
{
	// To be implemented
	return ParentClass::asRotateMatrix();
}


//
// Implementation of our custom transform
//

//
//	Constructor of the transform node
//
rockingTransformNode::rockingTransformNode()
: ParentClass()
{
	rockXValue = 0.0;
}

//
//	Constructor of the transform node
//
rockingTransformNode::rockingTransformNode(MPxTransformationMatrix *tm)
: ParentClass(tm)
{
	rockXValue = 0.0;
}

//
//	Post constructor method.  Have access to *this.  Node setup
//	operations that do not go into the initialize() method should go
//	here.
//
void rockingTransformNode::postConstructor()
{
	//	Make sure the parent takes care of anything it needs.
	//
	ParentClass::postConstructor();

	// 	The baseTransformationMatrix pointer should be setup properly 
	//	at this point, but just in case, set the value if it is missing.
	//
	if (NULL == baseTransformationMatrix) {
		MGlobal::displayWarning("NULL baseTransformationMatrix found!");
		baseTransformationMatrix = new MPxTransformationMatrix();
	}

	MPlug aRockInXPlug(thisMObject(), aRockInX);
}

//
//	Destructor of the rocking transform
//
rockingTransformNode::~rockingTransformNode()
{
}

//
//	Method that returns the new transformation matrix
//
MPxTransformationMatrix *rockingTransformNode::createTransformationMatrix()
{
	return new rockingTransformMatrix();
}

//
//	Method that returns a new transform node
//
void *rockingTransformNode::creator()
{
	return new rockingTransformNode();
}

//
//	Node initialize method.  We configure node
//	attributes here.  Static method so
//	*this is not available.
//
MStatus rockingTransformNode::initialize()
{
	MFnNumericAttribute numFn;
	aRockInX = numFn.create("RockInX", "rockx", MFnNumericData::kDouble, 0.0);
	numFn.setKeyable(true);
	numFn.setAffectsWorldSpace(true);
	addAttribute(aRockInX);

	//	This is required so that the validateAndSet method is called
	mustCallValidateAndSet(aRockInX);
	return MS::kSuccess;
}

//
//	Debugging method
//
const char* rockingTransformNode::className() 
{
	return "rockingTransformNode";
}

//
//	Reset transformation
//
void  rockingTransformNode::resetTransformation (const MMatrix &matrix)
{
	ParentClass::resetTransformation( matrix );
}

//
//	Reset transformation
//
void  rockingTransformNode::resetTransformation (MPxTransformationMatrix *resetMatrix )
{
	ParentClass::resetTransformation( resetMatrix );
}

//
// A very simple implementation of validAndSetValue().  No lock
// or limit checking on the rocking attribute is done in this method.
// If you wish to apply locks and limits to the rocking attribute, you
// would follow the approach taken in the rockingTransformCheck example.
// Meaning you would implement methods similar to:
//	* applyRotationLocks();
//	* applyRotationLimits();
//	* checkAndSetRotation();  
// but for the rocking attribute.  The method checkAndSetRotation()
// would be called below rather than updating the rocking attribute
// directly.
//
MStatus rockingTransformNode::validateAndSetValue(const MPlug& plug,
												const MDataHandle& handle,
												const MDGContext& context)
{
	MStatus status = MS::kSuccess;

	//	Make sure that there is something interesting to process.
	//
	if (plug.isNull())
		return MS::kFailure;

	MDataBlock block = forceCache(*(MDGContext *)&context);
	MDataHandle blockHandle = block.outputValue(plug, &status);
	ReturnOnError(status);
	
	if ( plug == aRockInX )
	{
		// Update our new rock in x value
		double rockInX = handle.asDouble();
		blockHandle.set(rockInX);
		rockXValue = rockInX;
		
		// Update the custom transformation matrix to the
		// right rock value.  
		rockingTransformMatrix *ltm = getRockingTransformMatrix();
		if (ltm)
			ltm->setRockInX(rockXValue);
		else 
			MGlobal::displayError("Failed to get rock transform matrix");
			
		blockHandle.setClean();
		
		// Mark the matrix as dirty so that DG information
		// will update.
		dirtyMatrix();		
	}
	
	// Allow processing for other attributes
	return ParentClass::validateAndSetValue(plug, handle, context);
}

//
//	Method for returning the current rocking transformation matrix
//
rockingTransformMatrix *rockingTransformNode::getRockingTransformMatrix()
{
	rockingTransformMatrix *ltm = (rockingTransformMatrix *) baseTransformationMatrix;
	return ltm;
}

//
// Utility class
//
double DegreeRadianConverter::degreesToRadians( double degrees )
{
	 return degrees * ( M_PI/ 180.0 );
}
double DegreeRadianConverter::radiansToDegrees( double radians )
{
	return radians * (180.0/M_PI);
}	

