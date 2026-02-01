#pragma once

#include <xaudio2.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Windows.h>

#pragma comment(lib, "xaudio2.lib")

// WAVファイルヘッダー構造体
struct WavHeader {
    char riff[4];
    DWORD fileSize;
    char wave[4];
    char fmt[4];
    DWORD fmtSize;
    WORD audioFormat;
    WORD numChannels;
    DWORD sampleRate;
    DWORD byteRate;
    WORD blockAlign;
    WORD bitsPerSample;
};

// 音声データ
struct AudioData {
    std::vector<BYTE> buffer;
    WAVEFORMATEX format;
};

// XAudio2ベースのオーディオマネージャー
class AudioManager {
public:
    AudioManager() : m_xaudio2(nullptr), m_masterVoice(nullptr) {}
    ~AudioManager() { Shutdown(); }

    bool Initialize() {
        // XAudio2初期化
        HRESULT hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) return false;

        hr = m_xaudio2->CreateMasteringVoice(&m_masterVoice);
        if (FAILED(hr)) return false;

        // 実行ファイルのパスを取得
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        m_soundPath = exePath.substr(0, lastSlash) + L"\\..\\..\\assets\\sounds\\";

        // 音声ファイルをプリロード
        LoadSound(L"shot", m_soundPath + L"shot.wav");
        LoadSound(L"hit", m_soundPath + L"hina_eyao.wav");
        LoadSound(L"destroy", m_soundPath + L"hina_eyao.wav");
        LoadSound(L"player_hit", m_soundPath + L"hina_buoo.wav");
        LoadSound(L"bomb", m_soundPath + L"hina_buoo.wav");
        LoadSound(L"item", m_soundPath + L"hina_eyao.wav");

        return true;
    }

    void Shutdown() {
        // 全ボイス停止
        for (auto& voice : m_activeVoices) {
            if (voice) {
                voice->Stop();
                voice->DestroyVoice();
            }
        }
        m_activeVoices.clear();

        if (m_masterVoice) {
            m_masterVoice->DestroyVoice();
            m_masterVoice = nullptr;
        }
        if (m_xaudio2) {
            m_xaudio2->Release();
            m_xaudio2 = nullptr;
        }
    }

    void PlaySound(const std::wstring& name, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;

        const AudioData& audio = it->second;

        // 新しいソースボイスを作成
        IXAudio2SourceVoice* sourceVoice = nullptr;
        HRESULT hr = m_xaudio2->CreateSourceVoice(&sourceVoice, &audio.format);
        if (FAILED(hr)) return;

        // バッファを設定
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = static_cast<UINT32>(audio.buffer.size());
        buffer.pAudioData = audio.buffer.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        sourceVoice->SetVolume(volume);
        sourceVoice->SubmitSourceBuffer(&buffer);
        sourceVoice->Start();

        // アクティブボイスリストに追加
        m_activeVoices.push_back(sourceVoice);

        // 終了したボイスをクリーンアップ（最大32個まで）
        CleanupVoices();
    }

    void PlayShot(float volume = 0.5f) { PlaySound(L"shot", volume); }
    void PlayEnemyHit(float volume = 0.7f) { PlaySound(L"hit", volume); }
    void PlayEnemyDestroy(float volume = 0.8f) { PlaySound(L"destroy", volume); }
    void PlayPlayerHit(float volume = 1.0f) { PlaySound(L"player_hit", volume); }
    void PlayBomb(float volume = 1.0f) { PlaySound(L"bomb", volume); }
    void PlayItemCollect(float volume = 0.6f) { PlaySound(L"item", volume); }
    void PlayGraze(float volume = 0.4f) { PlaySound(L"hit", volume); }
    void PlaySpecialReady(float volume = 0.8f) { PlaySound(L"item", volume); }
    void PlayCursor(float volume = 0.6f) { PlaySound(L"hit", volume); }

private:
    bool LoadSound(const std::wstring& name, const std::wstring& filepath) {
        // WAVファイルを読み込む
        HANDLE file = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        DWORD fileSize = GetFileSize(file, nullptr);
        std::vector<BYTE> fileData(fileSize);
        DWORD bytesRead;
        ReadFile(file, fileData.data(), fileSize, &bytesRead, nullptr);
        CloseHandle(file);

        // WAVヘッダー解析
        if (fileSize < sizeof(WavHeader)) return false;
        WavHeader* header = reinterpret_cast<WavHeader*>(fileData.data());

        // "data"チャンクを探す
        BYTE* ptr = fileData.data() + sizeof(WavHeader);
        BYTE* end = fileData.data() + fileSize;
        
        while (ptr < end - 8) {
            if (memcmp(ptr, "data", 4) == 0) {
                DWORD dataSize = *reinterpret_cast<DWORD*>(ptr + 4);
                ptr += 8;

                AudioData audio;
                audio.format.wFormatTag = header->audioFormat;
                audio.format.nChannels = header->numChannels;
                audio.format.nSamplesPerSec = header->sampleRate;
                audio.format.nAvgBytesPerSec = header->byteRate;
                audio.format.nBlockAlign = header->blockAlign;
                audio.format.wBitsPerSample = header->bitsPerSample;
                audio.format.cbSize = 0;

                audio.buffer.assign(ptr, ptr + dataSize);
                m_sounds[name] = std::move(audio);
                return true;
            }
            ptr++;
        }
        return false;
    }

    void CleanupVoices() {
        // 終了したボイスを削除
        auto it = m_activeVoices.begin();
        while (it != m_activeVoices.end()) {
            XAUDIO2_VOICE_STATE state;
            (*it)->GetState(&state);
            if (state.BuffersQueued == 0) {
                (*it)->DestroyVoice();
                it = m_activeVoices.erase(it);
            } else {
                ++it;
            }
        }

        // 最大32個を超えたら古いものを削除
        while (m_activeVoices.size() > 32) {
            m_activeVoices.front()->Stop();
            m_activeVoices.front()->DestroyVoice();
            m_activeVoices.erase(m_activeVoices.begin());
        }
    }

    IXAudio2* m_xaudio2;
    IXAudio2MasteringVoice* m_masterVoice;
    std::wstring m_soundPath;
    std::unordered_map<std::wstring, AudioData> m_sounds;
    std::vector<IXAudio2SourceVoice*> m_activeVoices;
};
