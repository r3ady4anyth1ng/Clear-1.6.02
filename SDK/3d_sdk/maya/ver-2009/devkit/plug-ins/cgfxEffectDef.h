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

// Utilities for dealing with varying attribute on CgFX shaders
//
//
#ifndef _cgfxEffectDef_h_
#define _cgfxEffectDef_h_

// This class holds the definition of a CG effect, including the 
// techniques, passes and varying parameters that it includes. Uniform
// parameters (which apply to all techniques and all passes) are 
// handled in cgfxAttrDef.h/cpp
//

#include "cgfxShaderCommon.h"

class cgfxShaderNode;
class cgfxEffect;
#include <maya/MDagPath.h>
#include <maya/MObjectHandle.h>

// A vertex attribute on the shader
// This describes both the cgfx varying parameter and where the data
// for that parameter is coming from in Maya
//
class cgfxVertexAttribute
{
public:
					cgfxVertexAttribute();

	// What is the CG varying parameter?
	MString			fName;
	MString			fSemantic;
	MString			fUIName;
	MString			fType;

	// Where is the data coming from in Maya?
	MString			fSourceName;
	enum
	{
		kNone,
		kPosition,
		kNormal,
		kUV,
		kTangent,
		kBinormal,
		kColor,
		kBlindData,
		kUnknown
	}				fSourceType;
	int				fSourceIndex;

	// The next vertex attribute in the list
	cgfxVertexAttribute* fNext;
};


//
// A vertex attribute structure (e.g. pack uvSet1 and uvSet2 into a single float4)
//
#define MAX_STRUCTURE_ELEMENTS 16
class cgfxVaryingParameterStructure
{
public:
	// How many elements are in this structure
	int				fLength;

	// How many bytes are in the structure
	int				fSize;

	struct cgfxVaryingParameterElement
	{
		cgfxVertexAttribute* fVertexAttribute;	// Which vertex attribute controls this parameter?
		int					fSize;
	};

	// Our list of elements
	cgfxVaryingParameterElement fElements[ MAX_STRUCTURE_ELEMENTS];
};


//
// A trivial data cache for feeding packed structures
//
class cgfxStructureCache
{
public:
					cgfxStructureCache( const MDagPath& shape, const MString& name, int stride, int count);
					~cgfxStructureCache();
	MObjectHandle	fShape;
	MString			fName;
	int				fElementSize;
	int				fVertexCount;
	char*			fData;
	cgfxStructureCache* fNext;
};


//
// A varying parameter to a pass
//
class cgfxVaryingParameter
{
public:
					cgfxVaryingParameter( CGparameter parameter);
					~cgfxVaryingParameter();

	void			setupAttributes( cgfxVertexAttribute*& vertexAttributes, CGprogram program);
	cgfxVertexAttribute* setupAttribute( MString name, const MString& semantic, CGparameter parameter, cgfxVertexAttribute*& vertexAttributes);
	void			bind( const MDagPath& shape, cgfxStructureCache** cache,
						int vertexCount, const float * vertexArray,
						int normalsPerVertex, int normalCount, const float ** normalArrays,
						int colorCount, const float ** colorArrays,
						int texCoordCount, const float ** texCoordArrays);
	bool			bind( const float* data, int stride);
	void			null();

	CGparameter		fParameter;			// The cg parameter
	MString			fName;				// The name of the parameter
	int				fGLType;			// Which GL parameter does this bind to?
	int				fGLIndex;			// Which GL parameter index (e.g. TEXCOORD*7*) does this use?
	cgfxVertexAttribute* fVertexAttribute;	// Which vertex attribute controls this parameter? (not used if a structure is present)
	cgfxVaryingParameterStructure* fVertexStructure; // The structure of elements feeding this parameter (not used if an attribute is present)
	cgfxVaryingParameter* fNext;		// The next parameter in this pass
};


//
// A pass in a technique
//
class cgfxPass
{
public:
					cgfxPass( CGpass pass, cgfxEffect* effect);
					~cgfxPass();

	void			setupAttributes( cgfxVertexAttribute*& vertexAttributes);
	void			bind( const MDagPath& shape, 
						int vertexCount, const float * vertexArray,
						int normalsPerVertex, int normalCount, const float ** normalArrays,
						int colorCount, const float ** colorArrays,
						int texCoordCount, const float ** texCoordArrays);

	CGpass			fPass;				// The cg pass
	CGprogram		fProgram;			// The vertex program
	MString			fName;				// The name of the pass
	cgfxEffect*		fEffect;			// The effect we're a part of
	cgfxVaryingParameter* fParameters;	// The list of parameters in this pass
	cgfxPass*		fNext;				// The next pass in this technique
};


//
// A technique in an effect
//
class cgfxTechnique
{
public:
					cgfxTechnique( CGtechnique technique, cgfxEffect* effect);
					~cgfxTechnique();

	void			setupAttributes( cgfxVertexAttribute*& vertexAttributes);

	CGtechnique		fTechnique;			// The cg technique
	MString			fName;				// The name of this technique
	cgfxEffect*		fEffect;			// The effect we're a part of
	cgfxPass*		fPasses;			// The list of passes in this technique
	bool			fValid;				// Is this technique valid on the current hardware?
	bool			fMultiPass;			// Is this a multi-pass technique?
	cgfxTechnique*	fNext;				// The next technique in the effect
};


//
// An effect
//
class cgfxEffect
{
public:
					cgfxEffect();
					~cgfxEffect();

	void			setEffect( CGeffect effect);
	void			setupAttributes( cgfxShaderNode* shader);

	void			flushCache();
	void			flushCache( const MDagPath& shape);

	CGeffect		fEffect;			// The cg effect
	cgfxTechnique*	fTechnique;			// The current technique
	cgfxTechnique*	fTechniques;		// The list of techniques in this effect
	cgfxStructureCache* fCache;			// Any cached structures the effect needs
};


#undef VALID
#endif /* _cgfxAttrDef_h_ */
