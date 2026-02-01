#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <string>

#pragma comment(lib, "winmm.lib")

class SoundManager {
public:
    SoundManager() : m_initialized(false), m_soundPath(L""), m_shotAlias(L"shot_sound") {}
    ~SoundManager() {
        // 発射音のクリーンアップ
        std::wstring cmd = L"close " + m_shotAlias;
        mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
    }

    void Initialize() {
        m_initialized = true;
        // Get executable path for sounds
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        m_soundPath = exePath.substr(0, lastSlash) + L"\\..\\..\\assets\\sounds\\";
        
        // 発射音を事前にロード（MCI）
        std::wstring shotFile = m_soundPath + L"shot.mp3";
        std::wstring cmd = L"open \"" + shotFile + L"\" type mpegvideo alias " + m_shotAlias;
        mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
        
        // 音量を10%に設定（0-1000）
        cmd = L"setaudio " + m_shotAlias + L" volume to 100";
        mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
    }

    // 発射音（wav、PlaySound使用）
    void PlayShot() {
        std::wstring shotFile = m_soundPath + L"shot.wav";
        PlaySoundW(shotFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
    }

    void PlayEnemyHit() {
        std::thread([]() {
            Beep(600, 30);
        }).detach();
    }

    void PlayEnemyDestroy() {
        // Play hina_eyao.wav for enemy destruction!
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayPlayerHit() {
        // Play hina_buoo.wav when player gets hit
        std::wstring soundFile = m_soundPath + L"hina_buoo.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayBomb() {
        std::thread([]() {
            for (int i = 0; i < 5; i++) {
                Beep(100 + i * 30, 30);
            }
        }).detach();
    }

    void PlayItemCollect() {
        std::thread([]() {
            Beep(1000, 15);
            Beep(1200, 15);
        }).detach();
    }

    void PlayGraze() {
        std::thread([]() {
            Beep(1500, 10);
        }).detach();
    }

    void PlaySpecialReady() {
        std::thread([]() {
            Beep(800, 50);
            Beep(1000, 50);
            Beep(1200, 80);
        }).detach();
    }

private:
    bool m_initialized;
    std::wstring m_soundPath;
    std::wstring m_shotAlias;
};
