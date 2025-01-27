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

#include <maya/MIOStream.h>
#include <math.h>
#include <stdlib.h>

#include <simpleEmitter.h>

#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnMatrixData.h>


MTypeId simpleEmitter::id( 0x80014 );


simpleEmitter::simpleEmitter()
:	lastWorldPoint(0, 0, 0, 1)
{
}


simpleEmitter::~simpleEmitter()
{
}


void *simpleEmitter::creator()
{
    return new simpleEmitter;
}


MStatus simpleEmitter::initialize()
//
//	Descriptions:
//		Initialize the node, create user defined attributes.
//
{
	return( MS::kSuccess );
}


MStatus simpleEmitter::compute(const MPlug& plug, MDataBlock& block)
//
//	Descriptions:
//		Call emit emit method to generate new particles.
//
{
	MStatus status;

	// Determine if we are requesting the output plug for this emitter node.
	//
	if( !(plug == mOutput) )
        return( MS::kUnknownParameter );

	// Get the logical index of the element this plug refers to,
	// because the node can be emitting particles into more 
    // than one particle shape.
	//
	int multiIndex = plug.logicalIndex( &status );
	McheckErr(status, "ERROR in plug.logicalIndex.\n");

	// Get output data arrays (position, velocity, or parentId)
	// that the particle shape is holding from the previous frame.
	//
	MArrayDataHandle hOutArray = block.outputArrayValue( mOutput, &status);
	McheckErr(status, "ERROR in hOutArray = block.outputArrayValue.\n");

	// Create a builder to aid in the array construction efficiently.
	//
	MArrayDataBuilder bOutArray = hOutArray.builder( &status );
	McheckErr(status, "ERROR in bOutArray = hOutArray.builder.\n");

	// Get the appropriate data array that is being currently evaluated.
	//
	MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
	McheckErr(status, "ERROR in hOut = bOutArray.addElement.\n");

	// Create the data and apply the function set,
	// particle array initialized to length zero, 
	// fnOutput.clear()
	//
	MFnArrayAttrsData fnOutput;
	MObject dOutput = fnOutput.create ( &status );
	McheckErr(status, "ERROR in fnOutput.create.\n");

	// Check if the particle object has reached it's maximum,
	// hence is full. If it is full then just return with zero particles.
	//
	bool beenFull = isFullValue( multiIndex, block );
	if( beenFull )
	{
		return( MS::kSuccess );
	}

	// Get input position and velocity arrays where new particles are from,
	// also known as the owner. An owner is determined if connections exist
	// to the emitter node from a shape such as nurbs, polymesh, curve, 
	// or a lattice shape.
	//

	// Get a single position from world transform
	//
	MVectorArray inPosAry;
	inPosAry.clear();

	MPoint worldPos(0.0, 0.0, 0.0);
	status = getWorldPosition( worldPos );
	MVector worldV;
	worldV[0] = worldPos[0];
	worldV[1] = worldPos[1];
	worldV[2] = worldPos[2];
	inPosAry.append( worldV );

	// Create a single velocity
	MVectorArray inVelAry;
	inVelAry.clear();
	MVector velocity(0,0,0);
	inVelAry.append( velocity );

	// Get deltaTime, currentTime and startTime.
	// If deltaTime <= 0.0, or currentTime <= startTime,
	// do not emit new pariticles and return.
	//
	MTime cT = currentTimeValue( block );
	MTime sT = startTimeValue( multiIndex, block );
	MTime dT = deltaTimeValue( multiIndex, block );
	if( (cT <= sT) || (dT <= 0.0) )
	{
		// We do not emit particles before the start time, 
		// and do not emit particles when moving backwards in time.
		// 

		// This code is necessary primarily the first time to 
		// establish the new data arrays allocated, and since we have 
		// already set the data array to length zero it does 
		// not generate any new particles.
		// 
		hOut.set( dOutput );
		block.setClean( plug );

		return( MS::kSuccess );
	}

	// Compute and store an emission rate
	//
	MIntArray emitCountPP;
	emitCountPP.clear();

    int plugIndex = plug.logicalIndex( &status );

    // Get rate and delta time.
    //
    double rate = rateValue( block );
    MTime dtRate = deltaTimeValue( plugIndex, block );
    double dblCount = rate * dtRate.as( MTime::kSeconds );
    int intCount = (int)dblCount;
	emitCountPP.append( intCount );


	// Get speed, direction vector, and inheritFactor attributes.
	//
	double speed = speedValue( block );
	MVector dirV = directionVector( block );
	double inheritFactor = inheritFactorValue( multiIndex, block );

	// Get the position, velocity, and normalized time arrays to append new particle data.
	//
	MVectorArray fnOutPos = fnOutput.vectorArray("position", &status);
	MVectorArray fnOutVel = fnOutput.vectorArray("velocity", &status);
	MDoubleArray fnOutTime = fnOutput.doubleArray("timeInStep", &status);

	// Convert deltaTime into seconds.
	//
	double dt = dT.as( MTime::kSeconds );

	// Rotate the direction attribute by world transform
	MVector rotatedV = useRotation ( dirV );

	// Start emitting particles.
	//
	emit( inPosAry, inVelAry, emitCountPP,
			dt, speed, inheritFactor, rotatedV, fnOutPos, fnOutVel, fnOutTime );

	// Update the data block with new dOutput and set plug clean.
	//
	hOut.set( dOutput );
	block.setClean( plug );

	return( MS::kSuccess );
}


