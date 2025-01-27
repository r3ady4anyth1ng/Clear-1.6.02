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

#include <ownerEmitter.h>

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


MTypeId ownerEmitter::id( 0x80015 );


ownerEmitter::ownerEmitter()
:	lastWorldPoint(0, 0, 0, 1)
{
}


ownerEmitter::~ownerEmitter()
{
}


void *ownerEmitter::creator()
{
    return new ownerEmitter;
}


MStatus ownerEmitter::initialize()
//
//	Descriptions:
//		Initialize the node, create user defined attributes.
//
{
	return( MS::kSuccess );
}


MStatus ownerEmitter::compute(const MPlug& plug, MDataBlock& block)
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

	// Check positions from an owner, if an owner exists.
	//
	MVectorArray inPosAry;
	inPosAry.clear();
	ownerPosition( block, inPosAry );

	// Check velocities from an owner, if an owner exists.
	//
	MVectorArray inVelAry;
	inVelAry.clear();
	ownerVelocity( multiIndex, block, inVelAry );

	// inPosAry and inVelAry should have the same length.
	// If not, return a failure.
	//
	unsigned inPosLength = inPosAry.length();
	if( (inPosLength == 0) || (inPosLength != inVelAry.length()) )
	{
		// something is wrong, these two arrays should always 
		// be the same length.
		//
		return( MS::kFailure );
	}

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

	// Compute and store the emission count per-point.
	//
	MIntArray emitCountPP;
	emitCountPP.clear();
	status = emitCountPerPoint( plug, block, inPosLength, emitCountPP );

	// Get speed, direction vector, and inheritFactor attributes.
	//
	double speed = speedValue( block );
	MVector dirV = directionVector( block );
	double inheritFactor = inheritFactorValue( multiIndex, block );

	// Get the position and velocity arrays to append new particle data.
	//
	MVectorArray fnOutPos = fnOutput.vectorArray("position", &status);
	MVectorArray fnOutVel = fnOutput.vectorArray("velocity", &status);

	// Convert deltaTime into seconds.
	//
	double dt = dT.as( MTime::kSeconds );

	
	MVector rotatedV = useRotation ( dirV );
	// Start emitting particles.
	//
	emit( inPosAry, inVelAry, emitCountPP,
			dt, speed, inheritFactor, rotatedV, fnOutPos, fnOutVel );

	// Update the data block with new dOutput and set plug clean.
	//
	hOut.set( dOutput );
	block.setClean( plug );

	return( MS::kSuccess );
}


void ownerEmitter::emit
	(
		const MVectorArray &inPosAry,	// points where new particles from
		const MVectorArray &inVelAry,	// initial velocity of new particles
		const MIntArray &emitCountPP,	// # of new particles per point
		double dt,						// elapsed time
		double speed,					// speed factor
		double inheritFactor,			// for inherit velocity
		MVector dirV,					// emit direction
		MVectorArray &outPosAry,		// holding new particles position
		MVectorArray &outVelAry			// holding new particles velocity
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
		}
	}
}


void ownerEmitter::ownerPosition
	(
		MDataBlock& block,
		MVectorArray &ownerPosArray
	)
//
//	Descriptions:
//		If this emitter has an owner, get the owner's position array or
//		centroid, then assign it to the ownerPosArray.
//		If it does not have owner, get the emitter position in the world
//		space, and assign it to the given array, ownerPosArray.
//
{
	MStatus status;

	bool hasOwner = false;

	MDataHandle hOwnerPos = block.inputValue( mOwnerPosData, &status );
	if( status == MS::kSuccess )
	{
		MObject dOwnerPos = hOwnerPos.data();
		MFnVectorArrayData fnOwnerPos( dOwnerPos );
		MVectorArray posArray = fnOwnerPos.array( &status );
		if( status == MS::kSuccess )
		{
			// assign vectors from block to ownerPosArray.
			//
			for( unsigned int i = 0; i < posArray.length(); i ++ )
				ownerPosArray.append( posArray[i] );

			// Got position array from owner, turn hasOwnerPos on.
			//
			hasOwner = true;
		}
	}

	// If there is not an owner, get the emitter position
	// in the world space and add it into ownerPosArray.
	//
	if( !hasOwner )
	{
		MPoint worldPos(0.0, 0.0, 0.0);
		status = getWorldPosition( worldPos );
		MVector worldV;
		worldV[0] = worldPos[0];
		worldV[1] = worldPos[1];
		worldV[2] = worldPos[2];
		ownerPosArray.append( worldV );
	}
}


