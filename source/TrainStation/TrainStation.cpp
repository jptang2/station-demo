//----------------------------------------------------------------------------------
#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTshapes.h>
#include <SDKmesh.h>
#include <NVUTmesh.h>
#include <DXUTgui.h>
#include <DXUTsettingsdlg.h>
#include "sdkmisc.h"
#include "TrainStation.h"
#include <math.h>

#define SHADOW_MAP_SIZE		2048  
#define LIGHT_SIZE			2500.0f
#define LIGHT_NEAR			500.0f
#define LIGHT_FAR			10000.0f
 
#define SHADOW_MIPS			4
#define SHADOW_HOLE_MIP		3 // SHADOW_HOLE_MIP < SHADOW_MIPS !!!

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

ID3DX10Font*          g_pFont = NULL; 
ID3DX10Sprite*        g_pSprite = NULL;
CDXUTTextHelper*      g_pTxtHelper = NULL;
ID3D10Effect*         g_pEffect = NULL;
CDXUTSDKMesh          g_ObjectMesh;
CFirstPersonCamera    g_ViewCamera;
CModelViewerCamera    g_LightCamera;

ID3D10InputLayout*      g_pSceneVertexLayout = NULL;

ID3D10Texture2D*            g_pShadowMapTexture = NULL;
ID3D10ShaderResourceView*   g_pShadowMapTextureSRV = NULL;
ID3D10DepthStencilView*     g_pShadowMapTextureDSV = NULL;

ID3D10Texture2D*            g_pShadowMapTextureWorld = NULL;
ID3D10ShaderResourceView*   g_pShadowMapTextureWorldSRV = NULL;
ID3D10RenderTargetView*     g_pShadowMapTextureWorldRTV = NULL;

ID3D10Texture2D*            g_pShadowMapTextureScaled[SHADOW_MIPS];
ID3D10ShaderResourceView*   g_pShadowMapTextureScaledSRV[SHADOW_MIPS];
ID3D10RenderTargetView*		g_pShadowMapTextureScaledRTV[SHADOW_MIPS];

ID3D10Texture2D*            g_pShadowMapTextureScaledOpt = NULL;
ID3D10ShaderResourceView*   g_pShadowMapTextureScaledOptSRV = NULL;
ID3D10RenderTargetView*		g_pShadowMapTextureScaledOptRTV = NULL;

ID3D10Texture2D*            g_pShadowMapTextureHole[2];
ID3D10ShaderResourceView*   g_pShadowMapTextureHoleSRV[2];
ID3D10RenderTargetView*     g_pShadowMapTextureHoleRTV[2];

ID3D10Texture2D*            g_pDepthBufferTexture = NULL;
ID3D10ShaderResourceView*   g_pDepthBufferTextureSRV = NULL;
ID3D10DepthStencilView*     g_pDepthBufferTextureDSV = NULL;

ID3D10Texture2D*            g_pDepthBufferTextureWS = NULL;
ID3D10ShaderResourceView*   g_pDepthBufferTextureWSSRV = NULL;
ID3D10RenderTargetView*     g_pDepthBufferTextureWSRTV = NULL;

ID3D10Texture2D*            g_pNoiseTexture = NULL;
ID3D10ShaderResourceView*   g_pNoiseTextureSRV = NULL;

D3DXVECTOR3 g_EyePosition;
D3DXVECTOR3 g_LightPosition;
D3DXMATRIX  g_MProjection;
D3DXMATRIX  g_MView;
D3DXMATRIX  g_MLight;
D3DXMATRIX  g_MWorldViewProjection;
D3DXMATRIX  g_MInvWorldViewProjection;
D3DXMATRIX  g_MWorldLightProjection;
D3DXMATRIX	g_MWorldLightProjectionInv;
D3DXMATRIX  g_MLightProjection;

D3DMATRIX g_LightProjMatrix;

bool g_DrawVolumeLight = true;
bool g_DrawScene = true;
bool g_UsePostProcessing = true;
bool g_UseAngleOptimization = false;
bool g_DrawBloom = true;
bool g_DrawScaled = true;
bool g_UseZOptimization = true;
bool g_ShowHelp = true;

CD3DSettingsDlg                g_D3DSettingsDlg;
CDXUTDialogResourceManager     g_DialogResourceManager;
CDXUTDialog                    g_HUD;

float	g_SamplingRate = 10.0f;
int		g_ScaleRatio = 4;
ID3D10EffectScalarVariable* g_pUseAngleOptimization;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC					-1
#define IDC_TOGGLEFULLSCREEN		1
#define IDC_TOGGLEREF				2
#define IDC_CHANGEDEVICE			3
#define IDC_SAMPLINGRATE_STATIC		4
#define IDC_SAMPLINGRATE			5
#define IDC_USEANGLEOPTIMIZATION	6
#define IDC_USEPOSTPROCESSING		7
#define IDC_DRAWVOLUMELIGHT			8
#define IDC_USEZOPTIMIZATION		10
#define IDC_SCALERATIO_STATIC		11
#define IDC_SCALERATIO				12
#define IDC_DRAWBLOOM				13
#define IDC_DRAWSCENE				14

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------

void        CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

//--------------------------------------------------------------------------------------
// HDR
//--------------------------------------------------------------------------------------

#define NUM_TONEMAP_TEXTURES  5       // Number of stages in the 3x3 down-scaling 
                                      // of average luminance textures
#define NUM_BLOOM_TEXTURES    2

ID3D10Texture2D*			g_pHDRTexture;
ID3D10ShaderResourceView*	g_pHDRTextureSRV;
ID3D10RenderTargetView*		g_pHDRTextureRTV;

ID3D10Texture2D*			g_pEdgeTextureFS = NULL;
ID3D10ShaderResourceView*	g_pEdgeTextureFSSRV = NULL;
ID3D10RenderTargetView*		g_pEdgeTextureFSRTV = NULL;

ID3D10Texture2D*			g_pHDRTextureScaled;
ID3D10ShaderResourceView*	g_pHDRTextureScaledSRV;
ID3D10RenderTargetView*		g_pHDRTextureScaledRTV;

ID3D10Texture2D*			g_pHDRTextureScaled2;
ID3D10ShaderResourceView*	g_pHDRTextureScaled2SRV;
ID3D10RenderTargetView*		g_pHDRTextureScaled2RTV;

ID3D10Texture2D*			g_pHDRTextureScaled3;
ID3D10ShaderResourceView*	g_pHDRTextureScaled3SRV;
ID3D10RenderTargetView*		g_pHDRTextureScaled3RTV;

ID3D10Texture2D*			g_apTexToneMap10[NUM_TONEMAP_TEXTURES]; // Tone mapping calculation textures
ID3D10ShaderResourceView*	g_apTexToneMapRV10[NUM_TONEMAP_TEXTURES];
ID3D10RenderTargetView*		g_apTexToneMapRTV10[NUM_TONEMAP_TEXTURES];
ID3D10Texture2D*			g_apTexBloom10[NUM_BLOOM_TEXTURES]; // Blooming effect intermediate texture
ID3D10ShaderResourceView*	g_apTexBloomRV10[NUM_BLOOM_TEXTURES];
ID3D10RenderTargetView*		g_apTexBloomRTV10[NUM_BLOOM_TEXTURES];

ID3D10EffectShaderResourceVariable* g_pS0;
ID3D10EffectShaderResourceVariable* g_pS1;
ID3D10EffectShaderResourceVariable* g_pS2;
ID3D10EffectVectorVariable* g_pavSampleOffsetsHorizontal;
ID3D10EffectVectorVariable* g_pavSampleOffsetsVertical;
ID3D10EffectVectorVariable* g_pavSampleWeights;

ID3D10Texture2D*          g_pTexBrightPass10 = NULL; // Bright pass filter
ID3D10ShaderResourceView* g_pTexBrightPassRV10 = NULL;
ID3D10RenderTargetView*   g_pTexBrightPassRTV10 = NULL;

HRESULT MeasureLuminance10();
HRESULT BrightPassFilter10();
HRESULT RenderBloom10();

