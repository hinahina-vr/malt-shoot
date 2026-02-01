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
    }

    // 発射音
    void PlayShot() {
        std::wstring soundFile = m_soundPath + L"shot.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayEnemyHit() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayEnemyDestroy() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayPlayerHit() {
        std::wstring soundFile = m_soundPath + L"hina_buoo.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayBomb() {
        std::wstring soundFile = m_soundPath + L"hina_buoo.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayCursor() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayItemCollect() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayGraze() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlaySpecialReady() {
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

private:
    bool m_initialized;
    std::wstring m_soundPath;
    std::wstring m_shotAlias;
};
