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

#include <stdio.h>
#include <iostream>

#include <D3DViewportRenderer.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MRenderingInfo.h>
#include <maya/MRenderTarget.h>
#include <maya/MFnCamera.h>
#include <maya/MAngle.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MBoundingBox.h>
#include <maya/MImage.h>
#include <maya/MDrawTraversal.h>
#include <maya/MGeometryManager.h>
#include <maya/MGeometry.h>
#include <maya/MGeometryData.h>
#include <maya/MGeometryPrimitive.h>
#include <maya/MNodeMessage.h> // For monitor geometry list
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFnSet.h>
#include <maya/MFnNumericData.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MMatrix.h>

#include <stdio.h>
#include <maya/MFnLight.h>
#include <maya/MFnSpotLight.h>

#include <maya/MPxHardwareShader.h>
#include <maya/MRenderProfile.h>

#if defined(D3D9_SUPPORTED)

// Screen space quad vertex
struct ScreenSpaceVertex
{
    D3DXVECTOR4 position; // position
    D3DXVECTOR2 texCoord; // texture coordinate

    static const DWORD FVF;
};
const DWORD ScreenSpaceVertex::FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;

//////////////////////////////////

//
// Class : D3DViewportRenderer
//
// Very simple renderer using D3D to render to an offscreen render target.
// The contents are read back into system memory to blit into an OpenGL context.
//
// This example has been test compiled against the both the Feb. and April 2006 
// DirectX developer SDKs. Define the D3D9_SUPPORTED preprocessor directive 
// to compile D3D code in.
//
// These code items are work in progress:
//
// - camera is fixed to be perspective. No orthographic cameras yet.
// - offscreen surface is fixed in size.
// - surfaces can either be fixed RGBA8888 or floating point 16. Final output
//   is always fixed, though post-process tone-mapping can be applied before
//	 final output.
// - post-process full screen effects are restricted to those which only require
//   color as input.
// - does not handle loss of device.
// - readback is for color only, no depth readback currently.
// - basic Maya material support in a fixed-function pipeline.
// - geometry support for polys for shaded with file texture on color channel.
//
//
#endif

D3DViewportRenderer::D3DViewportRenderer()
:	MViewportRenderer("D3DViewportRenderer")
{
	// Set the ui name
	fUIName.set( "Direct3D Renderer");

	// This renderer overrides all drawing
	fRenderingOverride = MViewportRenderer::kOverrideAllDrawing;

	// Set API and version number
	m_API = MViewportRenderer::kDirect3D;
	m_Version = 9.0f;

	// Default to something reasonable.
	m_renderWidth = 640;
	m_renderHeight = 480;

#if defined(D3D9_SUPPORTED)
	m_hWnd = 0;
	m_pD3D = 0;
	m_pD3DDevice = 0;
	m_pTextureOutput = 0;
	m_pTextureOutputSurface = 0;
	
	m_readBackBuffer.create(m_renderWidth, m_renderHeight, 4/* MPixelType type = kByte */);
	m_readBackBuffer.setRGBA( false );

	m_pBoundsBuffer = 0;
	m_pGeometry = 0;

	m_wantFloatingPointTargets = false;
	m_pTextureInterm = 0;
	m_pTextureIntermSurface = 0;
	m_pTexturePost = 0;

	m_pDepthStencilSurface = 0;
	m_SystemMemorySurface = 0;

	m_requireDepthStencilReadback = false;
#endif
}

/* virtual */
D3DViewportRenderer::~D3DViewportRenderer()
{
	uninitialize();
}

// Dummy window proc.
LRESULT CALLBACK D3DWindowProc( HWND   hWnd, 
							 UINT   msg, 
							 WPARAM wParam, 
							 LPARAM lParam )
{
    switch( msg )
	{
 		case WM_CLOSE:
		{
			//PostQuitMessage(0);	-- can't allow this. Will kill Maya
		}
		break;

        case WM_DESTROY:
		{
            //PostQuitMessage(0);	-- can't allow this. Will kill Maya
		}
        break;
		
		default:
		{
			return DefWindowProc( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return 0;
}

#if defined(D3D9_SUPPORTED)
bool
D3DViewportRenderer::buildRenderTargets(unsigned int width, unsigned int height)
{
	HRESULT hr = -1;

	// Nothing to do, just return
	if (width == m_renderWidth &&
		height == m_renderHeight &&
		m_pTextureInterm && 
		m_pTextureOutput &&
		m_pTexturePost)
	{
		return true;
	}

	// Set the new width and height
	m_renderWidth = width;
	m_renderHeight = height;

	//printf("New size = %d,%d\n", m_renderWidth, m_renderHeight);

	//
	// Create target for intermediate rendering
	//
	if (m_pTextureInterm)
	{
		m_pTextureInterm->Release();
		m_pTextureInterm = NULL;
	}
	if (m_pTextureIntermSurface)
	{
		m_pTextureIntermSurface->Release();
		m_pTextureIntermSurface = NULL;
	}
	if (!m_pTextureInterm)
	{
		hr = D3DXCreateTexture( m_pD3DDevice, 
			m_renderWidth, 
			m_renderHeight, 
			1, 
			D3DUSAGE_RENDERTARGET, 
			m_intermediateTargetFormat, /* Use intermediate target format */
			D3DPOOL_DEFAULT, 
			&m_pTextureInterm );

		// Failed to get target with desired intermediate format. Try for
		// fixed as a default
		m_intermediateTargetFormat = m_outputTargetFormat;
		if ( FAILED(hr) )
		{
			hr = D3DXCreateTexture( m_pD3DDevice, 
				m_renderWidth, 
				m_renderHeight, 
				1, 
				D3DUSAGE_RENDERTARGET, 
				m_intermediateTargetFormat, /* Use output target format */
				D3DPOOL_DEFAULT, 
				&m_pTextureInterm );
		}
		if ( FAILED(hr) )
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to create intermediate texture for offscreen render target.");
			return false;
		}
	}
	if (m_pTextureInterm)
	{
		hr = m_pTextureInterm->GetSurfaceLevel( 0, &m_pTextureIntermSurface );
	}
	if ( FAILED(hr) )
	{
		MGlobal::displayWarning("Direct3D renderer : Failed to get surface for off-screen render target.");
		return false;
	}


	//
	// 2. Create output render targets
	//
	// If we don't want floating point, then the intermediate is
	// the final output format, so don't bother creating another one.
	// Just make the output point to the intermediate target.
	if (m_wantFloatingPointTargets)
	{
		if (m_pTextureOutput)
		{
			m_pTextureOutput->Release();
			m_pTextureOutput = NULL;
		}
		if (m_pTextureOutputSurface)
		{
			m_pTextureOutputSurface->Release();
			m_pTextureOutputSurface = NULL;
		}

		if (!m_pTextureOutput)
		{
			// Create texture for render target
			hr = D3DXCreateTexture( m_pD3DDevice, 
				m_renderWidth, 
				m_renderHeight, 
				1, 
				D3DUSAGE_RENDERTARGET, 
				m_outputTargetFormat, 
				D3DPOOL_DEFAULT, 
				&m_pTextureOutput );

			if ( FAILED(hr) )
			{
				MGlobal::displayWarning("Direct3D renderer : Failed to create texture for offscreen render target.");
				return false;
			}
		}

		// Get the surface (0) for the texture. Could probably do this one
		// per refresh and not keep it around...	
		if (m_pTextureOutput)
		{
			hr = m_pTextureOutput->GetSurfaceLevel( 0, &m_pTextureOutputSurface );
		}
		if ( FAILED(hr) )
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to get surface for off-screen render target.");
			return false;
		}
	}
	else
	{
		m_pTextureOutput = m_pTextureInterm;
		m_pTextureOutputSurface = m_pTextureIntermSurface;
	}

	//
	// 3. Create post-process render targets
	//
	if (m_pTexturePost)
	{
		m_pTexturePost->Release();
		m_pTexturePost = NULL;
	}
	if (!m_pTexturePost)
	{
		hr = D3DXCreateTexture( m_pD3DDevice, 
			m_renderWidth, 
			m_renderHeight, 
			1, 
			D3DUSAGE_RENDERTARGET, 
			m_intermediateTargetFormat, /* Use intermediate target format */
			D3DPOOL_DEFAULT, 
			&m_pTexturePost );

		if ( FAILED(hr) )
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to create texture for offscreen post-processing.");
			return false;
		}
	}

	// 4. Create system memory surface for readback
	if (m_SystemMemorySurface)
	{
		m_SystemMemorySurface->Release();
		m_SystemMemorySurface = 0;
	}
	if (!m_SystemMemorySurface)
	{
		hr = m_pD3DDevice->CreateOffscreenPlainSurface( m_renderWidth,
			m_renderHeight, m_outputTargetFormat, D3DPOOL_SYSTEMMEM,
			&m_SystemMemorySurface, NULL );
		if (FAILED(hr))
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to create system memory readback surface.");
			return false;
		}
	}

	return (m_pTextureOutput && m_pTextureOutputSurface && m_pTextureInterm && 
			m_pTextureIntermSurface && m_pTexturePost && m_SystemMemorySurface);

