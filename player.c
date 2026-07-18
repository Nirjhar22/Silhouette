#include "player.h"

void InitPlayerSprites(PlayerSpriteSystem *sys) {
    sys->anims[ANIM_IDLE]   = (SpriteAnimation){ LoadTexture("knight/Knight_1/Idle.png"),     4, 0.15f, 70,  86 };
    sys->anims[ANIM_RUN]    = (SpriteAnimation){ LoadTexture("knight/Knight_1/Run.png"),      8, 0.10f, 70,  86 };
    sys->anims[ANIM_JUMP]   = (SpriteAnimation){ LoadTexture("knight/Knight_1/Jump.png"),     6, 0.12f, 80,  86 };
    sys->anims[ANIM_ATTACK] = (SpriteAnimation){ LoadTexture("knight/Knight_1/Attack 2.png"), 4, 0.08f, 108, 86 };
    sys->anims[ANIM_HURT]   = (SpriteAnimation){ LoadTexture("knight/Knight_1/Hurt.png"),     2, 0.15f, 70,  86 };
    sys->anims[ANIM_DEAD]   = (SpriteAnimation){ LoadTexture("knight/Knight_1/Dead.png"),     4, 0.20f, 120, 86 };

    sys->currentState = ANIM_IDLE;
    sys->currentFrame = 0;
    sys->frameTimer = 0.0f;
}

void UpdatePlayerSprites(PlayerSpriteSystem *sys, const Player *p, float dt) {
    AnimState nextState = ANIM_IDLE;

    if (p->health <= 0) {
        nextState = ANIM_DEAD;
    } else if (p->hitFlashTimer > 0) {
        nextState = ANIM_HURT;
    } else if (p->isAttacking) {
        nextState = ANIM_ATTACK;
    } else if (!p->isOnGround) {
        nextState = ANIM_JUMP;
    } else if (p->vx != 0) {
        nextState = ANIM_RUN;
    }

    if (nextState != sys->currentState) {
        sys->currentState = nextState;
        sys->currentFrame = 0;
        sys->frameTimer = 0.0f;
    }

    SpriteAnimation current = sys->anims[sys->currentState];
    sys->frameTimer += dt;
    if (sys->frameTimer >= current.frameDuration) {
        sys->frameTimer = 0.0f;
        sys->currentFrame++;
        
        if (sys->currentFrame >= current.frameCount) {
            if (sys->currentState == ANIM_DEAD) {
                sys->currentFrame = current.frameCount - 1; 
            } else {
                sys->currentFrame = 0;
            }
        }
    }
}

void DrawPlayerSprites(const PlayerSpriteSystem *sys, const Player *p) {
    SpriteAnimation current = sys->anims[sys->currentState];
    Rectangle sourceRec = {
        (float)(sys->currentFrame * current.frameWidth),
        0.0f,
        (float)(current.frameWidth * p->facing), 
        (float)current.frameHeight
    };
    
    float baseWidth = 70.0f;
    float basePadding = (baseWidth - 40.0f) / 2.0f; 

    float destX = 0.0f;
    if (p->facing == 1) {
        destX = p->x - basePadding;
    } else {
        destX = (p->x + 40.0f + basePadding) - (float)current.frameWidth;
    }
    float destY = p->y + 60.0f - (float)current.frameHeight;

    Rectangle destRec = {
        destX,
        destY,
        (float)current.frameWidth,
        (float)current.frameHeight
    };

    Vector2 origin = { 0.0f, 0.0f };
    Color tint = (p->hitFlashTimer > 0 && (int)(p->hitFlashTimer * 14) % 2 == 0) 
                 ? (Color){255, 80, 80, 255} 
                 : WHITE;

    DrawTexturePro(current.texture, sourceRec, destRec, origin, 0.0f, tint);
}

void UnloadPlayerSprites(PlayerSpriteSystem *sys) {
    for (int i = 0; i < ANIM_COUNT; i++) {
        UnloadTexture(sys->anims[i].texture);
    }
}