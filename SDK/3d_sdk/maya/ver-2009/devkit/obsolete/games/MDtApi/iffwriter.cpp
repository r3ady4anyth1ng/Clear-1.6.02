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

// Need to work around some file problems on the NT platform with Maya 1.0

#ifdef WIN32

#include <math.h>

// The include file flib.h needs to be modified on the NT
// platform so that HAWExport.h and NTDependencies.h not included.
//
// i.e. make changes such that flib looks like:
//
// #ifndef FCHECK
// //#include <HAWExport.h>
// #else
// #define FND_EXPORT
// #endif
// //#include <"NTDependencies.h"
// #endif
//

// The following include and typedef are sufficient to enable the 
// translator to be compiled.

typedef unsigned int uint;

#include <maya/MTypes.h>

#ifndef __uint32_t
typedef __int32 __uint32_t;
#endif

// End of the NT specific modifications (Maya NT 1.0)

#endif

#ifdef AW_NEW_IOSTREAMS
#include <iostream>
#else
#include <iostream.h>
#endif

#include <maya/MString.h>
#include <maya/MStatus.h>

#include <maya/ilib.h>
#include <iffwriter.h>

IFFimageWriter::IFFimageWriter ()
{
        fImage = NULL;
        fBuffer = NULL;
        fZBuffer = NULL;
		
		Bpp = 1;
		Xsize = 64;
		Ysize = 64;

}

IFFimageWriter::~IFFimageWriter ()
{
        close ();
}

MStatus IFFimageWriter::open (MString filename)
{
        close ();

		fImage = ILopen (filename.asChar (), "wb");
        if (NULL == fImage)
                return MS::kFailure;

        // Write/Read top-to-bottom, not bottom-to-top
		//ILctrl (fImage, ILF_Updown, 1);
		
        return MS::kSuccess;
}

MStatus IFFimageWriter::close ()
{
        if (NULL == fImage)
                return MS::kFailure;

        if (ILclose (fImage))
        {
                fImage = NULL;
                return MS::kFailure;
        }

        fImage = NULL;

		if ( fBuffer ) 
			free( fBuffer );
		fBuffer = NULL;

        return MS::kSuccess;
}

MStatus IFFimageWriter::outFilter ( const char *filter )
{
        if (NULL == fImage)
                return MS::kFailure;
                
        ILoutfilter ( filter );
        
        return MS::kSuccess;
}       

MStatus IFFimageWriter::setSize (int x, int y)
{
        if (NULL == fImage)
		{
            return MS::kFailure;
		}

        if (ILsetsize (fImage, x, y))
		{
            return MS::kFailure;
		}

		Xsize = x;
		Ysize = y;

        return MS::kSuccess;
}

MStatus IFFimageWriter::setBytesPerChannel (int bpp)
{
		if (NULL == fImage)
        {   
			return MS::kFailure;
		}
		
        if (ILsetbpp (fImage, bpp) )
        {   
			return MS::kFailure;
		}

		Bpp = bpp;
		return MS::kSuccess;
}

MStatus IFFimageWriter::setType(int type)
{
        if (NULL == fImage)
        {   
            return MS::kFailure;
		}
				
        if (ILsettype(fImage, type) )
        {   
            return MS::kFailure;
		}
				
        return MS::kSuccess;
}

MStatus IFFimageWriter::writeImage ( byte *fbuff, float *fzbuffer, int stride )
{
		int x, y;

        if (NULL == fImage)
        {   
            return MS::kFailure;
		}

        if (NULL == fbuff )
        {   
            return MS::kFailure;
		}

		if ( fBuffer )
			free ( fBuffer );

		fBuffer = (byte *)malloc( Xsize * Ysize * 4 );
	
	    const byte *pixel = fbuff;
	    unsigned char *imagePtr = fBuffer;
	    int bytesPerPixel = (Bpp == 1) ? 4 : 8;
		
		bytesPerPixel = 4;


	    for ( y = 0; y < Ysize; y++ ) {
            
	        for ( x = 0; x < Xsize; x++, pixel += bytesPerPixel ) {
                
            	if ( Bpp == 1 )
            	{
            	    imagePtr[0] = pixel[0];
            	    imagePtr[1] = pixel[1];
            	    imagePtr[2] = pixel[2];
            	    imagePtr[3] = pixel[3];
            	} else {
            	    imagePtr[0] = pixel[0];
            	    imagePtr[1] = pixel[2];
            	    imagePtr[2] = pixel[4];
            	    imagePtr[3] = pixel[6];
            	}   
            	
            	imagePtr += 4;
                     
        	}            
                     
    	}                

        if (ILsave (fImage, fBuffer, fzbuffer, stride))
        {   
			if ( fBuffer )
			{
				free( fBuffer );
				fBuffer = NULL;
			}

            return MS::kFailure;
		}

		if ( fBuffer )
		{
			free( fBuffer );
			fBuffer = NULL;
		}

        return MS::kSuccess;
}

MString IFFimageWriter::errorString ()
{
        return FLstrerror (FLerror ());
}
