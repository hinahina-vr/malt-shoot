#pragma once

#include <windows.h>
#include <memory>
#include "Graphics.h"
#include "Input.h"
#include "Player.h"
#include "BulletManager.h"
#include "EnemyManager.h"
#include "Background3D.h"
#include "ParticleSystem.h"
#include "ItemManager.h"
#include "SoundManager.h"
#include "BGMPlayer.h"
#include "TextRenderer.h"
#include "TextureLoader.h"

enum class GameState {
    Title,
    Playing,
    Paused,
    GameOver
};

class Game {
public:
    Game();
    ~Game();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();
    void Update();
    void Render();

private:
    std::unique_ptr<Graphics> m_graphics;
    std::unique_ptr<Input> m_input;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<BulletManager> m_bulletManager;
    std::unique_ptr<EnemyManager> m_enemyManager;
    std::unique_ptr<Background3D> m_background;
    std::unique_ptr<ParticleSystem> m_particles;
    std::unique_ptr<ItemManager> m_items;
    std::unique_ptr<SoundManager> m_sound;
    std::unique_ptr<BGMPlayer> m_bgm;
    std::unique_ptr<TextRenderer> m_text;

    HWND m_hWnd;
    int m_width;
    int m_height;
    bool m_isRunning;

    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_lastTime;
    float m_deltaTime;
    
    // FPS counter
    int m_frameCount;
    float m_fpsTimer;
    float m_currentFPS;

    void UpdateDeltaTime();
    void RenderUI();
    void CheckCollisions();
    void UpdateGraze();

    // Game stats
    int m_score;
    int m_hiScore;
    int m_lives;
    int m_bombs;
    int m_power;
    int m_maxPower;
    
    // Graze system
    int m_graze;
    float m_grazeRadius;  // Detection radius for graze
    
    // Special gauge (fills with graze)
    float m_specialGauge;
    float m_maxSpecialGauge;
    bool m_specialReady;
    
    // Combo system
    int m_combo;
    float m_comboTimer;
    
    // Debug mode
    bool m_bossMode;
    void SpawnBoss();
    void HandleDebugInput();

    // Settings menu (ESC to open)
    bool m_isPaused;
    int m_menuSelection;
    int m_bgmVolume;
    int m_sfxVolume;
    void UpdateSettingsMenu();
    void RenderSettingsMenu();

    // Game state and title screen
    GameState m_gameState;
    std::unique_ptr<TextureLoader> m_textureLoader;
    ComPtr<ID3D11ShaderResourceView> m_titleTexture;
    void UpdateTitle();
    void RenderTitle();
};
