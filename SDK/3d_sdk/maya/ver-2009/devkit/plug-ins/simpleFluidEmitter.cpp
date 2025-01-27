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

#include <simpleFluidEmitter.h>

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

#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnDynSweptGeometryData.h>
#include <maya/MDynSweptTriangle.h>


MTypeId simpleFluidEmitter::id( 0x81020 );

simpleFluidEmitter::simpleFluidEmitter()
{
}


simpleFluidEmitter::~simpleFluidEmitter()
{
}


void *simpleFluidEmitter::creator()
{
    return new simpleFluidEmitter;
}


MStatus simpleFluidEmitter::initialize()
//
//	Descriptions:
//		Initialize the node, create user defined attributes.
//
{
	return( MS::kSuccess );
}


MStatus simpleFluidEmitter::compute(const MPlug& plug, MDataBlock& block)
//
//	Description:
//
//		Fluid emitters do not perform emission in their compute() method.
//		Instead, each fluid to which the emitter is connected will call the
//		fluidEmitter() method once per frame, to allow the emitter to emit
//		directly into the fluid.
//
//		It is ESSENTIAL that the compute routine return MS::kUnknownParameter
//		when the "emissionFunction" attribute is being evaluated.  Doing so
//		will trigger the base class default compute() method, which will
//		register this node's "fluidEmitter" function with the fluid.  The
//		mechanisms for doing this are not exposed through the API, so it is
//		important to let the default code handle this case.
//
//		For all other attributes, users can override the compute() method.
//
{
	if( plug.attribute() == mEmissionFunction )
	{
		//	ESSENTIAL!  Let the base class default compute method handle this
		//
		return MS::kUnknownParameter;
	}
	else
	{
		//	can add custom handling for other attributes here
		//
		return MS::kUnknownParameter;
	}
}


