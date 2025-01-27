#ifndef _cgfxShaderNode_h_
#define _cgfxShaderNode_h_
//
// Copyright (C) 2002-2003 NVIDIA 
// 
// File: cgfxShaderNode.h
//
// Dependency Graph Node: cgfxShaderNode
//
// Author: Jim Atkinson
//
// Changes:
//  10/2003  Kurt Harriman - www.octopusgraphics.com +1-415-893-1023
//           - Multiple UV sets; user-specified texcoord assignment;
//             error handling.
//  12/2003  Kurt Harriman - www.octopusgraphics.com +1-415-893-1023
//           - attrDefList() no longer calls fAttrDefList->addRef()
//           - Added members: fTechniqueList, getTechniqueList()
//           - Deleted members: fCallbackSet, callbackSet(),
//             setCallback(), technique()
//           - Deleted class cgfxShaderNode::InternalError.
//           - Made protected instead of public: setShaderFxFile,
//             setTechnique, setTexCoordSource
//
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
#include "cgfxShaderCommon.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectArray.h>
#include <maya/MPxHwShaderNode.h>
#include <maya/MPxNode.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h> 

#include "cgfxEffectDef.h"
#include "cgfxAttrDef.h"

// Largest possible number of texture units (GL_MAX_TEXTURE_UNITS) for any
// OpenGL implementation, according to the OpenGL 1.2 multitexture spec.
#define  CGFXSHADERNODE_GL_TEXTURE_MAX  32       // GL_TEXTURE31-GL_TEXTURE0+1
#define  CGFXSHADERNODE_GL_COLOR_MAX 1

class cgfxShaderNode : public MPxHwShaderNode
{
	friend class cgfxShaderCmd;
	friend class cgfxAttrDef;
public:

	cgfxShaderNode();
	virtual void        postConstructor();
	virtual				~cgfxShaderNode(); 


	// Typically, hardware shaders to not need to compute any outputs.
	//
	virtual MStatus compute( const MPlug& plug, MDataBlock& data );

	// Create the node ...
	//
	static  void*		creator();

	// ... and initialize it.
	//
	static  MStatus		initialize();
	static  void        initializeNodeAttrs();

	// We want to track the changes to the shader attribute because
	// it causes us a large amount of work every time it is modified
	// and we want to do that work as rarely as possible.
	//
	virtual void copyInternalData( MPxNode* pSrc );
	virtual bool getInternalValueInContext( const MPlug&,
											  MDataHandle&,
											  MDGContext&);
    virtual bool setInternalValueInContext( const MPlug&,
											  const MDataHandle&,
											  MDGContext&);

	// Tell Maya that Cg effects can be batched
	virtual bool	supportsBatching() const;

	// Tell Maya we require inverted texture coordinates
	virtual bool	invertTexCoords() const;

	// Indicate pass transparency options 
	virtual bool	hasTransparency();
	virtual unsigned int	transparencyOptions();

	// Set up the graphics pipeline for drawing with the shader
	//
	virtual MStatus	glBind(const MDagPath& shapePath);
	void            bindAttrValues();
	void            bindViewAttrValues(const MDagPath& shapePath);


#if MAYA_API_VERSION >= 700
	#define _SWATCH_RENDERING_SUPPORTED_ 1
#endif
#if defined(_SWATCH_RENDERING_SUPPORTED_)
	// Render a swatch
	virtual MStatus renderSwatchImage( MImage & image );
#endif

	// Restore the graphics pipeline back to its original state
	//
	virtual MStatus	glUnbind(const MDagPath& shapePath);

	// Put the pixels on the screen
	//
	virtual MStatus		glGeometry( const MDagPath& shapePath,
		int prim,
		unsigned int writable,
		int indexCount,
		const unsigned int * indexArray,
		int vertexCount,
		const int * vertexIDs,
		const float * vertexArray,
		int normalCount,
		const float ** normalArrays,
		int colorCount,
		const float ** colorArrays,
		int texCoordCount,
		const float ** texCoordArrays);

	// Return the desired number of texture coordinates per vertex
	//
	virtual int     getTexCoordSetNames( MStringArray& names );

	// Return the desired colours per vertex
	//
#if MAYA_API_VERSION >= 700
	virtual int     getColorSetNames( MStringArray& names );
#else
	virtual int		colorsPerVertex();
#endif

	// Return the desired number of normals per vertex
	//
	virtual int		normalsPerVertex();

	// Data accessor for the attrDefList
	//
	cgfxAttrDefList*	attrDefList() const { return fAttrDefList; };

	void setAttrDefList(cgfxAttrDefList* list);

	// Data accessor for the .fx file name (the value of the shader
	// attribute).
	//
	MString shaderFxFile() const { return fShaderFxFile; };
	bool shaderFxFileChanged() const { return fShaderFxFileChanged; }
	void setShaderFxFileChanged(bool val) { fShaderFxFileChanged = val; }

	// Data accessor for the name of the chosen technique.
	MString         getTechnique() const { return fTechnique; };
	const MStringArray& getTechniqueList() const { return fTechniqueList; }

	// Data accessor the attribute list string (the value of the
	// attributeList attribute).
	//
	void getAttributeList(MStringArray& attrList) const;
	void setAttributeList(const MStringArray& attrList);

	// Set the current per-vertex attributes the shader needs (replacing any existing set)
	void				setVertexAttributes( cgfxVertexAttribute* attributeList);