#if defined(DEPTH_REQUIRED)
	// 5. Create depth stencil surface for access for readback.
	if (m_pDepthStencilSurface)
	{
		m_pDepthStencilSurface->Release();
		m_pDepthStencilSurface = 0;
	}
	if (m_requireDepthStencilReadback && !m_pDepthStencilSurface)
	{
		hr = m_pD3DDevice->CreateDepthStencilSurface( 
						m_renderWidth, m_renderHeight, m_depthStencilFormat, D3DMULTISAMPLE_NONE, 
						0, FALSE, 
						&m_pDepthStencilSurface, NULL );
		if (FAILED(hr))
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to create depth/stencil surface. Depth read back will not be available.");
		}
	}
#endif
}
#endif

/* virtual */	
MStatus	
D3DViewportRenderer::initialize()
{
	MStatus status = MStatus::kFailure;

#if defined(D3D9_SUPPORTED)

	// Do we want floating point targets
	//
	MString wantFloatingPoint("D3D_RENDERER_FLOAT_TARGETS");
	int value;
	if (!MGlobal::getOptionVarValue(wantFloatingPoint, value))
	{
		m_wantFloatingPointTargets = true;
	}
	else
	{
		m_wantFloatingPointTargets = (value != 0);
	}
	m_wantFloatingPointTargets = false;

	// Create the window to contain our off-screen target.
	//
	if (!m_hWnd)
	{
		// Register the window class
		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, (WNDPROC) D3DWindowProc, 0L, 0L, 
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					  "D3D Viewport Renderer", NULL };
		if (RegisterClassEx( &wc ))
		{
			m_hWnd = CreateWindow( "D3D Viewport Renderer", "D3D Viewport Renderer", 
									WS_OVERLAPPEDWINDOW, 0, 0, m_renderWidth, m_renderHeight,
									NULL, NULL, wc.hInstance, NULL );
		}
	}

	// Startup D3D
	if (m_hWnd)
	{
		if (!m_pD3D)
			m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	}

	HRESULT hr;

	// Test for floating point buffer usage for render targets
	if (m_wantFloatingPointTargets)
	{
		m_intermediateTargetFormat = D3DFMT_A16B16G16R16F;
	}
	else
	{
		m_intermediateTargetFormat = D3DFMT_A8R8G8B8;
	}
	// The output target is always fixed8 for now.
	m_outputTargetFormat = D3DFMT_A8R8G8B8;
	if (m_requireDepthStencilReadback)
	{
		m_depthStencilFormat = D3DFMT_D32; // Let's try for 32-bit depth, not stencil
	}
	else
		m_depthStencilFormat = D3DFMT_D24S8; 

	// Create an appropriate device
	if (m_pD3D)
	{
		if (!m_pD3DDevice)
		{
			D3DPRESENT_PARAMETERS d3dpp; 
			ZeroMemory( &d3dpp, sizeof(d3dpp) );

			d3dpp.BackBufferFormat		 = m_outputTargetFormat;
			d3dpp.Windowed				 = TRUE; // Don't want full screen
			d3dpp.BackBufferWidth        = 1920; // Make it big enough to avoid clipping.
			d3dpp.BackBufferHeight       = 1680;
			d3dpp.SwapEffect			 = D3DSWAPEFFECT_DISCARD;
			d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
			d3dpp.EnableAutoDepthStencil = TRUE;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8; 

			// Try hardware vertex processing first.
			//
			hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
										D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
										&d3dpp, &m_pD3DDevice );
			
			// Try software if we can't find hardware.
			if (FAILED(hr))
			{
				hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
										D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
										&d3dpp, &m_pD3DDevice );
			}
			if ( FAILED(hr) )
				m_pD3DDevice = 0;
		}
	}

	// Turn on Z buffer.
	if (m_pD3DDevice)
	{
		m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

		bool builtRenderTargets = buildRenderTargets(640, 480);	
		if (builtRenderTargets)
		{
			MString shaderLocation(MString(getenv("MAYA_LOCATION")) + MString("\\bin\\HLSL"));

			// Load in any post effects from a given directory
			bool loaded = m_resourceManager.initializePostEffects( shaderLocation, m_pD3DDevice );

			// Load in default surface effect from a given directory
			const MString defaultSurfaceEffect("Maya_fixedFunction");
			loaded = m_resourceManager.initializeDefaultSurfaceEffect( shaderLocation, m_pD3DDevice, defaultSurfaceEffect);
			//printf("Loaded Maya fixed function = %d\n", loaded);

			// All elements must exist for success
			if (m_hWnd && m_pD3D && m_pD3DDevice && m_pTextureOutput && m_pTextureOutputSurface && 
				m_pTextureInterm && m_pTextureIntermSurface )
			{
				status = MStatus::kSuccess;
			}
		}
	}

	// If for any reason we failed. Cleanup what we can.
	if (status != MStatus::kSuccess)
	{
		uninitialize();
	}
#else
	status = MStatus::kSuccess;
#endif

	return status;
}

/* virtual */	
MStatus	
D3DViewportRenderer::uninitialize()
{	
#if defined(D3D9_SUPPORTED)
	// 
	// Cleanup D3D items
	//
	if ( m_pTextureOutput )
	{
		if (m_pTextureOutput != m_pTextureInterm)		
			m_pTextureOutput->Release();
		m_pTextureOutput = 0;
	}

	if ( m_pTextureOutputSurface )
	{
		if (m_pTextureOutputSurface != m_pTextureIntermSurface)
			m_pTextureOutputSurface->Release();
		m_pTextureOutputSurface = 0;
	}

	if ( m_pTextureInterm )
	{
        m_pTextureInterm->Release();
		m_pTextureInterm = 0;
	}

	if ( m_pTextureIntermSurface )
	{
        m_pTextureIntermSurface->Release();
		m_pTextureIntermSurface = 0;
	}

	if ( m_pTexturePost )
	{
        m_pTexturePost->Release();
		m_pTexturePost = 0;
	}

	if ( m_SystemMemorySurface )
	{
		m_SystemMemorySurface->Release();
		m_SystemMemorySurface = 0;
	}
	if (m_pDepthStencilSurface)
	{
		m_pDepthStencilSurface->Release();
		m_pDepthStencilSurface = 0;
	}

	if ( m_pBoundsBuffer != NULL )
	{
        m_pBoundsBuffer->Release(); 
		m_pBoundsBuffer = 0;
	}
	if (m_pGeometry)
	{
		m_pGeometry->Release();
		m_pGeometry = 0;
	}
	m_resourceManager.clearResources(false, true); /* wipe out shaders */

    if ( m_pD3DDevice )
	{
        m_pD3DDevice->Release();
		m_pD3DDevice = 0;
	}

    if ( m_pD3D )
	{
        m_pD3D->Release();	
		m_pD3D = 0;
	}

	// Clean up windowing items.
	//
	if (m_hWnd)
	{
		ReleaseDC( m_hWnd, GetDC(m_hWnd ));
		DestroyWindow(m_hWnd);
		UnregisterClass("D3D Viewport Renderer", GetModuleHandle(NULL));
		m_hWnd = 0;
	}

#endif
	return MStatus::kSuccess;
}

