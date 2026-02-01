#include "BulletManager.h"
#include "Graphics.h"
#include "TextureLoader.h"
#include <cmath>

using namespace DirectX;

constexpr float PI = 3.14159265358979f;

BulletManager::BulletManager() {
}

BulletManager::~BulletManager() {
}

void BulletManager::Initialize(Graphics* graphics) {
    m_bullets.reserve(MAX_BULLETS);
    
    // 樽テクスチャを読み込み
    TextureLoader loader;
    loader.Initialize(graphics->GetDevice());
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring texturePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\barrel_bullet.png";
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (loader.LoadTexture(texturePath, &srv)) {
        m_barrelTexture.Attach(srv);
    }
}

void BulletManager::Update(float deltaTime, int screenWidth, int screenHeight) {
    for (auto& bullet : m_bullets) {
        if (!bullet.isActive) continue;

        // Angular velocity
        if (bullet.angularVelocity != 0.0f) {
            bullet.angle += bullet.angularVelocity * deltaTime;
            float speed = sqrtf(bullet.velocity.x * bullet.velocity.x + bullet.velocity.y * bullet.velocity.y);
            bullet.velocity.x = cosf(bullet.angle) * speed;
            bullet.velocity.y = sinf(bullet.angle) * speed;
        }

        // Position update
        bullet.position.x += bullet.velocity.x * deltaTime;
        bullet.position.y += bullet.velocity.y * deltaTime;

        // Screen bounds check
        float margin = 50.0f;
        if (bullet.position.x < -margin || bullet.position.x > screenWidth + margin ||
            bullet.position.y < -margin || bullet.position.y > screenHeight + margin) {
            bullet.isActive = false;
        }
    }
}

void BulletManager::Render(Graphics* graphics) {
    static float globalTime = 0.0f;
    globalTime += 0.016f;  // 約60FPSで回転
    
    for (size_t i = 0; i < m_bullets.size(); i++) {
        const auto& bullet = m_bullets[i];
        if (!bullet.isActive) continue;

        if (bullet.isPlayerBullet) {
            // プレイヤー弾：樽！
            if (m_barrelTexture) {
                float size = bullet.radius * 8.0f;  // 樽サイズ（倍増！）
                graphics->DrawTexturedSprite(
                    bullet.position.x - size/2, bullet.position.y - size/2,
                    size, size,
                    m_barrelTexture.Get(), XMFLOAT4(1, 1, 1, 1));
            } else {
                // フォールバック
                graphics->DrawGlowCircle(
                    bullet.position.x, bullet.position.y,
                    bullet.radius, bullet.color, 2);
            }
        } else {
            // Enemy bullets: beautiful glow effect
            graphics->DrawGlowCircle(
                bullet.position.x, bullet.position.y,
                bullet.radius, bullet.color, 3);
        }
    }
}

void BulletManager::Clear() {
    for (auto& bullet : m_bullets) {
        bullet.isActive = false;
    }
}

void BulletManager::SpawnPlayerBullet(float x, float y, float vx, float vy) {
    for (auto& bullet : m_bullets) {
        if (!bullet.isActive) {
            bullet.position = { x, y };
            bullet.velocity = { vx, vy };
            bullet.color = { 0.8f, 1.0f, 1.0f, 1.0f }; // Cyan-white
            bullet.radius = 5.0f;
            bullet.angle = atan2f(vy, vx);
            bullet.angularVelocity = 0.0f;
            bullet.type = BulletType::PlayerShot;
            bullet.isActive = true;
            bullet.isPlayerBullet = true;
            return;
        }
    }

    if (m_bullets.size() < MAX_BULLETS) {
        Bullet bullet;
        bullet.position = { x, y };
        bullet.velocity = { vx, vy };
        bullet.color = { 0.8f, 1.0f, 1.0f, 1.0f };
        bullet.radius = 5.0f;
        bullet.angle = atan2f(vy, vx);
        bullet.type = BulletType::PlayerShot;
        bullet.isActive = true;
        bullet.isPlayerBullet = true;
        m_bullets.push_back(bullet);
    }
}

void BulletManager::SpawnHomingMissile(float x, float y, float vx, float vy) {
    for (auto& bullet : m_bullets) {
        if (!bullet.isActive) {
            bullet.position = { x, y };
            bullet.velocity = { vx, vy };
            bullet.color = { 0.4f, 1.0f, 0.5f, 1.0f }; // 緑色のミサイル
            bullet.radius = 8.0f;  // 大きめ
            bullet.angle = atan2f(vy, vx);
            bullet.angularVelocity = 0.0f;
            bullet.type = BulletType::PlayerShot;
            bullet.isActive = true;
            bullet.isPlayerBullet = true;
            bullet.isHoming = true;  // ホーミングフラグ
            return;
        }
    }

    if (m_bullets.size() < MAX_BULLETS) {
        Bullet bullet;
        bullet.position = { x, y };
        bullet.velocity = { vx, vy };
        bullet.color = { 0.4f, 1.0f, 0.5f, 1.0f };
        bullet.radius = 8.0f;
        bullet.angle = atan2f(vy, vx);
        bullet.type = BulletType::PlayerShot;
        bullet.isActive = true;
        bullet.isPlayerBullet = true;
        bullet.isHoming = true;
        m_bullets.push_back(bullet);
    }
}

