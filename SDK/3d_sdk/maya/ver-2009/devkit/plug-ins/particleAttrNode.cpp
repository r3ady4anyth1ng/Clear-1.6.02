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
//	Class:	particleAttrNode
//
//	Author:	Lonnie Li
//
//	Description:
//
//		particleAttrNode is an example node that extends the MPxParticleAttributeMapperNode
//	type. These nodes allow us to define custom nodes to map per-particle attribute data
//	to a particle shape.
//
//		In this particular example, we have defined two operations:
//
//		- compute2DTexture()
//		- computeMesh()
//
//		compute2DTexture() replicates the internal behaviour of Maya's internal 'arrayMapper'
//	node at the API level. Given an input texture node and a U/V coord per particle input,
//	this node will evaluate the texture at the given coordinates and map the result back
//	to the outColorPP or outValuePP attributes. See the method description for compute2DTexture()
//	for details on how to setup a proper attribute mapping graph.
//
//		computeMesh() is a user-defined behaviour. It is called when the 'computeNode' attribute
//	is connected to a polygonal mesh. From there, given a particle count, it will map the
//	object space vertex positions of the mesh to a user-defined 'outPositions' vector attribute.
//	This 'outPositions' attribute can then be connected to a vector typed, per-particle attribute
//	on the shape to drive. In our particular example we drive the particle shape's 'rampPosition'
//	attribute. For further details, see the method description for computeMesh() to setup
//	this example.
//

#include <particleAttrNode.h>

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MPlugArray.h>
#include <maya/MPointArray.h>

#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

#include <maya/MDynamicsUtil.h>

MTypeId particleAttrNode::id = 0x81036;
MObject particleAttrNode::outPositionPP;
MObject particleAttrNode::particleCount;

particleAttrNode::particleAttrNode()
{}

particleAttrNode::~particleAttrNode()
{}

void* particleAttrNode::creator()
{
	return new particleAttrNode();
}

MStatus particleAttrNode::initialize()
{
	MStatus status = MS::kSuccess;

	// In addition to the standard arrayMapper attributes,
	// create an additional vector attribute.
	//
	MFnTypedAttribute typedAttrFn;
	MVectorArray defaultVectArray;
	MFnVectorArrayData vectArrayDataFn;
	vectArrayDataFn.create( defaultVectArray );
	typedAttrFn.create( "outPosition",
						"opos",
						MFnData::kVectorArray,
						vectArrayDataFn.object(),
						&status );
	// Transient output attribute. Not writable nor storable.
	//
	typedAttrFn.setWritable(false);
	typedAttrFn.setStorable(false);
	outPositionPP = typedAttrFn.object();
	addAttribute( outPositionPP );

	MFnNumericAttribute numAttrFn;
	numAttrFn.create( "particleCount", "pc", MFnNumericData::kInt );
	particleCount = numAttrFn.object();
	addAttribute( particleCount );

	attributeAffects( computeNode, outPositionPP );
	attributeAffects( particleCount, outPositionPP );

	return status;
}

MStatus particleAttrNode::compute( const MPlug& plug, MDataBlock& block )
{
	MStatus status = MS::kUnknownParameter;
	if( (plug.attribute() == outColorPP) || (plug.attribute() == outValuePP) )
	{
		status = compute2DTexture( plug, block );
	}
	else if( plug.attribute() == outPositionPP )
	{
		status = computeMesh( plug, block );
	}
	return status;
}