	// Set the data set names that will be populating our vertex attributes
	void				setVertexAttributeSource( const MStringArray& sources);

	// Analyse the per-vertex attributes to work out the minimum set of data we require
	void				analyseVertexAttributes();

	// Data accessor for the texCoordSource attribute.
	const MStringArray& getTexCoordSource() const;

	// Data accessor for the colorSource attribute.
	const MStringArray& getColorSource() const;

	// Data accessor for list of empty UV sets.
	const MStringArray& getEmptyUVSets() const;
	const MObjectArray& getEmptyUVSetShapes() const;

	// Data accessor for whether to use nodes for textures
	inline bool getTexturesByName() const { return fTexturesByName; };

	// Data accessor for the effect itself.
	//
	CGeffect effect() { return fEffect; };
	void setEffect(CGeffect effect);

	MStatus shouldSave ( const MPlug & plug, bool & ret );

	// Get cgfxShader version string.
	static MString  getPluginVersion();

protected:


	// Try and create a missing effect (e.g. once a GL context is available)
	bool			createEffect();

	// Set internal attributes.
	void            setShaderFxFile( const MString& fxFile ) 
	{ 
		if (fxFile != fShaderFxFile)
		{
			// Mark when the shader has changed
			fShaderFxFile = fxFile; 
			fShaderFxFileChanged = true;
		}
	};
	void            setTechniqueHasBlending( CGtechnique technique );
	void            setTechnique( const MString& techn );
	void            setDataSources( const MStringArray* texCoordSources, const MStringArray* colorSources );
	void            updateDataSource( MStringArray& sources, MIntArray& typeList, MIntArray& indexList );
	void            setTexturesByName( bool texturesByName, bool updateAttributes = false);

	// Error reporting
	void            reportInternalError( const char* function, size_t errcode);

public:

	// The typeid is a unique 32bit indentifier that describes this node.
	// It is used to save and retrieve nodes of this type from the binary
	// file format.  If it is not unique, it will cause file IO problems.
	//
	static  MTypeId sId;

	// There needs to be a MObject handle declared for each attribute that
	// the node will have.  These handles are needed for getting and setting
	// the values later.
	//
	// Input shader attribute
	//
	static  MObject	sShader;
	static  MObject	sTechnique;

	// Input list of attributes that map to CgFX uniform parameters
	//
	static	MObject	sAttributeList;

	// Input list of attributes that map to CgFX varying parameters
	//
	static	MObject	sVertexAttributeList;

	// Input list of where to get vertex attributes
	static  MObject sVertexAttributeSource;

	// Input list of where to get texcoords
	static  MObject sTexCoordSource;

	// Input list of where to get colors
	static  MObject sColorSource;

	// Should we use string names or file texture nodes for textures?
	static  MObject sTexturesByName;

	// Cg Context
	static	CGcontext	sCgContext;

	// Cg error callback
	static void			cgErrorCallBack();

	// Cg error handler
	static void			cgErrorHandler(CGcontext cgContext, CGerror cgError, void* userData);


protected:

	// Our description of the effect and it's varying parameters
	//
	cgfxEffect		fNewEffect;

	// The (merged) set of varying parameters the current technique requires
	// and the mapping onto Maya geometry data sets
	cgfxVertexAttribute*	fVertexAttributes;

	// This is a mapping of names to cgfxAttrDef pointers.  Each cgfxAttrDef
	// defines one dynamic attribute
	//
	cgfxAttrDefList* fAttrDefList;

	// Values of internal attributes
	MString			fShaderFxFile;
	bool			fShaderFxFileChanged;
	MString			fTechnique;
	bool			fTechniqueHasBlending;
	MStringArray	fAttributeListArray;
	MStringArray	fVertexAttributeListArray;
	MStringArray	fVertexAttributeSource;
	bool			fTexturesByName;

	// Backward compatibility: these are the old versions of fVertexAttributeSource
	MStringArray    fTexCoordSource;
	MStringArray    fColorSource;

	// The list of maya data we need
	MStringArray	fUVSets;
	MStringArray	fColorSets;
	int             fNormalsPerVertex;

	// These values are derived from the fTexCoordSource value
	// For each texture coord / colour unit, we describe what kind of
	// data should go in there (i.e. should this data be populated with
	// a uv set, a colour set, tangent, normal, etc) AND which set of
	// data to use (i.e. put uv set 0 in texcoord1, and colour set 2 in
	// texcoord 5)
	MIntArray       fTexCoordType;
	MIntArray		fTexCoordIndex;
	MIntArray       fColorType;
	MIntArray		fColorIndex;
	MStringArray    fDataSetNames;

	// This is a pointer to the actual CGeffect for this shader.
	CGeffect		fEffect;

	// Cached info derived from the current CGeffect
	MStringArray    fTechniqueList;

	// Rendering state (only valid between glBind and glUnbind)
	GLboolean		fDepthEnableState;
	GLint			fDepthFunc;
	GLint			fBlendSourceFactor;
	GLint			fBlendDestFactor;

	// Error handling
	bool            fConstructed;      // true => ok to call MPxNode member functions 
	short           fErrorCount;
	short           fErrorLimit;

	// Maya event callbacks
	MIntArray       fCallbackIds;
};

#endif /* _cgfxShaderNode_h_ */
