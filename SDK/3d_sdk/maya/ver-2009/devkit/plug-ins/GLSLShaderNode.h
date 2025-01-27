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

#ifndef GLSL_SHADER_NODE_H
#define GLSL_SHADER_NODE_H

#include <vector>

#include <maya/MFnNumericAttribute.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectArray.h>
#include <maya/MPxHwShaderNode.h>
#include <maya/MDGModifier.h>
#include <maya/MPxNode.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h> 

#include <IAshliFX.h>

#include "Shader.h"
#include "DefaultShader.h"
#include "glslFXShader.h"

// A wee cache to minimise our gl state changes
//
class glStateCache
{
public:
	inline glStateCache() { reset(); }
	inline void reset() { bits = 0; textureUnit = -1; }
	inline void enablePosition() { if( !(bits & (1 << kPosition))) { glEnableClientState(GL_VERTEX_ARRAY); bits |= (1 << kPosition); } }
	inline void disablePosition() { if( bits & (1 << kPosition)) { glDisableClientState(GL_VERTEX_ARRAY); bits &= ~(1 << kPosition); } }
	inline void enableNormal() { if( !(bits & (1 << kNormal))) { glEnableClientState(GL_NORMAL_ARRAY); bits |= (1 << kNormal); } }
	inline void disableNormal() { if( bits & (1 << kNormal)) { glDisableClientState(GL_NORMAL_ARRAY); bits &= ~(1 << kNormal); } }
	inline void enableColor() { if( !(bits & (1 << kColor0))) { glEnableClientState(GL_COLOR_ARRAY); bits |= (1 << kColor0); } }
	inline void disableColor() { if( bits & (1 << kColor0)) { glDisableClientState(GL_COLOR_ARRAY); bits &= ~(1 << kColor0); } }
	inline void activeTexture( int i) { if( i != textureUnit) { textureUnit = i; glClientActiveTexture( GL_TEXTURE0 + i); } }
	inline void enableAndActivateTexCoord( int i) { activeTexture( i); if( !(bits & (1 << (kTexCoord0 + i)))) { glEnableClientState(GL_TEXTURE_COORD_ARRAY); bits |= (1 << (kTexCoord0 + i)); } }
	inline void disableTexCoord( int i) { if( bits & (1 << (kTexCoord0 + i))) { activeTexture( i); glDisableClientState(GL_TEXTURE_COORD_ARRAY); bits &= ~(1 << (kTexCoord0 + i)); } }

private:

	enum
	{
		kPosition,
		kNormal,
		kColor0,
		kColor1,
		kTexCoord0
	};
	int		bits;
	int		textureUnit;
};
 

class MImage;
class MGeometryData;

class glslShaderNode : public MPxHwShaderNode
{
  public:

    glslShaderNode();
    virtual ~glslShaderNode();

    virtual void postConstructor();

    //compute when DAG change occurs (likely nothing for HwShader)
    virtual MStatus compute( const MPlug& plug, MDataBlock& data );

    //factory for node creation
    static void* creator();

    // initialization
    static MStatus initialize();

    // Drawing commands like the HW renderer needs
    virtual MStatus glBind(const MDagPath& shapePath);
    virtual MStatus glUnbind(const MDagPath& shapePath);
    virtual MStatus	glGeometry( const MDagPath& shapePath, int prim, unsigned int writable, int indexCount,
        const unsigned int * indexArray, int vertexCount, const int * vertexIDs, const float * vertexArray,
        int normalCount, const float ** normalArrays, int colorCount, const float ** colorArrays, int texCoordCount,
        const float ** texCoordArrays);

	// Method to override to let Maya know this shader is batchable. 
	virtual bool	supportsBatching() const;

	// Overridden to draw an image for swatch rendering.
	///
	virtual MStatus renderSwatchImage( MImage & image );

    //
    //attribute connection handling functions
    //
	virtual MStatus	connectionMade( const MPlug& plug, const MPlug& otherPlug, bool asSrc );
	virtual MStatus	connectionBroken( const MPlug& plug, const MPlug& otherPlug, bool asSrc );

	virtual bool getInternalValueInContext( const MPlug&,
											  MDataHandle&,
											  MDGContext&);
    virtual bool setInternalValueInContext( const MPlug&,
											  const MDataHandle&,
											  MDGContext&);
	
    virtual void copyInternalData ( MPxNode * ); 
 
    //
    //attribute request interface
    //
    virtual unsigned int dirtyMask() { return kDirtyAll;};
    virtual int normalsPerVertex();
    virtual int colorsPerVertex();
    virtual int texCoordsPerVertex();
	// See the dependent function parseUseAttributeList(). concerning
	// how this data is filled in.
    virtual int getColorSetNames( MStringArray &names);
    virtual int getTexCoordSetNames( MStringArray &names);

