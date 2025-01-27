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

///////////////////////////////////////////////////////////////////////////////
//
// File Name: geometryCacheFile.cpp
//
// Description : An interface used for reading cache file data, storing it 
// and converting it to ASCII. See geometryCacheFile.h for file format info.
//
///////////////////////////////////////////////////////////////////////////////

// Project includes
//
#include <geometryCacheFile.h>
#include <geometryCacheBlockBase.h>
#include <geometryCacheBlockStringData.h>
#include <geometryCacheBlockIntData.h>
#include <geometryCacheBlockDVAData.h>
#include <geometryCacheBlockFVAData.h>

// Maya includes
//
#include <maya/flib.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>

// Other includes
//
#include <fstream>

#if defined (OSMac_)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Methods
//
///////////////////////////////////////////////////////////////////////////////

geometryCacheFile::geometryCacheFile( const MString& fileName, MIffFile* iffFile )
	:cacheFileName( fileName )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Constructor
//
///////////////////////////////////////////////////////////////////////////////
{	
	readStatus = false;
	iffFilePtr = iffFile;
}

geometryCacheFile::~geometryCacheFile()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Destructor
//
///////////////////////////////////////////////////////////////////////////////
{
	// Free all the pointers in the blockList.
	// Start from the back of the list, delete the pointer returned then 
	// pop it off the stack.
	//
	while( blockList.size() )
	{	
		// Get current pointer
		//
		geometryCacheBlockBase * block = blockList.back();

		delete block;

		// Pop the back item of the stack
        //
		blockList.pop_back();
	}
	
	// Close the cache file
	//
	iffFilePtr->close();
}

const MString& geometryCacheFile::fileName()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Returns the file name
//
///////////////////////////////////////////////////////////////////////////////
{
	return cacheFileName;
}

const bool& geometryCacheFile::isRead()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Indicates if the file has been read
//
///////////////////////////////////////////////////////////////////////////////
{
	return readStatus;
}

bool geometryCacheFile::readCacheFiles()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Read the cache file
//
///////////////////////////////////////////////////////////////////////////////
{	
	MStatus status = MS::kSuccess;

	// Attempt to open the file
	//
	status = iffFilePtr->open( cacheFileName );
	if( !status ) return false;

	// Proceed if the file is open
	//
	if( iffFilePtr->isActive() ) {
		// Read the Header
		//
		readStatus	= readHeaderGroup( status );
		if( !readStatus ) return false;

		// Read all the channel groups
		//
		do {
			// Read the next channel group
			//
			readStatus = readChannelGroup( status );
			if( !readStatus ) return false;

		} while ( status == MS::kSuccess );

	} else
		// If file failed to open
		//
		return false;

	return true;
}

bool geometryCacheFile::convertToAscii()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Convert the file to Ascii
//
///////////////////////////////////////////////////////////////////////////////
{
	// Generate an output file name by changing the file name extention to txt
	//
	int loc = cacheFileName.rindex('.');
	MString outputFileName = cacheFileName.substring(0, loc-1);
	outputFileName += ".txt";

	// Create an output file steam to flush our data to ascii
	//
	std::ofstream oFile( outputFileName.asChar() );
	if( !oFile.bad() ) 
	{
		// Create an iterator to iterate through the blockList
		//
		cacheBlockIterator blockIt;

		// Write out all the blocks in the blockList
		//
		for( blockIt = blockList.begin();
			blockIt != blockList.end();
			blockIt++ )
		{
			// Get the current block
			//
			geometryCacheBlockBase* block = *blockIt;
			
			// OutputToAscii
			//
			block->outputToAscii( oFile );
		}
	} else
		// If output file stream could not open the file
		//
		return false;

	// Output a message to indicate that the file has been converted
	//
	MGlobal::displayInfo( "Converted file \"" +
							cacheFileName +
							"\" to file \"" +
							outputFileName + 
							"\"" );
	return true;
}

