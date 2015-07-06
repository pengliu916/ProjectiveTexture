//--------------------------------------------------------------------------------------
// File: DX11ProjectiveTexturing.cpp
//
// Empty starting point for new Direct3D 9 and/or Direct3D 11 applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUT\Optional\DXUTcamera.h"
#include "DXUT\Optional\SDKmisc.h"

#include <xnamath.h>

//--------------------------------------------------------------------------------------
//Structures
//--------------------------------------------------------------------------------------
struct MyVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};

struct CBChangesEveryFrame
{
	XMMATRIX mWorld; 
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMMATRIX mPTView;
	XMMATRIX mPTProj;
	D3DXVECTOR3 projectorPos;
	float timer;
	D3DXVECTOR3 projectorDir;
	float jjk;
	XMFLOAT4 vMeshColor;
};

//--------------------------------------------------------------------------------------
//Global Variables
//--------------------------------------------------------------------------------------
ID3D11VertexShader*					g_pVertexShader = NULL;
ID3D11PixelShader*					g_pPixelShader = NULL;
ID3D11GeometryShader*			g_pGeometryShader = NULL;
ID3D11PixelShader*					g_pRTPixelShader = NULL;
ID3D11InputLayout*					g_pVertexLayout = NULL;
ID3D11Buffer*							g_pVertexBuffer	= NULL;
ID3D11Buffer*							g_pVertexBufferP	= NULL;
ID3D11Buffer*							g_pIndexBuffer = NULL;
ID3D11Buffer*							g_pIndexBufferP = NULL;
ID3D11Buffer*							g_pCBChangesEveryFrame = NULL;
ID3D11Texture2D*						g_pRenderTargetTexture = NULL;
ID3D11RenderTargetView*			g_pRTTextureRTV = NULL;
ID3D11ShaderResourceView*		g_pTextureRVforPlane[3];//Last is the RTT
ID3D11ShaderResourceView*		g_pTextureRV[3];//Last is the RTT
ID3D10DepthStencilView*			g_pDepthStencilDSV = NULL;

D3D11_VIEWPORT						g_mRTViewport;
D3D11_VIEWPORT						g_mViewport;

ID3D11Texture2D*						pDepthStencil = NULL;
ID3D11DepthStencilView*			pDSVn;

ID3D11SamplerState*					g_pSamplerLinear = NULL;
XMMATRIX								g_World;
XMMATRIX								g_View;
XMMATRIX								g_Proj;
XMMATRIX								g_PTView;
XMMATRIX								g_PTProj;
XMFLOAT4									g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );

UINT											ProjectRTwidth=1920;
UINT											ProjectRTheight=1200;
CModelViewerCamera					g_VCamera;
CModelViewerCamera					g_TextureProjector;

const UINT numIdex = 288;

MyVertex objectVertices[numIdex];
WORD indices[numIdex];

MyVertex planeVertices[4]=
{
	{XMFLOAT3(-30.0f,-5.0f,30.0f), XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT2(0.0f,0.0f)},
	{XMFLOAT3(-30.0f,-5.0f,-30.0f), XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT2(0.0f,1.0f)},
	{XMFLOAT3(30.0f,-5.0f,-30.0f), XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT2(1.0f,1.0f)},
	{XMFLOAT3(30.0f,-5.0f,30.0f), XMFLOAT3(0.0f,1.0f,0.0f),XMFLOAT2(1.0f,0.0f)}
};
WORD planeIndices[6]=
{
	0,2,1,
	0,3,2
};

HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG |D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
	if( FAILED(hr) )
	{
		if( pErrorBlob != NULL )
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
		if( pErrorBlob ) pErrorBlob->Release();
		return hr;
	}
	if( pErrorBlob ) pErrorBlob->Release();

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
									  DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	return true;
}

