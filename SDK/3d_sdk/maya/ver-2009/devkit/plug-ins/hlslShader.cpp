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
#ifndef HLSL_VERSION  
#define HLSL_VERSION  "1.0"
#endif
#define ERROR_LIMIT 20
//#define VALIDATE_TECHNIQUE_LIST
//#define VALIDATE_TECHNIQUES

// Now, because we're rendering from system memory, DirectX can start to get
// suspicious that the graphics driver has hung if we try and render too much
// stuff in a single call. So, we'll take stability over performance and
// split up the draw. Given we only do this for large amounts of geometry, the
// hit shouldn't be too bad. Commenting this #define out will cause the plugin
// to try and render everything in a single render call, no matter how large 
// it is (which seems to work on some graphics cards, but not others)
#define MAX_SEGMENTED_BATCH_SIZE 40000


#include "hlslShader.h"

#include <maya/MGlobal.h>
#include <maya/MGeometry.h>
#include <maya/MGeometryData.h>
#include <maya/MGeometryPrimitive.h>
#include <maya/MGeometryList.h>
#include <maya/MGeometryManager.h>
#include <maya/MVaryingParameterList.h>
#include <maya/MUniformParameter.h>
#include <maya/MGlobal.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MSceneMessage.h>
#include <maya/MFileIO.h>
#include <maya/MMatrix.h>
#include <maya/MFileObject.h>

#include <maya/MD3D9Renderer.h>
#include <maya/MHwrCallback.h>
#include <stdio.h>
#include <new.h> // for fancy pants re-initialisation of existing memory version

// For swatch rendering
#include <maya/MHardwareRenderer.h>
#include <maya/MHWShaderSwatchGenerator.h>
#include <maya/MGeometryRequirements.h>
#include <maya/MHwTextureManager.h>
#include <maya/MImageFileInfo.h>

// Debugging
#define DEBUG_TEXTURE_CACHE(X) 

template <class T> class ObjArray
{
public:
	inline		ObjArray() : fLength( 0), fCapacity( 0), fData( NULL) {}
	inline		~ObjArray() { if( fData) { destroy( 0, fLength); delete[] ((char*)fData); } } // Don't call the destructors here, destroy did it!
	inline int	length() const { return fLength; }
	inline void	length( int l) { if( l > fLength) { if( l > fCapacity) resize( l); create( fLength, l); } else if( l < fLength) destroy( l, fLength); fLength = l; }
	inline const T& operator[]( int i) const { return fData[ i]; }
	inline T&	operator[]( int i) { return fData[ i]; }
private:
	inline void create( int s, int e) { for( ; s < e; s++) new ((void*)(fData + s)) T; }
	inline void destroy( int s, int e) { for( ; s < e; s++) (fData + s)->T::~T(); }
	enum
	{
		MIN_ARRAY_SIZE = 4
	};
	inline void resize( int l) 
	{ 
		if( l < MIN_ARRAY_SIZE) l = MIN_ARRAY_SIZE; 
		else if( l < fCapacity * 2) l = fCapacity * 2; 
		T* newData = (T*)new char[ l * sizeof( T)]; 
		if( fData) 
		{ 
			memcpy( newData, fData, fCapacity * sizeof( T)); 
			delete[] ((char*)fData); 
		} 
		fData = newData; 
		fCapacity = l; 
	}
	int			fLength;
	int			fCapacity;
	T*			fData;
};

// setup device callbacks as needed
// currently this class defined in hlslShader.h
//
void hlslDeviceManager::deviceNew()
{
	fState = kValid;
	//printf("calling setShader with name: %s\n", fShader.fShader.asChar() );	
	fShader.setShader( fShader.fShader);
}

void hlslDeviceManager::deviceLost()
{
	fState = kInvalid;
	//printf("devLost: releasing %s handles\n", fShader.fShader.asChar() );
	fShader.release();
}
void hlslDeviceManager::deviceDeleted()
{
	fState = kInvalid;
	//printf("devDeleted: releasing %s handles\n", fShader.fShader.asChar() );
	fShader.release();
}

void hlslDeviceManager::deviceReset()
{
	fState = kReset;
	//printf("devRest: releasing %s handles\n", fShader.fShader.asChar() );
}

void hlslDeviceManager::resetShader()
{
	fState = kValid;
	fShader.setShader( fShader.fShader);
	//printf("resetShader: releasing %s handles\n", fShader.fShader.asChar() );
}


//
// Lookup tables for the (default) names and internal semantics
// that map to D3D's usage types
//
struct SemanticInfo
{
	const char* Name;
	MVaryingParameter::MVaryingParameterSemantic Type;
	int			MinElements;
	int			MaxElements;
	BYTE		D3DType;
};

SemanticInfo gSemanticInfo[] = 
{ 
	"Position",				MVaryingParameter::kPosition,		2, 4,		D3DDECLTYPE_FLOAT3, 	
	"BlendWeight",			MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4,
	"BlendIndices",			MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4, 
	"Normal",				MVaryingParameter::kNormal,			3, 4,		D3DDECLTYPE_FLOAT3, 
	"PointSize",			MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT3, 
	"UV",					MVaryingParameter::kTexCoord,		2, 4,		D3DDECLTYPE_FLOAT2,
	"Tangent",				MVaryingParameter::kTangent,		3, 4,		D3DDECLTYPE_FLOAT3, 
	"BiNormal",				MVaryingParameter::kBinormal,		3, 4,		D3DDECLTYPE_FLOAT3, 
	"TesselateFactor",		MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4, 
	"PositionTransformed",	MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4, 
	"Color",				MVaryingParameter::kColor,			3, 4,		D3DDECLTYPE_FLOAT4, 
	"Fog",					MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4, 
	"Depth",				MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4, 
	"Sample",				MVaryingParameter::kNoSemantic,		1, 4,		D3DDECLTYPE_FLOAT4,
};


//
// A wee state manager used to flip from DX to GL culling handed-ness
//
class HLSLStateManager : public ID3DXEffectStateManager
{
public:
	static HLSLStateManager sInstance;
	inline HLSLStateManager() : fShapeCullMode( MGeometryList::kCullCW), fShaderCullMode( D3DCULL_CCW) {}
	
	// Set the cull mode Maya needs
	inline void shapeCullMode( MGeometryList::MCullMode cullMode, bool ignoreShaderCullMode = false) 
	{ 
		fShapeCullMode = cullMode; 
		if( ignoreShaderCullMode) fShaderSetCullMode = false;
		setupCulling(); 
	}

	// Reverse the cull direction if we need "normal" OpenGL culling as DirectX
	// has the opposite "handedness" to GL
	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE d3dRenderState, DWORD dwValue ) 
	{ 
		if( d3dRenderState == D3DRS_CULLMODE)
		{
			fShaderCullMode = dwValue;
			fShaderSetCullMode = true;
			return D3D_OK;
		}
		return fD3DDevice->SetRenderState( d3dRenderState, dwValue ); 
	}

	// Helper to setup a pass tracking whether cull direction was explicitly set, and reversing it if it was
	inline void BeginPass( LPD3DXEFFECT effect, int pass) 
	{ 
		fShaderSetCullMode = false;
		effect->BeginPass( pass); 
		setupCulling();
	}

	// Default implementations
	STDMETHOD(SetSamplerState)(THIS_ DWORD dwStage, D3DSAMPLERSTATETYPE d3dSamplerState, DWORD dwValue ) { return fD3DDevice->SetSamplerState( dwStage, d3dSamplerState, dwValue ); }
	STDMETHOD(SetTextureStageState)(THIS_ DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTextureStageState, DWORD dwValue ) { return fD3DDevice->SetTextureStageState( dwStage, d3dTextureStageState, dwValue ); }
	STDMETHOD(SetTexture)(THIS_ DWORD dwStage, LPDIRECT3DBASETEXTURE9 pTexture ) { return fD3DDevice->SetTexture( dwStage, pTexture ); }
	STDMETHOD(SetVertexShader)(THIS_ LPDIRECT3DVERTEXSHADER9 pShader ) { return fD3DDevice->SetVertexShader( pShader ); }
	STDMETHOD(SetPixelShader)(THIS_ LPDIRECT3DPIXELSHADER9 pShader ) { return fD3DDevice->SetPixelShader( pShader ); }
	STDMETHOD(SetFVF)(THIS_ DWORD dwFVF ) { return fD3DDevice->SetFVF( dwFVF ); }
	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix ) { return fD3DDevice->SetTransform( State, pMatrix ); }
	STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9 *pMaterial ) { return fD3DDevice->SetMaterial( pMaterial ); }
	STDMETHOD(SetLight)(THIS_ DWORD Index, CONST D3DLIGHT9 *pLight ) { return fD3DDevice->SetLight( Index, pLight ); }
	STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable ) { return fD3DDevice->LightEnable( Index, Enable ); }
	STDMETHOD(SetNPatchMode)(THIS_ FLOAT NumSegments ) { return fD3DDevice->SetNPatchMode( NumSegments ); }
	STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetVertexShaderConstantF( RegisterIndex, pConstantData, RegisterCount ); }
	STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetVertexShaderConstantI( RegisterIndex, pConstantData, RegisterCount ); }
	STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetVertexShaderConstantB( RegisterIndex, pConstantData, RegisterCount ); }
	STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetPixelShaderConstantF( RegisterIndex, pConstantData, RegisterCount ); }
	STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetPixelShaderConstantI( RegisterIndex, pConstantData, RegisterCount ); }
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount ) { return fD3DDevice->SetPixelShaderConstantB( RegisterIndex, pConstantData, RegisterCount ); }
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) { if (iid == IID_IUnknown || iid == IID_ID3DXEffectStateManager) { *ppv = static_cast<ID3DXEffectStateManager*>(this); } else { *ppv = NULL; return E_NOINTERFACE; } reinterpret_cast<IUnknown*>(this)->AddRef(); return S_OK; }
	STDMETHOD_(ULONG, AddRef)(THIS) { /*return (ULONG)InterlockedIncrement( &m_lRef ); */ return 1L; }
	STDMETHOD_(ULONG, Release)(THIS) { /*if( 0L == InterlockedDecrement( &m_lRef ) ) { delete this; return 0L; } return m_lRef; */ return 1L; }
	LPDIRECT3DDEVICE9 fD3DDevice;
