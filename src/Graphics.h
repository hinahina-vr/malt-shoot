#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
    XMFLOAT2 texCoord;
};

class Graphics {
public:
    Graphics();
    ~Graphics();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    // Draw functions
    void DrawSprite(float x, float y, float width, float height, XMFLOAT4 color);
    void DrawTexturedSprite(float x, float y, float width, float height, 
                            ID3D11ShaderResourceView* texture, XMFLOAT4 tint = XMFLOAT4(1,1,1,1));
    void DrawCircle(float x, float y, float radius, XMFLOAT4 color);
    void DrawGlowCircle(float x, float y, float radius, XMFLOAT4 color, int layers = 3);
    void DrawGradientCircle(float x, float y, float radius, XMFLOAT4 innerColor, XMFLOAT4 outerColor);
    void DrawCircleArc(float x, float y, float radius, float thickness, float startAngle, float endAngle, XMFLOAT4 color);
    
    // Blend mode
    void SetAdditiveBlend(bool additive);
    void SetTextureMode(bool useTexture);

    // Device access
    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain.Get(); }

private:
    bool CreateDevice(HWND hWnd, int width, int height);
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateSamplerState();

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11PixelShader> m_texturedPixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    ComPtr<ID3D11BlendState> m_blendState;
    ComPtr<ID3D11BlendState> m_additiveBlendState;
    ComPtr<ID3D11SamplerState> m_samplerState;

    int m_width;
    int m_height;

    struct ConstantBuffer {
        XMMATRIX projection;
    };
};

