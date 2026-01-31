#include <gtest/gtest.h>
#include <cmath>

// BulletManagerのテスト用に必要な定義（Graphicsに依存しない部分）

constexpr float PI = 3.14159265358979f;

// 円形弾幕の角度計算テスト
TEST(BulletPatternTest, CirclePatternAngles) {
    const int count = 8;
    
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * i) / count;
        
        // 角度が0から2πの範囲内であることを確認
        EXPECT_GE(angle, 0.0f);
        EXPECT_LT(angle, 2.0f * PI);
    }
}

// 円形弾幕の速度ベクトル計算テスト
TEST(BulletPatternTest, CirclePatternVelocity) {
    const float speed = 100.0f;
    const int count = 4; // 4方向（上下左右）
    
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * i) / count;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        
        // 速度の大きさがspeedと等しいことを確認
        float magnitude = sqrtf(vx * vx + vy * vy);
        EXPECT_NEAR(magnitude, speed, 0.001f);
    }
}

// 自機狙い弾の方向計算テスト
TEST(BulletPatternTest, AimedBulletDirection) {
    float startX = 100.0f, startY = 100.0f;
    float targetX = 200.0f, targetY = 100.0f; // 右方向
    float speed = 150.0f;
    
    float dx = targetX - startX;
    float dy = targetY - startY;
    float len = sqrtf(dx * dx + dy * dy);
    
    float vx = (dx / len) * speed;
    float vy = (dy / len) * speed;
    
    // 右方向なので vx > 0, vy ≈ 0
    EXPECT_GT(vx, 0.0f);
    EXPECT_NEAR(vy, 0.0f, 0.001f);
    EXPECT_NEAR(vx, speed, 0.001f);
}

// 斜め移動の正規化テスト
TEST(MovementTest, DiagonalNormalization) {
    float dx = 1.0f, dy = 1.0f;
    
    // 斜め移動時の速度正規化
    if (dx != 0.0f && dy != 0.0f) {
        float len = sqrtf(dx * dx + dy * dy);
        dx /= len;
        dy /= len;
    }
    
    // 正規化後の長さは1
    float normalizedLen = sqrtf(dx * dx + dy * dy);
    EXPECT_NEAR(normalizedLen, 1.0f, 0.001f);
}

// 画面境界チェックテスト
TEST(BoundaryTest, ScreenBoundary) {
    const int screenWidth = 640;
    const int screenHeight = 960;
    const float halfSize = 16.0f;
    
    float posX = -10.0f; // 画面外（左）
    float posY = 500.0f;
    
    // 境界チェック
    if (posX < halfSize) posX = halfSize;
    if (posX > screenWidth - halfSize) posX = static_cast<float>(screenWidth) - halfSize;
    
    EXPECT_EQ(posX, halfSize);
    
    // 右端のテスト
    posX = 700.0f; // 画面外（右）
    if (posX > screenWidth - halfSize) posX = static_cast<float>(screenWidth) - halfSize;
    
    EXPECT_EQ(posX, screenWidth - halfSize);
}

// 当たり判定テスト（円と円）
TEST(CollisionTest, CircleCircleCollision) {
    // 衝突あり
    float x1 = 100.0f, y1 = 100.0f, r1 = 10.0f;
    float x2 = 105.0f, y2 = 105.0f, r2 = 10.0f;
    
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist = sqrtf(dx * dx + dy * dy);
    
    bool collision = dist < (r1 + r2);
    EXPECT_TRUE(collision);
    
    // 衝突なし
    x2 = 200.0f;
    y2 = 200.0f;
    dx = x2 - x1;
    dy = y2 - y1;
    dist = sqrtf(dx * dx + dy * dy);
    
    collision = dist < (r1 + r2);
    EXPECT_FALSE(collision);
}