MStatus particleAttrNode::compute2DTexture( const MPlug& plug, MDataBlock& block )
//
//	Description:
//
//		This computes an outputValue/Color based on input U and V coordinates.
//	The code contained herein replicates the compute code of the internal Maya
//	'arrayMapper' node.
//
//	How to use:
//
//		1) Create a ramp
//		2) Create a particleAttr node
//		3) Create an emitter with a particleShape
//		4) Create a per particle color attribute (rgbPP) on the particleShape via the AE.
//		5) connectAttr ramp1.outColor particleAttr1.computeNode
//		6) connectAttr particleShape1.age particleAttr1.vCoordPP
//		7) connectAttr particleAttr1.outColorPP particleShape1.rgbPP
//
{
	MStatus status = MS::kSuccess;

	//
	// We request the computeNodeColor attributes here to make sure that
	// they get marked clean as the arrayMapper's output attribute(s) get
	// computed.
	//
	/* MDataHandle hComputeNodeColor = */ block.inputValue( computeNodeColor );
	/* MDataHandle hComputeNodeColorR = */ block.inputValue( computeNodeColorR );
	/* MDataHandle hComputeNodeColorG = */ block.inputValue( computeNodeColorG );
	/* MDataHandle hComputeNodeColorB = */ block.inputValue( computeNodeColorB );

	bool doUcoord = false;
	bool doVcoord = false;
	bool doOutColor = ( plug.attribute() == outColorPP );
	bool doOutValue = ( plug.attribute() == outValuePP );

	// Get pointers to the source attributes: aUCoordPP, aVCoordPP.
	//
	MFnDoubleArrayData dataDoubleArrayFn;
	MObject uCoordD = block.inputValue( uCoordPP ).data();
	status = dataDoubleArrayFn.setObject( uCoordD );
	MDoubleArray uAry;
	if( status == MS::kSuccess )
	{
		uAry = dataDoubleArrayFn.array();
		if (uAry.length())
		{
			doUcoord = true;
		}
	}

	status = MS::kSuccess;
	MObject vCoordD = block.inputValue( vCoordPP ).data();
	status = dataDoubleArrayFn.setObject( vCoordD );
	MDoubleArray vAry;
	if( status == MS::kSuccess )
	{
		vAry = dataDoubleArrayFn.array();
		if (vAry.length())
		{
			doVcoord = true;
		}
	}

	// Get pointers to destination attributes: outColorPP, outValuePP.
	//
	status = MS::kSuccess;
	MFnVectorArrayData dataVectorArrayFn;
	MVectorArray outColorAry;
	if( doOutColor )
	{
		MObject colorD = block.outputValue( outColorPP ).data();
		status = dataVectorArrayFn.setObject( colorD );

		if( status == MS::kSuccess )
		{
			outColorAry = dataVectorArrayFn.array();
		}
	}

	status = MS::kSuccess;
	MDoubleArray outValueAry;
	if( doOutValue )
	{
		MObject valueD = block.outputValue( outValuePP ).data();
		status = dataDoubleArrayFn.setObject( valueD );

		if( status == MS::kSuccess )
		{
			outValueAry = dataDoubleArrayFn.array();
		}
	}

	// Setup loop counter.
	//
	unsigned int uCount = ( doUcoord ? uAry.length() : 0 );
	unsigned int vCount = ( doVcoord ? vAry.length() : 0 );
	unsigned int count  = ( (uCount) > (vCount) ? (uCount) : (vCount) );

	// Resize the output arrays to match the max size
	// of the input arrays.
	//
	if( doOutColor ) outColorAry.setLength( count );
	if( doOutValue ) outValueAry.setLength( count );

	// Check for a texture node
	//
	bool hasTextureNode =
		MDynamicsUtil::hasValidDynamics2dTexture( thisMObject(),
												computeNode );

	// Call the texture node to compute color values for
	// the input u,v coordinate arrays.
	//
	if( hasTextureNode )
	{
		MDynamicsUtil::evalDynamics2dTexture( thisMObject(),
											computeNode,
											uAry,
											vAry,
											&outColorAry,
											&outValueAry );

		// Scale the output value and/or color to the min/max range.
		//
		const double& minValue = block.inputValue( outMinValue ).asDouble();
		const double& maxValue = block.inputValue( outMaxValue ).asDouble();

		if( (minValue != 0.0) || (maxValue != 1.0) )
		{
			double r = maxValue - minValue;
			if( doOutValue )
			{
				for( unsigned int i = 0; i < count; i++ )
				{
					outValueAry[i] = (r * outValueAry[i]) + minValue;
				}
			}

			if( doOutColor )
			{
				MVector minVector( minValue, minValue, minValue );
				for( unsigned int i = 0; i < count; i++ )
				{
					outColorAry[i] = (r * outColorAry[i]) + minVector;
				}
			}
		}
	}
	else
	{
		// Not connected to texture node, so simply set output
		// arrays to default values.
		//
		for( unsigned int i = 0; i < count; i++ )
		{
			MVector clr( 0.0, 0.0, 0.0 );
			if( doOutColor )
			{
				outColorAry[i] = clr;
			}
			if( doOutValue )
			{
				outValueAry[i] = clr.x;
			}
		}
	}
	dataVectorArrayFn.set( outColorAry );
	dataDoubleArrayFn.set( outValueAry );
	return MS::kSuccess;
}

