#include "Background3D.h"
#include "Graphics.h"
#include <cmath>
#include <cstdlib>

constexpr float PI = 3.14159265358979f;

Background3D::Background3D()
    : m_time(0.0f)
    , m_scrollSpeed(100.0f)
    , m_patternType(0)
    , m_screenWidth(1920)
    , m_screenHeight(1080)
{
}

Background3D::~Background3D() {
}

void Background3D::Initialize(int starCount) {
    m_stars.resize(starCount);
    
    for (auto& star : m_stars) {
        // Random position across entire screen
        star.position.x = static_cast<float>(rand() % m_screenWidth);
        star.position.y = static_cast<float>(rand() % m_screenHeight);
        star.position.z = static_cast<float>(rand() % 100) / 100.0f;
        
        // Speed based on depth
        star.speed = 50.0f + star.position.z * 150.0f;
        
        // Size based on depth
        star.size = 1.0f + star.position.z * 3.0f;
        
        // Whisky-themed colors (amber, gold, brown)
        int colorType = rand() % 3;
        float brightness = 0.3f + star.position.z * 0.7f;
        if (colorType == 0) {
            // Amber
            star.color = XMFLOAT4(brightness, brightness * 0.6f, brightness * 0.2f, 0.6f + star.position.z * 0.4f);
        } else if (colorType == 1) {
            // Gold
            star.color = XMFLOAT4(brightness, brightness * 0.8f, brightness * 0.3f, 0.5f + star.position.z * 0.5f);
        } else {
            // Blue-white (classic star)
            star.color = XMFLOAT4(brightness * 0.8f, brightness * 0.9f, brightness, 0.4f + star.position.z * 0.6f);
        }
    }
}

void Background3D::Update(float deltaTime) {
    m_time += deltaTime;
    
    for (auto& star : m_stars) {
        // Scroll down
        star.position.y += star.speed * deltaTime;
        
        // Wrap around
        if (star.position.y > static_cast<float>(m_screenHeight) + 20.0f) {
            star.position.y = -20.0f;
            star.position.x = static_cast<float>(rand() % m_screenWidth);
            star.position.z = static_cast<float>(rand() % 100) / 100.0f;
            star.speed = 50.0f + star.position.z * 150.0f;
            star.size = 1.0f + star.position.z * 3.0f;
        }
        
        // Subtle wobble
        float wobble = sinf(m_time * 2.0f + star.position.y * 0.01f) * 0.3f;
        star.position.x += wobble * deltaTime * 10.0f;
    }
}

void Background3D::Render(Graphics* graphics) {
    float centerX = static_cast<float>(m_screenWidth) / 2.0f;
    float centerY = static_cast<float>(m_screenHeight) / 2.0f;
    
    // Render stars with parallax effect
    for (const auto& star : m_stars) {
        float x = star.position.x;
        float y = star.position.y;
        
        // 3D perspective convergence
        float perspective = 0.3f + star.position.z * 0.7f;
        x = centerX + (x - centerX) * perspective;
        y = centerY + (y - centerY) * perspective;
        
        // Draw star with glow for larger ones
        if (star.size > 2.0f) {
            graphics->DrawGlowCircle(x, y, star.size, star.color, 2);
        } else {
            graphics->DrawCircle(x, y, star.size, star.color);
        }
    }
}

void Background3D::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}
