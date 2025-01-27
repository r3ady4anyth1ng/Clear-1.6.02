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
#include <algorithm>

#include "Platform.h"
#include "glslFXShader.h"

#include "ResourceManager.h"

#include "Platform.h"

//
//
//
////////////////////////////////////////////////////////////////////////////////
glslFXShader::glslFXShader() : m_techniqueCount(0), m_valid(false), m_stale(false), m_color(true), m_activePass(0), m_activeTechnique(0), m_normal(true),
m_tangent(true), m_binormal(true), m_texMask(0x1), m_error("") {

  
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
glslFXShader::~glslFXShader() {

	for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
		delete *it;
	}

	deleteShaders();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::deleteShaders() {

	//these assume destroying object 0 is OK

	//schedule all the programs for delete
	{
		for (std::vector<GLuint>::iterator it=m_programList.begin(); it<m_programList.end(); it++) {
			ResourceManager::destroyProgram( *it);
		}
	}
	m_programList.clear();

	//schedule all the vertex shaders for delete
	{
		for (std::vector<GLuint>::iterator it=m_vShaderList.begin(); it<m_vShaderList.end(); it++) {
			ResourceManager::destroyProgram( *it);
		}
	}
	m_vShaderList.clear();

	//schedule all the fragment shaders for delete
	{
		for (std::vector<GLuint>::iterator it=m_fShaderList.begin(); it<m_fShaderList.end(); it++) {
			ResourceManager::destroyProgram( *it);
		}
	}
	m_fShaderList.clear();
}

//
// createFromFile
//
//  This creates a GLSL effect from a file with the specified filename. It
// will not compile the effect, because this function is designed to not
// require a current GL context.
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::createFromFile( const char *filename) {
	m_error = ""; 
  IAshliFX *fx = new IAshliFX;
  fx->init();
  fx->setBinding( IAshliFX::GLSL);
  fx->setFX( filename);
	m_stale = true;
  m_valid = false;

  if (!fx->parse()) {
		//should store a string describing the failure for query
		m_error = "Unable to load FX file\n  ";
    m_error += fx->getError();
    delete fx;
		return false;
	}


  if ( fx->getNumTechniques()) {
    //handle any per-technique processing, none presently required
	}
	else {
    //the effect lacks any technique, consider it invalid
		m_error = "FX file lacked a valid shader";
    delete fx;
		return false;
	}

  if (!parseParameters(fx)) {
    delete fx;
		return false;
  }

  //need to extract all shader data from the fx file here 
  //these are to hold the ASHLIFX data
  m_techniqueCount = fx->getNumTechniques();
  m_techniqueNames.clear();
  m_passCount.clear();
  m_techniqueOffset.clear();
  m_vertexShaders.clear();
  m_fragmentShaders.clear();
  for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }
  m_stateList.clear();
  int totalPasses =0;

  stateObserver *observer = new stateObserver;

  fx->attach( (IObserveFX*)observer);

  for (int ii=0; ii<m_techniqueCount; ii++) {
    int passCount = fx->getNumPasses(ii);
    m_techniqueOffset.push_back(totalPasses);
    ITechniqueFX tech;
    fx->getTechnique(ii, tech);
    m_techniqueNames.push_back(tech.getId());
    m_passCount.push_back(passCount);
    for (int jj=0; jj<passCount; jj++) {
      std::string vString;
      std::string fString;
      if (!fx->isVertexNull(ii,jj))
        vString = fx->getVertexShader( ii, jj);
      if (!fx->isPixelNull(ii,jj))
        fString = fx->getPixelShader( ii, jj);

      m_vertexShaders.push_back(vString);
      m_fragmentShaders.push_back(fString);

      //accumulate the state data
      m_stateList.push_back(new passState);
      observer->setPassMonitor(m_stateList.back());
      for (int kk=0; kk<fx->getNumStates( ii,jj); kk++) {
        fx->getStateItem( ii, jj, kk);
      }
      observer->finalizePassMonitor();
    }
    totalPasses += passCount;
  }
    
  fx->attach(NULL);
  delete observer;
  delete fx;

  m_valid = true;

	return true;
}

