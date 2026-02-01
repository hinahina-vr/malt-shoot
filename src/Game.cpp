#include "Game.h"
#include <cmath>
#include <fstream>

// Play area constants (Full HD - 1920x1080)
// Vertical smartphone style with sidebar
constexpr int PLAY_AREA_WIDTH = 1200;   // 横幅倍増！
constexpr int PLAY_AREA_HEIGHT = 1080;
constexpr int SIDEBAR_WIDTH = 720;      // 残りをUI用に

Game::Game()
    : m_hWnd(nullptr)
    , m_width(0)
    , m_height(0)
    , m_isRunning(false)
    , m_deltaTime(0.0f)
    , m_frameCount(0)
    , m_fpsTimer(0.0f)
    , m_currentFPS(0.0f)
    , m_score(0)
    , m_hiScore(0)
    , m_lives(3)
    , m_bombs(3)
    , m_power(0)
    , m_maxPower(128)
    , m_graze(0)
    , m_grazeRadius(30.0f)
    , m_specialGauge(0.0f)
    , m_maxSpecialGauge(100.0f)
    , m_specialReady(false)
    , m_combo(0)
    , m_comboTimer(0.0f)
    , m_bossMode(false)
    , m_isPaused(false)
    , m_menuSelection(0)
    , m_bgmVolume(10)
    , m_sfxVolume(100)
    , m_gameState(GameState::Title)
    , m_difficulty(Difficulty::Normal)
    , m_titleSelection(1)
    , m_gameOverSelection(0)
    , m_continueCount(3)
{
}

Game::~Game() {
    Shutdown();
}

bool Game::Initialize(HWND hWnd, int width, int height) {
    m_hWnd = hWnd;
    m_width = width;
    m_height = height;

    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_lastTime);

    m_graphics = std::make_unique<Graphics>();
    if (!m_graphics->Initialize(hWnd, width, height)) {
        return false;
    }

    m_input = std::make_unique<Input>();
    if (!m_input->Initialize(hWnd)) {
        return false;
    }

    m_background = std::make_unique<Background3D>();
    m_background->Initialize(300);

    m_bulletManager = std::make_unique<BulletManager>();
    m_bulletManager->Initialize(m_graphics.get());

    m_player = std::make_unique<Player>();
    m_player->Initialize(m_graphics.get(), m_bulletManager.get(), m_sound.get());
    m_player->SetPosition(PLAY_AREA_WIDTH / 2.0f, PLAY_AREA_HEIGHT - 100.0f);

    m_enemyManager = std::make_unique<EnemyManager>();
    m_enemyManager->Initialize(m_graphics.get(), m_bulletManager.get());

    m_particles = std::make_unique<ParticleSystem>();
    m_particles->Initialize(500);

    m_items = std::make_unique<ItemManager>();
    m_items->Initialize(m_graphics.get());

    m_sound = std::make_unique<AudioManager>();
    m_sound->Initialize();

    m_bgm = std::make_unique<BGMPlayer>();
    m_bgm->Initialize();
    m_bgm->SetVolume(m_bgmVolume * 10);  // 75%で初期化
    m_bgm->PlayTitleBGM();  // タイトル画面BGM

    // Initialize text renderer with swap chain
    m_text = std::make_unique<TextRenderer>();
    m_text->Initialize(m_graphics->GetSwapChain());

    // Initialize texture loader and load title screen
    m_textureLoader = std::make_unique<TextureLoader>();
    m_textureLoader->Initialize(m_graphics->GetDevice());
    
    // Get executable path for textures
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring texturePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\title_screen.jpg";
    
    ID3D11ShaderResourceView* titleSRV = nullptr;
    if (m_textureLoader->LoadTexture(texturePath, &titleSRV)) {
        m_titleTexture.Attach(titleSRV);
    }

    m_isRunning = true;
    LoadHiScore();
    LoadCutinTextures();  // カットインテクスチャ読み込み
    LoadPortraits();      // ポートレートテクスチャ読み込み
    return true;
}

void Game::Shutdown() {
    SaveHiScore();
    if (m_items) m_items.reset();
    if (m_particles) m_particles.reset();
    if (m_background) m_background.reset();
    if (m_enemyManager) m_enemyManager.reset();
    if (m_player) m_player.reset();
    if (m_bulletManager) m_bulletManager.reset();
    if (m_input) m_input.reset();
    if (m_graphics) {
        m_graphics->Shutdown();
        m_graphics.reset();
    }
    m_isRunning = false;
}