void simpleEmitter::emit
	(
		const MVectorArray &inPosAry,	// points where new particles from
		const MVectorArray &inVelAry,	// initial velocity of new particles
		const MIntArray &emitCountPP,	// # of new particles per point
		double dt,						// elapsed time
		double speed,					// speed factor
		double inheritFactor,			// for inherit velocity
		MVector dirV,					// emit direction
		MVectorArray &outPosAry,		// holding new particles position
		MVectorArray &outVelAry,		// holding new particles velocity
		MDoubleArray &outTimeAry		// holding new particles emitted time
	)
//
//	Descriptions:
//
{
	// check the length of input arrays.
	//
	int posLength = inPosAry.length();
	int velLength = inVelAry.length();
	int countLength = emitCountPP.length();
	if( (posLength != velLength) || (posLength != countLength) )
		return;

	// Compute total emit count.
	//
	int index;
	int totalCount = 0;
	for( index = 0; index < countLength; index ++ )
		totalCount += emitCountPP[index];

	if( totalCount <= 0 )
		return;

	// Map direction vector into world space and normalize it.
	//
	dirV.normalize();

	// Start emission.
	//
	int emitCount;
	MVector newPos, newVel;
	MVector prePos, sPos, sVel;
	for( index = 0; index < posLength; index++ )
	{
		emitCount = emitCountPP[index];
		if( emitCount <= 0 )
			continue;

		sPos = inPosAry[index];
		sVel = inVelAry[index];
		prePos = sPos - sVel * dt;

		for( int i = 0; i < emitCount; i++ )
		{
			double alpha = ( (double)i + drand48() ) / (double)emitCount;
			newPos = (1 - alpha) * prePos + alpha * sPos;
			newVel = dirV * speed;

			newPos += newVel * ( dt * (1 - alpha) );
			newVel += sVel * inheritFactor;

			// Add new data into output arrays.
			//
			outPosAry.append( newPos );
			outVelAry.append( newVel );
			outTimeAry.append( alpha );
		}
	}
}


MStatus simpleEmitter::getWorldPosition( MPoint &point )
//
//	Descriptions:
//		get the emitter position in the world space.
//		The position value is from inherited attribute, aWorldMatrix.
//
{
	MStatus status;

	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode( thisNode );

	// get worldMatrix attribute.
	//
	MObject worldMatrixAttr = fnThisNode.attribute( "worldMatrix" );

	// build worldMatrix plug, and specify which element the plug refers to.
	// We use the first element(the first dagPath of this emitter).
	//
	MPlug matrixPlug( thisNode, worldMatrixAttr );
	matrixPlug = matrixPlug.elementByLogicalIndex( 0 );

	// Get the value of the 'worldMatrix' attribute
	//
	MObject matrixObject;
	status = matrixPlug.getValue( matrixObject );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get matrixObject");
		return( status );
	}

	MFnMatrixData worldMatrixData( matrixObject, &status );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get worldMatrixData");
		return( status );
	}

	MMatrix worldMatrix = worldMatrixData.matrix( &status );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get worldMatrix");
		return( status );
	}

	// assign the translate to the given vector.
	//
	point[0] = worldMatrix( 3, 0 );
	point[1] = worldMatrix( 3, 1 );
	point[2] = worldMatrix( 3, 2 );

    return( status );
}