//--------------------------------------------------------------------------------------

D3D10_INPUT_ELEMENT_DESC layout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },  
};


void DrawFullScreenQuad10( ID3D10Device* pd3dDevice )
{
	pd3dDevice->IASetInputLayout( NULL );
	pd3dDevice->IASetIndexBuffer( NULL, DXGI_FORMAT_R32_UINT, 0 );        
    pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	pd3dDevice->Draw( 1, 0 );
}

void DrawFullScreenQuad10( ID3D10Device* pd3dDevice, unsigned width, unsigned height )
{
	D3D10_VIEWPORT vpOld[1];
    UINT nViewPorts = 1;
    pd3dDevice->RSGetViewports( &nViewPorts, vpOld );

    // Setup the viewport to match the backbuffer
    D3D10_VIEWPORT vp;
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dDevice->RSSetViewports( 1, &vp );

	pd3dDevice->IASetInputLayout( NULL );
	pd3dDevice->IASetIndexBuffer( NULL, DXGI_FORMAT_R32_UINT, 0 );        
    pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	pd3dDevice->Draw( 1, 0 );

	// Restore the old viewport
    pd3dDevice->RSSetViewports( nViewPorts, vpOld );
}

void ReleaseResolutionDependentResources();

//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D10_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

	pDeviceSettings->d3d10.SyncInterval = 0;
	pDeviceSettings->d3d10.AutoCreateDepthStencil = false;
	
	// Override default MSAA settings
	pDeviceSettings->d3d10.sd.SampleDesc.Count = 1;
	pDeviceSettings->d3d10.sd.SampleDesc.Quality = 0;

	// Disable MSAA settings from the settings dialog
    g_D3DSettingsDlg.GetDialogControl()->GetComboBox( DXUTSETTINGSDLG_D3D10_MULTISAMPLE_COUNT )->SetEnabled(false);
    g_D3DSettingsDlg.GetDialogControl()->GetComboBox( DXUTSETTINGSDLG_D3D10_MULTISAMPLE_QUALITY )->SetEnabled(false);
    g_D3DSettingsDlg.GetDialogControl()->GetStatic( DXUTSETTINGSDLG_D3D10_MULTISAMPLE_COUNT_LABEL )->SetEnabled(false);
    g_D3DSettingsDlg.GetDialogControl()->GetStatic( DXUTSETTINGSDLG_D3D10_MULTISAMPLE_QUALITY_LABEL )->SetEnabled(false);
    
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;
    
	ZeroMemory( g_pShadowMapTextureScaled, sizeof(g_pShadowMapTextureScaled) );
	ZeroMemory( g_pShadowMapTextureHole, sizeof(g_pShadowMapTextureHole) );
	ZeroMemory( g_apTexToneMap10, sizeof(g_apTexToneMap10) );
    ZeroMemory( g_apTexBloom10, sizeof(g_apTexBloom10) );

	V_RETURN( g_DialogResourceManager.OnD3D10CreateDevice(pd3dDevice) );
    V_RETURN( g_D3DSettingsDlg.OnD3D10CreateDevice(pd3dDevice ) );
    V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                                L"Arial", &g_pFont ) );
    V_RETURN( D3DX10CreateSprite( pd3dDevice, 512, &g_pSprite ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont, g_pSprite, 15 );
    
    V_RETURN( LoadEffect( pd3dDevice, L"TrainStation.fx", &g_pEffect ) );

	g_pUseAngleOptimization = g_pEffect->GetVariableByName( "g_UseAngleOptimization" )->AsScalar();
	g_pUseAngleOptimization->SetBool(g_UseAngleOptimization);
	
    // Create our vertex input layouts
    const D3D10_INPUT_ELEMENT_DESC scenelayout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    
	D3D10_PASS_DESC PassDesc;
	g_pEffect->GetTechniqueByName( "RenderDiffuse" )->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    V_RETURN( pd3dDevice->CreateInputLayout( scenelayout, sizeof(scenelayout) / sizeof(scenelayout[0]), PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &g_pSceneVertexLayout ) );
    
    WCHAR str[MAX_PATH];  
	
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"TrainStation\\TrainStation.sdkmesh" ) );
	V_RETURN( g_ObjectMesh.Create( pd3dDevice, str, true ) );
    
    D3D10_TEXTURE2D_DESC desc;
    desc.Width = SHADOW_MAP_SIZE;
    desc.Height = SHADOW_MAP_SIZE;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    
    V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pShadowMapTexture ) );
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pShadowMapTextureWorld ) );
    
    D3D10_SHADER_RESOURCE_VIEW_DESC SRVdesc;
    SRVdesc.Format = DXGI_FORMAT_R32_FLOAT;
    SRVdesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = 1;
    SRVdesc.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowMapTexture, &SRVdesc, &g_pShadowMapTextureSRV ) );
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowMapTextureWorld, &SRVdesc, &g_pShadowMapTextureWorldSRV ) );
    
    D3D10_DEPTH_STENCIL_VIEW_DESC DSVdesc;
    DSVdesc.Format = DXGI_FORMAT_D32_FLOAT;
    DSVdesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    DSVdesc.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateDepthStencilView( g_pShadowMapTexture, &DSVdesc, &g_pShadowMapTextureDSV ) );

	D3D10_RENDER_TARGET_VIEW_DESC RTVdesc;
	RTVdesc.Format = DXGI_FORMAT_R32_FLOAT;
	RTVdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
	RTVdesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pShadowMapTextureWorld, &RTVdesc, &g_pShadowMapTextureWorldRTV ) );
	
	// Create shadow map MIN-MAX mips
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;

	RTVdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
	RTVdesc.Texture2D.MipSlice = 0;
	
	for( int i = 0; i < SHADOW_MIPS; i++ )
	{
		desc.Format = DXGI_FORMAT_R32G32_TYPELESS;
		SRVdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		RTVdesc.Format = DXGI_FORMAT_R32G32_FLOAT;

		desc.Width /= 2;
		desc.Height /= 2;
		
		V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pShadowMapTextureScaled[i] ) );
		V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowMapTextureScaled[i], &SRVdesc, &g_pShadowMapTextureScaledSRV[i] ) );
		V_RETURN( pd3dDevice->CreateRenderTargetView( g_pShadowMapTextureScaled[i], &RTVdesc, &g_pShadowMapTextureScaledRTV[i] ) );

		if( i == SHADOW_HOLE_MIP )
		{
			desc.Format = DXGI_FORMAT_R32_TYPELESS;
			SRVdesc.Format = DXGI_FORMAT_R32_FLOAT;
			RTVdesc.Format = DXGI_FORMAT_R32_FLOAT;
						
			for( int j = 0; j < 2; j++ )
			{
				V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pShadowMapTextureHole[j] ) );
				V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowMapTextureHole[j], &SRVdesc, &g_pShadowMapTextureHoleSRV[j] ) );
				V_RETURN( pd3dDevice->CreateRenderTargetView( g_pShadowMapTextureHole[j], &RTVdesc, &g_pShadowMapTextureHoleRTV[j] ) );
			}
		}
	}

	// Currently use a single optimized texture
	desc.Format = DXGI_FORMAT_R32G32_TYPELESS;  
	SRVdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	RTVdesc.Format = DXGI_FORMAT_R32G32_FLOAT;

	// Create texture for optimized steps detection, used during main shading pass
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pShadowMapTextureScaledOpt ) );
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pShadowMapTextureScaledOpt, &SRVdesc, &g_pShadowMapTextureScaledOptSRV ) );
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pShadowMapTextureScaledOpt, &RTVdesc, &g_pShadowMapTextureScaledOptRTV ) );

	D3DXVECTOR3 lookAt( 100.0f, 100.0f, -50.0f );
	g_EyePosition = D3DXVECTOR3( -80.0f, 80.0f, 900.0f );
    g_ViewCamera.SetViewParams( &g_EyePosition, &lookAt );
	g_ViewCamera.SetScalers( 0.005f, 100.0f );
	g_ViewCamera.SetRotateButtons( true, false, false );
        
    D3DXVECTOR3 ModelCenter( 0.0f, 0.0f, 0.0f );
	g_LightCamera.SetModelCenter( ModelCenter );
	g_LightPosition.x = 500.0f;
    g_LightPosition.y = 1500.0f;
	g_LightPosition.z = 500.0f;
    
    g_LightCamera.SetViewParams( &g_LightPosition, &ModelCenter );

	D3DXMatrixOrthoLH(( D3DXMATRIX*)&g_LightProjMatrix, LIGHT_SIZE, LIGHT_SIZE, LIGHT_NEAR, LIGHT_FAR );

	// Create a set of common samplers
	g_pS0 = g_pEffect->GetVariableByName("s0")->AsShaderResource();
    g_pS1 = g_pEffect->GetVariableByName("s1")->AsShaderResource();
    g_pS2 = g_pEffect->GetVariableByName("s2")->AsShaderResource();
	
	g_pavSampleOffsetsHorizontal = g_pEffect->GetVariableByName("g_avSampleOffsetsHorizontal")->AsVector();
	g_pavSampleOffsetsVertical = g_pEffect->GetVariableByName("g_avSampleOffsetsVertical")->AsVector();
    g_pavSampleWeights = g_pEffect->GetVariableByName("g_avSampleWeights")->AsVector();

    return S_OK;
}