MStatus 
simpleFluidEmitter::fluidEmitter( 
	const MObject& fluidObject, 
	const MMatrix& worldMatrix, 
	int plugIndex 
)
//==============================================================================
//
//	Description:
//
//		Callback function that gets called once per frame by each fluid
//		into which this emitter is emitting.  Emits values directly into
//		the fluid object.  The MFnFluid object passed to this routine is
//		not pointing to a DAG object, it is pointing to an internal fluid
//		data structure that the fluid node is constructing, eventually to
//		be set into the fluid's output attribute.
//
//	Parameters:
//
//		fluid:			fluid into which we are emitting
//		worldMatrix:	object->world matrix for the fluid
//		plugIndex:		identifies which fluid connected to the emitter
//						we are emitting into
//
//	Returns:
//
//		MS::kSuccess 			if the method wishes to override the default
//								emitter behaviour
//		MS::kUnknownParameter 	if the method wishes to have the default
//								emitter behaviour execute after this routine
//								exits.
//
//	Notes:
//
//		The method first does some work common to all emitter types, then
//		calls one of 4 different methods to actually do the emission.
//		The methods are:
//
//			omniEmitter: 	omni-directional emitter from a point,
//							or from the vertices of an owner object.
//
//			volumeEmitter:	emits from the surface of an exact cube, sphere,
//							cone, cylinder, or torus.
//
//			surfaceEmitter:	emits from the surface of an owner object.
//
//==============================================================================
{
	//	make sure the fluid is valid.  If it isn't, return MS::kSuccess, indicating
	//	that no work needs to be done.  If we return a failure code, then the default
	//	fluid emitter code will try to run, which is pointless if the fluid is not
	//	valid.
	//
	MFnFluid fluid( fluidObject );
	if( fluid.object() != MObject::kNullObj )
	{
		return MS::kSuccess;
	}

	//	get a data block for the emitter, so we can get attribute values
	//
	MDataBlock block = forceCache();

	//	figure out the time interval for emission for the given fluid
	//
	double dTime = getDeltaTime( plugIndex, block ).as(MTime::kSeconds);
	if( dTime == 0.0 )
	{
		// shouldn't happen, but if the time interval is 0, then no fluid should
		// be emitted
		return MS::kSuccess;
	}

	// 	if currentTime <= startTime, return. The startTime is connected to 
	// 	the target fluid object.
	//
	MTime cTime = getCurrentTime( block );
	MTime sTime = getStartTime( plugIndex, block );

	//	if we are at or before the start time, reset the random number
	//	state to the appropriate seed value for the given fluid
	//
	if( cTime < sTime )
	{
		resetRandomState( plugIndex, block );
		return MS::kSuccess;
	}

	//	check to see if we need to emit anything into the target fluid.
	//	if the emission rate is 0, or if the fluid doesn't have a grid
	//	for one of the quantities, then we needn't do any emission
	//
	
	//	emission rates
	double density = fluidDensityEmission( block );	
	double heat = fluidHeatEmission( block );	
	double fuel = fluidFuelEmission( block );	
	bool doColor = fluidEmitColor( block );	

	//	fluid grid settings
	MFnFluid::FluidMethod densityMode, tempMode, fuelMode;
	MFnFluid::ColorMethod colorMode;
	MFnFluid::FluidGradient grad;
	MFnFluid::FalloffMethod falloffMode;
	fluid.getDensityMode( densityMode, grad );
	fluid.getTemperatureMode( tempMode, grad );
	fluid.getFuelMode( fuelMode, grad );
	fluid.getColorMode( colorMode );
	fluid.getFalloffMode( falloffMode );

	//	see if we need to emit density, heat, fuel, or color
	bool densityToEmit = (density != 0.0) && ((densityMode == MFnFluid::kDynamicGrid)||(densityMode == MFnFluid::kStaticGrid));
	bool heatToEmit = (heat != 0.0) && ((tempMode == MFnFluid::kDynamicGrid)||(tempMode == MFnFluid::kStaticGrid));
	bool fuelToEmit = (fuel != 0.0) && ((fuelMode == MFnFluid::kDynamicGrid)||(fuelMode == MFnFluid::kStaticGrid));
	bool colorToEmit = doColor && ((colorMode == MFnFluid::kDynamicColorGrid)||(colorMode == MFnFluid::kStaticColorGrid));
	bool falloffEmit = (falloffMode == MFnFluid::kStaticFalloffGrid);
	

	//	nothing to emit, do nothing
	//
	if( !densityToEmit && !heatToEmit && !fuelToEmit && !colorToEmit && !falloffEmit )
	{
		return MS::kSuccess;
	}

	//	get the dropoff rate for the fluid 
	//	
	double dropoff = fluidDropoff( block );

	//	modify the dropoff rate to account for fluids that have
	//	been scaled in worldspace - larger scales mean slower
	//	falloffs and vice versa
	//
	MTransformationMatrix xform( worldMatrix );
	double xformScale[3];
	xform.getScale( xformScale, MSpace::kWorld );
	double dropoffScale = sqrt( xformScale[0]*xformScale[0] + 
								xformScale[1]*xformScale[1] + 
								xformScale[2]*xformScale[2] );
	if( dropoffScale > 0.1 )
	{
		dropoff /= dropoffScale;
	}
	
	//	retrieve the current random state from the "randState" attribute, and
	//	store it in the member variable "randState".  We will use this member 
	//	value numerous times via the randgen() method.  Once we are done emitting,
	//	we will set the random state back into the attribute via setRandomState().
	//
	getRandomState( plugIndex, block );

	//	conversion value used to map user input emission rates into internal
	//	values.
	//
	double conversion = 0.01;

	MEmitterType emitterType = getEmitterType( block );
	switch( emitterType )
	{
		case kOmni:
			omniFluidEmitter( fluid, worldMatrix, plugIndex, block, dTime,
							  conversion, dropoff );
			break;

		case kVolume:
			volumeFluidEmitter( fluid, worldMatrix, plugIndex, block, dTime,
							    conversion, dropoff );
			break;
			
		case kSurface:
			surfaceFluidEmitter( fluid, worldMatrix, plugIndex, block, dTime,
							     conversion, dropoff );
			break;

		default:
			break;
	}

	//	store the random state back into the datablock
	//	
	setRandomState( plugIndex, block );
	return MS::kSuccess;
}

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

