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
#include <maya/MStatus.h>
#include <maya/MDagPath.h>
#include <maya/MFloatPointArray.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MHairSystem.h>
#include <maya/MFnPlugin.h>

#include <math.h> 

#define	kPluginName		"hairCollisionSolver"

//
//
// OVERVIEW:
// This plug-in implements a custom collision solver for Maya's dynamic
// hair system. This allows users to override the following aspects of
// Maya's dynamic hair systems:
//
//		o Override the Maya dynamic hair system's object-to-hair
//		  collision algorithm with a user-defined algorithm.
//		o Optionally perform global filtering on hair, such as freeze,
//		  smoothing, etc.
//
// It should be noted that Maya's dynamic hair system involves four
// arears of collision detection, and this plug-in is specific to the
// Hair-to-Object aspect only. The four areas collision which the Maya
// dynamic hair system involves are:
//
//		1) Hair to object collision.
//		2) Hair to implicit object collision.
//		3) Hair to ground plane collision.
//		4) Self collision between hairs.
//
// This plug-in illustrates the first case, overriding Maya's internal
// Hair-to-Object collision solver. There is currently no API for over-
// riding the other three collision solvers.
//
//
// RATIONALE:
// There are several reasons why are user may wish to override Maya's
// internal hair to object collision algorithm. These are:
//
//		1) The internal algorithm may not be accurate enough. After all,
//		   it is a simulation of real-life physics and there are tradeoffs
//		   taken to provide reasonable performance. Note that there are
//		   means of increasing the accuracy without writing a custom
//		   implementation, such as decreasing the dynamics time step, or
//		   increasing the hair width.
//		2) The built-in algorithm might be too accurate. If you only want
//		   simplified collisions, such as against a bounding box repre-
//		   sentation instead of the internal algorithm's exhasutive test-
//		   ing against each surface of the object, you could write a
//		   lighter-weight implementation.
//		3) You might have a desire to process the hairs, such as smooth
//		   them out.
//
//
// STRATEGY:
// The basic idea is to implement a custom callback which is registered
// via MHairSystem::registerCollisionSolverCollide(). Your callback will
// then be invoked in place of Maya's internal collision solver. By simply
// registering a collision solver, you can completely implement a custom
// hair-to-object solution.
//
// However, since the collision solver is called once per hair times the
// number of solver iterations, it is wise to pre-process the data if
// possible to speed up the collision tests. For this reason, you can
// assign a pre-frame callback. One approach is to create a pre-processed
// representation of your object pre-frame (e.g. an octree representation)
// and then access this representation during the collision testing.
//
// For the purposes of our simple demo plug-in, our private data will
// consist of a COLLISION_INFO which contains an array of COLLISION_OBJ
// structures, each one holding the bounding box of the object in world
// space.
//
// One issue with pre-processing the data involves managing the private
// data. A collision object could be deleted or turned off during a
// simulation. One way to cleanly manage such data is to store your
// private data on a typed attribute which is added to the node. You
// would build your private data once at the start of simulation in your
// pre-frame callback (keep track of the current time, and if the curTime
// passed in is less than what you store locally, assume the playback
// has restarted from the beginning -- or the user is being silly and
// trying to play the simulation backwards :-) Since it is relatively
// expensive to look up a dynamic attribute value, and the collide()
// callback can get triggered 1000's of times per frame, for efficiency
// you can pass back your private data as a pointer from your pre-frame
// routine, and this pointer is then passed directly into your collide()
// callback.
// 
//
typedef struct {
	int				numVerts;	// Number of vertices in object.
	double			minx;		// Bounding box minimal extrema.
	double			miny;		// Bounding box minimal extrema.
	double			minz;		// Bounding box minimal extrema.
	double			maxx;		// Bounding box maximal extrema.
	double			maxy;		// Bounding box maximal extrema.
	double			maxz;		// Bounding box maximal extrema.
} COLLISION_OBJ ;

typedef struct {
	int				numObjs;	// Number of collision objects.
	COLLISION_OBJ	*objs;		// Array of per-object info.
} COLLISION_INFO ;

