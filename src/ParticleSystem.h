#pragma once

#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

struct Particle {
    XMFLOAT2 position;
    XMFLOAT2 velocity;
    XMFLOAT4 color;
    float size;
    float life;
    float maxLife;
    bool isActive;
};

class Graphics;

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void Initialize(int maxParticles = 500);
    void Update(float deltaTime);
    void Render(Graphics* graphics);

    // Explosion effect when enemy dies
    void SpawnExplosion(float x, float y, XMFLOAT4 color, int count = 30);
    
    // Star burst for power-up
    void SpawnStarBurst(float x, float y, XMFLOAT4 color, int count = 15);
    
    // Score pop-up effect
    void SpawnScorePopup(float x, float y, int score);

    // Sparkle trail
    void SpawnTrail(float x, float y, XMFLOAT4 color);

private:
    std::vector<Particle> m_particles;
    int m_maxParticles;
};
