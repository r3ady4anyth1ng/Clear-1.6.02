//
// Copyright (C) 2002-2004 NVIDIA 
// 
// File: cgfxAttrDef.cpp
//
// Author: Jim Atkinson
//
// cgfxAttrDef holds the "definition" of an attribute on a cgfxShader
// node.  This definition includes all the Maya attributes plus the
// CGeffect parameter index.
//
// Changes:
//  12/2003  Kurt Harriman - www.octopusgraphics.com +1-415-893-1023
//           - Shader parameter descriptions can be queried via the
//             "-des/description" flag of cgfxShader command, together
//             with "-lp/listParameters" or "-p/parameter <name>"
//           - "-ci/caseInsensitive" option for "-p/parameter <name>"
//           - "uimin"/"uimax" annotations set numeric slider bounds.
//           - "uiname" annotation is used as parameter description 
//             if there is no "desc" annotation.
//           - Attribute bounds and initial values are updated when
//             effect is changed or reloaded.  
//           - When creating dynamic attributes for shader parameters,
//             make them keyable if type is bool, int, float or color.
//             Vector types other than colors are made keyable if
//             they have a "desc" or "uiname" annotation.
//           - Dangling references to deleted dynamic attributes
//             caused exceptions in MObject destructor, terminating
//             the Maya process.  This has been fixed.
//           - Fixed some undo/redo bugs that caused crashes and
//             incorrect rendering.  Fixed some memory leaks.
//           - Improved error handling.  Use M_CHECK for internal errors. 
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

#include <float.h>                     // FLT_MAX
#include <limits.h>                    // INT_MAX, INT_MIN
#include <string.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MFnStringData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MDGModifier.h>
#include <maya/MFnMatrixData.h>
#include <maya/MPlugArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MSelectionList.h>

#include "cgfxAttrDef.h"
#include "cgfxShaderNode.h"
#include "cgfxFindImage.h"


//
// Defines
//

#define N_MAX_STRING_LENGTH 1024


#ifdef _WIN32

#else
#	define stricmp  strcasecmp
#	define strnicmp strncasecmp
#endif


// Constructor
cgfxAttrDef::cgfxAttrDef()
: fType( kAttrTypeUnknown )
, fSize( 0 )
, fHint( kVectorHintNone )
, fNumericMin( NULL )
, fNumericMax( NULL )
, fNumericSoftMin( NULL )
, fNumericSoftMax( NULL )
, fUnits( MDistance::kInvalid )
, fNumericDef( NULL )
, fTextureId(0 )
, fTextureMonitor(kNullCallback)
// , fParameterIndex( (LPCSTR)(-1) )
, fParameterHandle(0)
, fInvertMatrix( false )
, fTransposeMatrix( false )
, fTweaked( false )
, fInitOnUndo( false )
{
};


// Destructor
cgfxAttrDef::~cgfxAttrDef()
{
	release();
	delete [] fNumericMin;
	delete [] fNumericMax;
	delete [] fNumericSoftMin;
	delete [] fNumericSoftMax;
	delete [] fNumericDef;
};


// Release any associated resources
void cgfxAttrDef::release()
{
	releaseTexture();
	releaseCallback();
}
void cgfxAttrDef::releaseTexture()
{
	if( fTextureId != 0 )
	{
		glDeleteTextures( 1, &fTextureId);
		fTextureId = 0;
	}
}
void cgfxAttrDef::releaseCallback()
{
	if( fTextureMonitor != kNullCallback)
	{
		MMessage::removeCallback( fTextureMonitor);
		fTextureMonitor = kNullCallback;
	}
}


// ================ cgfxAttrDef::setTextureType ================

// This method looks at the parameter data type, semantic, and
// annotation and determines
void cgfxAttrDef::setTextureType(CGparameter cgParameter)
{
	fType = kAttrTypeColor2DTexture;

	const char* semantic = cgGetParameterSemantic(cgParameter);

	if (!semantic) return;

	// We have to go thru semantics and annotations to find the type of the texture

	if (semantic)
	{
		if (stricmp(semantic, "normal") == 0)
		{
			fType = kAttrTypeNormalTexture;
		}
		else if (stricmp(semantic, "height") == 0)
		{
			fType = kAttrTypeBumpTexture;
		}
		else if (stricmp(semantic, "environment") == 0)
		{
			fType = kAttrTypeEnvTexture;
		}
		else if (stricmp(semantic, "environmentnormal") == 0)
		{
			fType = kAttrTypeNormalizationTexture;
		}
	}

	// Now browse through the annotations to see if there is anything
	// interesting there too.

	CGannotation cgAnnotation = cgGetFirstParameterAnnotation(cgParameter);
	while (cgAnnotation)
	{
		const char*	annotationName	= cgGetAnnotationName(cgAnnotation);
		const char* annotationValue = cgGetStringAnnotationValue(cgAnnotation);
		
		if (stricmp(annotationName, "resourcetype") == 0)
		{
			if (stricmp(annotationValue, "1d") == 0)
			{
				fType = kAttrTypeColor1DTexture;
			}
			else if (stricmp(annotationValue, "2d") == 0)
			{
				fType = kAttrTypeColor2DTexture;
			}
			else if (stricmp(annotationValue, "rect") == 0)
			{
				fType = kAttrTypeColor2DRectTexture;
			}
			else if (stricmp(annotationValue, "3d") == 0)
			{
				fType = kAttrTypeColor3DTexture;
			}
			else if (stricmp(annotationValue, "cube") == 0)
			{
				fType = kAttrTypeCubeTexture;
			}
		}
		else if (stricmp(annotationName, "resourcename") == 0)
		{
			// Store the texture file to load as the
			// string default argument.  (I know, its kind
			// of a kludge; but if the texture attributes
			// were string values, it would be exactly
			// correct.
			//
			fStringDef = annotationValue;
		}

		cgAnnotation = cgGetNextAnnotation(cgAnnotation);
	}
}

void cgfxAttrDef::setSamplerType(CGparameter cgParameter)
{
	CGstateassignment cgStateAssignment = cgGetNamedSamplerStateAssignment(cgParameter, "texture");
	setTextureType(cgGetTextureStateAssignmentValue(cgStateAssignment));
}

void cgfxAttrDef::setMatrixType(CGparameter cgParameter)
{
	fType = kAttrTypeMatrix;

	const char* semantic = cgGetParameterSemantic(cgParameter);

	if (!semantic)
		return;

	if (stricmp(semantic, "world") == 0)
	{
		fType = kAttrTypeWorldMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldinverse") == 0)
	{
		fType = kAttrTypeWorldMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldtranspose") == 0)
	{
		fType = kAttrTypeWorldMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "worldinversetranspose") == 0)
	{
		fType = kAttrTypeWorldMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "worldview") == 0)
	{
		fType = kAttrTypeWorldViewMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldviewtranspose") == 0)
	{
		fType = kAttrTypeWorldViewMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "worldviewinverse") == 0)
	{
		fType = kAttrTypeWorldViewMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldviewinversetranspose") == 0)
	{
		fType = kAttrTypeWorldViewMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "worldviewprojection") == 0)
	{
		fType = kAttrTypeWorldViewProjectionMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldviewprojectiontranspose") == 0)
	{
		fType = kAttrTypeWorldViewProjectionMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "worldviewprojectioninverse") == 0)
	{
		fType = kAttrTypeWorldViewProjectionMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "worldviewprojectioninversetranspose") == 0)
	{
		fType = kAttrTypeWorldViewProjectionMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "view") == 0)
	{
		fType = kAttrTypeViewMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "viewinverse") == 0)
	{
		fType = kAttrTypeViewMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "viewtranspose") == 0)
	{
		fType = kAttrTypeViewMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "viewinversetranspose") == 0)
	{
		fType = kAttrTypeViewMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "projection") == 0)
	{
		fType = kAttrTypeProjectionMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "projectioninverse") == 0)
	{
		fType = kAttrTypeProjectionMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = false;
	}
	else if (stricmp(semantic, "projectiontranspose") == 0)
	{
		fType = kAttrTypeProjectionMatrix;
		fInvertMatrix = false;
		fTransposeMatrix = true;
	}
	else if (stricmp(semantic, "projectioninversetranspose") == 0)
	{
		fType = kAttrTypeProjectionMatrix;
		fInvertMatrix = true;
		fTransposeMatrix = true;
	}
}

