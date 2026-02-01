#pragma once

#include <vector>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include "Enemy.h"

using Microsoft::WRL::ComPtr;

class Graphics;
class BulletManager;

class EnemyManager {
public:
    EnemyManager();
    ~EnemyManager();

    void Initialize(Graphics* graphics, BulletManager* bulletManager);
    void Update(float deltaTime, int screenWidth, int screenHeight);
    void Render(Graphics* graphics);

    void SpawnEnemy(float x, float y, float health, int patternId, EnemyType type = EnemyType::Barrel);
    void SpawnWave(int waveNumber);
    void Clear();

    void SetPlayerPosition(DirectX::XMFLOAT2 pos) { m_playerPos = pos; }
    const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return m_enemies; }
    int GetCurrentWave() const { return m_currentWave; }
    bool IsBossWave() const { return (m_currentWave % 4) == 3; }
    bool AllEnemiesDead() const;
    bool HasActiveEnemies() const;  // 雑魚敵がいるかチェック
    void ResetWaves() { m_currentWave = 0; m_waveTimer = 0.0f; m_bossWaveJustStarted = false; }
    bool DidBossWaveJustStart() const { return m_bossWaveJustStarted; }
    void ClearBossWaveStartFlag() { m_bossWaveJustStarted = false; }
    void DamageBoss(float damage);  // デバッグ用ボスダメージ

private:
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    BulletManager* m_bulletManager;
    DirectX::XMFLOAT2 m_playerPos;
    float m_waveTimer;
    int m_currentWave;
    bool m_bossWaveJustStarted;  // ボスウェーブ開始フラグ
    
    // Enemy textures
    ComPtr<ID3D11ShaderResourceView> m_barrelTexture;
    ComPtr<ID3D11ShaderResourceView> m_bottleTexture;
    ComPtr<ID3D11ShaderResourceView> m_glassTexture;
    ComPtr<ID3D11ShaderResourceView> m_bossTexture;
    ComPtr<ID3D11ShaderResourceView> m_bossFrames[3];  // ボスアニメーション用3フレーム
};