bool geometryCacheFile::readHeaderGroup( MStatus& status )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read the Header group CACH. 
//		The header of every disk cache file consists of these tags
//
//		CACH	( header group )
//		VRSN	( version )
//		STIM	( startTime in ticks )
//		ETIM 	( endTime in ticks )
//
///////////////////////////////////////////////////////////////////////////////
{
	MIffTag tmpTag;
	MIffTag cachTag;

	status = iffFilePtr->beginReadGroup( tmpTag, cachTag );
	if( status ) {
		if( cachTag == MIffTag('C', 'A', 'C', 'H') ) {
			// Store a "CACH" tag into the blockList
			//
			storeCacheBlock( "CACH" );

			// Read the header version
			//
			readStatus = readHeaderVersion();
			if( !readStatus ) return false;

			// Read the Time Range
			//
			readStatus = readHeaderTimeRange();
			if( !readStatus ) return false;

			// Store an ending "/CACH" tag into the blockList
			//
			storeCacheBlock( "/CACH" );
		}		
	}
	iffFilePtr->endReadGroup();

	return true;
}

bool geometryCacheFile::readHeaderVersion()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the cache file version from the VRSN block.
//
///////////////////////////////////////////////////////////////////////////////
{
	MStatus status = MS::kSuccess;
	MIffTag tmpTag;
	unsigned int byteCount;
    
	status = iffFilePtr->beginGet( tmpTag, (unsigned *)&byteCount );
	if( !status ) return false;

	char *tmpName = (char*)alloca(sizeof(char)* (byteCount+1));
	//pointers allocated by alloca are from the stack and automatically freed when
	//the function exits.  Do not free explicitly.

	if( tmpName && tmpTag == MIffTag('V', 'R', 'S', 'N') ) {
		// read the data from the block
		//
		iffFilePtr->get( tmpName, byteCount, &status);
		if( !status ) return false;

		// Store the cache file version in the blockList
		//
		storeCacheBlock( "VRSN", tmpName );

		status = iffFilePtr->endGet();
		if( !status ) return false;
	}

	return true;
}

bool geometryCacheFile::readHeaderTimeRange()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the start and end time from the STIM and ETIM blocks.
//		The data from these blocks are only used for multiple cache files.
//		If the cache file is a single cache file, then the values will only be 
//		from 0 to 1.
//
///////////////////////////////////////////////////////////////////////////////
{
	MStatus status = MS::kSuccess;
	MIffTag tmpTag;
	unsigned int byteCount;
	
#if (!defined BYTE_ORDER) || (BYTE_ORDER == BIG_ENDIAN)
	const int *startTime = (int *)iffFilePtr->getChunk( 
								tmpTag, 
								(unsigned int *)&byteCount );  
	if(	startTime == NULL ||
		!(tmpTag == MIffTag ('S', 'T', 'I', 'M')) || 
		byteCount != sizeof(int) )
		return false;

	// Store the start time in the blockList
	//
	storeCacheBlock( "STIM", *startTime );

	const int *endTime = (int *)iffFilePtr->getChunk( 
								tmpTag, 
								(unsigned int *)&byteCount );  
	if(	endTime == NULL ||
		!(tmpTag == MIffTag('E', 'T', 'I', 'M')) ||
		byteCount != sizeof(int) )
		return false;				

	// Store the end time in the blockList
	//
	storeCacheBlock( "ETIM", *endTime );
#else
	const int *startTime = (int *)iffFilePtr->getChunk( tmpTag, (unsigned int *)&byteCount );  
	if(	startTime == NULL ||
		// There is no != operator for MIffTag
		!(tmpTag == MIffTag ('S', 'T', 'I', 'M')) ||
		byteCount != sizeof(int) )
		return false;

	// Store the start time in the blockList
	//
	storeCacheBlock( "STIM", FLswapword(*startTime) );

	const int *endTime = (int *)iffFilePtr->getChunk( tmpTag, (unsigned int *)&byteCount );  
	if(	endTime == NULL ||
		!(tmpTag == MIffTag('E', 'T', 'I', 'M')) ||
		byteCount != sizeof(int) )
		return false;

	// Store the end time in the blockList
	//
	storeCacheBlock( "ETIM", FLswapword(*endTime) );
#endif
	return true;
}

