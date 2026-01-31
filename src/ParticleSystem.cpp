#include "ParticleSystem.h"
#include "Graphics.h"
#include <cmath>
#include <cstdlib>

constexpr float PI = 3.14159265358979f;

ParticleSystem::ParticleSystem()
    : m_maxParticles(500)
{
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::Initialize(int maxParticles) {
    m_maxParticles = maxParticles;
    m_particles.reserve(maxParticles);
}

void ParticleSystem::Update(float deltaTime) {
    for (auto& p : m_particles) {
        if (!p.isActive) continue;

        // Update position
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;

        // Apply gravity (slight downward pull)
        p.velocity.y += 50.0f * deltaTime;

        // Fade out and shrink
        p.life -= deltaTime;
        if (p.life <= 0) {
            p.isActive = false;
        }

        // Update color alpha based on life
        float lifeRatio = p.life / p.maxLife;
        p.color.w = lifeRatio;
        p.size = p.size * (0.95f + 0.05f * lifeRatio);
    }
}

void ParticleSystem::Render(Graphics* graphics) {
    for (const auto& p : m_particles) {
        if (!p.isActive) continue;

        float lifeRatio = p.life / p.maxLife;
        
        // Draw glow effect
        graphics->DrawGlowCircle(
            p.position.x,
            p.position.y,
            p.size * (1.0f + (1.0f - lifeRatio) * 0.5f),
            p.color,
            2
        );
    }
}

void ParticleSystem::SpawnExplosion(float x, float y, XMFLOAT4 color, int count) {
    for (int i = 0; i < count; i++) {
        // Find inactive particle
        Particle* particle = nullptr;
        for (auto& p : m_particles) {
            if (!p.isActive) {
                particle = &p;
                break;
            }
        }
        
        if (!particle && m_particles.size() < static_cast<size_t>(m_maxParticles)) {
            m_particles.push_back(Particle());
            particle = &m_particles.back();
        }
        
        if (!particle) continue;

        // Random angle and speed
        float angle = (2.0f * PI * i) / count + (rand() % 100) / 100.0f * 0.5f;
        float speed = 100.0f + (rand() % 200);

        particle->position = { x, y };
        particle->velocity = { cosf(angle) * speed, sinf(angle) * speed - 100.0f };
        
        // Color variation
        float hueShift = (rand() % 100) / 100.0f * 0.3f;
        particle->color = {
            fminf(1.0f, color.x + hueShift),
            fminf(1.0f, color.y + hueShift * 0.5f),
            fminf(1.0f, color.z + (1.0f - hueShift)),
            1.0f
        };
        
        particle->size = 4.0f + (rand() % 8);
        particle->life = 0.5f + (rand() % 100) / 100.0f * 0.5f;
        particle->maxLife = particle->life;
        particle->isActive = true;
    }
}

void ParticleSystem::SpawnStarBurst(float x, float y, XMFLOAT4 color, int count) {
    for (int i = 0; i < count; i++) {
        Particle* particle = nullptr;
        for (auto& p : m_particles) {
            if (!p.isActive) {
                particle = &p;
                break;
            }
        }
        
        if (!particle && m_particles.size() < static_cast<size_t>(m_maxParticles)) {
            m_particles.push_back(Particle());
            particle = &m_particles.back();
        }
        
        if (!particle) continue;

        float angle = (2.0f * PI * i) / count;
        float speed = 150.0f + (rand() % 100);

        particle->position = { x, y };
        particle->velocity = { cosf(angle) * speed, sinf(angle) * speed };
        particle->color = color;
        particle->size = 6.0f;
        particle->life = 0.8f;
        particle->maxLife = particle->life;
        particle->isActive = true;
    }
}

void ParticleSystem::SpawnScorePopup(float x, float y, int score) {
    // Spawn golden particles for score
    XMFLOAT4 goldColor = { 1.0f, 0.9f, 0.3f, 1.0f };
    
    int count = 5 + score / 100;
    if (count > 20) count = 20;
    
    for (int i = 0; i < count; i++) {
        Particle* particle = nullptr;
        for (auto& p : m_particles) {
            if (!p.isActive) {
                particle = &p;
                break;
            }
        }
        
        if (!particle && m_particles.size() < static_cast<size_t>(m_maxParticles)) {
            m_particles.push_back(Particle());
            particle = &m_particles.back();
        }
        
        if (!particle) continue;

        float offsetX = (rand() % 40) - 20.0f;
        float offsetY = (rand() % 20) - 10.0f;

        particle->position = { x + offsetX, y + offsetY };
        particle->velocity = { offsetX * 2.0f, -100.0f - (rand() % 50) };
        particle->color = goldColor;
        particle->size = 3.0f + (rand() % 4);
        particle->life = 0.6f;
        particle->maxLife = particle->life;
        particle->isActive = true;
    }
}

void ParticleSystem::SpawnTrail(float x, float y, XMFLOAT4 color) {
    Particle* particle = nullptr;
    for (auto& p : m_particles) {
        if (!p.isActive) {
            particle = &p;
            break;
        }
    }
    
    if (!particle && m_particles.size() < static_cast<size_t>(m_maxParticles)) {
        m_particles.push_back(Particle());
        particle = &m_particles.back();
    }
    
    if (!particle) return;

    particle->position = { x + (rand() % 10) - 5.0f, y };
    particle->velocity = { 0, 30.0f };
    particle->color = color;
    particle->size = 3.0f;
    particle->life = 0.3f;
    particle->maxLife = particle->life;
    particle->isActive = true;
}