// This routine returns true if the semantic value is known to refer
// to a color value instead of a positional value.  We determine that
// this is a color value because it uses one of the known names or it
// contains the string "color" or "colour" in it.
//
void cgfxAttrDef::setVectorType(CGparameter cgParameter)
{
#if 0
	// This variable is not used
	static char* colorList[] = 
	{
		"diffuse",
		"specular",
		"ambient",
		"emissive",
	};
#endif

	const char* semantic = cgGetParameterSemantic(cgParameter);

	if ((semantic == NULL || strcmp(semantic,"") == 0) == false)
	{
		// Check the semantic value to see if this is a color
		//
		if (stricmp(semantic, "diffuse") == 0 ||
			stricmp(semantic, "specular") == 0 ||
			stricmp(semantic, "ambient") == 0 ||
			stricmp(semantic, "emissive") == 0)
		{
			fType = (fSize == 3) ? kAttrTypeColor3 : kAttrTypeColor4;
		}
		else if (stricmp(semantic, "direction") == 0)
		{
			fType = kAttrTypeFirstDir;
		}
		else if (stricmp(semantic, "position") == 0)
		{
			fType = kAttrTypeFirstPos;
		}
	}


	CGannotation cgAnnotation = cgGetFirstParameterAnnotation(cgParameter);
	while (cgAnnotation)
	{
		const char*	annotationName	= cgGetAnnotationName(cgAnnotation);
		const char* annotationValue = cgGetStringAnnotationValue(cgAnnotation);
		
		if (stricmp(annotationName, "type") == 0)
		{
			if (strlen(annotationValue) != 0)
			{
				if (stricmp(annotationValue, "color") == 0)
				{
					fType = (fSize == 3) ? kAttrTypeColor3 : kAttrTypeColor4;
				}
			}
		}
		else if (stricmp(annotationName, "space") == 0)
		{
			if (stricmp(annotationValue, "world") == 0)
			{
				fType = (cgfxAttrType)(fType + kAttrTypeWorldDir - kAttrTypeFirstDir);
			}
			else if (stricmp(annotationValue, "view") == 0 ||
				stricmp(annotationValue, "devicelightspace") == 0)
			{
				fType = (cgfxAttrType)(fType + kAttrTypeViewDir - kAttrTypeFirstDir);
			}
			else if (stricmp(annotationValue, "projection") == 0)
			{
				fType = (cgfxAttrType)(fType + kAttrTypeProjectionDir - kAttrTypeFirstDir);
			}
			else if (stricmp(annotationValue, "screen") == 0)
			{
				fType = (cgfxAttrType)(fType + kAttrTypeScreenDir - kAttrTypeFirstDir);
			}
		}
		else if (stricmp(annotationName, "object") == 0)
		{
			fHint = kVectorHintNone;

			if (stricmp(annotationValue, "dirlight") == 0)
			{
				fHint = kVectorHintDirLight;
			}
			else if (stricmp(annotationValue, "spotlight") == 0)
			{
				fHint = kVectorHintSpotLight;
			}
			else if (stricmp(annotationValue, "pointlight") == 0)
			{
				fHint = kVectorHintPointLight;
			}
			else if (stricmp(annotationValue, "camera") == 0 || stricmp(annotationValue, "eye") == 0)
			{
				fHint = kVectorHintEye;
			}
		}
		cgAnnotation = cgGetNextAnnotation(cgAnnotation);
	}
	return;
}

// ========== cgfxAttrDef::attrsFromEffect ==========
//
// This function parses through an effect and builds a list of cgfxAttrDef
// objects.
//
// The returned list and its elements are newly allocated.  The list
// initially has a refcount of 1 and is owned by the caller.  The caller
// must release() the list when no longer needed.
//
/* static */
cgfxAttrDefList* cgfxAttrDef::attrsFromEffect(CGeffect cgEffect, CGtechnique cgTechnique)
{
	if (!cgEffect)
		return NULL;

	cgfxAttrDefList* list = 0;

	try
	{
		list = new cgfxAttrDefList;     // NB: refcount is initially 1

		CGparameter cgParameter = cgGetFirstEffectParameter(cgEffect);
		int i = 0;
		while (cgParameter)
		{
			// const char* parameterName = cgGetParameterName(cgParameter);
			// bool				parameterReferenced = false;

			// Find if effect parameter is used by any program of the selected technique
			
			//if (cgTechnique == NULL) 
			//{
			//	cgTechnique = cgGetFirstTechnique(cgEffect);
			//}

			//CGpass cgPass = cgGetFirstPass(cgTechnique);
			//while (cgPass)
			//{
			//	CGstateassignment cgStateAssignmentVP = cgGetNamedStateAssignment(cgPass, "VertexProgram");
			//	CGstateassignment cgStateAssignmentFP = cgGetNamedStateAssignment(cgPass, "FragmentProgram");
			//	CGprogram					cgVertexProgram	= cgGetProgramStateAssignmentValue(cgStateAssignmentVP);
			//	CGprogram					cgFragmentProgram	= cgGetProgramStateAssignmentValue(cgStateAssignmentFP);
			//	
			//	CGparameter				cgVertexProgramParameter = cgGetNamedProgramParameter(cgVertexProgram, CG_GLOBAL, parameterName);
			//	CGparameter				cgFragmentProgramParameter = cgGetNamedProgramParameter(cgFragmentProgram, CG_GLOBAL, parameterName);

			//	if (cgVertexProgramParameter != NULL) // && cgIsParameterReferenced(cgVertexProgramParameter)) || 
			//	{
			//		parameterReferenced = true;
			//		break ;
			//	}
			//	if (cgFragmentProgramParameter != NULL) // && cgIsParameterReferenced(cgFragmentProgramParameter)))
			//	{
			//		parameterReferenced = true;
			//		break ;
			//	}
			//	cgPass = cgGetNextPass(cgPass);
			//}

			//if (!parameterReferenced)
			//{
			//	cgParameter = cgGetNextParameter(cgParameter);
			//	continue ;
			//}

			cgfxAttrDef* aDef = new cgfxAttrDef;

			aDef->fName = cgGetParameterName(cgParameter);
			aDef->fType = kAttrTypeOther;
			aDef->fSize = cgGetParameterRows(cgParameter) * cgGetParameterColumns(cgParameter);
			aDef->fParameterHandle = cgParameter;
			aDef->fSemantic = cgGetParameterSemantic(cgParameter);

			CGtype cgParameterType = cgGetParameterType(cgParameter);
			switch (cgParameterType)
			{
			case CG_BOOL	: aDef->fType = kAttrTypeBool; break ;
			case CG_INT		: aDef->fType = kAttrTypeInt; break ;
			case CG_HALF	:
			case CG_FLOAT :
#ifdef _WIN32
				if (stricmp(aDef->fSemantic.asChar() , "Time")==0)
					aDef->fType = kAttrTypeTime;
				else 
#endif
					aDef->fType = kAttrTypeFloat;
				break;
			case CG_HALF2				:
			case CG_FLOAT2			: 
				aDef->fType = kAttrTypeVector2; 
				break ;
			case CG_HALF3				:
			case CG_FLOAT3			: 
				aDef->fType = kAttrTypeVector3; 
				break ;
			case CG_HALF4				:
			case CG_FLOAT4			: 
				aDef->fType = kAttrTypeVector4; 
				break ;
			case CG_HALF4x4			:
			case CG_FLOAT4x4		: 
				aDef->setMatrixType(cgParameter); 
				break ;
			case CG_STRING			: 
				aDef->fType = kAttrTypeString; 
				break ;
			case CG_TEXTURE			:
				// handled by setSamplerType()
				break ;
			case CG_SAMPLER1D		: 
			case CG_SAMPLER2D		:
			case CG_SAMPLER3D		:
			case CG_SAMPLERRECT :
			case CG_SAMPLERCUBE :
				aDef->setSamplerType(cgParameter);
				break ;
			case CG_ARRAY				:
			case CG_STRUCT			:
				break ;

			default							: 
				MString msg = cgGetTypeString(cgParameterType);
				msg += " not yet supported";
				MGlobal::displayError(msg);
				M_CHECK( false );
			}

			if (aDef->fType == kAttrTypeVector3 || aDef->fType == kAttrTypeVector4)
			{
				// Set the specific vector type
				//
				aDef->setVectorType(cgParameter);
			}

			// Now that we know something about this attribute, walk through
			// the annotations on the parameter and see if there is any
			// additional information we can find.
			//

			MString	sUIName;

			CGannotation cgAnnotation = cgGetFirstParameterAnnotation(cgParameter);
			while (cgAnnotation)
			{
				const char* annotationName		= cgGetAnnotationName(cgAnnotation);
				const char* annotationValue		= cgGetStringAnnotationValue(cgAnnotation);
				CGtype			cgAnnotationType	= cgGetAnnotationType(cgAnnotation);

				if (stricmp(annotationName, "uihelp") == 0)
				{
					aDef->fDescription = MString(annotationValue);
				}
				else if (stricmp(annotationName, "uiname" ) == 0 )
				{
					sUIName = MString(annotationValue);
				}
				else if( stricmp( annotationName, "units") == 0)
				{
					if( stricmp( annotationValue, "inches") == 0)
					{
						aDef->fUnits = MDistance::kInches;
					}
					else if( stricmp( annotationValue, "millimetres") == 0 || stricmp( annotationValue, "millimeters") == 0 || stricmp( annotationValue, "mm") == 0)
					{
						aDef->fUnits = MDistance::kMillimeters;
					}
					else if( stricmp( annotationValue, "centimetres") == 0 || stricmp( annotationValue, "centimeters") == 0 || stricmp( annotationValue, "cm") == 0)
					{
						aDef->fUnits = MDistance::kCentimeters;
					}
					else if( stricmp( annotationValue, "metres") == 0 || stricmp( annotationValue, "meters") == 0 || stricmp( annotationValue, "m") == 0)
					{
						aDef->fUnits = MDistance::kMeters;
					}
					else if( stricmp( annotationValue, "kilometres") == 0 || stricmp( annotationValue, "kilometers") == 0 || stricmp( annotationValue, "km") == 0)
					{
						aDef->fUnits = MDistance::kKilometers;
					}
					else if( stricmp( annotationValue, "feet") == 0)
					{
						aDef->fUnits = MDistance::kFeet;
					}
				}
				else if ((aDef->fType >= kAttrTypeFirstTexture && aDef->fType <= kAttrTypeLastTexture))
				{
					if (stricmp(annotationName, "resourcetype") == 0)
					{
						if (stricmp(annotationValue, "1d") == 0)
						{
							aDef->fType = kAttrTypeColor1DTexture;
						}
						else if (stricmp(annotationValue, "2d") == 0)
						{
							aDef->fType = kAttrTypeColor2DTexture;
						}
						else if (stricmp(annotationValue, "3d") == 0)
						{
							aDef->fType = kAttrTypeColor3DTexture;
						}
						else if (stricmp(annotationValue, "cube") == 0)
						{
							aDef->fType = kAttrTypeCubeTexture;
						}
						else if (stricmp(annotationValue, "rect") == 0)
						{
							aDef->fType = kAttrTypeColor2DRectTexture;
						}
					}
					else if (stricmp(annotationName, "resourcename") == 0)
					{
						// Store the texture file to load as the
						// string default argument.  (I know, its kind
						// of a kludge; but if the texture attributes
						// were string values, it would be exactly
						// correct.
						//
						aDef->fStringDef = annotationValue;
					}
				}
				else if (cgAnnotationType == CG_BOOL )
				{}                     // no min/max for bool
				else if (stricmp(annotationName, "min") == 0 ||
					stricmp(annotationName, "max") == 0 ||
					stricmp(annotationName, "uimin") == 0 ||
					stricmp(annotationName, "uimax") == 0 )
				{
					double * tmp = new double [aDef->fSize];

					switch (cgAnnotationType)
					{
					case CG_INT:
						{
							// int* val = (int*) adesc.Value;
							// for (int k = 0; k < aDef->fSize; ++k)
							// {
							//     tmp[k] = val[k];
							// }

							int nValues;
							const int* annotationValues = cgGetIntAnnotationValues(cgAnnotation, &nValues);

							for (int iValue = 0; iValue < nValues; ++iValue)
								tmp[iValue] = static_cast<double>(annotationValues[iValue]);
						}
						break;

					case CG_FLOAT:
						{
							// float* val = (float*) adesc.Value;
							// for (int k = 0; k < aDef->fSize; ++k)
							// {
							//     tmp[k] = val[k];
							// }
							int nValues;
							const float* annotationValues = cgGetFloatAnnotationValues(cgAnnotation, &nValues);

							for (int iValue = 0; iValue < nValues; ++iValue)
								tmp[iValue] = static_cast<double>(annotationValues[iValue]);
						}
						break;

					default:
						// This is not a numeric attribute, reset tmp to NULL
						delete [] tmp;
						tmp = 0;
						break;
					}

					if (stricmp(annotationName, "min") == 0)
					{
						aDef->fNumericMin = tmp;
					}
					else if (stricmp(annotationName, "max") == 0)
					{
						aDef->fNumericMax = tmp;
					}
					else if (stricmp(annotationName, "uimin") == 0)
					{
						aDef->fNumericSoftMin = tmp;
					}
					else 
					{
						aDef->fNumericSoftMax = tmp;
					}
				} // end of if (adesc.Name == "min"|"max")

				cgAnnotation = cgGetNextAnnotation(cgAnnotation);
			}

			// Enforce limits on colors if they do not already have them.
			if ( aDef->fType == kAttrTypeColor3 || aDef->fType == kAttrTypeColor4 )
			{
				if ( !aDef->fNumericMin )
				{
					aDef->fNumericMin = new double[4];
					aDef->fNumericMin[0] = 0.0;
					aDef->fNumericMin[1] = 0.0;
					aDef->fNumericMin[2] = 0.0;
					aDef->fNumericMin[3] = 0.0;
				}
				if ( !aDef->fNumericMax )
				{
					aDef->fNumericMax = new double[4];
					aDef->fNumericMax[0] = 1.0;
					aDef->fNumericMax[1] = 1.0;
					aDef->fNumericMax[2] = 1.0;
					aDef->fNumericMax[3] = 1.0;
				}
			}

			// If no description, use UIName.
			if ( !aDef->fDescription.length() )
				aDef->fDescription = sUIName;

			// Now get the default values
			//
			double* tmp = new double [aDef->fSize];

			CGtype cgParameterBaseType = cgGetParameterBaseType(cgParameter);

			switch (cgParameterBaseType)
			{
			case CG_BOOL:
				{
					int val;
					if (cgGetParameterValueic(cgParameter, 1, &val) != 1)
					{
						delete [] tmp;
						tmp = 0;
						break;
					}

					for (int k = 0; k < aDef->fSize; ++k)
					{
						tmp[k] = val ? 1 : 0;
					}
				}
				break;

			case CG_INT:
				{
					int val;
					if (cgGetParameterValueic(cgParameter, 1, &val) != 1)
					{
						delete [] tmp;
						tmp = 0;
						break;
					}

					for (int k = 0; k < aDef->fSize; ++k)
					{
						tmp[k] = val;
					}
				}
				break;

			case CG_FLOAT:
				{
					if (aDef->fSize == 1)
					{
						float val;
						if (cgGetParameterValuefc(cgParameter, 1, &val) != 1)
						{
							delete [] tmp;
							tmp = 0;
							break;
						}

						tmp[0] = val;
					}
					else if (aDef->fSize <= 4 || aDef->fType == kAttrTypeMatrix)
					{
						float val[16];
						if (aDef->fType == kAttrTypeMatrix)
						{
							cgGetMatrixParameterfc(cgParameter, val);
						}
						else
						{
							unsigned int vecSize = aDef->fSize;
							cgGetParameterValuefc(cgParameter, vecSize, val);
						}
						/*if (result != S_OK)
						{
							delete [] tmp;
							tmp = 0;
							break;
						}*/

						for (int k = 0; k < aDef->fSize; ++k)
						{
							tmp[k] = val[k];
						}
					}
				}
				break;

			case CG_STRING:
				{
#ifdef _WIN32
					LPCSTR val;
#else
					const char* val = NULL;
#endif
					val = cgGetStringParameterValue(cgParameter);

					aDef->fStringDef = val;
				}
				// Fall through into the default case to destroy the
				// numeric default value.
				//

			default:
				// We don't know what to do but there is no point in
				// keeping tmp around.
				//
				delete [] tmp;
				tmp = 0;
			}

			// Don't save initial value if it is zero (or identity matrix).
			if ( tmp )
			{
				int k;
				if ( aDef->fSize == 16 )
				{
					const double* d = &MMatrix::identity[0][0];
					for ( k = 0; k < aDef->fSize; ++k )
						if ( tmp[ k ] != d[ k ] )
							break;
				}
				else
				{
					for ( k = 0; k < aDef->fSize; ++k )
						if ( tmp[ k ] != 0.0 )
							break;
				}
				if ( k == aDef->fSize )
				{
					delete [] tmp;
					tmp = 0;
				}
			}

			aDef->fNumericDef = tmp;

			list->add(aDef);

			cgParameter = cgGetNextParameter(cgParameter);
			++i;
		} // end of for each parameter
	}
	catch (...)
	{
		if (list)
		{
			list->release();
			list = 0;
		}
		throw;
	}

	return list;
}

