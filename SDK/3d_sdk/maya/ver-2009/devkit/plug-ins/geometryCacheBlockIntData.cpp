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
// File Name : geometryCacheBlockIntData.cpp
//
// Description : Stores and outputs cache blocks that carry int data
//
///////////////////////////////////////////////////////////////////////////////

// Project includes
//
#include <geometryCacheBlockIntData.h>

///////////////////////////////////////////////////////////////////////////////
//
// Methods
//
///////////////////////////////////////////////////////////////////////////////

geometryCacheBlockIntData::geometryCacheBlockIntData( 
		const MString& tag, 
		const int& value ) 
		: intData( value )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Constructor
//
///////////////////////////////////////////////////////////////////////////////
{
	blockTag = tag;
	group = false;
}

geometryCacheBlockIntData::~geometryCacheBlockIntData()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Destructor
//
///////////////////////////////////////////////////////////////////////////////
{	
}

const int& geometryCacheBlockIntData::data()
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Returns the block tag as an MString
//
///////////////////////////////////////////////////////////////////////////////
{	
	return intData;
}

void geometryCacheBlockIntData::outputToAscii( ostream& os )
///////////////////////////////////////////////////////////////////////////////
//
// Description : ( public method )
//		Outputs the data of this block to Ascii
//
///////////////////////////////////////////////////////////////////////////////
{
	MString tabs = "";
	if( !group )
		tabs = "\t";

	os << tabs << "[" << blockTag << "]\n" << tabs << tabs << intData << "\n";
}