void CreateMesh()
{
	XMVECTOR temp;
	MyVertex part[12]=
	{
		{ XMFLOAT3(-1.0f,1.0f,1.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.25f,0.0f)},
		{ XMFLOAT3(1.0f,1.0f,1.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.75f,0.0f)},
		{ XMFLOAT3(1.0f,1.0f,2.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.75f,0.75f)},

		{ XMFLOAT3(-1.0f,1.0f,1.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.25f,0.0f)},
		{ XMFLOAT3(1.0f,1.0f,2.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.75f,0.75f)},
		{ XMFLOAT3(-1.0f,1.0f,2.0f), XMFLOAT3(0.0f,-1.0f,0.0f),XMFLOAT2(0.25f,0.75f)},

		{ XMFLOAT3(-1.0f,1.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(0.25f,0.75f)},
		{ XMFLOAT3(1.0f,1.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(0.75f,0.75f)},
		{ XMFLOAT3(2.0f,2.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(1.0f,0.5f)},

		{ XMFLOAT3(-1.0f,1.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(0.25f,0.75f)},
		{ XMFLOAT3(2.0f,2.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(1.0f,0.5f)},
		{ XMFLOAT3(-2.0f,2.0f,2.0f), XMFLOAT3(0.0f,0.0f,1.0f),XMFLOAT2(0.0f,0.5f)},
	};
	UINT idx=0;

	for(int j=0;j<4;j++)
	{
		for(int x=0;x<12;x++)
		{
			temp = XMLoadFloat3( &part[x].Pos);
			//temp = XMVector3Transform(temp,XMMatrixRotationY(i*XM_2PI/4.0f));
			temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Pos,temp);

			temp = XMLoadFloat3( &part[x].Normal);
			//temp = XMVector3Transform(temp,XMMatrixRotationY(i*XM_2PI/4.0f));
			temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Normal,temp);

			objectVertices[idx].Tex = part[x].Tex;
			indices[idx] = idx;
			idx++;
		}
	}
	for(int i=1;i<4;i++)
	{
		for(int x=0;x<48;x++)
		{
			temp = XMLoadFloat3( &objectVertices[x].Pos);
			temp = XMVector3Transform(temp,XMMatrixRotationY(i*XM_2PI/4.0f));
			//temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Pos,temp);

			temp = XMLoadFloat3( &objectVertices[x].Normal);
			temp = XMVector3Transform(temp,XMMatrixRotationY(i*XM_2PI/4.0f));
			//temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Normal,temp);

			objectVertices[idx].Tex = objectVertices[x].Tex;
			indices[idx] = idx;
			idx++;
		}
	}
	for(int i=-1;i<2;i+=2)
	{
		for(int x=0;x<48;x++)
		{
			temp = XMLoadFloat3( &objectVertices[x].Pos);
			temp = XMVector3Transform(temp,XMMatrixRotationX(i*XM_2PI/4.0f));
			//temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Pos,temp);

			temp = XMLoadFloat3( &objectVertices[x].Normal);
			temp = XMVector3Transform(temp,XMMatrixRotationX(i*XM_2PI/4.0f));
			//temp = XMVector3Transform(temp,XMMatrixRotationZ(j*XM_2PI/4.0f));
			XMStoreFloat3(&objectVertices[idx].Normal,temp);

			objectVertices[idx].Tex = objectVertices[x].Tex;
			indices[idx] = idx;
			idx++;
		}
	}
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
									 void* pUserContext )
{
	HRESULT hr=S_OK;

	ID3D11DeviceContext* pd3dImmediateContext=DXUTGetD3D11DeviceContext();

	ID3DBlob* pVSBlob = NULL;
	V_RETURN(CompileShaderFromFile(L"ProjectiveTexture.fx","VS","vs_4_0",&pVSBlob));
	V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&g_pVertexShader));

	ID3DBlob* pPSBlob = NULL;
	V_RETURN(CompileShaderFromFile(L"ProjectiveTexture.fx","PS","ps_4_0",&pPSBlob));
	V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&g_pPixelShader));

	V_RETURN(CompileShaderFromFile(L"ProjectiveTexture.fx","RTPS","ps_4_0",&pPSBlob));
	V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&g_pRTPixelShader));
	pPSBlob->Release();

	ID3DBlob* pGSBlob = NULL;
	V_RETURN(CompileShaderFromFile(L"ProjectiveTexture.fx","GS","gs_4_0",&pGSBlob));
	V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&g_pGeometryShader));
	pGSBlob->Release();

	D3D11_INPUT_ELEMENT_DESC inputLayout[]=
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0 ,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT numElements = ARRAYSIZE(inputLayout);

	V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,numElements,pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&g_pVertexLayout));
	pVSBlob->Release();

	pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);

	CreateMesh();

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( MyVertex ) * numIdex;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = objectVertices;

	V_RETURN(pd3dDevice->CreateBuffer(&bd,&InitData,&g_pVertexBuffer));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * numIdex;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	V_RETURN(pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer ));


	bd.ByteWidth = sizeof( MyVertex ) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = planeVertices;

	V_RETURN(pd3dDevice->CreateBuffer(&bd,&InitData,&g_pVertexBufferP));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = planeIndices;
	V_RETURN(pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBufferP ));

	// Set primitive topology
	pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );


	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;



	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame ));

	V_RETURN(D3DX11CreateShaderResourceViewFromFile( pd3dDevice, L"concrete.bmp", NULL, NULL, &g_pTextureRV[0], NULL ));
	V_RETURN(D3DX11CreateShaderResourceViewFromFile( pd3dDevice, L"Projector.jpg", NULL, NULL, &g_pTextureRV[1], NULL ));

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear ));

	// Initialize RenderTarget related material!!!!
	D3D11_TEXTURE2D_DESC RTTextureDesc={0};
	RTTextureDesc.Width = ProjectRTwidth;
	RTTextureDesc.Height = ProjectRTheight;
	RTTextureDesc.MipLevels = 1;
	RTTextureDesc.ArraySize = 1;
	RTTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	RTTextureDesc.SampleDesc.Count = 1;
	RTTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	RTTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	RTTextureDesc.CPUAccessFlags = 0;
	RTTextureDesc.MiscFlags = 0;

	// Create the render target texture.
	V_RETURN(pd3dDevice->CreateTexture2D(&RTTextureDesc, NULL, &g_pRenderTargetTexture));

	D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
	ZeroMemory( &RTViewDesc, sizeof(RTViewDesc) );
	RTViewDesc.Format = RTTextureDesc.Format;
	RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	V_RETURN(pd3dDevice->CreateRenderTargetView(g_pRenderTargetTexture, &RTViewDesc, &g_pRTTextureRTV));

	D3D11_SHADER_RESOURCE_VIEW_DESC  SRViewDesc;
	ZeroMemory(&SRViewDesc,sizeof(SRViewDesc));
	// Setup the description of the shader resource view.
	SRViewDesc.Format = RTTextureDesc.Format;
	SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRViewDesc.Texture2D.MostDetailedMip = 0;
	SRViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pRenderTargetTexture, &SRViewDesc, &g_pTextureRV[2]));

	//Create DepthStencil buffer and view
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth,sizeof(descDepth));
	descDepth.Width = ProjectRTwidth;
	descDepth.Height = ProjectRTheight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format =DXUTGetDeviceSettings().d3d11.AutoDepthStencilFormat;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = pd3dDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencil );


	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV,sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	// Create the depth stencil view
	V_RETURN( pd3dDevice->CreateDepthStencilView( pDepthStencil, // Depth stencil texture
		&descDSV, // Depth stencil desc
		&pDSVn ));  // [out] Depth stencil view

	D3D11_RASTERIZER_DESC descRaster;
	// Setup the raster description which will determine how and what polygons will be drawn.
	descRaster.AntialiasedLineEnable = false;
	descRaster.CullMode = D3D11_CULL_BACK;
	descRaster.DepthBias = 0;
	descRaster.DepthBiasClamp = 0.0f;
	descRaster.DepthClipEnable = true;
	descRaster.FillMode = D3D11_FILL_WIREFRAME;
	descRaster.FrontCounterClockwise = false;
	descRaster.MultisampleEnable = false;
	descRaster.ScissorEnable = false;
	descRaster.SlopeScaledDepthBias = 0.0f;
	ID3D11RasterizerState* rasterState;

	V_RETURN(pd3dDevice->CreateRasterizerState(&descRaster,&rasterState));
	//pd3dImmediateContext->RSSetState(rasterState);
	SAFE_RELEASE(rasterState);

	//Set Viewport!!!
	// Setup the viewport for rendering.
	g_mRTViewport.Width = ProjectRTwidth;
	g_mRTViewport.Height = ProjectRTheight;
	g_mRTViewport.MinDepth = 0.0f;
	g_mRTViewport.MaxDepth = 1.0f;
	g_mRTViewport.TopLeftX = 0;
	g_mRTViewport.TopLeftY = 0;

	return hr;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										 const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
	g_VCamera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 4000.0f );
	g_VCamera.SetWindow(pBackBufferSurfaceDesc->Width,pBackBufferSurfaceDesc->Height );
	g_VCamera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
	g_TextureProjector.SetWindow(pBackBufferSurfaceDesc->Width,pBackBufferSurfaceDesc->Height );
	g_TextureProjector.SetButtonMasks( MOUSE_MIDDLE_BUTTON, NULL, MOUSE_RIGHT_BUTTON );
	g_mViewport.Width = pBackBufferSurfaceDesc->Width ;
	g_mViewport.Height = pBackBufferSurfaceDesc->Height;
	g_mViewport.MinDepth = 0.0f;
	g_mViewport.MaxDepth = 1.0f;
	g_mViewport.TopLeftX = 0;
	g_mViewport.TopLeftY = 0;

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	// Update the camera's position based on user input 
	g_VCamera.FrameMove( fElapsedTime );
	g_TextureProjector.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
								 double fTime, float fElapsedTime, void* pUserContext )
{
	// Clear render target and the depth stencil 
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float ClearRTColor[4] = { 0.0f, 1.0f, 0.0f, 0.0f };


	D3DXMATRIX mWorld;
	D3DXMATRIX mView,mPTView;
	D3DXMATRIX mProj,mPTProj;
	D3DXVECTOR3		projectorPos;
	D3DXVECTOR3		projectorDir;
	D3DXVECTOR3		Pos,Dir;


	// Update our time
	static float t = 0.0f;

	static DWORD dwTimeStart = 0;
	DWORD dwTimeCur = GetTickCount();
	if( dwTimeStart == 0 )
		dwTimeStart = dwTimeCur;
	t = ( dwTimeCur - dwTimeStart ) / 1000.0f;


	// Rotate cube around the origin
	g_World=XMMatrixRotationY (t *0.6);

	// Modify the color
	g_vMeshColor.x = ( sinf( t * 1.0f ) + 1.0f ) * 0.5f;
	g_vMeshColor.y = ( cosf( t * 3.0f ) + 1.0f ) * 0.5f;
	g_vMeshColor.z = ( sinf( t * 5.0f ) + 1.0f ) * 0.5f;
	CBChangesEveryFrame cb;
	// Get the projection & view matrix from the camera class
	mProj = *g_TextureProjector.GetProjMatrix();
	mView = *g_TextureProjector.GetViewMatrix();
	mPTProj = *g_TextureProjector.GetProjMatrix();
	mPTView = *g_TextureProjector.GetViewMatrix();
	projectorPos=*g_TextureProjector.GetEyePt();
	Pos=*g_TextureProjector.GetLookAtPt();
	projectorDir=*g_TextureProjector.GetEyePt();

	cb.mProjection=XMMatrixTranspose((XMMATRIX)mProj);
	cb.mView=XMMatrixTranspose((XMMATRIX)mView);
	cb.mWorld=XMMatrixTranspose((XMMATRIX)g_World);
	cb.mWorld=XMMatrixIdentity();
	cb.mPTView=XMMatrixTranspose((XMMATRIX)mPTView);
	cb.mPTProj=XMMatrixTranspose((XMMATRIX)mPTProj);
	cb.projectorPos=projectorPos;
	cb.projectorDir=projectorDir;
	cb.vMeshColor = g_vMeshColor;
	cb.timer=t;


	ID3D11ShaderResourceView* nullsrv=NULL;
	// Create the viewport.
	pd3dImmediateContext->RSSetViewports(1, &g_mRTViewport);
	pd3dImmediateContext->PSSetShaderResources(2,1,&nullsrv);
	pd3dImmediateContext->OMSetRenderTargets(1,&g_pRTTextureRTV,pDSVn);
	pd3dImmediateContext->ClearRenderTargetView(g_pRTTextureRTV,ClearRTColor);
	pd3dImmediateContext->ClearDepthStencilView( pDSVn, D3D10_CLEAR_DEPTH, 1.0f, 0);
	// Set vertex buffer
	UINT stride = sizeof( MyVertex );
	UINT offset = 0;
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );
	pd3dImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	pd3dImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
	pd3dImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->GSSetShader(g_pGeometryShader,NULL,0);
	pd3dImmediateContext->GSSetConstantBuffers(0,1,&g_pCBChangesEveryFrame);
	pd3dImmediateContext->PSSetShader( g_pRTPixelShader, NULL, 0 );
	pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
	pd3dImmediateContext->DrawIndexed( numIdex, 0, 0 );

	cb.mWorld=XMMatrixIdentity();
	pd3dImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
	pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBufferP, &stride, &offset );
	pd3dImmediateContext->IASetIndexBuffer( g_pIndexBufferP, DXGI_FORMAT_R16_UINT, 0 );
	pd3dImmediateContext->DrawIndexed( 6, 0, 0 );

	mProj = *g_VCamera.GetProjMatrix();
	mView = *g_VCamera.GetViewMatrix();
	cb.mWorld=XMMatrixTranspose((XMMATRIX)g_World);
	cb.mProjection=XMMatrixTranspose((XMMATRIX)mProj);
	cb.mView=XMMatrixTranspose((XMMATRIX)mView);
	cb.mWorld=XMMatrixIdentity();

	pd3dImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->RSSetViewports(1, &g_mViewport);
	pd3dImmediateContext->OMSetRenderTargets(1,&pRTV,pDSV);
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );
	pd3dImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	pd3dImmediateContext->ClearRenderTargetView(pRTV,ClearColor);
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);
	pd3dImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
	pd3dImmediateContext->PSSetShaderResources( 0, 3, g_pTextureRV );
	pd3dImmediateContext->DrawIndexed( numIdex, 0, 0 );

	cb.mWorld=XMMatrixIdentity();
	pd3dImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
	pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBChangesEveryFrame );
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBufferP, &stride, &offset );
	pd3dImmediateContext->IASetIndexBuffer( g_pIndexBufferP, DXGI_FORMAT_R16_UINT, 0 );
	pd3dImmediateContext->DrawIndexed( 6, 0, 0 );


}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	DXUTGetGlobalResourceCache().OnDestroyDevice();

	if( g_pSamplerLinear ) g_pSamplerLinear->Release();
	for(int i=0;i<3;i++)
	{
		if( g_pTextureRV[i] ) g_pTextureRV[i]->Release();
		if( g_pTextureRVforPlane[i] ) g_pTextureRVforPlane[i]->Release();
	}
	if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
	if( g_pVertexBuffer ) g_pVertexBuffer->Release();
	if( g_pIndexBuffer ) g_pIndexBuffer->Release();
	if( g_pVertexBufferP ) g_pVertexBufferP->Release();
	if( g_pIndexBufferP ) g_pIndexBufferP->Release();
	if( g_pVertexLayout ) g_pVertexLayout->Release();
	if( g_pVertexShader ) g_pVertexShader->Release();
	if( g_pPixelShader ) g_pPixelShader->Release();
	if( g_pGeometryShader ) g_pGeometryShader->Release();
	if( g_pRTPixelShader ) g_pRTPixelShader->Release();
	if( g_pRTTextureRTV ) g_pRTTextureRTV->Release();
	if( g_pRenderTargetTexture ) g_pRenderTargetTexture->Release();
	if( pDepthStencil ) pDepthStencil->Release();
	if( pDSVn ) pDSVn->Release();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
						 bool* pbNoFurtherProcessing, void* pUserContext )
{
	g_VCamera.HandleMessages( hWnd, uMsg, wParam, lParam );
	g_TextureProjector.HandleMessages( hWnd, uMsg, wParam, lParam );
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
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

void initial()
{
	FLOAT fObjectRadius = 5000.0f;
	// Setup the camera's view parameters
	D3DXVECTOR3 vecEye( 0.0f, 0.0f, 10.0f );
	D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
	g_VCamera.SetViewParams( &vecEye, &vecAt );
	g_VCamera.SetRadius( fObjectRadius * 0.0030f, fObjectRadius * 0.0005f, fObjectRadius * 10.0f );

	D3DXVECTOR3 vecPEye( 0.0f, 0.0f, 4.0f );
	D3DXVECTOR3 vecPAt ( 0.0f, 0.0f, -0.0f );
	g_TextureProjector.SetViewParams( &vecPEye, &vecPAt );
	g_TextureProjector.SetRadius( fObjectRadius * 0.0030f, fObjectRadius * 0.0005f, fObjectRadius * 10.0f );
	g_TextureProjector.SetProjParams( D3DX_PI / 3,ProjectRTwidth/(FLOAT)ProjectRTheight, 0.1f, 4000.0f );
	g_TextureProjector.SetButtonMasks( MOUSE_MIDDLE_BUTTON, NULL, MOUSE_RIGHT_BUTTON );
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

	// DXUT will create and use the best device (either D3D9 or D3D11) 
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackKeyboard( OnKeyboard );
	DXUTSetCallbackMouse( OnMouse );
	DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
	DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );


	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
	DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
	DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
	DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
	DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
	DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	// Perform any application-level initialization here

	DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
	DXUTCreateWindow( L"DX11ProjectiveTexturing" );

	initial();
	// Only require 10-level hardware
	DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 500, 500 );
	DXUTMainLoop(); // Enter into the DXUT ren  der loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}


