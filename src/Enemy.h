#pragma once

#include <DirectXMath.h>
#include <functional>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Graphics;
class BulletManager;

enum class EnemyState {
    Entering,   // 登場中
    Active,     // 行動中
    Leaving,    // 退場中
    Dying,      // 死亡アニメーション中（ボス用）
    Dead        // 撃破
};

enum class EnemyType {
    Barrel,
    Bottle,
    Glass,
    Boss
};

class Enemy {
public:
    Enemy();
    ~Enemy();

    void Initialize(float x, float y, float health, EnemyType type = EnemyType::Barrel);
    void SetTexture(ID3D11ShaderResourceView* texture);
    void Update(float deltaTime, int screenWidth, int screenHeight, BulletManager* bulletManager, DirectX::XMFLOAT2 playerPos);
    void Render(Graphics* graphics);

    void TakeDamage(float damage);
    bool IsActive() const { return m_state != EnemyState::Dead && m_health > 0; }
    
    DirectX::XMFLOAT2 GetPosition() const { return m_position; }
    float GetRadius() const { return m_radius; }
    EnemyState GetState() const { return m_state; }
    EnemyType GetType() const { return m_type; }
    float GetHealth() const { return m_health; }
    float GetMaxHealth() const { return m_maxHealth; }
    float GetHealthPercent() const { return m_maxHealth > 0 ? m_health / m_maxHealth : 0; }
    float GetDisplayHealthPercent() const { return m_maxHealth > 0 ? m_displayHealth / m_maxHealth : 0; }
    bool IsBoss() const { return m_type == EnemyType::Boss; }

    // 弾幕パターンの設定
    void SetBulletPattern(int patternId) { m_patternId = patternId; }
    
    // ボススペルカード（復活システム）
    int GetSpellCards() const { return m_spellCards; }
    int GetCurrentSpell() const { return m_currentSpell; }
    bool HasSpellsRemaining() const { return m_currentSpell < m_spellCards; }
    int GetRemainingSpells() const { return m_spellCards - m_currentSpell - 1; }
    bool IsInvincible() const { return m_invincibleTimer > 0; }
    bool IsShowingCutin() const { return m_showingCutin; }
    void ClearCutin() { m_showingCutin = false; }
    
    // スペルカード名（ウイスキーテーマ）
    const wchar_t* GetSpellCardName() const {
        static const wchar_t* names[] = {
            L"シングル「オーダーズ・オン・ザ・ロック」",
            L"ダブル「トワイライト・バレル」",
            L"スリーショット「スモーキー・アイラ」",
            L"フォーシーズン「シェリー・キャスク」",
            L"禁断「マスターズ・リザーブ」"
        };
        if (m_currentSpell >= 0 && m_currentSpell < 5) {
            return names[m_currentSpell];
        }
        return L"";
    }
    
    // ボスアニメーションフレーム設定
    void SetAnimFrames(ID3D11ShaderResourceView* frame1, ID3D11ShaderResourceView* frame2, ID3D11ShaderResourceView* frame3) {
        if (frame1) m_animFrames[0] = frame1;
        if (frame2) m_animFrames[1] = frame2;
        if (frame3) m_animFrames[2] = frame3;
    }

private:
    void ExecuteBulletPattern(BulletManager* bulletManager, DirectX::XMFLOAT2 playerPos);

    DirectX::XMFLOAT2 m_position;
    DirectX::XMFLOAT2 m_targetPosition;
    float m_health;
    float m_maxHealth;
    float m_radius;
    float m_speed;
    float m_shootTimer;
    float m_patternTimer;
    int m_patternId;
    int m_patternPhase;
    EnemyState m_state;
    EnemyType m_type;
    ComPtr<ID3D11ShaderResourceView> m_texture;
    
    // ボススペルカード
    int m_spellCards = 5;   // 復活回数（5つのスペル）
    int m_currentSpell = 0; // 現在のスペル番号
    float m_invincibleTimer = 0.0f;  // 無敵時間
    bool m_showingCutin = false;     // カットイン表示中
    
    // 雑魚敵タイムアウト
    float m_lifetime = 0.0f;          // 生存時間
    float m_maxLifetime = 8.0f;       // 最大生存時間（8秒で退場開始）
    
    // HP表示イージング
    float m_displayHealth = 0.0f;     // 表示用HP（イージング付き）
    
    // 被弾フラッシュ
    float m_flashTimer = 0.0f;        // 白フラッシュタイマー
    
    // 死亡アニメーション（ボス用）
    float m_deathTimer = 0.0f;        // 死亡アニメーションタイマー
    float m_deathDuration = 2.0f;     // 死亡アニメーション時間
    
    // ボスアニメーションフレーム
    ComPtr<ID3D11ShaderResourceView> m_animFrames[3];
    int m_currentFrame = 0;
    float m_frameTimer = 0.0f;
    float m_frameInterval = 0.2f;     // フレーム切り替え間隔（0.2秒）
};