//
// createFromFX
//
//  This is similar to createFromFile, except it takes an already parsed
// FX file as input.
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::createFromFX( IAshliFX *fx) {
	m_error = ""; 
	m_stale = true;
  m_valid = false;

  if ( fx->getNumTechniques()) {
		//handle any per-technique processing, none presently required
	}
	else {
		//the effect lacks any technique, consider it invalid
		m_error = "FX file lacked a valid shader";
		return false;
	}

  if (!parseParameters(fx))
		return false;

  //need to extract all shader data from the fx file here 
  //these are to hold the ASHLIFX data
  m_techniqueCount = fx->getNumTechniques();
  m_techniqueNames.clear();
  m_passCount.clear();
  m_techniqueOffset.clear();
  m_vertexShaders.clear();
  m_fragmentShaders.clear();
  for (std::vector<passState*>::iterator it=m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }
  m_stateList.clear();
  int totalPasses =0;

  stateObserver *observer = new stateObserver;

  fx->attach( (IObserveFX*)observer);

  for (int ii=0; ii<m_techniqueCount; ii++) {
    int passCount = fx->getNumPasses(ii);
    m_techniqueOffset.push_back(totalPasses);
    ITechniqueFX tech;
    fx->getTechnique(ii, tech);
    m_techniqueNames.push_back(tech.getId());
    m_passCount.push_back(passCount);
    for (int jj=0; jj<passCount; jj++) {
      std::string vString;
      std::string fString;
      if (!fx->isVertexNull(ii,jj))
        vString = fx->getVertexShader( ii, jj);
      if (!fx->isPixelNull(ii,jj))
        fString = fx->getPixelShader( ii, jj);

      m_vertexShaders.push_back(vString);
      m_fragmentShaders.push_back(fString);

      //accumulate the state data
      m_stateList.push_back(new passState);
      observer->setPassMonitor(m_stateList.back());
      for (int kk=0; kk<fx->getNumStates( ii,jj); kk++) {
        fx->getStateItem( ii, jj, kk);
      }
      observer->finalizePassMonitor();
    }
    totalPasses += passCount;
  }
    
  fx->attach(NULL);
  delete observer;

  m_valid = true;

	return true;
}

//
// parseParameters
//
//  This is the master function that iterates thorugh all the parameters
// in an effect and creates lists of samplers, attributes, and uniforms to
// use display to the user. 
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::parseParameters(const IAshliFX *fx) {
	int numPasses = 0, ii;

	//iterate over all techniques to find the max number of passes, we need to have handles for
  for (ii=0; ii<fx->getNumTechniques(); ii++) {
    int count = fx->getNumPasses( ii);
		numPasses = std::max<int>(numPasses, count);
	}

	//process the exposed shader parameters
	for (ii=0; ii<fx->getNumParameters(); ii++)
	{
		IParameterFX parm;

		//get the handle to the parameter
    fx->getParameter( "", ii, parm);

		//get all the data pointers
		const char *usage = parm.getUsage();
		const char *type = parm.getType();
		const char *name = parm.getId();
		const char *semantic = parm.getSemantic();
		const char *expression = parm.getExpression();

		//determine the type of the parameter
		if (!strcmp( "uniform", usage)) {

			//uniforms can be samplers, and we need to treat them specially
			if (strncmp( "sampler", type, 7)) {
				// this is a regular uniform (not a sampler)

				//should verify the compatibility of the following pieces of data
				shader::DataType dt = parseUniformType( type);
				shader::Semantic sm = parseUniformSemantic( semantic);
				uniform u;
				u.type = dt;
				u.handle.resize( numPasses, -1);
				u.usage = sm;
				u.name = name;
				//parse the expression to extract a default value
				parseExpression( u, expression);
				m_uniformList.push_back(u);
			}
			else {
				// this is a sampler uniform
				shader::SamplerType st = parseSamplerType( type);
				sampler s;
				s.handle.resize( numPasses, -1);
				s.type = st;
				s.texUnit.resize( numPasses, -1);
				s.name = name;
				m_samplerList.push_back(s);
			}
		}
		else if (!strcmp( "attribute", usage)) {
			//need to process the attribute and add it ot the attribute list
			shader::DataType dt = parseUniformType( type);
			attribute a;
			a.type = dt;
			a.handle.resize( numPasses, -1);
			a.name = name;
			m_attributeList.push_back(a);
		}
		else {
			//don't know what else we could encounter, might want to create warnings
		}
	}

	return true;
}

