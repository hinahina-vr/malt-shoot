#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

class TextRenderer {
public:
    TextRenderer() : m_initialized(false) {}
    ~TextRenderer() { Shutdown(); }

    bool Initialize(IDXGISwapChain* swapChain) {
        // Create D2D factory
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
        if (FAILED(hr)) return false;

        // Create DirectWrite factory
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
        );
        if (FAILED(hr)) return false;

        // Get DXGI surface from swap chain
        ComPtr<IDXGISurface> dxgiSurface;
        hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));
        if (FAILED(hr)) return false;

        // Create D2D render target
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        hr = m_d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface.Get(), &props, &m_renderTarget);
        if (FAILED(hr)) return false;

        // Create text formats (日本語対応フォント)
        CreateTextFormat(L"Meiryo UI", 14.0f, &m_smallFont);
        CreateTextFormat(L"Meiryo UI", 18.0f, &m_normalFont);
        CreateTextFormat(L"Meiryo UI", 24.0f, &m_largeFont);
        CreateTextFormat(L"Meiryo UI", 54.0f, &m_titleFont);

        // Create brushes
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.8f, 0.3f), &m_goldBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 0.8f, 1.0f), &m_blueBrush);

        m_initialized = true;
        return true;
    }

    void Shutdown() {
        m_whiteBrush.Reset();
        m_goldBrush.Reset();
        m_blueBrush.Reset();
        m_smallFont.Reset();
        m_normalFont.Reset();
        m_largeFont.Reset();
        m_titleFont.Reset();
        m_renderTarget.Reset();
        m_dwriteFactory.Reset();
        m_d2dFactory.Reset();
        m_initialized = false;
    }

    void BeginDraw() {
        if (m_renderTarget) {
            m_renderTarget->BeginDraw();
        }
    }

    void EndDraw() {
        if (m_renderTarget) {
            m_renderTarget->EndDraw();
        }
    }

    void DrawText(const std::wstring& text, float x, float y, float width, float height, 
                  int fontSize = 1, int colorType = 0) {
        if (!m_renderTarget) return;

        IDWriteTextFormat* font = nullptr;
        switch (fontSize) {
            case 0: font = m_smallFont.Get(); break;
            case 1: font = m_normalFont.Get(); break;
            case 2: font = m_largeFont.Get(); break;
            case 3: font = m_titleFont.Get(); break;
            default: font = m_normalFont.Get(); break;
        }

        ID2D1SolidColorBrush* brush = nullptr;
        switch (colorType) {
            case 0: brush = m_whiteBrush.Get(); break;
            case 1: brush = m_goldBrush.Get(); break;
            case 2: brush = m_blueBrush.Get(); break;
            default: brush = m_whiteBrush.Get(); break;
        }

        D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
        m_renderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()),
                                  font, rect, brush);
    }

    void DrawTextWithValue(const std::wstring& label, int value, float x, float y) {
        wchar_t buffer[64];
        swprintf_s(buffer, L"%s: %d", label.c_str(), value);
        DrawText(buffer, x, y, 300, 30, 1, 0);
    }

    void DrawTextWithAlpha(const std::wstring& text, float x, float y, float width, float height,
                           int fontSize = 1, int colorType = 0, float alpha = 1.0f) {
        if (!m_renderTarget) return;

        IDWriteTextFormat* font = nullptr;
        switch (fontSize) {
            case 0: font = m_smallFont.Get(); break;
            case 1: font = m_normalFont.Get(); break;
            case 2: font = m_largeFont.Get(); break;
            case 3: font = m_titleFont.Get(); break;
            default: font = m_normalFont.Get(); break;
        }

        // Create brush with alpha
        D2D1_COLOR_F color;
        switch (colorType) {
            case 0: color = D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha); break;
            case 1: color = D2D1::ColorF(0.9f, 0.8f, 0.3f, alpha); break;
            case 2: color = D2D1::ColorF(0.5f, 0.8f, 1.0f, alpha); break;
            default: color = D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha); break;
        }

        ComPtr<ID2D1SolidColorBrush> alphaBrush;
        m_renderTarget->CreateSolidColorBrush(color, &alphaBrush);

        D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
        m_renderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()),
                                  font, rect, alphaBrush.Get());
    }

private:
    bool CreateTextFormat(const wchar_t* fontName, float size, IDWriteTextFormat** format) {
        HRESULT hr = m_dwriteFactory->CreateTextFormat(
            fontName, nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size, L"ja-JP", format
        );
        if (SUCCEEDED(hr) && *format) {
            (*format)->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            (*format)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        return SUCCEEDED(hr);
    }

    bool m_initialized;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1RenderTarget> m_renderTarget;
    
    ComPtr<IDWriteTextFormat> m_smallFont;
    ComPtr<IDWriteTextFormat> m_normalFont;
    ComPtr<IDWriteTextFormat> m_largeFont;
    ComPtr<IDWriteTextFormat> m_titleFont;
    
    ComPtr<ID2D1SolidColorBrush> m_whiteBrush;
    ComPtr<ID2D1SolidColorBrush> m_goldBrush;
    ComPtr<ID2D1SolidColorBrush> m_blueBrush;
};
