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
    m_displayHealth = health;  // 表示用HP初期化
    m_state = EnemyState::Entering;
    m_shootTimer = 0.0f;
    m_patternTimer = 0.0f;
    m_patternPhase = 0;
    m_type = type;
    m_lifetime = 0.0f;  // 生存時間初期化
    
    // Set radius based on type
    switch (type) {
        case EnemyType::Boss: m_radius = 48.0f; break;
        case EnemyType::Bottle: m_radius = 28.0f; break;
        case EnemyType::Glass: m_radius = 20.0f; break;
        case EnemyType::Fairy: m_radius = 16.0f; break;  // 小さい
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
            m_lifetime += deltaTime;
            
            // HP表示イージング（スムーズに下がる）
            float easeSpeed = 5.0f * deltaTime;
            if (m_displayHealth > m_health) {
                m_displayHealth -= (m_displayHealth - m_health) * easeSpeed;
                if (m_displayHealth < m_health) m_displayHealth = m_health;
            }
            
            // 被弾フラッシュタイマー更新
            if (m_flashTimer > 0.0f) {
                m_flashTimer -= deltaTime;
            }
            
            // ボスアニメーションフレーム更新
            if (m_type == EnemyType::Boss) {
                m_frameTimer += deltaTime;
                if (m_frameTimer >= m_frameInterval) {
                    m_frameTimer = 0.0f;
                    m_currentFrame = (m_currentFrame + 1) % 3;
                }
            }
            
            // Fairy: 横移動（左から来たら右へ、右から来たら左へ）
            if (m_type == EnemyType::Fairy) {
                float moveSpeed = 200.0f;
                // 初期位置で判定（左端から出たなら右へ、右端から出たなら左へ）
                if (m_targetPosition.x < 320.0f) {
                    m_position.x += moveSpeed * deltaTime;  // 右へ
                } else {
                    m_position.x -= moveSpeed * deltaTime;  // 左へ
                }
                // 画面外で消滅
                if (m_position.x < -50.0f || m_position.x > 690.0f) {
                    m_state = EnemyState::Leaving;
                }
            }
            
            // 雑魚敵タイムアウト（ボス以外、8秒で退場開始）
            if (m_type != EnemyType::Boss && m_lifetime > m_maxLifetime) {
                m_state = EnemyState::Leaving;
            }
            
            // ボスはHP%に応じてパターン自動切り替え（スペルカード風）
            if (m_type == EnemyType::Boss) {
                float hpPercent = m_health / m_maxHealth;
                int prevPattern = m_patternId;
                if (hpPercent > 0.75f) m_patternId = 0;       // Phase 1: 琥珀符
                else if (hpPercent > 0.50f) m_patternId = 4;  // Phase 2: 熟成符
                else if (hpPercent > 0.25f) m_patternId = 2;  // Phase 3: 樽霊符
                else m_patternId = 3;                          // Phase 4: 終宴符
                
                // スペルカード毎にテーマを決めて動きを付ける
                float t = m_patternTimer;
                float centerX = m_targetPosition.x;
                float centerY = m_targetPosition.y;
                
                // ターゲット位置を計算（直接設定ではなく目標位置を算出）
                float targetX = centerX;
                float targetY = centerY;
                
                switch (m_patternId) {
                    case 0: {
                        // 琥珀符「アンバー・メモリー」: 優雅にゆったり左右
                        float slowWave = sinf(t * 0.3f);
                        targetX = centerX + slowWave * 100.0f;
                        targetY = centerY + sinf(t * 0.5f) * 20.0f;
                        break;
                    }
                    case 4: {
                        // 熟成符「12年の夢」: 8の字を描く動き
                        targetX = centerX + sinf(t * 0.6f) * 120.0f;
                        targetY = centerY + sinf(t * 1.2f) * 40.0f;
                        break;
                    }
                    case 2: {
                        // 樽霊符「バレル・ダンス」: 急停止→ダッシュ→急停止
                        float phase = fmodf(t, 4.0f);  // 4秒サイクル
                        if (phase < 1.5f) {
                            // 停止（じわじわ揺れ）
                            targetX = centerX + sinf(t * 3.0f) * 10.0f;
                            targetY = centerY;
                        } else if (phase < 2.5f) {
                            // ダッシュ！（右へ）
                            float dashProgress = (phase - 1.5f);
                            targetX = centerX + dashProgress * 200.0f;
                            targetY = centerY;
                        } else {
                            // ダッシュ！（戻る）
                            float returnProgress = (phase - 2.5f) / 1.5f;
                            targetX = centerX + 200.0f - returnProgress * 200.0f;
                            targetY = centerY;
                        }
                        break;
                    }
                    case 3: {
                        // 終宴符「ラスト・ドロップ」: 激しく不規則に動く
                        float chaos = sinf(t * 2.0f) * cosf(t * 1.7f);
                        targetX = centerX + chaos * 150.0f;
                        targetY = centerY + sinf(t * 2.5f) * 50.0f + cosf(t * 1.3f) * 30.0f;
                        break;
                    }
                    default: {
                        targetX = centerX + sinf(t * 0.5f) * 100.0f;
                        targetY = centerY + sinf(t * 0.8f) * 30.0f;
                        break;
                    }
                }
                
                // 画面内に収める
                if (targetX < m_radius + 50) targetX = m_radius + 50;
                if (targetX > screenWidth - m_radius - 50) targetX = screenWidth - m_radius - 50;
                if (targetY < m_radius) targetY = m_radius;
                if (targetY > 300.0f) targetY = 300.0f;
                
                // lerp補間でスムーズに移動（フレーム飛び防止）
                float lerpSpeed = 8.0f * deltaTime;  // スムーズ補間
                m_position.x += (targetX - m_position.x) * lerpSpeed;
                m_position.y += (targetY - m_position.y) * lerpSpeed;
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
        
        case EnemyState::Dying: {
            // 死亡アニメーション進行
            m_deathTimer += deltaTime;
            
            // 派手に振動
            m_position.x += (sinf(m_deathTimer * 50.0f) * 5.0f);
            m_position.y += (cosf(m_deathTimer * 40.0f) * 3.0f);
            
            // アニメーション終了後にDead状態へ
            if (m_deathTimer >= m_deathDuration) {
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
        case EnemyType::Fairy: glowColor = { 0.8f, 0.5f, 1.0f, 0.4f }; break;  // 紫ピンク
        case EnemyType::Boss: glowColor = { 1.0f, 0.8f, 0.3f, 0.6f }; break;
        default: glowColor = { 0.5f, 0.5f, 0.5f, 0.5f }; break;
    }
    // Dying状態（ボス死亡アニメーション）
    if (m_state == EnemyState::Dying) {
        float progress = m_deathTimer / m_deathDuration;
        
        // 派手な爆発エフェクト（複数のリング）
        for (int i = 0; i < 5; i++) {
            float ringProgress = fmodf(progress * 3.0f + i * 0.2f, 1.0f);
            float ringRadius = m_radius * (1.0f + ringProgress * 4.0f);
            float ringAlpha = (1.0f - ringProgress) * 0.8f;
            
            XMFLOAT4 ringColor = {
                1.0f,
                0.6f + sinf(m_deathTimer * 10.0f + i) * 0.4f,
                0.2f,
                ringAlpha
            };
            graphics->DrawGlowCircle(m_position.x, m_position.y, ringRadius, ringColor, 3);
        }
        
        // 中心の白いフラッシュ
        float flashAlpha = sinf(m_deathTimer * 20.0f) * 0.5f + 0.5f;
        graphics->DrawGlowCircle(m_position.x, m_position.y, m_radius * (1.0f - progress * 0.5f),
            XMFLOAT4(1.0f, 1.0f, 1.0f, flashAlpha), 5);
        
        // 本体は徐々に小さく、透明に
        float scale = 1.0f - progress * 0.8f;
        float alpha = 1.0f - progress;
        if (m_texture) {
            float size = m_radius * 2.0f * scale;
            graphics->DrawTexturedSprite(
                m_position.x - m_radius * scale, m_position.y - m_radius * scale,
                size, size, m_texture.Get(), XMFLOAT4(1, 1, 1, alpha));
        }
        return;  // Dying状態は通常描画をスキップ
    }

    // Draw glow
    float glowSize = m_type == EnemyType::Boss ? m_radius * 1.5f : m_radius;
    graphics->DrawGlowCircle(m_position.x, m_position.y, glowSize, glowColor, 
                             m_type == EnemyType::Boss ? 5 : 3);

    // Draw texture if available, otherwise fallback to shapes
    if (m_texture) {
        float size = m_radius * 2.0f;
        
        // ボスは浮遊アニメーション（上下に揺れる）
        float drawY = m_position.y;
        if (m_type == EnemyType::Boss) {
            float floatOffset = sinf(m_patternTimer * 2.0f) * 8.0f;  // 上下8pxの揺れ
            drawY += floatOffset;
        }
        
        // 被弾フラッシュ時は白く光らせる
        XMFLOAT4 tintColor = (m_flashTimer > 0.0f) 
            ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)  // フラッシュ中は白
            : XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 通常
        
        // ボスはアニメーションフレームを使用
        ID3D11ShaderResourceView* texToUse = m_texture.Get();
        if (m_type == EnemyType::Boss && m_animFrames[m_currentFrame]) {
            texToUse = m_animFrames[m_currentFrame].Get();
        }
        
        graphics->DrawTexturedSprite(
            m_position.x - m_radius, drawY - m_radius,
            size, size, texToUse, tintColor);
        
        // 白フラッシュオーバーレイ（被弾時）
        if (m_flashTimer > 0.0f) {
            float flashAlpha = m_flashTimer / 0.1f;  // 0.1秒でフェードアウト
            graphics->DrawCircle(m_position.x, m_position.y, m_radius, 
                XMFLOAT4(1.0f, 1.0f, 1.0f, flashAlpha * 0.5f));
        }
    } else {
        // Fallback: colored circle
        XMFLOAT4 drawColor = (m_flashTimer > 0.0f) 
            ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)  // フラッシュ中は白
            : glowColor;
        graphics->DrawCircle(m_position.x, m_position.y, m_radius, drawColor);
    }

    // HP円グラフ（ボスのみ表示）
    if (m_type == EnemyType::Boss) {
        float hpRatio = m_displayHealth / m_maxHealth;  // イージング適用
        float arcRadius = m_radius + 15.0f;  // 敵の少し外側
        float thickness = 8.0f;
        
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
}

void Enemy::TakeDamage(float damage) {
    // 無敵時間中はダメージを受けない
    if (m_invincibleTimer > 0) return;
    
    m_health -= damage;
    m_flashTimer = 0.1f;  // 被弾フラッシュ（0.1秒間白く光る）
    if (m_health <= 0) {
        // ボスはスペルカードが残っていれば復活
        if (m_type == EnemyType::Boss && m_currentSpell < m_spellCards - 1) {
            m_currentSpell++;
            m_health = m_maxHealth;  // HP全回復
            m_displayHealth = m_maxHealth;  // 表示HPも満タンに
            m_patternTimer = 0.0f;   // パターンリセット
            m_shootTimer = 0.0f;
            // 次のパターンに切り替え（currentSpellに応じて）
            m_patternId = m_currentSpell;
            // 無敵時間とカットイン
            m_invincibleTimer = 2.0f;  // 2秒間無敵
            m_showingCutin = true;     // カットイン表示
        } else {
            m_health = 0;
            // ボスは死亡アニメーションを再生
            if (m_type == EnemyType::Boss) {
                m_state = EnemyState::Dying;
                m_deathTimer = 0.0f;
            } else {
                m_state = EnemyState::Dead;
            }
        }
    }
}

void Enemy::ExecuteBulletPattern(BulletManager* bulletManager, XMFLOAT2 playerPos) {
    switch (m_patternId) {
        case 0: // Flower pattern (Touhou style)
            if (m_shootTimer >= 0.5f) {  // 3倍激しく
                bulletManager->SpawnFlower(
                    m_position.x, m_position.y,
                    6, 5, 120.0f, m_patternTimer,
                    XMFLOAT4(1.0f, 0.4f, 0.6f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;

        case 1: // Rose curve pattern
            if (m_shootTimer >= 0.05f) {  // 3倍激しく
                bulletManager->SpawnRose(
                    m_position.x, m_position.y,
                    8, 150.0f, m_patternTimer * 3.0f,
                    XMFLOAT4(0.3f, 0.5f, 1.0f, 1.0f)
                );
                m_shootTimer = 0.0f;
            }
            break;

        case 2: // Wave pattern with aimed bullets
            if (m_shootTimer >= 0.07f) {  // 3倍激しく
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
            if (m_shootTimer >= 0.27f) {  // 3倍激しく
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
            if (m_shootTimer >= 0.03f) {  // 3倍激しく
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