//
// parseUniformType
//
//  This is a helper function to convert a uniform type into a comparable
// type from the shader enumeration.
////////////////////////////////////////////////////////////////////////////////
shader::DataType glslFXShader::parseUniformType( const char *type) {
	if (!strcmp( "float", type)) {
		return shader::dtFloat;
	}
	else if (!strcmp( "vec2", type)) {
		return shader::dtVec2;
	}
	else if (!strcmp( "vec3", type)) {
		return shader::dtVec3;
	}
	else if (!strcmp( "vec4", type)) {
		return shader::dtVec4;
	}
	else if (!strcmp( "int", type)) {
		return shader::dtInt;
	}
	else if (!strcmp( "ivec2", type)) {
		return shader::dtIVec2;
	}
	else if (!strcmp( "ivec3", type)) {
		return shader::dtIVec3;
	}
	else if (!strcmp( "ivec4", type)) {
		return shader::dtIVec4;
	}
	else if (!strcmp( "bool", type)) {
		return shader::dtBool;
	}
	else if (!strcmp( "bvec2", type)) {
		return shader::dtBVec2;
	}
	else if (!strcmp( "bvec3", type)) {
		return shader::dtBVec3;
	}
	else if (!strcmp( "bvec4", type)) {
		return shader::dtBVec4;
	}
	else if (!strcmp( "mat2", type)) {
		return shader::dtMat2;
	}
	else if (!strcmp( "mat3", type)) {
		return shader::dtMat3;
	}
	else if (!strcmp( "mat4", type)) {
		return shader::dtMat4;
	}

	return shader::dtUnknown;
}

//
// parseUniformSemantic
//
//  This is a helper function to convert effect specific semantics into
// the enum semantics defined by the parent class. This function might be a
// candiadate for moving to the parent, since all present classes effect types
// recognize the same semantic keys.
////////////////////////////////////////////////////////////////////////////////
shader::Semantic glslFXShader::parseUniformSemantic( const char *semantic) {

	if (semantic == NULL)
		return shader::smNone;

	//this would be more efficient as a map
	if (!strcasecmp( "World", semantic)) {
		return shader::smWorld;
	}
	else if (!strcasecmp( "View", semantic)) {
		return shader::smView;
	}
	else if (!strcasecmp( "Projection", semantic)) {
		return shader::smProjection;
	}
	else if (!strcasecmp( "WorldView", semantic)) {
		return shader::smWorldView;
	}
	else if (!strcasecmp( "ViewProjection", semantic)) {
		return shader::smViewProjection;
	}
	else if (!strcasecmp( "WorldViewProjection", semantic)) {
		return shader::smWorldViewProjection;
	}
	else if ( (!strcasecmp( "WorldI", semantic)) || (!strcasecmp( "WorldInverse", semantic))) {
		return shader::smWorldI;
	}
	else if ( (!strcasecmp( "ViewI", semantic)) || (!strcasecmp( "ViewInverse", semantic)) ) {
		return shader::smViewI;
	}
	else if ( (!strcasecmp( "ProjectionI", semantic)) || (!strcasecmp( "ProjectionInverse", semantic)) ) {
		return shader::smProjectionI;
	}
	else if ( (!strcasecmp( "WorldViewI", semantic)) || (!strcasecmp( "WorldViewInverse", semantic)) ) {
		return shader::smWorldViewI;
	}
	else if ( (!strcasecmp( "ViewProjectionI", semantic)) || (!strcasecmp( "ViewProjectionInverse", semantic)) ) {
		return shader::smViewProjectionI;
	}
	else if ( (!strcasecmp( "WorldViewProjectionI", semantic)) || (!strcasecmp( "WorldViewProjectionInverse", semantic)) ) {
		return shader::smWorldViewProjectionI;
	}
	else if ( (!strcasecmp( "WorldT", semantic)) || (!strcasecmp( "WorldTranspose", semantic)) ) {
		return shader::smWorldT;
	}
	else if ( (!strcasecmp( "ViewT", semantic)) || (!strcasecmp( "ViewTranspose", semantic)) ) {
		return shader::smViewT;
	}
	else if ( (!strcasecmp( "ProjectionT", semantic)) || (!strcasecmp( "ProjectionTranspose", semantic)) ) {
		return shader::smProjectionT;
	}
	else if ( (!strcasecmp( "WorldViewT", semantic)) || (!strcasecmp( "WorldViewTranspose", semantic)) ) {
		return shader::smWorldViewT;
	}
	else if ( (!strcasecmp( "ViewProjectionT", semantic)) || (!strcasecmp( "ViewProjectionTranspose", semantic)) ) {
		return shader::smViewProjectionT;
	}
	else if ( (!strcasecmp( "WorldViewProjectionT", semantic)) || (!strcasecmp( "WorldViewProjectionTranspose", semantic)) ) {
		return shader::smWorldViewProjectionT;
	}
	else if ( (!strcasecmp( "WorldIT", semantic)) || (!strcasecmp( "WorldInverseTranspose", semantic)) ) {
		return shader::smWorldIT;
	}
	else if ( (!strcasecmp( "ViewIT", semantic)) || (!strcasecmp( "ViewInverseTranspose", semantic)) ) {
		return shader::smViewIT;
	}
	else if ( (!strcasecmp( "ProjectionIT", semantic)) || (!strcasecmp( "ProjectionInverseTranspose", semantic)) ) {
		return shader::smProjectionIT;
	}
	else if ( (!strcasecmp( "WorldViewIT", semantic)) || (!strcasecmp( "WorldViewInverseTranspose", semantic)) ) {
		return shader::smWorldViewIT;
	}
	else if ( (!strcasecmp( "ViewProjectionIT", semantic)) || (!strcasecmp( "ViewProjectionInverseTranspose", semantic)) ) {
		return shader::smViewProjectionIT;
	}
	else if ( (!strcasecmp( "WorldViewProjectionIT", semantic)) || (!strcasecmp( "WorldViewProjectionInverseTranspose", semantic)) ) {
		return shader::smWorldViewProjectionIT;
	}
    else if (!strcasecmp( "ViewPosition", semantic)) {
    return smViewPosition;
  }
  else if (!strcasecmp( "Time", semantic)) {
    return smTime;
  }
  else if (!strcasecmp( "RenderTargetDimensions", semantic)) {
    return smViewportSize;
  }
  else if (!strcasecmp( "Ambient", semantic)) {
    return smAmbient;
  }
  else if (!strcasecmp( "Diffuse", semantic)) {
    return smDiffuse;
  }
  else if (!strcasecmp( "Emissive", semantic)) {
    return smEmissive;
  }
  else if (!strcasecmp( "Specular", semantic)) {
    return smSpecular;
  }
  else if (!strcasecmp( "Opacity", semantic)) {
    return smOpacity;
  }
  else if (!strcasecmp( "SpecularPower", semantic)) {
    return smSpecularPower;
  }
  else if (!strcasecmp( "Height", semantic)) {
    return smHeight;
  }
  else if (!strcasecmp( "Normal", semantic)) {
    return smNormal;
  }

	return shader::smUnknown;
}