// ========== cgfxAttrDef::attrsFromNode ==========
//
// This function simply returns the parses through the dynamic
// attributes on an effect and builds a list of cgfxAttrDef objects.
// The cgfxAttrDef objects in the list are incomplete but they are
// only used to determine which attributes on the object need to be
// created, destroyed, or left alone.  Ultimately, the cgfxAttrDefList
// that is constructed from the effect itself will be the one held by
// the node.
//
// The returned list is owned by the node.  If the caller intends to
// retain the list after the node has released it, then the caller must 
// do AddRef()/release().  
//
/* static */
cgfxAttrDefList* cgfxAttrDef::attrsFromNode(MObject& oNode)
{
	MStatus status;
	cgfxAttrDefList* list = 0;

	MFnDependencyNode fnNode(oNode, &status);
	M_CHECK( status );
	M_CHECK( fnNode.typeId() == cgfxShaderNode::sId );

	cgfxShaderNode* pNode = (cgfxShaderNode *) fnNode.userNode();
	M_CHECK( pNode );

	list = pNode->attrDefList();

	// The list has not been initialized.  Create it and try again.
	//
	if (!list)
	{
		buildAttrDefList(oNode);
		list = pNode->attrDefList();
	}

	return list;
}

// ========== cgfxAttrDef::buildAttrDefList ==========
//
// This routine reconstructs the attrDefList from stringArray value in
// the attributeList attribute.  The reconstructed list is incomplete
// but it is good enough to compare to the list generated by
// attrsFromEffect to see if the connections are still valid.
//
void cgfxAttrDef::buildAttrDefList(MObject& oNode)
{
	MStatus status;
	cgfxAttrDefList* list = 0;

	try
	{
		MFnDependencyNode fnNode(oNode, &status);
		M_CHECK( status &&
			fnNode.typeId() == cgfxShaderNode::sId );

		cgfxShaderNode* pNode = (cgfxShaderNode *) fnNode.userNode();
		M_CHECK( pNode &&
			!pNode->attrDefList() );

		list = new cgfxAttrDefList;

		MStringArray saList;

		pNode->getAttributeList(saList);
		//         MStatus status;

		//         // Get the value of the attributeList attribute
		//         //
		//         MPlug    plug(oNode, cgfxShaderNode::sAttributeList);

		//         MObject saDataObject;

		//         status = plug.getValue(saDataObject);
		//         if (!status)
		//         {
		//             sprintf(errorMsg, "%s(%d): failed to get attributeList value: %s!",
		//                     __FILE__, __LINE__, status.errorString().asChar());
		//             throw errorMsg;
		//         }

		//         MFnStringArrayData fnSaData(saDataObject, &status);
		//         if (!status)
		//         {
		//             sprintf(errorMsg,
		//                     "%s(%d): failed to construct attributeList function set: %s!",
		//                     __FILE__, __LINE__, status.errorString().asChar());
		//             throw errorMsg;
		//         }

		//         fnSaData.copyTo(saList);

		// Ok, we succeeded, saList is now an array of "top level"
		// dynamic attribute names along with some minimal type
		// information.  Parse through it and reconstruct the
		// cgfxAttrDefList.
		//
		unsigned int i;
		for (i = 0; i < saList.length(); ++i)
		{
			MString item = saList[i];
			MStringArray  splitItem;
			MObject oAttr;

			item.split('\t', splitItem);

			cgfxAttrDef* attrDef = attrFromNode( fnNode,
				splitItem[0], 
				(cgfxAttrType)(splitItem[1].asInt()) );
			if ( attrDef )
			{
				attrDef->fDescription = splitItem[2];
				attrDef->fSemantic    = splitItem[3];
				list->add( attrDef );
			}
		}

		pNode->setAttrDefList(list);
	}
	catch (...)
	{
		if (list)
		{
			list->release();
			list = 0;
		}
		throw;
	}

	// We are now done with the list and can release it.  The
	// node, however is still holding it.
	list->release();
}


