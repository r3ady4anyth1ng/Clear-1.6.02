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

#include <maya/MFnStringData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MGlobal.h>
#include <maya/MDGModifier.h>
#include <maya/MImage.h>
#include <maya/MPlugArray.h>
#include <maya/MFileIO.h>
#include <maya/MMatrix.h>
#include <maya/MFnMesh.h>
#include <maya/MString.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>

#include "Platform.h"

#include "GLSLShaderNode.h"
#include "AshliShaderStrings.h"
#include "glExtensions.h"

#include "ResourceManager.h"

#include <maya/MHWShaderSwatchGenerator.h>
#include <maya/MHardwareRenderer.h>
#include <maya/MGeometryData.h>
#include <maya/MImage.h>

// If and when a bug in Maya is fixed so that you 
// can keyframe internal dynamic attributes, the
// code within this ifdef cannot be used.
// If you set the attributes to be internal, you
// can keyframe, but playback may or may not work
// erratically. This is a known API bug.
//
//#define _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
//

//
//statics
//
////////////////////////////////////////////////////////////////////////////////

// This id is supplied by Maya and should never be changed.
MTypeId glslShaderNode::sId( 0x81022);

//these are the attributes shared by all instances
MObject	glslShaderNode::sShader;
MObject	glslShaderNode::sTechnique;
MObject glslShaderNode::sTechniqueList; // Convenience attribute
MObject glslShaderNode::sShaderPath; // Convenience attribute
MObject glslShaderNode::sNakedTexCoords[8];
MObject glslShaderNode::sNakedColors[2];

//this is the default shader, used to render a checkered phong
shader *glslShaderNode::sDefShader = NULL;

//  This is a list of nodes presently created of this type.
// It allows us to find all shader nodes and update them after a file load,
// and it eases searching for use by the command.
std::vector<glslShaderNode*> glslShaderNode::sNodeList;

//default texture map strings
//const char *glslShaderNode::nakedSetNames[8] = { "map1", "", "", "", "", "", "", ""};
const char *glslShaderNode::nakedSetNames[8] = { "map1", "tangent", "binormal", "", "", "", "", ""};

//
//
//
////////////////////////////////////////////////////////////////////////////////
glslShaderNode::glslShaderNode() : m_shader(NULL), m_shaderDirty(false), m_parsedUserAttribList( false) {

	// Make sure we have our extensions initialised
	// We do it here rather than at render time so that setting a shader program
	// in batch mode will populate the shader attributes
	//
	InitializeExtensions();

  for (int ii=0; ii<8; ii++) {
    m_nakedTexSets[ii] = nakedSetNames[ii];
    m_setNums[ii] = -1; 
	m_mayaType[ii] = kNonMaya; 
  }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
glslShaderNode::~glslShaderNode() {

	if (MGlobal::mayaState() != MGlobal::kBatch)
	{
		MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
		if (pRenderer)
		{
			const MString & backEndStr = pRenderer->backEndString();
			MStatus status = pRenderer->makeResourceContextCurrent( backEndStr );
			if (status != MStatus::kSuccess) {
				MStatus strStat;
				MGlobal::displayError(MStringResource::getString(rGLSLShaderNodeFailedResourceContext, strStat));
				MGlobal::displayError(status.errorString());
			}
		}
	}

  //clean up any textures we have created
  for (std::vector<samplerAttrib>::iterator it=m_samplerAttribList.begin(); it<m_samplerAttribList.end(); it++) {
    ResourceManager::destroyTexture( it->texName);
  }

  //keep the list of allocated shaders correct
  { //this fixes MSVC and its busted scoping rules

    std::vector<glslShaderNode*>::iterator it = std::find( sNodeList.begin(), sNodeList.end(), this);
    if (it != sNodeList.end()) {
      //remove this one from the list
      sNodeList.erase(it);
    }
    else {
      //this is really bad, somehow one got created without going through create
    }
  }

  delete m_shader;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::postConstructor() {
}

//
//compute when DAG change occurs (likely nothing for HwShader)
//
////////////////////////////////////////////////////////////////////////////////
MStatus glslShaderNode::compute( const MPlug& plug, MDataBlock& data) {

  //need to look to compute a default color
  
  if ((plug == outColor) || (plug.parent() == outColor))
  {
    MFloatVector color(.95f, .1f, .07f);
    MDataHandle outputHandle = data.outputValue( outColor );
    outputHandle.asFloatVector() = color;
    outputHandle.setClean();
    return MS::kSuccess;
  }

  return MS::kUnknownParameter;
}

//
//factory for node creation
//
////////////////////////////////////////////////////////////////////////////////
void* glslShaderNode::creator() {
  glslShaderNode *ret = new glslShaderNode;

  //add this new node to the list
  sNodeList.push_back(ret);

  return ret;
}

//
// initialization (static)
//
////////////////////////////////////////////////////////////////////////////////
MStatus glslShaderNode::initialize() {

  //add the necessary nodes
  MFnTypedAttribute typedAttr;
  MFnStringData stringData;
  MStatus stat;
  MObject string;

  //  The attribute names and short names below are selected to be compatible with
  // the CgFX plugin for simplicity


  //
  // Create the shader attribute and add it to the node type
  //

  // The shader attribute holds the file name of an fx file
  string = stringData.create(&stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  sShader = typedAttr.create("shader", "s", MFnData::kString, string, &stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  stat = typedAttr.setInternal(true);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  // Add the attribute to the node
  if ( addAttribute(sShader) != MS::kSuccess)
    return MS::kFailure;

  //
  // Create the technique 
  //
  
  // The technique attribute holds the currently selected technique name
  string = stringData.create(&stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  sTechnique = typedAttr.create("technique", "t", MFnData::kString, string, &stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  stat = typedAttr.setInternal(true);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  // Add the attribute to the node
  if ( addAttribute(sTechnique) != MS::kSuccess)
    return MS::kFailure;


  // Create a techniqueList to allow access by the AE for UI purposes
  sTechniqueList = typedAttr.create("techniqueList", "tl", MFnData::kString, string, &stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  stat = typedAttr.setInternal(true);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  typedAttr.setWritable( false );
  typedAttr.setStorable( false );
  typedAttr.setHidden( true );
  typedAttr.setConnectable( false );

  // Add the attribute to the node
  if ( addAttribute(sTechniqueList) != MS::kSuccess)
    return MS::kFailure;

  // Create a shader path attrib to allow access by the AE for UI purposes
  sShaderPath = typedAttr.create("shaderPath", "sp", MFnData::kString, string, &stat);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  stat = typedAttr.setInternal(true);
  if (stat != MS::kSuccess)
    return MS::kFailure;

  typedAttr.setWritable( false );
  typedAttr.setStorable( false );
  typedAttr.setHidden( true );
  typedAttr.setConnectable( false );  

  // Add the attribute to the node
  if ( addAttribute(sShaderPath) != MS::kSuccess)
    return MS::kFailure;

  int ii;

  //
  // Add the naked texture coord sets to the node
  //
  for (ii=0; ii<8; ii++) {
    string = stringData.create( MString(nakedSetNames[ii]));
    sNakedTexCoords[ii] = typedAttr.create( MString("TexCoord") + ii, MString("t")+ ii, MFnData::kString, string);
    typedAttr.setInternal( true);
    typedAttr.setKeyable( true );
    addAttribute( sNakedTexCoords[ii]);
  }

  // Add the naked color sets to the node
  for (ii=0; ii<2; ii++) {
    sNakedColors[ii] = typedAttr.create( MString("Color") + ii, MString("c")+ ii, MFnData::kString);
    typedAttr.setInternal( true);
    typedAttr.setKeyable( true );
    addAttribute( sNakedColors[ii]);
  }

  return MS::kSuccess;
}

//
// callback to configure shaders, post file load
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::rejigShaders( void *data) {

  //iterate over all shaders, and rebuild them, so they can reconnect attributes
  for (std::vector<glslShaderNode*>::iterator it = sNodeList.begin(); it < sNodeList.end(); it++) {
    (*it)->rebuildShader();
  }
}

//
// Update shader and attributes
//
////////////////////////////////////////////////////////////////////////////////
static bool textureInitPowerOfTwo(unsigned int val, unsigned int & retval)
{
	unsigned int res = 0;				// May be we should return 1 when val == 0
	if (val)
	{
		// Muliply all values by 2, to simplify the testing:
		// 3*(res/2) < val*2 <= 3*res
		val <<= 1;
		unsigned int low = 3;
		res = 1;
		while (val > low)
		{
			low <<= 1;
			res <<= 1;
		}
	}

	retval = res;
    return (res == (val>>1)) ? 1 : 0;
}

bool glslShaderNode::rebuildShader()
{
  //handle shader initialization
  
	bool rebuiltAnything = false;

	GL_CHECK;

	// Re (build) current shader
	if (m_shaderDirty) {

		MGlobal::displayInfo(MString("Rebuild shader : ") + m_shaderPath.asChar() );

		// Clean up old shader (delete NULL is safe, does not need protected)
		delete m_shader;
        GL_CHECK;

		// Create a new shader
        m_shader = shader::create(m_shaderPath.asChar());
        GL_CHECK;

		if (!m_shader) {
			MStatus strStat;
			MGlobal::displayError(MStringResource::getString(rGLSLShaderNodeFailedLoadShader, strStat));
            MGlobal::displayError(shader::sError.c_str());
		}
		else {
			rebuiltAnything = true;


			// Try to reuse the existing technique name.
			// This will allow for being able to re-read
			// files which have saved a technique name which
			// is not technique 0.
			bool foundExistingTechnique = false;
			if (m_techniqueName.length())
			{
				for (int ii=0; ii<m_shader->techniqueCount(); ii++) {
					if (m_techniqueName  == m_shader->techniqueName(ii)) {
						m_shader->setTechnique(ii);
						foundExistingTechnique = true;
						break;
					}
				}
			}
			// Just use technique 0 as a default, since we don't 
			// know which one to use.
			if (!foundExistingTechnique)
			{
				m_techniqueName = m_shader->techniqueName(0);
			}

			m_techniqueList = m_techniqueName ;
			for (int ii=1; ii<m_shader->techniqueCount(); ii++)
				m_techniqueList = m_techniqueList + " " + m_shader->techniqueName(ii);

			// Reconfigure node attributes
			configureAttribs();
            GL_CHECK;
		}
		m_shaderDirty = false;
	}

    m_techniqueList = "";
	if (m_shader && m_shader->techniqueCount())
	{
		for (int ii=0; ii<m_shader->techniqueCount(); ii++)
			m_techniqueList = m_techniqueList + " " + m_shader->techniqueName(ii);
	}

	return rebuiltAnything;
}

//
// search the dependency graph for a file texture node
//
////////////////////////////////////////////////////////////////////////////////
MObject glslShaderNode::findFileTextureNode( const MPlug &plug) {
  MPlugArray arr;
  MStatus stat;
  plug.connectedTo(arr, true, false, &stat);
  MObject srcNode;

  if (stat != MStatus::kSuccess)
    return MObject::kNullObj;

  if (arr.length() == 1)
    srcNode = arr[0].node();
  else if (arr.length() == 0 ) {
    //plug is not connected
    return MObject::kNullObj;
  }
  else {
    //plug is multiply connected
    MGlobal::displayWarning("Texture plug is connected to multiple nodes");
    return MObject::kNullObj;
  }
  
  if( srcNode != MObject::kNullObj) {
    MStatus rc;
    MFnDependencyNode dgFn( srcNode, &rc);
    MPlug fnPlug = dgFn.findPlug( "fileTextureName", &rc);
    if (rc == MStatus::kSuccess) {
      MStatus tStat;
      MObject fTex = fnPlug.node(&tStat);
      if ( tStat != MStatus::kSuccess)
        return MObject::kNullObj;
      else
        return fTex;
    }
  }
  return MObject::kNullObj;
}

//
// search the dependency graph for an envCube node
//
////////////////////////////////////////////////////////////////////////////////
MObject glslShaderNode::findEnvCubeNode( const MPlug &plug) {
  MPlugArray arr;
  plug.connectedTo(arr, true, false);
  MObject srcNode;

  if (arr.length() == 1)
    srcNode = arr[0].node();
  else if (arr.length() == 0 ) {
    //plug is not connected
    return MObject::kNullObj;
  }
  else {
    //plug is multiply connected
    MGlobal::displayWarning("Texture plug is connected to multiple nodes");
    return MObject::kNullObj;
  }
  
  // if we have a real node, look for the envCube
  if( srcNode != MObject::kNullObj) {
    MStatus rc;
    MFnDependencyNode dgFn( srcNode, &rc);

    // searching for the plug seems a pretty good solution
    MPlug fnPlug = dgFn.findPlug( "left", &rc);

    if (rc == MStatus::kSuccess) {
      MStatus tStat;
      MObject fCube = fnPlug.node(&tStat);
      if ( tStat != MStatus::kSuccess)
        return MObject::kNullObj;
      else
        return fCube;
    }
  }
  return MObject::kNullObj;
}

//
// load a cubemap face from a cubeEnv node
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::loadCubeFace( glslShaderNode::CubeFace cf, MObject &cube, MFnDependencyNode &dn){
  const char* faceNames[] = { "left", "right", "top", "bottom", "front", "back"};
  const GLenum targets[] = {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
  float rgb[3];

  MObject face = dn.attribute( faceNames[cf]);
  MPlug plug( cube, face);
  MObject fTex = findFileTextureNode( plug);

  if (fTex != MObject::kNullObj) {
	  MImage img;
	  unsigned int width, height;

	  img.readFromTextureNode(fTex);
	  MStatus status = img.getSize( width, height);

	  // If not power of two and NPOT is not supported, then we need 
	  // to resize the original system pixmap before binding.
	  //
	  // Also handle non-square cube map textures
	  bool resizeToBeSquare = false;
	  if (width != height)
	  {
		  width = height;
		  resizeToBeSquare = true;
	  }
	  if (width > 0 && height > 0 && (status != MStatus::kFailure) )
	  {
		  // If not power of two and NPOT is not supported, then we need 
		  // to resize the original system pixmap before binding.
		  //
		  if (!glTextureNonPowerOfTwo || resizeToBeSquare )
		  {
			  if (width > 2 && height > 2)
			  {
				  unsigned int p2Width, p2Height;
				  bool widthPowerOfTwo  = textureInitPowerOfTwo(width,  p2Width);
				  bool heightPowerOfTwo = textureInitPowerOfTwo(height, p2Height);
				  if(!widthPowerOfTwo || !heightPowerOfTwo || resizeToBeSquare)
				  {
					  width = p2Width;
					  height = p2Height;
					  img.resize( p2Width, p2Height, false /* preserverAspectRatio */);
				  }
			  }
		  }

		  //MGlobal::displayInfo( MString("Loading ") + faceNames[cf]+ " face (" + width + "," + height + ")");
		  glTexImage2D( targets[cf], 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels());
		  GL_CHECK;
	  }
  }
  else {
    MObject data;
    plug.getValue( data);
    MFnNumericData val(data);
    val.getData( rgb[0], rgb[1], rgb[2]);
    glTexImage2D( targets[cf], 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_FLOAT, rgb);
  }
}

//
//  Bind data such as texture objects for samplers.
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::BindSamplerData() {

  //iterate over all samplers
  for ( std::vector<samplerAttrib>::iterator it = m_samplerAttribList.begin(); it < m_samplerAttribList.end(); it++) {

    //only update if it has been marked dirty
    if (it->dirty) {
      it->dirty = false;

      //get the plug and associated info
      MPlug plug(thisMObject(), it->attrib);
      MObject data;
      plug.getValue(data);
      MFnNumericData val(data);
      float rgb[3];

      if (it->texName == 0) {
        //need to gen a new name, one has not been created
        glGenTextures( 1, &(it->texName));
        m_shader->updateSampler( it->pNum, it->texName);
        GL_CHECK;
      }

      switch (m_shader->samplerType(it->pNum) ) {

        case shader::st1D:
          //just load a 1D texture from the plug value until we handle files directly
          val.getData( rgb[0], rgb[1], rgb[2]);
          glBindTexture( GL_TEXTURE_1D, it->texName);
          glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, it->minFilter);
          glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, it->magFilter);
          glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, it->wrapMode);
          glTexParameterf( GL_TEXTURE_1D, GL_TEXTURE_MAX_ANISOTROPY_EXT, it->maxAniso);
          if ( (it->minFilter == GL_LINEAR_MIPMAP_NEAREST) || (it->minFilter == GL_LINEAR_MIPMAP_LINEAR)) {
            glTexParameteri( GL_TEXTURE_1D, GL_GENERATE_MIPMAP, GL_TRUE);
          }
          else {
            glTexParameteri( GL_TEXTURE_1D, GL_GENERATE_MIPMAP, GL_FALSE);
          }
          glTexImage1D( GL_TEXTURE_1D, 0, GL_RGB8, 1, 0, GL_RGB, GL_FLOAT, rgb);
          GL_CHECK;
          break;

        case shader::st1DShadow:
          //are shadow maps accessible in Maya?
          break;

        case shader::st2D:
          {
            bool done = false;
            //set up the texture stuff
            glBindTexture( GL_TEXTURE_2D, it->texName);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, it->minFilter);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, it->magFilter);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, it->wrapMode);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, it->wrapMode);
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, it->maxAniso);
            if ( (it->minFilter == GL_LINEAR_MIPMAP_NEAREST) || (it->minFilter == GL_LINEAR_MIPMAP_LINEAR)) {
              glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            }
            else {
              glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
            }
            MObject fTex = findFileTextureNode( plug);

			if (fTex != MObject::kNullObj) {
				MImage img;
				unsigned int width, height;

				img.readFromTextureNode(fTex);
				MStatus status = img.getSize( width, height);
			    if (width > 0 && height > 0 && (status != MStatus::kFailure) )
				{
					// If not power of two and NPOT is not supported, then we need 
					// to resize the original system pixmap before binding.
					//
					if (!glTextureNonPowerOfTwo)
					{
						if (width > 2 && height > 2)
						{
							unsigned int p2Width, p2Height;
							bool widthPowerOfTwo  = textureInitPowerOfTwo(width,  p2Width);
							bool heightPowerOfTwo = textureInitPowerOfTwo(height, p2Height);
							if(!widthPowerOfTwo || !heightPowerOfTwo)
							{
								width = p2Width;
								height = p2Height;
								img.resize( p2Width, p2Height, false /* preserverAspectRatio */);
							}
						}
					}
					glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels());
					GL_CHECK;
					done = true;
				}
			}
            
            if (!done) {
              //just read it from the plug
              val.getData( rgb[0], rgb[1], rgb[2]);
              glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_FLOAT, rgb);
            }
            GL_CHECK;
          }
          break;

        case shader::stCube:
          {
            //search for envCube node, then traverse off the 6 faces
            glBindTexture( GL_TEXTURE_CUBE_MAP, it->texName);
            glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, it->minFilter);
            glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, it->magFilter);
            glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, it->wrapMode);
            glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, it->wrapMode);
            glTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, it->maxAniso);
            if ( (it->minFilter == GL_LINEAR_MIPMAP_NEAREST) || (it->minFilter == GL_LINEAR_MIPMAP_LINEAR)) {
              glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);
            }
            else {
              glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_FALSE);
            }
            MObject cube = findEnvCubeNode( plug);
            if ( cube != MObject::kNullObj) {
              MFnDependencyNode dn(cube);
              loadCubeFace( cfLeft, cube, dn);
              loadCubeFace( cfRight, cube, dn);
              loadCubeFace( cfTop, cube, dn);
              loadCubeFace( cfBottom, cube, dn);
              loadCubeFace( cfFront, cube, dn);
              loadCubeFace( cfBack, cube, dn);
            }
          }
          break;

        case shader::st2DShadow:
        case shader::st3D:
          //need to handle file loading directly for 3D textures
          break;
		default:
		  break;
      };
    }
  }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::BindUniformData() {

  for ( std::vector<uniformAttrib>::iterator it = m_uniformAttribList.begin(); it < m_uniformAttribList.end(); it++) {

	// [ANIM_UNIFORM_FIX2]
	// To allow animation to work, uniform data has been
	// left to be dirty all of the time, as internal attributes
    // do not update properly during playback.
	//
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
    if (it->dirty) {
#endif
      it->dirty = false;

      MPlug plug(thisMObject(), it->attrib);
      MObject data;
      plug.getValue(data);
      MFnNumericData val(data);
      float fTemp[4];
      int iTemp[4];
      bool bTemp[4];

      switch (m_shader->uniformType(it->pNum)) {

        case shader::dtBool:
          plug.getValue(bTemp[0]);
          m_shader->updateUniformBool( it->pNum, bTemp[0]);
          break;

        case shader::dtBVec2:
        case shader::dtBVec3:
        case shader::dtBVec4:
          //no Maya native type, these are unsupported for now
          break;

        case shader::dtInt:
          plug.getValue(iTemp[0]);
          m_shader->updateUniformInt( it->pNum, iTemp[0]);
          break;

        case shader::dtIVec2:
          val.getData( iTemp[0], iTemp[1]);
          m_shader->updateUniformIVec( it->pNum, iTemp);
          break;

        case shader::dtIVec3:
          val.getData( iTemp[0], iTemp[1], iTemp[2]);
          m_shader->updateUniformIVec( it->pNum, iTemp);
          break;

        case shader::dtIVec4:
          //no Maya native type, these are unsupported for now
          break;

        case shader::dtFloat:
          plug.getValue(fTemp[0]);
          m_shader->updateUniformFloat( it->pNum, fTemp[0]);
          break;

        case shader::dtVec2:
          val.getData( fTemp[0], fTemp[1]);
          m_shader->updateUniformVec( it->pNum, fTemp);
          break;

        case shader::dtVec3:
          val.getData( fTemp[0], fTemp[1], fTemp[2]);
          m_shader->updateUniformVec( it->pNum, fTemp);
          break;

        case shader::dtVec4:
          {
            MPlug plug2(thisMObject(), it->attrib2);
            val.getData( fTemp[0], fTemp[1], fTemp[2]);
            plug2.getValue( fTemp[3]);
            m_shader->updateUniformVec( it->pNum, fTemp);
          }
          break;

        case shader::dtMat4:
          {
            MFnMatrixData mValue(data);
            MMatrix matrix = mValue.matrix();
            m_shader->updateUniformMat( it->pNum, (double*)matrix.matrix);
          }
          break;

        case shader::dtMat2:
        case shader::dtMat3:
        
          //unsupported for now, due to Maya's interface presently lacking support
          break;
		default:
		  break;
      };
    }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
  }
#endif
}


