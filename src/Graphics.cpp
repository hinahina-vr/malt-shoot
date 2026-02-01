#include "Graphics.h"
#include <vector>

Graphics::Graphics()
    : m_width(0)
    , m_height(0)
{
}

Graphics::~Graphics() {
    Shutdown();
}

bool Graphics::Initialize(HWND hWnd, int width, int height) {
    m_width = width;
    m_height = height;

    if (!CreateDevice(hWnd, width, height)) {
        return false;
    }

    if (!CreateShaders()) {
        return false;
    }

    if (!CreateBuffers()) {
        return false;
    }

    return true;
}

void Graphics::Shutdown() {
    if (m_context) {
        m_context->ClearState();
    }
}

bool Graphics::CreateDevice(HWND hWnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, nullptr, 0, D3D11_SDK_VERSION,
        &scd, &m_swapChain, &m_device, &featureLevel, &m_context
    );

    if (FAILED(hr)) {
        return false;
    }

    // Render target view
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) {
        return false;
    }

    // Viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);

    // Alpha blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr)) {
        return false;
    }

    // Additive blend state (for glow effects)
    D3D11_BLEND_DESC additiveDesc = {};
    additiveDesc.RenderTarget[0].BlendEnable = TRUE;
    additiveDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    additiveDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    additiveDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    additiveDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    additiveDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    additiveDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    additiveDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&additiveDesc, &m_additiveBlendState);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool Graphics::CreateShaders() {
    // Vertex shader
    const char* vsSource = R"(
        cbuffer ConstantBuffer : register(b0) {
            matrix projection;
        };
        struct VS_INPUT {
            float3 pos : POSITION;
            float4 color : COLOR;
            float2 tex : TEXCOORD;
        };
        struct VS_OUTPUT {
            float4 pos : SV_POSITION;
            float4 color : COLOR;
            float2 tex : TEXCOORD;
        };
        VS_OUTPUT main(VS_INPUT input) {
            VS_OUTPUT output;
            output.pos = mul(float4(input.pos, 1.0f), projection);
            output.color = input.color;
            output.tex = input.tex;
            return output;
        }
    )";

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), "VS", nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, &m_vertexShader);
    if (FAILED(hr)) {
        return false;
    }

    // Pixel shader
    const char* psSource = R"(
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 color : COLOR;
            float2 tex : TEXCOORD;
        };
        float4 main(PS_INPUT input) : SV_TARGET {
            return input.color;
        }
    )";

    ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(psSource, strlen(psSource), "PS", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, &m_pixelShader);
    if (FAILED(hr)) {
        return false;
    }

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), &m_inputLayout);
    if (FAILED(hr)) {
        return false;
    }

    // Textured pixel shader
    const char* psTexturedSource = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float4 color : COLOR;
            float2 tex : TEXCOORD;
        };
        float4 main(PS_INPUT input) : SV_TARGET {
            float4 texColor = tex.Sample(samp, input.tex);
            return texColor * input.color;
        }
    )";

    ComPtr<ID3DBlob> psTexturedBlob;
    hr = D3DCompile(psTexturedSource, strlen(psTexturedSource), "PS_Textured", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &psTexturedBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        m_device->CreatePixelShader(psTexturedBlob->GetBufferPointer(), psTexturedBlob->GetBufferSize(),
            nullptr, &m_texturedPixelShader);
    }

    // Create sampler state
    CreateSamplerState();

    return true;
}

bool Graphics::CreateBuffers() {
    // Dynamic vertex buffer
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(Vertex) * 4096; // Increased for glow effects
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = m_device->CreateBuffer(&vbDesc, nullptr, &m_vertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // Orthographic projection
    ConstantBuffer cb;
    cb.projection = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(m_width),
        static_cast<float>(m_height), 0.0f, 0.0f, 1.0f);
    cb.projection = XMMatrixTranspose(cb.projection);
    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    return true;
}

void Graphics::BeginFrame() {
    float clearColor[] = { 0.0f, 0.0f, 0.02f, 1.0f }; // Very dark background
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    
    float blendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);
}

void Graphics::EndFrame() {
    m_swapChain->Present(1, 0);
}

void Graphics::SetAdditiveBlend(bool additive) {
    float blendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (additive) {
        m_context->OMSetBlendState(m_additiveBlendState.Get(), blendFactor, 0xffffffff);
    } else {
        m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);
    }
}

void Graphics::DrawSprite(float x, float y, float width, float height, XMFLOAT4 color) {
    Vertex vertices[] = {
        { XMFLOAT3(x, y, 0.0f), color, XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(x + width, y, 0.0f), color, XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(x, y + height, 0.0f), color, XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(x + width, y, 0.0f), color, XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(x + width, y + height, 0.0f), color, XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(x, y + height, 0.0f), color, XMFLOAT2(0.0f, 1.0f) },
    };

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, vertices, sizeof(vertices));
    m_context->Unmap(m_vertexBuffer.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);
}