void Game::Update() {
    UpdateDeltaTime();
    UpdateFade();  // フェード処理
    m_input->Update();
    
    // Handle game state
    switch (m_gameState) {
        case GameState::Title:
            UpdateTitle();
            return;
        case GameState::Playing:
            break;
        case GameState::Paused:
            UpdateSettingsMenu();
            return;
        case GameState::GameOver:
            UpdateGameOver();
            return;
        case GameState::StageClear:
            UpdateStageClear();
            return;
        case GameState::VictoryDialogue:
            UpdateVictoryDialogue();
            return;  // 後続処理をスキップ
    }
    
    // ESC to toggle pause/settings menu
    static bool escPressed = false;
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!escPressed) {
            m_gameState = GameState::Paused;
            m_menuSelection = 0;
        }
        escPressed = true;
    } else {
        escPressed = false;
    }
    
    HandleDebugInput();  // B=Boss, R=Reset, P=Power, G=Gauge
    
    // ボス会話中も背景は更新継続
    if (m_bossDialogueActive) {
        m_background->Update(m_deltaTime);
        m_particles->Update(m_deltaTime);
        UpdateBossDialogue();
        return;  // ゲームロジックはスキップ
    }
    
    m_background->Update(m_deltaTime);
    m_player->Update(m_input.get(), m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    m_enemyManager->SetPlayerPosition(m_player->GetPosition());
    m_enemyManager->Update(m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    
    // ボスウェーブ開始を検出して3秒ディレイ後にセリフ開始
    if (m_enemyManager->DidBossWaveJustStart() && !m_bossMode) {
        m_waitingForBoss = true;
        m_bossSpawnDelay = 0.0f;
        m_enemyManager->ClearBossWaveStartFlag();
    }
    
    // 10体撃破でボス登場（雑魚を即全滅させる）
    if (!m_bossMode && !m_waitingForBoss && m_killCount >= 10) {
        m_waitingForBoss = true;
        m_bossSpawnDelay = 0.0f;
        // 雑魚敵を全滅させる
        m_enemyManager->ClearNonBossEnemies();
    }
    
    // ボス登場前：2秒ディレイ処理
    if (m_waitingForBoss) {
        m_bossSpawnDelay += m_deltaTime;
        if (m_bossSpawnDelay >= 2.0f) {  // 2秒後にボス登場
            m_waitingForBoss = false;
            m_bossMode = true;
            m_bgm->Stop();
            m_bgm->PlayBossBGM();
            
            // ボスをスポーン！
            m_enemyManager->SpawnEnemy(320.0f, 150.0f, 500.0f, 3, EnemyType::Boss);
            
            StartBossDialogue();
        }
    }
    
    // ボス撃破時→勝利セリフ→ステージクリア
    if (m_enemyManager->IsBossWave() && m_enemyManager->AllEnemiesDead()) {
        if (m_score > m_hiScore) m_hiScore = m_score;
        m_victoryDialogueTimer = 0.0f;
        m_victoryDialogueLine = 0;
        m_gameState = GameState::VictoryDialogue;
    }
    
    // ホーミング用に敵の位置をBulletManagerに渡す
    std::vector<DirectX::XMFLOAT2> enemyPositions;
    for (const auto& enemy : m_enemyManager->GetEnemies()) {
        if (enemy->IsActive()) {
            enemyPositions.push_back(enemy->GetPosition());
        }
    }
    m_bulletManager->SetEnemyPositions(enemyPositions);
    
    m_bulletManager->Update(m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    m_particles->Update(m_deltaTime);
    m_items->Update(m_deltaTime, m_player->GetPosition(), PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    
    // ボス周りの禍々しいパーティクル（人魂風）
    if (m_bossMode) {
        for (const auto& enemy : m_enemyManager->GetEnemies()) {
            if (enemy->IsBoss() && enemy->IsActive()) {
                DirectX::XMFLOAT2 bossPos = enemy->GetPosition();
                static float particleTimer = 0.0f;
                particleTimer += m_deltaTime;
                if (particleTimer >= 0.08f) {  // 80msごとに発生
                    particleTimer = 0.0f;
                    // ランダムな位置に人魂パーティクル
                    float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
                    float radius = 70.0f + static_cast<float>(rand()) / RAND_MAX * 50.0f;
                    float px = bossPos.x + cosf(angle) * radius;
                    float py = bossPos.y + sinf(angle) * radius;
                    // 紫/赤の禍々しい色
                    float r = 0.6f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
                    float g = 0.0f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
                    float b = 0.4f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
                    m_particles->SpawnTrail(px, py, DirectX::XMFLOAT4(r, g, b, 0.8f));
                }
            }
        }
    }

    // Graze system
    UpdateGraze();
    
    // Collision detection
    CheckCollisions();
    
    // カットイン更新
    UpdateCutin();
    
    // ボス会話更新
    UpdateBossDialogue();

    // Combo timer
    if (m_comboTimer > 0) {
        m_comboTimer -= m_deltaTime;
        if (m_comboTimer <= 0) {
            m_combo = 0;
        }
    }

    // Collect items
    bool autoCollect = m_player->GetPosition().y < 200.0f; // Auto-collect at top
    auto collected = m_items->CollectItems(m_player->GetPosition(), 20.0f, autoCollect);
    
    // パワー取得→プレイヤーに渡して進化システム
    if (collected.power > 0) {
        m_player->AddPower(collected.power);
    }
    m_power = m_player->GetPower();  // UI表示用に同期
    
    m_score += collected.points;
    m_bombs += collected.bombs;
    m_lives += collected.lives;
    
    // Item collect sound
    if (collected.power > 0 || collected.points > 0) {
        m_sound->PlayItemCollect();
    }

    // Update hi-score
    if (m_score > m_hiScore) m_hiScore = m_score;
    
    // Bomb (X key)
    static bool bombPressed = false;
    if (GetAsyncKeyState('X') & 0x8000) {
        if (!bombPressed && m_bombs > 0) {
            m_bombs--;
            m_bulletManager->Clear();  // Clear all enemy bullets
            m_sound->PlayBomb();
            
            // Damage all enemies (ボス無敵時は除外)
            for (auto& enemy : m_enemyManager->GetEnemies()) {
                if (enemy->IsActive() && !enemy->IsInvincible()) {
                    enemy->TakeDamage(100.0f);
                }
            }
            
            // Spawn explosion particles
            m_particles->SpawnExplosion(m_player->GetPosition().x, m_player->GetPosition().y, 
                DirectX::XMFLOAT4(1.0f, 0.8f, 0.3f, 1.0f));
        }
        bombPressed = true;
    } else {
        bombPressed = false;
    }
}

void Game::UpdateGraze() {
    auto playerPos = m_player->GetPosition();
    auto& bullets = m_bulletManager->GetBullets();
    
    for (const auto& bullet : bullets) {
        if (!bullet.isActive || bullet.isPlayerBullet) continue;
        
        float dx = playerPos.x - bullet.position.x;
        float dy = playerPos.y - bullet.position.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        // Graze detection (close but not hit)
        if (dist < m_grazeRadius && dist > 5.0f) {
            m_graze++;
            m_score += 10;
            m_specialGauge += 0.5f;
            
            if (m_specialGauge >= m_maxSpecialGauge) {
                m_specialGauge = m_maxSpecialGauge;
                m_specialReady = true;
            }
        }
    }
}

void Game::CheckCollisions() {
    auto& bullets = m_bulletManager->GetBullets();
    auto playerPos = m_player->GetPosition();
    
    // Player bullets vs enemies
    for (auto& bullet : bullets) {
        if (!bullet.isActive || !bullet.isPlayerBullet) continue;
        
        for (auto& enemy : m_enemyManager->GetEnemies()) {
            if (!enemy->IsActive()) continue;
            
            float dx = bullet.position.x - enemy->GetPosition().x;
            float dy = bullet.position.y - enemy->GetPosition().y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < bullet.radius + enemy->GetRadius()) {
                // ボス無敵時は弾が通過
                if (enemy->IsInvincible()) continue;
                
                enemy->TakeDamage(10.0f);
                bullet.isActive = false;
                
                // Hit effect and sound
                m_particles->SpawnHitEffect(bullet.position.x, bullet.position.y,
                    DirectX::XMFLOAT4(1.0f, 0.9f, 0.5f, 1.0f));
                m_sound->PlayEnemyHit();
                
                // Combo and score
                m_combo++;
                m_comboTimer = 2.0f;
                int comboBonus = m_combo * 10;
                m_score += 100 + comboBonus;
                
                // Particles on hit
                m_particles->SpawnScorePopup(enemy->GetPosition().x, enemy->GetPosition().y, 100 + comboBonus);
                
                // If enemy died, spawn explosion and items
                if (!enemy->IsActive()) {
                    m_particles->SpawnExplosion(
                        enemy->GetPosition().x, 
                        enemy->GetPosition().y,
                        DirectX::XMFLOAT4(1.0f, 0.5f, 0.3f, 1.0f),
                        40
                    );
                    m_items->SpawnDrops(enemy->GetPosition().x, enemy->GetPosition().y, 1);
                    m_score += 500;
                    m_killCount++;  // 撃破カウント
                    m_sound->PlayEnemyDestroy();  // Play "Eyao!" voice
                }
                break;
            }
        }
    }
}

void Game::Render() {
    if (!m_graphics) return;

    m_graphics->BeginFrame();

    // Render title screen
    if (m_gameState == GameState::Title) {
        RenderTitle();
        m_graphics->EndFrame();
        return;
    }
    
    // Render game over screen
    if (m_gameState == GameState::GameOver) {
        RenderGameOver();
        m_graphics->EndFrame();
        return;
    }
    
    // Render stage clear screen
    if (m_gameState == GameState::StageClear) {
        RenderStageClear();
        m_graphics->EndFrame();
        return;
    }
    
    // VictoryDialogueはゲーム画面上にオーバーレイ（後で描画）
    
    // Render paused/settings menu
    if (m_gameState == GameState::Paused) {
        RenderSettingsMenu();
        m_graphics->EndFrame();
        return;
    }

    // Play area background
    m_graphics->DrawSprite(0, 0, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT,
        DirectX::XMFLOAT4(0.02f, 0.01f, 0.05f, 1.0f));

    m_background->Render(m_graphics.get());

    // Border
    m_graphics->DrawSprite(PLAY_AREA_WIDTH - 2, 0, 4, PLAY_AREA_HEIGHT,
        DirectX::XMFLOAT4(0.5f, 0.3f, 0.6f, 1.0f));

    // Game objects
    m_enemyManager->Render(m_graphics.get());
    m_bulletManager->Render(m_graphics.get());
    m_items->Render(m_graphics.get());
    m_player->Render(m_graphics.get());
    m_particles->Render(m_graphics.get());

    RenderUI();

    // Render settings menu if paused
    if (m_isPaused) {
        RenderSettingsMenu();
    }

    // Flush D3D before D2D text rendering
    m_graphics->GetContext()->Flush();

    // D2D text rendering (after D3D, before EndFrame)
    if (m_text) {
        m_text->BeginDraw();
        
        // UI Text labels (日本語化)
        float textX = static_cast<float>(PLAY_AREA_WIDTH + 20);
        m_text->DrawTextWithValue(L"ハイスコア", m_hiScore, textX, 50);
        m_text->DrawTextWithValue(L"スコア", m_score, textX, 90);
        
        // プレイヤー名表示
        const wchar_t* playerName = (m_playerCharacter == 0) ? L"★ ひなひな" : L"★ かい";
        m_text->DrawText(playerName, textX, 130, 200, 30, 1, 1);  // ゴールド
        
        m_text->DrawTextWithValue(L"残機", m_lives, textX, 170);
        m_text->DrawTextWithValue(L"ボム", m_bombs, textX, 210);
        m_text->DrawTextWithValue(L"パワー", m_power, textX, 250);
        m_text->DrawTextWithValue(L"バレル", m_graze, textX, 290);
        
        // FPS display
        wchar_t fpsBuffer[32];
        swprintf_s(fpsBuffer, L"FPS: %.1f", m_currentFPS);
        m_text->DrawText(fpsBuffer, textX, 410, 200, 30, 1, m_currentFPS >= 60 ? 0 : 1);
        
        // ボススペルカード名（プレイエリア上部に表示）
        if (!m_currentBossSpellName.empty()) {
            m_text->DrawText(m_currentBossSpellName.c_str(), 20.0f, 18.0f, 
                static_cast<float>(PLAY_AREA_WIDTH - 100), 30, 1, 1);  // ゴールド文字
        }
        
        // Build info
        m_text->DrawText(L"LoC: 4231", textX, static_cast<float>(PLAY_AREA_HEIGHT - 60), 200, 20, 0, 2);
        m_text->DrawText(L"ひなた vs ひなひな", textX, static_cast<float>(PLAY_AREA_HEIGHT - 35), 200, 20, 0, 1);
        
        m_text->EndDraw();
    }
    
    // カットイン描画（UIの上に表示）
    RenderCutin();
    
    // ボス会話描画
    RenderBossDialogue();
    
    // 勝利セリフオーバーレイ（ゲーム画面の上に表示）
    if (m_gameState == GameState::VictoryDialogue) {
        RenderVictoryDialogue();
    }

    // フェード効果を最後に描画
    RenderFade();

    m_graphics->EndFrame();
}

void Game::RenderUI() {
    int uiX = PLAY_AREA_WIDTH + 20;
    int uiY = 50;
    int lineHeight = 40;
    
    // ボス体力バー（プレイエリア上部に表示）
    for (const auto& enemy : m_enemyManager->GetEnemies()) {
        if (enemy->IsBoss() && enemy->IsActive()) {
            float barX = 20.0f;
            float barY = 50.0f;  // スペルカード名の下に配置
            float barWidth = PLAY_AREA_WIDTH - 40.0f;
            float barHeight = 16.0f;
            float healthPercent = enemy->GetDisplayHealthPercent();  // イージング適用版
            
            // ボス残機（★マーク）を右上に表示
            int remaining = enemy->GetRemainingSpells();
            for (int i = 0; i < remaining; i++) {
                float starX = PLAY_AREA_WIDTH - 30.0f - (i * 22.0f);
                m_graphics->DrawGlowCircle(starX, 28.0f, 8.0f,
                    DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f), 2);
            }
            
            // スペルカード名を保存（ボス名 + スペルカード名）
            std::wstring spellName = L"ひなひな ▸ ";
            spellName += enemy->GetSpellCardName();
            m_currentBossSpellName = spellName;
            m_bossRemainingSpells = remaining;
            
            // 背景
            m_graphics->DrawSprite(barX, barY, barWidth, barHeight,
                DirectX::XMFLOAT4(0.2f, 0.1f, 0.1f, 0.8f));
            // 体力バー（スペルごとに色変更）
            DirectX::XMFLOAT4 hpColor;
            switch (enemy->GetCurrentSpell()) {
                case 0: hpColor = { 0.9f, 0.6f, 0.2f, 1.0f }; break;  // 琥珀
                case 1: hpColor = { 0.8f, 0.3f, 0.8f, 1.0f }; break;  // 紫
                case 2: hpColor = { 0.3f, 0.8f, 0.9f, 1.0f }; break;  // シアン
                case 3: hpColor = { 0.9f, 0.2f, 0.2f, 1.0f }; break;  // 赤
                default: hpColor = { 0.9f, 0.2f, 0.2f, 1.0f }; break;
            }
            m_graphics->DrawSprite(barX, barY, barWidth * healthPercent, barHeight, hpColor);
            // 枠
            m_graphics->DrawSprite(barX - 2, barY - 2, barWidth + 4, 2,
                DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f));
            m_graphics->DrawSprite(barX - 2, barY + barHeight, barWidth + 4, 2,
                DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f));
            break;
        } else {
            m_currentBossSpellName = L"";
            m_bossRemainingSpells = 0;
        }
    }

    // Sidebar background
    m_graphics->DrawSprite(PLAY_AREA_WIDTH, 0, SIDEBAR_WIDTH, PLAY_AREA_HEIGHT,
        DirectX::XMFLOAT4(0.1f, 0.05f, 0.15f, 1.0f));

    // Hi-Score
    m_graphics->DrawSprite(uiX, uiY, 150, 25,
        DirectX::XMFLOAT4(0.8f, 0.7f, 0.2f, 0.3f));
    uiY += lineHeight;

    // Score
    m_graphics->DrawSprite(uiX, uiY, 200, 30,
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f));
    uiY += lineHeight * 2;

    // Lives
    for (int i = 0; i < m_lives; i++) {
        m_graphics->DrawGlowCircle(uiX + 20 + i * 35, uiY, 12,
            DirectX::XMFLOAT4(1.0f, 0.3f, 0.5f, 1.0f), 2);
    }
    uiY += lineHeight;

    // Bombs
    for (int i = 0; i < m_bombs; i++) {
        m_graphics->DrawGlowCircle(uiX + 20 + i * 35, uiY, 10,
            DirectX::XMFLOAT4(0.3f, 1.0f, 0.5f, 1.0f), 2);
    }
    uiY += lineHeight * 2;

    // Power bar
    float powerRatio = static_cast<float>(m_power) / m_maxPower;
    m_graphics->DrawSprite(uiX, uiY, 200, 20,
        DirectX::XMFLOAT4(0.2f, 0.1f, 0.3f, 0.8f));
    m_graphics->DrawSprite(uiX, uiY, 200 * powerRatio, 20,
        DirectX::XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f));
    uiY += lineHeight;

    // Special gauge
    float specialRatio = m_specialGauge / m_maxSpecialGauge;
    m_graphics->DrawSprite(uiX, uiY, 200, 25,
        DirectX::XMFLOAT4(0.1f, 0.1f, 0.2f, 0.8f));
    DirectX::XMFLOAT4 specialColor = m_specialReady 
        ? DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f)
        : DirectX::XMFLOAT4(0.3f, 0.5f, 1.0f, 1.0f);
    m_graphics->DrawSprite(uiX, uiY, 200 * specialRatio, 25, specialColor);
    uiY += lineHeight;

    // Graze count
    m_graphics->DrawSprite(uiX, uiY, 100, 20,
        DirectX::XMFLOAT4(0.5f, 0.5f, 0.8f, 0.5f));
    uiY += lineHeight;

    // Combo (removed growing circle per user request)
    uiY += lineHeight * 2;
    
    // FPS display (bottom of sidebar)
    float fpsBarWidth = fminf(200.0f, m_currentFPS * 3.0f);
    DirectX::XMFLOAT4 fpsColor = m_currentFPS >= 60.0f 
        ? DirectX::XMFLOAT4(0.2f, 1.0f, 0.3f, 0.8f)   // Green for 60+
        : m_currentFPS >= 30.0f 
            ? DirectX::XMFLOAT4(1.0f, 0.8f, 0.2f, 0.8f) // Yellow for 30-60
            : DirectX::XMFLOAT4(1.0f, 0.2f, 0.2f, 0.8f); // Red for <30
    
    m_graphics->DrawSprite(uiX, uiY, 200, 15,
        DirectX::XMFLOAT4(0.1f, 0.1f, 0.2f, 0.6f));
    m_graphics->DrawSprite(uiX, uiY, fpsBarWidth, 15, fpsColor);
    uiY += lineHeight;
    
    // Build info at bottom of sidebar
    int bottomY = PLAY_AREA_HEIGHT - 80;
    
    // Build timestamp
    m_graphics->DrawSprite(uiX, bottomY, 250, 20,
        DirectX::XMFLOAT4(0.15f, 0.1f, 0.2f, 0.6f));
    bottomY += 25;
    
    // LoC display (approximately 2100 lines)
    m_graphics->DrawSprite(uiX, bottomY, 180, 20,
        DirectX::XMFLOAT4(0.2f, 0.15f, 0.25f, 0.5f));
    bottomY += 25;
    
    // Character names
    m_graphics->DrawSprite(uiX, bottomY, 200, 20,
        DirectX::XMFLOAT4(0.8f, 0.6f, 0.3f, 0.4f));
}