void BulletManager::SpawnEnemyBullet(float x, float y, float vx, float vy, BulletType type, XMFLOAT4 color) {
    float radius = 4.0f;
    switch (type) {
        case BulletType::EnemySmall: radius = 4.0f; break;
        case BulletType::EnemyMedium: radius = 8.0f; break;
        case BulletType::EnemyLarge: radius = 12.0f; break;
        default: radius = 4.0f; break;
    }

    for (auto& bullet : m_bullets) {
        if (!bullet.isActive) {
            bullet.position = { x, y };
            bullet.velocity = { vx, vy };
            bullet.color = color;
            bullet.radius = radius;
            bullet.angle = atan2f(vy, vx);
            bullet.angularVelocity = 0.0f;
            bullet.type = type;
            bullet.isActive = true;
            bullet.isPlayerBullet = false;
            return;
        }
    }

    if (m_bullets.size() < MAX_BULLETS) {
        Bullet bullet;
        bullet.position = { x, y };
        bullet.velocity = { vx, vy };
        bullet.color = color;
        bullet.radius = radius;
        bullet.angle = atan2f(vy, vx);
        bullet.type = type;
        bullet.isActive = true;
        bullet.isPlayerBullet = false;
        m_bullets.push_back(bullet);
    }
}

void BulletManager::SpawnCircle(float x, float y, int count, float speed, BulletType type, XMFLOAT4 color) {
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * i) / count;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        SpawnEnemyBullet(x, y, vx, vy, type, color);
    }
}

void BulletManager::SpawnSpiral(float x, float y, int count, float speed, float angleOffset, BulletType type, XMFLOAT4 color) {
    for (int i = 0; i < count; i++) {
        float angle = angleOffset + (2.0f * PI * i) / count;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        SpawnEnemyBullet(x, y, vx, vy, type, color);
    }
}

void BulletManager::SpawnAimed(float x, float y, float targetX, float targetY, float speed, BulletType type, XMFLOAT4 color) {
    float dx = targetX - x;
    float dy = targetY - y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0f) {
        float vx = (dx / len) * speed;
        float vy = (dy / len) * speed;
        SpawnEnemyBullet(x, y, vx, vy, type, color);
    }
}

// Touhou-style flower pattern
void BulletManager::SpawnFlower(float x, float y, int petals, int bulletsPerPetal, float speed, float angleOffset, XMFLOAT4 color) {
    for (int p = 0; p < petals; p++) {
        float petalAngle = angleOffset + (2.0f * PI * p) / petals;
        
        for (int b = 0; b < bulletsPerPetal; b++) {
            float spreadAngle = petalAngle + (PI / 8.0f) * sinf(b * PI / bulletsPerPetal);
            float bulletSpeed = speed * (0.7f + 0.3f * (static_cast<float>(b) / bulletsPerPetal));
            
            float vx = cosf(spreadAngle) * bulletSpeed;
            float vy = sinf(spreadAngle) * bulletSpeed;
            
            // Gradient color for each petal
            XMFLOAT4 bulletColor = color;
            bulletColor.w = 0.7f + 0.3f * (static_cast<float>(b) / bulletsPerPetal);
            
            SpawnEnemyBullet(x, y, vx, vy, BulletType::EnemySmall, bulletColor);
        }
    }
}

// Rose curve pattern (mathematical rose)
void BulletManager::SpawnRose(float x, float y, int count, float speed, float time, XMFLOAT4 color) {
    float k = 5.0f; // Rose petals
    
    for (int i = 0; i < count; i++) {
        float theta = time + (2.0f * PI * i) / count;
        float r = cosf(k * theta);
        
        float angle = theta;
        float vx = cosf(angle) * speed * (0.5f + 0.5f * fabsf(r));
        float vy = sinf(angle) * speed * (0.5f + 0.5f * fabsf(r));
        
        // Rainbow color based on angle
        float hue = fmodf(theta / (2.0f * PI), 1.0f);
        XMFLOAT4 bulletColor = {
            fabsf(sinf(hue * 2.0f * PI)),
            fabsf(sinf(hue * 2.0f * PI + 2.0f * PI / 3.0f)),
            fabsf(sinf(hue * 2.0f * PI + 4.0f * PI / 3.0f)),
            1.0f
        };
        
        SpawnEnemyBullet(x, y, vx, vy, BulletType::EnemySmall, bulletColor);
    }
}

// Wave pattern
void BulletManager::SpawnWave(float x, float y, int count, float speed, float amplitude, float frequency, float time, XMFLOAT4 color) {
    for (int i = 0; i < count; i++) {
        float baseAngle = time + (2.0f * PI * i) / count;
        float waveOffset = amplitude * sinf(frequency * time + i * 0.5f);
        float angle = baseAngle + waveOffset;
        
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        
        SpawnEnemyBullet(x, y, vx, vy, BulletType::EnemySmall, color);
    }
}

// Double ring pattern with two colors
void BulletManager::SpawnRing(float x, float y, int count, float speed, float delay, BulletType type, XMFLOAT4 color1, XMFLOAT4 color2) {
    // Outer ring
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * i) / count;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        SpawnEnemyBullet(x, y, vx, vy, type, color1);
    }
    
    // Inner ring (offset)
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * i) / count + PI / count;
        float vx = cosf(angle) * speed * 0.7f;
        float vy = sinf(angle) * speed * 0.7f;
        SpawnEnemyBullet(x, y, vx, vy, type, color2);
    }
}
