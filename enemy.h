#ifndef ENEMY_H
#define ENEMY_H

#define IDLE_FRAMES 4
#define WALK_FRAMES 6
#define ATTACK_FRAMES 4
#define HURT_FRAMES 4

#include <raylib.h>

typedef enum {
    eANIM_IDLE = 0,
    eANIM_WALK,
    eANIM_ATTACK,
    eANIM_HURT,
    eANIM_COUNT
} EnemyAnimState;

typedef enum {
    PATROLLING,
    CHASE,
    ATTACKING,
    RETURN
} EnemyState;

typedef struct {
    Texture2D idle[IDLE_FRAMES];
    Texture2D walk[WALK_FRAMES];
    Texture2D attack[ATTACK_FRAMES];
    Texture2D hurt[HURT_FRAMES];
} EnemyTextures;

typedef struct {
    EnemyTextures whiteSet;
    
    int currentFrame[8];
    float frameTimer[8];
    EnemyAnimState currentAnim[8];
} EnemySpriteSystem;

typedef struct {
    float x, y;
    float vx;
    float health;
    int   maxHealth;
    bool  isAlive;
    float hitFlashTimer;
    float patrolLeft;
    float patrolRight;
    bool  isAttacking;
    float attackTimer;
    float attackCooldown;
    int   facing;
    EnemyState state;
    bool  elevated;
    float alertRange;
    float chaseSpeed;
} Enemy;

void InitEnemySprites(EnemySpriteSystem *sys);
void ResetEnemySprites(EnemySpriteSystem *sys);
void UpdateEnemySprites(EnemySpriteSystem *sys, Enemy enemies[], int enemyCount, float dt);
void DrawEnemySprite(EnemySpriteSystem *sys, int enemyIndex, const Enemy *e);
void UnloadEnemySprites(EnemySpriteSystem *sys);
Texture2D GetEnemyTexture(EnemySpriteSystem *sys, int enemyIndex, EnemyAnimState anim);

#endif