/* virtual */ 
MStatus	
D3DViewportRenderer::render(const MRenderingInfo &renderInfo)
{
	MStatus status;

	// Print some diagnostic information.
	//

#if defined(D3D9_SUPPORTED)
	const MRenderTarget & target = renderInfo.renderTarget();
	unsigned int currentWidth = target.width();
	unsigned int currentHeight = target.height();
	
	if (!buildRenderTargets(currentWidth, currentHeight))
		return MStatus::kFailure;
#endif

	//printf("Render using (%s : %s) renderer\n", fName.asChar(), fUIName.asChar());
	//printf("Render region: %d,%d -> %d, %d into target of size %d,%d\n", 
	//	renderInfo.originX(), renderInfo.originY(), renderInfo.width(), renderInfo.height(),
	//	target.width(), target.height() );


#if defined(D3D9_SUPPORTED)
	MViewportRenderer::RenderingAPI targetAPI = renderInfo.renderingAPI();
	//float targetVersion = renderInfo.renderingVersion();
	//printf("Render target API is %s (Version %g)\n", targetAPI == MViewportRenderer::kDirect3D ?
	//		"Direct3D" : "OpenGL", targetVersion);

	// Render if we get a valid camera
	const MDagPath &cameraPath = renderInfo.cameraPath();
	if ( m_resourceManager.translateCamera( cameraPath ) )
	{
		if ( renderToTarget( renderInfo ) )
		{
			// Read back results and set into an intermediate buffer,
			// if the target is not Direct3D. Also readback if we
			// want to debug the buffer.
			//
			bool requireReadBack = (targetAPI != MViewportRenderer::kDirect3D);
			if ( requireReadBack )
			{
				if (readFromTargetToSystemMemory())
				{
					// Blit image back to OpenGL 
					if (targetAPI == MViewportRenderer::kOpenGL)
					{
						// Center the image for now.
						//
						unsigned int targetW = target.width();
						unsigned int targetH = target.height();							
						unsigned int m_readBackBufferWidth, m_readBackBufferHeight;
						m_readBackBuffer.getSize(m_readBackBufferWidth, m_readBackBufferHeight);

						if (m_readBackBufferWidth && m_readBackBufferHeight)
						{
							if (m_readBackBufferWidth > targetW ||
								m_readBackBufferHeight > targetH)
							{
								m_readBackBuffer.resize(targetW, targetH);
								target.writeColorBuffer( m_readBackBuffer, 0, 0 );
							}
							else
							{
								target.writeColorBuffer( m_readBackBuffer, 
									(short)(targetW/2 - m_readBackBufferWidth/2),
									(short)(targetH/2 - m_readBackBufferHeight/2));
							}
							status = MStatus::kSuccess;
						}
					}

					// Blit image back to a software raster
					else
					{
						// To ADD
						status = MStatus::kFailure;
					}
				}
				else
					status = MStatus::kFailure;				
			}

			// Do nothing here. Direct rendering to D3D target
			// should be handled in renderToTarget().
			else 
			{
				status = MStatus::kSuccess;
			}
		}
		else
			status = MStatus::kFailure;
	}
	else
	{
		MGlobal::displayWarning("Direct3D renderer : No valid render camera to use. Nothing rendered\n");
		status = MStatus::kFailure;
	}
#else
		status = MStatus::kSuccess;
#endif
	return status;
}

/* virtual */ 
bool	
D3DViewportRenderer::nativelySupports( MViewportRenderer::RenderingAPI api, 
									   float version )
{
	// Do API and version check
	return ((api == m_API) && (version == m_Version) );
}

/* virtual */ bool	
D3DViewportRenderer::override( MViewportRenderer::RenderingOverride override )
{
	// Check override
	return (override == fRenderingOverride);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(D3D9_SUPPORTED)

bool					
D3DViewportRenderer::translateCamera( const MRenderingInfo &renderInfo )
//
// Description:
//		Translate Maya's camera 
//
{
	const MDagPath &cameraPath = renderInfo.cameraPath();
	if (cameraPath.isValid())
		return m_resourceManager.translateCamera( cameraPath );
	else
		return false;
}

bool D3DViewportRenderer::drawCube(float minVal[3], float maxVal[3], bool filled, bool useDummyGeometry,
								   float color[3], LPDIRECT3DVERTEXBUFFER9 buffer /* = NULL */)
//
// Description:
//		Draw a rectangular bounds of min->max size.
//
{
	if (!m_pD3DDevice)
		return false;

	D3DMATERIAL9 Material;
	Material.Emissive.r = color[0]; 
	Material.Emissive.g = color[1]; 
	Material.Emissive.b = color[2]; 
	Material.Emissive.a = 1.0f; 
	Material.Ambient.r = color[0]; 
	Material.Ambient.g = color[1]; 
	Material.Ambient.b = color[2]; 
	Material.Ambient.a = 1.0f; 
	m_pD3DDevice->SetMaterial( &Material);
	m_pD3DDevice->LightEnable( 0, false);
	m_pD3DDevice->LightEnable( 1, false);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	// Set up render state
	//
	if (!filled)
	{
		m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	}
	else
	{
		m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );		
	}
	m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

	// 
	if (!m_pGeometry)
	{
		if (useDummyGeometry)
		{
			//D3DXCreateSphere(m_pD3DDevice, 1.0f, 20, 20, &m_pGeometry, NULL);
			//D3DXCreateTeapot(m_pD3DDevice, &m_pGeometry, NULL);
			//D3DXCreateBox(m_pD3DDevice, 1.0, 1.0, 1.0, &m_pGeometry, NULL);
		}
	}
	if (useDummyGeometry && m_pGeometry)
	{
		m_pGeometry->DrawSubset(0);
		return true;
	}

	// Build a vertex buffer to hold a cube once.
	//
	LPDIRECT3DVERTEXBUFFER9 bufferToFill = m_pBoundsBuffer;
	if (!bufferToFill)
	{
		m_pD3DDevice->CreateVertexBuffer( 24*sizeof(PlainVertex),0, PlainVertex::FVF_Flags,
											D3DPOOL_DEFAULT, &bufferToFill, NULL );
	}
	if (!bufferToFill)
	{
		return false;
	}
	
	PlainVertex cube[] =
	{
		{ minVal[0], maxVal[1], minVal[2]},
		{ maxVal[0], maxVal[1], minVal[2]},
		{ minVal[0], minVal[1], minVal[2] },
		{ maxVal[0], minVal[1], minVal[2] },

		{minVal[0], maxVal[1], maxVal[2] },
		{minVal[0],minVal[1], maxVal[2] },
		{ maxVal[0], maxVal[1], maxVal[2] },
		{ maxVal[0],minVal[1], maxVal[2] },

		{minVal[0], maxVal[1], maxVal[2] },
		{ maxVal[0], maxVal[1], maxVal[2] },
		{minVal[0], maxVal[1],minVal[2] },
		{ maxVal[0], maxVal[1],minVal[2] },

		{minVal[0],minVal[1], maxVal[2] },
		{minVal[0],minVal[1],minVal[2] },
		{ maxVal[0],minVal[1], maxVal[2] },
		{ maxVal[0],minVal[1],minVal[2] },

		{ maxVal[0], maxVal[1],minVal[2] },
		{ maxVal[0], maxVal[1], maxVal[2] },
		{ maxVal[0],minVal[1],minVal[2] },
		{ maxVal[0],minVal[1], maxVal[2] },

		{minVal[0], maxVal[1],minVal[2] },
		{minVal[0],minVal[1],minVal[2] },
		{minVal[0], maxVal[1], maxVal[2] },
		{minVal[0],minVal[1], maxVal[2] }
	};

	void *pVertices = NULL;

	// Do this everytime as we don't store more than one box,
	// and we don't check for changes in bounds sizes.
	//
	bufferToFill->Lock( 0, sizeof(cube), (void**)&pVertices, 0 );
	memcpy( pVertices, cube, sizeof(cube) );
	bufferToFill->Unlock();

    m_pD3DDevice->SetStreamSource( 0, bufferToFill, 0, sizeof(PlainVertex) );
    m_pD3DDevice->SetFVF( PlainVertex::FVF_Flags );


	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );
	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  4, 2 );
	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  8, 2 );
	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
	m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );

	m_pD3DDevice->LightEnable( 0, true);
	m_pD3DDevice->LightEnable( 1, true);

	return true;
}

void					
D3DViewportRenderer::clearResources(bool onlyInvalidItems, bool clearShaders)
{
	m_resourceManager.clearResources( onlyInvalidItems, clearShaders );
}

MObject findShader( MObject& setNode )
//
//  Description:
//      Find the shading node for the given shading group set node.
//
{
	MFnDependencyNode fnNode(setNode);
	MPlug shaderPlug = fnNode.findPlug("surfaceShader");
			
	if (!shaderPlug.isNull()) {			
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );

		if (connectedPlugs.length() != 1)
			MGlobal::displayError("Error getting shader");
		else 
			return connectedPlugs[0].node();
	}			
	
	return MObject::kNullObj;
}

