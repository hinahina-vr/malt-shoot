#pragma once

#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include "Bullet.h"

class Graphics;

class BulletManager {
public:
    BulletManager();
    ~BulletManager();

    void Initialize(Graphics* graphics);
    void Update(float deltaTime, int screenWidth, int screenHeight);
    void Render(Graphics* graphics);
    void Clear();

    // Bullet spawn
    void SpawnPlayerBullet(float x, float y, float vx, float vy);
    void SpawnHomingMissile(float x, float y, float vx, float vy);  // ホーミングミサイル
    void SpawnEnemyBullet(float x, float y, float vx, float vy, BulletType type, DirectX::XMFLOAT4 color);
    
    // Basic patterns
    void SpawnCircle(float x, float y, int count, float speed, BulletType type, DirectX::XMFLOAT4 color);
    void SpawnSpiral(float x, float y, int count, float speed, float angleOffset, BulletType type, DirectX::XMFLOAT4 color);
    void SpawnAimed(float x, float y, float targetX, float targetY, float speed, BulletType type, DirectX::XMFLOAT4 color);
    
    // Touhou-style patterns
    void SpawnFlower(float x, float y, int petals, int bulletsPerPetal, float speed, float angleOffset, DirectX::XMFLOAT4 color);
    void SpawnRose(float x, float y, int count, float speed, float time, DirectX::XMFLOAT4 color);
    void SpawnWave(float x, float y, int count, float speed, float amplitude, float frequency, float time, DirectX::XMFLOAT4 color);
    void SpawnRing(float x, float y, int count, float speed, float delay, BulletType type, DirectX::XMFLOAT4 color1, DirectX::XMFLOAT4 color2);

    // 当たり判定用
    const std::vector<Bullet>& GetBullets() const { return m_bullets; }
    std::vector<Bullet>& GetBullets() { return m_bullets; }

private:
    std::vector<Bullet> m_bullets;
    static const int MAX_BULLETS = 2000;
    
    // 樽テクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_barrelTexture;
};
