#include "Enemy.h"
#include "Graphics.h"
#include "BulletManager.h"
#include <cmath>

using namespace DirectX;

constexpr float PI = 3.14159265358979f;

Enemy::Enemy()
    : m_position{ 0.0f, 0.0f }
    , m_targetPosition{ 0.0f, 0.0f }
    , m_health(100.0f)
    , m_maxHealth(100.0f)
    , m_radius(24.0f)
    , m_speed(100.0f)
    , m_shootTimer(0.0f)
    , m_patternTimer(0.0f)
    , m_patternId(0)
    , m_patternPhase(0)
    , m_state(EnemyState::Entering)
    , m_type(EnemyType::Barrel)
{
}

Enemy::~Enemy() {
}

void Enemy::Initialize(float x, float y, float health, EnemyType type) {
    m_position = { x, -50.0f };
    m_targetPosition = { x, y };
    m_health = health;
    m_maxHealth = health;
    m_state = EnemyState::Entering;
    m_shootTimer = 0.0f;
    m_patternTimer = 0.0f;
    m_patternPhase = 0;
    m_type = type;
    
    // Set radius based on type
    switch (type) {
        case EnemyType::Boss: m_radius = 48.0f; break;
        case EnemyType::Bottle: m_radius = 28.0f; break;
        case EnemyType::Glass: m_radius = 20.0f; break;
        default: m_radius = 24.0f; break;
    }
}

void Enemy::SetTexture(ID3D11ShaderResourceView* texture) {
    m_texture = texture;
}

void Enemy::Update(float deltaTime, int screenWidth, int screenHeight, BulletManager* bulletManager, XMFLOAT2 playerPos) {
    // 無敵時間のカウントダウン
    if (m_invincibleTimer > 0) {
        m_invincibleTimer -= deltaTime;
        if (m_invincibleTimer <= 0) {
            m_showingCutin = false;  // カットイン終了
        }
    }
    
    switch (m_state) {
        case EnemyState::Entering: {
            float dx = m_targetPosition.x - m_position.x;
            float dy = m_targetPosition.y - m_position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < 5.0f) {
                m_position = m_targetPosition;
                m_state = EnemyState::Active;
            } else {
                m_position.x += (dx / dist) * m_speed * 2.0f * deltaTime;
                m_position.y += (dy / dist) * m_speed * 2.0f * deltaTime;
            }
            break;
        }
        
        case EnemyState::Active: {
            m_shootTimer += deltaTime;
            m_patternTimer += deltaTime;
            
            // ボスはHP%に応じてパターン自動切り替え（スペルカード風）
            if (m_type == EnemyType::Boss) {
                float hpPercent = m_health / m_maxHealth;
                if (hpPercent > 0.75f) m_patternId = 0;       // Phase 1: Flower
                else if (hpPercent > 0.50f) m_patternId = 4;  // Phase 2: Rainbow Spiral
                else if (hpPercent > 0.25f) m_patternId = 2;  // Phase 3: Wave + Aimed
                else m_patternId = 3;                          // Phase 4: Double Ring
            }
            
            ExecuteBulletPattern(bulletManager, playerPos);
            break;
        }
        
        case EnemyState::Leaving: {
            m_position.y -= m_speed * deltaTime;
            if (m_position.y < -100.0f) {
                m_state = EnemyState::Dead;
            }
            break;
        }
        
        case EnemyState::Dead:
            break;
    }
}

void Enemy::Render(Graphics* graphics) {
    if (m_state == EnemyState::Dead) return;

    // Get colors for glow effects based on type
    XMFLOAT4 glowColor;
    switch (m_type) {
        case EnemyType::Barrel: glowColor = { 0.6f, 0.4f, 0.2f, 0.5f }; break;
        case EnemyType::Bottle: glowColor = { 0.9f, 0.6f, 0.2f, 0.5f }; break;
        case EnemyType::Glass: glowColor = { 0.4f, 0.7f, 0.9f, 0.5f }; break;
        case EnemyType::Boss: glowColor = { 1.0f, 0.8f, 0.3f, 0.6f }; break;
        default: glowColor = { 0.5f, 0.5f, 0.5f, 0.5f }; break;
    }

    // Draw glow
    float glowSize = m_type == EnemyType::Boss ? m_radius * 1.5f : m_radius;
    graphics->DrawGlowCircle(m_position.x, m_position.y, glowSize, glowColor, 
                             m_type == EnemyType::Boss ? 5 : 3);

    // Draw texture if available, otherwise fallback to shapes
    if (m_texture) {
        float size = m_radius * 2.0f;
        graphics->DrawTexturedSprite(
            m_position.x - m_radius, m_position.y - m_radius,
            size, size, m_texture.Get(), XMFLOAT4(1, 1, 1, 1));
    } else {
        // Fallback: colored circle
        graphics->DrawCircle(m_position.x, m_position.y, m_radius, glowColor);
    }

    // HP円グラフ（敵の周りを囲む）
    float hpRatio = m_health / m_maxHealth;
    float arcRadius = m_radius + 10.0f;  // 敵の少し外側
    float thickness = m_type == EnemyType::Boss ? 6.0f : 4.0f;
    
    // 背景（薄いグレー）
    graphics->DrawCircleArc(m_position.x, m_position.y, arcRadius, thickness,
        -PI / 2.0f, 3.0f * PI / 2.0f, XMFLOAT4(0.2f, 0.2f, 0.2f, 0.5f));
    
    // HP表示（緑→赤のグラデーション）
    XMFLOAT4 hpColor = hpRatio > 0.5f 
        ? XMFLOAT4(0.2f, 0.9f, 0.4f, 1.0f)
        : XMFLOAT4(0.9f, 0.3f, 0.2f, 1.0f);
    float endAngle = -PI / 2.0f + hpRatio * 2.0f * PI;  // 上から時計回り
    graphics->DrawCircleArc(m_position.x, m_position.y, arcRadius, thickness,
        -PI / 2.0f, endAngle, hpColor);
}

