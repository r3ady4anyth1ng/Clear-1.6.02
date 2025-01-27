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

///////////////////////////////////////////////////////////////////
// DESCRIPTION: 
//
//	Example DDS floating point image reader plugin.
//
///////////////////////////////////////////////////////////////////

#include <maya/MPxImageFile.h>
#include <maya/MImageFileInfo.h>
#include <maya/MImage.h>
#include <maya/MFnPlugin.h>
#include <maya/MStringArray.h>
#include <maya/MIOStream.h>

#if _WIN32   
#pragma warning( disable : 4290 )		// Disable STL warnings.
#endif

#include "ddsFloatReader.h"
#include <math.h>

using namespace dds_Float_Reader;
MString kImageFormatName( "DDS Float");

class ddsFloatReader : public MPxImageFile
{
public:
                    ddsFloatReader();
    virtual         ~ddsFloatReader();
    static void*	creator();
	virtual MStatus open( MString pathname, MImageFileInfo* info);
	virtual MStatus load( MImage& image, unsigned int idx);
	virtual MStatus close();

protected:
	// Data members 
	unsigned int		fWidth;
	unsigned int		fHeight;
	unsigned int		fNumChannels;
	unsigned int		fBytesPerPixel;

	// File and header description
	FILE				*fInputFile;
	DDS_HEADER			fHeader;
};

