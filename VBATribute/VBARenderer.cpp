#include "pch.h"
#include "VBARenderer.h"
#include "vba\VBA.h"
#include "vba\Display.h"
#include "gba\GBA.h"
#include "gba\Globals.h"
#include "System.h"
#include "gb\gbGlobals.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

VBARenderer::VBARenderer() :
	m_loadingComplete(false),
	m_createTextureReady(false),
	mbCurrentTexture(0),
	m_orientation(orientPortrait)
{
	tempImage = 0;
	emulatedImage[0] = 0;
	emulatedImage[1] = 0;

}

void VBARenderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("textureVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("texturePixelShader.cso");

	auto createVSTask = loadVSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreateVertexShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_vertexShader
				)
			);

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				fileData->Data,
				fileData->Length,
				&m_inputLayout
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);

	});

	auto createPSTask = loadPSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreatePixelShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_pixelShader
				)
			);




	});

	auto createSampleState = (createPSTask && createVSTask).then([this] () {

		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory( &samplerDesc, sizeof(samplerDesc) );
		// Create a texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		DX::ThrowIfFailed(m_d3dDevice->CreateSamplerState(&samplerDesc, &m_sampleState));

	});

	createSampleState.then([this] () {
		m_loadingComplete = true;
	});

	/* initialize vba */
	theVBA.initForD3D();

}

void VBARenderer::createPrimitive( float textureWidth, float textureHeight )
{
	float ratio = textureHeight / textureWidth;

	int left, top, right, bottom;
	left = 0;
	top = 0;
	right = (int)m_windowBounds.Width;
	bottom = (int)m_windowBounds.Width * ratio;

	VertexPositionTexture vertices[] = 
	{
		{XMFLOAT3((float)left - 0.5f, (float)bottom - 0.5f, 0.0f), XMFLOAT2(0.0f, 1.0)},
		{XMFLOAT3((float)left - 0.5f, (float)top - 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
		{XMFLOAT3((float)right - 0.5f, (float)bottom - 0.5f, 0.0f), XMFLOAT2(1.0, 1.0)},
		{XMFLOAT3((float)right - 0.5f, (float)top - 0.5f, 0.0f), XMFLOAT2(1.0, 0.0f)},
	};

	D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
	vertexBufferData.pSysMem = vertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER);
	vertexBufferDesc.ByteWidth = sizeof(VertexPositionTexture) * _countof(vertices);
	DX::ThrowIfFailed( m_d3dDevice->CreateBuffer( &vertexBufferDesc, &vertexBufferData, &m_vertexBuffer ) );



}

void VBARenderer::createTexture( int width, int height )
{
	if ( width == 0 || height == 0 )
		return;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture1D.MostDetailedMip = desc.MipLevels - 1;

	if ( !tempImage )
	{
		DX::ThrowIfFailed( m_d3dDevice->CreateTexture2D( &desc, NULL, &tempImagePtr ) );
		tempImage = tempImagePtr.Get();
		clearTexture( tempImage );
	}

	if( !emulatedImage[0] ) 
	{
		DX::ThrowIfFailed( m_d3dDevice->CreateTexture2D( &desc, NULL, &emulatedImage0Ptr ) );
		DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView( emulatedImage0Ptr.Get(), &srvDesc, &emulatedShaderResourceView0Ptr ) );
		emulatedShaderResourceView[0] = emulatedShaderResourceView0Ptr.Get();
		emulatedImage[0] = emulatedImage0Ptr.Get();
	}

	if( !emulatedImage[1] && theVBA.d3dMotionBlur ) 
	{
		DX::ThrowIfFailed( m_d3dDevice->CreateTexture2D( &desc, NULL, &emulatedImage1Ptr ) );
		DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView( emulatedImage1Ptr.Get(), &srvDesc, &emulatedShaderResourceView1Ptr ) );
		emulatedShaderResourceView[1] = emulatedShaderResourceView1Ptr.Get();
		emulatedImage[1] = emulatedImage1Ptr.Get();
	}

}

void VBARenderer::clearTexture( ID3D11Texture2D * texture )
{
	D3D11_TEXTURE2D_DESC desc = {0};
	texture->GetDesc( &desc );

	D3D11_MAPPED_SUBRESOURCE mapped = {0};
	DX::ThrowIfFailed( m_d3dContext->Map( texture, 0, D3D11_MAP_WRITE_DISCARD , 0, &mapped ) );
		
	// read the data out of the texture.
	unsigned int rPitch = mapped.RowPitch;
	BYTE *data = ((BYTE *)mapped.pData);

	memset( data, 0x00, mapped.RowPitch * desc.Height );

	//Release the staging texture
	m_d3dContext->Unmap( texture, 0 );
}

void VBARenderer::destroyTexture()
{
/*
	if ( tempImage )
		tempImage->Release();
	if ( emulatedImage[0] )
		emulatedImage[0]->Release();
	if ( emulatedImage[1] )
		emulatedImage[1]->Release();
*/
	tempImage = 0;
	emulatedImage[0] = 0;
	emulatedImage[1] = 0;
}