void 
simpleFluidEmitter::omniFluidEmitter(
	MFnFluid& 		fluid,
	const MMatrix&	fluidWorldMatrix,
	int 			plugIndex,
	MDataBlock& 	block,
	double 			dt,
	double			conversion,
	double			dropoff
)
//==============================================================================
//
//	Method:	
//
//		simpleFluidEmitter::omniFluidEmitter
//
//	Description:
//
//		Emits fluid from a point, or from a set of object control points.
//
//	Parameters:
//
//		fluid:				fluid into which we are emitting
//		fluidWorldMatrix:	object->world matrix for the fluid
//		plugIndex:			identifies which fluid connected to the emitter
//							we are emitting into
//		block:				datablock for the emitter, to retrieve attribute
//							values
//		dt:					time delta for this frame
//		conversion:			mapping from UI emission rates to internal units
//		dropoff:			specifies how much emission rate drops off as
//							we move away from each emission point.
//
//	Notes:
//
//		If no owner object is present for the emitter, we simply emit from
//		the emitter position.  If an owner object is present, then we emit
//		from each control point of that object in an identical fashion.
//		
//		To associate an owner object with an emitter, use the
//		addDynamic MEL command, e.g. "addDynamic simpleFluidEmitter1 pPlane1".
//
//==============================================================================
{
	//	find the positions that we need to emit from
	//
	MVectorArray emitterPositions;
	
	//	first, try to get them from an owner object, which will have its
	//	"ownerPositionData" attribute feeding into the emitter.  These
	//	values are in worldspace
	//
	bool gotOwnerPositions = false;
	MObject ownerShape = getOwnerShape();
	if( ownerShape != MObject::kNullObj )
	{
		MStatus status;
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
				{
					emitterPositions.append( posArray[i] );
				}
				
				gotOwnerPositions = true;
			}
		}
	}
	
	//	there was no owner object, so we just use the emitter position for
	//	emission.
	//
	if( !gotOwnerPositions )
	{
		MPoint emitterPos = getWorldPosition();
		emitterPositions.append( emitterPos );
	}

	//	get emission rates for density, fuel, heat, and emission color
	//	
	double densityEmit = fluidDensityEmission( block );
	double fuelEmit = fluidFuelEmission( block );
	double heatEmit = fluidHeatEmission( block );
	bool doEmitColor = fluidEmitColor( block );
	MColor emitColor = fluidColor( block );

	//	rate modulation based on frame time, user value conversion factor, and
	//	standard emitter "rate" value (not actually exposed in most fluid
	//	emitters, but there anyway).
	//
	double theRate = getRate(block) * dt * conversion;

	//	get voxel dimensions and sizes (object space)
	//
	double size[3];
	unsigned int res[3];
	fluid.getDimensions( size[0], size[1], size[2] );
	fluid.getResolution( res[0], res[1], res[2] );
	
	//	voxel sizes
	double dx = size[0] / res[0];
	double dy = size[1] / res[1];
	double dz = size[2] / res[2];
	
	//	voxel centers
	double Ox = -size[0]/2;
	double Oy = -size[1]/2;
	double Oz = -size[2]/2;	

	//	emission will only happen for voxels whose centers lie within
	//	"minDist" and "maxDist" of an emitter position
	//
	double minDist = getMinDistance( block );
	double maxDist = getMaxDistance( block );

	//	bump up the min/max distance values so that they
	//	are both > 0, and there is at least about a half
	//	voxel between the min and max values, to prevent aliasing
	//	artifacts caused by emitters missing most voxel centers
	//
	MTransformationMatrix fluidXform( fluidWorldMatrix );
	double fluidScale[3];
	fluidXform.getScale( fluidScale, MSpace::kWorld );
	
	//	compute smallest voxel diagonal length
	double wsX =  fabs(fluidScale[0]*dx);
	double wsY = fabs(fluidScale[1]*dy);
	double wsZ = fabs(fluidScale[2]*dz);
	double wsMin = MIN( MIN( wsX, wsY), wsZ );
	double wsMax = MAX( MAX( wsX, wsY), wsZ );
	double wsDiag  = wsMin * sqrt(3.0);

	//	make sure emission range is bigger than 0.5 voxels
	if ( maxDist <= minDist || maxDist <= (wsDiag/2.0) ) {
		if ( minDist < 0 ) minDist = 0;

		maxDist = minDist + wsDiag/2.0;
		dropoff = 0;
	}

	//	Now, it's time to actually emit into the fluid:
	//	
	//	foreach emitter point
	//		foreach voxel
	//			- select some points in the voxel
	//			- compute a dropoff function from the emitter point
	//			- emit an appropriate amount of fluid into the voxel
	//
	//	Since we've already expanded the min/max distances to cover
	//	the smallest voxel dimension, we should only need 1 sample per
	//	voxel, unless the voxels are highly non-square.  We increase the
	//	number of samples in these cases.
	//
	//	If the "jitter" flag is enabled, we jitter each sample position,
	//	using the rangen() function, which keeps track of independent 
	//	random states for each fluid, to make sure that results are
	//	repeatable for multiple simulation runs.
	//	

	// basic sample count
	int numSamples = 1;

	// increase samples if necessary for non-square voxels
	if(wsMin >.00001) 
	{
		numSamples = (int)(wsMax/wsMin + .5);
		if(numSamples > 8) 
			numSamples = 8;
		if(numSamples < 1)
			numSamples = 1;
	}
	
	bool jitter =  fluidJitter(block);
	if( !jitter )
	{
		//	I don't have a good uniform sample generator for an 
		//	arbitrary number of samples.  It would be a good idea to use
		//	one here.  For now, just use 1 sample for the non-jittered case.
		//
		numSamples = 1;
	}

	for( unsigned int p = 0; p < emitterPositions.length(); p++ )
	{
		MPoint emitterWorldPos = emitterPositions[p];

		//	loop through all voxels, looking for ones that lie at least
		//	partially within the dropoff field around this emitter point
		//
		for( unsigned int i = 0; i < res[0]; i++ )
		{
			double x = Ox + i*dx;
			
			for( unsigned int j = 0; j < res[1]; j++ )
			{
				double y = Oy + j*dy;
				
				for( unsigned int k = 0; k < res[2]; k++ )
				{
					double z = Oz + k*dz;
	
					int si;
					for( si = 0; si < numSamples; si++ )
					{
						//	compute sample point (fluid object space)
						//
						double rx, ry, rz;
						if( jitter )
						{
							rx = x + randgen()*dx;
							ry = y + randgen()*dy;
							rz = z + randgen()*dz;
						}
						else
						{
							rx = x + 0.5*dx;
							ry = y + 0.5*dy;
							rz = z + 0.5*dz;
						}

						//	compute distance from sample to emitter point
						//	
						MPoint point( rx, ry, rz );
						point *= fluidWorldMatrix;
						MVector diff = point - emitterWorldPos;
						double distSquared = diff * diff;
						double dist = diff.length();
					
						//	discard if outside min/max range
						//
						if( (dist < minDist) || (dist > maxDist) )
						{
							continue;
						}
						
						//	drop off the emission rate according to the falloff
						//	parameter, and divide to accound for multiple samples
						//	in the voxel
						//
						double distDrop = dropoff * distSquared;
						double newVal = theRate * exp( -distDrop ) / (double)numSamples;

						//	emit density/heat/fuel/color into the current voxel
						//
						if( newVal != 0 )
						{
							fluid.emitIntoArrays( (float) newVal, i, j, k, (float)densityEmit, (float)heatEmit, (float)fuelEmit, doEmitColor, emitColor );
						}

						float *fArray = fluid.falloff();
						if( fArray != NULL )
						{
							MPoint midPoint( x+0.5*dx, y+0.5*dy, z+0.5*dz );
							midPoint.x *= 0.2;
							midPoint.y *= 0.2;
							midPoint.z *= 0.2;

							float fdist = (float) sqrt( midPoint.x*midPoint.x + midPoint.y*midPoint.y + midPoint.z*midPoint.z );
							fdist /= sqrtf(3.0f);
							fArray[fluid.index(i,j,k)] = 1.0f-fdist;
						}
					}
				}
			}
		}
	}
}