protected:

	// Helper to work out the render cull setting for the current shape/shader cull modes
	// This code assumes that a pass render state selecting a cull direction OVERRIDES any
	// double-sidedness on the shape (given there's really no point in rendering a back or
	// front of the object when every point is part of the front and the back). You can
	// easily modify this code to preserve double sidedness in these cases by adding a
	// check for fShapeCullMode == kCullNone in the second two fShaderCullMode cases and
	// leave d3dCullMode as NONE to get the other behaviour.
	inline void setupCulling()
	{
		DWORD d3dCullMode = D3DCULL_NONE;
		if( !fShaderSetCullMode)
		{
			if( fShapeCullMode == MGeometryList::kCullCW)
				d3dCullMode = D3DCULL_CW; 
			else if( fShapeCullMode == MGeometryList::kCullCCW)
				d3dCullMode = D3DCULL_CCW; 
		}
		else if( fShaderCullMode == D3DCULL_CCW)
		{
			d3dCullMode = (fShapeCullMode == MGeometryList::kCullCCW) ? D3DCULL_CCW : D3DCULL_CW; 
		}
		else if( fShaderCullMode == D3DCULL_CW)
		{
			d3dCullMode = (fShapeCullMode == MGeometryList::kCullCCW) ? D3DCULL_CW : D3DCULL_CCW; 
		}
		fD3DDevice->SetRenderState( D3DRS_CULLMODE, d3dCullMode); 
	}

	MGeometryList::MCullMode fShapeCullMode;
	DWORD		fShaderCullMode;
	bool		fShaderSetCullMode;
};
HLSLStateManager HLSLStateManager::sInstance;


//
// A wee texture cache for our HLSL shaders
//
class HLSLTextureManager : public MHwrCallback
{
public:
	static HLSLTextureManager sInstance;

	HLSLTextureManager() 
	{ 
		addCallback( this); 
		sceneCallbackIds[0] = MSceneMessage::addCallback( MSceneMessage::kBeforeNew, HLSLTextureManager::newSceneCallback, (void*)this); 
		sceneCallbackIds[1] = MSceneMessage::addCallback( MSceneMessage::kBeforeOpen, HLSLTextureManager::newSceneCallback, (void*)this); 
	}
	virtual ~HLSLTextureManager() { removeCallback( this); release(); for( int i = 0; i < 2; i++) MSceneMessage::removeCallback( sceneCallbackIds[ i]); }

	void bind( const MString& name, MUniformParameter& uniform, LPD3DXEFFECT d3dEffect, D3DXHANDLE d3dParameter)
	{
		// Try and find an existing entry for this parameter
		User* user = NULL;
		for( int i = users.length(); i--; )
		{
			if( users[ i].parameter == d3dParameter && users[ i].effect == d3dEffect)
			{
				user = &users[ i];
				break;
			}
		}

		// If there's no existing entry and we have a texture, create a new one
		if( !user && name.length() > 0)
		{
			int idx = users.length();
			users.length( idx + 1);
			user = &users[ idx];
			DEBUG_TEXTURE_CACHE( printf( "Registering user %s at %d\n", uniform.name().asChar(), (int)((long long)user)); )
			user->parameter = d3dParameter;
			user->effect = d3dEffect;
			user->uniform = uniform;
		}

		// If there is already an entry for this parameter ...
		if( user && user->texture)
		{
			// ... is it using the right texture?
			Texture* texture = user->texture;
			if( texture->name == name)
			{
				// Same texture means reload!
				DEBUG_TEXTURE_CACHE( printf( "Reloading %s\n", name.asChar()); )
				if( texture->texture)
				{
					texture->texture->Release();
					texture->texture = NULL;

					// Now, we're going to be a little nasty here. If we have multiple shaders attached
					// to this texture, they're all going to be trying to reload this texture, and given
					// we don't pay attention to which frame the reload was performed in, we'll reload
					// the texture once for every user. To avoid this, the easiest solution is simply to
					// mark all the other users dirty (to make sure they pick up the new texture, and 
					// then delete their usage entries in our cache, so we see them as new users requesting
					// the texture and don't reload the texture multiple times.
					for( int i = users.length(); i--; )
					{
						if( &users[ i] != user && users[ i].texture == texture)
						{
							DEBUG_TEXTURE_CACHE( printf( "Breaking texture reference for shared entry %s to avoid duplicate reloads\n", users[ i].uniform.name().asChar()); )
							users[ i].uniform.setDirty();
							users[ i].texture = NULL;
							texture->numUsers--;
						}
					}
				}
			}
			else
			{
				decUsage( texture);

				// Now forget our texture so the code below will search for the new one
				user->texture = NULL;
			}
		}

		// If we don't have a texture try and find an existing entry we can reuse
		if( user && !user->texture && name.length() > 0)
		{
			for( int i = textures.length(); i--; )
			{
				if( textures[ i].name == name && textures[ i].type == uniform.type())
				{
					DEBUG_TEXTURE_CACHE( printf( "Sharing %s\n", textures[ i].name.asChar()); )
					user->texture = &textures[ i];
					user->texture->numUsers++;
					break;
				}
			}
		}

		// If we still don't have a texture, create a new entry
		if( user && !user->texture && name.length() > 0)
		{
			// Remember the current memory offset in case the array gets reallocated
			Texture* oldMem = &textures[ 0];

			// Create a new texture entry
			DEBUG_TEXTURE_CACHE( printf( "Creating %s\n", name.asChar()); )
			int idx = textures.length();
			textures.length( idx + 1);

			// Repair our texture pointers if the memory got reallocated
			Texture* newMem = &textures[ 0];
			if( oldMem != newMem)
				for( int i = users.length(); i--; )
					if( users[ i].texture)
						users[ i].texture = (Texture*)((char*)users[ i].texture + ((char*)newMem - (char*)oldMem));

			// Now setup our new entry
			Texture* texture = &textures[ idx];
			texture->name = name;
			texture->type = uniform.type();
			texture->numUsers = 1;
			user->texture = texture;
		}

		// If our texture entry doesn't have a d3d texture, load one in!
		if( user && user->texture && !user->texture->texture)
		{
			// Create a new texture for this parameter
			MD3D9Renderer *pRenderer = MD3D9Renderer::theRenderer();
			if (pRenderer != NULL) {
				IDirect3DDevice9* d3dDevice = pRenderer->getD3D9Device();
				if(d3dDevice != NULL)
				{
					DEBUG_TEXTURE_CACHE( printf( "Loading %s\n", name.asChar()); )
						if( uniform.type() == MUniformParameter::kTypeCubeTexture)
						{
							LPDIRECT3DCUBETEXTURE9 d3dTexture;
							if( D3DXCreateCubeTextureFromFile(d3dDevice, name.asChar(), &d3dTexture) == D3D_OK)
								user->texture->texture = d3dTexture;
						}
						//else if( uniform.type() == MUniformParameter::kType3DTexture)
						//{
						//	LPDIRECT3DVOLUMETEXTURE9 d3dTexture;
						//	if( D3DXCreateVolumeTextureFromFile( d3dDevice, name.asChar(), &d3dTexture) == D3D_OK)
						//		user->texture->texture = d3dTexture;
						//}
						else
						{
							LPDIRECT3DTEXTURE9 d3dTexture;
							if( D3DXCreateTextureFromFile(d3dDevice, name.asChar(), &d3dTexture) == D3D_OK)
								user->texture->texture = d3dTexture;
						}
				}
			}
		}

		// Finally, bind our texture to the parameter
		d3dEffect->SetTexture( d3dParameter, (user && user->texture) ? user->texture->texture : NULL);
	}