// Method to override to let Maya know this shader is batchable. 
//
bool glslShaderNode::supportsBatching() const
{
	return true;
}


//
// Old style drawing commands like the HW renderer needs
//
////////////////////////////////////////////////////////////////////////////////
MStatus glslShaderNode::glBind(const MDagPath& shapePath) {

    // Rebuild shader, if dirty
	// This is only required pre 7.0, and should not be part of
	// the plugin anymore unless we decide to ship with 6.5.
	//
  
  // Initialize GL extesions
  if (!InitializeExtensions()) {
	MStatus strStat;
    MGlobal::displayError(MStringResource::getString(rGLSLShaderNodeAshliFailedFindExtensions, strStat));
    return MStatus::kFailure;
  }
    GL_CHECK;

    //attempt to clean up any delayed deletions
    ResourceManager::recover();

	// Build default shader
	if (!sDefShader)
		sDefShader = new defaultShader;
    GL_CHECK;

  //
  //bind any dirty data

  //should add an extra global dirty flag
  if (m_shader) {
    if (!m_shader->build())
      MGlobal::displayError(MString(m_shader->errorString()));

    BindUniformData();

    //now the samplers
    BindSamplerData();
  }

  // Disable these to avoid possible problems with shader programs
  // going into software
  glDisable(GL_POINT_SMOOTH);
  glDisable( GL_LINE_SMOOTH);

  //check for active shader, if none, fall back to the default shader
  if ((m_shader != NULL) && (m_shader->valid())) 
  {
    m_passCount = m_shader->passCount();
    
    if (!m_shader->build()) {
      m_passCount = 0;
    }

	if( m_passCount == 1)
	{
		m_shader->setPass( 0);
		m_shader->bind();
	}
  }
  else
  {
    m_passCount = -1;
	m_maxSetNum = 1; // We need the default UV
	sDefShader->bind();
  }
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);

  GL_CHECK;

  return MS::kSuccess;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
MStatus glslShaderNode::glUnbind(const MDagPath& shapePath) {

	for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) 
		glDisableVertexAttribArray( it->handle);

	for( int ii = 0; ii < m_maxSetNum; ii++) 
		m_glState.disableTexCoord( ii);
	m_glState.activeTexture( 0);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_VERTEX_PROGRAM_TWO_SIDE);

	// If we're not using multi-pass, disable the effect here
	if( m_passCount < 0)
		sDefShader->unbind();
	else if( m_passCount == 1)
		m_shader->unbind();

  // Dirty our attribute list so it gets re-built next time
  // this shader is used
  m_parsedUserAttribList = false;

  return MS::kSuccess;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::updateBoundAttributes( const MDagPath& shapePath) {
  //
  // This code needs to get the current matrices
  //  the world matrix is easy (dagPath)
  //  The view matrix seems like it can come from the current M3DView
  //  I can't see any good way to get the projection matrix other than reading it
  //
  MMatrix world;
  MMatrix temp;
  MMatrix proj;
  MMatrix view;
  MMatrix worldview;

  //get the matrices
  if (shapePath.isValid())
	  world = shapePath.inclusiveMatrix();
  else
	  world.setToIdentity();
  glGetDoublev( GL_PROJECTION_MATRIX, (double*)proj.matrix);
  glGetDoublev( GL_MODELVIEW_MATRIX, (double*)worldview.matrix);
  view = world.inverse() * worldview;

  for (std::vector<boundUniform>::iterator it = m_boundUniformList.begin(); it < m_boundUniformList.end(); it++) {
    switch ( it->usage) {

      case shader::smWorld:
        m_shader->updateUniformMat( it->pNum, (double*)world.matrix);
        break;

      case shader::smView:
        m_shader->updateUniformMat( it->pNum, (double*)view.matrix);
        break;

      case shader::smProjection:
        m_shader->updateUniformMat( it->pNum, (double*)proj.matrix);
        break;

      case shader::smWorldView:
        m_shader->updateUniformMat( it->pNum, (double*)worldview.matrix);
        break;

      case shader::smViewProjection:
        temp = view * proj;
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewProjection:
        temp = worldview * proj;
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldI:
        temp = world.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewI:
        temp = view.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smProjectionI:
        temp = proj.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewI:
        temp = worldview.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewProjectionI:
        temp = view * proj;
        temp = temp.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewProjectionI:
        temp = worldview * proj;
        temp = temp.inverse();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldT:
        temp = world.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewT:
        temp = view.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smProjectionT:
        temp = proj.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewT:
        temp = worldview.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewProjectionT:
        temp = view * proj;
        temp = temp.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewProjectionT:
        temp = worldview * proj;
        temp = temp.transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldIT:
        temp = world.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewIT:
        temp = view.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smProjectionIT:
        temp = proj.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewIT:
        temp = worldview.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewProjectionIT:
        temp = view * proj;
        temp = temp.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smWorldViewProjectionIT:
        temp = worldview * proj;
        temp = temp.inverse().transpose();
        m_shader->updateUniformMat( it->pNum, (double*)temp.matrix);
        break;

      case shader::smViewPosition:
        {
          float vec[4];
          temp = view.inverse();

          vec[0] = (float)temp[3][0];
          vec[1] = (float)temp[3][1];
          vec[2] = (float)temp[3][2];
          vec[3] = (float)temp[3][3];

          m_shader->updateUniformVec( it->pNum, vec);
        }
        break;
      
      case shader::smTime:
        // need to find a good way to get the current animation time
        //  the cgfx plugin uses:
        //    (float)timeGetTime() * 1000.0f
        // This has two problems
        //  1. one it does not correspond to Maya time
        //  2. it has portability problems
        m_shader->updateUniformFloat( it->pNum, 0.0f);
        break;
      
      case shader::smViewportSize:
        {
          float vp[4];
          glGetFloatv( GL_VIEWPORT, vp);
          m_shader->updateUniformVec( it->pNum, &vp[2]);
        }
        break;

	  default:
		break;
    };
  }
}