bool D3DViewportRenderer::setSurfaceMaterialShader( const MDagPath &dagPath, D3DGeometry* Geometry,
												   	const D3DXMATRIXA16 &objectToWorld, 
												   	const D3DXMATRIXA16 &objectToWorldInvTranspose, 
													const D3DXMATRIXA16 &worldViewProjection,
													const D3DXVECTOR4 &worldEyePosition)
{
	if (!Geometry)
		return false;

	const SurfaceEffectItemList & surfaceEffects = m_resourceManager.getSurfaceEffectItemList();
	bool havePixelShader = (surfaceEffects.size() > 0);
	if (havePixelShader)
	{
		// Default is always the first shader !
		SurfaceEffectItem *defaultItem = surfaceEffects.front();
		if (defaultItem)
		{
			ID3DXEffect* effect = defaultItem->fEffect;
			if (effect)
			{
				//printf("Setup effects technique...\n");
				HRESULT hres = effect->SetTechnique("MayaPhong");
				if (hres != D3D_OK)
					havePixelShader = false;
				else
				{
					//printf("Set up effect parameters\n");
					D3DXHANDLE handle;
					handle = effect->GetParameterBySemantic( NULL, "WorldView" );
					effect->SetMatrix( handle, &objectToWorld );
					handle = effect->GetParameterBySemantic( NULL, "WorldViewInverseTranspose" );
					effect->SetMatrix( handle, &objectToWorldInvTranspose );
					handle = effect->GetParameterBySemantic( NULL, "WorldViewProjection" );
					effect->SetMatrix( handle, &worldViewProjection );

					// Required for specular lighting.
					//effect->SetVector( "worldEyePosition", &worldEyePosition );

					// Setup lighting parameters
					MItDag dagIterator( MItDag::kDepthFirst, MFn::kLight );
					MDagPath lightPath;
					for (; !dagIterator.isDone(); dagIterator.next())
					{
						if ( !dagIterator.getPath(lightPath) )
							continue;
						MFnLight    fnLight( lightPath );
						MTransformationMatrix worldMatrix = lightPath.inclusiveMatrix();

						MVector translation = worldMatrix.translation( MSpace::kWorld );
						MVector direction( 0.0, 0.0, 1.0 ); 
						direction *= worldMatrix.asMatrix();
						direction.normalize();

						// Hard coded for directional only currently
						D3DXVECTOR4 e_val((float)direction.x, (float)direction.y, (float)direction.z, 1.0f );
						hres = effect->SetVector( "lightDir", &e_val );

						MColor      colorVal = fnLight.color();
						float intensity = fnLight.intensity();

						D3DXVECTOR4 c_val( colorVal.r * intensity, colorVal.g * intensity, colorVal.b * intensity, 1.0f );
						hres = effect->SetVector( "lightColor", &c_val );
					}
	
					// Setup material parameters
					//
					bool isTransparent = false;
					MFnMesh fnMesh(dagPath);
					MObjectArray sets;
					MObjectArray comps;
					unsigned int instanceNum = dagPath.instanceNumber();
					if (!fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true))
						MGlobal::displayError("ERROR: MFnMesh::getConnectedSetsAndMembers");
					if (sets.length())
					{
						MObject set = sets[0];
						MObject comp = comps[0];

						MStatus status;
						MObject shaderNode = findShader(set);
						if (shaderNode != MObject::kNullObj)
						{								
							float rgb[3];

							MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
							D3DTexture* Texture = NULL;
							if (status != MS::kFailure)
							{
								MItDependencyGraph It( colorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream);
								if( !It.isDone())
								{
									Texture = m_resourceManager.getTexture( It.thisNode());
									hres = effect->SetTexture( "diffuseTexture", Texture->Texture( m_pD3DDevice ));
									D3DXVECTOR4 e_val(1.0f, 1.0f, 1.0f, 1.0f );
									hres = effect->SetVector( "diffuseMaterial", &e_val);

									// Change the technique to a textured one
									hres = effect->SetTechnique("MayaPhongTextured");
								}
								else
								{
									MObject data;
									colorPlug.getValue( data);
									MFnNumericData val(data);
									val.getData( rgb[0], rgb[1], rgb[2]);
									
									D3DXVECTOR4 e_val(rgb[0], rgb[1], rgb[2], 1.0f );
									hres = effect->SetVector( "diffuseMaterial", &e_val);
								}
							}

							MPlug diffusePlug = MFnDependencyNode(shaderNode).findPlug("diffuse", &status);
							if (status != MS::kFailure)
							{
								MObject data;
								float diff;
								diffusePlug.getValue( diff );
								//Material.Diffuse.r *= (float)diff;
								//Material.Diffuse.g *= (float)diff; 
								//Material.Diffuse.b *= (float)diff;
							}
							MPlug ambientColorPlug = MFnDependencyNode(shaderNode).findPlug("ambientColor", &status);
							if (status != MS::kFailure)
							{
								MObject data;
								ambientColorPlug .getValue( data);
								MFnNumericData val(data);
								val.getData( rgb[0], rgb[1], rgb[2]);

								D3DXVECTOR4 e_val( rgb[0], rgb[1], rgb[2], 1.0f );
								effect->SetVector( "ambientMaterial", &e_val );
							}
							MPlug transparencyPlug = MFnDependencyNode(shaderNode).findPlug("transparency", &status);
							if (status != MS::kFailure)
							{
								MObject data;
								transparencyPlug.getValue( data);
								MFnNumericData val(data);
								val.getData( rgb[0], rgb[1], rgb[2]);

								D3DXVECTOR4 e_val( 1.0f - rgb[0], 1.0f - rgb[1], 1.0f - rgb[2], 1.0f );
								effect->SetVector( "transparency", &e_val );
								if (rgb[0] < 1.0f || rgb[1] < 1.0f || rgb[2] < 1.0f)
									isTransparent = true;
							}
							MPlug incandescencePlug = MFnDependencyNode(shaderNode).findPlug("incandescence", &status);
							if (status != MS::kFailure)
							{
								MObject data;
								incandescencePlug.getValue( data);
								MFnNumericData val(data);
								val.getData( rgb[0], rgb[1], rgb[2]);
								//Material.Emissive.r = (float)rgb[0]; 
								//Material.Emissive.g = (float)rgb[1]; 
								//Material.Emissive.b = (float)rgb[2]; Material.Emissive.a = 1.0f; 
							}
							MPlug specularColorPlug = MFnDependencyNode(shaderNode).findPlug("specularColor", &status);
							if (status != MS::kFailure)
							{
								MObject data;
								specularColorPlug.getValue( data);
								MFnNumericData val(data);
								val.getData( rgb[0], rgb[1], rgb[2]);

								D3DXVECTOR4 e_val( rgb[0], rgb[1], rgb[2], 1.0f );
								effect->SetVector( "specularMaterial", &e_val );
							}						
							// Rough approximations for Phong, PhongE, and Blinn.
							//
							if (shaderNode.hasFn(MFn::kLambert))
							{
								effect->SetFloat( "specularPower", 0.0f );
							}
							if (shaderNode.hasFn(MFn::kPhong))
							{
								MPlug cosinePowerPlug = MFnDependencyNode(shaderNode).findPlug("cosinePower", &status);
								if (status != MS::kFailure)
								{
									MObject data;
									float cosPower = 0.0f;
									cosinePowerPlug.getValue( cosPower );
									hres = effect->SetFloat( "specularPower", cosPower * 4.0f );
								}						
							}
							else if (MFn::kBlinn)
							{
								MPlug eccentricityPlug = MFnDependencyNode(shaderNode).findPlug("eccentricity", &status);
								if (status != MS::kFailure)
								{
									// Maya's funky remapping of eccentricity into cosinePower.
									//
									MObject data;
									float eccentricity = 0.0f;
									eccentricityPlug.getValue( eccentricity );
									hres = effect->SetFloat( "specularPower", (eccentricity < 0.03125f) ? 128.0f : 4.0f / eccentricity );
								}
							}
							else // if (shaderNode.hasFn(MFn::kPhongE))
							{
								MPlug roughnessPlug = MFnDependencyNode(shaderNode).findPlug("roughness", &status);
								if (status != MS::kFailure)
								{
									MObject data;
									float roughness = 0.0f;
									roughnessPlug.getValue( roughness );
									hres = effect->SetFloat( "specularPower", roughness * 4.0f );
								}
							}
						}
					}

					unsigned int numPasses;
					effect->Begin( &numPasses, 0 );
					if (hres != D3D_OK)
						havePixelShader = false;
					else
					{
						//printf("Draw %d passes\n", numPasses);
						for (unsigned int p=0; p<numPasses; ++p)
						{
							hres = effect->BeginPass( p );
							if (hres == D3D_OK) 
							{
								// Enable transparency if required by material.
								//
								//m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, isTransparent);
								//m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
								//m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

								//printf("Draw surface shader pass\n");
								Geometry->Render( m_pD3DDevice);
							}
							hres = effect->EndPass();
						}
					}
					hres = effect->End();
					if (hres != D3D_OK) 
						havePixelShader = false;
				}
			}
		}
	}

	return havePixelShader;
}