void Game::UpdateDeltaTime() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    m_deltaTime = static_cast<float>(currentTime.QuadPart - m_lastTime.QuadPart) / 
                  static_cast<float>(m_frequency.QuadPart);
    m_lastTime = currentTime;
    if (m_deltaTime > 0.1f) m_deltaTime = 0.1f;
    
    // FPS calculation
    m_frameCount++;
    m_fpsTimer += m_deltaTime;
    if (m_fpsTimer >= 1.0f) {
        m_currentFPS = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }
}

void Game::HandleDebugInput() {
    static bool bPressed = false, rPressed = false, pPressed = false, gPressed = false;
    
    // Press B to spawn boss
    if (GetAsyncKeyState('B') & 0x8000) {
        if (!bPressed) {
            SpawnBoss();
        }
        bPressed = true;
    } else { bPressed = false; }
    
    // Press R to reset game
    if (GetAsyncKeyState('R') & 0x8000) {
        if (!rPressed) {
            m_score = 0;
            m_lives = 3;
            m_bombs = 3;
            m_power = 0;
            m_specialGauge = 0;
            m_combo = 0;
            m_graze = 0;
            m_bulletManager->Clear();
            m_items->Clear();
            m_bossMode = false;
        }
        rPressed = true;
    } else { rPressed = false; }
    
    // Press P for evolution level up（装備進化）
    if (GetAsyncKeyState('P') & 0x8000) {
        if (!pPressed) {
            int currentLevel = m_player->GetEvolutionLevel();
            if (currentLevel < 4) {
                m_player->SetEvolutionLevel(currentLevel + 1);
            }
        }
        pPressed = true;
    } else { pPressed = false; }
    
    // Press G to fill special gauge
    if (GetAsyncKeyState('G') & 0x8000) {
        if (!gPressed) {
            m_specialGauge = m_maxSpecialGauge;
            m_specialReady = true;
        }
        gPressed = true;
    } else { gPressed = false; }
    
    // Press K to damage boss (one spell card)
    static bool kPressed = false;
    if (GetAsyncKeyState('K') & 0x8000) {
        if (!kPressed && m_bossMode) {
            // ボスに大ダメージ（スペルカード1枚分）
            m_enemyManager->DamageBoss(100.0f);
        }
        kPressed = true;
    } else { kPressed = false; }
    
    // Press C to change character（ひなひな⇔かい）
    static bool cPressed = false;
    if (GetAsyncKeyState('C') & 0x8000) {
        if (!cPressed) {
            m_playerCharacter = (m_playerCharacter + 1) % 2;  // 0⇔1
            // プレイヤーテクスチャを切り替え（将来実装）
        }
        cPressed = true;
    } else { cPressed = false; }
}