void DeleteDepthBufferTexture()
{
    SAFE_RELEASE( g_pDepthBufferTexture );
	SAFE_RELEASE( g_pDepthBufferTextureSRV );
	SAFE_RELEASE( g_pDepthBufferTextureDSV );

	SAFE_RELEASE( g_pDepthBufferTextureWS );
	SAFE_RELEASE( g_pDepthBufferTextureWSSRV );
	SAFE_RELEASE( g_pDepthBufferTextureWSRTV );
}

HRESULT CreateDepthBufferTexture( ID3D10Device* pd3dDevice, UINT width, UINT height )
{
	HRESULT hr = S_OK;
	DeleteDepthBufferTexture();

	D3D10_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
	desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format of the main depth buffer
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pDepthBufferTexture ) );
	
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pDepthBufferTextureWS ) );
    
    D3D10_SHADER_RESOURCE_VIEW_DESC SRVdesc;
	SRVdesc.Format = DXGI_FORMAT_R32_FLOAT;
    SRVdesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = 1;
    SRVdesc.Texture2D.MostDetailedMip = 0;
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pDepthBufferTexture, &SRVdesc, &g_pDepthBufferTextureSRV ) );
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pDepthBufferTexture, &SRVdesc, &g_pDepthBufferTextureWSSRV ) );

    D3D10_DEPTH_STENCIL_VIEW_DESC DSVdesc;
    DSVdesc.Format = DXGI_FORMAT_D32_FLOAT;
    DSVdesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    DSVdesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateDepthStencilView( g_pDepthBufferTexture, &DSVdesc, &g_pDepthBufferTextureDSV ) );
	
	D3D10_RENDER_TARGET_VIEW_DESC RTVdesc;
    RTVdesc.Format = DXGI_FORMAT_R32_FLOAT;
    RTVdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVdesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pDepthBufferTextureWS, &RTVdesc, &g_pDepthBufferTextureWSRTV ) );

	return hr;
}

void ReleaseScaledTextures()
{
	SAFE_RELEASE( g_pHDRTextureScaled );
	SAFE_RELEASE( g_pHDRTextureScaledSRV );
	SAFE_RELEASE( g_pHDRTextureScaledRTV );

	SAFE_RELEASE( g_pHDRTextureScaled2 );
	SAFE_RELEASE( g_pHDRTextureScaled2SRV );
	SAFE_RELEASE( g_pHDRTextureScaled2RTV );

	SAFE_RELEASE( g_pHDRTextureScaled3 );
	SAFE_RELEASE( g_pHDRTextureScaled3SRV );
	SAFE_RELEASE( g_pHDRTextureScaled3RTV );
}