//
//  Configure the texture coordinate sets and color sets for rendering. This
// function sets up all the vertex arry pointers or current values, if a set
// is not available.
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::configureTexCoords( int normalCount, const float ** normalArrays, 
										int texCoordCount,
                                        const float ** texCoordArrays,
										int colorCount,
										const float ** colorArrays) 
{

  //printf("*********** configureTexCoords [normal count = %d]\n", normalCount);
											
  int ii;

  //bind in the naked texture sets
  for (ii=0; ii<m_maxSetNum; ii++) {

    //handle the special numbers
    if (m_setNums[ii] == -1) {
      // no name is specified, send a default texture coord of (0,0)
      glMultiTexCoord2f( GL_TEXTURE0 + ii, 0.0f, 0.0f);
      m_glState.disableTexCoord( ii);
      continue;
    }
    else if (m_mayaType[ii] == kNormal)  {

      //this set has requested a normal
      if ( (normalCount > 0) && normalArrays[0]) {
        //there is a tangent
        m_glState.enableAndActivateTexCoord( ii);
        glTexCoordPointer( 3, GL_FLOAT, 0, normalArrays[0]);
      }
      else {
        //there is no tangent available
        glMultiTexCoord3f( GL_TEXTURE0 + ii, 0.0f, 0.0f, 1.0f);
        m_glState.disableTexCoord( ii);
      }
      continue;
    }
    else if (m_mayaType[ii] == kTangent)  {

      //this set has requested a tangent
      if ( m_numNormals > 1  && normalCount > 1)
	  {
		  int offset = 0;
		  if (m_numNormals > 2)
			offset = ( m_setNums[ii] * 3 ) + 1;
		  else
			offset = ( m_setNums[ii] * 2 ) + 1;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 2)
				  offset = 1;
			  else
				  offset = -1;
		  }

		  //printf("TANGENT%d : Access tangents via collection id(%d) = offset %d\n",
		//		ii, m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset]) {
			  //there is a tangent
			  m_glState.enableAndActivateTexCoord( ii);
			  glTexCoordPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
		  }
		  else {
			  //there is no tangent available
			  glMultiTexCoord3f( GL_TEXTURE0 + ii, 1.0f, 0.0f, 0.0f);
			  m_glState.disableTexCoord( ii);
		  }
	  }
	  else
	  {
		  //there is no tangent available
		  glMultiTexCoord3f( GL_TEXTURE0 + ii, 1.0f, 0.0f, 0.0f);
		  m_glState.disableTexCoord( ii);
	  }
      continue;
    }
    else if (m_mayaType[ii] == kBinormal)  {

      //this set has requested a binormal
      if ( m_numNormals > 2 && normalCount > 1)
	  {
		  int offset = ( m_setNums[ii] * 3 ) + 2;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 3)
				  offset = 2;
			  else
				  offset = -1;
		  }

		  //printf("BINORMAL%d : Access binormals via collection id(%d) = offset %d\n",
			//	ii, m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset] )
		  {
			  //there is a binormal
			  m_glState.enableAndActivateTexCoord( ii);
			  glTexCoordPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
		  }
		  else
		  {
			  //there is no binormal available
			  glMultiTexCoord3f( GL_TEXTURE0 + ii, 0.0f, 1.0f, 0.0f);
			  m_glState.disableTexCoord( ii);
		  }
      }
      else 
	  {
        //there is no binormal available
        glMultiTexCoord3f( GL_TEXTURE0 + ii, 0.0f, 1.0f, 0.0f);
        m_glState.disableTexCoord( ii);
      }
      continue;
    }
    else if (m_mayaType[ii] == kColorSet) {

      //this set has requested a color
      if ( (m_setNums[ii] < colorCount) && colorArrays[m_setNums[ii]]) {
        //there is a color
        m_glState.enableAndActivateTexCoord( ii);
        glTexCoordPointer( 4, GL_FLOAT, 0, colorArrays[m_setNums[ii]]);
      }
      else {
        //there is no binormal available
        glMultiTexCoord4f( GL_TEXTURE0 + ii, 0.0f, 0.0f, 0.0f, 1.0f);
        m_glState.disableTexCoord( ii);
      }
      continue;
    }

    //now deal with regular sets
    if ( (m_setNums[ii] < texCoordCount) && texCoordArrays[m_setNums[ii]]) {
      //this set exists
      m_glState.enableAndActivateTexCoord( ii);
	  glTexCoordPointer( 2, GL_FLOAT, 0, texCoordArrays[m_setNums[ii]]);
    }
    else {
      //the set does not exist
      glMultiTexCoord2f( GL_TEXTURE0 + ii, 0.0f, 0.0f);
      m_glState.disableTexCoord( ii);
    }
  }

  //bind in the naked color sets

  //primary color first
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState( GL_COLOR_ARRAY);

  //handle the special numbers
  if (m_colorSetNums[0] == -1) {
    // no name is specified, send a default color of (1,1,1,1)
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState( GL_COLOR_ARRAY);
  }
  else if (m_colorMayaType[0] == kNormal)  {
	  
	  //this set has requested a normal
    if ( (normalCount > 0) && normalArrays[0]) {
      //there is a tangent
      glColorPointer( 3, GL_FLOAT, 0, normalArrays[0]);
      glEnableClientState( GL_COLOR_ARRAY);
    }
    else {
      //there is no normal available
      glColor4f( 0.0f, 0.0f, 1.0f, 1.0f);
      glDisableClientState( GL_COLOR_ARRAY);
    }
  }
  else if (m_colorMayaType[0] == kTangent)  {

	  //this set has requested a tangent
	  if ( m_numNormals > 1 && normalCount > 1)
	  {
		  int offset = 0;
		  if (m_numNormals > 2)
			  offset = ( m_colorSetNums[0] * 3 ) + 1;
		  else
			  offset = ( m_colorSetNums[0] * 2 ) + 1;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 2)
				  offset = 1;
			  else
				  offset = -1;
		  }

		  //printf("COLOR0: Access tangents via collection id(%d) = offset %d\n",
			//  m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset]) {
			  //there is a tangent
			  glColorPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
			  glEnableClientState( GL_COLOR_ARRAY);
		  }
		  else {
			  //there is no tangent available
			  glColor4f( 1.0f, 0.0f, 0.0f, 1.0f);
			  glDisableClientState( GL_COLOR_ARRAY);
		  }
	  }
	  else {
		  //there is no tangent available
		  glColor4f( 1.0f, 0.0f, 0.0f, 1.0f);
		  glDisableClientState( GL_COLOR_ARRAY);
	  }	  	  
  }
  else if (m_colorMayaType[0] == kBinormal)  {

      //this set has requested a binormal
	  if ( m_numNormals > 2 && normalCount > 1)
	  {
		  int offset = ( m_colorSetNums[0] * 3 ) + 2;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 3)
				  offset = 2;
			  else
				  offset = -1;
		  }

		  //printf("COLOR0: Access binormals via collection id(%d) = offset %d\n",
			//  m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset] )
		  {
			  //there is a binormal
			  glColorPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
			  glEnableClientState( GL_COLOR_ARRAY);
		  }
		  else
		  {
			  //there is no binormal available
			  glMultiTexCoord3f( GL_TEXTURE0 + ii, 0.0f, 1.0f, 0.0f);
			  m_glState.disableTexCoord( ii);
		  }
	  }
	  else 
	  {
		  //there is no binormal available
		  glColor4f( 0.0f, 1.0f, 0.0f, 1.0f);
		  glDisableClientState( GL_COLOR_ARRAY);
	  }
  }
  // Route color set through color 0
  else if (m_colorMayaType[0] == kColorSet) {
    //this set has requested a color
    if ( (m_colorSetNums[0] < colorCount) && colorArrays[m_colorSetNums[0]]) {
      //there is a color
      glColorPointer( 4, GL_FLOAT, 0, colorArrays[m_colorSetNums[0]]);
      glEnableClientState( GL_COLOR_ARRAY);
    }
    else {
      //there is no color available
      glColor4f( 0.0f, 0.0f, 0.0f, 1.0f);
      glDisableClientState( GL_COLOR_ARRAY);
    }
  }
  // Route a uv set through color 0
  else if (m_colorMayaType[0] == kUvSet) {
    //this set has requested a uvset
    if ( (m_colorSetNums[0] < texCoordCount) && texCoordArrays[m_colorSetNums[0]]) {
      // For now Maya only supports 2 compont tex coords.
      glColorPointer( 2, GL_FLOAT, 0, texCoordArrays[m_colorSetNums[0]]);
      glEnableClientState( GL_COLOR_ARRAY);
    }
    else {
      //there is no color available
      glColor4f( 1.0f, 0.0f, 0.0f, 1.0f);
      glDisableClientState( GL_COLOR_ARRAY);
    }
  }


  //secondary color
  if (glSecondaryColorSupport) {

	glSecondaryColor3f( 1.0f, 1.0f, 1.0f);
	glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
	  
	//handle the special numbers
    if (m_colorSetNums[1] == -1) {
		// no name is specified, send a default color of (1,1,1)
		glSecondaryColor3f( 1.0f, 1.0f, 1.0f);
		glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
    }
    else if (m_colorMayaType[1] == kNormal)  {
      //this set has requested a normal
      if ( (normalCount > 0) && normalArrays[0]) {
        //there is a tangent
        glSecondaryColorPointer( 3, GL_FLOAT, 0, normalArrays[0]);
        glEnableClientState( GL_SECONDARY_COLOR_ARRAY);
      }
      else {
        //there is no normal available
        glSecondaryColor3f( 0.0f, 0.0f, 1.0f);
        glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
      }
    }
    else if (m_colorMayaType[1] == kTangent)  {

	  //this set has requested a tangent
	  if ( m_numNormals > 1 && normalCount > 1)
	  {
		  int offset = 0;
		  if (m_numNormals > 2)
			  offset = ( m_colorSetNums[1] * 3 ) + 1;
		  else
			  offset = ( m_colorSetNums[1] * 2 ) + 1;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 2)
				  offset = 1;
			  else
				  offset = -1;
		  }

		  //printf("COLOR1: Access tangents via collection id(%d) = offset %d\n",
			//  m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset]) {
			  //there is a tangent
			  glSecondaryColorPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
			  glEnableClientState( GL_SECONDARY_COLOR_ARRAY);
		  }
		  else {
			  //there is no tangent available
			  glSecondaryColor3f( 1.0f, 0.0f, 0.0f);
			  glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
		  }
	  }
	  else {
		  //there is no tangent available
		  glSecondaryColor3f( 1.0f, 0.0f, 0.0f);
		  glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
	  }		
    }
    else if (m_colorMayaType[1] == kBinormal)  {

      //this set has requested a binormal
	  if ( m_numNormals > 2 && normalCount > 1 )
	  {
		  int offset = ( m_colorSetNums[1] * 3 ) + 2;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 3)
				  offset = 2;
			  else
				  offset = -1;
		  }

		  //printf("COLOR1 : Access binormals via collection id(%d) = offset %d\n",
			//  m_setNums[ii], offset );

		  if ((offset > 0) && normalArrays[offset] )
		  {
			  //there is a binormal
			  glSecondaryColorPointer( 3, GL_FLOAT, 0, normalArrays[offset]);
			  glEnableClientState( GL_SECONDARY_COLOR_ARRAY);
		  }
		  else
		  {
			  //there is no binormal available
			  glSecondaryColor3f( 0.0f, 1.0f, 0.0f);
			  glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
		  }
	  }
	  else 
	  {
		  //there is no binormal available
		  glSecondaryColor3f( 0.0f, 1.0f, 0.0f);
		  glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
	  }		
    }
	// Route a color set through color 1
    else if (m_colorMayaType[1] == kColorSet) {
      //this set has requested a color
      if ( (m_colorSetNums[1] < colorCount) && colorArrays[m_colorSetNums[1]]) {
        //there is a color
        glSecondaryColorPointer( 4, GL_FLOAT, 0, colorArrays[m_colorSetNums[1]]);
        glEnableClientState( GL_SECONDARY_COLOR_ARRAY);
      }
      else {
        //there is no color available
        glSecondaryColor3f( 1.0f, 1.0f, 1.0f);
        glDisableClientState( GL_SECONDARY_COLOR_ARRAY);
      }
    }
