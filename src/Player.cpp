#include "Player.h"
#include "Graphics.h"
#include "Input.h"
#include "BulletManager.h"
#include "TextureLoader.h"

using namespace DirectX;

Player::Player()
    : m_position{ 0.0f, 0.0f }
    , m_speed(400.0f)
    , m_slowSpeed(150.0f)
    , m_size(48.0f)
    , m_hitboxRadius(3.0f)
    , m_shootCooldown(0.08f)
    , m_currentCooldown(0.0f)
    , m_isSlow(false)
    , m_bulletManager(nullptr)
{
}

Player::~Player() {
}

void Player::Initialize(Graphics* graphics, BulletManager* bulletManager) {
    m_bulletManager = bulletManager;
    
    // Load player texture
    TextureLoader loader;
    loader.Initialize(graphics->GetDevice());
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring texturePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\sprites\\player.png";
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (loader.LoadTexture(texturePath, &srv)) {
        m_texture.Attach(srv);
    }
}

void Player::Update(Input* input, float deltaTime, int screenWidth, int screenHeight) {
    // 低速移動モード
    m_isSlow = input->IsKeyDown(VK_SHIFT);
    float currentSpeed = m_isSlow ? m_slowSpeed : m_speed;

    // 移動
    float dx = 0.0f, dy = 0.0f;
    if (input->IsKeyDown(VK_LEFT) || input->IsKeyDown('A')) dx -= 1.0f;
    if (input->IsKeyDown(VK_RIGHT) || input->IsKeyDown('D')) dx += 1.0f;
    if (input->IsKeyDown(VK_UP) || input->IsKeyDown('W')) dy -= 1.0f;
    if (input->IsKeyDown(VK_DOWN) || input->IsKeyDown('S')) dy += 1.0f;

    // 斜め移動時の速度正規化
    if (dx != 0.0f && dy != 0.0f) {
        float len = sqrtf(dx * dx + dy * dy);
        dx /= len;
        dy /= len;
    }

    m_position.x += dx * currentSpeed * deltaTime;
    m_position.y += dy * currentSpeed * deltaTime;

    // 画面内に制限
    float halfSize = m_size / 2.0f;
    if (m_position.x < halfSize) m_position.x = halfSize;
    if (m_position.x > screenWidth - halfSize) m_position.x = static_cast<float>(screenWidth) - halfSize;
    if (m_position.y < halfSize) m_position.y = halfSize;
    if (m_position.y > screenHeight - halfSize) m_position.y = static_cast<float>(screenHeight) - halfSize;

    // 射撃
    m_currentCooldown -= deltaTime;
    if (input->IsKeyDown('Z') && m_currentCooldown <= 0.0f) {
        // 自機弾を発射（弾速2倍！）- 中央から1発
        if (m_bulletManager) {
            m_bulletManager->SpawnPlayerBullet(m_position.x, m_position.y - 20.0f, 0.0f, -1600.0f);
            
            // パワーアップでホーミングミサイル（パワー2以上で発射）
            if (m_power >= 2) {
                m_bulletManager->SpawnHomingMissile(m_position.x - 25.0f, m_position.y, -50.0f, -400.0f);
                m_bulletManager->SpawnHomingMissile(m_position.x + 25.0f, m_position.y, 50.0f, -400.0f);
            }
        }
        m_currentCooldown = m_shootCooldown;
    }
}

void Player::Render(Graphics* graphics) {
    float halfSize = m_size / 2.0f;
    
    // Outer glow
    graphics->DrawGlowCircle(m_position.x, m_position.y, m_size * 0.8f,
        XMFLOAT4(1.0f, 0.6f, 0.7f, 0.3f), 3);
    
    // Draw player texture if available
    if (m_texture) {
        graphics->DrawTexturedSprite(
            m_position.x - halfSize, m_position.y - halfSize,
            m_size, m_size,
            m_texture.Get(), XMFLOAT4(1, 1, 1, 1));
    } else {
        // Fallback: simple shape
        graphics->DrawCircle(m_position.x, m_position.y, halfSize * 0.8f,
            XMFLOAT4(0.95f, 0.9f, 0.8f, 1.0f));
    }

    // Hitbox display when slow moving
    if (m_isSlow) {
        graphics->DrawGlowCircle(m_position.x, m_position.y, m_hitboxRadius * 3.0f,
            XMFLOAT4(1.0f, 1.0f, 1.0f, 0.9f), 2);
        graphics->DrawCircle(m_position.x, m_position.y, m_hitboxRadius,
            XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f));
    }
}

void Player::SetPosition(float x, float y) {
    m_position.x = x;
    m_position.y = y;
}