void Enemy::TakeDamage(float damage) {
    // 無敵時間中はダメージを受けない
    if (m_invincibleTimer > 0) return;
    
    m_health -= damage;
    if (m_health <= 0) {
        // ボスはスペルカードが残っていれば復活
        if (m_type == EnemyType::Boss && m_currentSpell < m_spellCards - 1) {
            m_currentSpell++;
            m_health = m_maxHealth;  // HP全回復
            m_patternTimer = 0.0f;   // パターンリセット
            m_shootTimer = 0.0f;
            // 次のパターンに切り替え（currentSpellに応じて）
            m_patternId = m_currentSpell;
            // 無敵時間とカットイン
            m_invincibleTimer = 2.0f;  // 2秒間無敵
            m_showingCutin = true;     // カットイン表示
        } else {
            m_health = 0;
            m_state = EnemyState::Dead;
        }
    }
}

void Enemy::ExecuteBulletPattern(BulletManager* bulletManager, XMFLOAT2 playerPos) {
    switch (m_patternId) {
        case 0: // Flower pattern (Touhou style)
            if (m_shootTimer >= 1.5f) {
                bulletManager->SpawnFlower(
                    m_position.x, m_position.y,
                    6, 5, 120.0f, m_patternTimer,
                    XMFLOAT4(1.0f, 0.4f, 0.6f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;

        case 1: // Rose curve pattern
            if (m_shootTimer >= 0.15f) {
                bulletManager->SpawnRose(
                    m_position.x, m_position.y,
                    8, 150.0f, m_patternTimer * 3.0f,
                    XMFLOAT4(0.3f, 0.5f, 1.0f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;

        case 2: // Wave pattern with aimed bullets
            if (m_shootTimer >= 0.2f) {
                bulletManager->SpawnWave(
                    m_position.x, m_position.y,
                    12, 140.0f, 0.3f, 5.0f, m_patternTimer,
                    XMFLOAT4(0.4f, 1.0f, 0.6f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            // Aimed shots
            if (static_cast<int>(m_patternTimer * 2.0f) % 5 == 0 && m_shootTimer < 0.1f) {
                bulletManager->SpawnAimed(
                    m_position.x, m_position.y,
                    playerPos.x, playerPos.y,
                    200.0f,
                    BulletType::EnemyMedium,
                    XMFLOAT4(1.0f, 0.8f, 0.2f, 1.0f)
                );
            }
            break;

        case 3: // Double ring pattern
            if (m_shootTimer >= 0.8f) {
                bulletManager->SpawnRing(
                    m_position.x, m_position.y,
                    20, 130.0f, 0.0f,
                    BulletType::EnemySmall,
                    XMFLOAT4(1.0f, 0.3f, 0.8f, 1.0f),
                    XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;

        case 4: // Spiral with color gradient
            if (m_shootTimer >= 0.08f) {
                float hue = fmodf(m_patternTimer * 0.5f, 1.0f);
                XMFLOAT4 color = {
                    fabsf(sinf(hue * 2.0f * PI)),
                    fabsf(sinf(hue * 2.0f * PI + 2.0f * PI / 3.0f)),
                    fabsf(sinf(hue * 2.0f * PI + 4.0f * PI / 3.0f)),
                    1.0f
                };
                bulletManager->SpawnSpiral(
                    m_position.x, m_position.y,
                    4, 180.0f, m_patternTimer * 3.0f,
                    BulletType::EnemySmall,
                    color
                );
                m_shootTimer = 0.0f;
            }
            break;

        default: // Default: circle pattern
            if (m_shootTimer >= 1.0f) {
                bulletManager->SpawnCircle(
                    m_position.x, m_position.y,
                    16, 150.0f,
                    BulletType::EnemySmall,
                    XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;
    }
}
