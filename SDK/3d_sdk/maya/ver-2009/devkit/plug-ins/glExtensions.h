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
#ifndef GL_EXTENSIONS_H
#define GL_EXTENSIONS_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#else
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_PROTOTYPE
#include <GL/glx.h>
#endif

#include <GL/glu.h>
#include "glATI.h"

/* GLSL extensions */

//no ifdefs, we want a compile failure if the aren't defined
extern PFNGLDELETEOBJECTARBPROC                     glDeleteObject;
extern PFNGLGETHANDLEARBPROC                        glGetHandle;
extern PFNGLDETACHOBJECTARBPROC                     glDetachObject;
extern PFNGLCREATESHADEROBJECTARBPROC               glCreateShaderObject;
extern PFNGLSHADERSOURCEARBPROC                     glShaderSource;
extern PFNGLCOMPILESHADERARBPROC                    glCompileShader;
extern PFNGLCREATEPROGRAMOBJECTARBPROC              glCreateProgramObject;
extern PFNGLATTACHOBJECTARBPROC                     glAttachObject;
extern PFNGLLINKPROGRAMARBPROC                      glLinkProgram;
extern PFNGLUSEPROGRAMOBJECTARBPROC                 glUseProgramObject;
extern PFNGLVALIDATEPROGRAMARBPROC                  glValidateProgram;
extern PFNGLUNIFORM1FARBPROC                        glUniform1f;
extern PFNGLUNIFORM2FARBPROC                        glUniform2f;
extern PFNGLUNIFORM3FARBPROC                        glUniform3f;
extern PFNGLUNIFORM4FARBPROC                        glUniform4f;
extern PFNGLUNIFORM1IARBPROC                        glUniform1i;
extern PFNGLUNIFORM2IARBPROC                        glUniform2i;
extern PFNGLUNIFORM3IARBPROC                        glUniform3i;
extern PFNGLUNIFORM4IARBPROC                        glUniform4i;
extern PFNGLUNIFORM1FVARBPROC                       glUniform1fv;
extern PFNGLUNIFORM2FVARBPROC                       glUniform2fv;
extern PFNGLUNIFORM3FVARBPROC                       glUniform3fv;
extern PFNGLUNIFORM4FVARBPROC                       glUniform4fv;
extern PFNGLUNIFORM1IVARBPROC                       glUniform1iv;
extern PFNGLUNIFORM2IVARBPROC                       glUniform2iv;
extern PFNGLUNIFORM3IVARBPROC                       glUniform3iv;
extern PFNGLUNIFORM4IVARBPROC                       glUniform4iv;
extern PFNGLUNIFORMMATRIX2FVARBPROC                 glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVARBPROC                 glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVARBPROC                 glUniformMatrix4fv;
extern PFNGLGETOBJECTPARAMETERFVARBPROC             glGetObjectParameterfv;
extern PFNGLGETOBJECTPARAMETERIVARBPROC             glGetObjectParameteriv;
extern PFNGLGETINFOLOGARBPROC                       glGetInfoLog;
extern PFNGLGETATTACHEDOBJECTSARBPROC               glGetAttachedObjects;
extern PFNGLGETUNIFORMLOCATIONARBPROC               glGetUniformLocation;
extern PFNGLGETACTIVEUNIFORMARBPROC                 glGetActiveUniform;
extern PFNGLGETUNIFORMFVARBPROC                     glGetUniformfv;
extern PFNGLGETUNIFORMIVARBPROC                     glGetUniformiv;
extern PFNGLGETSHADERSOURCEARBPROC                  glGetShaderSource;