bool geometryCacheFile::readChannelGroup( MStatus& groupStatus )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read the channel group MYCH.
//		The channel group can consist of these following tags
//
//		MYCH	( time group )
//		TIME	( time in ticks )
//		CHNM	( channel name )
//		SIZE 	( size )
//		DVCA	( geometry point data )
//
///////////////////////////////////////////////////////////////////////////////
{
	MIffTag mychTag;
	MIffTag tmpTag;
	unsigned int byteCount;

	groupStatus = iffFilePtr->beginReadGroup(tmpTag, mychTag);
	if( groupStatus == MS::kSuccess ) {
		if( mychTag == MIffTag('M', 'Y', 'C', 'H')) {
			// Store the "MYCH" group in the blockList
			//
			storeCacheBlock( "MYCH" );

			// Get the next tag
			//
			MStatus stat = iffFilePtr->beginGet(tmpTag, (unsigned *)&byteCount);
			if (!stat) return false;

			// If the tag is TIME, then this is a single file cache file
			// Read the time chunk before the channel name chunk
			//
			if( tmpTag == MIffTag('T', 'I', 'M', 'E') )
			{
				// Read channel time
				//
				readStatus = readChannelTime();
				if ( !readStatus ) return false;

				// End Get
				//
				stat = iffFilePtr->endGet();
				if (!stat) return false;
			}

			// Read Channel
			//
			MStatus channelStatus = MS::kSuccess;
			do {
				// Read channel name, size and data
				//
				readStatus = readChannel( channelStatus );
				if( !readStatus ) return false;

			} while ( channelStatus == MS::kSuccess );

			// Store the ending "/MYCH" in the blockList
			//
			storeCacheBlock( "/MYCH" );
		}
	} 
	iffFilePtr->endReadGroup();
	return readStatus;
}

bool geometryCacheFile::readChannelTime()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the channel time from the TIME block.
//
///////////////////////////////////////////////////////////////////////////////
{
	MIffTag tmpTag;
	unsigned int byteCount;

	const int *tmpNum =	(int*) iffFilePtr->getChunk( tmpTag, (unsigned *)&byteCount );
	if( tmpNum && 
		tmpTag == MIffTag('T', 'I', 'M', 'E') && 
		byteCount == sizeof(int)) {
#if (!defined BYTE_ORDER) || (BYTE_ORDER == BIG_ENDIAN)
		// Store the channel time in the blockList
		//
		storeCacheBlock( "TIME", *tmpNum );
#else
		// Store the channel time in the blockList
		//
		storeCacheBlock( "TIME", FLswapword(*tmpNum) );
#endif		
	} 
	return true;
}

bool geometryCacheFile::readChannel( MStatus& channelStatus )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the channel data.
//
///////////////////////////////////////////////////////////////////////////////
{
	readStatus = readChannelName( channelStatus );
	if( !readStatus ) return false;

	// If channelStatus is not MS::Success then this means that there is no
	// more channel data to read.  Return from function.
	//
	if( !channelStatus ) return true;

	readStatus = readChannelData();
	if( !readStatus ) return false;

	return true;
}
bool geometryCacheFile::readChannelName( MStatus& channelStatus )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the channel name from the CHNM block.
//
///////////////////////////////////////////////////////////////////////////////
{
	MStatus lStatus;
	unsigned int byteCount;
	MIffTag chnmTag;
	
	channelStatus = iffFilePtr->beginGet( chnmTag, (unsigned *)&byteCount );
	if( channelStatus == MS::kSuccess ) {
		char *tmpName = (char*)alloca(sizeof(char)* (byteCount+1));
		//pointers allocated by alloca are from the stack and automatically freed when
		//the function exits.  Do not free explicitly.

		if( tmpName && chnmTag == MIffTag('C', 'H', 'N', 'M') ) {
			iffFilePtr->get( tmpName, byteCount, &lStatus);
			if( !lStatus ) return false;
			MString channelName = tmpName;

			// Store the channel name in the blockList
			//
			storeCacheBlock( "CHNM", channelName );
		} else return false;
	} 
	iffFilePtr->endGet();
	return true;
}

