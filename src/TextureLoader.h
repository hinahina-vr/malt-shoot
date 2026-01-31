#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <string>

#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

class TextureLoader {
public:
    TextureLoader() : m_device(nullptr) {}
    ~TextureLoader() {}

    void Initialize(ID3D11Device* device) {
        m_device = device;
    }

    // Load texture from file using WIC
    bool LoadTexture(const std::wstring& filename, 
                     ID3D11ShaderResourceView** textureView,
                     int* width = nullptr, int* height = nullptr) {
        if (!m_device) return false;

        // Create WIC factory
        ComPtr<IWICImagingFactory> wicFactory;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        );
        if (FAILED(hr)) return false;

        // Create decoder
        ComPtr<IWICBitmapDecoder> decoder;
        hr = wicFactory->CreateDecoderFromFilename(
            filename.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder
        );
        if (FAILED(hr)) return false;

        // Get frame
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) return false;

        // Get size
        UINT texWidth, texHeight;
        hr = frame->GetSize(&texWidth, &texHeight);
        if (FAILED(hr)) return false;

        if (width) *width = static_cast<int>(texWidth);
        if (height) *height = static_cast<int>(texHeight);

        // Convert to RGBA
        ComPtr<IWICFormatConverter> converter;
        hr = wicFactory->CreateFormatConverter(&converter);
        if (FAILED(hr)) return false;

        hr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
        );
        if (FAILED(hr)) return false;

        // Copy pixels
        UINT stride = texWidth * 4;
        UINT bufferSize = stride * texHeight;
        std::vector<BYTE> pixels(bufferSize);
        hr = converter->CopyPixels(nullptr, stride, bufferSize, pixels.data());
        if (FAILED(hr)) return false;

        // Create texture
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = texWidth;
        texDesc.Height = texHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = stride;

        ComPtr<ID3D11Texture2D> texture;
        hr = m_device->CreateTexture2D(&texDesc, &initData, &texture);
        if (FAILED(hr)) return false;

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, textureView);
        return SUCCEEDED(hr);
    }

private:
    ID3D11Device* m_device;
};