HRESULT CreateScaledTextures( ID3D10Device* pd3dDevice )
{
	HRESULT hr = S_OK;

	ReleaseScaledTextures();
	
	D3D10_TEXTURE2D_DESC desc;
	g_pHDRTexture->GetDesc( &desc );
	desc.Format = DXGI_FORMAT_R16_FLOAT;
	desc.Width /= g_ScaleRatio;
	desc.Height /= g_ScaleRatio;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pHDRTextureScaled ) );
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pHDRTextureScaled2 ) );
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pHDRTextureScaled3 ) );

	D3D10_SHADER_RESOURCE_VIEW_DESC SRVdesc;
	SRVdesc.Format = DXGI_FORMAT_R16_FLOAT;
    SRVdesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = 1;
    SRVdesc.Texture2D.MostDetailedMip = 0;
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTextureScaled, &SRVdesc, &g_pHDRTextureScaledSRV ) );
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTextureScaled2, &SRVdesc, &g_pHDRTextureScaled2SRV ) );
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTextureScaled3, &SRVdesc, &g_pHDRTextureScaled3SRV ) );

	D3D10_RENDER_TARGET_VIEW_DESC RTVdesc;
    RTVdesc.Format = DXGI_FORMAT_R16_FLOAT;
    RTVdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVdesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTextureScaled, &RTVdesc, &g_pHDRTextureScaledRTV ) );
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTextureScaled2, &RTVdesc, &g_pHDRTextureScaled2RTV ) );
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTextureScaled3, &RTVdesc, &g_pHDRTextureScaled3RTV ) );

	return hr;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;
    
    V_RETURN( g_D3DSettingsDlg.OnD3D10ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc) );
    V_RETURN( g_DialogResourceManager.OnD3D10ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc) );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );

    float aspectRatio = (float)pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
    g_ViewCamera.SetProjParams( D3DX_PI * 0.25f, aspectRatio, 1.0f, 10000.0f );
    
    g_LightCamera.SetProjParams( D3DX_PI * 0.1f, 1.0f, 10.0f, 1000.0f );
    g_LightCamera.SetWindow( SHADOW_MAP_SIZE, SHADOW_MAP_SIZE );

	V_RETURN( CreateDepthBufferTexture( pd3dDevice, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height ) );

	g_pEffect->GetVariableByName( "g_BufferWidth" )->AsScalar()->SetFloat(float(pBackBufferSurfaceDesc->Width));
	g_pEffect->GetVariableByName( "g_BufferHeight" )->AsScalar()->SetFloat(float(pBackBufferSurfaceDesc->Height));
	g_pEffect->GetVariableByName( "g_BufferWidthInv" )->AsScalar()->SetFloat( 1.0f / float(pBackBufferSurfaceDesc->Width));
	g_pEffect->GetVariableByName( "g_BufferHeightInv" )->AsScalar()->SetFloat( 1.0f / float(pBackBufferSurfaceDesc->Height));

	ReleaseResolutionDependentResources();

	D3D10_TEXTURE2D_DESC desc;
    desc.Width = pBackBufferSurfaceDesc->Width / 8; // Noise texture scale
    desc.Height = pBackBufferSurfaceDesc->Height / 8;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
	
	D3D10_SUBRESOURCE_DATA data;
	BYTE *pNoiseData;
	pNoiseData = new BYTE[pBackBufferSurfaceDesc->Width * pBackBufferSurfaceDesc->Height];
	for( unsigned i = 0; i < pBackBufferSurfaceDesc->Width * pBackBufferSurfaceDesc->Height; i++ )
		pNoiseData[i] = BYTE( ( (float)rand() / (float)RAND_MAX ) * 256 );
	
	data.pSysMem = pNoiseData;
	data.SysMemPitch = pBackBufferSurfaceDesc->Width;
    V_RETURN( pd3dDevice->CreateTexture2D( &desc, &data, &g_pNoiseTexture ) );

	delete [] pNoiseData;
    
    D3D10_SHADER_RESOURCE_VIEW_DESC SRVdesc;
    SRVdesc.Format = DXGI_FORMAT_R8_UNORM;
    SRVdesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = 1;
    SRVdesc.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pNoiseTexture, &SRVdesc, &g_pNoiseTextureSRV ) );

	desc.Width = pBackBufferSurfaceDesc->Width;
    desc.Height = pBackBufferSurfaceDesc->Height;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pHDRTexture ) );

	desc.Format = DXGI_FORMAT_R16G16_FLOAT;
	desc.Width /= g_ScaleRatio;
	desc.Height /= g_ScaleRatio;
	V_RETURN( pd3dDevice->CreateTexture2D( &desc, NULL, &g_pEdgeTextureFS ) );

	SRVdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    SRVdesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = 1;
    SRVdesc.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pHDRTexture, &SRVdesc, &g_pHDRTextureSRV ) );

	SRVdesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	V_RETURN( pd3dDevice->CreateShaderResourceView( g_pEdgeTextureFS, &SRVdesc, &g_pEdgeTextureFSSRV ) );

	D3D10_RENDER_TARGET_VIEW_DESC RTVdesc;
    RTVdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    RTVdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVdesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pHDRTexture, &RTVdesc, &g_pHDRTextureRTV ) );

	RTVdesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	V_RETURN( pd3dDevice->CreateRenderTargetView( g_pEdgeTextureFS, &RTVdesc, &g_pEdgeTextureFSRTV ) );

	CreateScaledTextures( pd3dDevice );

	int nSampleLen = 1;
	for( int i=0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        D3D10_TEXTURE2D_DESC tmdesc;
        ZeroMemory( &tmdesc, sizeof( D3D10_TEXTURE2D_DESC ) );
        tmdesc.ArraySize = 1;
        tmdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
        tmdesc.Usage = D3D10_USAGE_DEFAULT;
        tmdesc.Format = DXGI_FORMAT_R16_FLOAT;
        tmdesc.Width = nSampleLen;
        tmdesc.Height = nSampleLen;
        tmdesc.MipLevels = 1;
        tmdesc.SampleDesc.Count = 1;

        V_RETURN( pd3dDevice->CreateTexture2D( &tmdesc, NULL, &g_apTexToneMap10[i] ) );

        D3D10_RENDER_TARGET_VIEW_DESC DescRT;
        DescRT.Format = tmdesc.Format;
        DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
        DescRT.Texture2D.MipSlice = 0;
        V_RETURN(pd3dDevice->CreateRenderTargetView( g_apTexToneMap10[i], &DescRT, &g_apTexToneMapRTV10[i] ));

        D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
        DescRV.Format = tmdesc.Format;
        DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        DescRV.Texture2D.MipLevels = 1;
        DescRV.Texture2D.MostDetailedMip = 0;
        V_RETURN(pd3dDevice->CreateShaderResourceView( g_apTexToneMap10[i], &DescRV, &g_apTexToneMapRV10[i] ));

        nSampleLen *= 3;
    }

    D3D10_TEXTURE2D_DESC Desc;
    ZeroMemory( &Desc, sizeof( D3D10_TEXTURE2D_DESC ) );
    Desc.ArraySize = 1;
    Desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    Desc.Usage = D3D10_USAGE_DEFAULT;
    Desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    Desc.Width = pBackBufferSurfaceDesc->Width;
    Desc.Height = pBackBufferSurfaceDesc->Height;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;

     // Create the bright pass texture
    Desc.Width /= 8;
    Desc.Height /= 8;
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pTexBrightPass10 ) );

    D3D10_RENDER_TARGET_VIEW_DESC DescRT;
	DescRT.Format = Desc.Format;
    DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN(pd3dDevice->CreateRenderTargetView( g_pTexBrightPass10, &DescRT, &g_pTexBrightPassRTV10 ));

    D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
	DescRV.Format = Desc.Format;
    DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN(pd3dDevice->CreateShaderResourceView( g_pTexBrightPass10, &DescRV, &g_pTexBrightPassRV10 ));

	// Create the temporary blooming effect textures
    for( int i=0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        D3D10_TEXTURE2D_DESC bmdesc;
        ZeroMemory( &bmdesc, sizeof( D3D10_TEXTURE2D_DESC ) );
        bmdesc.ArraySize = 1;
        bmdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
        bmdesc.Usage = D3D10_USAGE_DEFAULT;
        bmdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bmdesc.Width = pBackBufferSurfaceDesc->Width / 8;
        bmdesc.Height = pBackBufferSurfaceDesc->Height / 8;
        bmdesc.MipLevels = 1;
        bmdesc.SampleDesc.Count = 1;

        V_RETURN( pd3dDevice->CreateTexture2D( &bmdesc, NULL, &g_apTexBloom10[i] ) );

        // Create the render target view
        D3D10_RENDER_TARGET_VIEW_DESC DescRT;
        DescRT.Format = bmdesc.Format;
        DescRT.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
        DescRT.Texture2D.MipSlice = 0;
        V_RETURN(pd3dDevice->CreateRenderTargetView( g_apTexBloom10[i], &DescRT, &g_apTexBloomRTV10[i] ));

        // Create the shader resource view
        D3D10_SHADER_RESOURCE_VIEW_DESC DescRV;
        DescRV.Format = bmdesc.Format;
        DescRV.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        DescRV.Texture2D.MipLevels = 1;
        DescRV.Texture2D.MostDetailedMip = 0;
        V_RETURN(pd3dDevice->CreateShaderResourceView( g_apTexBloom10[i], &DescRV, &g_apTexBloomRV10[i] ));
    }

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    static float startTime = 0.0f;
    
	g_LightCamera.FrameMove( fElapsedTime );
	g_ViewCamera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
	
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
//     g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( true ) );
//     g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );    
  
    if( g_ShowHelp )
    {
        UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;
        g_pTxtHelper->SetInsertionPos( 2, nBackBufferHeight- 15 * 6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f ) );
		g_pTxtHelper->SetInsertionPos( 20, nBackBufferHeight - 15 * 5 );
        g_pTxtHelper->DrawTextLine( L"Move: 'W' 'S' 'A' 'D'\nEsc to exit \nTang Jian Ping");     
    }  

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    D3D10_VIEWPORT viewport;
	D3D10_TEXTURE2D_DESC desc;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
	
	static float clearColor[4] = { 0.65f, 0.65f, 0.9f, 1.0f };
	static float clearColorBlack[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	g_MProjection = *g_ViewCamera.GetProjMatrix();
    g_EyePosition = (D3DXVECTOR3)*g_ViewCamera.GetEyePt();
    g_MView = *g_ViewCamera.GetViewMatrix();

	g_pEffect->GetVariableByName( "g_rSamplingRate" )->AsScalar()->SetFloat( 1.0f / g_SamplingRate );

	D3DXVECTOR3 vecRight;
    vecRight.x = g_MView.m[0][0];
    vecRight.y = g_MView.m[1][0];
    vecRight.z = g_MView.m[2][0];
    
    D3DXVECTOR3 vecUp;
    vecUp.x = g_MView.m[0][1];
    vecUp.y = g_MView.m[1][1];
    vecUp.z = g_MView.m[2][1];

	D3DXVECTOR3 vecForward;
    vecForward.x = g_MView.m[0][2];
    vecForward.y = g_MView.m[1][2];
    vecForward.z = g_MView.m[2][2];

	g_pEffect->GetVariableByName( "g_WorldRight" )->AsVector()->SetFloatVectorArray( (float*)&vecRight, 0, 3 );
	g_pEffect->GetVariableByName( "g_WorldUp" )->AsVector()->SetFloatVectorArray( (float*)&vecUp, 0, 3 );
	g_pEffect->GetVariableByName( "g_WorldFront" )->AsVector()->SetFloatVectorArray( (float*)&vecForward, 0, 3 );
	g_pEffect->GetVariableByName( "g_ZNear" )->AsScalar()->SetFloat( g_ViewCamera.GetNearClip() );
	g_pEffect->GetVariableByName( "g_ZFar" )->AsScalar()->SetFloat( g_ViewCamera.GetFarClip() );

    g_MWorldViewProjection = g_MView * g_MProjection;
    g_pEffect->GetVariableByName( "g_ModelViewProj" )->AsMatrix()->SetMatrix( g_MWorldViewProjection );

	D3DXMATRIX g_MWorldViewProjectionInv;
	D3DXMatrixInverse( &g_MWorldViewProjectionInv, NULL, &g_MWorldViewProjection );
	g_pEffect->GetVariableByName( "g_MWorldViewProjectionInv" )->AsMatrix()->SetMatrix( g_MWorldViewProjectionInv );

	D3DXMATRIX g_ProjectionInv;
	D3DXMatrixInverse( &g_ProjectionInv, NULL, &g_MProjection );
	g_pEffect->GetVariableByName( "g_ProjectionInv" )->AsMatrix()->SetMatrix( g_ProjectionInv );

	g_pEffect->GetVariableByName( "g_EyePosition" )->AsVector()->SetFloatVector( (float*)&g_EyePosition );
	g_pEffect->GetVariableByName( "g_LightPosition" )->AsVector()->SetFloatVector( (float*)&g_LightPosition );

	D3DXMATRIX viewMatrix = *g_LightCamera.GetViewMatrix();
    
    vecRight.x = viewMatrix.m[0][0];
    vecRight.y = viewMatrix.m[1][0];
    vecRight.z = viewMatrix.m[2][0];
    
    vecUp.x = viewMatrix.m[0][1];
    vecUp.y = viewMatrix.m[1][1];
    vecUp.z = viewMatrix.m[2][1];
	
    vecForward.x = viewMatrix.m[0][2];
    vecForward.y = viewMatrix.m[1][2];
    vecForward.z = viewMatrix.m[2][2];
    
    g_pEffect->GetVariableByName( "g_LightRight" )->AsVector()->SetFloatVectorArray( (float*)&vecRight, 0, 3);
	g_pEffect->GetVariableByName( "g_LightUp" )->AsVector()->SetFloatVectorArray( (float*)&vecUp, 0, 3);
	g_pEffect->GetVariableByName( "g_LightForward" )->AsVector()->SetFloatVectorArray( (float*)&vecForward, 0, 3);

    g_MLightProjection = g_LightProjMatrix;
    g_MLight = viewMatrix;
    g_MWorldLightProjection = g_MLight * g_MLightProjection;
    g_pEffect->GetVariableByName( "g_ModelLightProj" )->AsMatrix()->SetMatrix( g_MWorldLightProjection );
	
	D3DXMatrixInverse( &g_MWorldLightProjectionInv, NULL, &g_MWorldLightProjection );
	g_pEffect->GetVariableByName( "g_ModelLightProjInv" )->AsMatrix()->SetMatrix( g_MWorldLightProjectionInv );

	ID3D10RenderTargetView *pDefaultRTV = DXUTGetD3D10RenderTargetView();	
	D3D10_VIEWPORT defaultViewport;

	// Set all critical states for main passes
	g_pEffect->GetTechniqueByName( "DummyPass" )->GetPassByIndex( 0 )->Apply( 0 );
    
    UINT viewPortsNum = 1;
    pd3dDevice->RSGetViewports( &viewPortsNum, &defaultViewport );
    
    pd3dDevice->ClearDepthStencilView( g_pShadowMapTextureDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );
    pd3dDevice->OMSetRenderTargets( 0, NULL, g_pShadowMapTextureDSV );
    
    viewport.Width = SHADOW_MAP_SIZE;
    viewport.Height = SHADOW_MAP_SIZE;
    pd3dDevice->RSSetViewports( 1, &viewport );
    
 	pd3dDevice->IASetInputLayout( g_pSceneVertexLayout );
	g_ObjectMesh.Render( pd3dDevice, g_pEffect->GetTechniqueByName( "ShadowmapPass" ) );

    pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureRTV, g_pDepthBufferTextureDSV );
	g_pHDRTexture->GetDesc( &desc );
	g_pEffect->GetVariableByName( "g_BufferWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width  );
	g_pEffect->GetVariableByName( "g_BufferHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );

    pd3dDevice->ClearRenderTargetView( g_pHDRTextureRTV, clearColor );
    pd3dDevice->ClearDepthStencilView( g_pDepthBufferTextureDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );
    pd3dDevice->RSSetViewports( 1, &defaultViewport );
    
	g_pEffect->GetVariableByName( "DepthTexture" )->AsShaderResource()->SetResource( g_pShadowMapTextureSRV );

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Draw main scene
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pd3dDevice->IASetInputLayout( g_pSceneVertexLayout );
	g_ObjectMesh.Render( pd3dDevice, g_pEffect->GetTechniqueByName( "RenderDiffuse" ), g_pEffect->GetVariableByName( "DiffuseTexture" )->AsShaderResource() );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Convert depth values to world scale
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureWorldRTV, NULL );
	g_pShadowMapTextureWorld->GetDesc( &desc );
	
	viewport.Width = desc.Width;
	viewport.Height = desc.Height;
    pd3dDevice->RSSetViewports( 1, &viewport );

	g_pS0->SetResource( g_pShadowMapTextureSRV );
	g_pEffect->GetVariableByName( "g_ModelLightProjInv" )->AsMatrix()->SetMatrix( g_MWorldLightProjectionInv );
	g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pConvertToWorld" )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice );

	g_pS0->SetResource( NULL );
	
	pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureScaledRTV[0], NULL );
	g_pShadowMapTextureScaled[0]->GetDesc( &desc );
	g_pEffect->GetVariableByName( "g_BufferWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width  );
	g_pEffect->GetVariableByName( "g_BufferHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );

	viewport.Width = desc.Width;
	viewport.Height = desc.Height;
    pd3dDevice->RSSetViewports( 1, &viewport );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Downscale Depth, produce Min and Max mips for optimized tracing
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	g_pS0->SetResource( g_pShadowMapTextureWorldSRV );
	g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pMinMax2x2_1" )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice );

	g_pS0->SetResource( NULL );

	for( int i = 0; i < SHADOW_MIPS - 1; i++ )
	{
		pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureScaledRTV[i + 1], NULL );
		g_pShadowMapTextureScaled[i + 1]->GetDesc( &desc );
		g_pEffect->GetVariableByName( "g_BufferWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width  );
		g_pEffect->GetVariableByName( "g_BufferHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );
		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );
				
		g_pS0->SetResource( g_pShadowMapTextureScaledSRV[i] );
		g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pMinMax2x2_2" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );

		g_pS0->SetResource( NULL );
	}

	pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureScaledOptRTV, NULL );
	g_pShadowMapTextureScaledOpt->GetDesc(&desc);
	g_pS0->SetResource( g_pShadowMapTextureScaledSRV[SHADOW_MIPS - 1] );
	g_pShadowMapTextureScaledOpt->GetDesc(&desc);
	g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pMinMax3x3" )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice );

	g_pS0->SetResource( NULL );
	
	pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureHoleRTV[0], NULL );
	pd3dDevice->ClearRenderTargetView( g_pShadowMapTextureHoleRTV[0], clearColorBlack );
	g_pShadowMapTextureHole[0]->GetDesc( &desc );
	viewport.Width = desc.Width;
	viewport.Height = desc.Height;
	pd3dDevice->RSSetViewports( 1, &viewport );

	g_pS0->SetResource( g_pShadowMapTextureScaledSRV[SHADOW_HOLE_MIP] );
	g_pEffect->GetVariableByName( "DepthTexture" )->AsShaderResource()->SetResource( g_pShadowMapTextureWorldSRV );
	
	g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pPropagateMinDepth_0" )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice );

	int oddPass = 0;

	for( int i = 0; i < 2; i++, oddPass = 1 - oddPass )
	{
		pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureHoleRTV[1 - oddPass], NULL );
		g_pS0->SetResource( g_pShadowMapTextureHoleSRV[oddPass] );
		g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pPropagateMinDepth_1" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );
	}

	for( int i = 0; i < 5; i++, oddPass = 1 - oddPass )
	{
		pd3dDevice->OMSetRenderTargets( 1, &g_pShadowMapTextureHoleRTV[1 - oddPass], NULL );
		g_pS0->SetResource( g_pShadowMapTextureHoleSRV[oddPass] );
		g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pPropagateMaxDepth" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	g_pS0->SetResource( NULL );

	if( g_DrawVolumeLight )
	{
		pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureScaledRTV, NULL );
		g_pHDRTextureScaled->GetDesc( &desc );
		g_pEffect->GetVariableByName( "g_BufferWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width  );
		g_pEffect->GetVariableByName( "g_BufferHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );

		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );

		g_pEffect->GetVariableByName( "DepthBufferTexture" )->AsShaderResource()->SetResource( g_pDepthBufferTextureSRV );
		g_pEffect->GetVariableByName( "NoiseTexture" )->AsShaderResource()->SetResource( g_pNoiseTextureSRV );
		g_pEffect->GetVariableByName( "DepthTexture" )->AsShaderResource()->SetResource( g_pShadowMapTextureWorldSRV );
		
		g_pS0->SetResource( g_pShadowMapTextureScaledOptSRV );
		g_pS1->SetResource( g_pShadowMapTextureHoleSRV[1] ); // Map for light density calculation

		g_pShadowMapTextureScaledOpt->GetDesc( &desc );
		g_pEffect->GetVariableByName( "g_CoarseDepthTexelSize" )->AsScalar()->SetFloat( LIGHT_SIZE / desc.Width );
			
		if( g_UseZOptimization )
			g_pEffect->GetTechniqueByName( "Tracing" )->GetPassByName( "FullScreen_Optimized" )->Apply( 0 );
		else
			g_pEffect->GetTechniqueByName( "Tracing" )->GetPassByName( "FullScreen_Base" )->Apply( 0 );

		DrawFullScreenQuad10( pd3dDevice );
		
		g_pS0->SetResource( NULL );
	}

	pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureRTV, NULL );
	pd3dDevice->RSSetViewports( 1, &defaultViewport );
		
	if( g_DrawVolumeLight )
	{
		pd3dDevice->OMSetRenderTargets( 1, &g_pDepthBufferTextureWSRTV, NULL );
		g_pShadowMapTextureWorld->GetDesc( &desc );
		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );

		g_pS0->SetResource( g_pDepthBufferTextureSRV );
		g_pEffect->GetTechniqueByName( "DepthProcessing" )->GetPassByName( "pConvertDepthWorldNormalized" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );
		
		//g_pS0->SetResource( g_pHDRTextureScaledSRV );
		g_pS0->SetResource( g_pDepthBufferTextureWSSRV ); // Use this map for edge detection
		
		g_pHDRTextureScaled->GetDesc( &desc );
		g_pEffect->GetVariableByName( "g_CoarseTextureWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width );
		g_pEffect->GetVariableByName( "g_CoarseTextureHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );
		
		// Perform edge detection based on the depth discontinuities
		pd3dDevice->OMSetRenderTargets( 1, &g_pEdgeTextureFSRTV, NULL );
		g_pEdgeTextureFS->GetDesc( &desc );
		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );
		
		g_pEffect->GetTechniqueByName( "EdgeProcessing" )->GetPassByName( "pEdgeDetection" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );

		g_pHDRTextureScaled->GetDesc( &desc );
		g_pEffect->GetVariableByName( "g_CoarseTextureWidthInv" )->AsScalar()->SetFloat( 1.0f / desc.Width );
		g_pEffect->GetVariableByName( "g_CoarseTextureHeightInv" )->AsScalar()->SetFloat( 1.0f / desc.Height );

		// Blur the image along the edges
		pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureScaled2RTV, NULL );
		g_pHDRTextureScaled2->GetDesc( &desc );
		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );
		g_pS0->SetResource( g_pHDRTextureScaledSRV );
		g_pS2->SetResource( g_pEdgeTextureFSSRV );
		g_pEffect->GetTechniqueByName( "EdgeProcessing" )->GetPassByName( "pGradientBlur" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );

		// Blur the image with 3x3 kernel
		pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureScaled3RTV, NULL );
		g_pHDRTextureScaled2->GetDesc( &desc );
		viewport.Width = desc.Width;
		viewport.Height = desc.Height;
		pd3dDevice->RSSetViewports( 1, &viewport );
		g_pS0->SetResource( g_pHDRTextureScaled2SRV );
		g_pEffect->GetTechniqueByName( "EdgeProcessing" )->GetPassByName( "pImageBlur" )->Apply( 0 );
		DrawFullScreenQuad10( pd3dDevice );

		// Blend Volume Light with main scene
		pd3dDevice->OMSetRenderTargets( 1, &g_pHDRTextureRTV, NULL );
		pd3dDevice->RSSetViewports( 1, &defaultViewport );
		g_pS0->SetResource( g_pHDRTextureScaledSRV );
		g_pS1->SetResource( g_pHDRTextureScaled3SRV );

		UINT passIndex = 0;
		
		if( g_DrawScene == false )
			passIndex = 1;

		if( g_UsePostProcessing )
			g_pEffect->GetTechniqueByName( "BlendFullscreenPP" )->GetPassByIndex( passIndex )->Apply( 0 );
		else
			g_pEffect->GetTechniqueByName( "BlendFullscreen" )->GetPassByIndex( passIndex )->Apply( 0 );
		
		
		DrawFullScreenQuad10( pd3dDevice );

		g_pS0->SetResource( NULL );
	}

	MeasureLuminance10();     
	BrightPassFilter10();
	
	if( g_DrawBloom )
		RenderBloom10();

	ID3D10RenderTargetView* RTViews[ 1 ] = { pDefaultRTV };
	pd3dDevice->OMSetRenderTargets( 1, RTViews, g_pDepthBufferTextureDSV );

	g_pS0->SetResource( g_pHDRTextureSRV );
	g_pS1->SetResource( g_apTexToneMapRV10[0] );
	g_pS2->SetResource( g_DrawBloom ? g_apTexBloomRV10[0] : NULL );
	
	g_pEffect->GetTechniqueByName( "tFinalPass" )->GetPassByIndex( 0 )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice );

	g_pS0->SetResource( NULL );
	g_pS1->SetResource( NULL );
	g_pS2->SetResource( NULL );

	// Restore all critical states for HUD drawing
    g_pEffect->GetTechniqueByName( "DummyPass" )->GetPassByIndex( 0 )->Apply( 0 );

    g_HUD.OnRender( fElapsedTime );
    RenderText();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10ReleasingSwapChain();
}