	// Drop all the textures being used by the specified effect
	void release( LPD3DXEFFECT effect)
	{
		for( int i = users.length(); i--; )
		{
			if( users[ i].effect == effect)
			{
				DEBUG_TEXTURE_CACHE( printf( "Releasing effect usage %s\n", users[ i].uniform.name().asChar()); )
				if( users[ i].texture) decUsage( users[ i].texture);
				int newLength = users.length() - 1;
				if( i != newLength)
					users[ i] = users[ newLength];
				users.length( newLength);
			}
		}
	}

	virtual void deviceNew() {}
	virtual void deviceLost() { release(); }
	virtual void deviceDeleted() { release(); }
	static void newSceneCallback( void* ptr) { ((HLSLTextureManager*)ptr)->release(); }

	struct Texture
	{
		inline			Texture() : numUsers( 0), texture( NULL) {}
		MString			name;
		MUniformParameter::DataType type;
		LPDIRECT3DBASETEXTURE9 texture;
		int				numUsers;
	};
	struct User
	{
		inline			User() : parameter( NULL), effect( NULL), texture( NULL) {}
		D3DXHANDLE		parameter;
		LPD3DXEFFECT	effect;
		MUniformParameter uniform;
		Texture*		texture;
	};

	ObjArray<Texture>	textures;
	ObjArray<User>		users;
	MCallbackId			sceneCallbackIds[ 2];

	void		release()
	{
		DEBUG_TEXTURE_CACHE( printf( "Releasing cache with %d textures and %d users\n", textures.length(), users.length()); )
		for( int i = users.length(); i--; )
			users[ i].uniform.setDirty();
		for( int i = textures.length(); i--; )
			if( textures[ i].texture)
				textures[ i].texture->Release();
		users.length( 0);
		textures.length( 0);
	}

	void		decUsage( Texture* texture)
	{
		// It's currently using a different texture
		if( --texture->numUsers == 0)
		{
			// Drat! We're the last user of this texture so we need to remove it
			DEBUG_TEXTURE_CACHE( printf( "Unloading %s\n", texture->name.asChar()); )
			if( texture->texture)
				texture->texture->Release();

			// If this isn't the last texture in our list, we need to shuffle our
			// textures around to fill the hole it leaves behind
			Texture* lastTexture = &textures[ textures.length() - 1];
			if( texture != lastTexture)
			{
				*texture = *lastTexture;
				for( int i = users.length(); i--; )
					if( users[ i].texture == lastTexture)
						users[ i].texture = texture;
			}

			// Now remove the unused texture (which is now guaranteed to be at the
			// end of the array) from our list
			textures.length( textures.length() - 1);
		}
		DEBUG_TEXTURE_CACHE( else printf( "Unsharing %s\n", texture->name.asChar()); )
	}
};
HLSLTextureManager HLSLTextureManager::sInstance;


//
// Find an annotation
//
bool hlslShader::GetAnnotation( D3DXHANDLE parameter, const char* name, LPCSTR& value)
{
	D3DXHANDLE d3dSemantic = fD3DEffect->GetAnnotationByName( parameter, name);
	return d3dSemantic && fD3DEffect->GetString( d3dSemantic, &value) == D3D_OK && value != NULL;
}
bool hlslShader::GetAnnotation( D3DXHANDLE parameter, const char* name, BOOL& value)
{
	D3DXHANDLE d3dSemantic = fD3DEffect->GetAnnotationByName( parameter, name);
	return d3dSemantic && fD3DEffect->GetBool( d3dSemantic, &value) == D3D_OK;
}


// 
// Convert a DX type into a Maya type
//
MUniformParameter::DataType hlslShader::ConvertType( D3DXHANDLE parameter, D3DXPARAMETER_DESC& description)
{
	MUniformParameter::DataType mtype = MUniformParameter::kTypeUnknown;
	switch( description.Type)
	{
		//case D3DXPT_VOID: break; 
		case D3DXPT_BOOL: mtype = MUniformParameter::kTypeBool; break; 
		case D3DXPT_INT: mtype = MUniformParameter::kTypeInt; break; 
		case D3DXPT_FLOAT: mtype = MUniformParameter::kTypeFloat; break; 
		case D3DXPT_STRING: mtype = MUniformParameter::kTypeString; break; 
		case D3DXPT_TEXTURE1D: mtype = MUniformParameter::kType1DTexture; break; 
		case D3DXPT_TEXTURE2D: mtype = MUniformParameter::kType2DTexture; break; 
		case D3DXPT_TEXTURE3D: mtype = MUniformParameter::kType3DTexture; break; 
		case D3DXPT_TEXTURECUBE: mtype = MUniformParameter::kTypeCubeTexture; break; 
		case D3DXPT_TEXTURE: 
		{
			// The shader hasn't used a typed texture, so first see if there's an annotation
			// that tells us which type to use.
			//
			LPCSTR textureType;
			if( ( GetAnnotation( parameter, "TextureType", textureType) || GetAnnotation( parameter, "ResourceType", textureType)) && textureType)
			{
				// Grab the type off from the annotation
				//
					 if( !stricmp( textureType, "1D")) mtype = MUniformParameter::kType1DTexture;
				else if( !stricmp( textureType, "2D")) mtype = MUniformParameter::kType2DTexture;
				else if( !stricmp( textureType, "3D")) mtype = MUniformParameter::kType3DTexture;
				else if( !stricmp( textureType, "Cube")) mtype = MUniformParameter::kTypeCubeTexture;
				else 
				{
					fDiagnostics += "Unrecognised texture type semantic ";
					fDiagnostics += textureType;
					fDiagnostics += " on parameter ";
					fDiagnostics += description.Name;
					fDiagnostics += "\n";
				}
			}

			if( mtype == MUniformParameter::kTypeUnknown)
			{
				// No explicit type. At this stage, it would be nice to take a look at the 
				// sampler which uses the texture and grab the type of that, but I can't see
				// any way to query for the sampler -> texture bindings through the effect
				// API. 
				//
				mtype = MUniformParameter::kType2DTexture;
				fDiagnostics += "No texture type provided for ";
				fDiagnostics += description.Name;
				fDiagnostics += ", assuming 2D\n";
			}
			
			break; 
		}
		//case D3DXPT_SAMPLER: break; 
		//case D3DXPT_SAMPLER1D: break; 
		//case D3DXPT_SAMPLER2D: break; 
		//case D3DXPT_SAMPLER3D: break; 
		//case D3DXPT_SAMPLERCUBE: break; 
		//case D3DXPT_PIXELSHADER: break; 
		//case D3DXPT_VERTEXSHADER: break; 
		//case D3DXPT_PIXELFRAGMENT: break; 
		//case D3DXPT_VERTEXFRAGMENT: break; 
		default: break;
	}
	return mtype;
}


//
// Convert a DX space into a Maya space
//
MUniformParameter::DataSemantic hlslShader::ConvertSpace( D3DXHANDLE parameter, D3DXPARAMETER_DESC& description, MUniformParameter::DataSemantic defaultSpace)
{
	MUniformParameter::DataSemantic space = defaultSpace;
	LPCSTR ann;
	if( GetAnnotation( parameter, "Space", ann) && ann)
	{
			 if( !stricmp( ann, "Object")) space = defaultSpace >= MUniformParameter::kSemanticObjectPos ? MUniformParameter::kSemanticObjectPos : MUniformParameter::kSemanticObjectDir;
		else if( !stricmp( ann, "World")) space = defaultSpace >= MUniformParameter::kSemanticObjectPos ? MUniformParameter::kSemanticWorldPos : MUniformParameter::kSemanticWorldDir;
		else if( !stricmp( ann, "View")) space = defaultSpace >= MUniformParameter::kSemanticObjectPos ? MUniformParameter::kSemanticViewPos : MUniformParameter::kSemanticViewDir;
		else if( !stricmp( ann, "Camera")) space = defaultSpace >= MUniformParameter::kSemanticObjectPos ? MUniformParameter::kSemanticViewPos : MUniformParameter::kSemanticViewDir;
		else
		{
			fDiagnostics += "Unrecognised space ";
			fDiagnostics += ann;
			fDiagnostics += " on parameter ";
			fDiagnostics += description.Name;
			fDiagnostics += "\n";
		}
	}
	return space;
}