/* static */
cgfxAttrDef*
cgfxAttrDef::attrFromNode( const MFnDependencyNode& fnNode,
													const MString&           sAttrName,
													cgfxAttrType             eAttrType )
{
	MStatus      status;
	cgfxAttrDef* attrDef = NULL;

	try
	{
		MObject obNode = fnNode.object();
		MObject obAttr = fnNode.attribute( sAttrName, &status );
		if ( !status )                 // if node doesn't have this attr
			return NULL;               // skip it

		attrDef = new cgfxAttrDef;

		attrDef->fName = sAttrName;
		attrDef->fType = eAttrType;
		attrDef->fAttr = obAttr;

		MFnCompoundAttribute fnCompound;
		MFnNumericAttribute  fnNumeric;
		MFnTypedAttribute    fnTyped;

		MPlug   plug( obNode, obAttr );
		double  numericMin[4];
		double  numericMax[4];
		double  numericValue[4];
		bool    hasMin    = false;
		bool    hasMax    = false; 
		bool    isNumeric = false; 

		// If compound attribute, get value and bounds of each element.
		if ( fnCompound.setObject( obAttr ) )
		{
			hasMin = true;
			hasMax = true;
			isNumeric = true;

			MObject      obChild;
			MStringArray saChild;
			int iChild;
			int nChild = fnCompound.numChildren();
			M_CHECK( nChild >= 2 && nChild <= 3 );
			for ( iChild = 0; iChild < nChild; ++iChild )
			{
				// Get child attribute.
				if ( iChild < 3 )
				{
					obChild = fnCompound.child( iChild, &status );
					M_CHECK( status );
				}

				status = fnNumeric.setObject( obChild );
				M_CHECK( status );

				// Min
				if ( fnNumeric.hasMin() )
					fnNumeric.getMin( numericMin[ iChild ] );
				else
					hasMin = false; 

				// Max
				if ( fnNumeric.hasMax() )
					fnNumeric.getMax( numericMax[ iChild ] );
				else
					hasMax = false; 

				// Value
				MPlug plChild( obNode, obChild );
				status = plChild.getValue( numericValue[ iChild ] );
				M_CHECK( status );

				// Check for 4-element vector.
				saChild.append( fnNumeric.name() );
				if ( iChild == 2 )
				{
					const char* suffix = NULL;
					if ( saChild[0] == sAttrName + "X" &&
						saChild[1] == sAttrName + "Y" &&
						saChild[2] == sAttrName + "Z" ) 
						suffix = "W";
					else if ( saChild[0] == sAttrName + "R" &&
						saChild[1] == sAttrName + "G" &&
						saChild[2] == sAttrName + "B" ) 
						suffix = "Alpha";
					if ( suffix )
					{
						MString sName2 = sAttrName + suffix;
						obChild = fnNode.attribute( sName2, &status );
						MFnNumericData::Type ndt = fnNumeric.unitType();
						if ( status &&
							fnNumeric.setObject( obChild ) &&
							fnNumeric.unitType() == ndt )
						{
							attrDef->fAttr2 = obChild;
							nChild = 4;     // loop again to get extra attr
						}
					}
				}
			}                          // loop over children
			attrDef->fSize = nChild;
		}                              // compound 

		// Simple numeric attribute?  
		else if ( fnNumeric.setObject( obAttr ) )
		{
			MFnNumericAttribute fnNumeric( obAttr, &status );
			M_CHECK( status );

			attrDef->fSize = 1;
			isNumeric = true;

			// Get min and max.
			if ( fnNumeric.hasMin() )
			{
				fnNumeric.getMin( numericMin[0] );
				hasMin = true;
			}
			if ( fnNumeric.hasMax() )
			{
				fnNumeric.getMax( numericMax[0] );
				hasMax = true;
			}

			// Get slider bounds.
			if ( fnNumeric.hasSoftMin() )
			{
				attrDef->fNumericSoftMin = new double[1];
				fnNumeric.getSoftMin( attrDef->fNumericSoftMin[0] );
			}
			if ( fnNumeric.hasSoftMax() )
			{
				attrDef->fNumericSoftMax = new double[1];
				fnNumeric.getSoftMax( attrDef->fNumericSoftMax[0] );
			}

			// Value 
			status = plug.getValue( numericValue[0] );
			M_CHECK( status );
		}                              // simple numeric

		// String attribute?
		else if ( fnTyped.setObject( obAttr ) &&
			fnTyped.attrType() == MFnData::kString )
		{
			attrDef->fSize = 1;
			status = plug.getValue( attrDef->fStringDef );
			M_CHECK( status );
		}                              // string

		// Matrix attribute?
		else if ( fnTyped.setObject( obAttr ) &&
			fnTyped.attrType() == MFnData::kMatrix )
		{
			MObject obData;
			status = plug.getValue( obData );
			M_CHECK( status );
			MFnMatrixData fnMatrixData( obData, &status );
			M_CHECK( status );
			const MMatrix& mat = fnMatrixData.matrix( &status );
			M_CHECK( status );
			attrDef->fSize = 16;
			attrDef->fNumericDef = new double[ attrDef->fSize ];
			const double* p = &mat.matrix[0][0];
			for ( int i = 0; i < 16; ++i )
				attrDef->fNumericDef[ i ] = p[ i ];
			M_CHECK( status );
		}                              // matrix 

		// Mystified...
		else
			M_CHECK( false );

		// Store numeric value, min and max.
		if ( isNumeric )
		{
			attrDef->fNumericDef = new double[ attrDef->fSize ];
			memcpy( attrDef->fNumericDef, numericValue, attrDef->fSize * sizeof( numericValue[0] ) );
			if ( hasMin )
			{
				attrDef->fNumericMin = new double[ attrDef->fSize ];
				memcpy( attrDef->fNumericMin, numericMin, attrDef->fSize * sizeof( numericMin[0] ) );
			}
			if ( hasMax )
			{
				attrDef->fNumericMax = new double[ attrDef->fSize ];
				memcpy( attrDef->fNumericMax, numericMax, attrDef->fSize * sizeof( numericMax[0] ) );
			}
		}
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		size_t ee = (size_t)e;
		MString sMsg = "(";
		sMsg += (int)ee;
		sMsg += ") cgfxShader node \"";
		sMsg += fnNode.name();
		sMsg += "\" has invalid attribute \"";
		sMsg += sAttrName;
		sMsg += "\" - ignored";
		MGlobal::displayWarning( sMsg );
		delete attrDef;
		attrDef = NULL; 
	}
	catch (...)
	{
		delete attrDef;
		M_CHECK( false );
	}

	return attrDef;
}                                      // cgfxAttrDef::attrFromNode