void 
simpleFluidEmitter::volumeFluidEmitter(
	MFnFluid& 		fluid,
	const MMatrix&	fluidWorldMatrix,
	int 			plugIndex,
	MDataBlock& 	block,
	double 			dt,
	double			conversion,
	double			dropoff
)
//==============================================================================
//
//	Method:	
//
//		simpleFluidEmitter::volumeFluidEmitter
//
//	Description:
//
//		Emits fluid from points distributed over the surface of the 
//		emitter's owner object.
//
//	Parameters:
//
//		fluid:				fluid into which we are emitting
//		fluidWorldMatrix:	object->world matrix for the fluid
//		plugIndex:			identifies which fluid connected to the emitter
//							we are emitting into
//		block:				datablock for the emitter, to retrieve attribute
//							values
//		dt:					time delta for this frame
//		conversion:			mapping from UI emission rates to internal units
//		dropoff:			specifies how much emission rate drops off as
//							we move away from the local y-axis of the 
//							volume emitter shape.
//
//==============================================================================
{
	//	get emitter position and relevant matrices 
	//	
	MPoint emitterPos = getWorldPosition();
	MMatrix emitterWorldMatrix = getWorldMatrix();
	MMatrix fluidInverseWorldMatrix = fluidWorldMatrix.inverse();
	
	//	get emission rates for density, fuel, heat, and emission color
	//	
	double densityEmit = fluidDensityEmission( block );
	double fuelEmit = fluidFuelEmission( block );
	double heatEmit = fluidHeatEmission( block );
	bool doEmitColor = fluidEmitColor( block );
	MColor emitColor = fluidColor( block );
	
	//	rate modulation based on frame time, user value conversion factor, and
	//	standard emitter "rate" value (not actually exposed in most fluid
	//	emitters, but there anyway).
	//
	double theRate = getRate(block) * dt * conversion;
	
	//	get voxel dimensions and sizes (object space)
	//
	double size[3];
	unsigned int res[3];
	fluid.getDimensions( size[0], size[1], size[2] );
	fluid.getResolution( res[0], res[1], res[2] );

	//	voxel sizes
	double dx = size[0] / res[0];
	double dy = size[1] / res[1];
	double dz = size[2] / res[2];
	
	// 	voxel centers
	double Ox = -size[0]/2;
	double Oy = -size[1]/2;
	double Oz = -size[2]/2;	

	//	find the voxels that intersect the bounding box of the volume
	//	primitive associated with the emitter
	//
	MBoundingBox bbox;
	if( !volumePrimitiveBoundingBox( bbox ) )
	{
		//	shouldn't happen
		//
		return;
	}
	
	//	transform volume primitive into fluid space
	//
	bbox.transformUsing( emitterWorldMatrix );
	bbox.transformUsing( fluidInverseWorldMatrix );
	MPoint lowCorner = bbox.min();
	MPoint highCorner = bbox.max();

	//	get fluid voxel coord range of bounding box
	//
	int3 lowCoords;
	int3 highCoords;
	fluid.toGridIndex( lowCorner, lowCoords );
	fluid.toGridIndex( highCorner, highCoords );
	
	int i;
	for ( i = 0; i < 3; i++ )
	{
		if ( lowCoords[i] < 0 ) {
			lowCoords[i] = 0;
		} else if ( lowCoords[i] > ((int)res[i])-1 ) {
			lowCoords[i] = ((int)res[i])-1;
		}

		if ( highCoords[i] < 0 ) {
			highCoords[i] = 0;
		} else if ( highCoords[i] > ((int)res[i])-1 ) {
			highCoords[i] = ((int)res[i])-1;
		}
		
	}

	//	figure out the emitter size relative to the voxel size, and compute
	//	a per-voxel sampling rate that uses 1 sample/voxel for emitters that
	//	are >= 2 voxels big in all dimensions.  For smaller emitters, use up
	//	to 8 samples per voxel.
	//
	double emitterVoxelSize[3];
	emitterVoxelSize[0] = (highCorner[0]-lowCorner[0])/dx;
	emitterVoxelSize[1] = (highCorner[1]-lowCorner[1])/dy;
	emitterVoxelSize[2] = (highCorner[2]-lowCorner[2])/dz;
		
	double minVoxelSize = MIN(emitterVoxelSize[0],MIN(emitterVoxelSize[1],emitterVoxelSize[2]));
	if( minVoxelSize < 1.0 )
	{
		minVoxelSize = 1.0;
	}
	int maxSamples = 8;
	int numSamples = (int)(8.0/(minVoxelSize*minVoxelSize*minVoxelSize) + 0.5);
	if( numSamples < 1 ) numSamples = 1;
	if( numSamples > maxSamples ) numSamples = maxSamples;
	
	//	non-jittered, just use one sample in the voxel center.  Should replace
	//	with uniform sampling pattern.
	//
	bool jitter = fluidJitter(block);
	if( !jitter )
	{
		numSamples = 1;
	}
	
	//	for each voxel that could potentially intersect the volume emitter
	//	primitive, take some samples in the voxel.  For those inside the
	//	volume, compute their dropoff relative to the primitive's local y-axis,
	//	and emit an appropriate amount into the voxel.
	//
	for( i = lowCoords[0]; i <= highCoords[0]; i++ )
	{
		double x = Ox + (i+0.5)*dx;
			
		for( int j = lowCoords[1]; j < highCoords[1]; j++ )
		{
			double y = Oy + (j+0.5)*dy;

			for( int k = lowCoords[2]; k < highCoords[2]; k++ )
			{
				double z = Oz + (k+0.5)*dz;
				
				for ( int si = 0; si < numSamples; si++) {
					
					//	compute voxel sample point (object space)
					//
					double rx, ry, rz;
					if(jitter) {
						rx = x + dx*(randgen() - 0.5);
						ry = y + dy*(randgen() - 0.5);
						rz = z + dz*(randgen() - 0.5);
					} else {
						rx = x;
						ry = y;
						rz = z;
					}
					
					//	to world space
					MPoint pt( rx, ry, rz );
					pt *= fluidWorldMatrix;

					//	test to see if point is inside volume primitive
					//
					if( volumePrimitivePointInside( pt, emitterWorldMatrix ) )
					{
						//	compute dropoff
						//
						double dist = pt.distanceTo( emitterPos );
						double distDrop = dropoff * (dist*dist);
						double newVal = (theRate * exp( -distDrop )) / (double)numSamples;
						
						//	emit into arrays
						//
						if( newVal != 0.0 )
						{
							fluid.emitIntoArrays( (float) newVal, i, j, k, (float)densityEmit, (float)heatEmit, (float)fuelEmit, doEmitColor, emitColor );
						}
					}
				}
			}
		}
	}
}