void Game::SpawnBoss() {
    m_bossMode = true;
    m_bulletManager->Clear();
    m_enemyManager->Clear();  // 雑魚クリア
    m_enemyManager->ResetWaves();  // ウェーブをリセット
    
    // ボスBGMに切り替え
    m_bgm->Stop();
    m_bgm->PlayBossBGM();
    
    // 最終ボス「ひなひな」を生成（体力800、4スペルカード完備）
    m_enemyManager->SpawnEnemy(PLAY_AREA_WIDTH / 2.0f, 150.0f, 800.0f, 0, EnemyType::Boss);
    
    // ボス会話開始！
    StartBossDialogue();
}

void Game::UpdateSettingsMenu() {
    // Menu navigation
    static bool upPressed = false, downPressed = false;
    static bool leftPressed = false, rightPressed = false;
    static bool escPressed = false;
    static float escCooldown = 0.0f;
    
    // ESC連打防止（0.3秒クールダウン）
    if (escCooldown > 0.0f) escCooldown -= m_deltaTime;
    
    // ESC to close and resume
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!escPressed && escCooldown <= 0.0f) {
            m_gameState = GameState::Playing;
            escCooldown = 0.3f;
        }
        escPressed = true;
    } else { escPressed = false; }
    
    // Up/Down to select menu item (5項目に変更)
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        if (!upPressed) {
            m_menuSelection = (m_menuSelection - 1 + 5) % 5;
        }
        upPressed = true;
    } else { upPressed = false; }
    
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        if (!downPressed) {
            m_menuSelection = (m_menuSelection + 1) % 5;
        }
        downPressed = true;
    } else { downPressed = false; }
    
    // Left/Right to adjust values
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        if (!leftPressed) {
            if (m_menuSelection == 0) {
                m_bgmVolume = max(0, m_bgmVolume - 10);
                m_bgm->SetVolume(m_bgmVolume * 10);
            } else if (m_menuSelection == 1) {
                m_sfxVolume = max(0, m_sfxVolume - 10);
                if (m_sound) m_sound->SetVolume(m_sfxVolume / 100.0f);
            }
        }
        leftPressed = true;
    } else { leftPressed = false; }
    
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        if (!rightPressed) {
            if (m_menuSelection == 0) {
                m_bgmVolume = min(100, m_bgmVolume + 10);
                m_bgm->SetVolume(m_bgmVolume * 10);
            } else if (m_menuSelection == 1) {
                m_sfxVolume = min(100, m_sfxVolume + 10);
                if (m_sound) m_sound->SetVolume(m_sfxVolume / 100.0f);
            }
        }
        rightPressed = true;
    } else { rightPressed = false; }
    
    // Z or マウス左クリック to confirm selection
    static bool zPressed = false;
    bool isZorClick = (GetAsyncKeyState('Z') & 0x8000) || (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    if (isZorClick) {
        if (!zPressed) {
            if (m_menuSelection == 2) {
                m_gameState = GameState::Playing;
            } else if (m_menuSelection == 3) {
                ResetGame();
                m_gameState = GameState::Title;
            } else if (m_menuSelection == 4) {
                m_isRunning = false;
            }
        }
        zPressed = true;
    } else { zPressed = false; }
}