void ReleaseResolutionDependentResources()
{
	ReleaseScaledTextures();

	SAFE_RELEASE( g_pNoiseTexture );
	SAFE_RELEASE( g_pNoiseTextureSRV );

	SAFE_RELEASE( g_pHDRTexture );
	SAFE_RELEASE( g_pHDRTextureSRV );
	SAFE_RELEASE( g_pHDRTextureRTV );

	SAFE_RELEASE( g_pEdgeTextureFS );
	SAFE_RELEASE( g_pEdgeTextureFSSRV );
	SAFE_RELEASE( g_pEdgeTextureFSRTV );
	
	for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        SAFE_RELEASE( g_apTexToneMap10[i] );
		SAFE_RELEASE( g_apTexToneMapRTV10[i] );
		SAFE_RELEASE( g_apTexToneMapRV10[i] );
    }

	for( int i = 0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        SAFE_RELEASE( g_apTexBloom10[i] );
		SAFE_RELEASE( g_apTexBloomRTV10[i] );
		SAFE_RELEASE( g_apTexBloomRV10[i] );
    }

	SAFE_RELEASE( g_pTexBrightPass10 );
	SAFE_RELEASE( g_pTexBrightPassRV10 );
	SAFE_RELEASE( g_pTexBrightPassRTV10 );	
}