#if 0
	// Route a uv set through color 1
	else if (m_colorMayaType[1] == kUvSet) {
		//this set has requested a uvset
		if ( (m_colorSetNums[1] < texCoordCount) && texCoordArrays[m_colorSetNums[1]]) {
			// For now Maya only supports 2 compont tex coords.
			glSecondaryColorPointer( 2, GL_FLOAT, 0, texCoordArrays[m_colorSetNums[1]]);
			glEnableClientState( GL_COLOR_ARRAY);
		}
		else {
			//there is no color available
			glColor4f( 0.0f, 0.0f, 0.0f, 1.0f);
			glDisableClientState( GL_COLOR_ARRAY);
		}
	}
#endif
  }


  //handle the generic attribute sets
  for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) {
    //I need to find a better place for this then here
    it->handle = m_shader->attributeHandle(it->pNum);
    if (it->set == -1) {
      glDisableVertexAttribArray( it->handle);
      glVertexAttrib2f( it->handle, 0.0f, 0.0f);
      continue;
    }
    else if (it->mtype == kNormal) {
      //this set has requested a tangent
      if (( normalCount > 0) && normalArrays[0]) {
        //there is a tangent
        glVertexAttribPointer( it->handle, 3, GL_FLOAT, GL_FALSE, 0, normalArrays[0]);
        glEnableVertexAttribArray( it->handle);
      }
      else {
        //there is no tangent available
        glVertexAttrib2f( it->handle, 0.0f, 0.0f);
        glDisableVertexAttribArray( it->handle);
      }
      continue;
    }
    else if (it->mtype == kTangent) {

	  //this set has requested a tangent
	  if ( m_numNormals > 1 && normalCount > 1)
	  {
		  int offset = 0;
		  if (m_numNormals > 2)
			  offset = ( it->set * 3 ) + 1;
		  else
			  offset = ( it->set * 2 ) + 1;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 2)
				  offset = 1;
			  else
				  offset = -1;
		  }

		  //printf("VTX_ATTRIB: Access tangents via collection id(%d) = offset %d\n",
			//  it->set, offset );

		  if ((offset > 0) && normalArrays[offset]) {
			  //there is a tangent
			  glVertexAttribPointer( it->handle, 3, GL_FLOAT, GL_FALSE, 0, normalArrays[offset]);
			  glEnableVertexAttribArray( it->handle);
		  }
		  else {
			  //there is no tangent available
			  glVertexAttrib2f( it->handle, 0.0f, 0.0f);
			  glDisableVertexAttribArray( it->handle);
		  }
	  }
	  else {
		  //there is no tangent available
		  glVertexAttrib2f( it->handle, 0.0f, 0.0f);
		  glDisableVertexAttribArray( it->handle);
	  }				
      continue;
    }
    else if (it->mtype == kBinormal) {

      //this set has requested a binormal
	  if ( m_numNormals > 2 && normalCount > 1)
	  {
		  int offset = ( it->set * 3 ) + 2;

		  // Can occur with swatch rendering.
		  if (offset > normalCount)
		  {
			  if (normalCount >= 3)
				  offset = 2;
			  else
				  offset = -1;
		  }

		  //printf("VTX_ATTRIB : Access binormals via collection id(%d) = offset %d\n",
			//  it->set, offset );

		  if ((offset > 0) && normalArrays[offset] )
		  {
			  //there is a binormal
			  glVertexAttribPointer( it->handle, 3, GL_FLOAT, GL_FALSE, 0, normalArrays[offset]);
			  glEnableVertexAttribArray( it->handle);
		  }
		  else
		  {
			  //there is no binormal available
			  glVertexAttrib2f( it->handle, 0.0f, 0.0f);
			  glDisableVertexAttribArray( it->handle);
		  }
	  }
	  else
	  {
        //there is no binormal available
        glVertexAttrib2f( it->handle, 0.0f, 0.0f);
        glDisableVertexAttribArray( it->handle);
	  }
      continue;
    }
    else if (it->mtype == kColorSet) {
      //this set has requested a color
      if (( it->set < colorCount ) && colorArrays[it->set]) {
        //there is a color
        glVertexAttribPointer( it->handle, 4, GL_FLOAT, GL_FALSE, 0, colorArrays[it->set]);
        glEnableClientState( it->handle );
      }
      else {
        //there is no color available
        glVertexAttrib4f( it->handle, 0.0f, 0.0f, 0.0f, 1.0f);
        glDisableVertexAttribArray( it->handle);
      }
      continue;
    }


    //now deal with regular sets
    if (( it->set < texCoordCount ) && texCoordArrays[it->set]) {
      //this set exists
      glVertexAttribPointer( it->handle, 2, GL_FLOAT, GL_FALSE, 0, texCoordArrays[it->set]);
      glEnableVertexAttribArray( it->handle);
    }
    else {
      //the set does not exist
      glVertexAttrib2f( it->handle, 0.0f, 0.0f);
      glDisableVertexAttribArray( it->handle);
    }
  }

  m_glState.activeTexture( 0);
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
MStatus	glslShaderNode::glGeometry( const MDagPath& shapePath, int prim, unsigned int writable, int indexCount,
		const unsigned int * indexArray, int vertexCount, const int * vertexIDs, const float * vertexArray,
		int normalCount, const float ** normalArrays, int colorCount, const float ** colorArrays, int texCoordCount,
        const float ** texCoordArrays) {

  if (!(glShadingLanguageSupport && glMultiTextureSupport && glTexture3DSupport && glAsmProgramSupport))
    return MStatus::kFailure;

  // If we're not using the default shader, setup any shape specific 
  // shader parameters
  if( m_passCount > 0)
    updateBoundAttributes(shapePath);

  // this is inefficient for the moment
  int passCount = m_passCount >= 0 ? m_passCount : 1;
  for( int jj=0; jj < passCount; jj++) 
  {
	// If this effect has more than 1 pass, bind the
	// shader per-pass. If it's a single pass effect,
	// the pass will have been bound in glBind()
    if( passCount > 1) 
	{
		m_shader->setPass( jj);
		m_shader->bind();
	}

	// Setup any shape dependent uniforms (e.g. matrices)
	if( m_passCount > 0)
	{
		m_shader->setShapeDependentState();
    }
	else
	{
		sDefShader->setShapeDependentState();
	}

    //all of the following should come from shader attribute querying
    // might want to add VBO support for multiple pass shaders
    glVertexPointer( 3, GL_FLOAT, 0, vertexArray);
    
    if (normalCount && normalArrays[0] ) {
      glNormalPointer( GL_FLOAT, 0, &normalArrays[0][0]);
      glEnableClientState(GL_NORMAL_ARRAY);
    }
	else
	{
      glDisableClientState(GL_NORMAL_ARRAY);
	}
  
    configureTexCoords( normalCount, normalArrays, texCoordCount, texCoordArrays,
					    colorCount, colorArrays );
    

    GL_CHECK;

    //should probably switch to draw range elements
    glDrawElements( prim, indexCount, GL_UNSIGNED_INT, indexArray);

	// If this effect has more than 1 pass, unbind the
	// shader per-pass (if it's a single pass effect,
	// the effect will be bound once in glBind()
    if( m_passCount > 1) 
	{
      m_shader->unbind();
	}

    GL_CHECK;
  }// for num passes
	
  if( m_passCount > 1) 
	m_shader->setPass(0);

  return MS::kSuccess;
}

//
// If the the attribute is a shader attribute
// mark it dirty, and pass it to the parent
////////////////////////////////////////////////////////////////////////////////
MStatus			
glslShaderNode::connectionMade( const MPlug& plug,
							   const MPlug& otherPlug,
							   bool asSrc )
{

	//need to grab the root in case we are dealing with a vector 
	const MPlug &root = plug.isChild() ? plug.parent() : plug; 

	//MGlobal::displayInfo(MString("Connection made: ") + root.name());

	//
	//  Is this time critical enough to use something like a map, instead or linearly searching all
	// attribute? Present expectation is for shaders to have significantly fewer than 100 attributes.
	{
		for ( std::vector<uniformAttrib>::iterator it = m_uniformAttribList.begin(); it < m_uniformAttribList.end(); it++) {
			if ( root == it->attrib) {
				it->dirty = true;
				//allow the set to proceed using the default method
				break;
			}
		}
	}

	{
		for ( std::vector<samplerAttrib>::iterator it = m_samplerAttribList.begin(); it < m_samplerAttribList.end(); it++) {
			if ( root == it->attrib) {
				it->dirty = true;
				//allow the set to proceed using the default method
				break;
			}
		}
	}

	// Pass to parent class. Must do this to ensure keyframe
	// animation works !
	return MS::kUnknownParameter;
}

//
// If the the attribute is a shader attribute
// mark it dirty, and pass it to the parent
////////////////////////////////////////////////////////////////////////////////
MStatus		
glslShaderNode::connectionBroken( const MPlug& plug,
								 const MPlug& otherPlug,
								 bool asSrc )
{
	//need to grab the root in case we are dealing with a vector 
	const MPlug &root = plug.isChild() ? plug.parent() : plug; 

	//MGlobal::displayInfo(MString("Connection broken: ") + root.name());

	//
	//  Is this time critical enough to use something like a map, instead or linearly searching all
	// attribute? Present expectation is for shaders to have significantly fewer than 100 attributes.
	{
		for ( std::vector<uniformAttrib>::iterator it = m_uniformAttribList.begin(); it < m_uniformAttribList.end(); it++) {
			if ( root == it->attrib) {
				it->dirty = true;
				//allow the set to proceed using the default method
				break;
			}
		}
	}

	{
		for ( std::vector<samplerAttrib>::iterator it = m_samplerAttribList.begin(); it < m_samplerAttribList.end(); it++) {
			if ( root == it->attrib) {
				it->dirty = true;
				//allow the set to proceed using the default method
				break;
			}
		}
	}

	// Pass to parent class. Must do this to ensure keyframe
	// animation works !
	return MS::kUnknownParameter;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
/* virtual */
bool glslShaderNode::getInternalValueInContext( const MPlug &plug, 
											   MDataHandle &handle,
											   MDGContext& )
{
  bool retVal = false;

  if ( plug == sShader) {
    handle.set(m_shaderName);
    retVal = true;
  }
  else if ( plug == sTechnique) {
    handle.set(m_techniqueName);
    retVal = true;
  }
  else if ( plug == sTechniqueList )
  {
	handle.set(m_techniqueList);
	retVal = true;
  }
  else if ( plug == sShaderPath )
  {
	handle.set(m_shaderPath);
	retVal = true;
  }  
  else {
    int ii;
    for (ii=0; ii<8; ii++) {
      if ( plug == sNakedTexCoords[ii]) {
        handle.set(m_nakedTexSets[ii]);
        retVal = true;
        break;
      }
    }

    for (ii=0; ii<2; ii++) {
      if (plug == sNakedColors[ii]) {
        handle.set(m_nakedColorSets[ii]);
        retVal = true;
        break;
      }
    }

    //handle the generic attribute sets
    for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) {
      if (plug == it->attrib) {
        handle.set(it->setName);
        retVal = true;
        break;
      }
    }
  }

  return retVal;
}