void Game::RenderSettingsMenu() {
    // Semi-transparent overlay
    m_graphics->DrawSprite(0, 0, m_width, m_height,
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.7f));
    
    float menuX = m_width / 2.0f - 150;
    float menuY = m_height / 2.0f - 150;
    float itemHeight = 60;
    
    // Menu background (5項目用に拡大)
    m_graphics->DrawSprite(menuX - 20, menuY - 20, 340, 360,
        DirectX::XMFLOAT4(0.1f, 0.05f, 0.15f, 0.9f));
    
    // Title bar
    m_graphics->DrawSprite(menuX - 20, menuY - 20, 340, 40,
        DirectX::XMFLOAT4(0.6f, 0.3f, 0.2f, 1.0f));
    
    // Menu items
    const char* items[] = { "BGM Volume", "SFX Volume", "Resume", "Quit" };
    int values[] = { m_bgmVolume, m_sfxVolume, -1, -1 };
    
    for (int i = 0; i < 4; i++) {
        float y = menuY + 40 + i * itemHeight;
        
        // Selection highlight
        DirectX::XMFLOAT4 color = (i == m_menuSelection) 
            ? DirectX::XMFLOAT4(0.8f, 0.6f, 0.3f, 1.0f)
            : DirectX::XMFLOAT4(0.4f, 0.3f, 0.5f, 0.8f);
        
        m_graphics->DrawSprite(menuX, y, 300, 45, color);
        
        // Value bar for volume items
        if (values[i] >= 0) {
            float barWidth = 200.0f * (values[i] / 100.0f);
            m_graphics->DrawSprite(menuX + 80, y + 25, 200, 12,
                DirectX::XMFLOAT4(0.2f, 0.2f, 0.3f, 1.0f));
            m_graphics->DrawSprite(menuX + 80, y + 25, barWidth, 12,
                DirectX::XMFLOAT4(0.9f, 0.7f, 0.3f, 1.0f));
        }
    }
    
    // Draw text labels
    m_graphics->GetContext()->Flush();
    if (m_text) {
        m_text->BeginDraw();
        
        // Title (日本語)
        m_text->DrawText(L"ポーズ", menuX + 100, menuY - 15, 140, 35, 2, 1);
        
        // Menu items (日本語)
        const wchar_t* labels[] = { L"BGM音量", L"効果音", L"ゲームに戻る", L"タイトルに戻る", L"終了" };
        for (int i = 0; i < 5; i++) {
            float y = menuY + 45 + i * itemHeight;
            int textColor = (i == m_menuSelection) ? 1 : 0;
            m_text->DrawText(labels[i], menuX + 10, y, 180, 35, 1, textColor);
            
            // Show volume percentage
            if (i < 2) {
                wchar_t volText[16];
                swprintf_s(volText, L"%d%%", values[i]);
                m_text->DrawText(volText, menuX + 240, y, 50, 35, 1, textColor);
            }
        }
        
        m_text->EndDraw();
    }
}

void Game::UpdateFade() {
    const float fadeSpeed = 0.5f;  // 2秒でフェード完了
    
    if (m_fadeIn) {
        m_fadeAlpha -= fadeSpeed * m_deltaTime;
        if (m_fadeAlpha <= 0.0f) {
            m_fadeAlpha = 0.0f;
            m_fadeIn = false;
        }
    }
    
    if (m_fadeOut) {
        m_fadeAlpha += fadeSpeed * m_deltaTime;
        if (m_fadeAlpha >= 1.0f) {
            m_fadeAlpha = 1.0f;
            m_fadeOut = false;
            m_fadeIn = true;  // 次の画面でフェードイン
            m_gameState = m_nextState;
            
            // 状態に応じてBGM切り替え
            if (m_nextState == GameState::Playing) {
                ResetGame();
            } else if (m_nextState == GameState::Title) {
                if (m_bgm) m_bgm->PlayTitleBGM();
            }
        }
    }
}

void Game::RenderFade() {
    if (m_fadeAlpha > 0.001f) {
        m_graphics->DrawSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
            DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, m_fadeAlpha));
    }
}

void Game::ResetGame() {
    // プレイヤー初期化
    m_player->SetPosition(static_cast<float>(PLAY_AREA_WIDTH / 2), static_cast<float>(PLAY_AREA_HEIGHT - 100));
    m_player->SetPower(0);
    m_player->SetEvolutionLevel(0);
    
    // 弾クリア
    m_bulletManager->Clear();
    
    // 敵クリアとウェーブリセット
    m_enemyManager->Clear();
    m_enemyManager->ResetWaves();
    
    // アイテムクリア
    m_items->Clear();
    
    // パーティクルクリア
    m_particles->Clear();
    
    // ゲーム状態リセット
    m_score = 0;
    m_lives = 3;
    m_bombs = 3;
    m_graze = 0;
    m_killCount = 0;  // 撃破カウントリセット
    m_specialGauge = 0.0f;
    m_bossMode = false;
    m_bossDialogueActive = false;
    m_waitingForBoss = false;
    m_bossSpawnDelay = 0.0f;
    m_victoryDialogueTimer = 0.0f;
    m_victoryDialogueLine = 0;
    
    // BGMをステージBGMに切り替え
    if (m_bgm) {
        m_bgm->PlayStageBGM();
    }
}