//
// DESCRIPTION:
///////////////////////////////////////////////////////
ddsFloatReader::ddsFloatReader()
:	fWidth(0), 
	fHeight(0), 
	fNumChannels(0), 
	fBytesPerPixel(0),
	fInputFile(NULL)
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
ddsFloatReader::~ddsFloatReader()
{
	close();
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
/* virtual */
MStatus  ddsFloatReader::close()
{
	fWidth = 0;
	fHeight = 0;
	fNumChannels = 0;
	fBytesPerPixel = 0;

	// Close our file
	if (fInputFile != NULL)
		fclose(fInputFile);
	fInputFile = NULL;

	return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void * ddsFloatReader::creator()
{
    return new ddsFloatReader();
}

// Need to swap byte order on Mac
//
inline void swap_endian(void *val)
{
#if defined(OSMac_)
    unsigned int *ival = (unsigned int *)val;

    *ival = ((*ival >> 24) & 0x000000ff) |
            ((*ival >>  8) & 0x0000ff00) |
            ((*ival <<  8) & 0x00ff0000) |
            ((*ival << 24) & 0xff000000);

#endif
}

inline void swap_endian_half(void *val)
{
#if defined(OSMac_)
    unsigned short *ival = (unsigned short *)val;

    *ival = ((*ival >> 8) & 255) |
            ((*ival << 8) & 65280);

#endif
}

const double two_pow_neg14 = pow(2.0, -14.0);

//
// Half to float conversion
//
inline float halfToFloat(unsigned short val)
{
	float outValue = 0.0f;


#if defined(OSMac_)
	// Need to swap bytes on Power PC (Mac)
	swap_endian_half( &val );
#endif

	// Convert 16-value into a float...
	double h_mantissa = (float) (val & 1023); // Mantissa = low order 10  bits
	double h_exponent = (float)  ((val >> 10) & 31); // Exponent = next 5 bits
	unsigned int i_sign = (val >> 15) & 1;  ; // Sign is the highest bit
	double h_sign = (i_sign == 0) ? 1.0 : -1.0;

	if (h_exponent != 30.0)
	{
		outValue = (float) (h_sign * pow(2.0, h_exponent-15.0) * ( 1.0 + ( h_mantissa / 1024.0 )));
	}
	else
	{
		outValue = (float) ( h_sign * pow(2.0, two_pow_neg14) * ( h_mantissa / 1024.0 ) );
	}
	//printf("[%d][%d] = inValue=%d, isign =%d, sign=%g, exp=%g, mant=%g, value=%g\n", y,x, *inPtr, i_sign, h_sign, h_exponent, h_mantissa, outValue);
	return outValue;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus ddsFloatReader::open( MString filename, MImageFileInfo* info)
{	
#if _WIN32   
    if ( ( fInputFile = fopen( filename.asChar(), "rb" ) ) == NULL )
#else
    if ( ( fInputFile = fopen( filename.asChar(), "r" ) ) == NULL )
#endif
	{
		// Unable to open the file
		return MS::kFailure;
	}

	// Read the DDS header in 
	//
    if ( fread( &fHeader, 1, sizeof(DDS_HEADER), fInputFile) == sizeof(DDS_HEADER) )
	{
		swap_endian(&fHeader.fCapabilities.dwCaps2);

		// Cube maps and volume textures are not supported currently.
		//
		if (( (fHeader.fCapabilities.dwCaps2) & DDSCAPS2_CUBEMAP_FLAG ) ||
			( (fHeader.fCapabilities.dwCaps2) & DDSCAPS2_VOLUME_FLAG ))
		{
			close();
			return MS::kFailure;
		}

		// Get dimensions of image
		//
		swap_endian(&fHeader.fWidth);
		swap_endian(&fHeader.fHeight);
		fWidth = fHeader.fWidth;
		fHeight = fHeader.fHeight;
		fNumChannels = 0;

		if (fWidth==0 || fHeight==0)
		{
			close();
			return MS::kFailure;
		}


		// Check for float formats
		//
		swap_endian(& fHeader.fFormat.fFlags );
		if (fHeader.fFormat.fFlags & DDS_FOURCC_FLAG)
		{
			swap_endian(&fHeader.fFormat.fPixelFormat);

			bool supportedFormat = true;

			// Half float formats
			switch (fHeader.fFormat.fPixelFormat)
			{

			// Half float formats 
			//
			case DDS_R16F:
				fNumChannels = 1;
				fBytesPerPixel = 2; // 16-bits
				break;
			case DDS_G16R16F:
				fNumChannels = 2;
				fBytesPerPixel = 4; // 32-bits
				break;
			case DDS_A16B16G16R16F:
				fNumChannels = 4;
				fBytesPerPixel = 8; // 64-bits
				break;

			// IEEE 32 bit formats
			case DDS_R32F:
				fNumChannels = 1;
				fBytesPerPixel = 4; // 32-bits
				break;
			case DDS_G32R32F:
				fNumChannels = 2;
				fBytesPerPixel = 8; // 64-bits
				break;
			case DDS_A32B32G32R32F:
				fNumChannels = 4;
				fBytesPerPixel = 16; // 128-bits
				break;

			// Anything else is not supported
			default:
				supportedFormat = false;
				break;
			}

			if (!supportedFormat)
			{
				close();
				return MS::kFailure;
			}
		}
		else
		{
			close();
			return MS::kFailure;
		}

		// Return image information based on the header
		//
		if (info)
		{
			//printf("** Opened DDS: %s (w=%d,h=%d, fmt=%d)\n",
			//	filename.asChar(),
			//	fWidth,
			//	fHeight,
			//	fHeader.fFormat.fPixelFormat);

			// Only read in 1 image for now
			info->numberOfImages( 1 );
			info->width( fWidth );
			info->height( fHeight );
			info->channels( fNumChannels );
			info->hasAlpha( fNumChannels == 4 );

			// Mip maps not handled for now
			info->hasMipMaps( false );

			info->imageType( MImageFileInfo::kImageTypeColor );
			info->hardwareType( MImageFileInfo::kHwTexture2D );
			info->pixelType( MImage::kFloat ); 
		}
	}
	return MS::kSuccess;
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus ddsFloatReader::load( MImage& image, unsigned int imageNumber)
{
	MStatus loaded = MS::kFailure;

	// Check for half type, since we need to perform
	// some special bit conversions to get a float back.
	//
	bool isHalfType = false;
	switch (fHeader.fFormat.fPixelFormat)
	{
	case DDS_R16F:
		isHalfType = true;
		break;
	case DDS_G16R16F:
		isHalfType = true;
		break;
	case DDS_A16B16G16R16F:
		isHalfType = true;
		break;
	default:
		break;
	}	

	// Create the output buffer
	//
	image.create( fWidth, fHeight, fNumChannels, MImage::kFloat);
	float* outputBuffer = image.floatPixels();

	// Create the input buffer to read one scanline at a time.
	//
	float *inputFloatBuffer = NULL;			// For IEEE float
	unsigned short *inputIntBuffer = NULL;	// For half-float = 16-bits per channel
	if (!isHalfType)
	{
		inputFloatBuffer = new float[fWidth * fNumChannels];
	}
	else
		inputIntBuffer= new unsigned short[fWidth * fNumChannels];

	// Can't get sufficient memory so fail and close up the
	// file.
	if ((!inputFloatBuffer && !inputIntBuffer) || !outputBuffer)
	{
		if (inputFloatBuffer)
		{
			delete inputFloatBuffer;
			inputFloatBuffer = NULL;
		}
		if (inputIntBuffer)
		{
			delete inputIntBuffer;
			inputIntBuffer = NULL;
		}
		close();
		return MS::kFailure;
	}

	// Transfer to output buffer.
	//
	/// Half float (16-bit)
	if (isHalfType) 
	{
		unsigned int x,y;
		float *outPtr = outputBuffer;

		// Do a scanline at a time. From top-to-bottom
		// so that scan lines are flipped for Maya's usage.
		//
		outPtr += (fHeight-1) * (fWidth * fNumChannels);

		for (y=0; y<fHeight; y++)
		{
			// Read in one scan line of data
			unsigned int numBytes = fWidth * fBytesPerPixel;
			fread( inputIntBuffer, 1, numBytes, fInputFile);
			
			unsigned short *inPtr = inputIntBuffer;

			for (x=0; x<fWidth * fNumChannels; x++)
			{
				float outValue = halfToFloat(*inPtr);
				*(outPtr + x) = outValue;
				inPtr++; // Jump 2 bytes at a time = 1 channel
			}
			outPtr -= (fWidth * fNumChannels);
		}
		loaded = MS::kSuccess;
	}
	
	// IEEE 32-bit float
	else 
	{
		unsigned int x,y;
		float *outPtr = outputBuffer;

		// Do a scanline at a time. From top-to-bottom
		// so that scan lines are flipped for Maya's usage.
		//
		outPtr += (fHeight-1) * (fWidth * fNumChannels);

		for (y=0; y<fHeight; y++)
		{
			// Read in one scan line of data
			unsigned int numBytes = fWidth * fBytesPerPixel;
			fread( inputFloatBuffer, 1, numBytes, fInputFile);
			
#if defined(OSMac_)
			// Need to swap bytes on Power PC (Mac)
			unsigned int *bPtr = (unsigned int *)inputFloatBuffer;
			unsigned int b;
			for (b=0; b<numBytes / 4; b++)
			{
				swap_endian( bPtr );
				bPtr++;
			}
#endif
			float *inPtr = inputFloatBuffer;
			for (x=0; x<fWidth * fNumChannels; x++)
			{
				*(outPtr + x) = *inPtr;
				inPtr++;
			}
			outPtr -= (fWidth * fNumChannels);
		}
		loaded = MS::kSuccess;
	}

	// Cleanup memory and close the file
	if (inputFloatBuffer)
	{
		delete inputFloatBuffer;
		inputFloatBuffer = NULL;
	}
	close();

	return loaded;
}


//////////////////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "8.0", "Any" );
	MStringArray extensions;
	extensions.append( "dds");
    CHECK_MSTATUS( plugin.registerImageFile( 
					kImageFormatName,
					ddsFloatReader::creator, 
					extensions));
    
    return MS::kSuccess;
}

// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );
    CHECK_MSTATUS( plugin.deregisterImageFile( kImageFormatName ) );

    return MS::kSuccess;
}

