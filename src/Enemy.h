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
            L"琥珀符「アンバー・メモリー」",
            L"熟成符「12年の夢」",
            L"樽霊符「バレル・ダンス」",
            L"終宴符「ラスト・ドロップ」"
        };
        if (m_currentSpell >= 0 && m_currentSpell < 4) {
            return names[m_currentSpell];
        }
        return L"";
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
    int m_spellCards = 4;   // 復活回数（4つのスペル）
    int m_currentSpell = 0; // 現在のスペル番号
    float m_invincibleTimer = 0.0f;  // 無敵時間
    bool m_showingCutin = false;     // カットイン表示中
    
    // 雑魚敵タイムアウト
    float m_lifetime = 0.0f;          // 生存時間
    float m_maxLifetime = 8.0f;       // 最大生存時間（8秒で退場開始）
    
    // HP表示イージング
    float m_displayHealth = 0.0f;     // 表示用HP（イージング付き）
};