bool
cgfxAttrDef::createAttribute( const MObject& oNode, MDGModifier* mod, cgfxShaderNode* pNode)
{
	MFnDependencyNode fnNode( oNode );

	// Return if node already has an attribute with the specified name.
	// (Shader var name could conflict with a predefined static attr.)
	MObject obExistingAttr = fnNode.attribute( fName );
	if ( !obExistingAttr.isNull() )
	{
		return false;
	}

	try
	{
		MStatus status;
		MObject oAttr, oAttr2;

		MFnNumericAttribute     nAttr;
		MFnTypedAttribute       tAttr;
		MFnMatrixAttribute      mAttr;

		MObject oSrcNode, oDstNode;
		MFnDependencyNode fnFile;
		MObject oSrcAttr, oDstAttr;

		bool doConnection = false;

		switch (fType)
		{
		case kAttrTypeBool:
			oAttr = nAttr.create( fName, fName, MFnNumericData::kBoolean,
				0.0, &status );
			M_CHECK( status );
			nAttr.setKeyable( true );
			break;

		case kAttrTypeInt:
			oAttr = nAttr.create( fName, fName, MFnNumericData::kInt,
				0.0, &status );
			M_CHECK( status );
			nAttr.setKeyable( true );
			break;

		case kAttrTypeFloat:
			oAttr = nAttr.create( fName, fName, MFnNumericData::kFloat,
				0.0, &status );
			M_CHECK( status );
			nAttr.setKeyable( true );
			break;

		case kAttrTypeString:
			oAttr = tAttr.create(fName, fName, MFnData::kString,
				MObject::kNullObj, &status );
			M_CHECK( status );
			break;

		case kAttrTypeVector2:
		case kAttrTypeVector3:
		case kAttrTypeVector4:
		case kAttrTypeColor3:
		case kAttrTypeColor4:

		case kAttrTypeObjectDir:
		case kAttrTypeWorldDir:
		case kAttrTypeViewDir:
		case kAttrTypeProjectionDir:
		case kAttrTypeScreenDir:

		case kAttrTypeObjectPos:
		case kAttrTypeWorldPos:
		case kAttrTypeViewPos:
		case kAttrTypeProjectionPos:
		case kAttrTypeScreenPos:
			{
				const char** suffixes = compoundAttrSuffixes( fType );
				MString      sChild;
				MObject      oaChildren[4];
				M_CHECK( fSize <= 4 );
				for ( int iChild = 0; iChild < fSize; ++iChild )
				{
					const char* suffix = suffixes[ iChild ];
					sChild = fName + suffix;
					oaChildren[ iChild ] = nAttr.create( sChild,
						sChild,
						MFnNumericData::kFloat,
						0.0,
						&status );
					M_CHECK( status );
				}

				if ( fSize == 4 )
				{
					oAttr2 = oaChildren[3];
					if ( fType == kAttrTypeColor3 ||
						fType == kAttrTypeColor4 ||
						fDescription.length() > 0 )
						nAttr.setKeyable( true );
				}

				oAttr = nAttr.create( fName,
					fName,
					oaChildren[0],
					oaChildren[1],
					oaChildren[2], 
					&status );
				M_CHECK( status );

				if ( fType == kAttrTypeColor3 ||
					fType == kAttrTypeColor4 )
				{
					nAttr.setKeyable( true );
					nAttr.setUsedAsColor( true );
				}
				else if ( fDescription.length() > 0 )
					nAttr.setKeyable( true );

				break;
			}

		case kAttrTypeMatrix:
			// Create a generic matrix
			//
			oAttr = mAttr.create(fName, fName,
				MFnMatrixAttribute::kFloat, &status );
			M_CHECK( status );
			break;

		case kAttrTypeWorldMatrix:
		case kAttrTypeViewMatrix:
		case kAttrTypeProjectionMatrix:
		case kAttrTypeWorldViewMatrix:
		case kAttrTypeWorldViewProjectionMatrix:
			// These matricies are handled internally and have no attribute.
			//
			break;

		case kAttrTypeColor1DTexture:
		case kAttrTypeColor2DTexture:
		case kAttrTypeColor3DTexture:
		case kAttrTypeColor2DRectTexture:
		case kAttrTypeNormalTexture:
		case kAttrTypeBumpTexture:
		case kAttrTypeCubeTexture:
		case kAttrTypeEnvTexture:
		case kAttrTypeNormalizationTexture:
			if( pNode->getTexturesByName())
			{
				oAttr = tAttr.create(fName, fName, MFnData::kString,
					MObject::kNullObj, &status );
				M_CHECK( status );
			}
			else
			{
				const char* suffix1 = "R";
				const char* suffix2 = "G";
				const char* suffix3 = "B";

				MObject oChild1 = nAttr.create(fName + suffix1,
					fName + suffix1,
					MFnNumericData::kFloat,
					0.0, &status);
				MObject oChild2 = nAttr.create(fName + suffix2,
					fName + suffix2,
					MFnNumericData::kFloat,
					0.0, &status);

				MObject oChild3 = nAttr.create(fName + suffix3,
					fName + suffix3,
					MFnNumericData::kFloat,
					0.0, &status);

				oAttr = nAttr.create( fName,
					fName,
					oChild1,
					oChild2,
					oChild3, 
					&status );
				M_CHECK( status );

				// Although it's not strictly necessary, set this attribute
				// to be a color so the user can will at least get the
				// texture assignment button in the AE if for some reason
				// our AE template is missing
				nAttr.setUsedAsColor( true );
			}
			break;
#ifdef _WIN32
		case kAttrTypeTime:
#endif
		case kAttrTypeOther:
			break;
		default:
			M_CHECK( false );
		}

		if (oAttr.isNull())
		{
			// There is no attribute for this parameter
			//
			return false;
		}

		// Add the attribute to the node
		//
		status = mod->addAttribute( oNode, oAttr );
		M_CHECK( status );

		if (!oAttr2.isNull())
		{
			status = mod->addAttribute( oNode, oAttr2 );
			M_CHECK( status );
		}

		// Hold onto a copy of the attribute for easy access later.
		fAttr  = oAttr;
		fAttr2 = oAttr2;

		// If we need to connect this node to some other node, do so.
		//
		if (doConnection)
		{
			status = mod->connect(oSrcNode, oSrcAttr, oDstNode, oDstAttr);
			M_CHECK( status );
		}
		return true;                   // success
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		size_t ee = (size_t)e;
		fType = kAttrTypeUnknown;
		MString sMsg = "(";
		sMsg += (int)ee;
		sMsg += ") cgfxShader node \"";
		sMsg += fnNode.name();
		sMsg += "\": unable to add attribute \"";
		sMsg += fName;
		sMsg += "\"";
		MGlobal::displayWarning( sMsg );
		return false;                  // failure
	}
	catch (...)
	{
		M_CHECK( false );
	}

	return true;
}                                      // cgfxAttrDef::createAttribute


bool
cgfxAttrDef::destroyAttribute( MObject& oNode, MDGModifier* dgMod)
{
	MStatus status;

	// If this is a texture node, clear the value (which will destroy
	// any attached textures)
	if( fType >= kAttrTypeFirstTexture && fType <= kAttrTypeLastTexture)
		setTexture( oNode, "", dgMod);

	// New effect won't need this old attr anymore.  
	status = dgMod->removeAttribute( oNode, fAttr );

	// If there is a secondary attribute, remove that too.
	if ( !fAttr2.isNull() )
		status = dgMod->removeAttribute( oNode, fAttr2 );

	// Don't leave dangling references to deleted attributes.
	fAttr  = MObject::kNullObj;  
	fAttr2 = MObject::kNullObj;  

	return status == MStatus::kSuccess;
}                                      // cgfxAttrDef::destroyAttribute




// ========== cgfxAttrDef::updateNode ==========
//
// This routine takes a node and an effect and ensures that all those
// and only those attributes that should be on the node, are on the
// node.
//
// The output cgfxAttrDefList and its elements are newly 
// allocated.  The list initially has a refcount of 1 and 
// will be owned by the caller.  The caller must release() 
// the list when no longer needed.
//
/* static */
void
cgfxAttrDef::updateNode( CGeffect effect,              // IN
												cgfxShaderNode*   pNode,               // IN
												MDGModifier*      dgMod,               // UPD
												cgfxAttrDefList*& effectList,          // OUT
												MStringArray&     attributeList )      // OUT
{
	MStatus status;

	effectList = NULL;

	try
	{
		MObject           oNode = pNode->thisMObject();
		MFnDependencyNode fnNode( oNode );
		MFnAttribute      fnAttr;

		effectList = attrsFromEffect( effect, cgGetNamedTechnique(effect, pNode->getTechnique().asChar()) );  // caller will own this list
		cgfxAttrDefList* nodeList = attrsFromNode( oNode ); // oNode owns this one

		cgfxAttrDefList::iterator emIt;
		cgfxAttrDefList::iterator nmIt;

		cgfxAttrDef* adef;

		// Walk through the nodeList.  Delete each attribute that is not
		// also found in the effect list.
		//
		for (nmIt = nodeList->begin(); nmIt; ++nmIt)
		{                              // loop over nodeList
			adef = (*nmIt);

			// Skip if node doesn't have this attribute.
			if ( adef->fAttr.isNull() )
				continue;

			// Look for a matching attribute in the effect
			emIt = effectList->find( adef->fName );

			// Drop Maya attribute from node if shader var
			//   was declared in old effect, but not declared
			//   in new effect, or data type is not the same.
			if ( !emIt ||
				(*emIt)->fType != adef->fType )
			{
				adef->destroyAttribute( oNode, dgMod);
			}

			// If this is a texture and it has a non-default
			// value, we should switch to this mode of texture
			// definition (names or nodes)
			else if( emIt && 
				(*emIt)->fType >= kAttrTypeFirstTexture &&
				(*emIt)->fType <= kAttrTypeLastTexture)
			{
				// Get the attribute type of the existing attribute
				// and ensure our node is setup to digest the correct
				// type of textures
				MFnTypedAttribute typedFn( adef->fAttr);
				bool usesName = typedFn.attrType() == MFnData::kString;
				pNode->setTexturesByName( usesName);

				// Finally, if this texture uses fileTexture nodes then
				// mark the value as "tweaked" to prevent the default value 
				// code from trying to attach a default fileTexture node. 
				// This is crucial to being able to load shaders back in. The
				// Maya file will create our node using the following steps:
				//	1) Create an empty cgfxShader node
				//	2) Create all the dynamic attributes (like this texture)
				//	3) Set the effect attribute (which calls this code)
				//	4) Create DG connections to file texture nodes
				// So, if we did setup a default file texture node, the load
				// would be unable to connect the real file texture. 
				//
				if( !usesName)
					(*emIt)->fTweaked = true;
			}


		}                              // loop over nodeList

		// Delete any unnecessary attributes before starting to add new ones
		// (in case we're deleting and re-creating a property with a different
		// type)
		// 
		dgMod->doIt();

		// Walk through the effectList.  Add each item that is not also
		// found in the node list.
		//
		for (emIt = effectList->begin(); emIt; ++emIt)
		{                              // loop over effectList 
			adef = (*emIt);

			// Note: nodeList::find will work with a null this pointer
			// so nodeList->find() will work even if nodeList is NULL.
			//
			nmIt = nodeList->find( adef->fName );

			// Double check that the attr still exists.  Get current value.
			cgfxAttrDef* cdef = NULL;
			if ( nmIt &&
				!(*nmIt)->fAttr.isNull() )
				cdef = attrFromNode( fnNode,
				(*nmIt)->fName, 
				(*nmIt)->fType );

			// Add new Maya attribute.
			if ( !cdef )
				adef->createAttribute( oNode, dgMod, pNode );

			// Go on with existing attribute.
			else
			{                          // use existing attribute
				// Copy the attribute handles to the new effect's cgfxAttrDef.
				adef->fAttr  = (*nmIt)->fAttr;
				adef->fAttr2 = (*nmIt)->fAttr2;

				// Did we already notice that the user has set this attr?
				if ( (*nmIt)->fTweaked )
					adef->fTweaked = true;

				// If no old effect, then the current values were
				//   loaded from the scene file, and should override 
				//   the new effect's defaults if they are different.
				else if ( !pNode->effect() )
				{
					if ( !adef->isInitialValueEqual( *cdef ) )
						adef->fTweaked = true;
				}

				// If current value is not the same as old effect's 
				//   default, then user has adjusted it.  Current value
				//   takes precedence over the new effect's default.
				else if ( !(*nmIt)->isInitialValueEqual( *cdef ) )
					adef->fTweaked = true;

				// User hasn't changed this value.  New effect's
				// default takes precedence.  Since we are going to
				// change the value, remember to change it back 
				// to the old effect's default in case of undo.
				// Among other things, this logic allows the UI shader
				// description to update as different effects are chosen.
				else
					(*nmIt)->fInitOnUndo = true;

				delete cdef;
			}                          // use existing attribute
		}                              // loop over effectList 

		// Now rebuild the attributeList attribute value.  This is an array
		// of strings of the format "attrName<TAB>type<TAB>Description<TAB>Semantic".
		//
		MString tmpStr;

		attributeList.clear();

		cgfxAttrDefList::iterator it(effectList);

		while (it)
		{
			cgfxAttrDef* aDef = *it;

			tmpStr = aDef->fName;
			tmpStr += "\t";
			tmpStr += (int)aDef->fType;
			tmpStr += "\t";
			tmpStr += aDef->fDescription;
			tmpStr += "\t";
			tmpStr += aDef->fSemantic;

			// Drop trailing tabs.
			const char* bp = tmpStr.asChar();
			const char* ep;
			for ( ep = bp + tmpStr.length(); bp < ep; --ep )
				if ( ep[-1] != '\t' )
					break;

			attributeList.append( MString( bp, (int)(ep - bp) ) );
			++it;
		}
	}
	catch ( cgfxShaderCommon::InternalError* )   
	{
		if ( effectList )
			effectList->release();    
		effectList = NULL;
		throw;
	}
	catch (...)
	{
		if ( effectList )
			effectList->release();    
		effectList = NULL;
		M_CHECK( false );
	}
}                                      // cgfxAttrDef::updateNode


