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
#include <stdlib.h>

#include "Observer.h"
#include "Platform.h"

#define USE_STATE_ITEMS

//
//
////////////////////////////////////////////////////////////////////////////////
void blendStateItem::apply() {
  //handle enables
  if (m_useEnable) {
    if (m_enable)
      glEnable( GL_BLEND);
    else
      glDisable(GL_BLEND);
  }

  //handle the blend func
  if (m_useBlend) {
    if (m_getSrcFactor)
      glGetIntegerv( GL_BLEND_SRC, (GLint*)&m_srcFactor);
    if (m_getDstFactor)
      glGetIntegerv( GL_BLEND_DST, (GLint*)&m_dstFactor);
    glBlendFunc( m_srcFactor, m_dstFactor);
  }

  //handle the blend equation
  if ( (m_useBlendOp) &&(glBlendEquationSupport)) {
    glBlendEquation( m_blendOp);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void depthStateItem::apply() {

  if (m_useEnable) {
    if (m_enable)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
  }

  if (m_useDepthFunc) {
    glDepthFunc(m_depthFunc);
  }

  if (m_useDepthMask) {
    glDepthMask(m_depthMask);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stencilStateItem::apply() {

  if (m_useEnable) {
    if (m_enable)
      glEnable(GL_STENCIL_TEST);
    else
      glDisable(GL_STENCIL_TEST);
  }

  if (m_useStencilFunc) {

    //get any unset values
    if (m_getStencilFunc)
      glGetIntegerv( GL_STENCIL_FUNC, (GLint*)&m_func);
    if (m_getStencilRMask)
      glGetIntegerv( GL_STENCIL_VALUE_MASK, (GLint*)&m_rmask);
    if (m_getStencilRef)
      glGetIntegerv( GL_STENCIL_REF, (GLint*)&m_ref);

    //set the values
    glStencilFunc( m_func, m_ref, m_rmask);
  }

  if (m_useStencilOp) {

    //get the unset values
    if (m_getDepthPass)
      glGetIntegerv( GL_STENCIL_PASS_DEPTH_PASS, (GLint*)&m_depthPassOp);
    if (m_getDepthFail)
      glGetIntegerv( GL_STENCIL_PASS_DEPTH_FAIL, (GLint*)&m_depthFailOp);
    if (m_getStencilFail)
      glGetIntegerv( GL_STENCIL_FAIL, (GLint*)&m_stencilFailOp);

    //set the values
    glStencilOp( m_stencilFailOp, m_depthFailOp, m_depthPassOp);
  }

  if (m_useStencilMask) {
    glStencilMask(m_mask);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void primitiveStateItem::apply() {

  //polygon mode
  if (m_usePolygonMode) {
    glPolygonMode( GL_FRONT_AND_BACK, m_polygonMode);
  }

  //face culling
  if (m_useEnableCull) {
    if (m_enableCull)
      glEnable(GL_CULL_FACE);
    else
      glDisable(GL_CULL_FACE);
  }

  if (m_useCullFace) {
      glCullFace(m_cullFace);
  }

  //polygon offset
  if (m_useEnablePolygonOffset) {
    //need to set all posible modes, since FX has just one offset
    if (m_enablePolygonOffset){
      glEnable( GL_POLYGON_OFFSET_FILL);
      glEnable( GL_POLYGON_OFFSET_LINE);
      glEnable( GL_POLYGON_OFFSET_POINT);
    }
    else {
      glDisable( GL_POLYGON_OFFSET_FILL);
      glDisable( GL_POLYGON_OFFSET_LINE);
      glDisable( GL_POLYGON_OFFSET_POINT);
    }
  }

  if (m_usePolygonOffset) {
    if (m_getFactor) {
      glGetFloatv( GL_POLYGON_OFFSET_FACTOR, &m_factor);
    }
    if (m_getUnits) {
      glGetFloatv( GL_POLYGON_OFFSET_UNITS, &m_units);
    }
    glPolygonOffset( m_factor, m_units);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void alphaStateItem::apply() {
  if (m_useEnable) {
    if (m_enable)
      glEnable(GL_ALPHA_TEST);
    else
      glEnable(GL_ALPHA_TEST);
  }

  if (m_useAlphaFunc) {
    if (m_getAlphaFunc)
      glGetIntegerv( GL_ALPHA_TEST_FUNC, (GLint*)&m_alphaFunc);
    if (m_getRef)
      glGetFloatv( GL_ALPHA_TEST_REF, &m_ref);
    glAlphaFunc( m_alphaFunc, m_ref);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void colorStateItem::apply() {
  if (m_useDither) {
    if (m_dither)
      glEnable(GL_DITHER);
    else
      glDisable(GL_DITHER);
  }
  if (m_useMask) {
    glColorMask( m_mask[0], m_mask[1], m_mask[2], m_mask[3]);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void fogStateItem::apply() {
  if (m_useEnable) {
    if (m_enable)
      glEnable(GL_FOG);
    else
      glDisable(GL_FOG);
  }

  if (m_useMode) {
    glFogi( GL_FOG_MODE, m_mode);
  }

  if (m_useFogStart) {
    glFogf( GL_FOG_START, m_start);
  }

  if (m_useFogEnd) {
    glFogf( GL_FOG_END, m_end);
  }

  if (m_useFogDensity) {
    glFogf( GL_FOG_DENSITY, m_density);
  }

  if (m_useFogColor) {
    glFogfv( GL_FOG_COLOR, m_color);
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void pointStateItem::apply() {

  if (m_usePointSize) {
    glPointSize( m_pointSize);
  }

  if ( (m_usePointSizeMin) && (glPointParametersSupport)) {
    glPointParameterf( GL_POINT_SIZE_MIN_ARB, m_pointSizeMin);
  }

  if ( (m_usePointSizeMax) && (glPointParametersSupport)) {
    glPointParameterf( GL_POINT_SIZE_MAX_ARB, m_pointSizeMax);
  }

  if ( (m_usePointAtten) && (glPointParametersSupport)) {
    if (m_getAttenA | m_getAttenB | m_getAttenC) {
      float atten[3];
      glGetFloatv( GL_POINT_DISTANCE_ATTENUATION_ARB, atten);
      if (m_getAttenA) {
        m_pointAtten[0] = atten[0];
      }
      if (m_getAttenB) {
        m_pointAtten[1] = atten[1];
      }
      if (m_getAttenC) {
        m_pointAtten[2] = atten[2];
      }
    }
    glPointParameterfv( GL_POINT_DISTANCE_ATTENUATION_ARB, m_pointAtten);
  }

  if (m_usePointSprite) {
    if (m_pointSprite){
      glEnable( GL_POINT_SPRITE_ARB);
      //asuming tex 0 is set
      glTexEnvi( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
    }
    else{
      glDisable( GL_POINT_SPRITE_ARB);
      //asuming tex 0 is set
      glTexEnvi( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_FALSE);
    }
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
passState::~passState() {
  for (std::vector<stateItem*>::iterator it = m_stateList.begin(); it<m_stateList.end(); it++) {
    delete *it;
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void passState::setState() {
  for (std::vector<stateItem*>::iterator it = m_stateList.begin(); it<m_stateList.end(); it++) {
    (*it)->apply();
  }
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setPassMonitor( passState *state) { 
  m_state = state;
  m_bs = new blendStateItem;
  m_ds = new depthStateItem;
  m_ss = new stencilStateItem;
  m_ps = new primitiveStateItem;
  m_as = new alphaStateItem;
  m_cs = new colorStateItem;
  m_fs = new fogStateItem;
  m_ptS = new pointStateItem;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::finalizePassMonitor() {
  
  //copy the blend states to the pass state list
  if (m_bs->m_options)
    m_state->m_stateList.push_back(m_bs);
  else 
    delete m_bs;
  m_bs = NULL;

  //copy the depth states to the pass state list
  if (m_ds->m_options)
    m_state->m_stateList.push_back(m_ds);
  else
    delete m_ds;
  m_ds = NULL;

  //copy the stencil states to the pass state list
  if (m_ss->m_options)
    m_state->m_stateList.push_back(m_ss);
  else
    delete m_ss;
  m_ss = NULL;

  //copy the prim states to the pass state list
  if (m_ps->m_options)
    m_state->m_stateList.push_back(m_ps);
  else
    delete m_ps;
  m_ps = NULL;

  //copy the alpha states to the pass state list
  if (m_as->m_options)
    m_state->m_stateList.push_back(m_as);
  else
    delete m_as;
  m_as = NULL;

  //copy the color states to the pass state list
  if (m_cs->m_options)
    m_state->m_stateList.push_back(m_cs);
  else
    delete m_cs;
  m_cs = NULL;

  //copy the depth states to the pass state list
  if (m_fs->m_options)
    m_state->m_stateList.push_back(m_fs);
  else
    delete m_fs;
  m_fs = NULL;

  //copy the depth states to the pass state list
  if (m_ptS->m_options)
    m_state->m_stateList.push_back(m_ptS);
  else
    delete m_ptS;
  m_ptS = NULL;

  m_state = NULL; // for safety's sake
}

//
//
////////////////////////////////////////////////////////////////////////////////
GLenum stateObserver::compareFunc( const char* value) {
  if (!strcasecmp( value, "LessEqual")) {
    return GL_LEQUAL;
  }
  else if (!strcasecmp( value, "LEqual")) {
    return GL_LEQUAL;
  }
  else if (!strcasecmp( value, "Less")) {
    return GL_LESS;
  }
  else if (!strcasecmp( value, "Equal")) {
    return GL_EQUAL;
  }
  else if (!strcasecmp( value, "GreaterEqual")) {
    return GL_GEQUAL;
  }
  else if (!strcasecmp( value, "NotEqual")) {
    return GL_NOTEQUAL;
  }
  else if (!strcasecmp( value, "NEqual")) {
    return GL_NOTEQUAL;
  }
  else if (!strcasecmp( value, "Always")) {
    return GL_ALWAYS;
  }
  else if (!strcasecmp( value, "Never")) {
    return GL_NEVER;
  } 

  return GL_NEVER;
}

//
//
////////////////////////////////////////////////////////////////////////////////
GLenum stateObserver::blendFactor( const char* value) {
  if (!strcasecmp( value, "ZERO")) {
    return GL_ZERO;
  }
  else if (!strcasecmp( value, "ONE")) {
    return GL_ONE;
  }
  else if (!strcasecmp( value, "SRCCOLOR")) {
    return GL_SRC_COLOR;
  }
  else if (!strcasecmp( value, "INVSRCCOLOR")) {
    return GL_ONE_MINUS_SRC_COLOR;
  }
  else if (!strcasecmp( value, "SRCALPHA")) {
    return GL_SRC_ALPHA;
  }
  else if (!strcasecmp( value, "INVSRCALPHA")) {
    return GL_ONE_MINUS_SRC_ALPHA;
  }
  else if (!strcasecmp( value, "DESTALPHA")) {
    return GL_DST_ALPHA;
  }
  else if (!strcasecmp( value, "INVDESTALPHA")) {
    return GL_ONE_MINUS_DST_ALPHA;
  }
  else if (!strcasecmp( value, "DESTCOLOR")) {
    return GL_DST_COLOR;
  }
  else if (!strcasecmp( value, "INVDESTCOLOR")) {
    return GL_ONE_MINUS_DST_COLOR;
  }
  else if (!strcasecmp( value, "SRCALPHASAT")) {
    return GL_SRC_ALPHA_SATURATE;
  }
  else if (!strcasecmp( value, "D3DBLEND_BLENDFACTOR")) {
      return GL_CONSTANT_COLOR;
  }
  else if (!strcasecmp( value, "D3DBLEND_INVBLENDFACTOR")) {
      return GL_ONE_MINUS_CONSTANT_COLOR;
  }

  return GL_ZERO;

}

//
//
////////////////////////////////////////////////////////////////////////////////
GLenum stateObserver::stencilOp( const char* value) {
  if (!strcasecmp( value, "KEEP")) {
    return GL_KEEP;
  }
  else if (!strcasecmp( value, "ZERO")) {
    return GL_ZERO;
  }
  else if (!strcasecmp( value, "REPLACE")) {
    return GL_REPLACE;
  }
  else if (!strcasecmp( value, "INCRSAT")) {
    return GL_INCR;
  }
  else if (!strcasecmp( value, "DECRSAT")) {
    return GL_DECR;
  }
  else if (!strcasecmp( value, "INVERT")) {
    return GL_INVERT;
  }
  else if (!strcasecmp( value, "INCR")) {
    return GL_INCR_WRAP;
  }
  else if (!strcasecmp( value, "DECR")) {
    return GL_DECR_WRAP;
  }

  return GL_KEEP;
}

//
//
////////////////////////////////////////////////////////////////////////////////
GLenum stateObserver::blendOp( const char* value) {
  if (!strcasecmp( value, "ADD")) {
    return GL_FUNC_ADD;
  }
  else if (!strcasecmp( value, "SUBTRACT")) {
    return GL_FUNC_SUBTRACT;
  }
  else if (!strcasecmp( value, "REVERSESUBTRACT")) {
    return GL_FUNC_REVERSE_SUBTRACT;
  }
  else if (!strcasecmp( value, "MIN")) {
    return GL_MIN;
  }
  else if (!strcasecmp( value, "MAX")) {
    return GL_MAX;
  }

  return GL_FUNC_ADD;
}

//
//
////////////////////////////////////////////////////////////////////////////////
GLenum stateObserver::polyMode( const char* value) {
  if (!strcasecmp( value, "POINT")) {
    return GL_POINT;
  }
  else if (!strcasecmp( value, "WIREFRAME")) {
    return GL_LINE;
  }
  else if (!strcasecmp( value, "SOLID")) {
    return GL_FILL;
  }

  return GL_FILL;
}


//
//
////////////////////////////////////////////////////////////////////////////////
bool stateObserver::isTrue( const char* value) {
  return strcasecmp( value, "true") == 0;
}

//
//
////////////////////////////////////////////////////////////////////////////////
bool stateObserver::isFalse( const char* value) {
  return strcasecmp( value, "false") == 0;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setLightState(LightState state, int handle, const char* value) {
  //right now we track state one time, and track register values another
  
  if (m_state)
    return;

  //ignoring light state
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setMaterialState(MaterialState state, int handle, const char* value) {
  //right now we track state one time, and track register values another


  if (m_state)
    return;

  //ignoring material state
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setVertexRenderState(VertexRenderState state, int handle, const char* value) {


  //right now we track state one time, and track register values another
  //if (m_state)
  //  return;

  //vertex/polygon operations
  switch (state ) {
    case VertexRenderNone:
      //is this case an error?
      break;
    case Ambient:
    case AmbientMaterialSource:
    case Clipping:
    case ClipPlaneEnable:
    case ColorVertex:
      //not supported, due to a lack of meaning
      break;
    case CullMode:
#ifdef USE_STATE_ITEMS
      {
        m_ps->m_useEnableCull = 1;
        m_ps->m_useCullFace = 1;
        if (!strcasecmp( value, "none")) {
          m_ps->m_enableCull = GL_FALSE;
        }
        else if (!strcasecmp( value, "cw")) {
          m_ps->m_enableCull = GL_TRUE;
          m_ps->m_cullFace = GL_BACK;
        }
        else if (!strcasecmp( value, "ccw")) {
          m_ps->m_enableCull = GL_TRUE;
          m_ps->m_cullFace = GL_FRONT;
        }
        else if (!strcasecmp( value, "all")) {
          m_ps->m_enableCull = GL_TRUE;
          m_ps->m_cullFace = GL_FRONT_AND_BACK;
        }
      }
#else //USE_STATE_ITEMS
      {
        if (!strcasecmp( value, "none")) {
          glDisable(GL_CULL_FACE);
        }
        else if (!strcasecmp( value, "cw")) {
          glEnable( GL_CULL_FACE);
          glCullFace( GL_BACK);
        }
        else if (!strcasecmp( value, "ccw")) {
          glEnable( GL_CULL_FACE);
          glCullFace( GL_FRONT);
        }
        else if (!strcasecmp( value, "all")) {
          glEnable( GL_CULL_FACE);
          glCullFace(GL_FRONT_AND_BACK);
        }
      }
#endif //USE_STATE_ITEMS
      break;
    case DiffuseMaterialSource:
    case EmissiveMaterialSource:
      //unsupported
      break;
    case FogColor:
    case FogDensity: 
    case FogEnable:
    case FogEnd: 
    case FogStart: 
    case FogTableMode: 
    case FogVertexMode:
      break;
    case IndexedVertexBlendEnable:
    case Lighting:
    case LocalViewer:
      //no fixed function
      break;

    case MultiSampleAntialias: 
    case MultiSampleMask:
      //no multisample in someone else's viewport
      break;

    case NormalizeNormals: 
    case PatchSegments:
      //unsupported
      break;

    case PointScaleA:
#ifdef USE_STATE_ITEMS
      if (m_ptS->m_usePointAtten) {
        //another attentuation factor is already set
        m_ptS->m_pointAtten[0] = (float)atof(value);
        m_ptS->m_getAttenA = 0;
      }
      else {
        //this is the first/only attentuation factor
        m_ptS->m_pointAtten[0] = (float)atof(value);
        m_ptS->m_getAttenA = 0;
        m_ptS->m_getAttenB = 1;
        m_ptS->m_getAttenC = 1;
        m_ptS->m_usePointAtten = 1;
      }
#endif //USE_STATE_ITEMS
      break;

    case PointScaleB: 
#ifdef USE_STATE_ITEMS
      if (m_ptS->m_usePointAtten) {
        //another attentuation factor is already set
        m_ptS->m_pointAtten[1] = (float)atof(value);
        m_ptS->m_getAttenB = 0;
      }
      else {
        //this is the first/only attentuation factor
        m_ptS->m_pointAtten[1] = (float)atof(value);
        m_ptS->m_getAttenA = 1;
        m_ptS->m_getAttenB = 0;
        m_ptS->m_getAttenC = 1;
        m_ptS->m_usePointAtten = 1;
      }
#endif //USE_STATE_ITEMS
      break;

    case PointScaleC:
#ifdef USE_STATE_ITEMS
      if (m_ptS->m_usePointAtten) {
        //another attentuation factor is already set
        m_ptS->m_pointAtten[2] = (float)atof(value);
        m_ptS->m_getAttenC = 0;
      }
      else {
        //this is the first/only attentuation factor
        m_ptS->m_pointAtten[0] = (float)atof(value);
        m_ptS->m_getAttenA = 1;
        m_ptS->m_getAttenB = 1;
        m_ptS->m_getAttenC = 0;
        m_ptS->m_usePointAtten = 1;
      }
#endif //USE_STATE_ITEMS
      break;

    case PointScaleEnable: 
#ifdef USE_STATE_ITEMS
      //TODO: this actually is never turned off in GL
#endif //USE_STATE_ITEMS
      break;

    case PointSize:
#ifdef USE_STATE_ITEMS
      m_ptS->m_usePointSize = 1;
      m_ptS->m_pointSize = (float)atof(value); 
#else //USE_STATE_ITEMS
      glPointSize( (float)atof(value));
#endif //USE_STATE_ITEMS
      break;

    case PointSizeMin:
#ifdef USE_STATE_ITEMS
      m_ptS->m_usePointSizeMin = 1;
      m_ptS->m_pointSizeMin = (float) atof(value);
#else //USE_STATE_ITEMS
      //glPointParameterf( GL_POINT_SIZE_MIN, atof(value));
#endif //USE_STATE_ITEMS
      break;

    case PointSizeMax: 
#ifdef USE_STATE_ITEMS
      m_ptS->m_usePointSizeMax = 1;
      m_ptS->m_pointSizeMax = (float) atof(value);
#else //USE_STATE_ITEMS
      //glPointParameterf( GL_POINT_SIZE_MAX, atof(value));
#endif //USE_STATE_ITEMS
      break;

    case PointSpriteEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_ptS->m_usePointSprite = 1;
        m_ptS->m_pointSprite = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_ptS->m_usePointSprite = 1;
        m_ptS->m_pointSprite = GL_FALSE;
      }
      else {
        //indeterminate value, error
        m_ptS->m_usePointSprite = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glEnable(GL_POINT_SPRITE);
      else if (isFalse(value))
        glDisable(GL_POINT_SPRITE);
      else
        ;//error
#endif //USE_STATE_ITEMS
      break;

    case RangeFogEnable: 
    case SpecularEnable: 
    case SpecularMaterialSource:
    case TweenFactor:
    case VertexBlend:
      //no fixed function
      break;
  };
  GL_CHECK;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setPixelRenderState(PixelRenderState state, int handle, const char* value) {


  //right now we track state one time, and track register values another
  //if (m_state)
  //  return;

  //pixel operations
  switch (state) {
    case PixelRenderNone:
      //is this an error?
      break;
    case AlphaBlendEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_bs->m_useEnable = 1;
        m_bs->m_enable = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_bs->m_useEnable = 1;
        m_bs->m_enable = GL_TRUE;
      }
      else {
        //bad value, just set the enable to not happen
        m_bs->m_useEnable = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glEnable( GL_BLEND);
      else if (isFalse(value))
        glDisable(GL_BLEND);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;
    case AlphaFunc:
#ifdef USE_STATE_ITEMS
      if (m_as->m_useAlphaFunc) {
        //already being set
        m_as->m_getAlphaFunc = 0;
      }
      else {
        m_as->m_getAlphaFunc = 0;
        m_as->m_getRef = 1;
        m_as->m_useAlphaFunc = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum func;
        float val;
        glGetFloatv( GL_ALPHA_TEST_REF, &val);
        func = compareFunc(value);
        glAlphaFunc( func, val);
      }
#endif //USE_STATE_ITEMS
      break;
    case AlphaRef:
#ifdef USE_STATE_ITEMS
      if (m_as->m_useAlphaFunc) {
        //already being set
        m_as->m_getRef = 0;
      }
      else {
        m_as->m_getAlphaFunc = 1;
        m_as->m_getRef = 0;
        m_as->m_useAlphaFunc = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum func;
        float val;
        glGetIntegerv( GL_ALPHA_TEST_FUNC, (GLint*)&func);
        val = (float)atof(value);
        glAlphaFunc( func, val);
      }
#endif //USE_STATE_ITEMS
      break;

    case AlphaTestEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_as->m_useEnable = 1;
        m_as->m_enable = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_as->m_useEnable = 1;
        m_as->m_enable = GL_TRUE;
      }
      else {
        //bad value, just set the enable to not happen
        m_as->m_useEnable = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glEnable( GL_ALPHA_TEST);
      else if (isFalse(value))
        glDisable(GL_ALPHA_TEST);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;

    case BlendOp:
#ifdef USE_STATE_ITEMS
      m_bs->m_useBlendOp = 1;
      m_bs->m_blendOp = blendOp(value);
      
#else //USE_STATE_ITEMS
#endif //USE_STATE_ITEMS
      break;

    case ColorWriteEnable:
#ifdef USE_STATE_ITEMS
      {
        unsigned int val = strtol(value,NULL,0);
        m_cs->m_useMask = 1;
        m_cs->m_mask[0] = (val&0x1) ? GL_TRUE : GL_FALSE;
        m_cs->m_mask[1] = (val&0x2) ? GL_TRUE : GL_FALSE;
        m_cs->m_mask[2] = (val&0x4) ? GL_TRUE : GL_FALSE;
        m_cs->m_mask[3] = (val&0x8) ? GL_TRUE : GL_FALSE;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      else if (isFalse(value))
        glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;
    case DepthBias:
#ifdef USE_STATE_ITEMS
      if (m_ps->m_usePolygonOffset) {
        m_ps->m_units = (float)atof(value);
        m_ps->m_getUnits = 0;
        if (( m_ps->m_units != 0.0f) || (m_ps->m_factor != 0.0f)) {
          m_ps->m_enablePolygonOffset = 1;
        }
        else {
          m_ps->m_enablePolygonOffset = 0;
        }
      }
      else {
        m_ps->m_units = (float)atof(value);
        m_ps->m_getUnits = 0;
        m_ps->m_getFactor = 1;
        if ( m_ps->m_units != 0.0f){
          m_ps->m_enablePolygonOffset = 1;
        }
        else {
          m_ps->m_enablePolygonOffset = 0;
        }
        m_ps->m_useEnablePolygonOffset = 1;
        m_ps->m_usePolygonOffset = 1;
      }
#endif //USE_STATE_ITEMS
      break;
    case DestBlend:
#ifdef USE_STATE_ITEMS
      if (m_bs->m_useBlend) {
        m_bs->m_getDstFactor = 0;
        m_bs->m_dstFactor = blendFactor(value);
      }
      else {
        m_bs->m_getDstFactor = 0;
        m_bs->m_getSrcFactor = 1;
        m_bs->m_dstFactor = blendFactor(value);
        m_bs->m_useBlend = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum dst,src;
        glGetIntegerv( GL_BLEND_SRC, (GLint*)&src);
        dst = blendFactor(value);
        glBlendFunc( src,dst);
      }
#endif //USE_STATE_ITEMS
      break;

    case DitherEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_cs->m_useDither = 1;
        m_cs->m_dither = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_cs->m_useDither = 1;
        m_cs->m_dither = GL_FALSE;
      }
      else {
        m_cs->m_useDither = 0;
      }
#endif //USE_STATE_ITEMS
      break;

    case FillMode:
#ifdef USE_STATE_ITEMS
      m_ps->m_usePolygonMode = 1;
      m_ps->m_polygonMode = polyMode(value);
#else //USE_STATE_ITEMS
#endif //USE_STATE_ITEMS
      break;
    case LastPixel:
      //unsupported
      break;
    case ShadeMode:
      //
      break;
    case SlopeScaleDepthBias:
#ifdef USE_STATE_ITEMS
      if (m_ps->m_usePolygonOffset) {
        m_ps->m_factor = (float)atof(value);
        m_ps->m_getFactor = 0;
        if (( m_ps->m_units != 0.0f) || (m_ps->m_factor != 0.0f)) {
          m_ps->m_enablePolygonOffset = 1;
        }
        else {
          m_ps->m_enablePolygonOffset = 0;
        }
      }
      else {
        m_ps->m_factor = (float)atof(value);
        m_ps->m_getUnits = 1;
        m_ps->m_getFactor = 0;
        if ( m_ps->m_factor != 0.0f){
          m_ps->m_enablePolygonOffset = 1;
        }
        else {
          m_ps->m_enablePolygonOffset = 0;
        }
        m_ps->m_useEnablePolygonOffset = 1;
        m_ps->m_usePolygonOffset = 1;
      }
#endif //USE_STATE_ITEMS
      break;

    case SrcBlend:
#ifdef USE_STATE_ITEMS
      if (m_bs->m_useBlend) {
        m_bs->m_getSrcFactor = 0;
        m_bs->m_srcFactor = blendFactor(value);
      }
      else {
        m_bs->m_getDstFactor = 1;
        m_bs->m_getSrcFactor = 0;
        m_bs->m_srcFactor = blendFactor(value);
        m_bs->m_useBlend = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum dst,src;
        glGetIntegerv( GL_BLEND_DST, (GLint*)&dst);
        src = blendFactor(value);
        glBlendFunc( src, dst);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_ss->m_useEnable = 1;
        m_ss->m_enable = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_ss->m_useEnable = 1;
        m_ss->m_enable = GL_FALSE;
      }
      else {
        m_ss->m_useEnable = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glEnable( GL_STENCIL_TEST);
      else if (isFalse(value))
        glDisable(GL_STENCIL_TEST);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;

    case StencilFail:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilOp) {
        m_ss->m_getStencilFail = 0;
        m_ss->m_stencilFailOp = stencilOp(value);
      }
      else {
        m_ss->m_getStencilFail = 0;
        m_ss->m_getDepthFail = 1;
        m_ss->m_getDepthPass = 1;
        m_ss->m_stencilFailOp = stencilOp(value);
        m_ss->m_useStencilOp = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum fail, pass, zfail;
        glGetIntegerv( GL_STENCIL_PASS_DEPTH_PASS, (GLint*)&pass);
        glGetIntegerv( GL_STENCIL_PASS_DEPTH_FAIL, (GLint*)&zfail);
        fail = stencilOp(value);
        glStencilOp( fail, zfail, pass);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilFunc:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilFunc) {
        m_ss->m_getStencilFunc = 0;
        m_ss->m_func = compareFunc(value);
      }
      else {
        m_ss->m_getStencilFunc = 0;
        m_ss->m_getStencilRef = 1;
        m_ss->m_getStencilRMask = 1;
        m_ss->m_func = compareFunc(value);
        m_ss->m_useStencilFunc = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum func;
        GLint ref;
        GLuint mask;
        func = compareFunc(value);
        glGetIntegerv( GL_STENCIL_REF, &ref);
        glGetIntegerv( GL_STENCIL_VALUE_MASK, (GLint*)&mask);
        glStencilFunc( func, ref, mask);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilMask:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilFunc) {
        m_ss->m_getStencilRMask = 0;
        m_ss->m_rmask = strtol(value, NULL, 0);
      }
      else {
        m_ss->m_getStencilFunc = 1;
        m_ss->m_getStencilRef = 1;
        m_ss->m_getStencilRMask = 0;
        m_ss->m_rmask = strtol(value, NULL, 0);
        m_ss->m_useStencilFunc = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum func;
        GLint ref;
        GLuint mask;
        glGetIntegerv( GL_STENCIL_REF, &ref);
        glGetIntegerv( GL_STENCIL_FUNC, (GLint*)&func);
        mask = strtol(value, NULL, 0);
        glStencilFunc( func, ref, mask);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilPass:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilOp) {
        m_ss->m_getDepthPass = 0;
        m_ss->m_depthPassOp = stencilOp(value);
      }
      else {
        m_ss->m_getStencilFail = 1;
        m_ss->m_getDepthFail = 1;
        m_ss->m_getDepthPass = 0;
        m_ss->m_depthPassOp = stencilOp(value);
        m_ss->m_useStencilOp = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum fail, pass, zfail;
        glGetIntegerv( GL_STENCIL_FAIL, (GLint*)&fail);
        glGetIntegerv( GL_STENCIL_PASS_DEPTH_FAIL, (GLint*)&zfail);
        pass = stencilOp(value);
        glStencilOp( fail, zfail, pass);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilRef:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilFunc) {
        m_ss->m_getStencilRef = 0;
        m_ss->m_ref = strtol(value, NULL, 0);
      }
      else {
        m_ss->m_getStencilFunc = 1;
        m_ss->m_getStencilRef = 0;
        m_ss->m_getStencilRMask = 1;
        m_ss->m_ref = strtol(value, NULL, 0);
        m_ss->m_useStencilFunc = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum func;
        GLint ref;
        GLuint mask;
        glGetIntegerv( GL_STENCIL_FUNC, (GLint*)&func);
        glGetIntegerv( GL_STENCIL_VALUE_MASK, (GLint*)&mask);
        ref = strtol(value, NULL, 0);
        glStencilFunc( func, ref, mask);
      }
#endif //USE_STATE_ITEMS
      break;

    case StencilWriteMask:
#ifdef USE_STATE_ITEMS
      m_ss->m_useStencilMask = 1;
      m_ss->m_mask = strtol(value, NULL, 0);
#else //USE_STATE_ITEMS
#endif //USE_STATE_ITEMS
      break;

    case StencilZFail:
#ifdef USE_STATE_ITEMS
      if (m_ss->m_useStencilOp) {
        m_ss->m_getDepthFail = 0;
        m_ss->m_depthFailOp = stencilOp(value);
      }
      else {
        m_ss->m_getStencilFail = 1;
        m_ss->m_getDepthFail = 0;
        m_ss->m_getDepthPass = 1;
        m_ss->m_depthFailOp = stencilOp(value);
        m_ss->m_useStencilOp = 1;
      }
#else //USE_STATE_ITEMS
      {
        GLenum fail, pass, zfail;
        glGetIntegerv( GL_STENCIL_PASS_DEPTH_PASS, (GLint*)&pass);
        glGetIntegerv( GL_STENCIL_FAIL, (GLint*)&fail);
        zfail = stencilOp(value);
        glStencilOp( fail, zfail, pass);
      }
#endif //USE_STATE_ITEMS
      break;

    case TextureFactor: 
    case Wrap0:
    case Wrap1:
    case Wrap2:
    case Wrap3:
    case Wrap4:
    case Wrap5:
    case Wrap6:
    case Wrap7:
    case Wrap8:
    case Wrap9:
    case Wrap10:
    case Wrap11:
    case Wrap12:
    case Wrap13:
    case Wrap14:
    case Wrap15:
      //not supported
      break;

    case ZEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_ds->m_useEnable = 1;
        m_ds->m_enable = GL_TRUE;
      }
      else if (isFalse(value)){
        m_ds->m_useEnable = 1;
        m_ds->m_enable = GL_FALSE;
      }
      else {
        m_ds->m_useEnable = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glEnable( GL_DEPTH_TEST);
      else if (isFalse(value))
        glDisable(GL_DEPTH_TEST);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;

    case ZFunc:
#ifdef USE_STATE_ITEMS
      m_ds->m_useDepthFunc = 1;
      m_ds->m_depthFunc = compareFunc(value);
#else //USE_STATE_ITEMS
      {
        GLenum func =  compareFunc(value);
        glDepthFunc( func);
      }
#endif //USE_STATE_ITEMS
      break;

    case ZWriteEnable:
#ifdef USE_STATE_ITEMS
      if (isTrue(value)) {
        m_ds->m_useDepthMask = 1;
        m_ds->m_depthMask = GL_TRUE;
      }
      else if (isFalse(value)) {
        m_ds->m_useDepthMask = 1;
        m_ds->m_depthMask = GL_FALSE;
      }
      else {
        m_ds->m_useDepthMask = 0;
      }
#else //USE_STATE_ITEMS
      if (isTrue(value))
        glDepthMask( GL_TRUE);
      else if (isFalse(value))
        glDepthMask( GL_FALSE);
      else
        ;//file is invalid
#endif //USE_STATE_ITEMS
      break;
    }
    GL_CHECK;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setSamplerState(SamplerState state, int handle, const char* value) {
  //right now we track state one time, and track register values another

  if (!m_state)
    return;

  m_state->m_fRegMap[ std::string(value)] = handle;

  //ignoring
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setVertexShaderState(VertexShaderState state, int handle, const char* value) {
  //right now we track state one time, and track register values another

  if (!m_state)
    return;

  m_state->m_vRegMap[ std::string(value)] = handle;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setPixelShaderState(PixelShaderState state, int handle, const char* value) {
  //right now we track state one time, and track register values another
  if (!m_state)
    return;

  

  m_state->m_fRegMap[ std::string(value)] = handle;
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setTextureState(TextureState state, int handle, const char* value) {
  //right now we track state one time, and track register values another

  if (m_state)
    return;

  //ignoring
}

//
//
////////////////////////////////////////////////////////////////////////////////
void stateObserver::setTransformState(TransformState state, int handle, const char* value) {
  //right now we track state one time, and track register values another
  if (m_state)
    return;

  //ignoring
}