//
// Convert a DX semantic into a Maya semantic
//
MUniformParameter::DataSemantic hlslShader::ConvertSemantic( D3DXHANDLE parameter, D3DXPARAMETER_DESC& description)
{
	MUniformParameter::DataSemantic msemantic = MUniformParameter::kSemanticUnknown;

	// First try the explicit semantic
	if( description.Semantic && *description.Semantic)
	{
			 if( !stricmp( description.Semantic, "World"))								msemantic = MUniformParameter::kSemanticWorldMatrix;
		else if( !stricmp( description.Semantic, "WorldInverse"))						msemantic = MUniformParameter::kSemanticWorldInverseMatrix;
		else if( !stricmp( description.Semantic, "WorldInverseTranspose"))				msemantic = MUniformParameter::kSemanticWorldInverseTransposeMatrix;
		else if( !stricmp( description.Semantic, "View"))								msemantic = MUniformParameter::kSemanticViewMatrix;
		else if( !stricmp( description.Semantic, "ViewInverse"))						msemantic = MUniformParameter::kSemanticViewInverseMatrix;
		else if( !stricmp( description.Semantic, "ViewInverseTranspose"))				msemantic = MUniformParameter::kSemanticViewInverseTransposeMatrix;
		else if( !stricmp( description.Semantic, "Projection"))							msemantic = MUniformParameter::kSemanticProjectionMatrix;
		else if( !stricmp( description.Semantic, "ProjectionInverse"))					msemantic = MUniformParameter::kSemanticProjectionInverseMatrix;
		else if( !stricmp( description.Semantic, "ProjectionInverseTranspose"))			msemantic = MUniformParameter::kSemanticProjectionInverseTransposeMatrix;
		else if( !stricmp( description.Semantic, "WorldView"))							msemantic = MUniformParameter::kSemanticWorldViewMatrix;
		else if( !stricmp( description.Semantic, "WorldViewInverse"))					msemantic = MUniformParameter::kSemanticWorldViewInverseMatrix;
		else if( !stricmp( description.Semantic, "WorldViewInverseTranspose"))			msemantic = MUniformParameter::kSemanticWorldViewInverseTransposeMatrix;
		else if( !stricmp( description.Semantic, "WorldViewProjection"))				msemantic = MUniformParameter::kSemanticWorldViewProjectionMatrix;
		else if( !stricmp( description.Semantic, "WorldViewProjectionInverse"))			msemantic = MUniformParameter::kSemanticWorldViewProjectionInverseMatrix;
		else if( !stricmp( description.Semantic, "WorldViewProjectionInverseTranspose"))msemantic = MUniformParameter::kSemanticWorldViewProjectionInverseTransposeMatrix;
		else if( !stricmp( description.Semantic, "Diffuse"))							msemantic = MUniformParameter::kSemanticColor;
		else if( !stricmp( description.Semantic, "Specular"))							msemantic = MUniformParameter::kSemanticColor;
		else if( !stricmp( description.Semantic, "Ambient"))							msemantic = MUniformParameter::kSemanticColor;
		else if( !stricmp( description.Semantic, "Color"))								msemantic = MUniformParameter::kSemanticColor;
		else if( !stricmp( description.Semantic, "Normal"))								msemantic = MUniformParameter::kSemanticNormal;
		else if( !stricmp( description.Semantic, "Bump"))								msemantic = MUniformParameter::kSemanticBump;
		else if( !stricmp( description.Semantic, "Environment"))						msemantic = MUniformParameter::kSemanticEnvironment;
		else if( !stricmp( description.Semantic, "Time"))								msemantic = MUniformParameter::kSemanticTime;
		else if( !stricmp( description.Semantic, "Position"))							msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticWorldPos);
		else if( !stricmp( description.Semantic, "Direction"))							msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticViewDir);
		else 
		{
			fDiagnostics += "Unrecognised semantic ";
			fDiagnostics += description.Semantic;
			fDiagnostics += " on parameter ";
			fDiagnostics += description.Name;
			fDiagnostics += "\n";
		}
	}

	// Next, try annotation semantic
	if( msemantic == MUniformParameter::kSemanticUnknown)
	{
		LPCSTR sasSemantic;
		if( GetAnnotation( parameter, "SasBindAddress", sasSemantic) && *sasSemantic)
		{
			MString str( sasSemantic);
				 if( !stricmp( sasSemantic, "Sas.Skeleton.MeshToJointToWorld[0]"))	msemantic = MUniformParameter::kSemanticWorldMatrix;
			else if( !stricmp( sasSemantic, "Sas.Camera.WorldToView"))				msemantic = MUniformParameter::kSemanticViewMatrix;
			else if( !stricmp( sasSemantic, "Sas.Camera.Projection"))				msemantic = MUniformParameter::kSemanticProjectionMatrix;
			else if( !stricmp( sasSemantic, "Sas.Time.Now"))						msemantic = MUniformParameter::kSemanticTime;
			else if( str.rindexW( ".Position") >= 0)								msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticWorldPos);
			else if( str.rindexW( ".Direction") >= 0 && str.rindexW( ".Direction") != str.rindexW( ".Directional"))	msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticViewDir);
			else 
			{
				fDiagnostics += "Unrecognised semantic ";
				fDiagnostics += sasSemantic;
				fDiagnostics += " on parameter ";
				fDiagnostics += description.Name;
				fDiagnostics += "\n";
			}
		}
	}

	// Next try control type
	if( msemantic == MUniformParameter::kSemanticUnknown)
	{
		LPCSTR sasSemantic;
		if( GetAnnotation( parameter, "SasUiControl", sasSemantic) && *sasSemantic)
		{
				 if( !stricmp( sasSemantic, "ColorPicker"))							msemantic = MUniformParameter::kSemanticColor;
		}
	}
	
	// As a last ditch effort, look for an obvious parameter name
	if( msemantic == MUniformParameter::kSemanticUnknown)
	{
		MString name( description.Name);
		if( name.rindexW( "Position") >= 0)									msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticWorldPos);
		else if( name.rindexW( "Direction") >= 0 && name.rindexW( "Direction") != name.rindexW( "Directional"))	msemantic = ConvertSpace( parameter, description, MUniformParameter::kSemanticWorldDir);
		else if( name.rindexW( "Color") >= 0 || name.rindexW( "Colour") >= 0 || name.rindexW( "Diffuse") >= 0 || name.rindexW( "Specular") >= 0 || name.rindexW( "Ambient") >= 0)
																					msemantic = MUniformParameter::kSemanticColor;
	}

	return msemantic;
}



// This typeid must be unique across the universe of Maya plug-ins.  
//

MTypeId     hlslShader::sId( 0xF3560C30 );

// Our rendering profile
//
MRenderProfile hlslShader::sProfile;

// Attribute declarations
// 
MObject     hlslShader::sShader;
MObject     hlslShader::sTechnique;
MObject     hlslShader::sTechniques;
MObject     hlslShader::sDescription;
MObject     hlslShader::sDiagnostics;


#define M_CHECK(assertion)  if (assertion) ; else throw ((hlsl::InternalError*)__LINE__)

namespace hlsl
{
#ifdef _WIN32
	class InternalError;    // Never defined.  Used like this:
	//   throw (InternalError*)__LINE__;
#else
	struct InternalError
	{
		char* message;
	};
	//   throw (InternalError*)__LINE__;
#endif
}


//--------------------------------------------------------------------//
// Constructor:
//
hlslShader::hlslShader()
: fErrorCount( 0), fD3DEffect( NULL), fD3DTechnique( NULL), 
  fD3DVertexDeclaration( NULL), fVertexStructure( "VertexStructure", MVaryingParameter::kStructure), 
  fDeviceManager( me()),
  fTechniqueHasBlending( false )
{
}


// Post-constructor
void
hlslShader::postConstructor()
{
	// Don't create any default varying parameters (e.g. position + normal) for empty
	// shaders as this just bloats the cache with a useless structure that is currently
	// not flushed out of the cache until the geometry changes. Instead, just render
	// the default geometry directly off the position array in the cache
}


// Destructor:
//
hlslShader::~hlslShader()
{
	release();
}


//
// Release all our device handles
//
void hlslShader::release()
{
	releaseVertexDeclaration();
	if( fD3DEffect) 
	{
		HLSLTextureManager::sInstance.release( fD3DEffect);
		fD3DEffect->Release();
		fD3DEffect = NULL;
	}
	fD3DTechnique = NULL;
	fTechniqueHasBlending = false;
}