//////////////////////////////////////////////////////////////////////////
//
// Synopsis:
//		bool	preFrame( hairSystem, curTime, privateData )
//
// Description:
//		This callback is invoked once at the start of each frame, allowing
//	the user to perform any pre-processing as they see fit, such as build-
//	ing or updating private collision-detection structures.
//
//		Note: it is possible for collision objects to change between
//	frames during a simulation (for example, the user could delete a col-
//	lision object), so if you choose to store your pre-processed data, it
//	is critical to track any edits or deletions to the collision object
//	to keep your pre-processed data valid.
//
//		There are lots of hints for writing an effective pre-frame call-
//	back in the STRATEGY section listed earlier in this file.
//
// Parameters:
//		MObject hairSystem	: (in)	The hair system shape node.
//		double	curTime		: (in)	Current time in seconds.
//		void	**privateData:(out)	Allows the user to return a private
//									data pointer to be passed into their
//									collision solver. If you store your
//									pre-processed data in data structure
//									which is difficult to access, such as
//									on a typed attribute, this provides
//									an easy way to provide the pointer.
//
// Returns:
//		bool	true		: Successfully performed any needed initial-
//							  isation for the hair simulation this frame.
//		bool	false		: An error was detected.
//
//////////////////////////////////////////////////////////////////////////


bool		preFrame(
				const MObject		hairSystem,
				const double		curTime,
				void				**privateData )
{
	MStatus status;

	// If you need want to perform any preprocessing on your collision
	// objects pre-frame, do it here. One option for storing the pre-
	// processed data is on a typed attribute on the hairSystem node.
	// That data could be fetched and updated here.
	//
	// In our example, we'll just compute a bounding box here and NOT use
	// attribute storage. That is an exercise for the reader.
	//
	MFnDependencyNode fnHairSystem( hairSystem, &status );
	CHECK_MSTATUS_AND_RETURN( status, false );
	fprintf( stderr,
			"preFrame: calling hairSystem node=`%s', time=%g\n",
			fnHairSystem.name().asChar(), curTime );
	MObjectArray	cols;
	MIntArray		logIdxs;
	CHECK_MSTATUS_AND_RETURN( MHairSystem::getCollisionObject( hairSystem,
			cols, logIdxs ), false );
	int nobj = cols.length();

	// Allocate private data.
	// This allows us to pre-process data on a pre-frame basis to avoid
	// calculating it per hair inside the collide() call. As noted earlier
	// we could allocate this in preFrame() and hang it off the hairSystem
	// node via a dynamic attribute.
	// Instead we'll allocate it here.
	//
	COLLISION_INFO *collisionInfo = (COLLISION_INFO *) malloc(
			sizeof( COLLISION_INFO ) );
	collisionInfo->objs = (COLLISION_OBJ *) malloc(
			nobj * sizeof( COLLISION_OBJ ) );
	collisionInfo->numObjs = nobj;

	// Create the private data that we'll make available to the collide
	// method. The data should actually be stored in a way that it can
	// be cleaned up (such as storing the pointer on the hairSystem node
	// using a dynamic attribute). As it stands right now, there is a
	// memory leak with this plug-in because the memory we're allocating
	// for the private data is never cleaned up.
	//
	// Note that when using the dynamic attribute approach, it is still
	// wise to set *privateData because this avoids the need to look up
	// the plug inside the collide() routine which is a high-traffic
	// method.
	//
	*privateData = (void *) collisionInfo;

	// Loop through the collision objects and pre-process, storing the
	// results in the collisionInfo structure.
	//
	int	   obj;
	for ( obj = 0; obj < nobj; ++obj ) {
		// Get the ith collision geometry we are connected to.
		//
		MObject colObj = cols[obj];

		// Get the DAG path for the collision object so we can transform
		// the vertices to world space.
		//
		MFnDagNode fnDagNode( colObj, &status );
		CHECK_MSTATUS_AND_RETURN( status, false );
		MDagPath path;
		status = fnDagNode.getPath( path );
		CHECK_MSTATUS_AND_RETURN( status, false );
		MFnMesh fnMesh( path, &status );
		if ( MS::kSuccess != status ) {
			fprintf( stderr,
					"%s:%d: collide was not passed a valid mesh shape\n",
					__FILE__, __LINE__ );
			return( false );
		}

		// Get the vertices of the object transformed to world space.
		//
		MFloatPointArray	verts;
		status = fnMesh.getPoints( verts, MSpace::kWorld );
		CHECK_MSTATUS_AND_RETURN( status, false );

		// Compute the bounding box for the collision object.
		// As this is a quick and dirty demo, we'll just support collisions
		// between hair and the bbox.
		//
		double minx, miny, minz, maxx, maxy, maxz, x, y, z;
		minx = maxx = verts[0].x;
		miny = maxy = verts[0].y;
		minz = maxz = verts[0].z;
		int nv = verts.length();
		int i;
		for ( i = 1; i < nv; ++i ) {
			x = verts[i].x;
			y = verts[i].y;
			z = verts[i].z;

			if ( x < minx ) {
				minx = x;
			}
			if ( y < miny ) {
				miny = y;
			}
			if ( z < minz ) {
				minz = z;
			}

			if ( x > maxx ) {
				maxx = x;
			}
			if ( y > maxy ) {
				maxy = y;
			}
			if ( z > maxz ) {
				maxz = z;
			}
		}

		// Store this precomputed informantion into our private data
		// structure.
		//
		collisionInfo->objs[obj].numVerts = nv;
		collisionInfo->objs[obj].minx = minx;
		collisionInfo->objs[obj].miny = miny;
		collisionInfo->objs[obj].minz = minz;
		collisionInfo->objs[obj].maxx = maxx;
		collisionInfo->objs[obj].maxy = maxy;
		collisionInfo->objs[obj].maxz = maxz;
		fprintf( stderr, "Inside preFrameInit, bbox=%g %g %g %g %g %g\n",
				minx,miny,minz,maxx,maxy,maxz);
	}

	return( true );
}