extern PFNGLBINDATTRIBLOCATIONARBPROC               glBindAttribLocation;
extern PFNGLGETACTIVEATTRIBARBPROC                  glGetActiveAttrib;
extern PFNGLGETATTRIBLOCATIONARBPROC                glGetAttribLocation;
extern PFNGLVERTEXATTRIB1SARBPROC                   glVertexAttrib1s;
extern PFNGLVERTEXATTRIB1FARBPROC                   glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1DARBPROC                   glVertexAttrib1d;
extern PFNGLVERTEXATTRIB2SARBPROC                   glVertexAttrib2s;
extern PFNGLVERTEXATTRIB2FARBPROC                   glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2DARBPROC                   glVertexAttrib2d;
extern PFNGLVERTEXATTRIB3SARBPROC                   glVertexAttrib3s;
extern PFNGLVERTEXATTRIB3FARBPROC                   glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3DARBPROC                   glVertexAttrib3d;
extern PFNGLVERTEXATTRIB4SARBPROC                   glVertexAttrib4s;
extern PFNGLVERTEXATTRIB4FARBPROC                   glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4DARBPROC                   glVertexAttrib4d;
extern PFNGLVERTEXATTRIB4NUBARBPROC                 glVertexAttrib4Nub;
extern PFNGLVERTEXATTRIB1SVARBPROC                  glVertexAttrib1sv;
extern PFNGLVERTEXATTRIB1FVARBPROC                  glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB1DVARBPROC                  glVertexAttrib1dv;
extern PFNGLVERTEXATTRIB2SVARBPROC                  glVertexAttrib2sv;
extern PFNGLVERTEXATTRIB2FVARBPROC                  glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB2DVARBPROC                  glVertexAttrib2dv;
extern PFNGLVERTEXATTRIB3SVARBPROC                  glVertexAttrib3sv;
extern PFNGLVERTEXATTRIB3FVARBPROC                  glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB3DVARBPROC                  glVertexAttrib3dv;
extern PFNGLVERTEXATTRIB4SVARBPROC                  glVertexAttrib4sv;
extern PFNGLVERTEXATTRIB4FVARBPROC                  glVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4DVARBPROC                  glVertexAttrib4dv;
extern PFNGLVERTEXATTRIB4IVARBPROC                  glVertexAttrib4iv;
extern PFNGLVERTEXATTRIB4BVARBPROC glVertexAttrib4bv;
extern PFNGLVERTEXATTRIB4USVARBPROC glVertexAttrib4usv;
extern PFNGLVERTEXATTRIB4UBVARBPROC glVertexAttrib4ubv;
extern PFNGLVERTEXATTRIB4UIVARBPROC glVertexAttrib4uiv;
extern PFNGLVERTEXATTRIB4NBVARBPROC glVertexAttrib4Nbv;
extern PFNGLVERTEXATTRIB4NSVARBPROC glVertexAttrib4Nsv;
extern PFNGLVERTEXATTRIB4NIVARBPROC glVertexAttrib4Niv;
extern PFNGLVERTEXATTRIB4NUBVARBPROC glVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIB4NUSVARBPROC glVertexAttrib4Nusv;
extern PFNGLVERTEXATTRIB4NUIVARBPROC glVertexAttrib4Nuiv;
extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArray;

extern PFNGLGETVERTEXATTRIBDVARBPROC glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVARBPROC glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointerv;

/* ARB_vp/ARB_fp */
extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
extern PFNGLPROGRAMENVPARAMETER4DARBPROC glProgramEnvParameter4dARB;
extern PFNGLPROGRAMENVPARAMETER4DVARBPROC glProgramEnvParameter4dvARB;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC glProgramEnvParameter4fARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB;
extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glProgramLocalParameter4dvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;
extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC glGetProgramEnvParameterdvARB;
extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC glGetProgramEnvParameterfvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB;
extern PFNGLGETPROGRAMIVARBPROC glGetProgramivARB;
extern PFNGLGETPROGRAMSTRINGARBPROC glGetProgramStringARB;
extern PFNGLISPROGRAMARBPROC glIsProgramARB;



/* multi texture */