//
// Release our vertex declaration
//
void hlslShader::releaseVertexDeclaration()
{
	if( fD3DVertexDeclaration) 
	{
		for( unsigned int p = 0; p < fD3DTechniqueDesc.Passes; p++)
			if( fD3DVertexDeclaration[ p])
				fD3DVertexDeclaration[ p]->Release();
		delete[] fD3DVertexDeclaration;
		fD3DVertexDeclaration = NULL;
	}
}


// ========== hlslShader::creator ==========
//
//	Description:
//		this method exists to give Maya a way to create new objects
//      of this type.
//
//	Return Value:
//		a new object of this type.
//
/* static */
void* hlslShader::creator()
{
	return new hlslShader();
}


// ========== hlslShader::initialize ==========
//
//	Description:
//		This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called 
//		once when the node type is registered with Maya.
//
//	Return Values:
//		MS::kSuccess
//		MS::kFailure
//		
/* static */
MStatus
hlslShader::initialize()
{
	MStatus ms;

	try
	{
		initializeNodeAttrs();
	}
	catch ( ... )
	{
		// [mashworth] Don't forget I18N
		MGlobal::displayError( "hlslShader internal error: Unhandled exception in initialize" );
		ms = MS::kFailure;
	}

	sProfile.addRenderer( MRenderProfile::kMayaD3D);

	return ms;
}


// Create all the attributes. 
/* static */
void
hlslShader::initializeNodeAttrs()
{
	MFnTypedAttribute	typedAttr;
	MFnStringData		stringData;
	MFnStringArrayData	stringArrayData;
	MStatus				stat, stat2;

	// The shader attribute holds the name of the .fx file that defines
	// the shader
	//
	sShader = typedAttr.create("shader", "s", MFnData::kString, stringData.create(&stat2), &stat); 
	M_CHECK( stat );
	typedAttr.setInternal( true); 
	typedAttr.setKeyable( false); 
	stat = addAttribute(sShader); 
	M_CHECK( stat );

	//
	// technique
	//
	sTechnique = typedAttr.create("technique", "t", MFnData::kString, stringData.create(&stat2), &stat); 
	M_CHECK( stat );
	typedAttr.setInternal( true);
	typedAttr.setKeyable( true);
	stat = addAttribute(sTechnique);
	M_CHECK( stat );

	//
	// technique list
	//
	sTechniques = typedAttr.create("techniques", "ts", MFnData::kStringArray, stringArrayData.create(&stat2), &stat); 
	M_CHECK( stat );
	typedAttr.setInternal( true);
	typedAttr.setKeyable( false);
	typedAttr.setStorable( false);
	typedAttr.setWritable( false);
	stat = addAttribute(sTechniques);
	M_CHECK( stat );

	// The description field where we pass compile errors etc back for the user to see
	//
	sDescription = typedAttr.create("description", "desc", MFnData::kString, stringData.create(&stat2), &stat); 
	M_CHECK( stat );
	typedAttr.setKeyable( false); 
	typedAttr.setWritable( false); 
	typedAttr.setStorable( false); 
	stat = addAttribute(sDescription); 
	M_CHECK( stat );

	// The feedback field where we pass compile errors etc back for the user to see
	//
	sDiagnostics = typedAttr.create("diagnostics", "diag", MFnData::kString, stringData.create(&stat2), &stat); 
	M_CHECK( stat );
	typedAttr.setKeyable( false); 
	typedAttr.setWritable( false); 
	typedAttr.setStorable( false); 
	stat = addAttribute(sDiagnostics); 
	M_CHECK( stat );

	// 
	// Specify our dependencies
	//
	attributeAffects( sShader, sTechniques);
	attributeAffects( sShader, sTechnique);
}


void
hlslShader::copyInternalData( MPxNode* pSrc )
{
	const hlslShader& src = *(hlslShader*)pSrc;
	fTechnique = src.fTechnique;
	fTechniqueHasBlending = src.fTechniqueHasBlending;
	setShader( src.fShader);
}


bool hlslShader::setInternalValueInContext( const MPlug& plug,
											  const MDataHandle& handle,
											  MDGContext& context)
{
	bool retVal = true;

	try
	{
		if (plug == sShader)
		{
			setShader( handle.asString());
		}
		else if (plug == sTechnique)
		{
			setTechnique( handle.asString());
		}
		else
		{
			retVal = MPxHardwareShader::setInternalValue(plug, handle);
		}
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		retVal = false;
	}

	return retVal;
}


/* virtual */
bool hlslShader::getInternalValueInContext( const MPlug& plug,
											  MDataHandle& handle,
											  MDGContext&)
{
	bool retVal = true;

	try
	{
		if (plug == sShader)
		{
			handle.set( fShader);
		}
		else if (plug == sTechnique)
		{
			handle.set( fTechnique);
		}
		else if (plug == sTechniques)
		{
			handle.set( MFnStringArrayData().create( fTechniques));
		}
		else
		{
			retVal = MPxHardwareShader::getInternalValue(plug, handle);
		}
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		retVal = false;
	}

	return retVal;
}