// Return true if initial value of 'this' is same as 'that'.
bool
cgfxAttrDef::isInitialValueEqual( const cgfxAttrDef& that ) const
{
	if ( fStringDef != that.fStringDef )
		return false;

	const double* thisNumericDef = fNumericDef;
	const double* thatNumericDef = that.fNumericDef;

	if ( thisNumericDef == thatNumericDef ) 
		return true;

	// Make sure we don't proceed to test default colour values for
	// texture attributes as the colour itself is meaningless!
	//
	if( fType == that.fType && fType >= kAttrTypeFirstTexture && fType <= kAttrTypeLastTexture)
		return true;

	if ( !thisNumericDef )
	{
		thisNumericDef = thatNumericDef;
		thatNumericDef = fNumericDef;
	}

	if ( !thatNumericDef )
	{
		if ( fType == kAttrTypeMatrix )
		{
			thatNumericDef = &MMatrix::identity.matrix[0][0];
			M_CHECK( fSize == 16 && that.fSize == 16 );
		}
		else
		{
			static const double d0[4] = {0.0, 0.0, 0.0, 0.0};
			thatNumericDef = d0;
			M_CHECK( fSize <= sizeof(d0)/sizeof(d0[0]) );
			if ( fSize != that.fSize )
				MGlobal::displayWarning( "CgFX attribute size mismatch" );
		}
	}

	double eps = 0.0001;
	int i;
	for ( i = 0; i < fSize; ++i )
		if ( thisNumericDef[ i ] + eps < thatNumericDef[ i ] ||
			thatNumericDef[ i ] + eps < thisNumericDef[ i ] )
			return false;

	return true;
}                                      // cgfxAttrDef::isInitialValueEqual


// Copy initial value from given attribute.
void
cgfxAttrDef::setInitialValue( const cgfxAttrDef& from )
{
	if ( from.fNumericDef )
	{
		M_CHECK( fSize == from.fSize );
		if ( !fNumericDef )
			fNumericDef = new double[ fSize ];
		memcpy( fNumericDef, from.fNumericDef, fSize * sizeof( *fNumericDef ) );
	}
	else
	{
		delete fNumericDef;
		fStringDef = from.fStringDef;
	}
}                                      // cgfxAttrDef::setInitialValue


// Set Maya attributes to their initial values.
/* static */
void
cgfxAttrDef::initializeAttributes( MObject& oNode, cgfxAttrDefList* list, bool bUndoing, MDGModifier* dgMod )
{
	MStatus             status;
	MFnMatrixAttribute  fnMatrix;
	MFnNumericAttribute fnNumeric;
	MFnTypedAttribute   fnTyped;
	for ( cgfxAttrDefList::iterator it( list ); it; ++it )
	{                                  // loop over cgfxAttrDefList
		cgfxAttrDef* aDef = (*it);

		if ( aDef->fAttr.isNull() )    // if no Maya attr
			continue;                  // try next

		bool bSetValue = bUndoing ? aDef->fInitOnUndo
			: !aDef->fTweaked;

		try
		{
			// Boolean
			if ( aDef->fType == kAttrTypeBool )
			{
				if ( bSetValue )
					aDef->setValue( oNode, aDef->fNumericDef && aDef->fNumericDef[0] );
			}

			// Texture node: must be check before both numeric (as a
			// texture could be a float3 colour) and string (as it
			// could also be a string)
			else if( aDef->fType >= kAttrTypeFirstTexture && 
				aDef->fType <= kAttrTypeLastTexture)
			{
				if ( bSetValue )
					aDef->setTexture( oNode, aDef->fStringDef, dgMod);
			}

			// Numeric
			else if ( fnNumeric.setObject( aDef->fAttr ) ) 
			{                          // numeric attr

				// Constants for removing old bounds...
				double vMin = -FLT_MAX;
				double vMax = FLT_MAX;
				if ( aDef->fType == kAttrTypeInt )
				{
					vMin = INT_MIN;
					vMax = INT_MAX;
				}

				// Set or remove bounds.

				switch ( aDef->fSize )
				{                      // switch to set/remove bounds
				case 1:
					if ( aDef->fNumericMin )
						fnNumeric.setMin( aDef->fNumericMin[0] );
					else if ( fnNumeric.hasMin() )
						fnNumeric.setMin( vMin );

					if ( aDef->fNumericMax )
						fnNumeric.setMax( aDef->fNumericMax[0] );
					else if ( fnNumeric.hasMax() )
						fnNumeric.setMax( vMax );

					if ( aDef->fNumericSoftMin )
						fnNumeric.setSoftMin( aDef->fNumericSoftMin[0] );
					else if ( fnNumeric.hasSoftMin() )
						fnNumeric.setSoftMin( vMin );

					if ( aDef->fNumericSoftMax )
						fnNumeric.setSoftMax( aDef->fNumericSoftMax[0] );
					else if ( fnNumeric.hasSoftMax() )
						fnNumeric.setSoftMax( vMax );
					break;

				case 2:
					if ( aDef->fNumericMin )
						fnNumeric.setMin( aDef->fNumericMin[0], 
						aDef->fNumericMin[1] );
					else if ( fnNumeric.hasMin() )
						fnNumeric.setMin( vMin, vMin );

					if ( aDef->fNumericMax )
						fnNumeric.setMax( aDef->fNumericMax[0], 
						aDef->fNumericMax[1] );
					else if ( fnNumeric.hasMax() )
						fnNumeric.setMax( vMax, vMax );
					break;

				case 3:
				case 4:
					if ( aDef->fNumericMin )
						fnNumeric.setMin( aDef->fNumericMin[0], 
						aDef->fNumericMin[1],
						aDef->fNumericMin[2] );
					else if ( fnNumeric.hasMin() )
						fnNumeric.setMin( vMin, vMin, vMin );

					if ( aDef->fNumericMax )
						fnNumeric.setMax( aDef->fNumericMax[0], 
						aDef->fNumericMax[1],
						aDef->fNumericMax[2] );
					else if ( fnNumeric.hasMax() )
						fnNumeric.setMax( vMax, vMax, vMax );
					break;

				default:
					M_CHECK( false );
				}                      // switch to set/remove bounds

				// Set initial value.
				//   Use 0 if no initial value specified in .fx file.
				if ( bSetValue )
				{                      // set numeric initial value
					static const double d0[4] = {0.0, 0.0, 0.0, 0.0};
					const double* pNumericDef = aDef->fNumericDef ? aDef->fNumericDef
						: d0;
					switch ( aDef->fSize )
					{                  
					case 1:
						if ( aDef->fType == kAttrTypeInt )
							aDef->setValue( oNode, (int)pNumericDef[0] );
						else
							aDef->setValue( oNode, (float)pNumericDef[0] );
						break;

					case 2:
						aDef->setValue( oNode,
							(float)pNumericDef[0],
							(float)pNumericDef[1] );
						break;

					case 3:
						aDef->setValue( oNode,
							(float)pNumericDef[0],
							(float)pNumericDef[1],
							(float)pNumericDef[2] );
						break;

					case 4:
						status = fnNumeric.setObject( aDef->fAttr2 );
						M_CHECK( status );

						if ( aDef->fNumericMin )
							fnNumeric.setMin( aDef->fNumericMin[3] );
						else if ( fnNumeric.hasMin() )
							fnNumeric.setMin( vMin );

						if ( aDef->fNumericMax )
							fnNumeric.setMax( aDef->fNumericMax[3] );
						else if ( fnNumeric.hasMax() )
							fnNumeric.setMax( vMax );

						aDef->setValue( oNode,
							(float)pNumericDef[0],
							(float)pNumericDef[1],
							(float)pNumericDef[2],
							(float)pNumericDef[3] );
						break;

					default:
						M_CHECK( false );
					}                  // switch ( aDef->fSize )
				}                      // set numeric initial value
			}                          // numeric attr

			// String
			else if ( fnTyped.setObject( aDef->fAttr ) &&
				fnTyped.attrType() == MFnData::kString )
			{
				if ( bSetValue )
					aDef->setValue( oNode, aDef->fStringDef );
			}

			// Matrix
			else if ( fnMatrix.setObject( aDef->fAttr ) )
			{
				if ( !bSetValue )
				{}
				else if ( aDef->fNumericDef )
				{
					MMatrix m;
					double* p = &m.matrix[0][0];
					for ( int k = 0; k < 16; ++k )
						p[ k ] = aDef->fNumericDef[ k ];
					aDef->setValue( oNode, m );
				}
				else
					aDef->setValue( oNode, MMatrix::identity );
			}
		}
		catch ( cgfxShaderCommon::InternalError* e )   
		{
			size_t ee = (size_t)e;
			MFnDependencyNode fnNode( oNode );
			MString sMsg = "(";
			sMsg += (int)ee;
			sMsg += ") cgfxShader node \"";
			sMsg += fnNode.name();
			sMsg += "\": unable to initialize attribute \"";
			sMsg += aDef->fName;
			sMsg += "\"";
			MGlobal::displayWarning( sMsg );
		}
	}                                  // loop over cgfxAttrDefList
}                                      // cgfxAttrDef::initializeAttributes