void Graphics::DrawCircle(float x, float y, float radius, XMFLOAT4 color) {
    const int segments = 24;
    std::vector<Vertex> vertices;
    vertices.reserve(segments * 3);

    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.14159f * i) / segments;
        float angle2 = (2.0f * 3.14159f * (i + 1)) / segments;

        vertices.push_back({ XMFLOAT3(x, y, 0.0f), color, XMFLOAT2(0.5f, 0.5f) });
        vertices.push_back({ XMFLOAT3(x + radius * cosf(angle1), y + radius * sinf(angle1), 0.0f), color, XMFLOAT2(0.5f + 0.5f * cosf(angle1), 0.5f + 0.5f * sinf(angle1)) });
        vertices.push_back({ XMFLOAT3(x + radius * cosf(angle2), y + radius * sinf(angle2), 0.0f), color, XMFLOAT2(0.5f + 0.5f * cosf(angle2), 0.5f + 0.5f * sinf(angle2)) });
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
    m_context->Unmap(m_vertexBuffer.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(static_cast<UINT>(vertices.size()), 0);
}

void Graphics::DrawGlowCircle(float x, float y, float radius, XMFLOAT4 color, int layers) {
    // Enhanced glow effect with HDR-like bloom
    SetAdditiveBlend(true);
    
    // Outer glow layers (soft, diffuse light)
    for (int i = layers + 2; i >= 1; i--) {
        float layerRadius = radius * (1.0f + i * 0.6f);
        float alpha = color.w * 0.15f / (i * 0.8f);
        XMFLOAT4 glowColor = { color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, alpha };
        DrawGradientCircle(x, y, layerRadius, glowColor, XMFLOAT4(0, 0, 0, 0));
    }
    
    // Inner bright glow (HDR-like intensity)
    DrawGradientCircle(x, y, radius * 1.3f, 
        XMFLOAT4(color.x * 0.9f, color.y * 0.9f, color.z * 0.9f, color.w * 0.4f),
        XMFLOAT4(0, 0, 0, 0));
    
    SetAdditiveBlend(false);
    
    // Bright solid core with white center
    XMFLOAT4 coreColor = { 
        fminf(color.x + 0.3f, 1.0f), 
        fminf(color.y + 0.3f, 1.0f), 
        fminf(color.z + 0.3f, 1.0f), 
        color.w 
    };
    DrawGradientCircle(x, y, radius, coreColor, 
        XMFLOAT4(color.x * 0.6f, color.y * 0.6f, color.z * 0.6f, color.w));
}

void Graphics::DrawGradientCircle(float x, float y, float radius, XMFLOAT4 innerColor, XMFLOAT4 outerColor) {
    const int segments = 24;
    std::vector<Vertex> vertices;
    vertices.reserve(segments * 3);

    for (int i = 0; i < segments; i++) {
        float angle1 = (2.0f * 3.14159f * i) / segments;
        float angle2 = (2.0f * 3.14159f * (i + 1)) / segments;

        // Center with inner color
        vertices.push_back({ XMFLOAT3(x, y, 0.0f), innerColor, XMFLOAT2(0.5f, 0.5f) });
        // Edge with outer color
        vertices.push_back({ XMFLOAT3(x + radius * cosf(angle1), y + radius * sinf(angle1), 0.0f), outerColor, XMFLOAT2(0.5f + 0.5f * cosf(angle1), 0.5f + 0.5f * sinf(angle1)) });
        vertices.push_back({ XMFLOAT3(x + radius * cosf(angle2), y + radius * sinf(angle2), 0.0f), outerColor, XMFLOAT2(0.5f + 0.5f * cosf(angle2), 0.5f + 0.5f * sinf(angle2)) });
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
    m_context->Unmap(m_vertexBuffer.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(static_cast<UINT>(vertices.size()), 0);
}

void Graphics::DrawTexturedSprite(float x, float y, float width, float height,
                                   ID3D11ShaderResourceView* texture, XMFLOAT4 tint) {
    if (!texture) {
        DrawSprite(x, y, width, height, tint);
        return;
    }

    // Enable alpha blending
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);

    // Switch to textured shader
    m_context->PSSetShader(m_texturedPixelShader.Get(), nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &texture);
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // Draw quad with texture coordinates
    Vertex vertices[] = {
        { XMFLOAT3(x, y, 0.0f), tint, XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(x + width, y, 0.0f), tint, XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(x, y + height, 0.0f), tint, XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(x + width, y, 0.0f), tint, XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(x + width, y + height, 0.0f), tint, XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(x, y + height, 0.0f), tint, XMFLOAT2(0.0f, 1.0f) },
    };

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, vertices, sizeof(vertices));
    m_context->Unmap(m_vertexBuffer.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);

    // Switch back to non-textured shader
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullSRV);
}

void Graphics::SetTextureMode(bool useTexture) {
    if (useTexture) {
        m_context->PSSetShader(m_texturedPixelShader.Get(), nullptr, 0);
    } else {
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    }
}

bool Graphics::CreateSamplerState() {
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerState);
    return SUCCEEDED(hr);
}

// 円弧描画（HP円グラフ用）- 滑らかな円
void Graphics::DrawCircleArc(float x, float y, float radius, float thickness, 
                             float startAngle, float endAngle, XMFLOAT4 color) {
    const int segments = 64;  // より滑らかに
    float angleRange = endAngle - startAngle;
    
    // 各セグメントを小さな円として描画（滑らかに見える）
    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        float angle = startAngle + angleRange * t;
        float arcX = x + cosf(angle) * (radius - thickness / 2);
        float arcY = y + sinf(angle) * (radius - thickness / 2);
        
        // 小さな円として描画
        DrawGlowCircle(arcX, arcY, thickness / 2, color, 1);
    }
}
