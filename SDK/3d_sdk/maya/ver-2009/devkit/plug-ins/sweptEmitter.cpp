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

#include <sweptEmitter.h>

#include <maya/MDataHandle.h>
#include <maya/MFnDynSweptGeometryData.h>
#include <maya/MDynSweptLine.h>
#include <maya/MDynSweptTriangle.h>

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



MTypeId sweptEmitter::id( 0x80016 );


sweptEmitter::sweptEmitter()
:	lastWorldPoint(0, 0, 0, 1)
{
}


sweptEmitter::~sweptEmitter()
{
}


void *sweptEmitter::creator()
{
    return new sweptEmitter;
}


MStatus sweptEmitter::initialize()
//
//	Descriptions:
//		Initialize the node, create user defined attributes.
//
{
	return( MS::kSuccess );
}


MStatus sweptEmitter::compute(const MPlug& plug, MDataBlock& block)
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
	MArrayDataHandle hOutArray = block.outputArrayValue(mOutput, &status);
	McheckErr(status, "ERROR in hOutArray = block.outputArrayValue.\n");

	// Create a builder to aid in the array construction efficiently.
	//
	MArrayDataBuilder bOutArray = hOutArray.builder( &status );
	McheckErr(status, "ERROR in bOutArray = hOutArray.builder.\n");

	// Get the appropriate data array that is being currently evaluated.
	//
	MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
	McheckErr(status, "ERROR in hOut = bOutArray.addElement.\n");

    // Get the data and apply the function set.
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
	
	// Apply rotation to the direction vector
	MVector rotatedV = useRotation ( dirV );


	// position,
	MVectorArray inPosAry;
	// velocity
	MVectorArray inVelAry;
	// emission rate
	MIntArray emitCountPP;


	// Get the swept geometry data
	//
	MObject thisObj = this->thisMObject();
	MPlug sweptPlug( thisObj, mSweptGeometry );

	if ( sweptPlug.isConnected() ) 
	{
		MDataHandle sweptHandle = block.inputValue( mSweptGeometry );
		// MObject sweptData = sweptHandle.asSweptGeometry();
		MObject sweptData = sweptHandle.data();
		MFnDynSweptGeometryData fnSweptData( sweptData );


		// Curve emission
		//
		if (fnSweptData.lineCount() > 0) {
			int numLines = fnSweptData.lineCount();
		
			for ( int i=0; i<numLines; i++ )
			{
				inPosAry.clear();
				inVelAry.clear();
				emitCountPP.clear();

				MDynSweptLine line = fnSweptData.sweptLine( i );

				// ... process current line ...
				MVector p1 = line.vertex( 0 );
				MVector p2 = line.vertex( 1 );

				inPosAry.append( p1 );
				inPosAry.append( p2 );

				inVelAry.append( MVector( 0,0,0 ) );
				inVelAry.append( MVector( 0,0,0 ) );

				// emit Rate for two points on line
				emitCountPP.clear();
				status = emitCountPerPoint( plug, block, 2, emitCountPP );

				emit( inPosAry, inVelAry, emitCountPP,
					dt, speed, inheritFactor, rotatedV, fnOutPos, fnOutVel );

			}
		}

		// Surface emission (nurb or polygon)
		//
		if (fnSweptData.triangleCount() > 0) {
			int numTriangles = fnSweptData.triangleCount();
		
			for ( int i=0; i<numTriangles; i++ )
			{
				inPosAry.clear();
				inVelAry.clear();
				emitCountPP.clear();

				MDynSweptTriangle tri = fnSweptData.sweptTriangle( i );

				// ... process current triangle ...
				MVector p1 = tri.vertex( 0 );
				MVector p2 = tri.vertex( 1 );
				MVector p3 = tri.vertex( 2 );

				MVector center = p1 + p2 + p3;
				center /= 3.0;

				inPosAry.append( center );

				inVelAry.append( MVector( 0,0,0 ) );

				// emit Rate for two points on line
				emitCountPP.clear();
				status = emitCountPerPoint( plug, block, 1, emitCountPP );

				emit( inPosAry, inVelAry, emitCountPP,
					dt, speed, inheritFactor, rotatedV, fnOutPos, fnOutVel );

			}
		}
	}

	// Update the data block with new dOutput and set plug clean.
	//
	hOut.set( dOutput );
	block.setClean( plug );

	return( MS::kSuccess );
}


void sweptEmitter::emit
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

MVector sweptEmitter::useRotation ( MVector &direction )
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
		status.perror("sweptEmitter::useRotation: get matrixObject");
		return ( direction );
	}

	MFnMatrixData worldMatrixData( matrixObject, &status );
	if( !status )
	{
		status.perror("sweptEmitter::useRotation: get worldMatrixData");
		return( direction );
	}

	MMatrix worldMatrix = worldMatrixData.matrix( &status );
	if( !status )
	{
		status.perror("sweptEmitter::useRotation: get worldMatrixData.matrix");
		return( direction );
	}

	rotatedVector = direction * worldMatrix;

    return( rotatedVector );
}



MStatus sweptEmitter::emitCountPerPoint
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

	status = plugin.registerNode( "sweptEmitter", sweptEmitter::id,
							&sweptEmitter::creator, &sweptEmitter::initialize,
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

	status = plugin.deregisterNode( sweptEmitter::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}

