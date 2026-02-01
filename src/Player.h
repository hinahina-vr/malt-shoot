#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Graphics;
class Input;
class BulletManager;
class SoundManager;

class Player {
public:
    Player();
    ~Player();

    void Initialize(Graphics* graphics, BulletManager* bulletManager, SoundManager* sound = nullptr);
    void Update(Input* input, float deltaTime, int screenWidth, int screenHeight);
    void Render(Graphics* graphics);

    void SetPosition(float x, float y);
    DirectX::XMFLOAT2 GetPosition() const { return m_position; }
    float GetRadius() const { return m_hitboxRadius; }
    int GetPower() const { return m_power; }
    int GetEvolutionLevel() const { return m_evolutionLevel; }
    void SetPower(int power);
    void AddPower(int amount);

private:
    DirectX::XMFLOAT2 m_position;
    float m_speed;
    float m_slowSpeed;
    float m_size;
    float m_hitboxRadius;
    float m_shootCooldown;
    float m_currentCooldown;
    bool m_isSlow;
    int m_power = 0;           // パワーレベル（0-100、100で進化）
    int m_evolutionLevel = 0;  // 進化段階（0-4、5段階）

    BulletManager* m_bulletManager;
    SoundManager* m_sound;
    ComPtr<ID3D11ShaderResourceView> m_texture;
};
