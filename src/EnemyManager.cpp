#include "EnemyManager.h"
#include "Graphics.h"
#include "BulletManager.h"
#include "TextureLoader.h"

using namespace DirectX;

EnemyManager::EnemyManager()
    : m_bulletManager(nullptr)
    , m_playerPos{ 0.0f, 0.0f }
    , m_waveTimer(0.0f)
    , m_currentWave(0)
    , m_bossWaveJustStarted(false)
{
}

EnemyManager::~EnemyManager() {
}

void EnemyManager::Initialize(Graphics* graphics, BulletManager* bulletManager) {
    m_bulletManager = bulletManager;
    
    // Load enemy textures
    TextureLoader loader;
    loader.Initialize(graphics->GetDevice());
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring basePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\sprites\\";
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (loader.LoadTexture(basePath + L"enemy_barrel.png", &srv)) {
        m_barrelTexture.Attach(srv);
    }
    srv = nullptr;
    if (loader.LoadTexture(basePath + L"enemy_bottle.png", &srv)) {
        m_bottleTexture.Attach(srv);
    }
    srv = nullptr;
    if (loader.LoadTexture(basePath + L"boss_kai.png", &srv)) {
        m_bossTexture.Attach(srv);
    }
    srv = nullptr;
    if (loader.LoadTexture(basePath + L"enemy_glass.png", &srv)) {
        m_glassTexture.Attach(srv);
    }
    
    // 初期ウェーブの生成
    SpawnWave(0);
}

void EnemyManager::Update(float deltaTime, int screenWidth, int screenHeight) {
    // ウェーブタイマー
    m_waveTimer += deltaTime;
    
    // 敵の更新
    for (auto& enemy : m_enemies) {
        if (enemy->IsActive()) {
            enemy->Update(deltaTime, screenWidth, screenHeight, m_bulletManager, m_playerPos);
        }
    }

    // すべての敵が倒されたら次のウェーブ
    bool allDead = true;
    for (const auto& enemy : m_enemies) {
        if (enemy->IsActive()) {
            allDead = false;
            break;
        }
    }

    if (allDead && m_waveTimer > 2.0f) {
        // ボスウェーブ時は自動進行しない（Game側でクリア判定）
        if ((m_currentWave % 4) != 3) {
            // 次がボスウェーブなら5秒待機、それ以外は2秒
            float waitTime = ((m_currentWave + 1) % 4 == 3) ? 5.0f : 2.0f;
            if (m_waveTimer > waitTime) {
                m_currentWave++;
                SpawnWave(m_currentWave);
                m_waveTimer = 0.0f;
            }
        }
    }
}

void EnemyManager::Render(Graphics* graphics) {
    for (const auto& enemy : m_enemies) {
        if (enemy->IsActive()) {
            enemy->Render(graphics);
        }
    }
}

void EnemyManager::SpawnEnemy(float x, float y, float health, int patternId, EnemyType type) {
    auto enemy = std::make_unique<Enemy>();
    enemy->Initialize(x, y, health, type);
    enemy->SetBulletPattern(patternId);
    
    // Set texture based on type
    switch (type) {
        case EnemyType::Barrel:
            if (m_barrelTexture) enemy->SetTexture(m_barrelTexture.Get());
            break;
        case EnemyType::Bottle:
            if (m_bottleTexture) enemy->SetTexture(m_bottleTexture.Get());
            break;
        case EnemyType::Glass:
            if (m_glassTexture) enemy->SetTexture(m_glassTexture.Get());
            break;
        case EnemyType::Boss:
            if (m_bossTexture) enemy->SetTexture(m_bossTexture.Get());
            break;
        default:
            break;
    }
    
    m_enemies.push_back(std::move(enemy));
}

void EnemyManager::SpawnWave(int waveNumber) {
    Clear();
    
    // ボスウェーブ開始フラグ
    if ((waveNumber % 4) == 3) {
        m_bossWaveJustStarted = true;
    }
    
    switch (waveNumber % 4) {
        case 0:
            // ウェーブ1: 中央に樽1体
            SpawnEnemy(320.0f, 150.0f, 200.0f, 0, EnemyType::Barrel);
            break;
            
        case 1:
            // ウェーブ2: 左右にボトル2体
            SpawnEnemy(160.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            SpawnEnemy(480.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            break;
            
        case 2:
            // ウェーブ3: 混合3体
            SpawnEnemy(320.0f, 100.0f, 300.0f, 2, EnemyType::Glass);
            SpawnEnemy(160.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            SpawnEnemy(480.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            break;
            
        case 3:
            // ウェーブ4: ボス かい
            SpawnEnemy(320.0f, 150.0f, 500.0f, 3, EnemyType::Boss);
            break;
    }
}

void EnemyManager::Clear() {
    m_enemies.clear();
}

bool EnemyManager::AllEnemiesDead() const {
    for (const auto& enemy : m_enemies) {
        if (enemy->IsActive()) {
            return false;
        }
    }
    return !m_enemies.empty();
}