//
// parseExpression
//
//  This function is a helper to parse initialization expressions, to allow
// proper default values to be applied to parameters. Future development
// should add a real parser here instead of just identifying the most common
// initialization expressions.
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::parseExpression( glslFXShader::uniform &u, const char *exp) {

	//set the defaults for the maximum type size (mat4)
	for (int ii=0; ii<16; ii++)
		u.fDefault[ii] = 0.0f;

	//the plugin forces strong typing, so only look for exact matches
	switch ( u.type) {
	case shader::dtInt:
		if ( 1 == sscanf( exp, " int ( %f )", &u.fDefault[0])) {
		}
		else if ( 1 == sscanf( exp, " %f ", &u.fDefault[0])) {
		}
		else {
			u.iDefault[0] = 0;
		}
		break;
	case shader::dtFloat:
		if ( 1 == sscanf( exp, " float ( %f )", &u.fDefault[0])) {
		}
		else if ( 1 == sscanf( exp, " %f ", &u.fDefault[0])) {
		}
		else {
			u.fDefault[0] = 0.0f;
		}
		break;
	case shader::dtVec2:
		if ( 2 != sscanf( exp, " vec2 ( %f , %f )", &u.fDefault[0], &u.fDefault[1])) {
			u.fDefault[0] = 0.0f;
			u.fDefault[1] = 0.0f;
		}
		break;
	case shader::dtVec3:
		if ( 3 != sscanf( exp, " vec3 ( %f , %f , %f )", &u.fDefault[0], &u.fDefault[1], &u.fDefault[2])) {
			u.fDefault[0] = 0.0f;
			u.fDefault[1] = 0.0f;
			u.fDefault[2] = 0.0f;
		}
		break;
	case shader::dtVec4:
		if ( 4 != sscanf( exp, " vec4 ( %f , %f , %f , %f )", &u.fDefault[0], &u.fDefault[1], &u.fDefault[2], &u.fDefault[3])) {
			u.fDefault[0] = 0.0f;
			u.fDefault[1] = 0.0f;
			u.fDefault[2] = 0.0f;
			u.fDefault[3] = 0.0f;
		}
		break;
	default:
      //keep GCC happy
		break;
	};
}