bool D3DViewportRenderer::setSurfaceMaterial( const MDagPath &dagPath )
{
	D3DMATERIAL9 Material;
	bool isTransparent = false;

	MFnMesh fnMesh(dagPath);
	MObjectArray sets;
	MObjectArray comps;
	unsigned int instanceNum = dagPath.instanceNumber();
	if (!fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true))
		MGlobal::displayError("ERROR : MFnMesh::getConnectedSetsAndMembers");
	for ( unsigned i=0; i<sets.length(); i++ ) 
	{
		MObject set = sets[i];
		MObject comp = comps[i];

		MStatus status;
		MFnSet fnSet( set, &status );
		if (status == MS::kFailure) {
			MGlobal::displayError("ERROR: MFnSet::MFnSet");
			continue;
		}

		MObject shaderNode = findShader(set);
		if (shaderNode != MObject::kNullObj)
		{								
			float rgb[3];

			MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
			D3DTexture* Texture = NULL;
			if (status != MS::kFailure)
			{
				MItDependencyGraph It( colorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream);
				if( !It.isDone())
				{
					Texture = m_resourceManager.getTexture( It.thisNode());
					m_pD3DDevice->SetTexture( 0, Texture->Texture( m_pD3DDevice));
					m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
					m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
					m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
					m_pD3DDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0);
					m_pD3DDevice->SetRenderState(D3DRS_WRAP1, D3DWRAPCOORD_1);
					Material.Diffuse.r = Material.Diffuse.g = Material.Diffuse.b = Material.Diffuse.a = 1.0f; 
				}
				else
				{
					m_pD3DDevice->SetTexture( 0, NULL);
					MObject data;
					colorPlug.getValue( data);
					MFnNumericData val(data);
					val.getData( rgb[0], rgb[1], rgb[2]);
					Material.Diffuse.r = (float)rgb[0]; 
					Material.Diffuse.g = (float)rgb[1]; 
					Material.Diffuse.b = (float)rgb[2]; Material.Diffuse.a = 1.0f; 
				}
			}

			MPlug diffusePlug = MFnDependencyNode(shaderNode).findPlug("diffuse", &status);
			if (status != MS::kFailure)
			{
				MObject data;
				float diff;
				diffusePlug.getValue( diff );
				Material.Diffuse.r *= (float)diff;
				Material.Diffuse.g *= (float)diff; 
				Material.Diffuse.b *= (float)diff;
			}
			MPlug ambientColorPlug = MFnDependencyNode(shaderNode).findPlug("ambientColor", &status);
			if (status != MS::kFailure)
			{
				MObject data;
				ambientColorPlug .getValue( data);
				MFnNumericData val(data);
				val.getData( rgb[0], rgb[1], rgb[2]);
				Material.Ambient.r = (float)rgb[0]; 
				Material.Ambient.g = (float)rgb[1]; 
				Material.Ambient.b = (float)rgb[2]; Material.Ambient.a = 1.0f; 
			}
			MPlug transparencyPlug = MFnDependencyNode(shaderNode).findPlug("transparency", &status);
			if (status != MS::kFailure)
			{
				MObject data;
				transparencyPlug.getValue( data);
				MFnNumericData val(data);
				val.getData( rgb[0], rgb[1], rgb[2]);
				Material.Diffuse.a = 1.0f - (rgb[0] + rgb[1] + rgb[2]) / 3.0f;
				if (Material.Diffuse.a < 1.0f)
					isTransparent = true;
			}
			MPlug incandescencePlug = MFnDependencyNode(shaderNode).findPlug("incandescence", &status);
			if (status != MS::kFailure)
			{
				MObject data;
				incandescencePlug.getValue( data);
				MFnNumericData val(data);
				val.getData( rgb[0], rgb[1], rgb[2]);
				Material.Emissive.r = (float)rgb[0]; 
				Material.Emissive.g = (float)rgb[1]; 
				Material.Emissive.b = (float)rgb[2]; Material.Emissive.a = 1.0f; 
			}
			MPlug specularColorPlug = MFnDependencyNode(shaderNode).findPlug("specularColor", &status);
			if (status != MS::kFailure)
			{
				MObject data;
				specularColorPlug.getValue( data);
				MFnNumericData val(data);
				val.getData( rgb[0], rgb[1], rgb[2]);
				Material.Specular.r = (float)rgb[0]; 
				Material.Specular.g = (float)rgb[1]; 
				Material.Specular.b = (float)rgb[2]; Material.Specular.a = 1.0f; 
			}						
			// Rough approximations for Phong, PhongE, and Blinn.
			//
			Material.Power = 20.0f;
			if (shaderNode.hasFn(MFn::kLambert))
			{
				Material.Power = 0.0f;
			}
			if (shaderNode.hasFn(MFn::kPhong))
			{
				MPlug cosinePowerPlug = MFnDependencyNode(shaderNode).findPlug("cosinePower", &status);
				if (status != MS::kFailure)
				{
					MObject data;
					float cosPower = 0.0f;
					cosinePowerPlug.getValue( cosPower );
					Material.Power = cosPower * 4.0f;
				}						
			}
			else if (MFn::kBlinn)
			{
				MPlug eccentricityPlug = MFnDependencyNode(shaderNode).findPlug("eccentricity", &status);
				if (status != MS::kFailure)
				{
					// Maya's funky remapping of eccentricity into cosinePower.
					//
					MObject data;
					float eccentricity = 0.0f;
					eccentricityPlug.getValue( eccentricity );
					Material.Power = (eccentricity < 0.03125f) ? 128.0f : 4.0f / eccentricity;
				}
			}
			else // if (shaderNode.hasFn(MFn::kPhongE))
			{
				MPlug roughnessPlug = MFnDependencyNode(shaderNode).findPlug("roughness", &status);
				if (status != MS::kFailure)
				{
					MObject data;
					float roughness = 0.0f;
					roughnessPlug.getValue( roughness );
					Material.Power = roughness * 4.0f;
				}
			}
			break;
		}
	}

	// Enable transparency if required by material.
	//
	m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, isTransparent);
	m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	m_pD3DDevice->SetMaterial( &Material);

	return true;
}