// Override geometry rendering
//
MStatus hlslShader::render( MGeometryList& iter)
{
	MD3D9Renderer *pRenderer = MD3D9Renderer::theRenderer();
	if (pRenderer == NULL) 
	{
		// If there isn't a render, we can't render anything
		return MStatus::kFailure;
	}

	IDirect3DDevice9* d3dDevice = pRenderer->getD3D9Device();  	
	if ( NULL == d3dDevice ) 
	{
		// If there isn't a valid device, we can't render anything
		return MStatus::kFailure;	
	}

	if (fDeviceManager.deviceState() == hlslDeviceManager::kReset)
	{
		fDeviceManager.resetShader();
	}

	// We have a device, now do we have a valid effect, or should we render 
	// some stand-in geometry?
	//
	HLSLStateManager::sInstance.fD3DDevice = d3dDevice;
	if( fD3DEffect && fD3DVertexDeclaration)
	{
		UINT numPasses = 0;

		fD3DEffect->Begin( &numPasses, 0); // The 0 tells DX to save and restore all state modified by the effect ... hmmmm perf hit?
		bool firstShape = true;
		for( ; !iter.isDone(); iter.next())
		{
			// Setup our shape dependent parameters
			for( int u = fUniformParameters.length(); u--; )
			{
				MUniformParameter uniform = fUniformParameters.getElement( u);
				if( uniform.hasChanged( iter))
				{
					D3DXHANDLE d3dParameter = (D3DXHANDLE)uniform.userData();
					if( d3dParameter)
					{
						switch( uniform.type())
						{
							case MUniformParameter::kTypeFloat: 
								{
								const float* data = uniform.getAsFloatArray( iter);
								if( data) fD3DEffect->SetValue( d3dParameter, data, uniform.numElements() * sizeof( float));
								}
								break;
							case MUniformParameter::kTypeInt: 
								fD3DEffect->SetInt( d3dParameter, uniform.getAsInt( iter));
								break;
							case MUniformParameter::kTypeBool: 
								fD3DEffect->SetBool( d3dParameter, uniform.getAsBool( iter));
								break;
							case MUniformParameter::kTypeString: 
								fD3DEffect->SetString( d3dParameter, uniform.getAsString( iter).asChar());
								break;
							default:
								if( uniform.isATexture())
									HLSLTextureManager::sInstance.bind( uniform.getAsString( iter).asChar(), uniform, fD3DEffect, d3dParameter);
								break;
						}
					}
				}
			}

			MGeometry& geometry = iter.geometry( MGeometryList::kNone);
			const void* data;
			unsigned int elements, count;
			if( fVertexStructure.numElements() && fVertexStructure.getBuffer( geometry, data, elements, count) == MStatus::kSuccess)
			{
				MGeometryPrimitive primitives = geometry.primitiveArray( 0);
				HLSLStateManager::sInstance.shapeCullMode( iter.cullMode());
				for( UINT p = 0; p < numPasses; p++)
				{
					if( !fD3DVertexDeclaration[ p]) continue;
					if( numPasses > 1 || firstShape) 
						HLSLStateManager::sInstance.BeginPass( fD3DEffect, p);
					else 
						fD3DEffect->CommitChanges(); // Make sure any shape-dependent uniforms get sent down the to effect when rendering a single pass effect!
					firstShape = false;
					d3dDevice->SetVertexDeclaration( fD3DVertexDeclaration[ p]); 

#ifdef MAX_SEGMENTED_BATCH_SIZE
					unsigned int numPrimitives = primitives.elementCount() / 3;
					unsigned int batchSize = numPrimitives / (numPrimitives / MAX_SEGMENTED_BATCH_SIZE + 1) + 1;
					for( unsigned int start = 0; start < numPrimitives; )
					{
						unsigned int end = start + batchSize;
						if( end > numPrimitives) end = numPrimitives;
						d3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, count, end - start, ((const unsigned int*)primitives.data()) + (start * 3), D3DFMT_INDEX32, data, elements);
						start = end;
					}
#else
					d3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, count, primitives.elementCount() / 3, primitives.data(), D3DFMT_INDEX32, data, elements);
#endif
					if( numPasses > 1) fD3DEffect->EndPass();
				}
			}
		}
		if( numPasses == 1) fD3DEffect->EndPass();
		fD3DEffect->End();
	}
	else
	{
		// Hokey DX draw
		// There is no lighting so the only color will be the emissive color
		D3DMATERIAL9 Material;
		Material.Emissive.r = 0.0f; 
		Material.Emissive.g = 0.6f; 
		Material.Emissive.b = 0.0f; 
		Material.Emissive.a = 1.0f; 

		d3dDevice->SetMaterial( &Material);
		d3dDevice->LightEnable( 0, false);
		d3dDevice->LightEnable( 1, false);
		d3dDevice->SetFVF( D3DFVF_XYZ );

		for( ; !iter.isDone(); iter.next())
		{
			MGeometry& geometry = iter.geometry( MGeometryList::kNone );
			const MGeometryData position = geometry.position();
			if( position.data())
			{
				// set up the matrices as this aren't picked up automatically as may be
				// different in the case of swatch rendering from the current directX state.
				//
				const MMatrix& mtm = iter.objectToWorldMatrix();
				D3DXMATRIXA16 tm( (float)mtm.matrix[0][0], (float)mtm.matrix[0][1], (float)mtm.matrix[0][2], (float)mtm.matrix[0][3], 
								  (float)mtm.matrix[1][0], (float)mtm.matrix[1][1], (float)mtm.matrix[1][2], (float)mtm.matrix[1][3], 
								  (float)mtm.matrix[2][0], (float)mtm.matrix[2][1], (float)mtm.matrix[2][2], (float)mtm.matrix[2][3], 
								  (float)mtm.matrix[3][0], (float)mtm.matrix[3][1], (float)mtm.matrix[3][2], (float)mtm.matrix[3][3]);
				
				d3dDevice->SetTransform( D3DTS_WORLD, &tm);

				// Setup the camera for drawing new stuff
				const MMatrix& mproject = iter.projectionMatrix();
				D3DXMATRIXA16 projection( (float)mproject.matrix[0][0], (float)mproject.matrix[0][1], (float)mproject.matrix[0][2], (float)mproject.matrix[0][3], 
								  (float)mproject.matrix[1][0], (float)mproject.matrix[1][1], (float)mproject.matrix[1][2], (float)mproject.matrix[1][3], 
								  (float)mproject.matrix[2][0], (float)mproject.matrix[2][1], (float)mproject.matrix[2][2], (float)mproject.matrix[2][3], 
								  (float)mproject.matrix[3][0], (float)mproject.matrix[3][1], (float)mproject.matrix[3][2], (float)mproject.matrix[3][3]);
				d3dDevice->SetTransform( D3DTS_PROJECTION, &projection );

				const MMatrix& mview = iter.viewMatrix();
				D3DXMATRIXA16 view( (float)mview.matrix[0][0], (float)mview.matrix[0][1], (float)mview.matrix[0][2], (float)mview.matrix[0][3], 
								  (float)mview.matrix[1][0], (float)mview.matrix[1][1], (float)mview.matrix[1][2], (float)mview.matrix[1][3], 
								  (float)mview.matrix[2][0], (float)mview.matrix[2][1], (float)mview.matrix[2][2], (float)mview.matrix[2][3], 
								  (float)mview.matrix[3][0], (float)mview.matrix[3][1], (float)mview.matrix[3][2], (float)mview.matrix[3][3]);
				d3dDevice->SetTransform( D3DTS_VIEW, &view );

				HLSLStateManager::sInstance.shapeCullMode( iter.cullMode(), true);
				MGeometryPrimitive primitives = geometry.primitiveArray( 0);
				d3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, position.elementCount(), primitives.elementCount() / 3, primitives.data(), D3DFMT_INDEX32, position.data(), 3 * sizeof( float));
			} 
			else 
			{
				 MStatus::kFailure;
			}
		}
	}
	return MStatus::kSuccess;
}



/* virtual */
unsigned int hlslShader::transparencyOptions()
{
	if (fTechniqueHasBlending)
		return ( kIsTransparent | kNoTransparencyFrontBackCull | kNoTransparencyPolygonSort );
	return 0;
}


// Query the renderers supported by this shader
//
const MRenderProfile& hlslShader::profile() { return sProfile; }


void hlslShader::setupUniform( D3DXHANDLE d3dParameter, const MString& prefix)
{
	if( d3dParameter)
	{
		D3DXPARAMETER_DESC parameterDesc;
		fD3DEffect->GetParameterDesc( d3dParameter, &parameterDesc);
		if( parameterDesc.Class == D3DXPC_STRUCT)
		{
			for( unsigned int p = 0; p < parameterDesc.StructMembers; p++)
			{
				setupUniform( fD3DEffect->GetParameter( d3dParameter, p), prefix + MString( parameterDesc.Name) + ".");
			}
		}
		else
		{
			// Is this a special parameter?
			if( !stricmp( parameterDesc.Semantic, "SasGlobal"))
			{
				LPCSTR sasDescription;
				if( GetAnnotation( d3dParameter, "SasEffectDescription", sasDescription) && *sasDescription)
					fDescription += sasDescription;
				return;
			}
			else if( !stricmp( parameterDesc.Name, "description") && parameterDesc.Type == D3DXPT_STRING)
			{
				LPCSTR Value; 
				if( fD3DEffect->GetString( d3dParameter, &Value) == D3D_OK) 
					fDescription += Value;
				return;
			}

			MUniformParameter::DataType type = ConvertType( d3dParameter, parameterDesc);
			if( type != MUniformParameter::kTypeUnknown)
			{
				MUniformParameter::DataSemantic semantic = ConvertSemantic( d3dParameter, parameterDesc);
				int rows = parameterDesc.Rows;
				int columns = parameterDesc.Columns;

				// If we don't know what this parameter is, and we've been told to hide it, do so
				// NOTE that for now, we only hide simple constants as hiding everything we're
				// told to hide actually starts hiding textures and things the artist wants to see. 
				if( semantic == MUniformParameter::kSemanticUnknown && (type == MUniformParameter::kTypeFloat || type == MUniformParameter::kTypeString))
				{
					BOOL visible = true;
					if( GetAnnotation( d3dParameter, "SasUiVisible", visible) && !visible)
						return;
				}

				MUniformParameter uniform( prefix + parameterDesc.Name, type, semantic, rows, columns, (void*)d3dParameter);
				switch( type)
				{
					case MUniformParameter::kTypeFloat: { FLOAT Value[ 16]; if( fD3DEffect->GetFloatArray( d3dParameter, Value, 16) == D3D_OK) uniform.setAsFloatArray( Value, 16); break; }
					case MUniformParameter::kTypeString: { LPCSTR Value; if( fD3DEffect->GetString( d3dParameter, &Value) == D3D_OK) uniform.setAsString( MString( Value)); break; }
					case MUniformParameter::kTypeBool: { BOOL Value; if( fD3DEffect->GetBool( d3dParameter, &Value) == D3D_OK) uniform.setAsBool( Value ? true : false); break; }
					case MUniformParameter::kTypeInt: { INT Value; if( fD3DEffect->GetInt( d3dParameter, &Value) == D3D_OK) uniform.setAsInt( Value); break; }
					default:
						if( type >= MUniformParameter::kType1DTexture && type <= MUniformParameter::kTypeEnvTexture)
						{
							LPCSTR resource;
							if( GetAnnotation( d3dParameter, "ResourceName", resource) && *resource)
								uniform.setAsString( findResource( MString( resource), fShader));
							else if( GetAnnotation( d3dParameter, "SasResourceAddress", resource) && *resource)
								uniform.setAsString( findResource( MString( resource), fShader));
						}
				}
				fUniformParameters.append( uniform);
			}
		}
	}
}