MStatus simpleEmitter::getWorldPosition( MDataBlock& block, MPoint &point )
//
//	Descriptions:
//		Find the emitter position in the world space.
//
{
    MStatus status;

	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode( thisNode );

	// get worldMatrix attribute.
	//
	MObject worldMatrixAttr = fnThisNode.attribute( "worldMatrix" );

	// build worldMatrix plug, and specify which element the plug refers to.
	// We use the first element(the first dagPath of this emitter).
	//
	MPlug matrixPlug( thisNode, worldMatrixAttr );
	matrixPlug = matrixPlug.elementByLogicalIndex( 0 );
    MDataHandle hWMatrix = block.inputValue( matrixPlug, &status );

	McheckErr(status, "ERROR getting hWMatrix from dataBlock.\n");

    if( status == MS::kSuccess )
    {
        MMatrix wMatrix = hWMatrix.asMatrix();
        point[0] = wMatrix(3, 0);
        point[1] = wMatrix(3, 1);
        point[2] = wMatrix(3, 2);
    }
    return( status );
}

MVector simpleEmitter::useRotation ( MVector &direction )
{
	MStatus status;
	MVector rotatedVector;

	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode( thisNode );

	// get worldMatrix attribute.
	//
	MObject worldMatrixAttr = fnThisNode.attribute( "worldMatrix" );

	// build worldMatrix plug, and specify which element the plug refers to.
	// We use the first element(the first dagPath of this emitter).
	//
	MPlug matrixPlug( thisNode, worldMatrixAttr );
	matrixPlug = matrixPlug.elementByLogicalIndex( 0 );

	// Get the value of the 'worldMatrix' attribute
	//
	MObject matrixObject;
	status = matrixPlug.getValue( matrixObject );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get matrixObject");
		return ( direction );
	}

	MFnMatrixData worldMatrixData( matrixObject, &status );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get worldMatrixData");
		return( direction );
	}

	MMatrix worldMatrix = worldMatrixData.matrix( &status );
	if( !status )
	{
		status.perror("simpleEmitter::getWorldPosition: get worldMatrix");
		return( direction );
	}

	rotatedVector = direction * worldMatrix;

    return( rotatedVector );
}

#define TORUS_PI 3.14159265
#define TORUS_2PI 2*TORUS_PI
#define EDGES 30
#define SEGMENTS 20

//
//	Descriptions:
//		Draw a set of rings to symbolie the field. This does not override default icon, you can do that by implementing the iconBitmap() function
//

void simpleEmitter::draw( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus )
{
	view.beginGL();
	for (int j = 0; j < SEGMENTS; j++ )
	{
		glPushMatrix();
		glRotatef( GLfloat(360 * j / SEGMENTS), 0.0, 1.0, 0.0 );
		glTranslatef( 1.5, 0.0, 0.0 );

		for (int i = 0; i < EDGES; i++ )
		{
			glBegin(GL_LINE_STRIP);
			float p0 = float( TORUS_2PI * i / EDGES );
			float p1 = float( TORUS_2PI * (i+1) / EDGES );
			glVertex2f( cos(p0), sin(p0) );
			glVertex2f( cos(p1), sin(p1) );
			glEnd();
		}
		glPopMatrix();
	}
	view.endGL ();
}


MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "simpleEmitter", simpleEmitter::id,
							&simpleEmitter::creator, &simpleEmitter::initialize,
							MPxNode::kEmitterNode );
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

	status = plugin.deregisterNode( simpleEmitter::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}