void 
simpleFluidEmitter::surfaceFluidEmitter(
	MFnFluid& 		fluid,
	const MMatrix&	fluidWorldMatrix,
	int 			plugIndex,
	MDataBlock& 	block,
	double 			dt,
	double			conversion,
	double			dropoff
)
//==============================================================================
//
//	Method:	
//
//		simpleFluidEmitter::surfaceFluidEmitter
//
//	Description:
//
//		Emits fluid from one of a predefined set of volumes (cube, sphere,
//		cylinder, cone, torus).
//
//	Parameters:
//
//		fluid:				fluid into which we are emitting
//		fluidWorldMatrix:	object->world matrix for the fluid
//		plugIndex:			identifies which fluid connected to the emitter
//							we are emitting into
//		block:				datablock for the emitter, to retrieve attribute
//							values
//		dt:					time delta for this frame
//		conversion:			mapping from UI emission rates to internal units
//		dropoff:			specifies how much emission rate drops off as
//							the surface points move away from the centers
//							of the voxels in which they lie.
//
//	Notes:
//		
//		To associate an owner object with an emitter, use the
//		addDynamic MEL command, e.g. "addDynamic simpleFluidEmitter1 pPlane1".
//
//==============================================================================
{
	//	get relevant world matrices
	//
	MMatrix fluidInverseWorldMatrix = fluidWorldMatrix.inverse();
	
	//	get emission rates for density, fuel, heat, and emission color
	//	
	double densityEmit = fluidDensityEmission( block );
	double fuelEmit = fluidFuelEmission( block );
	double heatEmit = fluidHeatEmission( block );
	bool doEmitColor = fluidEmitColor( block );
	MColor emitColor = fluidColor( block );
	
	//	rate modulation based on frame time, user value conversion factor, and
	//	standard emitter "rate" value (not actually exposed in most fluid
	//	emitters, but there anyway).
	//
	double theRate = getRate(block) * dt * conversion;

	//	get voxel dimensions and sizes (object space)
	//
	double size[3];
	unsigned int res[3];
	fluid.getDimensions( size[0], size[1], size[2] );
	fluid.getResolution( res[0], res[1], res[2] );
		
	//	voxel sizes
	double dx = size[0] / res[0];
	double dy = size[1] / res[1];
	double dz = size[2] / res[2];
	
	//	voxel centers
	double Ox = -size[0]/2;
	double Oy = -size[1]/2;
	double Oz = -size[2]/2;	

	//	get the "swept geometry" data for the emitter surface.  This structure
	//	tracks the motion of each emitter triangle over the time interval
	//	for this simulation step.  We just use positions on the emitter
	//	surface at the end of the time step to do the emission.
	//
	MDataHandle sweptHandle = block.inputValue( mSweptGeometry );
	MObject sweptData = sweptHandle.data();
	MFnDynSweptGeometryData fnSweptData( sweptData );

	//	for "non-jittered" sampling, just reset the random state for each 
	//	triangle, which gives us a fixed set of samples all the time.
	//	Sure, they're still jittered, but they're all jittered the same,
	//	which makes them kinda uniform.
	//
	bool jitter = fluidJitter(block);
	if( !jitter )
	{
		resetRandomState( plugIndex, block );
	}

	if( fnSweptData.triangleCount() > 0 )
	{
		//	average voxel face area - use this as the canonical unit that
		//	receives the emission rate specified by the users.  Scale the
		//	rate for other triangles accordingly.
		//
		double vfArea = pow(dx*dy*dz, 2.0/3.0);
		
		//	very rudimentary support for textured emission rate and
		//	textured emission color.  We simply sample each texture once
		//	at the center of each emitter surface triangle.  This will 
		//	cause aliasing artifacts when these triangles are large.
		//
		MFnDependencyNode fnNode( thisMObject() );
		MObject rateTextureAttr = fnNode.attribute( "textureRate" );
		MObject colorTextureAttr = fnNode.attribute( "particleColor" );

		bool texturedRate = hasValidEmission2dTexture( rateTextureAttr );
		bool texturedColor = hasValidEmission2dTexture( colorTextureAttr );
		
		//	construct texture coordinates for each triangle center
		//	
		MDoubleArray uCoords, vCoords;
		if( texturedRate || texturedColor )
		{
			uCoords.setLength( fnSweptData.triangleCount() );
			vCoords.setLength( fnSweptData.triangleCount() );
			
			int t;
			for( t = 0; t < fnSweptData.triangleCount(); t++ )
			{
				MDynSweptTriangle tri = fnSweptData.sweptTriangle( t );
				MVector uv0 = tri.uvPoint(0);
				MVector uv1 = tri.uvPoint(1);
				MVector uv2 = tri.uvPoint(2);
				
				MVector uvMid = (uv0+uv1+uv2)/3.0;
				uCoords[t] = uvMid[0];
				vCoords[t] = uvMid[1];
			}
		}

		//	evaluate textured rate and color values at the triangle centers
		//
		MDoubleArray texturedRateValues;
		if( texturedRate )
		{
			texturedRateValues.setLength( uCoords.length() );
			evalEmission2dTexture( rateTextureAttr, uCoords, vCoords, NULL, &texturedRateValues );
		}
		
		MVectorArray texturedColorValues;
		if( texturedColor )
		{
			texturedColorValues.setLength( uCoords.length() );
			evalEmission2dTexture( colorTextureAttr, uCoords, vCoords, &texturedColorValues, NULL );
		}
		
		for( int t = 0; t < fnSweptData.triangleCount(); t++ )
		{
			//	calculate emission rate and color values for this triangle
			//
			double curTexturedRate = texturedRate ? texturedRateValues[t] : 1.0;
			MColor curTexturedColor;
			if( texturedColor )
			{
				MVector& curVec = texturedColorValues[t];
				curTexturedColor.r = (float)curVec[0];
				curTexturedColor.g = (float)curVec[1];
				curTexturedColor.b = (float)curVec[2];
				curTexturedColor.a = 1.0;
			}
			else
			{
				curTexturedColor = emitColor;
			}

			MDynSweptTriangle tri = fnSweptData.sweptTriangle( t );
			MVector v0 = tri.vertex(0);
			MVector v1 = tri.vertex(1);
			MVector v2 = tri.vertex(2);

			//	compute number of samples for this triangle based on area,
			//	with large triangles receiving approximately 1 sample for 
			//	each voxel that they intersect
			//
			double triArea = tri.area();
			int numSamples = (int)(triArea / vfArea);
			if( numSamples < 1 ) numSamples = 1;
			
			//	compute emission rate for the points on the triangle.
			//	Scale the canonical rate by the area ratio of this triangle
			//	to the average voxel size, then split it amongst all the samples.
			//
			double triRate = (theRate*(triArea/vfArea))/numSamples;
			
			triRate *= curTexturedRate;
			
			for( int j = 0; j < numSamples; j++ )
			{
				//	generate a random point on the triangle,
				//	map it into fluid local space
				//
				double r1 = randgen();
				double r2 = randgen();
				
				if( r1 + r2 > 1 )
				{
					r1 = 1-r1;
					r2 = 1-r2;
				}
				double r3 = 1 - (r1+r2);
				MPoint randPoint = r1*v0 + r2*v1 + r3*v2;
				randPoint *= fluidInverseWorldMatrix;
				
				//	figure out where the current point lies
				//
				int3 coord;
				fluid.toGridIndex( randPoint, coord );
				
				if( (coord[0]<0) || (coord[1]<0) || (coord[2]<0) ||
					(coord[0]>=(int)res[0]) || (coord[1]>=(int)res[1]) || (coord[2]>=(int)res[2]) )
				{
					continue;
				}
				
				//	do some falloff based on how far from the voxel center 
				//	the current point lies
				//
				MPoint gridPoint;
				gridPoint.x = Ox + (coord[0]+0.5)*dx;
				gridPoint.y = Oy + (coord[1]+0.5)*dy;
				gridPoint.z = Oz + (coord[2]+0.5)*dz;
				
				MVector diff = gridPoint - randPoint;
				double distSquared = diff * diff;
				double distDrop = dropoff * distSquared;
				
				double newVal = triRate * exp( -distDrop );
		
				//	emit into the voxel
				//
				if( newVal != 0 )
				{
					fluid.emitIntoArrays( (float) newVal, coord[0], coord[1], coord[2], (float)densityEmit, (float)heatEmit, (float)fuelEmit, doEmitColor, curTexturedColor );		
				}
			}
		}
	}
}

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "simpleFluidEmitter", simpleFluidEmitter::id,
							&simpleFluidEmitter::creator, &simpleFluidEmitter::initialize,
							MPxNode::kFluidEmitterNode );
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

	status = plugin.deregisterNode( simpleFluidEmitter::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}