//////////////////////////////////////////////////////////////////////////
//
// Synopsis:
//		bool	belowCollisionPlane( co, pnt )
//
// Description:
//		Test if `pnt' is directly below the collision plane of `co'.
//
// Parameters:
//		COLLISION_OBJ	*co	: (in)	The collision object to test against.
//		MVector			&pnt: (in)	Point to test.
//
// Returns:
//		bool	true		: The `pnt' is directly below the collision
//							  plane specified by `co'.
//		bool	false		: The `pnt' is not directly below the collis-
//							  ion plane specified by `co'.
//
//////////////////////////////////////////////////////////////////////////


bool	belowCollisionPlane( const COLLISION_OBJ *co, const MVector &pnt )
{
	return(    pnt.x > co->minx && pnt.x < co->maxx
			                    && pnt.y < co->maxy
			&& pnt.z > co->minz && pnt.z < co->maxz );
}

//////////////////////////////////////////////////////////////////////////
//
// Synopsis:
//		MStatus	collide( hairSystem, follicleIndex, hairPositions,
//						hairPositionsLast, hairWidths, startIndex,
//						endIndex, curTime, friction, privateData )
//
// Description:
//		This callback is invoked by Maya to test if a collision exists be-
//	tween the follicle (defined by `hairPositions', `hairPositionsLast'
//	and `hairWidths') and the collision objects associated with the
//	hairSystem. If a collision is detected, this routine should adjust
//	`hairPositions' to compensate. The `hairPositionsLast' can also be
//	modified to adjust the velocity, but this should only be a dampening
//	effect as the hair solver expects collisions to be dampened,
//
//		This method is invoked often (actually its once per hair times the
//	hairSystem shape's iterations factor). Thus with 10,000 follicles it
//	would be called 80,000 times per frame (Note: as of Maya 7.0, the
//	iterations factor is multipled by 2, so at its default value of 4, you
//	get 8x calls. However, if you set iterations=0, it clamps to 1x calls).
//
// Parameters:
//		MObject hairSystem	: (in)	The hair system shape node.
//		int		follicleIndex:(in)	Which follicle we are processing.
//									You can get the follicle node if you
//									wish via MHairSystem::getFollicle().
//		MVectorArray &hairPositions:
//							  (mod)	Array of locations in world space
//									where the hair system is trying to
//									move the follicle. The first entry
//									corresponds to the root of the hair
//									and the last entry to the tip. If a
//									collision is detected, these values
//									should be updated appropriately.
//									Note that hairPositions can change
//									from iteration to iteration on the
//									same hair and same frame. You can set
//									a position, and then find the hair has
//									moved a bit the next iteration. There
//									are two reasons for this phenomenom:
//									  1) Other collisions could occur,
//										 including self collision.
//									  2) Stiffness is actually applied PER
//										 ITERATION.
//		MVectorArray &hairPositionsLast:
//							  (mod)	Array of the position at the previous
//									time for each entry in `hairPositions'.
//		MDoubleArray &hairWidths:
//							  (in)	Array of widths, representing the
//									width of the follcle at each entry in
//									`hairPositions'.
//		int		startIndex	: (in)	First index in `hairPositions' we
//									can move. This will be 0 unless the
//									root is locked in which case it will
//									be 1.
//		int		endIndex	: (in)	Last index in `hairPositions' we can
//									move. Will be the full array (N-1)
//									unless the tip is locked.
//		double	curTime		: (in)	Start of current time interval.
//		double	friction	: (in)	Frictional coefficient.
//		void	*privateData: (in)	If a privateData record was returned
//									from preFrame() it will be passed in
//									here. This is an optimisation to save
//									expensive lookups of your private data
//									if stored on the node.
//
// Returns:
//		bool	true		: Successfully performed collision testing and
//							  adjustment of the hair array data.
//		bool	false		: An error occurred.
//
//////////////////////////////////////////////////////////////////////////