    //query functions
    bool printVertexShader( int pass);
    bool printPixelShader( int pass);

    static glslShaderNode* findNodeByName(const MString &name);

    //
    //static variables
    //

    //shader Type ID to allow it to live in files
    static  MTypeId sId;
    
    //attributes for selecting shaders
	static  MObject	sShader;
	static  MObject	sTechnique;
	static  MObject	sTechniqueList;
	static  MObject sShaderPath;

    //attributes for naked textures
    static MObject sNakedTexCoords[8];

    //attributes for naked colors
    static MObject sNakedColors[2];

    static const char *nakedSetNames[8];

    //call back to configure shaders, post load
    static void rejigShaders( void *data);

  private:


    enum CubeFace { cfLeft, cfRight, cfTop, cfBottom, cfFront, cfBack};

	// Enum to delineate Maya vertex attribute data 
	enum MayaType {
		kNonMaya,
        kNormal,
		kTangent,
		kBinormal,
		kUvSet,
		kColorSet
	};
    MString m_nakedTexSets[8];
    int m_setNums[8]; // These are the actual Maya set numbers
	int m_maxSetNum; // The highest actual set we are using
	MayaType m_mayaType[8];

	glStateCache m_glState;

    MString m_nakedColorSets[2];
    int m_colorSetNums[2]; // These are the actual Maya set numbers
	MayaType m_colorMayaType[2];

    //find a fileTexture2D node from a plug
    MObject findFileTextureNode( const MPlug &plug);

    //find an envCube node from a plug
    MObject findEnvCubeNode( const MPlug &plug);

    void loadCubeFace( CubeFace cf, MObject &cube, MFnDependencyNode &dn);

    void updateBoundAttributes( const MDagPath& shapePath);
    void configureUniformAttrib( MString &name, int pNum, shader::DataType type, shader::Semantic sm, float* defVal,
                                            MFnDependencyNode &dn, MDGModifier &mod);

    //
    // data value updates
    //
    //   Thes routines are called at bind time to propagate attribute
    // state to the shader. It is done at bind time, because all updates
    // arrive piecemeal.
    void BindSamplerData();
    void BindUniformData();

    MString m_shaderName;
    MString m_shaderPath;
    MString m_techniqueName;
    MString m_techniqueList;
    int m_technique; //only figure this out when the technique or shader changes
    shader *m_shader;
    bool m_shaderDirty;

	// The number of passes to render in between bind/unbind. 0 means we're using
	// the default shader
	int m_passCount;

    static shader *sDefShader;

    //
    // these handle uniform parameters exposed to the user
    //  They can be connected to the scene via plugs
    struct uniformAttrib {
      MString name;
      MObject attrib;
      MObject attrib2; //for vec4s
      shader::DataType type; //to track for type matching
      int pNum; //parameter number
      bool dirty;
    };

    std::vector<uniformAttrib> m_uniformAttribList;

    //
    // These handle textures
    //  They can be connected to the scene with a plug
    //  A plug to a texture node allows it to connect to an image
    struct samplerAttrib {
      MString name;
      MObject attrib;
      MObject minFilterAttrib;
      MObject magFilterAttrib;
      MObject wrapAttrib;
      MObject anisoAttrib;
      int pNum;
      GLuint texName;
      GLenum minFilter;
      GLenum magFilter;
      GLenum wrapMode;
      float maxAniso;
      bool dirty;
    };

    std::vector<samplerAttrib> m_samplerAttribList;

    //
    // These handle texture coords
    //  They need to be connected to the scene
    struct attributeAttrib {
      MString name;
      MString setName;
      MObject attrib;
      int handle; //which generic attribite to use
      int set;    //which set id to use from Maya 
	  MayaType mtype; // set type from Maya
      int pNum;
    };

    std::vector<attributeAttrib> m_attributeAttribList;

	// Local variables to keep track information of attrib information
	// that is required from multiple entry points in Maya. 
	//
	bool m_parsedUserAttribList;
	int	m_numUVsets;
	MStringArray m_uvSetNames;
	int m_numColorSets;
	MStringArray m_colorSetNames;
	int m_numNormals;

	void parseUserAttributeList();

    //
    // These handle uniform parameters hidden from the user
    struct boundUniform {
      MString name;
      int pNum;
      shader::Semantic usage;
    };

    std::vector<boundUniform> m_boundUniformList;

    bool rebuildShader();

    //create / delete attributes based on active shader
    void configureAttribs();

    void configureTexCoords( int normalCount, const float ** normalArrays, int texCoordCount,
                                        const float ** texCoordArrays,
										int colorCount, const float **colorArrays );

    bool locateFile( const MString &name, MString &path);

    //this list allows us to access all nodes on callbacks
    // would it be better to just query from Maya?
    static std::vector<glslShaderNode*> sNodeList;


};


#endif //GLSL_SHADER_NODE_H