//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
    SAFE_RELEASE( g_pFont );
    SAFE_RELEASE( g_pSprite );
    SAFE_RELEASE( g_pEffect );
   
    SAFE_DELETE( g_pTxtHelper );
    
	SAFE_RELEASE( g_pShadowMapTexture );
    SAFE_RELEASE( g_pShadowMapTextureSRV );
    SAFE_RELEASE( g_pShadowMapTextureDSV );

	SAFE_RELEASE( g_pShadowMapTextureWorld );
    SAFE_RELEASE( g_pShadowMapTextureWorldSRV );
    SAFE_RELEASE( g_pShadowMapTextureWorldRTV );

	for( int i = 0; i < SHADOW_MIPS; i++ )
	{
		SAFE_RELEASE( g_pShadowMapTextureScaled[i] );
		SAFE_RELEASE( g_pShadowMapTextureScaledSRV[i] );
		SAFE_RELEASE( g_pShadowMapTextureScaledRTV[i] );
	}

	SAFE_RELEASE( g_pShadowMapTextureScaledOpt );
	SAFE_RELEASE( g_pShadowMapTextureScaledOptSRV );
	SAFE_RELEASE( g_pShadowMapTextureScaledOptRTV );

	for( int i = 0; i < 2; i++ )
	{
		SAFE_RELEASE( g_pShadowMapTextureHole[i] );
		SAFE_RELEASE( g_pShadowMapTextureHoleSRV[i] );
		SAFE_RELEASE( g_pShadowMapTextureHoleRTV[i] );
	}

	ReleaseResolutionDependentResources();

	DeleteDepthBufferTexture();
	g_ObjectMesh.Destroy();

	SAFE_RELEASE( g_pSceneVertexLayout );

	g_D3DSettingsDlg.OnD3D10DestroyDevice();
    g_DialogResourceManager.OnD3D10DestroyDevice();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Try to pass messages to settings dialog
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    if( g_HUD.MsgProc( hWnd, uMsg, wParam, lParam ) )
    {
        return 0;
    }

	static unsigned MouseMask = 0;
    	
	g_LightCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
	g_ViewCamera.HandleMessages(hWnd, uMsg, wParam, lParam);
    
    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( !bKeyDown )
        return;

    if( nChar == VK_F1 )
    {
        g_ShowHelp = !g_ShowHelp;
    }
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, 
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, 
                       int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D10 DXUT callbacks. Remove these sets if the app doesn't need to support D3D10
    DXUTSetCallbackD3D10DeviceAcceptable( IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );
    DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );

    // IMPORTANT: set SDK media search path to include source directory of this sample, when started from .\Bin
    HRESULT hr;
    V_RETURN( DXUTSetMediaSearchPath(L"..\\Source\\TrainStation") );
    
    // Perform any application-level initialization here

    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    
    int iY = 10; 
    g_HUD.Init( &g_DialogResourceManager );
    g_HUD.SetCallback( OnGUIEvent );

	/*
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

	iY += 50;
    WCHAR str[MAX_PATH];
    StringCchPrintf( str, MAX_PATH, L"Sampling Rate: %.1f", g_SamplingRate );
    g_HUD.AddStatic( IDC_SAMPLINGRATE_STATIC, str, 0, iY+=24, 140, 24 );
    g_HUD.AddSlider( IDC_SAMPLINGRATE, 0, iY+=24, 140, 24,  1, 100, int(g_SamplingRate * 10) );

	iY+=24;
	CDXUTCheckBox* pCheckBox = NULL;
	g_HUD.AddCheckBox( IDC_USEANGLEOPTIMIZATION, L"Use Angle (O)ptimization", 0, iY+=24, 160, 24, g_UseAngleOptimization, 'O', false, &pCheckBox );
	g_HUD.AddCheckBox( IDC_USEZOPTIMIZATION, L"Use Min & Max (Z)", 0, iY+=24, 140, 24, g_UseZOptimization, 'Z', false, &pCheckBox );
	
	iY+=24;
	g_HUD.AddCheckBox( IDC_DRAWSCENE, L"(R)ender Scene", 0, iY+=24, 140, 24, g_DrawScene, 'R', false, &pCheckBox );
	g_HUD.AddCheckBox( IDC_DRAWVOLUMELIGHT, L"Render (V)olume Light", 0, iY+=24, 140, 24, g_DrawVolumeLight, 'V', false, &pCheckBox );
	g_HUD.AddCheckBox( IDC_USEPOSTPROCESSING, L"Use (P)ost Processing", 0, iY+=24, 140, 24, g_UsePostProcessing, 'P', false, &pCheckBox );
	iY+=24;
	g_HUD.AddCheckBox( IDC_DRAWBLOOM, L"Render (B)loom", 0, iY+=24, 140, 24, g_DrawBloom, 'B', false, &pCheckBox );
		
	iY+=24;
	StringCchPrintf( str, MAX_PATH, L"Downscale Ratio: %d", (int)g_ScaleRatio );
	g_HUD.AddStatic( IDC_SCALERATIO_STATIC, str, 0, iY+=24, 140, 24 );
    g_HUD.AddSlider( IDC_SCALERATIO, 0, iY+=24, 140, 24,  0, 3, (int)sqrtf((float)g_ScaleRatio) );
	g_DrawScaled = g_ScaleRatio > 1 ? true : false;
	*/ 

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"TrainStation" ); 
    DXUTCreateDevice( true, 1024, 576 );   
	DXUTToggleFullScreen();
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Load effect from file
//--------------------------------------------------------------------------------------
HRESULT LoadEffect( ID3D10Device* pd3dDevice, WCHAR *pFileName, ID3D10Effect **ppEffect )
{
    HRESULT hr;
    WCHAR path[MAX_PATH];

    V_RETURN( DXUTFindDXSDKMediaFileCch( path, MAX_PATH, pFileName ) );
    V_RETURN( D3DX10CreateEffectFromFile( path, NULL, NULL, "fx_4_0", D3D10_SHADER_NO_PRESHADER, 0, pd3dDevice, NULL, NULL, ppEffect, NULL, &hr ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	WCHAR str[MAX_PATH] = {0};
	int scaleRatio = g_ScaleRatio;
	
	switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_TOGGLEREF:
            DXUTToggleREF();
            break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
            break;
		case IDC_SAMPLINGRATE:
            g_SamplingRate = (float)g_HUD.GetSlider( IDC_SAMPLINGRATE )->GetValue() * 0.1f;
            StringCchPrintf( str, MAX_PATH, L"Sampling Rate: %.1f", g_SamplingRate );
            g_HUD.GetStatic( IDC_SAMPLINGRATE_STATIC )->SetText( str );
            break;
		case IDC_USEANGLEOPTIMIZATION:
			g_UseAngleOptimization = !g_UseAngleOptimization;
			g_pUseAngleOptimization->SetBool(g_UseAngleOptimization);
			break;
		case IDC_USEPOSTPROCESSING:
			g_UsePostProcessing = !g_UsePostProcessing;
			break;
		case IDC_DRAWVOLUMELIGHT:
			g_DrawVolumeLight = !g_DrawVolumeLight;
			break;
		case IDC_DRAWSCENE:
			g_DrawScene = !g_DrawScene;
			break;
		case IDC_DRAWBLOOM:
			g_DrawBloom = !g_DrawBloom;
			break;
		case IDC_USEZOPTIMIZATION:
			g_UseZOptimization = !g_UseZOptimization;
		case IDC_SCALERATIO:
            g_ScaleRatio = (int)pow(2.0, g_HUD.GetSlider( IDC_SCALERATIO )->GetValue());
            StringCchPrintf( str, MAX_PATH, L"Downscale Ratio: %d", g_ScaleRatio );
            g_HUD.GetStatic( IDC_SCALERATIO_STATIC )->SetText( str );

			g_DrawScaled = g_ScaleRatio > 1 ? true : false;

			if( scaleRatio != g_ScaleRatio )
				CreateScaledTextures( DXUTGetD3D10Device() );

			g_pEffect->GetVariableByName( "g_DownscaleRatio" )->AsScalar()->SetFloat( (float)g_ScaleRatio );
           
			break;
    }
}

// HDR

//-----------------------------------------------------------------------------
// Name: MeasureLuminance()
// Desc: Measure the average log luminance in the scene.
//-----------------------------------------------------------------------------
HRESULT MeasureLuminance10()
{
    ID3D10EffectShaderResourceVariable* pS0;
    ID3D10EffectShaderResourceVariable* pS1;
    pS0 = g_pS0;
    pS1 = g_pS1;

    ID3D10ShaderResourceView* pTexSrc = NULL;
    ID3D10RenderTargetView* pSurfDest = NULL;

    //-------------------------------------------------------------------------
    // Initial sampling pass to convert the image to the log of the grayscale
    //-------------------------------------------------------------------------
    pTexSrc = g_pHDRTextureSRV;
    pSurfDest = g_apTexToneMapRTV10[NUM_TONEMAP_TEXTURES-1];

    D3D10_TEXTURE2D_DESC descSrc; 
    g_pHDRTexture->GetDesc( &descSrc );
    D3D10_TEXTURE2D_DESC descDest;
    g_apTexToneMap10[NUM_TONEMAP_TEXTURES-1]->GetDesc( &descDest );

    ID3D10Device* pd3dDevice = DXUTGetD3D10Device();
    ID3D10RenderTargetView* aRTViews[ 1 ] = { pSurfDest };
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );
    pS0->SetResource( pTexSrc );

    g_pEffect->GetTechniqueByName( "tDownScale2x2_Lum" )->GetPassByIndex(0)->Apply(0);
	DrawFullScreenQuad10( pd3dDevice, descDest.Width, descDest.Height );

    pS0->SetResource( NULL );

    //-------------------------------------------------------------------------
    // Iterate through the remaining tone map textures
    //------------------------------------------------------------------------- 
    for( int i = NUM_TONEMAP_TEXTURES - 1; i > 0; i-- )
    {
        // Cycle the textures
        pTexSrc = g_apTexToneMapRV10[i];
        pSurfDest = g_apTexToneMapRTV10[i-1];

        D3D10_TEXTURE2D_DESC desc;
        g_apTexToneMap10[i]->GetDesc( &desc );

        aRTViews[ 0 ] = pSurfDest;
        pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

        pS0->SetResource( pTexSrc );
  
        g_pEffect->GetTechniqueByName( "tDownScale3x3" )->GetPassByIndex(0)->Apply(0);
		DrawFullScreenQuad10( pd3dDevice, desc.Width/3, desc.Height/3 );
       
        pS0->SetResource( NULL );
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: BrightPassFilter
// Desc: Prepare for the bloom pass by removing dark information from the scene
//-----------------------------------------------------------------------------
HRESULT BrightPassFilter10()
{
    ID3D10EffectShaderResourceVariable* pS0;
    ID3D10EffectShaderResourceVariable* pS1;
    pS0 = g_pS0;
    pS1 = g_pS1;

    ID3D10Device* pd3dDevice = DXUTGetD3D10Device();

    const DXGI_SURFACE_DESC* pBackBufDesc = DXUTGetDXGIBackBufferSurfaceDesc();

    ID3D10RenderTargetView* aRTViews[ 1 ] = { g_pTexBrightPassRTV10 };
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    pS0->SetResource( g_pHDRTextureSRV );
    pS1->SetResource( g_apTexToneMapRV10[0] );

	g_pEffect->GetTechniqueByName( "tDownScale3x3_BrightPass" )->GetPassByIndex(0)->Apply(0);
	DrawFullScreenQuad10( pd3dDevice, pBackBufDesc->Width/8, pBackBufDesc->Height/8 );

    pS0->SetResource( NULL );
    pS1->SetResource( NULL );

    return S_OK;
}

//--------------------------------------------------------------------------------------
float GaussianDistribution( float x, float y, float rho )
{
    float g = 1.0f / sqrtf( 2.0f * D3DX_PI * rho * rho );
    g *= expf( -(x*x + y*y)/(2*rho*rho) );

    return g;
}

//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_Bloom()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT GetSampleOffsets_Bloom_D3D10(DWORD dwD3DTexSize,
                                                float afTexCoordOffset[15],
                                                D3DXVECTOR4* avColorWeight,
                                                float fDeviation, float fMultiplier )
{
    int i=0;
    float tu = 1.0f / (float)dwD3DTexSize;

    // Fill the center texel
    float weight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
    avColorWeight[7] = D3DXVECTOR4( weight, weight, weight, 1.0f );

    afTexCoordOffset[7] = 0.0f;
    
    // Fill the right side
    for( i=1; i < 8; i++ )
    {
        weight = fMultiplier * GaussianDistribution( (float)i, 0, fDeviation );
        afTexCoordOffset[7-i] = i * tu;

        avColorWeight[7-i] = D3DXVECTOR4( weight, weight, weight, 1.0f );
    }

    // Copy to the left side
    for( i=8; i < 15; i++ )
    {
        avColorWeight[i] = avColorWeight[14-i];
        afTexCoordOffset[i] = -afTexCoordOffset[14-i];
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: RenderBloom()
// Desc: Render the blooming effect
//-----------------------------------------------------------------------------
HRESULT RenderBloom10()
{
    ID3D10Device* pd3dDevice = DXUTGetD3D10Device();

    HRESULT hr = S_OK;
    int i=0;

    ID3D10EffectShaderResourceVariable* pS0;
    ID3D10EffectShaderResourceVariable* pS1;
    pS0 = g_pS0;
    pS1 = g_pS1;

    D3DXVECTOR4 avSampleOffsets[15];
    float       afSampleOffsets[15];
    D3DXVECTOR4 avSampleWeights[15];

    // Vertical Blur
    ID3D10RenderTargetView* pSurfDest = g_apTexBloomRTV10[1];
    D3D10_TEXTURE2D_DESC RTDesc;
    g_apTexBloom10[1]->GetDesc( &RTDesc );
    
    D3D10_TEXTURE2D_DESC desc;
    g_pTexBrightPass10->GetDesc( &desc );

    hr = GetSampleOffsets_Bloom_D3D10( (DWORD)desc.Width, afSampleOffsets, avSampleWeights, 3.0f, 1.25f );
    
	for( i = 0; i < 15; i++ )
    {
        avSampleOffsets[i] = D3DXVECTOR4( afSampleOffsets[i], 0.0f, 0.0f, 0.0f );
    }
     
    g_pavSampleOffsetsHorizontal->SetFloatVectorArray( (float*)avSampleOffsets, 0, 15 );
    g_pavSampleWeights->SetFloatVectorArray( (float*)avSampleWeights, 0, 15 );
   
    ID3D10RenderTargetView* aRTViews[ 1 ] = { pSurfDest };
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    pS0->SetResource( g_pTexBrightPassRV10 );
    g_pEffect->GetTechniqueByName( "BloomTech" )->GetPassByIndex( 0 )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice, RTDesc.Width, RTDesc.Height );
    
	pS0->SetResource( NULL );
    ID3D10ShaderResourceView* pViews[4] = {0,0,0,0};
    pd3dDevice->PSSetShaderResources( 0, 4, pViews );

    // Horizontal Blur
    pSurfDest = g_apTexBloomRTV10[0];
    
    hr = GetSampleOffsets_Bloom_D3D10( (DWORD)desc.Height, afSampleOffsets, avSampleWeights, 3.0f, 1.25f );
    for( i = 0; i < 15; i++ )
    {
        avSampleOffsets[i] = D3DXVECTOR4( 0.0f, afSampleOffsets[i], 0.0f, 0.0f );
    }

    g_pavSampleOffsetsVertical->SetRawValue( avSampleOffsets, 0, sizeof(avSampleOffsets) );
    g_pavSampleWeights->SetRawValue( avSampleWeights, 0, sizeof(avSampleWeights) );
   
    aRTViews[ 0 ] = pSurfDest;
    pd3dDevice->OMSetRenderTargets( 1, aRTViews, NULL );

    pS0->SetResource( g_apTexBloomRV10[1] );       
    g_pEffect->GetTechniqueByName( "BloomTech" )->GetPassByIndex( 1 )->Apply( 0 );
	DrawFullScreenQuad10( pd3dDevice, RTDesc.Width, RTDesc.Height );
    pS0->SetResource( NULL );

    return hr;
}