#define	EPSILON	0.0001

bool	collide(
				const MObject		hairSystem,
				const int			follicleIndex,
				MVectorArray		&hairPositions,
				MVectorArray		&hairPositionsLast,
				const MDoubleArray	&hairWidths,
				const int			startIndex,
				const int			endIndex,
				const double		curTime,
				const double		friction,
				const void			*privateData )
{
	// Get the private data for the collision objects which was returned
	// from preFrame().
	//
	COLLISION_INFO *ci = (COLLISION_INFO *) privateData;
	if ( !ci ) {
		fprintf( stderr,"%s:%d: collide() privateData pointer is NULL\n",
				__FILE__, __LINE__ );
		return( false );
	}

	// If object has no vertices, or hair has no segments, then there is
	// nothing to collide. In our example, we'll return here, but if you
	// want to implement your own hair processing such as smoothing or
	// freeze on the hair, you could proceed and let the processing happen
	// after the object loop so that the data gets processed even if no
	// collisions occur.
	//
	if ( ci->numObjs <= 0 || hairPositions.length() <= 0 ) {
		return( true );
	}

	int		obj;
	for ( obj = 0; obj < ci->numObjs; ++obj ) {
		COLLISION_OBJ *co = &ci->objs[obj];

		// For our plug-in example, we just collide the segments of the
		// hair against the top of the bounding box for the geometry.
		// In an implementation where you only care about hair falling
		// downwards onto flat objects, this might be OK. However, in the
		// most deluxe of implementation, you should do the following:
		//
		//		o  Determine the motion of each face of your collision
		//		   object during the time range.
		//		o  Step through the follicle, and consider each segment
		//		   to be a moving cylinder where the start and end radii
		//		   can differ. The radii come from `hairWidths' and the
		//		   motion comes from the difference between `hairPositions'
		//		   and `hairPositionsLast'.
		//		o  Intersect each moving element (e.g. moving triangle
		//		   if you have a mesh obect) with each hair segment
		//		   e.g. moving cylinder). This intersection may occur
		//		   at a point within the frame. (Remember that the
		//		   hairPositions[] array holds the DESIRED location where
		//		   the hair system wants the hair to go.
		//		o  There can be multiple collisions along a hair segment.
		//		   Use the first location found and the maximal velocity.
		//
		// If a collision is detected, the `hairPositions' array should be
		// updated. `hairPositionsLast' may also be updated to provide a
		// dampening effect only.
		//

		// Loop through the follicle, starting at the root and advancing
		// toward the tip.
		//
		int		numSegments = hairPositions.length() - 1;
		int		seg;
		for ( seg = 0; seg < numSegments; ++seg ) {
			// Get the desired position of the segment (i.e. where the
			// solver is trying to put the hair for the current frame)
			// and the velocity required to get to that desired position.
			//
			// Thus,
			//		P = hairPositions		// Desired pos'n at cur frame.
			//		L = hairPositionsLast	// Position at prev frame.
			//		V = P - L				// Desired velocity of hair.
			//
			MVector pStart = hairPositions[seg];
			MVector pEnd = hairPositions[seg + 1];

			MVector vStart = pStart - hairPositionsLast[seg];
			MVector vEnd = pEnd - hairPositionsLast[seg + 1];

			// The proper way to time sample is to intersect the moving
			// segment of width `hairWidths[seg] to hairWidths[seg + 1]'
			// with the moving collision object. For the purposes of our
			// demo plug-in, we will simply assume the collision object is
			// static, the hair segment has zero width, and instead of
			// intersecting continuously in time, we will sample discrete
			// steps along the segment.
			//
			#define STEPS 4
			int		step;
			for ( step = 0; step < STEPS; ++step ) {
				// Compute the point for this step and its corresponding
				// velocity. This is a "time swept" point:
				//	p1 = desired position at current time
				//	p0 = position at previous time to achieve desired pos
				//
				MVector pCur, pPrev, v;
				double fracAlongSeg = step / ( (double) STEPS );
				v = vStart * ( 1.0 - fracAlongSeg ) + vEnd * fracAlongSeg;
				MVector p1 = pStart * ( 1.0 - fracAlongSeg )
						+ pEnd * fracAlongSeg;
				MVector p0 = p1 - v;

				// See if BOTH endpoints are outside of the bounding box
				// on the same side. If so, then the segment cannot
				// intersect the bounding box. Note that we assume the
				// bounding box is static.
				//
				if (       p0.x < co->minx && p1.x < co->minx
						|| p0.y < co->miny && p1.y < co->miny
						|| p0.z < co->minz && p1.z < co->minz
						|| p0.x > co->maxx && p1.x > co->maxx
						|| p0.y > co->maxy && p1.y > co->maxy
						|| p0.z > co->maxz && p1.z > co->maxz ) {
					continue;
				}

				// For the purposes of this example plug-in, we'll assume
				// the hair always moves downwards (due to gravity and
				// thus in the negative Y direction). As such, we only
				// need to test for collisions with the TOP of the
				// bounding box. Expanding the example to all 6 sides is
				// left as an exercise for the reader.
				//
				// Remember that p1 is the point at current time, and
				// p0 is the point at the previous time. Since we assume
				// the bounding box to be static, this simplifies things.
				//
				MVector	where(-100000,-100000,-100000);	// Loc'n of collision
				double  fracTime;	// Time at which collision happens 0..1

				if ( fabs( v.y ) < EPSILON		// velocity is zero
						&& fabs( p1.y - co->maxy ) < EPSILON ) {	// right on the bbox
					// Velocity is zero and the desired location (p1) is
					// right on top of the bounding box.
					//
					where = p1;
					fracTime = 1.0;		// Collides right at end;
				} else {
					fracTime = ( co->maxy -  p0.y ) / v.y;
					if ( fracTime >= 0.0 && fracTime <= 1.0 ) {
						// Compute the collision of the swept point with the
						// plane defined by the top of the bounding box.
						//
						where = p0 + v * fracTime;
					}
				}

				// If `seg' lies between startIndex and endIndex
				// we can move it. If its <= startIndex, the root is
				// locked and if >= endIndex the tip is locked.
				//
				if ( seg >= startIndex && seg <= endIndex ) {
					// Since we are just colliding with the top of the
					// bounding box, the normal where we hit is (0,1,0).
					// For the object velocity, we SHOULD measure the
					// relative motion of the object during the time
					// interval, but for our example, we'll assume its
					// not moving (0,0,0).
					//
					bool segCollides = false;
					MVector normal(0.0,1.0,0.0);	// normal is always up
					MVector objectVel(0.0,0.0,0.0);	// assume bbox is static

					// If we get the this point, then the intersection
					// point `where' is on the plane of the bounding box
					// top. See if it lies within the actual bounds of the
					// boxtop, and if so compute the position and velocity
					// information and apply to the hair segment.
					//
					if ( where.x >= co->minx && where.x <= co->maxx
							&& where.z >= co->minz && where.z <= co->maxz
							&& fracTime >= 0.0 && fracTime <= 0.0 ) {
						// We have a collision at `where' with the plane.
						// Compute the new velocity for the hair at the
						// point of collision.
						//
						MVector objVelAlongNormal = (objectVel * normal)
								* normal;
						MVector objVelAlongTangent = objectVel
								- objVelAlongNormal;
						MVector pntVelAlongTangent = v - ( v * normal )
								* normal;
						MVector reflectedPntVelAlongTangent
								= pntVelAlongTangent * ( 1.0 - friction )
								+ objVelAlongTangent * friction;
						MVector newVel = objVelAlongNormal
								+ reflectedPntVelAlongTangent;

						// Update the hair position. It actually looks
						// more stable from a simulation standpoint to
						// just move the closest segment endpoint, but
						// you are free to experiment. What we'll do in
						// our example is move the closest segment end-
						// point by the amount the collided point (where)
						// has to move to have velocity `newVel' yet still
						// pass through `where' at time `fracTime'.
						//
						if ( fracAlongSeg > 0.5 ) {
							MVector	deltaPos = -newVel * fracTime;
							hairPositionsLast[seg + 1] += deltaPos;
							hairPositions[seg + 1] = hairPositionsLast[seg]
									+ newVel;
						} else {
							MVector	deltaPos = newVel * fracTime;
							hairPositionsLast[seg] += deltaPos;
							hairPositions[seg] = hairPositionsLast[seg]
									+ newVel;
						}
						segCollides = true;
					}

                    // Check for segment endpoints that may still be
					// inside. Note that segments which started out being
					// totally inside will never collide using the
					// algorithm we use above, so we'll simply clamp them
					// here. One might expect an inside-the-bounding-box
					// test instead. However, this does not work if the
					// collision object is a very thin object.
					//
					bool inside = false;
                    if ( belowCollisionPlane( co, hairPositions[seg] ) ) {
						hairPositions[seg].y = co->maxy + EPSILON;
						hairPositionsLast[seg] = hairPositions[seg] - objectVel;
						inside = true;
                    }
                    if ( belowCollisionPlane( co, hairPositions[seg+1] ) ) {
						hairPositions[seg+1].y = co->maxy + EPSILON;
						hairPositionsLast[seg+1] = hairPositions[seg+1] - objectVel;
						inside = true;
                    }

					// If we collided, go onto the next segment.
					//
					if ( segCollides || inside ) {
						goto nextSeg;
					}
				}
			}
nextSeg:;
    	}
	}

	// You could perform any global filtering that you want on the hair
	// right here. For example: smoothing. This code is independent of
	// whether or not a collision occurred, and note that collide() is
	// called even with zero collision objects, so you will be guaranteed
	// of reaching here once per hair per iteration.
	//

	return( true );
}

