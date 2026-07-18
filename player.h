#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>
typedef struct {
    float x, y;
    float vx, vy;
    bool  isOnGround;
    bool  isAttacking;
    float attackTimer;
    int   facing;
    bool  hasHit;
    int   health;
    float hitFlashTimer;
    float invincibleTimer;
    float attackCooldown;
    int max_jumps;
    int jumps_left;
} Player;

typedef enum {
    ANIM_IDLE = 0,
    ANIM_RUN,
    ANIM_JUMP,
    ANIM_ATTACK,
    ANIM_HURT,
    ANIM_DEAD,
    ANIM_COUNT
} AnimState;

typedef struct {
    Texture2D texture;
    int frameCount;
    float frameDuration;
    int frameWidth;
    int frameHeight;
} SpriteAnimation;

typedef struct {
    SpriteAnimation anims[ANIM_COUNT];
    AnimState currentState;
    int currentFrame;
    float frameTimer;
} PlayerSpriteSystem;
void InitPlayerSprites(PlayerSpriteSystem *sys);
void UpdatePlayerSprites(PlayerSpriteSystem *sys, const Player *p, float dt);
void DrawPlayerSprites(const PlayerSpriteSystem *sys, const Player *p);
void UnloadPlayerSprites(PlayerSpriteSystem *sys);

#endif