MStatus particleAttrNode::computeMesh( const MPlug& plug, MDataBlock& block )
//
//	Description:
//
//		To illustrate a custom use of the arrayMapper node, we'll define this
//	alternative compute that will map particles to mesh vertex positions. If
//	array lengths do not match, we'll wrap around the arrays.
//
//	How to use:
//
//		1) Create a poly surface.
//		2) connectAttr polyShape1.outMesh particleAttr1.computeNode
//		3) connectAttr particleShape1.count particleAttr1.particleCount
//		4) connectAttr particleAttr1.outPosition particleShape1.rampPosition (*)
//
//		* You may choose to drive any vector based per particle attribute.
//		  rampPosition was selected here as an example.
//
{
	MStatus status = MS::kSuccess;

	// Verify that computeNode has a mesh connected to computeNode
	//
	MPlug compPlug( thisMObject(), computeNode );
	MPlugArray conns;
	compPlug.connectedTo( conns, true, false, &status );
	if( conns.length() <= 0 )
	{
		return MS::kFailure;
	}
	MPlug conn = conns[0];
	MObject compNode = conn.node();
	MFnMesh meshFn( compNode, &status );
	if( status != MS::kSuccess )
	{
		return MS::kFailure;
	}

	// Retrieve the mesh vertices
	//
	MPointArray points;
	meshFn.getPoints( points );
	unsigned int nPoints = points.length();

	// Retrieve the current particle count.
	//
	// NOTE: Due to the order in which the particle system requests
	// various pieces of data, some attributes are requested prior
	// to the actual emission of particles (eg. rampPosition), whereas
	// other attributes are requested after particles have been emitted.
	//
	// If the driven PP attribute on the particleShape is requested prior
	// to the emission of particles, this compute() method will be called
	// before any particles have been emitted. As a result, the effect
	// will be lagged by one frame.
	//
	unsigned int nParticles = 0;
	int nSignedPart = block.inputValue( particleCount ).asInt();
	if( nSignedPart > 0 )
	{
		nParticles = nSignedPart;
	}

	// Get pointer to destination attribute: outPositionPP
	//
	MFnVectorArrayData dataVectorArrayFn;
	MVectorArray outPosArray;
	MObject posD = block.outputValue( outPositionPP ).data();
	//const char* typeStr = posD.apiTypeStr();
	status = dataVectorArrayFn.setObject( posD );
	if( status == MS::kSuccess )
	{
		outPosArray = dataVectorArrayFn.array();
	}

	outPosArray.setLength( nParticles );
	for( unsigned int i = 0; i < nParticles; i++ )
	{
		unsigned int index = i;
		if( nParticles > nPoints )
		{
			index = i % nPoints;
		}
		MPoint point = points[index];
		MVector pos( point.x, point.y, point.z );
		outPosArray[i] = pos;
	}
	dataVectorArrayFn.set( outPosArray );
	return MS::kSuccess;
}