#ifdef LINUX
extern "C" {
extern void glActiveTexture(GLenum);
extern void glClientActiveTexture(GLenum);
extern void glMultiTexCoord2f( GLenum, GLfloat, GLfloat );	
extern void glMultiTexCoord3f( GLenum, GLfloat, GLfloat, GLfloat );
extern void glMultiTexCoord4f( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
}
#endif

#ifdef WIN32
extern PFNGLACTIVETEXTUREPROC       glActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
extern PFNGLMULTITEXCOORD1DPROC glMultiTexCoord1d;
extern PFNGLMULTITEXCOORD1DVPROC glMultiTexCoord1dv;
extern PFNGLMULTITEXCOORD1FPROC glMultiTexCoord1f;
extern PFNGLMULTITEXCOORD1FVPROC glMultiTexCoord1fv;
extern PFNGLMULTITEXCOORD1IPROC glMultiTexCoord1i;
extern PFNGLMULTITEXCOORD1IVPROC glMultiTexCoord1iv;
extern PFNGLMULTITEXCOORD1SPROC glMultiTexCoord1s;
extern PFNGLMULTITEXCOORD1SVPROC glMultiTexCoord1sv;
extern PFNGLMULTITEXCOORD2DPROC glMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2DVPROC glMultiTexCoord2dv;
extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD2FVPROC glMultiTexCoord2fv;
extern PFNGLMULTITEXCOORD2IPROC glMultiTexCoord2i;
extern PFNGLMULTITEXCOORD2IVPROC glMultiTexCoord2iv;
extern PFNGLMULTITEXCOORD2SPROC glMultiTexCoord2s;
extern PFNGLMULTITEXCOORD2SVPROC glMultiTexCoord2sv;
extern PFNGLMULTITEXCOORD3DPROC glMultiTexCoord3d;
extern PFNGLMULTITEXCOORD3DVPROC glMultiTexCoord3dv;
extern PFNGLMULTITEXCOORD3FPROC glMultiTexCoord3f;
extern PFNGLMULTITEXCOORD3FVPROC glMultiTexCoord3fv;
extern PFNGLMULTITEXCOORD3IPROC glMultiTexCoord3i;
extern PFNGLMULTITEXCOORD3IVPROC glMultiTexCoord3iv;
extern PFNGLMULTITEXCOORD3SPROC glMultiTexCoord3s;
extern PFNGLMULTITEXCOORD3SVPROC glMultiTexCoord3sv;
extern PFNGLMULTITEXCOORD4DPROC glMultiTexCoord4d;
extern PFNGLMULTITEXCOORD4DVPROC glMultiTexCoord4dv;
extern PFNGLMULTITEXCOORD4FPROC glMultiTexCoord4f;
extern PFNGLMULTITEXCOORD4FVPROC glMultiTexCoord4fv;
extern PFNGLMULTITEXCOORD4IPROC glMultiTexCoord4i;
extern PFNGLMULTITEXCOORD4IVPROC glMultiTexCoord4iv;
extern PFNGLMULTITEXCOORD4SPROC glMultiTexCoord4s;
extern PFNGLMULTITEXCOORD4SVPROC glMultiTexCoord4sv;
#endif


/* 3D texture */
#ifdef WIN32
extern PFNGLTEXIMAGE3DEXTPROC glTexImage3D;
extern PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
extern PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D;
#endif

/* Point Parameters */
extern PFNGLPOINTPARAMETERFPROC glPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC glPointParameterfv;

/* Blend Equation */
#ifdef LINUX
extern "C" void glBlendEquation(GLenum);
#endif

#ifdef WIN32
extern PFNGLBLENDEQUATIONPROC glBlendEquation;
#endif

/* Secondary Color */
extern PFNGLSECONDARYCOLOR3BPROC glSecondaryColor3b;
extern PFNGLSECONDARYCOLOR3BVPROC glSecondaryColor3bv;
extern PFNGLSECONDARYCOLOR3DPROC glSecondaryColor3d;
extern PFNGLSECONDARYCOLOR3DVPROC glSecondaryColor3dv;
extern PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;
extern PFNGLSECONDARYCOLOR3FVPROC glSecondaryColor3fv;
extern PFNGLSECONDARYCOLOR3IPROC glSecondaryColor3i;
extern PFNGLSECONDARYCOLOR3IVPROC glSecondaryColor3iv;
extern PFNGLSECONDARYCOLOR3SPROC glSecondaryColor3s;
extern PFNGLSECONDARYCOLOR3SVPROC glSecondaryColor3sv;
extern PFNGLSECONDARYCOLOR3UBPROC glSecondaryColor3ub;
extern PFNGLSECONDARYCOLOR3UBVPROC glSecondaryColor3ubv;
extern PFNGLSECONDARYCOLOR3UIPROC glSecondaryColor3ui;
extern PFNGLSECONDARYCOLOR3UIVPROC glSecondaryColor3uiv;
extern PFNGLSECONDARYCOLOR3USPROC glSecondaryColor3us;
extern PFNGLSECONDARYCOLOR3USVPROC glSecondaryColor3usv;
extern PFNGLSECONDARYCOLORPOINTERPROC glSecondaryColorPointer;

//Extension support
extern bool glShadingLanguageSupport;
extern bool glMultiTextureSupport;
extern bool glTexture3DSupport;
extern bool glAsmProgramSupport;
extern bool glPointParametersSupport;
extern bool glBlendEquationSupport;
extern bool glSecondaryColorSupport;
extern bool glTextureNonPowerOfTwo;


#ifdef CHECK_GL_ERRORS
#define GL_CHECK \
  {              \
    GLenum err;  \
  if ( (err = glGetError()) != GL_NONE) \
    printf( "GLError: %d at " __FILE__ "(%d)\n", err, __LINE__); \
  } 
#else
#define GL_CHECK glGetError();
#endif //CHECK_GL_ERRORS

bool InitializeExtensions();

extern bool extensionsInitialized;

#endif //GL_EXTENSIONS_H