void VBARenderer::CreateWindowSizeDependentResources()
{
	Direct3DBase::CreateWindowSizeDependentResources();

	float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(
			XMMatrixOrthographicOffCenterLH(
				0,
				m_windowBounds.Width,				
				m_windowBounds.Height,
				0,
				-10,
				10
				)
			)
		);

	createPrimitive( theVBA.sizeX, theVBA.sizeY );
	createTexture( theVBA.sizeX, theVBA.sizeY );

	m_createTextureReady = true;

}

void VBARenderer::setOrientation( int orientation )
{
	m_orientation = orientation;
}

void VBARenderer::Update(float timeTotal, float timeDelta)
{
	(void) timeDelta; // Unused parameter.

	XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixIdentity());
	//XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixTranspose(XMMatrixRotationZ(timeTotal * XM_PIDIV4)));

	float ratio = (float)theVBA.sizeX / (float)theVBA.sizeY;
	float diff = ( m_windowBounds.Height - m_windowBounds.Width * ratio ) / 2.0;
	XMMATRIX tmp = XMMatrixIdentity();
	switch (m_orientation)
	{
	case orientLandscape:
		tmp = XMMatrixMultiply( XMMatrixScaling( ratio, ratio, 1.0 ), XMMatrixRotationZ(XM_PIDIV2) );
		tmp = XMMatrixMultiply( tmp, XMMatrixTranslation( m_windowBounds.Width, diff, 0) );
		XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixTranspose( tmp ));
		break;
	case orientPortrait:
		XMStoreFloat4x4(&m_constantBufferData.world, tmp);
		break;
	case orientLandscapeFlipped:
		tmp = XMMatrixMultiply( XMMatrixScaling( ratio, ratio, 1.0 ), XMMatrixRotationZ(XM_PI+XM_PIDIV2) );
		tmp = XMMatrixMultiply( tmp, XMMatrixTranslation( 0, m_windowBounds.Height-diff, 0) );
		XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixTranspose( tmp ));
		break;
	case orientPortraitFlipped:
		tmp = XMMatrixMultiply( XMMatrixScaling( ratio, ratio, 1.0 ), XMMatrixRotationZ(XM_PI) );
		tmp = XMMatrixMultiply( tmp, XMMatrixTranslation( m_windowBounds.Width, m_windowBounds.Height, 0) );
		XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixTranspose( tmp ));
		break;
	default:
		break;
	}
}

void VBARenderer::Render()
{
	const float midnightBlue[] = { 0.0f, 0.0f, 0.f, 1.000f };
	m_d3dContext->ClearRenderTargetView(
		m_renderTargetView.Get(),
		midnightBlue
		);

	m_d3dContext->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH,
		1.0f,
		0
		);

	// 
	if (!m_loadingComplete || !m_createTextureReady || !emulating )
	{
		return;
	}

	m_d3dContext->OMSetRenderTargets(
		1,
		m_renderTargetView.GetAddressOf(),
		m_depthStencilView.Get()
		);

	m_d3dContext->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

	// update texture
	D3D11_TEXTURE2D_DESC desc = {0};
	tempImage->GetDesc( &desc );

	D3D11_MAPPED_SUBRESOURCE mapped = {0};
	DX::ThrowIfFailed( m_d3dContext->Map( tempImage, 0, D3D11_MAP_WRITE_DISCARD , 0, &mapped ) );
		
	// read the data out of the texture.
	unsigned int rPitch = mapped.RowPitch;
	BYTE *data = ((BYTE *)mapped.pData);

	// copy pix to data	
	u32 pitch = theVBA.sizeX * ( systemColorDepth >> 3 ) + 4;
	switch ( systemColorDepth )
	{
	case 32:
		{
			cpyImg32( data, rPitch, pix + pitch, pitch, theVBA.sizeX, theVBA.sizeY );
		}
		break;
	case 16:
		{
			cpyImg16( data, rPitch, pix + pitch, pitch, theVBA.sizeX, theVBA.sizeY );
		}
		break;
	default:
		break;
	}
	

	//Release the staging texture
	m_d3dContext->Unmap( tempImage, 0 );

	m_d3dContext->CopyResource( emulatedImage[mbCurrentTexture], tempImage );


	unsigned int stride;
	unsigned int offset;
	// Set vertex buffer stride and offset.
	stride = sizeof(VertexPositionTexture); 
	offset = 0;    
	// Set the vertex buffer to active in the input assembler so it can be rendered.
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    // Set the index buffer to active in the input assembler so it can be rendered.
	m_d3dContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );	

	// Now set the constant buffer in the vertex shader with the updated values.
    m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	// Set shader texture resource in the pixel shader.
	m_d3dContext->PSSetShaderResources(0, 1, &emulatedShaderResourceView[mbCurrentTexture] );

	// Set the vertex input layout.
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

    // Set the vertex and pixel shaders that will be used to render this triangle.
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	// Set the sampler state in the pixel shader.
	m_d3dContext->PSSetSamplers(0, 1, m_sampleState.GetAddressOf());

	// Render the triangle.
	m_d3dContext->Draw( 4, 0 );

}