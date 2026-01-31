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

    // 弾幕パターンの設定
    void SetBulletPattern(int patternId) { m_patternId = patternId; }

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
};
