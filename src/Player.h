#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Graphics;
class Input;
class BulletManager;

class Player {
public:
    Player();
    ~Player();

    void Initialize(Graphics* graphics, BulletManager* bulletManager);
    void Update(Input* input, float deltaTime, int screenWidth, int screenHeight);
    void Render(Graphics* graphics);

    void SetPosition(float x, float y);
    DirectX::XMFLOAT2 GetPosition() const { return m_position; }
    float GetRadius() const { return m_hitboxRadius; }
    int GetPower() const { return m_power; }
    void SetPower(int power) { m_power = power; }

private:
    DirectX::XMFLOAT2 m_position;
    float m_speed;
    float m_slowSpeed;
    float m_size;
    float m_hitboxRadius;
    float m_shootCooldown;
    float m_currentCooldown;
    bool m_isSlow;
    int m_power = 1;  // パワーレベル（1-4）

    BulletManager* m_bulletManager;
    ComPtr<ID3D11ShaderResourceView> m_texture;
};
