#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Background star/particle
struct BackgroundStar {
    XMFLOAT3 position;
    float speed;
    float size;
    XMFLOAT4 color;
};

class Background3D {
public:
    Background3D();
    ~Background3D();

    void Initialize(int starCount = 200);
    void Update(float deltaTime);
    void Render(class Graphics* graphics);
    void SetScreenSize(int width, int height);

private:
    std::vector<BackgroundStar> m_stars;
    float m_time;
    float m_scrollSpeed;
    int m_patternType;
    int m_screenWidth;
    int m_screenHeight;
};