bool D3DViewportRenderer::drawSurface( const MDagPath &dagPath, bool active, bool templated)
{
	bool drewSurface = false;

	if ( !dagPath.hasFn( MFn::kMesh ))
	{
		MMatrix  matrix = dagPath.inclusiveMatrix();
		MFnDagNode dagNode(dagPath);
		MBoundingBox box = dagNode.boundingBox();
		float color[3] = {0.6f, 0.3f, 0.0f};
		if (active)
		{
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
		}
		else if (templated)
		{
			color[0] = 1.0f;
			color[1] = 0.686f;
			color[2] = 0.686f;
		}
		drawBounds( matrix, box, false, true, color);
		return true;
	}

	if ( dagPath.hasFn( MFn::kMesh ))
	{
		MMatrix  matrix = dagPath.inclusiveMatrix();
		MFnDagNode dagNode(dagPath);

		// Look for any hardware shaders which can draw D3D first.
		//
		bool drewWithHwShader = false;
		{
			MFnMesh fnMesh(dagPath);
			MObjectArray sets;
			MObjectArray comps;
			unsigned int instanceNum = dagPath.instanceNumber();
			if (!fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true))
				MGlobal::displayError("ERROR : MFnMesh::getConnectedSetsAndMembers");
			for ( unsigned i=0; i<sets.length(); i++ ) 
			{
				MObject set = sets[i];
				MObject comp = comps[i];

				MStatus status;
				MFnSet fnSet( set, &status );
				if (status == MS::kFailure) {
					MGlobal::displayError("ERROR: MFnSet::MFnSet");
					continue;
				}

				MObject shaderNode = findShader(set);
				if (shaderNode != MObject::kNullObj)
				{
					MPxHardwareShader * hwShader = 
						MPxHardwareShader::getHardwareShaderPtr( shaderNode );

					if (hwShader)
					{
						const MRenderProfile & profile = hwShader->profile();
						if (profile.hasRenderer( MRenderProfile::kMayaD3D))
						{
							// Render a Maya D3D hw shader here....
							//printf("Found a D3D hw shader\n");
							//drewWithHwShader = true;
						}
					}
				}
			}
		}

		// Get the geometry buffers for this bad boy and render them
		D3DGeometry* Geometry = m_resourceManager.getGeometry( dagPath, m_pD3DDevice);
		if( Geometry)
		{
			// Transform from object to world space
			//
			D3DXMATRIXA16 objectToWorld = D3DXMATRIXA16
				(
				(float)matrix.matrix[0][0], (float)matrix.matrix[0][1], (float)matrix.matrix[0][2], (float)matrix.matrix[0][3],
				(float)matrix.matrix[1][0], (float)matrix.matrix[1][1], (float)matrix.matrix[1][2], (float)matrix.matrix[1][3],
				(float)matrix.matrix[2][0], (float)matrix.matrix[2][1], (float)matrix.matrix[2][2], (float)matrix.matrix[2][3],
				(float)matrix.matrix[3][0], (float)matrix.matrix[3][1], (float)matrix.matrix[3][2], (float)matrix.matrix[3][3]
			);

			m_pD3DDevice->SetTransform( D3DTS_WORLD, &objectToWorld );
			m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
			m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
			m_pD3DDevice->SetRenderState( D3DRS_AMBIENT, 0xFFFFFFFF );
			m_pD3DDevice->SetRenderState( D3DRS_VERTEXBLEND, FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
			m_pD3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0xFFFFFFFF );

			if (!drewWithHwShader)
			{
				// Get material properties for shader associated with mesh
				//
				// 1. Try to draw with the sample internal programmable shader
				bool drewGeometryWithShader = false;
				// optionVar -iv "D3D_USE_PIXEL_SHADER" 1; // Set it
				// optionVar -remove "D3D_USE_PIXEL_SHADER"; // Remove it
				MString usePixelShader("D3D_USE_PIXEL_SHADER");
				int val = 0;
				if (MGlobal::getOptionVarValue(usePixelShader, val))
				{
					MMatrix  itmatrix = matrix.inverse().transpose();
					D3DXMATRIXA16 objectToWorldInvTransp = D3DXMATRIXA16
						(
						(float)itmatrix.matrix[0][0], (float)itmatrix.matrix[0][1], (float)itmatrix.matrix[0][2], (float)itmatrix.matrix[0][3],
						(float)itmatrix.matrix[1][0], (float)itmatrix.matrix[1][1], (float)itmatrix.matrix[1][2], (float)itmatrix.matrix[1][3],
						(float)itmatrix.matrix[2][0], (float)itmatrix.matrix[2][1], (float)itmatrix.matrix[2][2], (float)itmatrix.matrix[2][3],
						(float)itmatrix.matrix[3][0], (float)itmatrix.matrix[3][1], (float)itmatrix.matrix[3][2], (float)itmatrix.matrix[3][3]
					);

					MVector eyePos( 0.0, 0.0, 0.0 );
					eyePos *= mm_currentViewMatrix;
					D3DXVECTOR4 worldEye( (float)eyePos[0], (float)eyePos[1], (float)eyePos[2], (float)eyePos[3]);

					drewGeometryWithShader = setSurfaceMaterialShader( dagPath, Geometry, objectToWorld, objectToWorldInvTransp,
						objectToWorld * m_currentViewMatrix * m_currentProjectionMatrix,
						worldEye );
				}

				// 2. Draw with fixed function shader
				if (!drewGeometryWithShader)
				{
					// Set up a default material, just in case there is none.
					//
					D3DMATERIAL9 Material;
					if (active)
					{
						m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
						Material.Diffuse.r = 1.0f; Material.Diffuse.g = 1.0f; Material.Diffuse.b = 1.0f; Material.Diffuse.a = 1.0f; 
					}
					else if (templated)
					{
						m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
						Material.Diffuse.r = 1.0f; Material.Diffuse.g = 0.686f; Material.Diffuse.b = 0.686f; Material.Diffuse.a = 1.0f; 
					}
					else
					{
						m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
						Material.Diffuse.r = 0.5f; Material.Diffuse.g = 0.5f; Material.Diffuse.b = 0.5f; Material.Diffuse.a = 1.0f; 
					}
					Material.Ambient.r = 0.0f; Material.Ambient.g = 0.0f; Material.Ambient.b = 0.0f; Material.Ambient.a = 0.0f; 
					Material.Specular.r = 1.0f; Material.Specular.g = 1.0f; Material.Specular.b = 1.0f; Material.Specular.a = 1.0f; 
					Material.Emissive.r = 0.0f; Material.Emissive.g = 0.0f; Material.Emissive.b = 0.0f; Material.Emissive.a = 0.0f; 
					Material.Power = 50.0f;

					bool scanForMayaMaterial = true;
					if (!templated && scanForMayaMaterial)
					{
						if (!setSurfaceMaterial( dagPath ))
						{
							m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
							m_pD3DDevice->SetMaterial( &Material);
						}
					}

					Geometry->Render( m_pD3DDevice);
				}
			}

			// Draw wireframe on top
			//
			m_pD3DDevice->SetTexture( 0, NULL);
			m_pD3DDevice->SetRenderState(D3DRS_WRAP0, 0);
			m_pD3DDevice->SetRenderState(D3DRS_WRAP1, 0);

			if (active)
			{
				bool drawActiveWithBounds = false;
				if (drawActiveWithBounds)
				{
					MBoundingBox box = dagNode.boundingBox();
					float color[3] = {1.0f, 1.0f, 1.0f};
					drawBounds( matrix, box, false, false, color );
				}
				else
				{
					D3DMATERIAL9 Material;
					Material.Emissive.r = 1.0; 
					Material.Emissive.g = 1.0; 
					Material.Emissive.b = 1.0; 
					Material.Emissive.a = 1.0f; 
					Material.Ambient.r = 1.0; 
					Material.Ambient.g = 1.0; 
					Material.Ambient.b = 1.0; 
					Material.Ambient.a = 1.0f; 
					m_pD3DDevice->SetMaterial( &Material);
					m_resourceManager.enableLights( FALSE, m_pD3DDevice );
					m_pD3DDevice->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, TRUE );
					m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
					m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
					m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
					m_pD3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, 100 );
					m_pD3DDevice->SetRenderState( D3DRS_DEPTHBIAS, 10 );

					Geometry->Render( m_pD3DDevice);				

					m_pD3DDevice->SetRenderState( D3DRS_DEPTHBIAS, 0 );
					m_pD3DDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, 0 );
					m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );		
					m_resourceManager.enableLights( TRUE, m_pD3DDevice );
				}
			}
		} // If Geometry
	}
	return drewSurface;
}

bool D3DViewportRenderer::drawScene(const MRenderingInfo &renderInfo)
//
// Description:
//		Draw the Maya scene, using a custom traverser.
//
{
	bool useDrawTraversal = true;
	float groundPlaneColor[3] = { 0.8f, 0.8f, 0.8f };

	if (useDrawTraversal)
	{
		const MDagPath &cameraPath = renderInfo.cameraPath();
		if (cameraPath.isValid())
		{
			// You can actually keep the traverser classes around
			// if desired. Here we just create temporary traversers
			// on the fly.
			//
			MDrawTraversal *trav = new MDrawTraversal;
			trav->enableFiltering( false );
			if (!trav)
			{
				MGlobal::displayWarning("Direct3D renderer : failed to create a traversal class !\n");
				return true;
			}

			const MRenderTarget &renderTarget = renderInfo.renderTarget();
			trav->setFrustum( cameraPath, renderTarget.width(), 
							  renderTarget.height() );

			if (!trav->frustumValid())
			{
				MGlobal::displayWarning("Direct3D renderer : Frustum is invalid !\n");
				return true;
			}

			trav->traverse();

			unsigned int numItems = trav->numberOfItems();
			unsigned int i;
			for (i=0; i<numItems; i++)
			{
				MDagPath path;
				trav->itemPath(i, path);

				if (path.isValid())
				{
					bool drawIt = false;

					// Default traverer may have view manips showing up.
					// This is currently a known Maya bug.
					if ( path.hasFn( MFn::kViewManip ))
						continue;

					//
					// Draw surfaces (polys, nurbs, subdivs)
					//
					bool active = false;
					bool templated = false;
					if ( path.hasFn( MFn::kMesh) || 
						 path.hasFn( MFn::kNurbsSurface) || 
						 path.hasFn( MFn::kSubdiv) )
					{
						drawIt = true;
						if (trav->itemHasStatus( i, MDrawTraversal::kActiveItem ))
						{
							active = true;
						}
						else if (trav->itemHasStatus( i, MDrawTraversal::kTemplateItem ))
						{
							templated = true;
						}
						else
						{
							if (path.hasFn( MFn::kMesh ))
								;
							else if (path.hasFn( MFn::kNurbsSurface))
								;
							else
								;
						}
					}

					//
					// Draw the ground plane
					//
					else if (path.hasFn( MFn::kSketchPlane ) ||
							 path.hasFn( MFn::kGroundPlane ))
					{
						MMatrix  matrix = path.inclusiveMatrix();
						MFnDagNode dagNode(path);
						MBoundingBox box = dagNode.boundingBox();
						drawBounds( matrix, box, false, false, groundPlaneColor );
					}

					if (drawIt)
					{
						drawSurface( path, active, templated );
					}
				}
			}

			if (trav)
				delete trav;

			// Cleanup any unused resource items
			bool onlyInvalidItems = true;
			clearResources( onlyInvalidItems, false );
		}
	}
	else
	{
		// Draw some poly bounding boxes 
		//
		MItDag::TraversalType traversalType = MItDag::kDepthFirst;
		MFn::Type filter = MFn::kMesh;
		MStatus status;

		MItDag dagIterator( traversalType, filter, &status);

		for ( ; !dagIterator.isDone(); dagIterator.next() ) 
		{

			MDagPath dagPath;

			status = dagIterator.getPath(dagPath);
			if ( !status ) {
				status.perror("MItDag::getPath");
				continue;
			}

			MFnDagNode dagNode(dagPath, &status);
			if ( !status ) {
				status.perror("MFnDagNode constructor");
				continue;
			}

			MMatrix  matrix = dagPath.inclusiveMatrix();
			MBoundingBox box = dagNode.boundingBox();
			drawBounds( matrix, box, false, false, NULL );
		}
	}
	return true;
}

bool D3DViewportRenderer::drawBounds(const MMatrix &matrix, const MBoundingBox &box, bool filled, bool useDummyGeometry,
									 float color[3],
									 LPDIRECT3DVERTEXBUFFER9 buffer /* = NULL */)
{
	// Transform from object to world space
	//
	D3DXMATRIXA16 mat = D3DXMATRIXA16
		(
		(float)matrix.matrix[0][0], (float)matrix.matrix[0][1], (float)matrix.matrix[0][2], (float)matrix.matrix[0][3],
		(float)matrix.matrix[1][0], (float)matrix.matrix[1][1], (float)matrix.matrix[1][2], (float)matrix.matrix[1][3],
		(float)matrix.matrix[2][0], (float)matrix.matrix[2][1], (float)matrix.matrix[2][2], (float)matrix.matrix[2][3],
		(float)matrix.matrix[3][0], (float)matrix.matrix[3][1], (float)matrix.matrix[3][2], (float)matrix.matrix[3][3]
		);
	m_pD3DDevice->SetTransform( D3DTS_WORLD, &mat );

	// Draw the bounding scaled and offset bounds. Handles component transforms
	//
	MPoint	minPt = box.min();
	MPoint	maxPt = box.max();
	float minVal[3] = { (float)minPt.x, (float)minPt.y, (float)minPt.z };
	float maxVal[3] = { (float)maxPt.x, (float)maxPt.y, (float)maxPt.z };
	drawCube( minVal, maxVal, filled, useDummyGeometry, color, buffer );

	return true;
}