//
// parseSamplerType
//
//  Helper function to map the sampler type to the samplerType enumeration
// from shader.
////////////////////////////////////////////////////////////////////////////////
shader::SamplerType glslFXShader::parseSamplerType( const char *type) {
	if (!strcmp( "sampler1D", type)) {
		return shader::st1D;
	}
	else if (!strcmp( "sampler2D", type)) {
		return shader::st2D;
	}
	else if (!strcmp( "sampler3D", type)) {
		return shader::st3D;
	}
	else if (!strcmp( "samplerCube", type)) {
		return shader::stCube;
	}
	else if (!strcmp( "sampler1DShadow", type)) {
		return shader::st1DShadow;
	}
	else if (!strcmp( "sampler2DShadow", type)) {
		return shader::st2DShadow;
	}

	//add sampler RECT stuff here

	return shader::stUnknown;
}

//
// updateHandles
//
//  This function is called at draw time, to ensure that the uniforms
// have the right value handles. It is only exectued when the shader is
// marked dirty.
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateHandles() {
	int passNum=0;

  for (std::vector<GLuint>::iterator pit=m_programList.begin(); pit<m_programList.end(); pit++,passNum++) {
		int texNum = 0;

		{
			for (std::vector<uniform>::iterator it = m_uniformList.begin(); it < m_uniformList.end(); it++) {
				it->handle[passNum] = glGetUniformLocation( *pit, it->name.c_str()); //get the proper location for the new shader
				it->dirty = true; //need to mark it dirty, don't know the old state
			}
		}

		{
			for (std::vector<sampler>::iterator it = m_samplerList.begin(); it < m_samplerList.end(); it++) {
				it->handle[passNum] = glGetUniformLocation( *pit, it->name.c_str()); //get the proper location for the new shader
				it->texUnit[passNum] = texNum++;
				//this should only happen once per compile, we never remap samplers
				it->dirty = true;
			}
		}

		{
			for (std::vector<attribute>::iterator it = m_attributeList.begin(); it < m_attributeList.end(); it++) {
				it->handle[passNum] = glGetAttribLocation( *pit, it->name.c_str());
				//nothing to mark dirty
			}
		}
	}
}

