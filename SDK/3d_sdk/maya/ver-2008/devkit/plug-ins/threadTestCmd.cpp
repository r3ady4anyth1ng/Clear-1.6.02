//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors, which is protected by U.S. and Canadian federal copyright 
// law and by international treaties.
//
// The Data is provided for use exclusively by You. You have the right 
// to use, modify, and incorporate this Data into other products for 
// purposes authorized by the Autodesk software license agreement, 
// without fee.
//
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the 
// following disclaimer, must be included in all copies of the 
// Software, in whole or in part, and all derivative works of 
// the Software, unless such copies or derivative works are solely 
// in the form of machine-executable object code generated by a 
// source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
// PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
// TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
// BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
// DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
// AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
// OR PROBABILITY OF SUCH DAMAGES.
//
// ==========================================================================
//+

#include <math.h>

#include <maya/MIOStream.h>
#include <maya/MSimple.h>
#include <maya/MTimer.h>
#include <maya/MGlobal.h>
#include <maya/MThreadPool.h>

DeclareSimpleCommand( threadTestCmd, PLUGIN_COMPANY, "2008");

typedef struct _threadDataTag
{
	int threadNo;
	long primesFound;
	long start, end;

} threadData;

typedef struct _taskDataTag
{
	long start, end, totalPrimes;

} taskData;

#define NUM_TASKS	16

// No global information used in function
static bool TestForPrime(int val)
{
	int limit, factor = 3;
	limit = (long)(sqrtf((float)val)+0.5f);
	while( (factor <= limit) && (val % factor))
		factor ++;

	return (factor > limit);
}


// Primes finder. This function is called from multiple threads
MThreadRetVal Primes(void *data)
{
	threadData *myData = (threadData *)data;
	for( int i = myData->start + myData->threadNo*2; i <= myData->end; i += 2*NUM_TASKS )
	{
		if( TestForPrime(i) )
			myData->primesFound++;
	}
	return (MThreadRetVal)0;
}

// Function to create thread tasks
void DecomposePrimes(void *data, MThreadRootTask *root)
{
	taskData *taskD = (taskData *)data;
	
	threadData tdata[NUM_TASKS];

	for( int i = 0; i < NUM_TASKS; ++i )
	{
		tdata[i].threadNo    = i;
		tdata[i].primesFound = 0;
		tdata[i].start       = taskD->start;
		tdata[i].end         = taskD->end;

		MThreadPool::createTask(Primes, (void *)&tdata[i], root);
	}

	MThreadPool::executeAndJoin(root);

	for( int i = 0; i < NUM_TASKS; ++i )
	{
		taskD->totalPrimes += tdata[i].primesFound;
	}
}

// Single threaded calculation
int SerialPrimes(int start, int end)
{
	int primesFound = 0;
	for( int i = start; i <= end; i+=2)
	{
		if( TestForPrime(i) )
			primesFound++;
	}
	return primesFound;
}

// Set up and tear down parallel tasks
int ParallelPrimes(int start, int end)
{
	MStatus stat = MThreadPool::init();
	if( MStatus::kSuccess != stat ) {
		MString str = MString("Error creating threadpool");
		MGlobal::displayError(str);
		return 0;
	}

	taskData tdata;
	tdata.totalPrimes = 0;
	tdata.start       = start;
	tdata.end         = end;
	MThreadPool::newParallelRegion(DecomposePrimes, (void *)&tdata);

	// pool is reference counted. Release reference to current thread instance
	MThreadPool::release();

	// release reference to whole pool which deletes all threads
	MThreadPool::release();

	return tdata.totalPrimes;
}

// MSimple command that invokes the serial and parallel thread calculations
MStatus threadTestCmd::doIt( const MArgList& args )
{
	MString introStr = MString("Computation of primes using the Maya API");
	MGlobal::displayInfo(introStr);

	if(args.length() != 2) {
		MString str = MString("Invalid number of arguments, usage: threadTestCmd 1 10000");
		MGlobal::displayError(str);
		return MStatus::kFailure;
	}

	MStatus stat;

	int start = args.asInt( 0, &stat );
	if ( MS::kSuccess != stat ) {
		MString str = MString("Invalid argument 1, usage: threadTestCmd 1 10000");
		MGlobal::displayError(str);
		return MStatus::kFailure;
	}

	int end = args.asInt( 1, &stat );
	if ( MS::kSuccess != stat ) {
		MString str = MString("Invalid argument 2, usage: threadTestCmd 1 10000");
		MGlobal::displayError(str);
		return MStatus::kFailure;
	}

	// start search on an odd number
	if((start % 2) == 0 ) start++;

	// run single threaded
	MTimer timer;
	timer.beginTimer();
	int serialPrimes = SerialPrimes(start, end);
	timer.endTimer();
	double serialTime = timer.elapsedTime();

	// run multithreaded
	timer.beginTimer();
	int parallelPrimes = ParallelPrimes(start, end);
	timer.endTimer();
	double parallelTime = timer.elapsedTime();

	// check for correctness
	if ( serialPrimes != parallelPrimes ) {
		MString str("Error: Computations inconsistent");
		MGlobal::displayError(str);
		return MStatus::kFailure;
	}

	// print results
	double ratio = serialTime/parallelTime;
	MString str = MString("\nElapsed time for serial computation: ") + serialTime + MString("s\n");
	str += MString("Elapsed time for parallel computation: ") + parallelTime + MString("s\n");
	str += MString("Speedup: ") + ratio + MString("x\n");
	MGlobal::displayInfo(str);

	return MStatus::kSuccess;
}