bool geometryCacheFile::readChannelData()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Read and store the channel size and geometry point data from the SIZE
//		and DVCA or FVCA blocks.
//
///////////////////////////////////////////////////////////////////////////////
{
    MIffTag tmpTag;
	unsigned int byteCount;
	uint size;

	// Read the channel size
	//
	const int *tmpNum = (int*) iffFilePtr->getChunk(tmpTag, (unsigned *)&byteCount);
	if( tmpNum && 
		tmpTag == MIffTag('S', 'I', 'Z', 'E') && 
		byteCount == sizeof(int)) 
	{
#if (!defined BYTE_ORDER) || (BYTE_ORDER == BIG_ENDIAN)
			size = *tmpNum;
#else
			size = FLswapword(*tmpNum);
#endif
		// Store the channel size in the blockList
		//
		storeCacheBlock( "SIZE", size );
	} else return false;

	// Read the channel data
	//
	if( size ) {
		const void *tmpVec = (double*) iffFilePtr->getChunk(tmpTag, (unsigned *)&byteCount);

		if( tmpVec && 
			tmpTag == MIffTag('D', 'V', 'C', 'A') &&
			size * sizeof(double)*3 == byteCount )
		{
			double *tmpVecDbl = (double*) tmpVec;
			double *dataArray = new double[size*3];
#if (!defined BYTE_ORDER) || (BYTE_ORDER == BIG_ENDIAN)
			memcpy(dataArray, tmpVec, byteCount);
#else
			for(unsigned int i = 0; i < size*3; i++) {
				FLswapdouble(tmpVecDbl[i], &(dataArray[i]));
			}
#endif		
			// Store the channel geometry points in the blockList
			//
			storeCacheBlock( "DVCA", dataArray, size );
			delete [] dataArray;
		} else if( tmpVec && 
				   tmpTag == MIffTag('F', 'V', 'C', 'A') &&
				   size * sizeof(float)*3 == byteCount ) {
			float *tmpVecFlt = (float*) tmpVec;
			float *dataArray = new float[size*3];
#if (!defined BYTE_ORDER) || (BYTE_ORDER == BIG_ENDIAN)
			memcpy(dataArray, tmpVec, byteCount);
#else
			for(unsigned int i = 0; i < size*3; i++) {
				FLswapfloat(tmpVecFlt[i], &(dataArray[i]));
			}
#endif		
			// Store the channel geometry points in the blockList
			//
			storeCacheBlock( "FVCA", dataArray, size );
			delete [] dataArray;
		} else {
			return false;
		}
	}
	return true;
}

void geometryCacheFile::storeCacheBlock( const MString& tag )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Stores the specified data into the blockList.
//
///////////////////////////////////////////////////////////////////////////////
{
	blockList.push_back( new geometryCacheBlockBase( tag ) );
}

void geometryCacheFile::storeCacheBlock( const MString& tag, const int& value )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Stores the specified data into the blockList.
//
///////////////////////////////////////////////////////////////////////////////
{
	blockList.push_back( new geometryCacheBlockIntData( tag, value ) );
}

void geometryCacheFile::storeCacheBlock( const MString& tag, const MString& value  )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Stores the specified data into the blockList.
//
///////////////////////////////////////////////////////////////////////////////
{
	blockList.push_back( new geometryCacheBlockStringData( tag, value ) );
}

void geometryCacheFile::storeCacheBlock( const MString& tag, const double* value, const uint& size )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Stores the specified data into the blockList.
//
///////////////////////////////////////////////////////////////////////////////
{
	blockList.push_back( new geometryCacheBlockDVAData( tag, value, size ) );
}


void geometryCacheFile::storeCacheBlock( const MString& tag, const float* value, const uint& size )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( private method )
//		Stores the specified data into the blockList.
//
///////////////////////////////////////////////////////////////////////////////
{
	blockList.push_back( new geometryCacheBlockFVAData( tag, value, size ) );
}