void ownerEmitter::ownerVelocity
	(
		int plugIndex,
		MDataBlock& block,
		MVectorArray &ownerVelArray
	)
//
//	Descriptions:
//		If this emitter has an owner, get the owner's velocity array
//		then assign it to the ownerVelArray.  If it does not have owner,
//		get the current emitter position, compute velocity vector, modify
//		lastWorldPoint, and assign it to the given array, ownerVelArray.
//
{
	MStatus status;

	bool hasOwner = false;

	MDataHandle hOwnerVel = block.inputValue( mOwnerVelData, &status );
	if( status == MS::kSuccess )
	{
		MObject dOwnerVel = hOwnerVel.data();
		MFnVectorArrayData fnOwnerVel( dOwnerVel );
		MVectorArray velArray = fnOwnerVel.array( &status );
		if( status == MS::kSuccess )
		{
			// assign vectors from block to ownerPosArray.
			//
			for( unsigned int i = 0; i < velArray.length(); i ++ )
				ownerVelArray.append( velArray[i] );

			// Got position array from owner, turn hasOwnerPos on.
			//
			hasOwner = true;
		}
	}

	// If there is not an owner, get the emitter position
	// in the world space and calcuate velocity and add it 
	// into ownerVelArray.
	if( !hasOwner )
	{
		MVector velV(0.0, 0.0, 0.0);
		MPoint worldPos(0.0, 0.0, 0.0);
		status = getWorldPosition( worldPos );

		MTime dT = deltaTimeValue( plugIndex, block );
		double dt = dT.as( MTime::kSeconds );
		if( dt > 0.0 )
			velV = (worldPos - lastWorldPoint) / dt;

		ownerVelArray.append( velV );

		lastWorldPoint = worldPos;
	}
}



MStatus ownerEmitter::getWorldPosition( MPoint &point )
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
		status.perror("ownerEmitter::getWorldPosition: get matrixObject");
		return( status );
	}

	MFnMatrixData worldMatrixData( matrixObject, &status );
	if( !status )
	{
		status.perror("ownerEmitter::getWorldPosition: get worldMatrixData");
		return( status );
	}

	MMatrix worldMatrix = worldMatrixData.matrix( &status );
	if( !status )
	{
		status.perror("ownerEmitter::getWorldPosition: get worldMatrix");
		return( status );
	}

	// assign the translate to the given vector.
	//
	point[0] = worldMatrix( 3, 0 );
	point[1] = worldMatrix( 3, 1 );
	point[2] = worldMatrix( 3, 2 );

    return( status );
}


MStatus ownerEmitter::getWorldPosition( MDataBlock& block, MPoint &point )
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

MVector ownerEmitter::useRotation ( MVector &direction )
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
		status.perror("ownerEmitter::getWorldPosition: get matrixObject");
		return ( direction );
	}

	MFnMatrixData worldMatrixData( matrixObject, &status );
	if( !status )
	{
		status.perror("ownerEmitter::getWorldPosition: get worldMatrixData");
		return( direction );
	}

	MMatrix worldMatrix = worldMatrixData.matrix( &status );
	if( !status )
	{
		status.perror("ownerEmitter::getWorldPosition: get worldMatrix");
		return( direction );
	}

	rotatedVector = direction * worldMatrix;

    return( rotatedVector );
}



MStatus ownerEmitter::emitCountPerPoint
	(
		const MPlug &plug,
		MDataBlock &block,
		int length,					// length of emitCountPP
		MIntArray &emitCountPP		// output: emitCount for each point
	)
//
//	Descriptions:
//		Compute emitCount for each point where new particles come from.
//
{
	MStatus status;

	int plugIndex = plug.logicalIndex( &status );
	McheckErr(status, "ERROR in emitCountPerPoint: when plug.logicalIndex.\n");

	// Get rate and delta time.
	//
	double rate = rateValue( block );
	MTime dt = deltaTimeValue( plugIndex, block );

	// Compute emitCount for each point.
	//
	double dblCount = rate * dt.as( MTime::kSeconds );

	int intCount = (int)dblCount;
	for( int i = 0; i < length; i++ )
	{
		emitCountPP.append( intCount );
	}

	return( MS::kSuccess );
}


MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "ownerEmitter", ownerEmitter::id,
							&ownerEmitter::creator, &ownerEmitter::initialize,
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

	status = plugin.deregisterNode( ownerEmitter::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}