//////////////////////////////////////////////////////////////////////////
//
// Synopsis:
//		MStatus initializePlugin( MObject obj )
//
// Description:
//		Invoked upon plug-in load to register the plug-in and initialise.
//
// Parameters:
//		MObject obj			: (in)	Plug-in object being loaded.
//
// Returns:
//		MStatus::kSuccess	: Successfully performed any needed initial-
//							  isation for the plug-in.
//		MStatus statusCode	: Error was detected.
//
//////////////////////////////////////////////////////////////////////////


MStatus initializePlugin( MObject obj )
{
	MFnPlugin	plugin( obj, "Autodesk", "8.0", "Any" );

	CHECK_MSTATUS( MHairSystem::registerCollisionSolverCollide( collide ) );
	CHECK_MSTATUS( MHairSystem::registerCollisionSolverPreFrame( preFrame ) );

	return( MS::kSuccess );
}

//////////////////////////////////////////////////////////////////////////
//
// Synopsis:
//		MStatus uninitializePlugin( MObject obj )
//
// Description:
//		Invoked upon plug-in unload to deregister the plug-in and clean up.
//
// Parameters:
//		MObject obj			: (in)	Plug-in object being unloaded.
//
// Returns:
//		MStatus::kSuccess	: Successfully performed any needed cleanup.
//		MStatus statusCode	: Error was detected.
//
//////////////////////////////////////////////////////////////////////////


MStatus uninitializePlugin( MObject obj )
{
	MFnPlugin	plugin( obj );

	CHECK_MSTATUS( MHairSystem::unregisterCollisionSolverCollide() );
	CHECK_MSTATUS( MHairSystem::unregisterCollisionSolverPreFrame() );

	return( MS::kSuccess );
}