//
// Set the shader we're using
//
MStatus hlslShader::setShader( const MString& shader)
{
	fDiagnostics = "";
	fDescription = "";
	fShader = shader;

	MD3D9Renderer *pRenderer = MD3D9Renderer::theRenderer();
	if (pRenderer != NULL) {
		IDirect3DDevice9* pDevice = pRenderer->getD3D9Device();
		if( pDevice )
		{

			release();
			fTechniques.setLength( 0);
			LPD3DXBUFFER errors = NULL;

			// Make sure the shader's directory is current, as the compiler will try and resolve
			// everything relative to that
			//
			MFileObject fileObject;
			fileObject.setRawFullName( shader);
			MString resolvedPath = fileObject.resolvedPath();
			TCHAR pwd[ MAX_PATH];
			if( resolvedPath.length() > 0)
			{
				::GetCurrentDirectory( MAX_PATH, pwd);
				::SetCurrentDirectory( resolvedPath.asChar());
			}

			// Note that for some strange reason, DX9 doesn't support D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
			// so we try the old DX9 compiler separately. We can remove the second attempt once we move to DX10
			// (which doesn't support the D3DXSHADER_USE_LEGACY_D3DX9_31_DLL  flag)
			//
#ifndef D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY
#define D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY 0
#endif
			if( FAILED( D3DXCreateEffectFromFile( pDevice, shader.asChar(), NULL, NULL, D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY, NULL, &fD3DEffect, &errors)) 
#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
			 && FAILED( D3DXCreateEffectFromFile( pDevice, shader.asChar(), NULL, NULL, D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, NULL, &fD3DEffect, NULL))
#endif
				)
			{
				fDiagnostics += "Error trying to create effect " + shader + ":\n\t";
				if( errors)
					fDiagnostics += (const char*)errors->GetBufferPointer();
				fDiagnostics += "\n";
			}
			else
			{
				if( errors)
					fDiagnostics += (const char*)errors->GetBufferPointer();
				fD3DEffect->SetStateManager( &HLSLStateManager::sInstance);
				fD3DEffect->GetDesc( &fD3DEffectDesc);
				for( unsigned int t = 0; t < fD3DEffectDesc.Techniques; t++)
				{
					D3DXHANDLE technique = fD3DEffect->GetTechnique( t);
					if( technique)
					{
#ifdef VALIDATE_TECHNIQUE_LIST
						if( fD3DEffect->ValidateTechnique( technique))
#endif
						{
							D3DXTECHNIQUE_DESC techniqueDesc;
							fD3DEffect->GetTechniqueDesc( technique, &techniqueDesc);
							fTechniques.append( techniqueDesc.Name);
						}
					}
				}
			}

			// Restore the previous working directory if we changed it
			if( resolvedPath.length() > 0)
			{
				::SetCurrentDirectory( pwd);
			}


			// Update our uniform parameters
			if( fD3DEffect)
			{
				fUniformParameters.setLength( 0);
				for( unsigned int p = 0; p < fD3DEffectDesc.Parameters; p++)
				{
					setupUniform( fD3DEffect->GetParameter( NULL, p), "");
				}
				setUniformParameters( fUniformParameters);

				// And re-select our current technique (which triggers a UI change
				// and also handles the case where the current technique doesn't 
				// exist in this effect
				MPlug( thisMObject(), sTechnique).setValue( fTechnique);
			}
		}
	}

	// Update our shader info attributes
	MPlug diagnosticsPlug( thisMObject(), sDiagnostics);
	diagnosticsPlug.setValue( fDiagnostics);
	MPlug descriptionPlug( thisMObject(), sDescription);
	descriptionPlug.setValue( fDescription);

	return MS::kSuccess;
}


bool hlslShader::passHasTranparency( D3DXHANDLE d3dPass )
{
	bool hasTransparency = false;

	//printf("Pass %s has %d annotations\n", passDesc.Name, passDesc.Annotations);
	BOOL boolParameter;
	INT intParameter1, intParameter2;
	boolParameter = false;
	{
		BOOL getAnnot = GetAnnotation( d3dPass, "Zenable", boolParameter);
		//printf("Get zenable %d, %d\n", getAnnot, boolParameter);
		getAnnot = GetAnnotation( d3dPass, "ZWriteEnable", boolParameter);
		//printf("Get zwriteenable %d, %d\n", getAnnot, boolParameter);
	}

	BOOL getAnnot = false;
	{
		D3DXHANDLE d3dSemantic = fD3DEffect->GetAnnotationByName( d3dPass, "AlphaBlendEnable");
		if (d3dSemantic)
		{
			fD3DEffect->GetBool( d3dSemantic, &boolParameter);
			getAnnot = true;
		}
	}

	//printf("Alpha blend found %d, is enabled %d !\n", getAnnot, boolParameter);
	if (getAnnot && boolParameter)
	{
		hasTransparency = true;

		{
			D3DXHANDLE d3dSemantic = fD3DEffect->GetAnnotationByName( d3dPass, "SrcBlend");
			if (d3dSemantic)
				fD3DEffect->GetInt( d3dSemantic, &intParameter1);
		}
		{
			D3DXHANDLE d3dSemantic = fD3DEffect->GetAnnotationByName( d3dPass, "DestBlend");
			if (d3dSemantic)
				fD3DEffect->GetInt( d3dSemantic, &intParameter2);
		}
	}				

	return hasTransparency;
}

//
// Set the technique we're using
//
MStatus hlslShader::setTechnique( const MString& technique)
{
	// We're changing technique, blow away any existing vertex declaration
	releaseVertexDeclaration();

	// Now, if we have an effect update our internal data and parameter list
	// (otherwise just leave the current parameters in place)
	if( fD3DEffect && fTechniques.length())
	{
		// Try and switch to this technique
		fD3DTechnique = fD3DEffect->GetTechniqueByName( technique.asChar());
#ifdef VALIDATE_TECHNIQUES
		if( !fD3DTechnique || !fD3DEffect->ValidateTechnique( fD3DTechnique))
#else
		if( !fD3DTechnique)
#endif
		{
			fD3DTechnique = fD3DEffect->GetTechniqueByName( fTechnique.asChar());
#ifdef VALIDATE_TECHNIQUES
			if( !fD3DTechnique || !fD3DEffect->ValidateTechnique( fD3DTechnique))
#else
			if( !fD3DTechnique)
#endif
			{
#ifdef VALIDATE_TECHNIQUES
				if( FAILED( fD3DEffect->FindNextValidTechnique( fD3DTechnique, &fD3DTechnique)))
				{
					fD3DTechnique = NULL;
				}
#else
				fD3DTechnique = fD3DEffect->GetTechnique( 0);
#endif
			}
		}

		// Now suck the name back out
		fD3DEffect->GetTechniqueDesc( fD3DTechnique, &fD3DTechniqueDesc);
		fTechnique = fD3DTechniqueDesc.Name;

		// Make this the active technique
		fD3DEffect->SetTechnique( fD3DTechnique);

		// Update our passes and varying parameters
		fVertexStructure.removeElements();
		fD3DVertexDeclaration = new IDirect3DVertexDeclaration9*[ fD3DTechniqueDesc.Passes];

		int offset = 0;
		for( unsigned int p = 0; p < fD3DTechniqueDesc.Passes; p++)
		{
			D3DXHANDLE d3dPass = fD3DEffect->GetPass( fD3DTechnique, p);

#if defined(_BLENDPARSING_READY_)
			if (p == 0)
				fTechniqueHasBlending = passHasTranparency( d3dPass );
#endif
			D3DXPASS_DESC passDesc;
			fD3DEffect->GetPassDesc( d3dPass, &passDesc);

			D3DXSEMANTIC Semantics[ MAXD3DDECLLENGTH];
			D3DVERTEXELEMENT9 VertexElements[ MAXD3DDECLLENGTH + 1];
			UINT NumSemantics = 0;
			D3DXGetShaderInputSemantics( passDesc.pVertexShaderFunction, Semantics, &NumSemantics);
			for( unsigned int s = 0; s < NumSemantics; s++)
			{
				const SemanticInfo& Info = gSemanticInfo[ Semantics[ s].Usage];
				MString Name = Info.Name; // Can't get the name through DX so use a semantic based name =(
				if( Info.Type != MVaryingParameter::kPosition && Info.Type != MVaryingParameter::kNormal)
					Name += Semantics[ s].UsageIndex;
				MVaryingParameter varying( Name, MVaryingParameter::kFloat, Info.MinElements, Info.MaxElements, Info.Type, Info.Type == MVaryingParameter::kTexCoord ? true : false);
				fVertexStructure.addElement( varying);

				VertexElements[ s].Stream = 0;
				VertexElements[ s].Offset = (WORD)offset;
				VertexElements[ s].Type = Info.D3DType;
				VertexElements[ s].Method = D3DDECLMETHOD_DEFAULT;
				VertexElements[ s].Usage = (BYTE)Semantics[ s].Usage;
				VertexElements[ s].UsageIndex = (BYTE)Semantics[ s].UsageIndex;
				offset += varying.getMaximumStride();
			}
			static D3DVERTEXELEMENT9 VertexElementEnd = D3DDECL_END();
			VertexElements[ NumSemantics] = VertexElementEnd;
			
			// Make sure that the point is initialized in case something fails
			fD3DVertexDeclaration[p] = NULL;

			MD3D9Renderer* pRenderer = MD3D9Renderer::theRenderer();
			if (pRenderer != NULL) {
				IDirect3DDevice9* d3dDevice = pRenderer->getD3D9Device();
				if( d3dDevice) {
					d3dDevice->CreateVertexDeclaration( VertexElements, &fD3DVertexDeclaration[ p]);
				}
			}
		}
		MVaryingParameterList list;
		if( fVertexStructure.numElements() > 0)
		{
			// Only add our parameters if we have some - otherwise the empty structure
			// shows up in the UI which is a bit ugly
			list.append( fVertexStructure);
		}
		setVaryingParameters( list);
	}

	return MS::kSuccess;
}


