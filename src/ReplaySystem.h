#pragma once

#include <vector>
#include <fstream>
#include <cstdint>

// フレームごとの入力状態
struct InputFrame {
    uint32_t frameNumber;
    bool up, down, left, right;
    bool shoot, bomb, slow;
};

class ReplaySystem {
public:
    ReplaySystem() : m_isRecording(false), m_isPlaying(false), m_currentFrame(0) {}
    
    // 記録開始/停止
    void StartRecording() {
        m_frames.clear();
        m_currentFrame = 0;
        m_isRecording = true;
        m_isPlaying = false;
    }
    
    void StopRecording() {
        m_isRecording = false;
    }
    
    // 再生開始/停止
    void StartPlayback() {
        m_currentFrame = 0;
        m_isPlaying = true;
        m_isRecording = false;
    }
    
    void StopPlayback() {
        m_isPlaying = false;
    }
    
    // フレーム記録
    void RecordFrame(bool up, bool down, bool left, bool right, 
                     bool shoot, bool bomb, bool slow) {
        if (!m_isRecording) return;
        
        InputFrame frame;
        frame.frameNumber = m_currentFrame;
        frame.up = up;
        frame.down = down;
        frame.left = left;
        frame.right = right;
        frame.shoot = shoot;
        frame.bomb = bomb;
        frame.slow = slow;
        
        m_frames.push_back(frame);
        m_currentFrame++;
    }
    
    // フレーム取得（再生用）
    bool GetFrame(InputFrame& outFrame) {
        if (!m_isPlaying || m_currentFrame >= m_frames.size()) {
            m_isPlaying = false;
            return false;
        }
        outFrame = m_frames[m_currentFrame];
        m_currentFrame++;
        return true;
    }
    
    // ファイル保存
    bool SaveToFile(const char* filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        uint32_t count = static_cast<uint32_t>(m_frames.size());
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (const auto& frame : m_frames) {
            file.write(reinterpret_cast<const char*>(&frame), sizeof(InputFrame));
        }
        
        file.close();
        return true;
    }
    
    // ファイル読み込み
    bool LoadFromFile(const char* filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        uint32_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        m_frames.clear();
        m_frames.resize(count);
        
        for (uint32_t i = 0; i < count; i++) {
            file.read(reinterpret_cast<char*>(&m_frames[i]), sizeof(InputFrame));
        }
        
        file.close();
        m_currentFrame = 0;
        return true;
    }
    
    bool IsRecording() const { return m_isRecording; }
    bool IsPlaying() const { return m_isPlaying; }
    uint32_t GetFrameCount() const { return static_cast<uint32_t>(m_frames.size()); }
    
private:
    std::vector<InputFrame> m_frames;
    uint32_t m_currentFrame;
    bool m_isRecording;
    bool m_isPlaying;
};