//
// buildShaders
//
//  This is doen when the shader is marked dirty to rebuild the shaders. It
// requires a current context.
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::buildShaders() {

	//clean up any old shaders
	deleteShaders();

	//create all the passes up front
	m_programList.resize( m_passCount[m_activeTechnique], 0);
	m_vShaderList.resize( m_passCount[m_activeTechnique], 0);
	m_fShaderList.resize( m_passCount[m_activeTechnique], 0);


	int offset = m_techniqueOffset[m_activeTechnique];

  for (int ii=0; ii<m_passCount[m_activeTechnique]; ii++) {



		//build the programs
		GLuint program = glCreateProgramObject();
		m_programList[ii] = program;
		bool fail = false;

		GL_CHECK;

		if (m_vertexShaders[offset+ii].length() > 0) {
			const char* temp;
			GLuint vShader = glCreateShaderObject( GL_VERTEX_SHADER);
			m_vShaderList[ii] = vShader;
			glAttachObject( program, vShader);
			temp = m_vertexShaders[offset+ii].c_str();
			glShaderSource( vShader, 1, &temp, NULL);
			glCompileShader(vShader);
			//need to check for success here
			GLint success;
			glGetObjectParameteriv( vShader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char log[256];
				glGetInfoLog( vShader, 256, NULL, log);
				fail = true; //allow fragment shader errors to occur
				m_error = "Vertex shader failed compile:\n";
				m_error += log;
				m_error += "\n";
			}
		}
		GL_CHECK;

		if (m_fragmentShaders[offset+ii].length() > 0) {
			const char* temp;
			GLuint fShader = glCreateShaderObject( GL_FRAGMENT_SHADER);
			m_fShaderList[ii] = fShader;
			glAttachObject( program, fShader);
			temp = m_fragmentShaders[offset+ii].c_str();
			glShaderSource( fShader, 1, &temp, NULL);
			glCompileShader(fShader);
			//need to check for success here
			GLint success;
			glGetObjectParameteriv( fShader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char log[256];
				glGetInfoLog( fShader, 256, NULL, log);
				fail = true; //allow fragment shader errors to occur
				m_error += "Fragment shader failed compile:\n";
				m_error += log;
			}
		}
		GL_CHECK;

		if (fail) {
			glDeleteObject( m_programList[ii]);
			m_programList[ii] = 0;
			if (m_fShaderList[ii]) {
				glDeleteObject( m_fShaderList[ii]);
				m_fShaderList[ii] = 0;
			}
			if (m_vShaderList[ii]) {
				glDeleteObject( m_vShaderList[ii]);
				m_vShaderList[ii] = 0;
			}
			return false;
		}

		glLinkProgram(program);
		GLint success;
		glGetObjectParameteriv( program, GL_LINK_STATUS, &success);
		if (!success) {
			char log[256];
			glGetInfoLog( program, 256, NULL, log);
			m_error = "Link failed:\n";
			m_error += log;
			glDeleteObject( m_programList[ii]);
			m_programList[ii] = 0;
			if (m_fShaderList[ii]) {
				glDeleteObject( m_fShaderList[ii]);
				m_fShaderList[ii] = 0;
			}
			if (m_vShaderList[ii]) {
				glDeleteObject( m_vShaderList[ii]);
				m_vShaderList[ii] = 0;
			}
			GL_CHECK;
			return false;
		}

		GL_CHECK;
	}

	return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::valid() {
  return m_valid;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::passCount() {
  if (m_techniqueCount)
    return m_passCount[m_activeTechnique];
	return 0;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::techniqueCount() {
	//ignoring techniques for now
  return m_techniqueCount;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::techniqueName( int n) {
  if ( n<m_techniqueCount)
    return m_techniqueNames[n].c_str();

  return NULL;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::build() {
	if (m_stale) {
		m_stale = false;
		if (!buildShaders())
			return false;
		updateHandles();
	}
	return true;
}

//
// bind
//
//  This function is used to bind in the active program and set all of
// the associated shape independent state. Please note that only dirty 
// program state is sent to the shader. 
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::bind() {

  

	glUseProgramObject( m_programList[m_activePass]);
	GL_CHECK;

	GLenum targets[] = { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_1D, GL_TEXTURE_2D};

	//update dirty samplers
	for (std::vector<sampler>::iterator it2 = m_samplerList.begin(); it2<m_samplerList.end(); it2++) {
		if ( it2->handle[m_activePass] != -1) {
			if (it2->dirty) {
				glUniform1i( it2->handle[m_activePass], it2->texUnit[m_activePass]);
				//it->dirty = false;
			}
			glActiveTexture( GL_TEXTURE0_ARB + it2->texUnit[m_activePass]);
			glBindTexture( targets[it2->type], it2->texObject);
		}
	}
	glActiveTexture( GL_TEXTURE0_ARB);

	GL_CHECK;

	//set up the render states
  m_stateList[m_techniqueOffset[m_activeTechnique]+m_activePass]->setState();

	GL_CHECK;

	return;
}

//
// setShapeDependentState
//
//  This function is used to setup any shape dependent data
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::setShapeDependentState() {

	//update dirty values
	for (std::vector<uniform>::iterator it = m_uniformList.begin(); it<m_uniformList.end(); it++) {
		if ( it->dirty) {

			//only mark it clean on the last pass
      if ( m_activePass == m_passCount[m_activeTechnique]-1)
				it->dirty = false;

			if ( it->handle[m_activePass] == -1)
				continue; // avoid causing a GL error, by submitting a uniform with a -1 (does not exist) handle

			switch (it->type) {
		case shader::dtBool:
			glUniform1i( it->handle[m_activePass], it->iVal[0]);
			break;
		case shader::dtBVec2:
			glUniform2iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtBVec3:
			glUniform3iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtBVec4:
			glUniform4iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtInt:
			glUniform1i( it->handle[m_activePass], it->iVal[0]);
			break;
		case shader::dtIVec2:
			glUniform2iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtIVec3:
			glUniform3iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtIVec4:
			glUniform4iv( it->handle[m_activePass], 1, it->iVal);
			break;
		case shader::dtFloat:
			glUniform1f( it->handle[m_activePass], it->fVal[0]);
			break;
		case shader::dtVec2:
			glUniform2fv( it->handle[m_activePass], 1, it->fVal);
			break;
		case shader::dtVec3:
			glUniform3fv( it->handle[m_activePass], 1, it->fVal);
			break;
		case shader::dtVec4:
			glUniform4fv( it->handle[m_activePass], 1, it->fVal);
			break;
		case shader::dtMat2:
			glUniformMatrix2fv( it->handle[m_activePass], 1, false, it->fVal);
			break;
		case shader::dtMat3:
			glUniformMatrix3fv( it->handle[m_activePass], 1, false, it->fVal);
			break;
		case shader::dtMat4:
			glUniformMatrix4fv( it->handle[m_activePass], 1, false, it->fVal);
			break;
		default:
			break;
			};
			GL_CHECK;
		}
	}

	return;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::unbind() {
	glUseProgramObject(0);
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::setTechnique( int t) {
	if ( t != m_activeTechnique) {
		m_activeTechnique = t;
		m_stale = true;
	}
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::setPass( int p) {
	m_activePass = p;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::uniformCount() {
  return (int)m_uniformList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::samplerCount() {
  return (int)m_samplerList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::attributeCount() {
  return (int)m_attributeList.size();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::uniformName(int i) {
  //should check for array bounds
  return m_uniformList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::DataType glslFXShader::uniformType(int i) {
  //should check for array bounds
  return m_uniformList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::Semantic glslFXShader::uniformSemantic(int i) {
  //might want to check array bounds
  return m_uniformList[i].usage;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
float* glslFXShader::uniformDefault(int i) {
  //might want to check array bounds
  return m_uniformList[i].fDefault;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::samplerName(int i) {
  //should check for array bounds
  return m_samplerList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::SamplerType glslFXShader::samplerType(int i) {
  //should check for array bounds
  return m_samplerList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::attributeName(int i) {
  //should check for array bounds
  return m_attributeList[i].name.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
shader::DataType glslFXShader::attributeType(int i) {
  return m_attributeList[i].type;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::attributeHandle(int i) {
  return m_attributeList[i].handle[m_activePass];
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformBool( int i, bool val) {
  m_uniformList[i].iVal[0] = val ? 1 : 0;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformInt( int i, int val) {
  m_uniformList[i].iVal[0] = val;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformFloat( int i, float val) {
  m_uniformList[i].fVal[0] = val;
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformBVec( int i, const bool* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].iVal[ii] = val[ii] ? 0 : 1;
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformIVec( int i, const int* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].iVal[ii] = val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformVec( int i, const float* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformMat( int i, const float* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateUniformMat( int i, const double* val) {
  for (int ii=0; ii<size(m_uniformList[i].type); ii++) {
    m_uniformList[i].fVal[ii] = (float)val[ii];
  }
  m_uniformList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslFXShader::updateSampler( int i, unsigned int val) {
  m_samplerList[i].texObject = val;
  //m_samplerList[i].dirty = true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::usesColor() {
  return m_color;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::usesNormal() {
  return m_normal;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::usesTexCoord( int set) {
  return ((m_texMask>>set) & 0x1) == 1;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::usesTangent() {
  return m_tangent;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslFXShader::usesBinormal() {
  return m_binormal;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::tangentSlot() {
  return m_tangentSlot;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslFXShader::binormalSlot() {
  return m_binormalSlot;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::errorString() {
  return m_error.c_str();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::getVertexShader( int pass) {

  if (m_activeTechnique < m_techniqueCount) {
    if ( pass < m_passCount[m_activeTechnique]) {
      return m_vertexShaders[m_techniqueOffset[m_activeTechnique]+pass].c_str();
    }
  }

  return NULL;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
const char* glslFXShader::getPixelShader( int pass) {
  if (m_activeTechnique < m_techniqueCount) {
    if ( pass < m_passCount[m_activeTechnique]) {
      return m_fragmentShaders[m_techniqueOffset[m_activeTechnique]+pass].c_str();
    }
  }

  return NULL;
}