// Error reporting
void hlslShader::reportInternalError( const char* function, size_t errcode )
{
	MString es = "hlslShader";

	try
	{
		if ( this )
		{
			if ( ++fErrorCount > ERROR_LIMIT ) 
				return;
			MString s;
			s += "\"";
			s += name();
			s += "\": ";
			s += typeName();
			es = s;
		}
	}
	catch ( ... )
	{}
	es += " internal error ";
	es += (int)errcode;
	es += " in ";
	es += function;
	MGlobal::displayError( es );
}                                      // hlslShader::reportInternalError


void parameterRequirements(const MVaryingParameter& parameter, MGeometryRequirements& requirements)
// recursive requirements 
{
	if (parameter.numElements() == 0) {

		switch( parameter.semantic())
		{

		case (MVaryingParameter::kTexCoord) :
			requirements.addTexCoord(parameter.name());	
			break;
		case (MVaryingParameter::kPosition) :
			requirements.addPosition();
			break;
		case (MVaryingParameter::kNormal) :
			requirements.addNormal();
			break;
		case (MVaryingParameter::kColor) :
			requirements.addColor(parameter.name());
			break;
		default:
			// don't know what it is.
			break;
		}
	}
	else {
		for (unsigned int i = 0; i < parameter.numElements(); ++i) {
			parameterRequirements(parameter.getElement(i), requirements);
		}
	}
}
 

/* virtual */
MStatus hlslShader::renderSwatchImage(MImage& image)
// Override this method to draw a image for swatch rendering.
///
{

// #define _USE_OPENGL_
#if defined _USE_OPENGL_	// 

	MStatus status = MStatus::kFailure;

	// Get the hardware openGL renderer utility class
	MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
	if (pRenderer)
	{
		const MString& backEndStr = pRenderer->backEndString();

		// get the geometry requirements
		MDagPath path;							// NULL path
		MGeometryRequirements requirements;
		requirements.addPosition();
		requirements.addNormal();
		requirements.addTexCoord(MString("map1"));	
		requirements.addComponentId();

		MGeometryManager::GeometricShape shape = MGeometryManager::kDefaultSphere;
		MGeometryList* geomIter = MGeometryManager::referenceDefaultGeometry(shape, requirements);

		if(geomIter == NULL) {
			return MStatus::kFailure;
		}

		// Make the swatch context current
		// ===============================
		//
		unsigned int width, height;
		image.getSize(width, height);

		MStatus status2 = pRenderer->makeSwatchContextCurrent( backEndStr, width, height );

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		if( status2 != MS::kSuccess ) {
			MGeometryManager::dereferenceDefaultGeometry(geomIter);
		}

		// Get the light direction from the API, and use it
		// =============================================
		{    
			static float  specular[] = { 0.7f, 0.7f, 0.7f, 1.0f};
			static float  shine[] = { 100.0f };
			static float  ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };

			glEnable(GL_LIGHTING);
			glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shine);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);

			float lightPos[4];
			pRenderer->getSwatchLightDirection( lightPos[0], lightPos[1], lightPos[2], lightPos[3] );

			static float default_ambient_light[]={0.4f, 0.4f, 0.4f, 1.0f};
			static float default_diffuse_light[]={0.8f, 0.8f, 0.8f, 1.0f};
			static float default_specular_light[]={1.0f, 1.0f, 1.0f, 1.0f};

			glLightfv(GL_LIGHT0, GL_AMBIENT, default_ambient_light);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, default_diffuse_light);
			glLightfv(GL_LIGHT0, GL_SPECULAR, default_specular_light);

			glPushMatrix();
			glLoadIdentity();
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
			glEnable(GL_LIGHT0);
			glPopMatrix();

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

			// want to store these into the 
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

		for( ; !geomIter->isDone(); geomIter->next())
		{

			MGeometry& geometry = geomIter->geometry(MGeometryList::kNone);
			const MMatrix& mtm = geomIter->objectToWorldMatrix();

			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixd(mtm[0]);

			// TO DO the geomIterator needs to give up the project matrix.

			MGeometryPrimitive primitives = geometry.primitiveArray(0);

			const MGeometryData pos = geometry.position();
			const MGeometryData normal = geometry.normal();

			// Draw The Swatch

			if (pos.data() != NULL && primitives.data() != NULL) {

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, (float*) pos.data());

				if (normal.data()) {
					glEnableClientState(GL_NORMAL_ARRAY);
					glNormalPointer(GL_FLOAT, 0,  normal.data());
				}

				// get the first texture and blind it
				bool boundTexture = false;
				MString defaultMap("map1");
				MGeometryData uvCoord = geometry.texCoord(defaultMap);

				// Setup our shape dependent parameters
				for( int u = fUniformParameters.length(); u--; ){
					MUniformParameter uniform = fUniformParameters.getElement( u);

					if ( uniform.isATexture()) {
						// make sure that it a 2d texture as we can't handle anything else.
						if( uniform.type() == MUniformParameter::kType2DTexture) 
						{
							// Get the plug used for the first texture - but be careful to preserve
							// the hasChanged state so that the shader correctly updates this parameter
							// (otherwise it may never setup the texture parameter because we suppressed
							// the initial hasChanged trigger)
							bool wasDirty = uniform.hasChanged( *geomIter);
							MPlug plug = uniform.getSource();
							if( wasDirty) uniform.setDirty();

							MImageFileInfo::MHwTextureType hwType;
							if (MS::kSuccess == MHwTextureManager::glBind(plug, hwType))
							{
								boundTexture = true;
								break;
							}
						}
					}
				}

				if (uvCoord.data() && boundTexture) {

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0, uvCoord.data());

					// Base colour is always white
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				}
				else {

					glDisable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);
					glColor4f(0.1f, 0.7f, 0.7f, 1.0f);
				}

				MGeometryPrimitive primitives = geometry.primitiveArray(0);

				// renderer
				glDrawElements(GL_TRIANGLES, primitives.elementCount(), GL_UNSIGNED_INT, primitives.data());

				// Read pixels back from swatch context to MImage
				pRenderer->readSwatchContextPixels( backEndStr, image );

				// Double check the outing going image size as image resizing
				// was required to properly read from the swatch context
				image.getSize( width, height );

				glPopAttrib();
				glPopClientAttrib();

				status = MStatus::kSuccess;
			}
		}

		MGeometryManager::dereferenceDefaultGeometry(geomIter);
	}

#else

	MStatus status = MStatus::kFailure;

	unsigned int width, height;
	image.setRGBA(true);
	image.getSize(width, height);

	// get the geometry requirements
	ShaderContext context;					// NULL path and shading engine
	MGeometryRequirements requirements;
	populateRequirements(context, requirements);

	MD3D9Renderer*  Render = MD3D9Renderer::theRenderer();
	if (Render != NULL) {

		Render->makeSwatchContextCurrent(width, height);

		// render the back ground
		Render->setBackgroundColor(MColor(0.1f, 0.1f, 0.1f));

		MGeometryList* geomIter = MGeometryManager::referenceDefaultGeometry(MGeometryManager::kDefaultSphere, 
			requirements);

		if (geomIter == NULL) {
			return status;
		}

		// set up lighting
		// TO DO

		// render the object this the shader and defaulting geometry
		if (render(*geomIter) != MStatus::kSuccess) {
			// revert to a simple fixed color material render
			// TO DO
		}

		// read back the image
		Render->readSwatchContextPixels(image);

		// let go of the geoemtry
		MGeometryManager::dereferenceDefaultGeometry(geomIter);

		status = MStatus::kSuccess;
	}

#endif

	return status;
}