// Clear all Maya attribute references in a cgfxAttrDefList.
//   This should be called whenever the list is detached from 
//   the cgfxShader node, to avert an eventual exception in 
//   MObject::~MObject() in case the referenced Maya attribute 
//   happens to be deleted while the cgfxAttrDefList is in
//   suspense on Maya's undo queue.
//
/* static */
void cgfxAttrDef::purgeMObjectCache( cgfxAttrDefList* list )
{                                       
	cgfxAttrDefList::iterator it( list );
	for ( ; it; ++it )
	{
		cgfxAttrDef* aDef = (*it);
		aDef->fAttr  = MObject::kNullObj;
		aDef->fAttr2 = MObject::kNullObj;
	}
}                                      // cgfxAttrDef::purgeMObjectCache


// Refresh Maya attribute references in a cgfxAttrDefList.
//   This should be called whenever a saved list is re-attached
//   to the cgfxShader node in the course of undo or redo.
//
/* static */
void cgfxAttrDef::validateMObjectCache( const MObject&   obCgfxShader, 
																			 cgfxAttrDefList* list )
{                                       
	MStatus status;
	MString sName2;
	MFnDependencyNode fnNode( obCgfxShader, &status );
	cgfxAttrDefList::iterator it( list );
	for ( ; it; ++it )
	{
		cgfxAttrDef* aDef = (*it);
		aDef->fAttr = fnNode.attribute( aDef->fName, &status );

		// 4-element vectors use an extra attribute for the 4th element.
		const char* suffix = aDef->getExtraAttrSuffix();
		if ( suffix )
			aDef->fAttr2 = fnNode.attribute( aDef->fName + suffix, &status );
	}
}                                      // cgfxAttrDef::validateMObjectCache


// Return suffix for Color4/Vector4 extra attribute, or NULL.
const char*
cgfxAttrDef::getExtraAttrSuffix() const
{
	if ( fSize == 4 )
		return compoundAttrSuffixes( fType )[ 3 ];
	return NULL;
}                                      // cgfxAttrDef::getExtraAttrSuffix


// Get a string representation of a cgfxAttrType
//
/* static */
const char* cgfxAttrDef::typeName( cgfxAttrType type )
{
#define CASE(name) case kAttrType##name: return #name

	switch (type)
	{
	default:    // Fall through into case unknown
		CASE(Unknown);
		CASE(Bool);
		CASE(Int);
		CASE(Float);
		CASE(String);
		CASE(Vector2);
		CASE(Vector3);
		CASE(Vector4);
		CASE(ObjectDir);
		CASE(WorldDir);
		CASE(ViewDir);
		CASE(ProjectionDir);
		CASE(ScreenDir);
		CASE(ObjectPos);
		CASE(WorldPos);
		CASE(ViewPos);
		CASE(ProjectionPos);
		CASE(ScreenPos);
		CASE(Color3);
		CASE(Color4);
		CASE(Matrix);
		CASE(WorldMatrix);
		CASE(ViewMatrix);
		CASE(ProjectionMatrix);
		CASE(WorldViewMatrix);
		CASE(WorldViewProjectionMatrix);
		CASE(Color1DTexture);
		CASE(Color2DTexture);
		CASE(Color3DTexture);
		CASE(Color2DRectTexture);
		CASE(NormalTexture);
		CASE(BumpTexture);
		CASE(CubeTexture);
		CASE(EnvTexture);
		CASE(NormalizationTexture);
#ifdef _WIN32
		CASE(Time);
#endif
		CASE(Other);
	}
}


/* static */
const char**
cgfxAttrDef::compoundAttrSuffixes( cgfxAttrType eAttrType )
{
	static const char* simple[] = { NULL, NULL, NULL, NULL,    NULL };
	static const char* vector[] = { "X",  "Y",  "Z",  "W",     NULL };
	static const char* color[]  = { "R",  "G",  "B",  "Alpha", NULL };

	const char** p;
	switch ( eAttrType )
	{
	case kAttrTypeVector2:
	case kAttrTypeVector3:
	case kAttrTypeVector4:
	case kAttrTypeObjectDir:
	case kAttrTypeWorldDir:
	case kAttrTypeViewDir:
	case kAttrTypeProjectionDir:
	case kAttrTypeScreenDir:
	case kAttrTypeObjectPos:
	case kAttrTypeWorldPos:
	case kAttrTypeViewPos:
	case kAttrTypeProjectionPos:
	case kAttrTypeScreenPos:
		p = vector;
		break;
	case kAttrTypeColor3:
	case kAttrTypeColor4:
		p = color;
		break;
	default:
		p = simple;
		break;
	}
	return p;
}                                      // cgfxAttrDef::compoundAttrSuffixes



// Methods to get attribute values
//
void
cgfxAttrDef::getValue( MObject& oNode, bool& value ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.getValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::getValue( MObject& oNode, int& value ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.getValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::getValue( MObject& oNode, float& value ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.getValue(value);
	if( fUnits != MDistance::kInvalid)
	{
		value = (float)MDistance( value, fUnits).as( MDistance::internalUnit());
	}
	M_CHECK( status );
}

void
cgfxAttrDef::getValue( MObject& oNode, MString& value ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.getValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::getValue( MObject& oNode, float& v1, float& v2 ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	MObject oData;
	status = plug.getValue(oData);
	M_CHECK( status );

	MFnNumericData fnData(oData, &status);
	M_CHECK( status );

	status = fnData.getData(v1, v2);
	M_CHECK( status );

	if( fUnits != MDistance::kInvalid)
	{
		v1 = (float)MDistance( v1, fUnits).as( MDistance::internalUnit());
		v2 = (float)MDistance( v2, fUnits).as( MDistance::internalUnit());
	}
}

void
cgfxAttrDef::getValue( MObject& oNode,
											float& v1, float& v2, float& v3 ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	MObject oData;
	status = plug.getValue(oData);
	M_CHECK( status );

	MFnNumericData fnData(oData, &status);
	M_CHECK( status );

	status = fnData.getData(v1, v2, v3);
	M_CHECK( status );

	if( fUnits != MDistance::kInvalid)
	{
		v1 = (float)MDistance( v1, fUnits).as( MDistance::internalUnit());
		v2 = (float)MDistance( v2, fUnits).as( MDistance::internalUnit());
		v3 = (float)MDistance( v3, fUnits).as( MDistance::internalUnit());
	}
}

void
cgfxAttrDef::getValue( MObject& oNode,
											float& v1, float& v2, float& v3, float& v4 ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	MPlug plug2(oNode, fAttr2);

	MObject oData;
	status = plug.getValue(oData);
	M_CHECK( status );

	MFnNumericData fnData(oData, &status);
	M_CHECK( status );

	status = fnData.getData(v1, v2, v3);
	M_CHECK( status );

	// Get the 4th value from the extra attribute.
	//
	status = plug2.getValue(v4);
	M_CHECK( status );

	if( fUnits != MDistance::kInvalid)
	{
		v1 = (float)MDistance( v1, fUnits).as( MDistance::internalUnit());
		v2 = (float)MDistance( v2, fUnits).as( MDistance::internalUnit());
		v3 = (float)MDistance( v3, fUnits).as( MDistance::internalUnit());
		v4 = (float)MDistance( v4, fUnits).as( MDistance::internalUnit());
	}
}

void
cgfxAttrDef::getValue( MObject& oNode, MMatrix& value ) const
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	MObject oData;
	status = plug.getValue(oData);
	M_CHECK( status );

	MFnMatrixData fnData(oData, &status);
	M_CHECK( status );

	value = fnData.matrix(&status);
	M_CHECK( status );
}

void
cgfxAttrDef::getValue( MObject& oNode, MImage& value ) const
{
	MStatus status = MS::kFailure;
	MPlug plug(oNode, fAttr);

	if (fType >= kAttrTypeFirstTexture &&
		fType <= kAttrTypeLastTexture)
	{
		MPlugArray plugArray;
		plug.connectedTo(plugArray, true, false, &status);
		M_CHECK( status );

		if (plugArray.length() != 1)
			M_CHECK( status );

		MPlug srcPlug = plugArray[0];
		MObject oSrcNode = srcPlug.node();

		// OutputDebugStrings("Source texture object = ", oSrcNode.apiTypeStr());

		value.release();
		status = value.readFromTextureNode(oSrcNode);
		M_CHECK( status );
	}
	M_CHECK( status );
}


// Get the source of an attribute value
//
void
cgfxAttrDef::getSource( MObject& oNode, MPlug& src) const
{
	MStatus status = MS::kFailure;
	MPlug plug(oNode, fAttr);

	MPlugArray plugArray;
	plug.connectedTo(plugArray, true, false, &status);
	M_CHECK( status && plugArray.length() <= 1);

	if (plugArray.length() == 1)
		src = plugArray[0];
}


// Methods to set attribute values
//
void
cgfxAttrDef::setValue( MObject& oNode, bool value )
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.setValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, int value )
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.setValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, float value )
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.setValue(value);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, const MString& value )
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	status = plug.setValue((MString &)value);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, float v1, float v2 )
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	MFnNumericData fnData;
	MObject oData = fnData.create(MFnNumericData::k2Float, &status);
	M_CHECK( status );

	fnData.setData(v1, v2);
	status = plug.setValue(oData);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, float v1, float v2, float v3 )
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	MFnNumericData fnData;
	MObject oData = fnData.create(MFnNumericData::k3Float, &status);
	M_CHECK( status );

	fnData.setData(v1, v2, v3);
	status = plug.setValue(oData);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, float v1, float v2, float v3, float v4 )
{
	MStatus status;
	MPlug plug(oNode, fAttr);
	MPlug plug2(oNode, fAttr2);

	MFnNumericData fnData;
	MObject oData = fnData.create(MFnNumericData::k3Float, &status);
	M_CHECK( status );

	fnData.setData(v1, v2, v3);
	status = plug.setValue(oData);
	M_CHECK( status );

	status = plug2.setValue(v4);
	M_CHECK( status );
}