bool D3DViewportRenderer::renderToTarget( const MRenderingInfo &renderInfo )
//
// Description:
//		Rener to off-screen render target and read back into system memory 
//		output buffer.
//
//
{
	// Direct rendering to a D3D surface
	//
	if (renderInfo.renderingAPI() == MViewportRenderer::kDirect3D)
	{
		// Maya does not support D3D currently. Would need
		// to have access to the device, and surface here
		// from an MRenderTarget. API doesn't exist, so
		// do nothing.
		return false;
	}

	//
	// Offscreen rendering
	//
	if (!m_pD3DDevice || !m_pTextureOutput || !m_pTextureInterm)
		return false;

	// START RENDER
	HRESULT hres;

	// Set colour and depth surfaces.
	//
	hres = m_pD3DDevice->SetRenderTarget( 0, m_pTextureIntermSurface );
	if (m_requireDepthStencilReadback && m_pDepthStencilSurface)
		hres = m_pD3DDevice->SetDepthStencilSurface( m_pDepthStencilSurface );

	hres = m_pD3DDevice->BeginScene();
	if (hres == D3D_OK)
	{
		// Setup projection and view matrices
		setupMatrices( renderInfo );

		// Clear the entire buffer (RGB, Depth). Leave stencil for now.
		//
		hres = m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
									D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f), 1.0f, 0 );

		// Setup lighting
		//
		m_resourceManager.setupLighting(m_pD3DDevice);

		if (hres == D3D_OK)
		{
			// Render the scene
			drawScene(renderInfo);
		}

		m_resourceManager.cleanupLighting(m_pD3DDevice);
	}
	// END SCENE RENDER

	hres = m_pD3DDevice->EndScene();
	if (hres != D3D_OK)
		return false;

	// Do post-rendering
	postRenderToTarget();

	return true;
}

bool D3DViewportRenderer::setupMatrices( const MRenderingInfo &info )
//
// Description:
//
//		Set up camera matrices. Mechanism to check for changes in camera
//		parameters should be done before matrix setup.
//
//		Note that we *must* use a "right-handed" system (RH method 
//		versions) for computations to match what is coming from Maya.
//
{
	if (!m_pD3DDevice)
		return false;

	const MMatrix & view = info.viewMatrix(); 
	const MMatrix & projection = info.projectionMatrix();

	// Double to float conversion
	D3DXMATRIXA16 vm( (float)view.matrix[0][0], (float)view.matrix[0][1], (float)view.matrix[0][2], (float)view.matrix[0][3], 
		(float)view.matrix[1][0], (float)view.matrix[1][1], (float)view.matrix[1][2], (float)view.matrix[1][3], 
		(float)view.matrix[2][0], (float)view.matrix[2][1], (float)view.matrix[2][2], (float)view.matrix[2][3], 
		(float)view.matrix[3][0], (float)view.matrix[3][1], (float)view.matrix[3][2], (float)view.matrix[3][3]);

	D3DXMATRIXA16 pm( (float)projection.matrix[0][0], (float)projection.matrix[0][1], (float)projection.matrix[0][2], (float)projection.matrix[0][3], 
		(float)projection.matrix[1][0], (float)projection.matrix[1][1], (float)projection.matrix[1][2], (float)projection.matrix[1][3], 
		(float)projection.matrix[2][0], (float)projection.matrix[2][1], (float)projection.matrix[2][2], (float)projection.matrix[2][3], 
		(float)projection.matrix[3][0], (float)projection.matrix[3][1], (float)projection.matrix[3][2], (float)projection.matrix[3][3]);


	m_pD3DDevice->SetTransform( D3DTS_PROJECTION, &pm );
	m_pD3DDevice->SetTransform( D3DTS_VIEW, &vm  );

	return true;
}

void D3DViewportRenderer::drawFullScreenQuad(float leftU, float topV, 
											 float rightU, float bottomV,
											 float targetWidth, float targetHeight,
											 LPDIRECT3DDEVICE9 D3D)
//
// Description:
//		Draw a screen space quad.
//
{
    float width = targetWidth - 0.5f;
    float height = targetHeight - 0.5f;

    // Draw the quad
    ScreenSpaceVertex screenQuad[4];

    screenQuad[0].position = D3DXVECTOR4(-0.5f, -0.5f, 0.5f, 1.0f);
    screenQuad[0].texCoord = D3DXVECTOR2(leftU, topV);

    screenQuad[1].position = D3DXVECTOR4(width, -0.5f, 0.5f, 1.0f);
    screenQuad[1].texCoord = D3DXVECTOR2(rightU, topV);

    screenQuad[2].position = D3DXVECTOR4(-0.5f, height, 0.5f, 1.0f);
    screenQuad[2].texCoord = D3DXVECTOR2(leftU, bottomV);

    screenQuad[3].position = D3DXVECTOR4(width, height, 0.5f, 1.0f);
    screenQuad[3].texCoord = D3DXVECTOR2(rightU, bottomV);

    D3D->SetRenderState(D3DRS_ZENABLE, FALSE);
    D3D->SetFVF(ScreenSpaceVertex::FVF);
    D3D->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, screenQuad, sizeof(ScreenSpaceVertex));
    D3D->SetRenderState(D3DRS_ZENABLE, TRUE);
}

bool D3DViewportRenderer::postRenderToTarget()
//
// Description:
//	Render using post-process screen effects. Uses a 2 target buffering
//	scheme to ping-pong results back and forth for each effect change.
//
{
	if (!m_pTexturePost)
		return false;

	IDirect3DSurface9 *postSurface = NULL;
	HRESULT hres = m_pTexturePost->GetSurfaceLevel( 0, &postSurface );
	if (hres != D3D_OK)
		return false;	

	IDirect3DSurface9 *currentSurface[2] = { m_pTextureIntermSurface, postSurface };
	LPDIRECT3DTEXTURE9  currentTexture[2] = { m_pTextureInterm, m_pTexturePost };

	// Render source texture into destination, and then flip back
	// and forth as required until we finish all effects
	//
	unsigned int currentTarget = 0;
	unsigned int newTarget = 0;

	// Determine the quad render size once
	//
	D3DSURFACE_DESC surfaceDesc;
	m_pTextureOutputSurface->GetDesc( &surfaceDesc );
	float quad_renderWidth = (float)surfaceDesc.Width;
	float quad_renderHeight = (float)surfaceDesc.Height;

	unsigned int numEffectsApplied = 0;

	const MStringArray &enabledEffects = m_resourceManager.getListOfEnabledPostEffects();
	const PostEffectItemList &postEffectList = m_resourceManager.getPostEffectItemList();

	unsigned int numOfEnabledEffects = enabledEffects.length();
	unsigned int numEffects = (unsigned int) postEffectList.size();

	// We don't want floating point, and we don't have any effects.
	// So there's nothing to do.
	if (!m_wantFloatingPointTargets && !numOfEnabledEffects || !numEffects)
		return false;

	// Don't need depth anymore
	m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );

	for (unsigned int m=0; m<numOfEnabledEffects ; m++)
	{
		ID3DXEffect* effect = NULL;
		PostEffectItem *effectItem = NULL;

		// Scan all existing effects item to see if the name matches
		// the enabled effect name.
		PostEffectItemList::const_iterator eit, end_eit;
		end_eit = postEffectList.end();
		for (eit = postEffectList.begin(); eit != end_eit;  eit++)
		{
			PostEffectItem *item = *eit;
			if (item)
			{
				// Enable item hase been found in the global effects list.
				if (enabledEffects[m] == item->fName)
				{
					effectItem = item;
					effect = item->fEffect;
					break;
				}
			}
		}
		if (effect != NULL)
		{
			// Flip the current render target
			newTarget = ( currentTarget + 1 ) % 2;
			m_pD3DDevice->SetRenderTarget( 0, currentSurface[newTarget] );

			// Start scene rendering
			m_pD3DDevice->BeginScene();
			{
				/* WARNING Magic word lookup */
				hres = effect->SetTechnique( "PostProcess" ); 
				if (hres != D3D_OK)
					continue;

				// Set the deltas for the kernel, if required
				/* WARNING Magic word lookup */
				effect->SetFloat( "duKernel", 1.0f / (float)quad_renderWidth );				
				effect->SetFloat( "dvKernel", 1.0f / (float)quad_renderHeight);

				// Hacks set the value for effects via Maya option variables.
				if (effectItem->fName == MString("PostProcess_ToneMapFilter"))
				{
					double value = 1.0;
					MString toneMapExp("PostProcess_ToneMapFilter_Exposure");
					if (!MGlobal::getOptionVarValue(toneMapExp, value))
						MGlobal::setOptionVarValue(toneMapExp, value);
					effect->SetFloat( "exposure", (float)value );				
				}
				else if (effectItem->fName == MString("PostProcess_SobelFilter"))
				{
					double value = 20;
					MString thickness("PostProcess_SobelFilter_edgeThickness");
					if (!MGlobal::getOptionVarValue(thickness, value))
						MGlobal::setOptionVarValue(thickness, value);
					effect->SetFloat( "edgeThickness", (float)value);	
				}

				// Start effect rendering
				unsigned int numPasses;
				hres = effect->Begin( &numPasses, 0 );
				if (hres != D3D_OK)
					continue;
				else
				{
					// Flip the current input texture
					/* WARNING Magic word lookup */
					hres = effect->SetTexture( "textureSourceColor", currentTexture[currentTarget] );
					if (hres != D3D_OK)
					{
						m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET /* | D3DCLEAR_ZBUFFER */, 
							D3DCOLOR_COLORVALUE(1.0f,0.0f,0.0f,1.0f), 1.0f, 0 );

						continue;
					}

					// Loop through all passes				
					for (unsigned int p=0; p<numPasses; ++p)
					{
						hres = effect->BeginPass( p );
						if (hres != D3D_OK) 
						{
							m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET /* | D3DCLEAR_ZBUFFER */, 
								D3DCOLOR_COLORVALUE(1.0f,0.0f,0.0f,1.0f), 1.0f, 0 );
							continue;
						}
						// Draw quad for the effect
						drawFullScreenQuad( 0.0f, 0.0f, 1.0f, 1.0f, 
							quad_renderWidth, quad_renderHeight, m_pD3DDevice );

						hres = effect->EndPass();
						if (hres != D3D_OK) 
						{
							m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET /* | D3DCLEAR_ZBUFFER */, 
								D3DCOLOR_COLORVALUE(1.0f,0.0f,0.0f,1.0f), 1.0f, 0 );

							continue;
						}
					}
				}
				hres = effect->End();

				numEffectsApplied++;
			}
			m_pD3DDevice->SetTexture( 0, NULL);
			m_pD3DDevice->EndScene();