void Game::UpdateTitle() {
    static bool zPressed = false, leftPressed = false, rightPressed = false;
    
    // Left/Right to select difficulty
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        if (!leftPressed) {
            m_titleSelection = (m_titleSelection - 1 + 3) % 3;
            if (m_sound) m_sound->PlayCursor();
        }
        leftPressed = true;
    } else { leftPressed = false; }
    
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        if (!rightPressed) {
            m_titleSelection = (m_titleSelection + 1) % 3;
            if (m_sound) m_sound->PlayCursor();
        }
        rightPressed = true;
    } else { rightPressed = false; }
    
    // Z key to start game with selected difficulty (フェードアウト開始)
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed && !m_fadeOut) {
            if (m_sound) m_sound->PlayConfirm();  // 決定音
            m_difficulty = static_cast<Difficulty>(m_titleSelection);
            m_nextState = GameState::Playing;
            m_fadeOut = true;  // フェードアウト開始
        }
        zPressed = true;
    } else { zPressed = false; }
    
    // ESC to quit
    if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) {
        m_isRunning = false;
    }
}

void Game::RenderTitle() {
    // Draw title screen background (full screen)
    if (m_titleTexture) {
        m_graphics->DrawTexturedSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
            m_titleTexture.Get(), DirectX::XMFLOAT4(1, 1, 1, 1));
    } else {
        // Fallback: gradient background
        m_graphics->DrawSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
            DirectX::XMFLOAT4(0.2f, 0.1f, 0.05f, 1.0f));
    }
    
    // Flush D3D commands before D2D rendering
    m_graphics->GetContext()->Flush();
    
    if (m_text) {
        m_text->BeginDraw();
        
        // Difficulty selection（日本語化）
        const wchar_t* difficulties[] = { L"かんたん", L"ふつう", L"むずい" };
        float diffY = m_height - 280.0f;
        float centerX = m_width / 2.0f;
        
        for (int i = 0; i < 3; i++) {
            float x = centerX - 170.0f + i * 140.0f;
            int colorType = (i == m_titleSelection) ? 1 : 0;  // Gold for selected, white otherwise
            m_text->DrawText(difficulties[i], x, diffY, 120, 40, 2, colorType);
        }
        
        // Selection arrows
        m_text->DrawText(L"◀", centerX - 220.0f, diffY, 40, 40, 2, 1);
        m_text->DrawText(L"▶", centerX + 200.0f, diffY, 40, 40, 2, 1);
        
        // "Press Z to Start" fading text - centered（日本語化）
        float time = static_cast<float>(GetTickCount64()) * 0.003f;
        float alpha = (sinf(time) + 1.0f) * 0.5f;
        
        std::wstring startText = L"Zキーでスタート";
        float textWidth = static_cast<float>(m_width);  // 画面幅全体
        float textX = 0.0f;  // 左端から
        float textY = m_height - 180.0f;
        m_text->DrawTextWithAlpha(startText, textX, textY, textWidth, 80, 3, 1, alpha);
        
        m_text->EndDraw();
    }
}

float Game::GetBulletSpeedMultiplier() const {
    switch (m_difficulty) {
        case Difficulty::Easy: return 0.7f;
        case Difficulty::Normal: return 1.0f;
        case Difficulty::Hard: return 1.3f;
        default: return 1.0f;
    }
}

float Game::GetEnemyBulletCountMultiplier() const {
    switch (m_difficulty) {
        case Difficulty::Easy: return 0.6f;
        case Difficulty::Normal: return 1.0f;
        case Difficulty::Hard: return 1.5f;
        default: return 1.0f;
    }
}

void Game::UpdateGameOver() {
    static bool upPressed = false, downPressed = false, zPressed = false;
    static float gameOverTimer = 0.0f;
    
    // 2秒待機してからスキップ可能
    gameOverTimer += m_deltaTime;
    if (gameOverTimer < 2.0f) return;
    
    // Up/Down to select option
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        if (!upPressed) m_gameOverSelection = (m_gameOverSelection - 1 + 2) % 2;
        upPressed = true;
    } else { upPressed = false; }
    
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        if (!downPressed) m_gameOverSelection = (m_gameOverSelection + 1) % 2;
        downPressed = true;
    } else { downPressed = false; }
    
    // Z to confirm
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed) {
            if (m_gameOverSelection == 0 && m_continueCount > 0) {
                // Continue
                m_continueCount--;
                m_lives = 3;
                m_bombs = 3;
                m_gameState = GameState::Playing;
                gameOverTimer = 0.0f;  // リセット
            } else {
                // Return to title
                ResetGame();
                m_gameState = GameState::Title;
                gameOverTimer = 0.0f;  // リセット
            }
        }
        zPressed = true;
    } else { zPressed = false; }
}

void Game::RenderGameOver() {
    // Dark overlay
    m_graphics->DrawSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.8f));
    
    m_graphics->GetContext()->Flush();
    
    if (m_text) {
        m_text->BeginDraw();
        
        float centerX = m_width / 2.0f;
        float centerY = m_height / 2.0f;
        
        // Game Over text
        m_text->DrawText(L"GAME OVER", centerX - 150, centerY - 120, 300, 60, 3, 1);
        
        // Final score
        wchar_t scoreText[64];
        swprintf_s(scoreText, L"Score: %d", m_score);
        m_text->DrawText(scoreText, centerX - 100, centerY - 40, 200, 40, 2, 0);
        
        // Continue option
        const wchar_t* continueText = m_continueCount > 0 ? L"Continue" : L"No Credits";
        int continueColor = (m_gameOverSelection == 0 && m_continueCount > 0) ? 1 : 2;
        m_text->DrawText(continueText, centerX - 80, centerY + 40, 200, 40, 2, continueColor);
        
        // Credits remaining
        if (m_continueCount > 0) {
            wchar_t creditText[32];
            swprintf_s(creditText, L"(%d left)", m_continueCount);
            m_text->DrawText(creditText, centerX + 60, centerY + 40, 100, 40, 1, 2);
        }
        
        // Return to title option
        int titleColor = (m_gameOverSelection == 1) ? 1 : 0;
        m_text->DrawText(L"Return to Title", centerX - 100, centerY + 90, 200, 40, 2, titleColor);
        
        m_text->EndDraw();
    }
}

void Game::UpdateStageClear() {
    static bool zPressed = false;
    static float stageClearTimer = 0.0f;
    
    // 2秒待機してからスキップ可能
    stageClearTimer += m_deltaTime;
    if (stageClearTimer < 2.0f) return;
    
    // Z to return to title
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed) {
            ResetGame();
            m_gameState = GameState::Title;
            stageClearTimer = 0.0f;  // リセット
        }
        zPressed = true;
    } else { zPressed = false; }
}

