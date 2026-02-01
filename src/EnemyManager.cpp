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
    if (loader.LoadTexture(basePath + L"boss_hinahina.png", &srv)) {
        m_bossTexture.Attach(srv);
    }
    // ボスアニメーション用3フレーム読み込み
    for (int i = 0; i < 3; i++) {
        srv = nullptr;
        wchar_t filename[64];
        swprintf_s(filename, L"boss_frame_%d.png", i + 1);
        if (loader.LoadTexture(basePath + filename, &srv)) {
            m_bossFrames[i].Attach(srv);
        }
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
        // ウェーブ自動進行（ボスウェーブはスキップして雑魚ループ）
        float waitTime = 2.0f;
        if (m_waveTimer > waitTime) {
            m_currentWave++;
            // ボスウェーブ（4の倍数-1）はスキップして雑魚に戻る
            if ((m_currentWave % 4) == 3) {
                m_currentWave++;  // ボスウェーブを飛ばす
            }
            SpawnWave(m_currentWave);
            m_waveTimer = 0.0f;
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
            // 1枚絵を使用（アニメーションなし）
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
            // ウェーブ1: 樽2体（2倍）
            SpawnEnemy(220.0f, 150.0f, 200.0f, 0, EnemyType::Barrel);
            SpawnEnemy(420.0f, 150.0f, 200.0f, 0, EnemyType::Barrel);
            break;
            
        case 1:
            // ウェーブ2: ボトル4体（2倍）
            SpawnEnemy(100.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            SpawnEnemy(220.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            SpawnEnemy(420.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            SpawnEnemy(540.0f, 120.0f, 150.0f, 1, EnemyType::Bottle);
            break;
            
        case 2:
            // ウェーブ3: 混合6体（2倍）
            SpawnEnemy(220.0f, 100.0f, 300.0f, 2, EnemyType::Glass);
            SpawnEnemy(420.0f, 100.0f, 300.0f, 2, EnemyType::Glass);
            SpawnEnemy(100.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            SpawnEnemy(260.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            SpawnEnemy(380.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            SpawnEnemy(540.0f, 180.0f, 100.0f, 0, EnemyType::Barrel);
            break;
            
        case 3:
            // ウェーブ4: ボス かい
            // ボスはここではスポーンしない（Game側で3秒ディレイ後にスポーン）
            m_bossWaveJustStarted = true;
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

// 雑魚敵がいるかチェック（ボス以外のアクティブな敵）
bool EnemyManager::HasActiveEnemies() const {
    for (const auto& enemy : m_enemies) {
        if (enemy->IsActive() && !enemy->IsBoss()) {
            return true;
        }
    }
    return false;
}

// 雑魚敵を全滅させる（ボス登場時）
void EnemyManager::ClearNonBossEnemies() {
    m_enemies.erase(
        std::remove_if(m_enemies.begin(), m_enemies.end(),
            [](const std::unique_ptr<Enemy>& e) {
                return !e->IsBoss();
            }),
        m_enemies.end()
    );
}

void EnemyManager::DamageBoss(float damage) {
    for (auto& enemy : m_enemies) {
        if (enemy->IsActive() && enemy->GetType() == EnemyType::Boss) {
            enemy->TakeDamage(damage);
            break;
        }
    }
}
