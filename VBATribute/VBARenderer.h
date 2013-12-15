#pragma once

#include "Direct3DBase.h"

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct VertexPositionTexture
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texture;
};

struct VertexPositionColor
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
};

enum PhoneOrientation
{	
	orientLandscape=1,
	orientPortrait,
	orientLandscapeFlipped,
	orientPortraitFlipped,
};

ref class VBARenderer sealed : public Direct3DBase
{
public:
	VBARenderer();

	// Direct3DBase methods.
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	virtual void Render() override;
	
	// Method for updating time-dependent objects.
	void Update(float timeTotal, float timeDelta);

	void setOrientation( int orientation );
private:
	void createTexture( int width, int height );
	void clearTexture( ID3D11Texture2D * texture );
	void createPrimitive( float textureWidth, float textureHeight );
	void destroyTexture();

	bool m_loadingComplete;
	bool m_createTextureReady;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampleState;

	ID3D11Texture2D * tempImage;
	ID3D11Texture2D * emulatedImage[2];
	ID3D11ShaderResourceView * emulatedShaderResourceView[2];

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tempImagePtr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> emulatedImage0Ptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> emulatedImage1Ptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emulatedShaderResourceView0Ptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emulatedShaderResourceView1Ptr;

	unsigned char mbCurrentTexture;
	ModelViewProjectionConstantBuffer m_constantBufferData;
	int m_orientation;
};