void
cgfxAttrDef::setValue( MObject& oNode, const MMatrix& v )
{
	MStatus       status;
	MFnMatrixData fnData;
	MObject       oData = fnData.create( v, &status );
	M_CHECK( status );

	MPlug plug( oNode, fAttr );
	status = plug.setValue( oData );
	M_CHECK( status );
}

// Utility to check if a node is used by any nodes other than us
//
bool isUsedElsewhere( MObject node, MObject user)
{
	for( MItDependencyGraph iter( node); !iter.isDone(); iter.next())
	{
		// If there is a downstream connection to something other than our shader ...
		//
		if( iter.thisNode() != node)
		{
			if( iter.thisNode() != user)
			{
				// And that connection uses anything other than the message attribute
				// of the texture (which is used to connect the texture to the 
				// global texture list)
				//
				MPlugArray src;
				iter.thisPlug().connectedTo( src, true, false);
				if( src.length() == 1 && src[ 0].partialName() != "msg")
				{
					// Finally, check this isn't just the swatch renderer taking
					// a quick look at this node
					//
					MFnDependencyNode dgFn( iter.thisNode());
					if( dgFn.name() != "swatchShadingGroup")
					{
						// Then we're not the only user of this node!
						//
						//cout<<"Not removing node due to connection<<src[0].name().asChar()<<" to "<<iter.thisPlug().name().asChar()<<endl;
						return true;
					}
				}
			}

			// If this downstream connection is to another shader, or a
			// message connection, don't follow the connection any further
			//
			iter.prune();
		}
	}
	return false;
}


void
cgfxAttrDef::setTexture( MObject& oNode, const MString& value, MDGModifier*	dgMod)
{
	MStatus status;
	MPlug plug(oNode, fAttr);

	// Is this a node or name based texture?
	//
	MFnAttribute attrFn( fAttr);
	if( attrFn.isUsedAsColor() )
	{
		// Node based texture.
		// Remove any existing texture
		//
		if( plug.isConnected())
		{
			MPlugArray src;
			plug.connectedTo( src, true, false, &status);
			M_CHECK( status );
			if( src.length() > 0)
			{
				MObject textureNode = src[ 0].node();

				// If no other nodes use this texture, we can remove it to 
				// avoid cluttering up the scene with unused texture nodes
				//
				if( !isUsedElsewhere( textureNode, oNode))
				{
					// We are the only user of this texture node so we
					// can delete it. Before we do that though, are
					// we the only user of the placement node too?
					//
					MFnDependencyNode textureFn( textureNode);
					MPlug uvPlug = textureFn.findPlug( "uv", &status);
					if( status == MS::kSuccess)
					{
						MPlugArray placementNode;
						uvPlug.connectedTo( placementNode, true, false, &status);
						M_CHECK( status );

						// If we are the only user of the placement node, delete 
						// it as well
						//
						if( placementNode.length() > 0 &&
							!isUsedElsewhere( placementNode[ 0].node(), textureNode))
							M_CHECK( dgMod->deleteNode( placementNode[ 0].node()) );
					}

					// Delete the texture node
					//
					M_CHECK( dgMod->deleteNode( textureNode) );
				}
				else
				{
					// We're not deleting the texture, so just disconnect it
					//
					M_CHECK( dgMod->disconnect( src[ 0], plug) );
				}
			}
		}

		// Do we have a (default) value to set?
		//
		if( value.length() > 0)
		{
			// Resolve the texture value as either an absolute or
			// project relative path. Even though we re-resolve paths
			// when loading textures ourselves, if we want the Maya
			// texture swatch to display, Maya needs to be able to
			// find the texture as well.
			//
			MString relativePath = cgfxFindFile( value, true );

			// If we didn't find it, just leave the original path (even
			// though it wont work)
			//
			if( relativePath.length() == 0) relativePath = value;

			// Create a new file texture and placement node
			// Use the MEL commands (as opposed to dgMod.createNode) as these
			// correctly hook up the rendering message connections so our 
			// nodes show up in the hypershade etc.
			//
			MObject textureNode, placementNode;
			MSelectionList originalSelection, newlyCreatedNode;
			MGlobal::getActiveSelectionList( originalSelection);
			MGlobal::executeCommand( "shadingNode -asTexture file", false, true);
			MGlobal::getActiveSelectionList( newlyCreatedNode);
			M_CHECK( newlyCreatedNode.length() > 0 && newlyCreatedNode.getDependNode( 0, textureNode));
			MGlobal::executeCommand( "shadingNode -asUtility place2dTexture", false, true);
			MGlobal::getActiveSelectionList( newlyCreatedNode);
			M_CHECK( newlyCreatedNode.length() > 0 && newlyCreatedNode.getDependNode( 0, placementNode));
			MGlobal::setActiveSelectionList( originalSelection);
			MFnDependencyNode fileTextureFn( textureNode, &status);
			M_CHECK( status );
			MFnDependencyNode placementFn( placementNode, &status);
			M_CHECK( status );

			// Connect the placement node to the file texture node
			//
			M_CHECK( dgMod->connect( placementFn.findPlug( "coverage"), fileTextureFn.findPlug( "coverage")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "translateFrame"), fileTextureFn.findPlug( "translateFrame")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "rotateFrame"), fileTextureFn.findPlug( "rotateFrame")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "mirrorU"), fileTextureFn.findPlug( "mirrorU")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "mirrorV"), fileTextureFn.findPlug( "mirrorV")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "stagger"), fileTextureFn.findPlug( "stagger")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "wrapU"), fileTextureFn.findPlug( "wrapU")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "wrapV"), fileTextureFn.findPlug( "wrapV")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "repeatUV"), fileTextureFn.findPlug( "repeatUV")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "offset"), fileTextureFn.findPlug( "offset")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "rotateUV"), fileTextureFn.findPlug( "rotateUV")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "noiseUV"), fileTextureFn.findPlug( "noiseUV")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "vertexUvOne"), fileTextureFn.findPlug( "vertexUvOne")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "vertexUvTwo"), fileTextureFn.findPlug( "vertexUvTwo")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "vertexUvThree"), fileTextureFn.findPlug( "vertexUvThree")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "vertexCameraOne"), fileTextureFn.findPlug( "vertexCameraOne")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "outUV"), fileTextureFn.findPlug( "uv")) );
			M_CHECK( dgMod->connect( placementFn.findPlug( "outUvFilterSize"), fileTextureFn.findPlug( "uvFilterSize")) );

			// Connect our file texture node to our shader attribute, then set the texture
			//
			M_CHECK( dgMod->connect( fileTextureFn.findPlug( "outColor"), plug) );
			status = fileTextureFn.findPlug( "fileTextureName").setValue( relativePath);
			M_CHECK( status );
		}

		M_CHECK( dgMod->doIt() );
	}
	else
	{
		// Simple String attribute - but do a safety check to be sure
		//
		MFnTypedAttribute fnTyped;
		if( fnTyped.setObject( fAttr ) &&
			fnTyped.attrType() == MFnData::kString )
		{
			status = plug.setValue((MString &)value);
			M_CHECK( status );
		}
	}
}


//--------------------------------------------------------------------//
//                          cgfxAttrDefList                           //
//--------------------------------------------------------------------//

cgfxAttrDefList::iterator
cgfxAttrDefList::findInsensitive( const MString& name )
{
	const char* pName = name.asChar();
	unsigned    lName = name.length();
	iterator    it( this );
	for ( ; it; ++it )
		if ( lName == (*it)->fName.length() &&
			0 == stricmp( pName, (*it)->fName.asChar() ) )
			break;
	return it;
};                                     // cgfxAttrDefList::findInsensitive

void cgfxAttrDefList::release()
{
	--refcount;
	if (refcount <= 0)
	{
		delete this;
	}
	// Need to go through and dereference / delete textures.
	else
	{
		iterator it(this);
		while (it)
		{
			(*it)->release();
			++it;
		}
	}
};