//
//
//
////////////////////////////////////////////////////////////////////////////////
/* virtual */
bool glslShaderNode::setInternalValueInContext( const MPlug &plug, 
											   const MDataHandle &handle,
											   MDGContext & )
{
  bool retVal = false;
  
  //if reading file, short circuit things, we'll clean up post load
  bool reading = MFileIO::isReadingFile();
  if (reading)
	  m_shader = NULL;

  //MGlobal::displayInfo(MString("Plug update: ") + plug.name());

  if ( plug == sShader) {
    m_shaderName = handle.asString();
	m_shaderPath = m_shaderName;
    if (locateFile( m_shaderName, m_shaderPath)) {
	  // Set shader name to be full path so that we 
	  // can access a fully qualified name from the UI (e.g.
	  // for reloading, viewing and editing the fx file.
	  //
      m_shaderDirty = true;
      if (!reading) {

		  if (MGlobal::mayaState() == MGlobal::kBatch)
  			rebuildShader();
  		else
  		{
  			MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
  			if (pRenderer)
  			{
  				const MString & backEndStr = pRenderer->backEndString();
  				MStatus status = pRenderer->makeResourceContextCurrent( backEndStr );
  				if (status != MStatus::kSuccess) {
					MStatus strStat;
  					MGlobal::displayError(MStringResource::getString(rGLSLShaderNodeFailedResourceContext, strStat));
  					MGlobal::displayError(status.errorString());
  				}
  				else
  					rebuildShader();
  			}
  		}
      }
    }
    else {
      MGlobal::displayError(MString("Couldn't find shader file: '") + m_shaderPath + "'");
    }
    retVal = true;
  }
  else if ( plug == sTechnique) {
    MString temp = handle.asString();
	// We are in file read and m_shader has not been built yet,
	// so don't print out an error message here.
	//
	if (reading)
	{
		m_techniqueName = temp;
		retVal = true;
	}
	else
	{
    if (m_shader) {
      for (int ii=0; ii<m_shader->techniqueCount(); ii++) {
        if (temp == m_shader->techniqueName(ii)) {
          m_shader->setTechnique(ii);
          retVal = true;
          break;
        }
      }
    }

    if (retVal)
      m_techniqueName = temp;
    else
		{
			MGlobal::displayWarning(MString("Unknown technique: ") + temp +
			MString(" for shader: ") + m_shaderPath.asChar());
		}
	}
    retVal = true;
  }
  else if ( plug == sTechniqueList )
  {
	  // Do nothing
  }
  else {

    int ii;

    //handle the naked texture coord sets
    for (ii=0; ii<8; ii++) {
      if (plug == sNakedTexCoords[ii]) {
        m_nakedTexSets[ii] = handle.asString();
        //MGlobal::displayInfo( MString("Setting Tex ") + ii + " to " + m_nakedTexSets[ii]);
        return true;
      }
    }

    for (ii=0; ii<2; ii++) {
      if (plug == sNakedColors[ii]) {
        m_nakedColorSets[ii] = handle.asString();
        //MGlobal::displayInfo( MString("Setting Color ") + ii + " to " + m_nakedColorSets[ii]);
        return true;
      }
    }

    //handle the generic attribute sets
    for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) {
      if (plug == it->attrib) {
        it->setName = handle.asString();
        return true;
      }
    }
    
    if (!reading) {
      //process dynamic attributes

      //need to grab the root in case we are dealing with a vector 
      const MPlug &root = plug.isChild() ? plug.parent() : plug; 
      
      //MGlobal::displayInfo(MString("Root name: ") + root.name());
  
      //
      //  Is this time critical enough to use something like a map, instead or linearly searching all
      // attribute? Present expectation is for shaders to have significantly fewer than 100 attributes.
      for ( std::vector<uniformAttrib>::iterator it2 = m_uniformAttribList.begin(); it2 < m_uniformAttribList.end(); it2++) {
        if ( ( root == it2->attrib) || ( root == it2->attrib2) ){
			it2->dirty = true;
          //allow the set to proceed using the default method
          break;
        }
      }

      for ( std::vector<samplerAttrib>::iterator it = m_samplerAttribList.begin(); it < m_samplerAttribList.end(); it++) {
        if (root == it->attrib) {
          it->dirty = true;
          //allow the set to proceed using the default method
          break;
        }
        if (root == it->anisoAttrib) {
          it->dirty = true;
          it->maxAniso = handle.asFloat();

          //allow the set to proceed using the default method
          break;
        }
        if (root == it->magFilterAttrib) {
          it->dirty = true;
          switch (handle.asShort()) {
            case 0:
              it->magFilter = GL_NEAREST;
              break;
            case 1:
              it->magFilter = GL_LINEAR;
              break;
            default:
              //this should be impossible
              it->magFilter = GL_LINEAR;
              break;
          };

          //allow the set to proceed using the default method
          break;
        }
        if (root == it->minFilterAttrib) {
          it->dirty = true;
          switch (handle.asShort()) {
            case 0:
              it->minFilter = GL_NEAREST;
              break;
            case 1:
              it->minFilter = GL_LINEAR;
              break;
            case 2:
              it->minFilter = GL_LINEAR_MIPMAP_NEAREST;
              break;
            case 3:
              it->minFilter = GL_LINEAR_MIPMAP_LINEAR;
              break;
            default:
              //this should be impossible
              it->minFilter = GL_LINEAR;
              break;
          };

          //allow the set to proceed using the default method
          break;
        }
        if (root == it->wrapAttrib) {
          it->dirty = true;
          switch (handle.asShort()) {
            case 0:
              it->wrapMode = GL_REPEAT;
              break;
            case 1:
              it->wrapMode = GL_CLAMP_TO_EDGE;
              break;
            case 2:
              it->wrapMode = GL_MIRRORED_REPEAT;
              break;
            default:
              //this should be impossible
              it->wrapMode = GL_REPEAT;
              break;
          };

          //allow the set to proceed using the default method
          break;
        }
      }
    }
  }

  GL_CHECK;

  return retVal;
}

void glslShaderNode::copyInternalData( MPxNode* pMPx )
{
	glslShaderNode* pNode = dynamic_cast<glslShaderNode*>( pMPx );

	m_shader = NULL;
	m_shaderDirty = false;

	// These get rebuilt
	m_uniformAttribList.clear();
	m_samplerAttribList.clear();
	m_attributeAttribList.clear();
	m_boundUniformList.clear();

	// Reset temporaries
	m_parsedUserAttribList = false;
	m_numUVsets = 0;
	m_uvSetNames.clear();
	m_numColorSets = 0;
	m_colorSetNames.clear();	
	m_numNormals = 3;
	m_maxSetNum = pNode->m_maxSetNum;

	if( pNode )
	{
		for(int ii=0; ii<8; ii++)
		{
			m_nakedTexSets[ii] = pNode->m_nakedTexSets[ii];
			m_setNums[ii] = pNode->m_setNums[ii];
			m_mayaType[ii] = pNode->m_mayaType[ii];
		}

		m_nakedColorSets[0] = pNode->m_nakedColorSets[0];
		m_nakedColorSets[1] = pNode->m_nakedColorSets[1];

		m_colorSetNums[0] = pNode->m_colorSetNums[0]; 
		m_colorSetNums[1] = pNode->m_colorSetNums[1]; 

		m_colorMayaType[0] = pNode->m_colorMayaType[0];
		m_colorMayaType[1] = pNode->m_colorMayaType[1];

		m_shaderName = pNode->m_shaderName;
		m_shaderPath = m_shaderName;		
		m_techniqueList = "";
		m_techniqueName = "";

		if (locateFile( m_shaderName, m_shaderPath))
		{
			// Must be done before rebuild to set the correct technique
			m_techniqueName = pNode->m_techniqueName;
			m_shaderDirty = true;

			MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
			if (pRenderer)
			{
				const MString & backEndStr = pRenderer->backEndString();
				MStatus status = pRenderer->makeResourceContextCurrent( backEndStr );
				if (status != MStatus::kSuccess) {
					MStatus strStat;
					MGlobal::displayError(MStringResource::getString(rGLSLShaderNodeFailedResourceContext, strStat));
					MGlobal::displayError(status.errorString());
				}
				else
					rebuildShader();
			}
			else
				rebuildShader();
		}
	}
}