#if defined(_DEBUG_POST_BUFFERS_)
			bool dumpToFile= false;
			if (dumpToFile)
			{
				const char fileName[] = "c:\\temp\\d3dDump_newTarget.jpg";
				HRESULT hres = D3DXSaveSurfaceToFile( fileName, D3DXIFF_JPG,
					currentSurface[newTarget], NULL /*palette*/, NULL /*rect*/ );

				const char fileName2[] = "c:\\temp\\d3dDump_oldTarget.jpg";
				hres = D3DXSaveSurfaceToFile( fileName2, D3DXIFF_JPG,
					currentSurface[currentTarget], NULL /*palette*/, NULL /*rect*/ );

			}
#endif

			// Flip to the next render target
			//
			currentTarget = newTarget;
		}
	}

	// 
	// Copy the intermediate target to our final output target,
	// if they are not the same. If we aren't using floating point
	// then there's nothing to do.
	//
	if ((currentSurface[currentTarget] != m_pTextureIntermSurface) || 
		(m_pTextureOutputSurface != m_pTextureIntermSurface))
	{
		m_pD3DDevice->SetRenderTarget( 0, m_pTextureOutputSurface );
		m_pD3DDevice->BeginScene();
		{
			m_pD3DDevice->SetTexture( 0, currentTexture[currentTarget] );
			drawFullScreenQuad( 0.0f, 0.0f, 1.0f, 1.0f, 
				quad_renderWidth, quad_renderHeight, m_pD3DDevice );
		}
		m_pD3DDevice->EndScene();
	}

	// Release temporary surface
	if (postSurface)
		postSurface->Release();

	// Turn depth back on
	m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

	return true;
}

bool D3DViewportRenderer::readFromTargetToSystemMemory()
//
// Description:
//		Read back render target memory into system memory to 
//		transfer back to calling code.
// 
{
	if (!m_pD3DDevice || !m_pTextureOutputSurface || m_renderWidth==0 || m_renderHeight == 0 ||
		!m_SystemMemorySurface)
		return false;

	bool readBuffer = false;

	// Dump to file option for debugging purposes.
	//
#if defined(_DUMP_SURFACE_READBACK_CONTENTS_)
	bool dumpToFile= false;
	if (dumpToFile)
	{
		const char fileName[] = "c:\\temp\\d3dDump.jpg";
		HRESULT hres = D3DXSaveSurfaceToFile( fileName, D3DXIFF_JPG,
							m_pTextureOutputSurface, NULL /*palette*/, NULL /*rect*/ );
		if (hres != D3D_OK)
		{
			MGlobal::displayWarning("Direct3D renderer : Failed to dump surface contents to file !\n");
		}
	}
#endif

	D3DSURFACE_DESC surfaceDesc;
	m_pTextureOutputSurface->GetDesc( &surfaceDesc );
	//m_renderWidth = surfaceDesc.Width;
	//m_renderHeight = surfaceDesc.Height;

	D3DLOCKED_RECT rectInfo;
	// RECTs use upper-left to lower-right scheme.
	RECT rectToRead;
	rectToRead.top = 0;
	rectToRead.left = 0;
	rectToRead.bottom = m_renderHeight;
	rectToRead.right = m_renderWidth;
	DWORD readFlags = D3DLOCK_READONLY;

	// Since the texture is in D3DPOOL_DEFAULT, need to call
	// GetRenderTargetData to get the contents back into system
	// system. Pretty gruesome code for now....
	//
	HRESULT hres = m_pD3DDevice->GetRenderTargetData( m_pTextureOutputSurface,
		m_SystemMemorySurface );

	if (hres == D3D_OK)
	{
		// Lock the system memory surface, and read back based on lock info
		// returned.
		//
		hres = m_SystemMemorySurface->LockRect( &rectInfo, &rectToRead, readFlags );
		if (hres == D3D_OK)
		{					
			INT pitch = rectInfo.Pitch;
			BYTE *data = (BYTE *)rectInfo.pBits;

			// ** Magic number warning ***
			// We use D3DFMT_A8R8G8B8 as the buffer format for now as we
			// assume 32 bits per pixel = 4 bytes per pixel. Will need to
			// change when buffer format changes possibly be float.
			//
			const unsigned int bytesPerPixel = 4;

			// Reallocate buffer block as required.
			unsigned int m_readBackBufferWidth = 0;
			unsigned int m_readBackBufferHeight = 0;
			m_readBackBuffer.getSize(m_readBackBufferWidth, m_readBackBufferHeight);

			BYTE *m_readBackBufferPtr = NULL;
			bool replaceReadBackBuffer = false;

			if (!m_readBackBufferWidth || !m_readBackBufferHeight ||
				m_readBackBufferWidth != m_renderWidth ||
				m_readBackBufferHeight != m_renderHeight)
			{
				// This crashes Maya. Need to figure out why ?????
				m_readBackBuffer.resize(m_renderWidth, m_renderHeight, false);
				m_readBackBuffer.getSize(m_readBackBufferWidth, m_readBackBufferHeight);
				if (m_readBackBufferWidth != m_renderWidth ||
					m_readBackBufferHeight != m_renderHeight)
				{
					MGlobal::displayError("D3D Renderer : Could not resize MImage buffer for readback !\n");
					return false;
				}
				m_readBackBufferPtr = (BYTE *)(m_readBackBuffer.pixels());
			}
			else
				m_readBackBufferPtr = (BYTE *)(m_readBackBuffer.pixels());

			if (m_readBackBufferPtr)
			{
				// Copy a row at a time.
				// The jump by pitch, may differ if pitch is not the same as width.
				//
				unsigned int myLineSize = m_renderWidth * bytesPerPixel;
				unsigned int offsetMyData = (m_renderHeight-1) * myLineSize;
				unsigned int offsetData = 0;

				unsigned int i;
				for ( i=0 ; i < m_renderHeight; i++ )
				{
					memcpy( m_readBackBufferPtr + offsetMyData, 
						data + offsetData, 
						myLineSize );
					offsetMyData -= myLineSize;
					offsetData += pitch;
				}

				readBuffer = true;
			}

			if (replaceReadBackBuffer)
			{
				m_readBackBuffer.setPixels( m_readBackBufferPtr, m_renderWidth,
					m_renderHeight );
				delete[] m_readBackBufferPtr;					
			}
			m_readBackBufferPtr = 0;

			// Unlock 
			hres = m_SystemMemorySurface->UnlockRect();
		}
	}
	return readBuffer;
}

#endif



