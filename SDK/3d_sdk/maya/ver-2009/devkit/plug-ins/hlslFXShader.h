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
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HLSL_FX_SHADER_H
#define HLSL_FX_SHADER_H

#include <vector>
#include <string>

#include <IAshliFX.h>
#include <IAshli.h>

#include "Shader.h"
#include "Observer.h"
#include "glExtensions.h"

class hlslFXShader : public shader {
  struct uniform;

  public:

    hlslFXShader( );
    virtual ~hlslFXShader();

  //
  // Shader loading and management routines
  //
  //  These routines are intended to deal with the realities of deferred processing
  // in the Maya environemnt. The shader file ideally needs to be parsed and parameters
  // extracted prior to the availability of a GL context. This forces shader compilation
  // to be defered until first use.
  //
    bool createFromFile( const char *filename);
    bool createFromFX( IAshliFX *fx);
  protected:
    bool parseParameters( const IAshliFX *fx);
    DataType parseUniformType( const char *type);
    Semantic parseUniformSemantic( const char *semantic);
    SamplerType parseSamplerType( const char *type);
    void parseExpression( uniform &u, const char *exp);
    
    void updateHandles(); //configure the handles for the uniforms, etc.
    bool buildShaders(); //build the shaders that we had to defer compilation on
    const char* getVertexShader( int pass);
    const char* getPixelShader( int pass);

  public:
    virtual bool valid();
    virtual int passCount();
    virtual int techniqueCount();
    virtual const char* techniqueName( int n);
    virtual bool build();
    virtual void bind();
	virtual void setShapeDependentState();
    virtual void unbind();
    virtual void setTechnique( int t);
    virtual void setPass( int p);

    //need to have queries for attribs and uniforms
    virtual int uniformCount();
    virtual int samplerCount();
    virtual int attributeCount();
    virtual const char* uniformName(int i);
    virtual DataType uniformType(int i);
    virtual Semantic uniformSemantic(int i);
    virtual float* uniformDefault(int i);
    virtual const char* samplerName(int i);
    virtual SamplerType samplerType(int i);
    virtual const char* attributeName(int i);
    virtual DataType attributeType(int i);
    virtual int attributeHandle(int i) {return 0;};

    //need set functions for current values
    virtual void updateUniformBool( int i, bool val);
    virtual void updateUniformInt( int i, int val);
    virtual void updateUniformFloat( int i, float val);
    virtual void updateUniformBVec( int i, const bool* val);
    virtual void updateUniformIVec( int i, const int* val);
    virtual void updateUniformVec( int i, const float* val);
    virtual void updateUniformMat( int i, const float* val);
    virtual void updateUniformMat( int i, const double* val);
    virtual void updateSampler( int i, unsigned int val);

    //predefined attributes
    virtual bool usesColor();
    virtual bool usesNormal();
    virtual bool usesTexCoord( int set);
    virtual bool usesTangent();
    virtual bool usesBinormal();
    virtual int tangentSlot();
    virtual int binormalSlot();

    //error reporting
    virtual const char* errorString();

  private:

    void deleteShaders();

    std::vector<GLuint> m_fShader;
    std::vector<GLuint> m_vShader;

    //these are to hold the ASHLI/ASHLIFX data
    int m_techniqueCount;
    std::vector<std::string> m_techniqueNames;
    std::vector<int> m_passCount; //per technique
    std::vector<int> m_techniqueOffset; //where does this technique begin
    std::vector<std::string> m_vertexShaders; // vertex shader strings
    std::vector<std::string> m_fragmentShaders; // fragment shader strings
    std::vector<std::string> m_orgVertexShaders; // vertex shader strings
    std::vector<std::string> m_orgFragmentShaders; // fragment shader strings
    std::vector<passState*> m_stateList;
    bool m_valid;


    struct uniform {
      std::string name; //should switch to a string or MString
      DataType type;
      Semantic usage; //handles semantic values
      std::vector<int> fReg; //will need to switch to a vector to handle Multipass
      std::vector<int> vReg; //will need to switch to a vector to handle Multipass
      float fVal[16];
      float defaultVal[16];
      bool dirty;
    };

    struct attribute {
      std::string name;
      DataType type;
      std::vector<int> handle;
    };

    struct sampler {
      std::string name;
      SamplerType type;
      GLuint texObject; 
      std::vector<int> texUnit; //will need to switch to a vector to handle Multipass
      bool dirty;
    };

    std::vector<uniform> m_uniformList;
    std::vector<attribute> m_attributeList;
    std::vector<sampler> m_samplerList;

    bool m_stale;

    bool m_color;
    bool m_normal;
    bool m_tangent;
    bool m_binormal;
    unsigned int m_texMask;

    int m_tangentSlot;
    int m_binormalSlot;

    int m_activePass;
    int m_activeTechnique;

    std::string m_error;

    

};

#endif //GLSL_FX_SHADER_H