void Game::RenderStageClear() {
    // ステージクリアイラストを背景として表示（フルスクリーン）
    if (m_stageClearTexture) {
        m_graphics->DrawTexturedSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
            m_stageClearTexture.Get(), DirectX::XMFLOAT4(1, 1, 1, 1));
    } else {
        // フォールバック：ゴールドのオーバーレイ
        m_graphics->DrawSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
            DirectX::XMFLOAT4(0.1f, 0.08f, 0.0f, 0.8f));
    }
    
    m_graphics->GetContext()->Flush();
    
    if (m_text) {
        m_text->BeginDraw();
        
        float centerX = m_width / 2.0f;
        float centerY = m_height / 2.0f;
        
        // Stage Clear text（日本語化）
        m_text->DrawText(L"ステージクリア！", centerX - 200, centerY - 150, 400, 70, 3, 1);
        
        // Final score
        wchar_t scoreText[64];
        swprintf_s(scoreText, L"スコア: %d", m_score);
        m_text->DrawText(scoreText, centerX - 120, centerY - 40, 240, 40, 2, 0);
        
        // Hi-Score
        if (m_score >= m_hiScore) {
            m_text->DrawText(L"★ ハイスコア更新！ ★", centerX - 150, centerY + 20, 300, 40, 2, 1);
        }
        
        // Press Z to continue（日本語化）
        float time = static_cast<float>(GetTickCount64()) * 0.003f;
        float alpha = (sinf(time) + 1.0f) * 0.5f;
        m_text->DrawTextWithAlpha(L"Zキーでタイトルへ", centerX - 130, centerY + 100, 260, 40, 2, 0, alpha);
        
        m_text->EndDraw();
    }
}

void Game::UpdateVictoryDialogue() {
    // 背景とパーティクルは更新継続
    m_background->Update(m_deltaTime);
    m_particles->Update(m_deltaTime);
    
    m_victoryDialogueTimer += m_deltaTime;
    
    // 5秒間セリフ表示後、StageClearへ
    if (m_victoryDialogueTimer > 5.0f) {
        m_gameState = GameState::StageClear;
        if (m_bgm) {
            m_bgm->Stop();
            m_bgm->PlayScoreBGM();  // スコア画面BGM
        }
        return;
    }
    
    // Zキー or ENTERキーでスキップ（1秒後から有効）
    if (m_victoryDialogueTimer > 1.0f) {
        bool zPressed = (GetAsyncKeyState('Z') & 0x8000) != 0;
        bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        
        if (zPressed || enterPressed) {
            m_gameState = GameState::StageClear;
            if (m_bgm) {
                m_bgm->Stop();
                m_bgm->PlayScoreBGM();  // スコア画面BGM
            }
        }
    }
}

void Game::RenderVictoryDialogue() {
    // 背景を少し暗く
    m_graphics->DrawSprite(0, 0, static_cast<float>(PLAY_AREA_WIDTH), static_cast<float>(PLAY_AREA_HEIGHT),
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.4f));
    
    // セリフウィンドウ
    float windowY = PLAY_AREA_HEIGHT - 200.0f;
    m_graphics->DrawSprite(20, windowY, PLAY_AREA_WIDTH - 40, 180,
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.3f, 0.9f));
    m_graphics->DrawSprite(25, windowY + 5, PLAY_AREA_WIDTH - 50, 170,
        DirectX::XMFLOAT4(0.1f, 0.1f, 0.4f, 0.9f));
    
    // かいポートレート
    if (m_portraitKai) {
        m_graphics->DrawTexturedSprite(40, windowY + 20, 140, 140, m_portraitKai.Get());
    }
    
    // 勝利セリフ - かい画像とかいのセリフ
    m_text->BeginDraw();
    const wchar_t* victoryLine = L"かい「ほいじゃ、また見てね」";
    
    m_text->DrawText(victoryLine, 200, windowY + 50, 400, 120, 3, 0);  // サイズ3で大きく
    
    // スキップヒント
    float alpha = (sinf(m_victoryDialogueTimer * 4.0f) + 1.0f) * 0.5f;
    m_text->DrawTextWithAlpha(L"Zキーでスキップ", PLAY_AREA_WIDTH - 200, windowY + 150, 160, 30, 1, 0, alpha);
    
    m_text->EndDraw();
}


void Game::SaveHiScore() {
    std::ofstream file("hiscore.dat", std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&m_hiScore), sizeof(m_hiScore));
        file.close();
    }
}

void Game::LoadHiScore() {
    std::ifstream file("hiscore.dat", std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&m_hiScore), sizeof(m_hiScore));
        file.close();
    }
}

// カットインテクスチャ読み込み
void Game::LoadCutinTextures() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring basePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\";
    
    const wchar_t* cutinFiles[] = {
        L"cutin_spell1.png",
        L"cutin_spell2.png",
        L"cutin_spell3.png",
        L"cutin_spell4.png",
        L"cutin_spell5.png"
    };
    
    for (int i = 0; i < 5; i++) {
        std::wstring texturePath = basePath + cutinFiles[i];
        ID3D11ShaderResourceView* srv = nullptr;
        if (m_textureLoader->LoadTexture(texturePath, &srv)) {
            m_cutinTextures[i].Attach(srv);
        }
    }
}

// ポートレートテクスチャ読み込み
void Game::LoadPortraits() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring basePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\portraits\\";
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (m_textureLoader->LoadTexture(basePath + L"hinata.png", &srv)) {
        m_portraitHinata.Attach(srv);
    }
    srv = nullptr;
    if (m_textureLoader->LoadTexture(basePath + L"kai.png", &srv)) {
        m_portraitKai.Attach(srv);
    }
    
    // ステージクリアイラスト読み込み
    std::wstring uiPath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\ui\\";
    srv = nullptr;
    if (m_textureLoader->LoadTexture(uiPath + L"stage_clear.png", &srv)) {
        m_stageClearTexture.Attach(srv);
    }
}

// カットイン更新（毎フレーム呼び出し）
void Game::UpdateCutin() {
    // ボスがカットインを表示中か確認
    for (const auto& enemy : m_enemyManager->GetEnemies()) {
        if (enemy->IsBoss() && enemy->IsActive() && enemy->IsShowingCutin()) {
            int spellIndex = enemy->GetCurrentSpell();
            if (spellIndex >= 0 && spellIndex < 5) {
                m_currentCutinIndex = spellIndex;
                m_cutinTimer = 1.5f;  // 1.5秒間表示
                enemy->ClearCutin();  // フラグをクリア
                
                // スペルカード切り替え時に敵弾消し
                m_bulletManager->Clear();
                
                // スペルカード切り替えSE
                if (m_sound) {
                    m_sound->PlayEnemyDestroy();  // スペル切替時に派手な音
                }
            }
        }
    }
    
    // タイマー減少
    if (m_cutinTimer > 0.0f) {
        m_cutinTimer -= m_deltaTime;
        if (m_cutinTimer <= 0.0f) {
            m_currentCutinIndex = -1;  // カットイン終了
        }
    }
}