//
//attribute request interface
//
////////////////////////////////////////////////////////////////////////////////
int glslShaderNode::colorsPerVertex() {
  return 1;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslShaderNode::texCoordsPerVertex() {
    return 0;
}

#if MAYA_API_VERSION >= 700
  //  There are two versions of parseUserAttributeList, so the plug-in
  // can work on multiple revisions of Maya. The version for 700 and
  // above relies on the presence of the named color sets, and a fix
  // to a bug in the Maya API. 

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::parseUserAttributeList()
{
  if (m_parsedUserAttribList)
    return;

  // Reset shared data which is used by getColorSetNames() and
  // getTexCoordSetNames
  //
  m_numUVsets = 0;
  m_numColorSets = 0;
  m_uvSetNames.clear();
  m_colorSetNames.clear();


  m_numNormals = 3;

  MStringArray uvSetNames;
  MStringArray colorSetNames;

  bool haveNurbs = false;
  // "Magic" name for nurbs uvs
  MString nurbsDefaultUV("Nurbs Tri UVs"); 
  const MDagPath & path = currentPath();	
  if (path.hasFn( MFn::kMesh ) )
  {

    // Check the # of uvsets
    MFnMesh fnMesh( path.node() );
    if (fnMesh.numUVSets())
      fnMesh.getUVSetNames(uvSetNames);

    // Check the # of color sets
    if (fnMesh.numColorSets())
      fnMesh.getColorSetNames(colorSetNames);
  }
  else // is a NURBS
  {
    haveNurbs = true;
    uvSetNames.append(nurbsDefaultUV);

    // There is no such thing as color sets no nurbs
    // for now, so don't add any "default"
  }

  //
  // The code here needs to work as follows:
  //  1. Add nurbs uv set if it is a nurbs.
  //  2. Iterate the list of requested maps
  //  3. Check for special maps (tangent/binormal)
  //  4. If the map is not yet added, add it
  //  5. Record the index

  int ii = 0;
  for (; ii<8; ii++) 
  {
    m_setNums[ii] = -1;
    m_mayaType[ii] = kNonMaya;
  }


  // Fixed splitting character to find qualifiers for inputs.
  // For now only used to scope the tangent, and binormal to
  // a given tangent or binormal set.
  //
  const char _splitChar = ':';

  ii = 0;
  for (; ii<8; ii++) 
  {
	  MStringArray splitStrings;	
	  MString comparisonString = m_nakedTexSets[ii];
	  if ((MStatus::kSuccess == comparisonString.split(_splitChar, splitStrings)) &&
		   splitStrings.length() > 1)
	  {
		  comparisonString = splitStrings[0];
	  }

	  // Special case nurbs. If "map1" is specified then
	  // append the "special" uv set name for nurbs instead
	  if (haveNurbs && comparisonString == "map1") 
	  {
		  m_setNums[ii] = m_uvSetNames.length();
		  m_uvSetNames.append(nurbsDefaultUV);
		  m_mayaType[ii] = kUvSet;
		  continue;
	  }
	  else if (comparisonString == "normal") {
		  m_setNums[ii] = 0; // Normal is currently always for set 0
		  m_mayaType[ii] = kNormal;
		  continue;
	  }
	  else if (comparisonString == "tangent") {
		  m_setNums[ii] = 0; // Tangent is currently always for set 0
		  m_mayaType[ii] = kTangent;
		  continue;
	  }
	  // Note that for NURBS there is currently an API bug
	  // in that you cannot get the binormals even when you
	  // ask for them.
	  else if (comparisonString == "binormal") {
		  m_setNums[ii] = 0; // Binormal is currently always for set 0
		  m_mayaType[ii] = kBinormal;
		  continue;
	  }
	  else if (comparisonString == "") {
		  m_setNums[ii] = -1;
		  m_mayaType[ii] = kNonMaya;
		  continue;
	  }
	  else {
		  // Test out uv set name matches first
		  bool matchUVSet = false;
		  unsigned int numNames = uvSetNames.length();
		  if (numNames)
		  {
			  unsigned int jj = 0;
			  for (jj=0; jj<numNames; jj++) {
				  if (comparisonString == uvSetNames[jj])
				  {
					  // Track name, number and type
					  m_setNums[ii] = m_uvSetNames.length();
					  m_uvSetNames.append( uvSetNames[jj] );
					  m_mayaType[ii] = kUvSet;
					  matchUVSet = true;
					  break;
				  }
			  }
		  }

		  // Test out color set name matches second
		  if (!matchUVSet)
		  {
			  numNames = colorSetNames.length();
			  if (numNames)
			  {
				  unsigned int jj = 0;
				  for (jj=0; jj<numNames ; jj++) {
					  if (comparisonString == colorSetNames[jj])
					  {
						  // Track name, number and type
						  m_setNums[ii] = m_colorSetNames.length();
						  m_colorSetNames.append( colorSetNames[jj] );
						  m_mayaType[ii] = kColorSet;
						  break;
					  }
				  }
			  }
		  }
	  }
  }


  //now handle the colors
  //
  // Colors need to be either 3 or 4 components, so they do not
  // draw from the texture coord sets.
  ii = 0;
  for (; ii<2; ii++) 
  {
	  m_colorSetNums[ii] = -1;
	  m_colorMayaType[ii] = kNonMaya;
  }
  ii = 0;
  for (; ii<2; ii++) 
  {
	  MStringArray splitStrings;	
	  MString comparisonString = m_nakedColorSets[ii];
	  if ((MStatus::kSuccess == comparisonString.split(_splitChar, splitStrings)) &&
		  splitStrings.length() > 1)
	  {
		  comparisonString = splitStrings[0];
	  }


	  if (comparisonString == "normal") {
		  m_colorSetNums[ii] = 0; // Normal is currently always for set 0
		  m_colorMayaType[ii] = kNormal;
		  continue;
	  }
	  else if (comparisonString == "tangent") {
		  m_colorSetNums[ii] = 0; // Tangent is currently always for set 0
		  m_colorMayaType[ii] = kTangent;
		  continue;
	  }
	  // Note that for NURBS there is currently an API bug
	  // in that you cannot get the binormals even when you
	  // ask for them.
	  else if (comparisonString == "binormal") {
		  m_colorSetNums[ii] = 0; // Binormal is currently always for set 0
		  m_colorMayaType[ii] = kBinormal;
		  continue;
	  }
	  else if (comparisonString == "") {
		  m_colorSetNums[ii] = -1;
		  m_colorMayaType[ii] = kNonMaya;
		  continue;
	  }
	  else {

		  // Test out color set name matches first.
		  // Basically the opposite of uv set lookup.
		  //
		  bool matchUVSet = false;

		  // Test out color set name matches
		  unsigned int numNames = colorSetNames.length();
		  if (numNames)
		  {
			  unsigned int jj = 0;
			  for (jj=0; jj<numNames ; jj++) {
				  if (comparisonString == colorSetNames[jj])
				  {
					  // Track name, number and type
					  m_colorSetNums[ii] = m_colorSetNames.length();
					  m_colorSetNames.append( colorSetNames[jj] );
					  m_colorMayaType[ii] = kColorSet;
					  matchUVSet = true;
					  break;
				  }
			  }
		  }

#ifdef _INTERACTIVE_SHADING_WORKS_FOR_ROUTING_TEXCOORDS_TO_COLOUR
		  // Currently this crashes when not in hi-quality mode
		  // so disable.
		  numNames = uvSetNames.length();
		  if (numNames)
		  {
			  unsigned int jj = 0;
			  for (jj=0; jj<numNames; jj++) {
				  if (comparisonString == uvSetNames[jj])
				  {
					  // Track name, number and type
					  m_colorSetNums[ii] = m_uvSetNames.length();
					  m_uvSetNames.append( uvSetNames[jj] );
					  m_colorMayaType[ii] = kUvSet;
					  break;
				  }
			  }
		  }
#endif

	  }
  }

  // now handle the generic attributes
  // unsigned int setNum = 0;
  for (std::vector<attributeAttrib>::iterator it = m_attributeAttribList.begin(); 
	  it<m_attributeAttribList.end(); it++) 
  {
	  MStringArray splitStrings;	
	  MString comparisonString( it->setName );
	  if ((MStatus::kSuccess == comparisonString.split(_splitChar, splitStrings)) &&
		   splitStrings.length() > 1)
	  {
		  comparisonString = splitStrings[0];
	  }

	  if (comparisonString == "normal") {
		  it->mtype = kNormal;
		  it->set = 0;
		  continue;
	  }
	  else if (comparisonString == "tangent") {
		  it->mtype = kTangent;
		  it->set = 0;
		  continue;
	  }
	  else if (comparisonString == "binormal") {
		  it->mtype = kBinormal;
		  it->set = 0;
		  continue;
	  }
	  else if (comparisonString == "") {
		  it->set = -1;
		  continue;
	  }
	  else {
		  // Test out uv set name matches first
		  bool matchUVSet = false;
		  unsigned int numNames = uvSetNames.length();
		  if (numNames)
		  {
			  unsigned int jj = 0;
			  for (jj=0; jj<numNames; jj++) 
			  {
				  // Track name, number and type
				  if (comparisonString == uvSetNames[jj]) 
				  {
					  it->set = m_uvSetNames.length();
					  m_uvSetNames.append( uvSetNames[jj] );
					  it->mtype = kUvSet;
					  matchUVSet = true;
					  break;
				  }
			  }
		  }

		  // Test out color set name matches second
		  if (!matchUVSet)
		  {
			  numNames = colorSetNames.length();
			  if (numNames)
			  {
				  unsigned int jj = 0;
				  for (jj=0; jj<numNames ; jj++) {
					  if (m_nakedTexSets[ii] == colorSetNames[jj])
					  {
						  // Track name, number and type
						  m_colorSetNames.append( colorSetNames[jj] );
						  it->set = m_colorSetNames.length();
						  it->mtype = kColorSet;
						  break;
					  }
				  }
			  }
		  }
	  }
  }

  /////////////////////////////////////////////////////////////////////////////////////////


  // Do a post-scan to see if a specific uvset is specified for tangent
  // and binormal in the uv set entries.
  //
  //printf("----------------------- POST SCAN TEXCOORDS --------------------\n");
  unsigned int numUVSets = uvSetNames.length();
  if (numUVSets)
  {
	  for (ii=0; ii<8; ii++) 
	  {

		  if (m_mayaType[ii] == kTangent || m_mayaType[ii] == kBinormal)
		  {
			  MStringArray splitStrings;
			  MString origString( m_nakedTexSets[ii] );

			  //printf("Attempt to split the TEXCOORD string[%d] = %s.\n",
				//		ii, origString.asChar());

			  if ((MStatus::kSuccess == origString.split(_splitChar, splitStrings)) &&
				  splitStrings.length() > 1)
			  {
				  // Look for a match of existing uvset names

				  //printf("--- Managed to split the string[%d]. Look for uvset %s\n",
					//  ii, splitStrings[1].asChar());
				  unsigned int jj = 0;
				  for (jj=0; jj<numUVSets; jj++)
				  {
					  if (uvSetNames[jj] == splitStrings[1])
					  {
						  int foundSet = -1;
						  for (unsigned int kk=0; kk<m_uvSetNames.length(); kk++)
						  {
							  if (splitStrings[1] == m_uvSetNames[kk])
							  {
								  foundSet = kk;
								  break;
							  }
						  }

						  if (foundSet > 0)
						  {
							  // Set number is index into uvset name array.
							  m_setNums[ii] = foundSet;
						  }
						  else
						  {
							  // Append a new uvset name, and point to end of
							  // uvset name array.
							  m_setNums[ii] = m_uvSetNames.length();
							  m_uvSetNames.append( splitStrings[1] );
						  }

						  //printf("---- Add uvset Index=%d. uvset name %s\n", m_setNums[ii],
							//	splitStrings[1].asChar());

						  break;
					  }
				  }
			  }
		  }
	  }
  }


  // Do a post-scan to see if a specific uvset is specified for tangent
  // and binormal in the color set entries.
  //
//  printf("----------------------- POST SCAN COLORS --------------------\n");
  if (numUVSets)
  {
	  for (ii=0; ii<2; ii++) 
	  {
		  if (m_colorMayaType[ii] == kTangent || m_colorMayaType[ii] == kBinormal)
		  {
			  MStringArray splitStrings;
			  MString origString( m_nakedColorSets[ii] );

			  //printf("Attempt to split the COLOR string[%d] = %s.\n",
				//		ii, origString.asChar());

			  if ((MStatus::kSuccess == origString.split(_splitChar, splitStrings)) &&
				  splitStrings.length() > 1)
			  {
				  // Look for a match of existing uvset names
				  unsigned int jj = 0;
				  for (jj=0; jj<numUVSets; jj++)
				  {
					  if (uvSetNames[jj] == splitStrings[1])
					  {
						  int foundSet = -1;
						  for (unsigned int kk=0; kk<m_uvSetNames.length(); kk++)
						  {
							  if (splitStrings[1] == m_uvSetNames[kk])
							  {
								  foundSet = kk;
								  break;
							  }
						  }

						  if (foundSet > 0)
						  {
							  // Set number is index into uvset name array.
							  m_colorSetNums[ii] = foundSet;
						  }
						  else
						  {
							  // Append a new uvset name, and point to end of
							  // uvset name array.
							  m_colorSetNums[ii] = m_uvSetNames.length();
							  m_uvSetNames.append( splitStrings[1] );
						  }

						  //printf("---- Add uvset Index=%d. uvset name %s\n", m_colorSetNums[ii],
							//	splitStrings[1].asChar());

						  break;
					  }
				  }
			  }
		  }
	  }
  }


  // Do a post-scan to see if a specific uvset is specified for tangent
  // and binormal in the generic entries.
  //
  if (numUVSets)
  {
	  for (std::vector<attributeAttrib>::iterator it2 = m_attributeAttribList.begin(); 
		  it2<m_attributeAttribList.end(); it2++)
	  {
		  if (it2->mtype == kTangent || it2->mtype == kBinormal)
		  {
			  MStringArray splitStrings;
			  MString origString( it2->setName );
			  if ((MStatus::kSuccess == origString.split(_splitChar, splitStrings)) &&
				  splitStrings.length() > 1)
			  {
				  // Look for a match of existing uvset names
				  unsigned int jj = 0;
				  for (jj=0; jj<numUVSets; jj++)
				  {
					  if (uvSetNames[jj] == splitStrings[1])
					  {
						  int foundSet = -1;
						  for (unsigned int kk=0; kk<m_uvSetNames.length(); kk++)
						  {
							  if (splitStrings[1] == m_uvSetNames[kk])
							  {
								  foundSet = kk;
								  break;
							  }
						  }

						  if (foundSet > 0)
						  {
							  // Set number is index into uvset name array.
							  it2->set = foundSet;
						  }
						  else
						  {
							  // Append a new uvset name, and point to end of
							  // uvset name array.
							  it2->set = m_uvSetNames.length();
							  m_uvSetNames.append( splitStrings[1] );
						  }

						  //printf("---- uvset Index=%d. uvset name %s\n", it->set,
							//	splitStrings[1].asChar());

						  break;
					  }		
				  }
			  }
		  }
	  }
  }

  m_numUVsets = m_uvSetNames.length();
  m_numColorSets = m_colorSetNames.length();

	m_maxSetNum = 0;
	for ( ii = 0; ii<8; ii++) 
		if( m_mayaType[ii] != kNonMaya)
			m_maxSetNum = ii + 1;

  m_parsedUserAttribList = true;
}

#else // if MAYA_7

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::parseUserAttributeList()
{
  if (m_parsedUserAttribList)
    return;

  // Reset shared data which is used by getColorSetNames() and
  // getTexCoordSetNames
  //
  m_numUVsets = 0;
  m_numColorSets = 0;
  m_uvSetNames.clear();
  m_colorSetNames.clear();

  //
  //  Always requesting a default set of map1 first, works around
  // an annyoing bug seen in Maya 6.5.
  //
  m_uvSetNames.append("map1");

  int ii = 0;
  for (; ii<8; ii++) 
  {
    m_setNums[ii] = -1;
    m_mayaType[ii] = kNonMaya;
  }
  ii = 0;
  for (; ii<8; ii++) 
  {
    // Special case, grab the special set from set 0
    if ( m_nakedTexSets[ii] == "map1") 
    {
      m_setNums[ii] = 0;
      m_mayaType[ii] = kUvSet;
      continue;
    }
    else if (m_nakedTexSets[ii] == "normal") {
      m_setNums[ii] = 0; // Normal is currently always for set 0
      m_mayaType[ii] = kNormal;
      continue;
    }
    else if (m_nakedTexSets[ii] == "tangent") {
      m_setNums[ii] = 0; // Tangent is currently always for set 0
      m_mayaType[ii] = kTangent;
      continue;
    }
    // Note that for NURBS there is currently an API bug
    // in that you cannot get the binormals even when you
    // ask for them.
    else if (m_nakedTexSets[ii] == "binormal") {
      m_setNums[ii] = 0; // Binormal is currently always for set 0
      m_mayaType[ii] = kBinormal;
      continue;
    }
    else if (m_nakedTexSets[ii] == "") {
      m_setNums[ii] = -1;
      m_mayaType[ii] = kNonMaya;
      continue;
    }
    else {
      //just append the set
      m_setNums[ii] = m_uvSetNames.length();
      m_uvSetNames.append( m_nakedTexSets[ii] );
      m_mayaType[ii] = kUvSet;
    }
  }

  //now handle the generic attributes
  unsigned int setNum = 0;
  for (std::vector<attributeAttrib>::iterator it = m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) {
    if (it->setName == "normal") {
      it->mtype = kNormal;
      it->set = 0;
      continue;
    }
    else if (it->setName == "tangent") {
      it->mtype = kTangent;
      it->set = 0;
      continue;
    }
    else if (it->setName == "binormal") {
      it->mtype = kBinormal;
      it->set = 0;
      continue;
    }
    else if (it->setName == "") {
      it->set = -1;
      continue;
    }
    else {
      //just append the set
      it->set = m_uvSetNames.length();
      m_uvSetNames.append( it->setName );
      it->mtype = kUvSet;
     
    }
  }

  m_numUVsets = m_uvSetNames.length();
  m_numColorSets = m_colorSetNames.length();
	m_maxSetNum = 0;
	for ( ii = 0; ii<8; ii++) 
		if( m_mayaType[ii] != kNonMaya)
			m_maxSetNum = ii + 1;

  m_parsedUserAttribList = true;
}

#endif // if MAYA_7

//
//
//
////////////////////////////////////////////////////////////////////////////////
int	glslShaderNode::getColorSetNames(MStringArray& names)
{
	if (!m_parsedUserAttribList)
		parseUserAttributeList();

	names = m_colorSetNames;
	return m_numColorSets;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslShaderNode::getTexCoordSetNames( MStringArray &names) 
{	
	if (!m_parsedUserAttribList)
		parseUserAttributeList();

	names = m_uvSetNames;
	return m_numUVsets;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
int glslShaderNode::normalsPerVertex() {

  // this can be revisited, now that we have dynamic attributes
  m_numNormals = 3;
  return m_numNormals;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::configureUniformAttrib( MString &name, int pNum, shader::DataType type, shader::Semantic sm, float *defVal,
                                            MFnDependencyNode &dn, MDGModifier &mod) {
  uniformAttrib ua;
  MFnNumericAttribute nAttrib;
  MFnTypedAttribute tAttrib;
  MStatus stat;

  //set the generic attribute values 
  ua.name = name;
  ua.pNum = pNum;
  ua.dirty = true;
  ua.type = type;
  ua.attrib = MObject::kNullObj;
  ua.attrib2 = MObject::kNullObj;

  // [ANIM_UNIFORM_FIX1] : 
  // Note that all uniforms are not set as being internal as this
  // makes animation playback unreliable. [See related code for binding
  // unforms [ANIM_UNIFORM_FIX2]]

  //obtain relavent information
  float min=0.0f, max=0.0f;
  bool color = shader::isColor(sm);
  bool clamped = shader::isClamped(sm);
  shader::getLimits( sm, min, max);

  switch (ua.type) {

    case shader::dtFloat:
      if ( !dn.hasAttribute( name )) {
        ua.attrib = nAttrib.create( name, name, MFnNumericData::kFloat);
        if (clamped) {
          nAttrib.setMin(min);
          nAttrib.setMax(max);
        }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( defVal[0]);
        else
          nAttrib.setDefault( -1.0 );
		stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtVec2:
      if ( !dn.hasAttribute( name )) {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::k2Float);
        if (clamped) {
          nAttrib.setMin( min, min);
          nAttrib.setMax( max, max);
        }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( defVal[0], defVal[1] );
        else
          nAttrib.setDefault( -1.0, -1.0 );
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtVec3:
      if ( !dn.hasAttribute( name )) {
        if (color) {
          ua.attrib = nAttrib.createColor( name, name);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
          nAttrib.setInternal(true);
#endif
		  nAttrib.setKeyable( true );
          if (defVal)
            nAttrib.setDefault( defVal[0], defVal[1], defVal[2] );
        }
        else {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::k3Float);
          if (clamped) {
            nAttrib.setMin( min, min, min);
            nAttrib.setMax( max, max, max);
          }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( defVal[0], defVal[1], defVal[2] );
        else
          nAttrib.setDefault( -1.0, -1.0, -1.0 );
        }
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtVec4:
      if ( !dn.hasAttribute( name )) {
        if (color) {
          ua.attrib = nAttrib.createColor( name, name);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
          nAttrib.setInternal(true);
#endif
		  nAttrib.setKeyable( true );
          if (defVal)
            nAttrib.setDefault( defVal[0], defVal[1], defVal[2] );
          stat = mod.addAttribute( thisMObject(), ua.attrib );
          ua.attrib2 = nAttrib.create( name + "Alpha", name + "Alpha", MFnNumericData::kFloat);
          if (clamped) {
            nAttrib.setMin( 0.0f);
            nAttrib.setMax( 1.0f);
          }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
          nAttrib.setInternal(true);
#endif
		  nAttrib.setKeyable( true );
          if (defVal)
            nAttrib.setDefault( defVal[3]);
          else
            nAttrib.setDefault( 1.0 );
          stat = mod.addAttribute( thisMObject(), ua.attrib2 );
        }
        else {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::k3Float);
          if (clamped) {
            nAttrib.setMin( min, min, min);
            nAttrib.setMax( max, max, max);
          }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( defVal[0], defVal[1], defVal[2] );
        else
          nAttrib.setDefault( -1.0, -1.0, -1.0 );
        stat = mod.addAttribute( thisMObject(), ua.attrib );
        ua.attrib2 = nAttrib.create( name + "W", name + "W", MFnNumericData::kFloat);
          if (clamped) {
            nAttrib.setMin( min);
            nAttrib.setMax( max);
          }
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( defVal[3]);
        else
          nAttrib.setDefault( -1.0 );
        stat = mod.addAttribute( thisMObject(), ua.attrib2 );
      }
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
        ua.attrib2 = dn.attribute( name + "W", &stat);
      }
      break;

    case shader::dtBool:
      if ( !dn.hasAttribute( name )) {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::kBoolean);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        nAttrib.setDefault( true );  //Will this work?
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtInt:
      if ( !dn.hasAttribute( name )) {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::kInt);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        if (defVal)
          nAttrib.setDefault( (int)defVal[0]);
        else
          nAttrib.setDefault( -1 );
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtIVec2:
      if ( !dn.hasAttribute( name )) {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::k2Int);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        nAttrib.setDefault( -1, -1 );
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtIVec3:
      if ( !dn.hasAttribute( name )) {
		ua.attrib = nAttrib.create( name, name, MFnNumericData::k3Int);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        nAttrib.setInternal(true);
#endif
        nAttrib.setKeyable( true );
        nAttrib.setDefault( -1, -1, -1 );
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtMat4:
      if ( !dn.hasAttribute( name )) {
        MFnMatrixAttribute mAttrib;
        ua.attrib = mAttrib.create( name, name, MFnMatrixAttribute::kFloat);
#ifdef _CAN_ANIMATE_INTERNAL_DYNAMIC_ATTRIBS
        mAttrib.setInternal(true);
#endif
		mAttrib.setKeyable( true );
        //How do I set default????
        stat = mod.addAttribute( thisMObject(), ua.attrib );
      }
      else {
        //just grab it, need to make sure it is correct
        ua.attrib = dn.attribute(name, &stat);
      }
      break;

    case shader::dtBVec2:
    case shader::dtBVec3:
    case shader::dtIVec4:
    case shader::dtBVec4:
    case shader::dtMat2:
    case shader::dtMat3:
      //not yet supported, primarly due to no native Maya type
      return;
    default:
      MGlobal::displayWarning(MString("Unknown uniform parameter type for: ") + name);
  };
  if (stat != MStatus::kSuccess) {
	MStatus strStat;
	MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
	error.format(error, name);
	MGlobal::displayError(error);
    MGlobal::displayError(stat.errorString());
  }
  m_uniformAttribList.push_back(ua); //will this cause reference counting problems?
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
void glslShaderNode::configureAttribs() {
  MDGModifier mod;

  {
	  for (std::vector<uniformAttrib>::iterator it=m_uniformAttribList.begin(); it<m_uniformAttribList.end(); it++) {
		  it->pNum = -1; //mark it as unused
	  }
  }

  {
	  for (std::vector<samplerAttrib>::iterator it=m_samplerAttribList.begin(); it<m_samplerAttribList.end(); it++) {
		  it->pNum = -1; //mark it as unused
	  }
  }

  {
	  for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++) {
		  it->pNum = -1; //mark it as unused
	  }
  }

  //no need to worry about reattachment with these, they don't create attributes
  m_boundUniformList.clear();

  MFnDependencyNode dn(thisMObject());

  int ii;

  if (m_shader && m_shader->valid()) {

    //find the uniforms that the new shader uses
    for (ii=0; ii<m_shader->uniformCount(); ii++) {
      MString name(m_shader->uniformName(ii));

      //determine if it is bound or not
      if ( shader::isBound(m_shader->uniformSemantic(ii))) {

        //handle bound uniforms

        //add a new one to the list
        boundUniform bu;
        bu.name = name;
        bu.usage = m_shader->uniformSemantic(ii);
        bu.pNum = ii;

        m_boundUniformList.push_back(bu);
      }
      else {
        //handle uniform attributes (tweakables)
        //iterate the list to find it
        std::vector<uniformAttrib>::iterator it=m_uniformAttribList.begin();

        //might want to store this stuff in a map instead
        while (it < m_uniformAttribList.end()) {
          if (it->name == name)
            break; //found it
          it++; // look at the next
        }

        //see if we found the correct attribute
        if (it !=m_uniformAttribList.end()) {
          // make sure we have the same type
          if ( it->type == m_shader->uniformType(ii)) {

            //record the handle, and mark it as dirty, so we update it
            it->pNum = ii;
            it->dirty = true;

            //process the next attribute
            continue;
          }
          else {
            //need to delete the old one here, otherwise we can't create the new one
            mod.removeAttribute( thisMObject(), it->attrib);
            it->attrib = MObject::kNullObj;

            //check to see if it was a vec4, and kill the last component
            if (it->attrib2 != MObject::kNullObj) {
              mod.removeAttribute( thisMObject(), it->attrib2);
              it->attrib2 = MObject::kNullObj;
            }
          }
        }
        
        //if there was no appropriate attribute create a new one and add it
        configureUniformAttrib( name, ii, m_shader->uniformType(ii), m_shader->uniformSemantic(ii),
                                m_shader->uniformDefault(ii), dn, mod);
          
        GL_CHECK;
      }
    }

    //samplers
    for (ii=0; ii<m_shader->samplerCount(); ii++) {
      MString name(m_shader->samplerName(ii));

      //iterate the list to find it
      std::vector<samplerAttrib>::iterator it=m_samplerAttribList.begin();

      //might want to store this stuff in a map instead
      while (it < m_samplerAttribList.end()) {
        if (it->name == name)
          break; //found it
        
        it++; // look at the next
      }

      if (it !=m_samplerAttribList.end()) {
        //if found, set the pNum to point to this one
        it->pNum = ii;
        it->dirty = false;
        m_shader->updateSampler( ii, it->texName);
      }
      else {
        //if not found, create a new one and add it
        samplerAttrib sa;
        MFnNumericAttribute nAttrib;
        MFnEnumAttribute eAttrib;
        MStatus stat;

        sa.name = name;
        sa.pNum = ii;
        sa.dirty = true;
        sa.texName = 0;
        sa.minFilter = GL_LINEAR;
        sa.magFilter = GL_LINEAR;
        sa.wrapMode = GL_REPEAT;
        sa.maxAniso = 1.0f;

        if ( !dn.hasAttribute( name )) {
          sa.attrib = nAttrib.createColor( name, name);
          nAttrib.setInternal(true);
          stat = mod.addAttribute( thisMObject(), sa.attrib);
        }
        else {
          sa.attrib = dn.attribute(name, &stat);
        }
        if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
			error.format(error, name);
			MGlobal::displayError(error);
	        MGlobal::displayError(stat.errorString());
        }
        if ( !dn.hasAttribute( name +"MinFilter")) {
          sa.minFilterAttrib = eAttrib.create( name+"MinFilter", name+"MinFilter");
          eAttrib.addField( "Nearest", 0);
          eAttrib.addField( "Linear", 1);
          eAttrib.addField( "Linear, Mipmaped", 2);
          eAttrib.addField( "Trilinear", 3);
          eAttrib.setDefault( 1);
          eAttrib.setInternal(true);
          
          stat = mod.addAttribute( thisMObject(), sa.minFilterAttrib);
        }
        else {
          sa.minFilterAttrib = dn.attribute(name+"MinFilter", &stat);
          //need to query the filter attribute
		  MPlug plug(thisMObject(), sa.minFilterAttrib);
          short val;
          plug.getValue(val);
          switch (val) {
            case 0:
              sa.minFilter = GL_NEAREST;
              break;
            case 1:
              sa.minFilter = GL_LINEAR;
              break;
            case 2:
              sa.minFilter = GL_LINEAR_MIPMAP_NEAREST;
              break;
            case 3:
              sa.minFilter = GL_LINEAR_MIPMAP_LINEAR;
              break;
            default:
              //this should be impossible
              sa.minFilter = GL_LINEAR;
              break;
          };
        }
        if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
			error.format(error, name + "MinFilter");
			MGlobal::displayError(error);
			MGlobal::displayError(stat.errorString());
        }
        if ( !dn.hasAttribute( name +"MagFilter")) {
          sa.magFilterAttrib = eAttrib.create( name+"MagFilter", name+"MagFilter");
          eAttrib.addField( "Nearest", 0);
          eAttrib.addField( "Linear", 1);
          eAttrib.setDefault( 1);
          eAttrib.setInternal(true);
          
          stat = mod.addAttribute( thisMObject(), sa.magFilterAttrib);
        }
        else {
          sa.magFilterAttrib = dn.attribute(name+"MagFilter", &stat);
          //need to query the filter attribute
          MPlug plug(thisMObject(), sa.magFilterAttrib);
          short val;
          plug.getValue(val);
          switch (val) {
            case 0:
              sa.magFilter = GL_NEAREST;
              break;
            case 1:
              sa.magFilter = GL_LINEAR;
              break;
            default:
              //this should be impossible
              sa.magFilter = GL_LINEAR;
              break;
          };
        }
        if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
			error.format(error, name + "MagFilter");
			MGlobal::displayError(error);
			MGlobal::displayError(stat.errorString());
        }
        if ( !dn.hasAttribute( name+"Wrap" )) {
          sa.wrapAttrib = eAttrib.create( name+"Wrap", name+"Wrap");
          eAttrib.addField( "Repeat", 0);
          eAttrib.addField( "Clamp", 1);
          eAttrib.addField( "Mirror", 2);
          eAttrib.setDefault( 0);
          eAttrib.setInternal(true);
          
          stat = mod.addAttribute( thisMObject(), sa.wrapAttrib);
        }
        else {
          sa.wrapAttrib = dn.attribute(name+"Wrap", &stat);
          //need to query the filter attribute
		  MPlug plug(thisMObject(), sa.wrapAttrib);
          short val;
          plug.getValue(val);
          switch (val) {
            case 0:
              sa.wrapMode = GL_REPEAT;
              break;
            case 1:
              sa.wrapMode = GL_CLAMP_TO_EDGE;
              break;
            case 2:
              sa.wrapMode = GL_MIRRORED_REPEAT;
              break;
            default:
              //this should be impossible
              sa.wrapMode = GL_REPEAT;
              break;
          };
        }
        if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
			error.format(error, name + "Wrap");
			MGlobal::displayError(error);
			MGlobal::displayError(stat.errorString());
        }
        if ( !dn.hasAttribute( name+"MaxAniso" )) {
          sa.anisoAttrib = nAttrib.create( name+"MaxAniso", name+"MaxAniso", MFnNumericData::kFloat, 1.0);
          
          nAttrib.setInternal(true);
          nAttrib.setMin( 1.0);
          nAttrib.setMax( 16.0);
          stat = mod.addAttribute( thisMObject(), sa.anisoAttrib);
        }
        else {
          sa.anisoAttrib = dn.attribute(name+"MaxAniso", &stat);
          //need to query the filter attribute
		  MPlug plug(thisMObject(), sa.anisoAttrib);
          float val;
          plug.getValue(val);
          sa.maxAniso = val;
        }
        if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedAddAttribute, strStat);
			error.format(error, name + "MaxAniso");
			MGlobal::displayError(error);
			MGlobal::displayError(stat.errorString());
        }
        m_samplerAttribList.push_back(sa);
        GL_CHECK;
      }
    }

    //attributes
    for (ii=0; ii<m_shader->attributeCount(); ii++) {
      MString name(m_shader->attributeName(ii));

      //iterate the list to find it
      std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin();

      //might want to store this stuff in a map instead
      while (it < m_attributeAttribList.end()) {
        if (it->name == name)
          break; //found it
        
        it++; // look at the next
      }

      if (it !=m_attributeAttribList.end()) {
        //if found, set the pNum to point to this one
        it->pNum = ii;
        it->handle = m_shader->attributeHandle( ii);
        it->set = -1;
      }
      else {
        MFnTypedAttribute tAttrib;
        attributeAttrib aa;
        MStatus stat;

        aa.name = name;
        aa.setName = name;
        aa.pNum = ii;
        aa.handle = m_shader->attributeHandle(ii);
        aa.set = -1; //no set
		aa.mtype = kNonMaya;

        if ( !dn.hasAttribute( name )) {
          aa.attrib = tAttrib.create( name, name, MFnData::kString);
          
          tAttrib.setInternal(true);
          tAttrib.setKeyable( true );
          stat = mod.addAttribute( thisMObject(), aa.attrib);
        }
        else {
          aa.attrib = dn.attribute(name, &stat);
        }
        m_attributeAttribList.push_back(aa);
      }
    }
  }

  for (ii=0; ii< (int)m_uniformAttribList.size(); ) {
    if (m_uniformAttribList[ii].pNum == -1) {

      //attrib is unused, remove it
      if (m_uniformAttribList[ii].attrib != MObject::kNullObj ) {
        mod.removeAttribute( thisMObject(), m_uniformAttribList[ii].attrib);
        m_uniformAttribList[ii].attrib = MObject::kNullObj;
      }

      if ( m_uniformAttribList[ii].attrib2 != MObject::kNullObj ) {
        mod.removeAttribute( thisMObject(), m_uniformAttribList[ii].attrib2);
        m_uniformAttribList[ii].attrib2 = MObject::kNullObj;
      }
      
      m_uniformAttribList.erase(m_uniformAttribList.begin()+ii);
    }
    else {
      //move to the next item
      ii++;
    }
  }

  for (ii=0; ii<(int)m_samplerAttribList.size(); ) {
    MStatus stat, stat2, stat3, stat4, stat5;
    if (m_samplerAttribList[ii].pNum == -1) {
      //attrib is unused, disconnect it and remove it
      
      if ( m_samplerAttribList[ii].attrib != MObject::kNullObj )
        stat = mod.removeAttribute( thisMObject(), m_samplerAttribList[ii].attrib);

      if ( m_samplerAttribList[ii].magFilterAttrib != MObject::kNullObj )
        stat2 = mod.removeAttribute( thisMObject(), m_samplerAttribList[ii].magFilterAttrib);

      if ( m_samplerAttribList[ii].minFilterAttrib != MObject::kNullObj )
        stat3 = mod.removeAttribute( thisMObject(), m_samplerAttribList[ii].minFilterAttrib);

      if ( m_samplerAttribList[ii].wrapAttrib != MObject::kNullObj )
        stat4 = mod.removeAttribute( thisMObject(), m_samplerAttribList[ii].wrapAttrib);

      if ( m_samplerAttribList[ii].anisoAttrib != MObject::kNullObj )
        stat5 = mod.removeAttribute( thisMObject(), m_samplerAttribList[ii].anisoAttrib);

      if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedDestroyAttribute, strStat);
			error.format(error, m_samplerAttribList[ii].name);
			MGlobal::displayError(error);
			MGlobal::displayError(MString("  ") + stat.errorString());
      }

      //I don't think we have a context for certain here
      ResourceManager::destroyTexture( m_samplerAttribList[ii].texName);
      
      m_samplerAttribList[ii].attrib = MObject::kNullObj;
      m_samplerAttribList.erase(m_samplerAttribList.begin()+ii);
    }
    else {
      //move to the next item
      ii++;
    }
  }

  for (ii=0; ii<(int)m_attributeAttribList.size(); ) {
    MStatus stat;
    if (m_attributeAttribList[ii].pNum == -1) {
      //attrib is unused, disconnect it and remove it
      
      stat = mod.removeAttribute( thisMObject(), m_attributeAttribList[ii].attrib);
      if (stat != MStatus::kSuccess) {
			MStatus strStat;
			MString error = MStringResource::getString(rGLSLShaderNodeFailedDestroyAttribute, strStat);
			error.format(error, m_attributeAttribList[ii].name);
			MGlobal::displayError(MString("  ") + stat.errorString());
      }

      m_attributeAttribList[ii].attrib = MObject::kNullObj;
      m_attributeAttribList.erase(m_attributeAttribList.begin()+ii);
    }
    else {
      //move to the next item
      ii++;
    }
  }

  GL_CHECK;

  mod.doIt();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////
