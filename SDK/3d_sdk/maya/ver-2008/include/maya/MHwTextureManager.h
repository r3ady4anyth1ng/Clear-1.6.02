
#ifndef _MHwTextureManager
#define _MHwTextureManager
//
//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc., and/or its licensors.  All 
// rights reserved.
// 
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors,  which is protected by U.S. and Canadian federal copyright law 
// and by international treaties.
// 
// The Data may not be disclosed or distributed to third parties or be 
// copied or duplicated, in whole or in part, without the prior written 
// consent of Autodesk.
// 
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the following 
// disclaimer, must be included in all copies of the Software, in whole 
// or in part, and all derivative works of the Software, unless such copies 
// or derivative works are solely in the form of machine-executable object 
// code generated by a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, 
// OR ARISING FROM A COURSE OF DEALING, USAGE, OR TRADE PRACTICE. IN NO 
// EVENT WILL AUTODESK AND/OR ITS LICENSORS BE LIABLE FOR ANY LOST 
// REVENUES, DATA, OR PROFITS, OR SPECIAL, DIRECT, INDIRECT, OR 
// CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK AND/OR ITS LICENSORS HAS 
// BEEN ADVISED OF THE POSSIBILITY OR PROBABILITY OF SUCH DAMAGES. 
// ==========================================================================
//+
//
// CLASS:    MHwTextureManager
//
// *****************************************************************************
//
// CLASS DESCRIPTION (MHwTextureManager)
//
// The MHwTextureManager class allows you to define new fixed and floating point
// image file format support in Maya.
//
//
// *****************************************************************************

#if defined __cplusplus

// *****************************************************************************

// INCLUDED HEADER FILES


#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MTypes.h>
#include <maya/MImageFileInfo.h>

// *****************************************************************************

// CLASS DECLARATION (MHwTextureManager)

/// Hardware Texture management (OpenMayaRender) (OpenMayaRender.py)
/**
This class provides methods for managing hardware texture resources.
*/
#ifdef _WIN32
#pragma warning(disable: 4522)
#endif // _WIN32

typedef int MHwTextureFileHandle;

class OPENMAYARENDER_EXPORT MHwTextureManager  
{
public:
	///
	static MStatus	glBind( const MObject& fileTextureObject, 
							MImageFileInfo::MHwTextureType& targetType,
							MImageFileInfo::MImageType imageType = MImageFileInfo::kImageTypeUnknown);

	///
	static MStatus	glBind( const MPlug& fileTextureConnection, 
							MImageFileInfo::MHwTextureType& targetType,
							MImageFileInfo::MImageType imageType = MImageFileInfo::kImageTypeUnknown);

	///
	static MStatus	registerTextureFile( const MString& fileName,
										 MHwTextureFileHandle& hTexture);

	///
	static MStatus	deregisterTextureFile( const MHwTextureFileHandle& hTexture );

	///
	static MStatus	textureFile( const MHwTextureFileHandle& hTexture,
								MString& fileName );

	///
	static MStatus	glBind( const MHwTextureFileHandle& hTexture,
							MImageFileInfo::MHwTextureType& targetType,
							MImageFileInfo::MImageType imageType = MImageFileInfo::kImageTypeUnknown);

protected:
	static const char* className();

private:
	~MHwTextureManager();
#ifdef __GNUC__
	friend class shutUpAboutPrivateDestructors;
#endif
};

#ifdef _WIN32
#pragma warning(default: 4522)
#endif // _WIN32

// *****************************************************************************
#endif /* __cplusplus */
#endif /* _MHwTextureManager */