// カットイン描画
void Game::RenderCutin() {
    if (m_currentCutinIndex < 0 || m_currentCutinIndex >= 5) return;
    if (!m_cutinTextures[m_currentCutinIndex]) return;
    
    // タイマー進行（0→1.5秒）
    float t = 1.5f - m_cutinTimer;  // 0→1.5に変換
    
    // フェードイン・アウト計算（透明度80%）
    float alpha = 0.8f;
    if (t < 0.2f) {
        alpha = (t / 0.2f) * 0.8f;  // フェードイン
    } else if (t > 1.3f) {
        alpha = ((1.5f - t) / 0.2f) * 0.8f;  // フェードアウト
    }
    
    // 拡大アニメーション（登場時に大きくなる）
    float scale = 1.0f;
    if (t < 0.15f) {
        // イーズアウト：1.3→1.0に縮小
        float p = t / 0.15f;
        scale = 1.3f - 0.3f * (p * p);  // 二次イージング
    }
    
    // 画面全体に大きく表示
    float baseWidth = static_cast<float>(PLAY_AREA_WIDTH);
    float baseHeight = static_cast<float>(PLAY_AREA_HEIGHT);
    float cutinWidth = baseWidth * scale;
    float cutinHeight = baseHeight * scale;
    float x = (PLAY_AREA_WIDTH - cutinWidth) / 2.0f;
    float y = (PLAY_AREA_HEIGHT - cutinHeight) / 2.0f;
    
    // フラッシュ効果（登場時に白フラッシュ）
    if (t < 0.08f) {
        float flashAlpha = 1.0f - (t / 0.08f);
        m_graphics->DrawSprite(0, 0, static_cast<float>(PLAY_AREA_WIDTH), static_cast<float>(PLAY_AREA_HEIGHT),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, flashAlpha * 0.8f));
    }
    
    // カットイン画像描画
    m_graphics->DrawTexturedSprite(x, y, cutinWidth, cutinHeight,
        m_cutinTextures[m_currentCutinIndex].Get(),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, alpha));
    
    // キラキラ効果（1.5秒版）
    if (t > 0.2f && t < 1.3f) {
        for (int i = 0; i < 3; i++) {
            float sparkleT = fmodf(t * 3.0f + i * 0.5f, 1.0f);
            float sparkleAlpha = sinf(sparkleT * 3.14159f);
            float sparkleX = fmodf((t * 200.0f + i * 150.0f), static_cast<float>(PLAY_AREA_WIDTH));
            float sparkleY = fmodf((t * 180.0f + i * 220.0f), static_cast<float>(PLAY_AREA_HEIGHT));
            
            m_graphics->DrawGlowCircle(sparkleX, sparkleY, 15.0f + sparkleT * 10.0f,
                DirectX::XMFLOAT4(1.0f, 1.0f, 0.8f, sparkleAlpha * 0.5f), 3);
        }
    }
}

// Boss dialogue (Hinahina vs Kai - Touhou style)
static const wchar_t* g_bossDialogues[] = {
    L"ひなひな「だれ？」",
    L"かい「おや、ひなひなさんじゃありませんか」",
    L"ひなひな「すごいよかいさん、汁まみれだよ」",
    L"かい「ええ　旧Aoの怨念で汁が大量発生しているわ」",
    L"ひなひな「平将門みたいだねえ」",
    L"かい「旧Aoを除霊しにいくわよ」",
    L"ひなひな「やだねえ」",
    L"かい「はあ」",
    L"ひなひな「ここで一生汁まみれになってドンキーコングでエーヤオ」",
    L"かい「おやりになってますわね」",
    L"ひなひな「かいさんだからって通すわけにはいかないねえ」"
};
static const int g_numDialogues = 11;

// ボス会話開始
void Game::StartBossDialogue() {
    m_bossDialogueActive = true;
    m_dialogueLine = 0;
    m_dialogueTimer = 0.0f;
    m_dialogueCharTimer = 0.0f;
    m_dialogueCharIndex = 0;
}

// ボス会話更新
void Game::UpdateBossDialogue() {
    if (!m_bossDialogueActive) return;
    
    // セリフは一気に表示（タイプライター効果なし）
    int len = static_cast<int>(wcslen(g_bossDialogues[m_dialogueLine]));
    m_dialogueCharIndex = len;  // 全文字表示
    
    // Zキー or マウス左クリックで次のセリフ
    static bool zPressed = false;
    bool isPressed = m_input->IsKeyDown('Z') || (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    if (isPressed) {
        if (!zPressed) {
            zPressed = true;
            m_dialogueLine++;
            m_dialogueCharIndex = 0;
            
            if (m_dialogueLine >= g_numDialogues) {
                m_bossDialogueActive = false;
            }
        }
    } else {
        zPressed = false;
    }
}

// ボス会話描画
void Game::RenderBossDialogue() {
    if (!m_bossDialogueActive) return;
    
    // 会話ウィンドウ（画面下部）- 大きく
    float boxX = 50.0f;
    float boxY = PLAY_AREA_HEIGHT - 280.0f;
    float boxW = PLAY_AREA_WIDTH - 100.0f;
    float boxH = 230.0f;
    
    // 半透明背景
    m_graphics->DrawSprite(boxX, boxY, boxW, boxH, 
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.1f, 0.85f));
    
    // 枠線
    m_graphics->DrawSprite(boxX, boxY, boxW, 3.0f, 
        DirectX::XMFLOAT4(1.0f, 0.8f, 0.3f, 1.0f));
    m_graphics->DrawSprite(boxX, boxY + boxH - 3.0f, boxW, 3.0f, 
        DirectX::XMFLOAT4(1.0f, 0.8f, 0.3f, 1.0f));
    
    // 顔イラスト（左側）- セリフに応じてひなひな/かい切替
    float portraitSize = 180.0f;
    bool isKaiSpeaking = (m_dialogueLine < g_numDialogues) && 
        (wcsstr(g_bossDialogues[m_dialogueLine], L"かい「") != nullptr);
    
    if (isKaiSpeaking && m_portraitKai) {
        m_graphics->DrawTexturedSprite(boxX + 20.0f, boxY + 25.0f, portraitSize, portraitSize,
            m_portraitKai.Get(), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    } else if (m_portraitHinata) {
        m_graphics->DrawTexturedSprite(boxX + 20.0f, boxY + 25.0f, portraitSize, portraitSize,
            m_portraitHinata.Get(), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    // テキスト開始位置を左寄せに
    float textStartX = boxX + portraitSize + 30.0f;
    
    // セリフテキスト - 1.5倍大きく、左寄せ
    if (m_text && m_dialogueLine < g_numDialogues) {
        std::wstring displayText(g_bossDialogues[m_dialogueLine], m_dialogueCharIndex);
        m_text->BeginDraw();
        m_text->DrawText(displayText.c_str(), textStartX, boxY + 35.0f, boxW - portraitSize - 60.0f, 160.0f, 3, 0);  // サイズ3で1.5倍
        
        // 次へ進むヒント
        int len = static_cast<int>(wcslen(g_bossDialogues[m_dialogueLine]));
        if (m_dialogueCharIndex >= len) {
            m_text->DrawText(L"Z or Click >>", boxX + boxW - 180.0f, boxY + boxH - 35.0f, 160.0f, 25.0f, 0, 2);
        }
        m_text->EndDraw();
    }
}
