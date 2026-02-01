#include "Game.h"
#include <cmath>
#include <fstream>

// Play area constants (Full HD - 1920x1080)
// Vertical smartphone style with sidebar
constexpr int PLAY_AREA_WIDTH = 608;    // 9:16 ratio vertical style
constexpr int PLAY_AREA_HEIGHT = 1080;
constexpr int SIDEBAR_WIDTH = 1312;     // Rest for UI panel

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
    m_player->Initialize(m_graphics.get(), m_bulletManager.get());
    m_player->SetPosition(PLAY_AREA_WIDTH / 2.0f, PLAY_AREA_HEIGHT - 100.0f);

    m_enemyManager = std::make_unique<EnemyManager>();
    m_enemyManager->Initialize(m_graphics.get(), m_bulletManager.get());

    m_particles = std::make_unique<ParticleSystem>();
    m_particles->Initialize(500);

    m_items = std::make_unique<ItemManager>();
    m_items->Initialize();

    m_sound = std::make_unique<SoundManager>();
    m_sound->Initialize();

    m_bgm = std::make_unique<BGMPlayer>();
    m_bgm->Initialize();
    m_bgm->PlayStageBGM();

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
    std::wstring texturePath = path.substr(0, lastSlash) + L"\\..\\..\\assets\\textures\\title_screen.png";
    
    ID3D11ShaderResourceView* titleSRV = nullptr;
    if (m_textureLoader->LoadTexture(texturePath, &titleSRV)) {
        m_titleTexture.Attach(titleSRV);
    }

    m_isRunning = true;
    LoadHiScore();
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
    m_background->Update(m_deltaTime);
    m_player->Update(m_input.get(), m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    m_enemyManager->SetPlayerPosition(m_player->GetPosition());
    m_enemyManager->Update(m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    
    // ボス撃破時→ステージクリア
    if (m_enemyManager->IsBossWave() && m_enemyManager->AllEnemiesDead()) {
        if (m_score > m_hiScore) m_hiScore = m_score;
        m_gameState = GameState::StageClear;
    }
    
    m_bulletManager->Update(m_deltaTime, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);
    m_particles->Update(m_deltaTime);
    m_items->Update(m_deltaTime, m_player->GetPosition(), PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT);

    // Graze system
    UpdateGraze();
    
    // Collision detection
    CheckCollisions();

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
    m_power += collected.power;
    if (m_power > m_maxPower) m_power = m_maxPower;
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
            
            // Damage all enemies
            for (auto& enemy : m_enemyManager->GetEnemies()) {
                if (enemy->IsActive()) {
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
                enemy->TakeDamage(10.0f);
                bullet.isActive = false;
                
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
        
        // UI Text labels
        float textX = static_cast<float>(PLAY_AREA_WIDTH + 20);
        m_text->DrawTextWithValue(L"Hi-Score", m_hiScore, textX, 50);
        m_text->DrawTextWithValue(L"Score", m_score, textX, 90);
        m_text->DrawTextWithValue(L"Lives", m_lives, textX, 170);
        m_text->DrawTextWithValue(L"Bombs", m_bombs, textX, 210);
        m_text->DrawTextWithValue(L"Power", m_power, textX, 250);
        m_text->DrawTextWithValue(L"Graze", m_graze, textX, 330);
        
        // FPS display
        wchar_t fpsBuffer[32];
        swprintf_s(fpsBuffer, L"FPS: %.1f", m_currentFPS);
        m_text->DrawText(fpsBuffer, textX, 410, 200, 30, 1, m_currentFPS >= 60 ? 0 : 1);
        
        // Build info
        m_text->DrawText(L"LoC: 3300+", textX, static_cast<float>(PLAY_AREA_HEIGHT - 60), 200, 20, 0, 2);
        m_text->DrawText(L"Hinata vs Kai", textX, static_cast<float>(PLAY_AREA_HEIGHT - 35), 200, 20, 0, 1);
        
        m_text->EndDraw();
    }

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
            float barY = 20.0f;
            float barWidth = PLAY_AREA_WIDTH - 40.0f;
            float barHeight = 16.0f;
            float healthPercent = enemy->GetHealthPercent();
            
            // 背景
            m_graphics->DrawSprite(barX, barY, barWidth, barHeight,
                DirectX::XMFLOAT4(0.2f, 0.1f, 0.1f, 0.8f));
            // 体力バー
            m_graphics->DrawSprite(barX, barY, barWidth * healthPercent, barHeight,
                DirectX::XMFLOAT4(0.9f, 0.2f, 0.2f, 1.0f));
            // 枠
            m_graphics->DrawSprite(barX - 2, barY - 2, barWidth + 4, 2,
                DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f));
            m_graphics->DrawSprite(barX - 2, barY + barHeight, barWidth + 4, 2,
                DirectX::XMFLOAT4(1.0f, 0.9f, 0.3f, 1.0f));
            break;
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

    // Combo
    if (m_combo > 0) {
        float comboAlpha = fminf(1.0f, m_comboTimer);
        m_graphics->DrawGlowCircle(uiX + 100, uiY, 20 + m_combo,
            DirectX::XMFLOAT4(1.0f, 0.8f, 0.2f, comboAlpha), 3);
    }
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
    
    // Press P for full power
    if (GetAsyncKeyState('P') & 0x8000) {
        if (!pPressed) {
            m_power = m_maxPower;
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
}

void Game::SpawnBoss() {
    m_bossMode = true;
    m_bulletManager->Clear();
    m_enemyManager->Clear();  // 雑魚クリア
    
    // ボスBGMに切り替え
    m_bgm->Stop();
    m_bgm->PlayBossBGM();
    
    // Spawn boss enemy at top center (x, y, health=500, pattern=0)
    m_enemyManager->SpawnEnemy(PLAY_AREA_WIDTH / 2.0f, 150.0f, 500.0f, 0);
}

void Game::UpdateSettingsMenu() {
    // Menu navigation
    static bool upPressed = false, downPressed = false;
    static bool leftPressed = false, rightPressed = false;
    static bool escPressed = false;
    
    // ESC to close and resume
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!escPressed) {
            m_gameState = GameState::Playing;
        }
        escPressed = true;
    } else { escPressed = false; }
    
    // Up/Down to select menu item
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        if (!upPressed) {
            m_menuSelection = (m_menuSelection - 1 + 4) % 4;
        }
        upPressed = true;
    } else { upPressed = false; }
    
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        if (!downPressed) {
            m_menuSelection = (m_menuSelection + 1) % 4;
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
            }
        }
        rightPressed = true;
    } else { rightPressed = false; }
    
    // Z to confirm selection
    static bool zPressed = false;
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed) {
            if (m_menuSelection == 2) {
                m_gameState = GameState::Playing;
            } else if (m_menuSelection == 3) {
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
    
    // Menu background
    m_graphics->DrawSprite(menuX - 20, menuY - 20, 340, 300,
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
        
        // Title
        m_text->DrawText(L"PAUSE", menuX + 100, menuY - 15, 140, 35, 2, 1);
        
        // Menu items (Japanese)
        const wchar_t* labels[] = { L"BGM", L"SFX", L"Resume", L"Quit" };
        for (int i = 0; i < 4; i++) {
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

void Game::UpdateTitle() {
    static bool zPressed = false, leftPressed = false, rightPressed = false;
    
    // Left/Right to select difficulty
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
        if (!leftPressed) {
            m_titleSelection = (m_titleSelection - 1 + 3) % 3;
        }
        leftPressed = true;
    } else { leftPressed = false; }
    
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        if (!rightPressed) {
            m_titleSelection = (m_titleSelection + 1) % 3;
        }
        rightPressed = true;
    } else { rightPressed = false; }
    
    // Z key to start game with selected difficulty
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed) {
            m_difficulty = static_cast<Difficulty>(m_titleSelection);
            m_gameState = GameState::Playing;
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
        
        // Difficulty selection
        const wchar_t* difficulties[] = { L"Easy", L"Normal", L"Hard" };
        float diffY = m_height - 280.0f;
        float centerX = m_width / 2.0f;
        
        for (int i = 0; i < 3; i++) {
            float x = centerX - 150.0f + i * 120.0f;
            int colorType = (i == m_titleSelection) ? 1 : 0;  // Gold for selected, white otherwise
            m_text->DrawText(difficulties[i], x, diffY, 100, 40, 2, colorType);
        }
        
        // Selection arrows
        m_text->DrawText(L"< ", centerX - 180.0f, diffY, 30, 40, 2, 1);
        m_text->DrawText(L" >", centerX + 150.0f, diffY, 30, 40, 2, 1);
        
        // "Press Z to Start" fading text
        float time = static_cast<float>(GetTickCount64()) * 0.003f;
        float alpha = (sinf(time) + 1.0f) * 0.5f;
        
        std::wstring startText = L"Press Z to Start";
        float textWidth = 500.0f;
        float textX = (m_width - textWidth) / 2.0f;
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
            } else {
                // Return to title
                ResetGame();
                m_gameState = GameState::Title;
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
    
    // Z to return to title
    if (GetAsyncKeyState('Z') & 0x8000) {
        if (!zPressed) {
            ResetGame();
            m_gameState = GameState::Title;
        }
        zPressed = true;
    } else { zPressed = false; }
}

void Game::RenderStageClear() {
    // Dark overlay with golden tint
    m_graphics->DrawSprite(0, 0, static_cast<float>(m_width), static_cast<float>(m_height),
        DirectX::XMFLOAT4(0.1f, 0.08f, 0.0f, 0.8f));
    
    m_graphics->GetContext()->Flush();
    
    if (m_text) {
        m_text->BeginDraw();
        
        float centerX = m_width / 2.0f;
        float centerY = m_height / 2.0f;
        
        // Stage Clear text
        m_text->DrawText(L"STAGE CLEAR!", centerX - 180, centerY - 120, 360, 60, 3, 1);
        
        // Final score
        wchar_t scoreText[64];
        swprintf_s(scoreText, L"Score: %d", m_score);
        m_text->DrawText(scoreText, centerX - 100, centerY - 20, 200, 40, 2, 0);
        
        // Hi-Score
        if (m_score >= m_hiScore) {
            m_text->DrawText(L"NEW HIGH SCORE!", centerX - 120, centerY + 30, 240, 40, 2, 1);
        }
        
        // Press Z to continue
        float time = static_cast<float>(GetTickCount64()) * 0.003f;
        float alpha = (sinf(time) + 1.0f) * 0.5f;
        m_text->DrawTextWithAlpha(L"Press Z", centerX - 80, centerY + 100, 160, 40, 2, 0, alpha);
        
        m_text->EndDraw();
    }
}

void Game::ResetGame() {
    m_score = 0;
    m_lives = 3;
    m_bombs = 3;
    m_power = 0;
    m_graze = 0;
    m_combo = 0;
    m_specialGauge = 0;
    m_specialReady = false;
    m_continueCount = 3;
    m_bossMode = false;
    m_bulletManager->Clear();
    m_items->Clear();
    m_enemyManager->Clear();
    m_enemyManager->ResetWaves();
    m_enemyManager->SpawnWave(0);
    m_player->SetPosition(PLAY_AREA_WIDTH / 2.0f, PLAY_AREA_HEIGHT - 100.0f);
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
