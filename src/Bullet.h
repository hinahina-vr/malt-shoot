#pragma once

#include <DirectXMath.h>

enum class BulletType {
    PlayerShot,     // 自機弾
    EnemySmall,     // 小弾
    EnemyMedium,    // 中弾
    EnemyLarge,     // 大弾
    EnemyLaser      // レーザー
};

struct Bullet {
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 velocity;
    DirectX::XMFLOAT4 color;
    float radius;
    float angle;
    float angularVelocity;
    BulletType type;
    bool isActive;
    bool isPlayerBullet;

    Bullet()
        : position{ 0.0f, 0.0f }
        , velocity{ 0.0f, 0.0f }
        , color{ 1.0f, 1.0f, 1.0f, 1.0f }
        , radius(4.0f)
        , angle(0.0f)
        , angularVelocity(0.0f)
        , type(BulletType::EnemySmall)
        , isActive(false)
        , isPlayerBullet(false)
    {}
};