bool glslShaderNode::locateFile( const MString &name, MString &path) {
  MString proj;
  struct stat fstats;
  MString temp;

  // 
  // I swear, there must be a better way
  MGlobal::executeCommand( MString("workspace -q -rd;"), proj);

  //search order:
  // current directory
  // project/shaders directory
  // project directory

  // Try absolute path first
  if ( stat(name.asChar(), &fstats) == 0){
    //found it in the current path
    path = name;
    return true;
  }

  // Get the current working directory and
  // try relative path
  char buffer[MAX_PATH_LENGTH];
  if ( getcwd( buffer, MAX_PATH_LENGTH ) != NULL)
  {
	  temp = MString(buffer) + "\\" + name;
	  if ( stat(temp.asChar(), &fstats) == 0){
		  //found it in the current path
		  path = temp;
		  return true;
	  }
  }

  temp = proj + "shaders/" + name;
  if ( stat(temp.asChar(), &fstats) == 0){
    //found it in the project/shaders path
    path = temp;
    return true;
  }
 
  temp = proj + name;
  if ( stat(temp.asChar(), &fstats) == 0){
    //found it in the project path
    path = temp;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////
// Swatch rendering
////////////////////////////////////////////////////////////////////////

/* virtual */
MStatus glslShaderNode::renderSwatchImage( MImage & outImage )
{
	MStatus status = MStatus::kFailure;

	// Get the hardware renderer utility class
	MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
	if (pRenderer)
	{
		const MString& backEndStr = pRenderer->backEndString();

		// Get geometry
		// ============
		unsigned int* pIndexing = 0;
		unsigned int  numberOfData = 0;
		unsigned int  indexCount = 0;

		MHardwareRenderer::GeometricShape gshape = MHardwareRenderer::kDefaultSphere;
		MGeometryData* pGeomData =
			pRenderer->referenceDefaultGeometry( gshape, numberOfData, pIndexing, indexCount );
		if( !pGeomData )
		{
			return MStatus::kFailure;
		}

		// Make the swatch context current
		// ===============================
		//
		unsigned int width, height;
		outImage.getSize( width, height );
		unsigned int origWidth = width;
		unsigned int origHeight = height;

		MStatus status2 = pRenderer->makeSwatchContextCurrent( backEndStr, width, height );

		if( status2 != MS::kSuccess )
		{
			pRenderer->dereferenceGeometry( pGeomData, numberOfData );
            return status2;
		}

		// Get the light direction from the API, and use it
		// =============================================
		{
			float light_pos[4];
			pRenderer->getSwatchLightDirection( light_pos[0], light_pos[1], light_pos[2], light_pos[3] );

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glLightfv(GL_LIGHT0, GL_POSITION, light_pos );
			glPopMatrix();

			float light_ambt[4] = {1.0f, 1.0f, 1.0f, 1.0};
			float light_diff[4] = {1.0f, 1.0f, 1.0f, 1.0};
			float light_spec[4] = {1.0f, 1.0f, 1.0f, 1.0};

			glLightfv( GL_LIGHT0, GL_AMBIENT,  light_ambt );
			glLightfv( GL_LIGHT0, GL_DIFFUSE,  light_diff );
			glLightfv( GL_LIGHT0, GL_SPECULAR, light_spec );

			glEnable( GL_LIGHTING );
			glEnable( GL_LIGHT0 );

            GL_CHECK;
		}

		// Get camera
		// ==========
		{
			// Get the camera frustum from the API
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			double l, r, b, t, n, f;
			pRenderer->getSwatchPerspectiveCameraSetting( l, r, b, t, n, f );
			glFrustum( l, r, b, t, n, f );

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			float x, y, z, w;
			pRenderer->getSwatchPerspectiveCameraTranslation( x, y, z, w );
			glTranslatef( x, y, z );

            GL_CHECK;
		}

		// Get the default background color and clear the background
		//
		float r, g, b, a;
		MHWShaderSwatchGenerator::getSwatchBackgroundColor( r, g, b, a );
		glClearColor( r, g, b, a );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        GL_CHECK;

		// Draw The Swatch
		// ===============
		//drawTheSwatch( pGeomData, pIndexing, numberOfData, indexCount );
		MDagPath dummyPath;
		glBind( dummyPath );

		float *vertexData = (float *)( pGeomData[0].data() );
		float *normalData = (float *)( pGeomData[1].data() );
		float *uvData = (float *)( pGeomData[2].data() );
		float *tangentData = (float *)( pGeomData[3].data() );
		float *binormalData = (float *)( pGeomData[4].data() );
		unsigned int normalCount = 0;

		// Stick normal, tangent, binormals into ptr array
		float ** normalArrays = new float * [3];
		if (normalData) 
		{
			normalArrays[0] = normalData;
			normalCount++;
		}
		else
			normalArrays[0] = NULL;
		if (tangentData) 
		{
			normalArrays[1] = tangentData;
			normalCount++;
		}
		else
			normalArrays[1] = NULL;
		if (binormalData)
		{
			normalArrays[2] = binormalData;
			normalCount++;
		}
		else
			normalArrays[2] = NULL;

		// Stick uv data into ptr array
		unsigned int uvCount = 0;
		float ** texCoordArrays = new float * [1];
		if (uvData)
		{
			texCoordArrays[0] = uvData;
			uvCount = 1;
		}
		else
			texCoordArrays[0] = NULL;

		m_setNums[0] = 0;
		m_setNums[1] = 0;
		m_setNums[2] = 0;
		m_setNums[3] = 0;
		m_setNums[4] = 0;
		m_setNums[5] = 0;
		m_setNums[6] = 0;
		m_setNums[7] = 0;

		m_mayaType[0] = kUvSet;
		m_mayaType[1] = kTangent;
		m_mayaType[2] = kBinormal;
		m_mayaType[3] = kUvSet;
		m_mayaType[4] = kUvSet;
		m_mayaType[5] = kUvSet;
		m_mayaType[6] = kUvSet;
		m_mayaType[7] = kUvSet;

		// No colours
		m_colorSetNums[0] = -1;
		m_colorSetNums[1] = -1;
		m_colorMayaType[0] = kColorSet;
		m_colorMayaType[1] = kColorSet;

		glGeometry( dummyPath,
					GL_TRIANGLES,
					false,
					indexCount,
					pIndexing,
					0, 
					NULL, /* no vertex ids */
					vertexData,
					normalCount,
					(const float **) normalArrays,
					0, 
					NULL, /* no colours */
					uvCount,
					(const float **) texCoordArrays);


		glUnbind( dummyPath );
		glFinish();

		normalArrays[0] = NULL;
		normalArrays[1] = NULL;
		delete[] normalArrays;
		texCoordArrays[0] = NULL;
		delete[] texCoordArrays;

		// Read pixels back from swatch context to MImage
		// ==============================================
		pRenderer->readSwatchContextPixels( backEndStr, outImage );

		// Double check the outing going image size as image resizing
		// was required to properly read from the swatch context
		outImage.getSize( width, height );
		if (width != origWidth || height != origHeight)
		{
			status = MStatus::kFailure;
		}
		else
		{
			status = MStatus::kSuccess;
		}
	}
    GL_CHECK;
	return status;
}

//
//
////////////////////////////////////////////////////////////////////////////////
bool glslShaderNode::printVertexShader( int pass) {

  if ( m_shader) {
    const char *shader = m_shader->getVertexShader( pass);
    if (shader) {
      MGlobal::displayInfo(shader);
      return true;
    }
  }

  return false;
}

//
//
////////////////////////////////////////////////////////////////////////////////
bool glslShaderNode::printPixelShader( int pass) {
  if ( m_shader) {
    const char *shader = m_shader->getPixelShader( pass);
    if (shader) {
      MGlobal::displayInfo(shader);
      return true;
    }
  }

  return false;
}

//
//
////////////////////////////////////////////////////////////////////////////////
glslShaderNode* glslShaderNode::findNodeByName(const MString &name) {
  for (std::vector<glslShaderNode*>::iterator it = sNodeList.begin(); it < sNodeList.end(); it++) {
    if ( (*it)->name() == name)
      return *it;
  }

  return NULL;
}

