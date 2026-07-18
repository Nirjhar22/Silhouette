#include "enemy.h"

void InitEnemySprites(EnemySpriteSystem *sys) {

for (int i = 0; i < 4; i++)
{
    sys->whiteSet.idle[i] =
        LoadTexture(TextFormat("enemy/idle-%d.png", i + 1));

    sys->whiteSet.attack[i] =
        LoadTexture(TextFormat("enemy/attack-%d.png", i + 1));

    sys->whiteSet.hurt[i] =
        LoadTexture(TextFormat("enemy/get-hit-%d.png", i + 1));
}

for (int i = 0; i < 6; i++)
{
    sys->whiteSet.walk[i] =
        LoadTexture(TextFormat("enemy/walk-R-%d.png", i + 1));
}
   
    ResetEnemySprites(sys);
}

void ResetEnemySprites(EnemySpriteSystem *sys) {
    for (int i = 0; i < 8; i++) {
        sys->currentFrame[i] = 0;
        sys->frameTimer[i] = 0.0f;
        sys->currentAnim[i] = eANIM_IDLE;
    }
}

Texture2D GetEnemyTexture(EnemySpriteSystem *sys, int enemyIndex, EnemyAnimState anim) {
    Texture2D empty = {0};
    return empty;
}

void UpdateEnemySprites(EnemySpriteSystem *sys, Enemy enemies[], int enemyCount, float dt) {
    for (int i = 0; i < enemyCount; i++) {
        Enemy *e = &enemies[i];
        EnemyAnimState nextAnim = eANIM_IDLE;
        if (!e->isAlive) {
            continue;
        } else if (e->hitFlashTimer > 0) {
            nextAnim = eANIM_HURT;
        } else if (e->isAttacking) {
            nextAnim = eANIM_ATTACK;
        } else {
            float absVx = (e->vx < 0.0f) ? -e->vx : e->vx;
            if (absVx > 0.01f) {
                nextAnim = eANIM_WALK;
            }
        }

        if (sys->currentAnim[i] != nextAnim) {
            sys->currentAnim[i] = nextAnim;
            sys->currentFrame[i] = 0;
            sys->frameTimer[i] = 0.0f;
        }
        


int frameCount = 1;

switch (sys->currentAnim[i])
{
    case eANIM_IDLE:
        frameCount = IDLE_FRAMES;
        break;

    case eANIM_WALK:
        frameCount = WALK_FRAMES;
        break;

    case eANIM_ATTACK:
        frameCount = ATTACK_FRAMES;
        break;

    case eANIM_HURT:
        frameCount = HURT_FRAMES;
        break;
}

        if (frameCount <= 0) frameCount = 1;
        
        sys->frameTimer[i] += dt;
        
        float frameDuration = 0.1f; 
        if (sys->currentAnim[i] == eANIM_ATTACK) {
            frameDuration = 0.20f / (float)frameCount; 
        } else if (sys->currentAnim[i] == eANIM_HURT) {
            frameDuration = 0.15f / (float)frameCount;
        }
        
        if (sys->frameTimer[i] >= frameDuration) {
            sys->frameTimer[i] -= frameDuration;
            sys->currentFrame[i]++;
            
            if (sys->currentFrame[i] >= frameCount) {
                if (sys->currentAnim[i] == eANIM_HURT) {
                    sys->currentFrame[i] = frameCount - 1; 
                } else {
                    sys->currentFrame[i] = 0; 
                }
            }
        }
    }
}

void DrawEnemySprite(EnemySpriteSystem *sys, int enemyIndex, const Enemy *e) {
    EnemyAnimState anim = sys->currentAnim[enemyIndex];
    int frame = sys->currentFrame[enemyIndex];

    Texture2D tex = {0};

    switch(anim)
    {
        case eANIM_IDLE:
            if(frame >= IDLE_FRAMES) frame = IDLE_FRAMES - 1;
            tex = sys->whiteSet.idle[frame];
            break;

        case eANIM_WALK:
            if(frame >= WALK_FRAMES) frame = WALK_FRAMES - 1;
            tex = sys->whiteSet.walk[frame];
            break;

        case eANIM_ATTACK:
            if(frame >= ATTACK_FRAMES) frame = ATTACK_FRAMES - 1;
            tex = sys->whiteSet.attack[frame];
            break;

        case eANIM_HURT:
            if(frame >= HURT_FRAMES) frame = HURT_FRAMES - 1;
            tex = sys->whiteSet.hurt[frame];
            break;

        default:
            tex = sys->whiteSet.idle[0];
            break;
    }

    if(tex.id == 0)
        return;

    Rectangle srcRec = {
        0,
        0,
        (float)tex.width,
        (float)tex.height
    };

    if(e->facing == -1)
    {
        srcRec.x = (float)tex.width;
        srcRec.width = -(float)tex.width;
    }

    float scale = 3.0f;

    Rectangle destRec = {
        e->x - 28,
        e->y - 56,
        tex.width * scale,
        tex.height * scale
    };
    Vector2 origin = { 0.0f, 0.0f };
    Color tint = (e->hitFlashTimer > 0.0f) ? RED : WHITE;
    
    DrawTexturePro(tex, srcRec, destRec, origin, 0.0f, tint);
}

void UnloadEnemySprites(EnemySpriteSystem *sys)
{
    for(int i = 0; i < IDLE_FRAMES; i++)
        UnloadTexture(sys->whiteSet.idle[i]);

    for(int i = 0; i < WALK_FRAMES; i++)
        UnloadTexture(sys->whiteSet.walk[i]);

    for(int i = 0; i < ATTACK_FRAMES; i++)
        UnloadTexture(sys->whiteSet.attack[i]);

    for(int i = 0; i < HURT_FRAMES; i++)
        UnloadTexture(sys->whiteSet.hurt[i]);
}