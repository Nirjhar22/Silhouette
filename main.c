#include <raylib.h>
#include <math.h>
#include <stdio.h>

#include "player.h"
#include "enemy.h"

void UpdateEnemy(Enemy *e, Player *p, float dt, float attackRange, float alertRange, float enemyAttackDur, float enemyAttackCDMax, float chaseSpeed)
{
    if (!e->isAlive)
        return;

    if (e->hitFlashTimer > 0)
        e->hitFlashTimer -= dt;
    if (e->attackCooldown > 0)
        e->attackCooldown -= dt;
    
    float dx = (p->x + 20) - (e->x + 20);
    float dy = (p->y + 30) - (e->y + 30);
    float dist = sqrtf(dx * dx + dy * dy);
    
    switch (e->state)
    {
        case PATROLING:
        {
            e->x += e->vx * dt;
            if (e->x <= e->patrolLeft)
            {
                e->x = e->patrolLeft;
                e->vx = fabsf(e->vx);
                e->facing = 1;
            }
            if (e->x + 40 >= e->patrolRight)
            {
                e->x = e->patrolRight - 40;
                e->vx = -fabsf(e->vx);
                e->facing = -1;
            }
            if (dist < alertRange)
                e->state = CHASE;
            break;
        }
        case CHASE:
        {
            if(fabsf(dx) > 4.0f)
            {
                e->facing = (dx >= 0) ? 1 : -1;
                e->vx = e->facing * chaseSpeed;
                
                float futureX = e->x + e->vx * dt;

                if(!e->elevated)
                    e->x = futureX;

                else if(futureX >= e->patrolLeft && futureX + 40 <= e-> patrolRight)
                    e->x = futureX;
            }
            else
            {
                e->vx = 0;
            }

            if (dist < attackRange && e->attackCooldown <= 0)
            {
                e->state = ATTACKING;
                e->attackTimer = enemyAttackDur;
                e->isAttacking = true;
            }
            else if (dist > alertRange * 1.5f)
            {
                e->state = RETURN;
            }
            break;
            }
        case ATTACKING:
        {
            e->attackTimer -= dt;
            if (e->attackTimer <= 0)
            {
                e->state = CHASE;
                e->attackCooldown = enemyAttackCDMax;
                e->isAttacking = false;
            }
            break;
        }
        case RETURN:
        {
            if(e->x >= e->patrolLeft && e->x +40 <= e->patrolRight)
            {
                e->state = PATROLING;
                break;
            }
            if(e->x< e->patrolLeft)
            {
                e->facing = 1;
                e->x += chaseSpeed * dt;
            }
            else
            {
                e->facing = -1;
                e->x -= chaseSpeed * dt;
            }
            if(dist < alertRange)
                e->state = CHASE;

            break;
        }
    }
}

typedef struct
{
    float x, y, vx, patrolLeft, patrolRight;
    bool elevated;
} EnemySpawn;

typedef struct
{
    Rectangle *platforms;
    int platformCount;
    Texture2D *background;
    float levelWidth, levelHeight;
    float playerStartX, playerStartY;
    EnemySpawn enemySpawns[8];
    int enemyCount;
} LevelDef;

#define LEVEL_COUNT 2
LevelDef levels[LEVEL_COUNT];
int currentLevel = 0;

Texture2D *currentBackground;
Rectangle *currentPlatforms;

int currentPlatformCount;
float currentLevelWidth, currentLevelHeight;

Rectangle GetAttackHitbox(Player p)
{
    float w = 50, h = 40;
    float x = (p.facing == 1) ? p.x + 40 : p.x - w;
    return (Rectangle){x, p.y + 10, w, h};
}

Rectangle GetEnemyAttackHitbox(Enemy e)
{
    float w = 40, h = 30;
    float x = (e.facing == 1) ? e.x + 40 : e.x - w;
    return (Rectangle){x, e.y + 5, w, h};
}

void DrawHealthBar(float x, float y, float w, float h, float hp, float maxHp, Color fill, Color bg)
{
    DrawRectangle((int)x, (int)y, (int)w, (int)h, bg);
    float ratio = hp / maxHp;
    if (ratio < 0)
        ratio = 0;
    DrawRectangle((int)x, (int)y, (int)(w * ratio), (int)h, fill);
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, WHITE);
}

void ResolvePlatformCollisionX(float *x, float y, float w, float h, float *vx, Rectangle *platforms, int count)
{
    Rectangle pr = {*x, y, w, h};
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionRecs(pr, platforms[i]))
        {
            if (*vx > 0)
                *x = platforms[i].x - w;
            else if (*vx < 0)
                *x = platforms[i].x + platforms[i].width;
            *vx = 0;
            pr.x = *x;
        }
    }
}

bool ResolvePlatformCollisionY(float x, float *y, float w, float h, float *vy, Rectangle *platforms, int count)
{
    bool landed = false;
    Rectangle pr = {x, *y, w, h};
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionRecs(pr, platforms[i]))
        {
            if (*vy > 0)
            {
                *y = platforms[i].y - h;
                *vy = 0;
                landed = true;
            }
            else if (*vy < 0)
            {
                *y = platforms[i].y + platforms[i].height;
                *vy = 0;
            }
            pr.y = *y;
        }
    }
    return landed;
}

void ResetGame(Player *p, Enemy enemies[], int enemyCount)
{
    (void)enemyCount;

    p->x = 100;
    p->y = 300;
    p->vx = 0;
    p->vy = 0;
    p->isOnGround = false;
    p->isAttacking = false;
    p->attackTimer = 0;
    p->facing = 1;
    p->hasHit = false;
    p->health = 15;
    p->hitFlashTimer = 0;
    p->invincibleTimer = 0;
    p->max_jumps = 2;
    p->jumps_left = 2;
}

Rectangle level1Platforms[] = {
    {736,  128, 320, 32},
    {288,  160, 224, 32},
    {1248, 192, 544, 32},
    {0,    256, 160, 32},
    {672,  288, 192, 32},
    {192,  320, 32,  64},
    {1056, 320, 224, 32},
    {576,  352, 32,  64},
    {1664, 352, 128, 32},
    {352,  416, 160, 32},
    {288,  448, 160, 32},
    {896,  448, 96,  32},
    {1344, 448, 224, 32},
    {0,    576, 1920, 64},
};
#define LEVEL1_PLATFORM_COUNT (sizeof(level1Platforms)/sizeof(level1Platforms[0]))

Rectangle level2Platforms[] = {
    {704,  96,  160, 32},
    {992,  128, 576, 32},
    {288,  160, 288, 32},
    {704,  256, 128, 32},
    {1568, 256, 160, 32},
    {0,    288, 192, 32},
    {1408, 320, 32,  96},
    {192,  352, 32,  64},
    {512,  352, 128, 32},
    {1120, 352, 160, 32},
    {800,  416, 192, 32},
    {288,  448, 160, 32},
    {0,    576, 1920, 64},
};
#define LEVEL2_PLATFORM_COUNT (sizeof(level2Platforms)/sizeof(level2Platforms[0]))

void SetupLevels(Texture2D *bg1, Texture2D *bg2)
{
    levels[0] = (LevelDef){
        .platforms = level1Platforms, .platformCount = LEVEL1_PLATFORM_COUNT,
        .background = bg1,
        .levelWidth = 1920, .levelHeight = 640,
        .playerStartX = 100, .playerStartY = 300,
        .enemySpawns= {
            {420, 170, 80, 405, 505, false},
            {480, 360, -100, 320, 620, false},
            {680, 360, 90, 610, 760, false},
        },
        .enemyCount = 3
    };

    levels[1] = (LevelDef){
        .platforms = level2Platforms, .platformCount = LEVEL2_PLATFORM_COUNT,
        .background = bg2,
        .levelWidth = 1920, .levelHeight = 640,
        .playerStartX = 100, .playerStartY = 300,
        .enemySpawns= {
            {710,  56,  80, 710,  815, false},
            {1000, 88,  90, 1000, 1500, false},
            {300,  120, -70, 300, 525, false},
            {710,  216, 100, 710,  790, false},
            {1575, 216, -85, 1575, 1680, false},
            {515,  312, 95,  515,  595, false},
            {805,  376, -75, 805,  945, false},
            {295,  408, 110, 295,  405, false},
        },
        .enemyCount = 8
    };
}

void LoadLevel(int idx, Player *p, Enemy enemies[], int *enemyCountOut)
{
    LevelDef *lv = &levels[idx];

    currentBackground = lv->background;
    currentPlatforms = lv->platforms;
    currentPlatformCount = lv->platformCount;
    currentLevelWidth = lv->levelWidth;
    currentLevelHeight = lv->levelHeight;

    p->x = lv->playerStartX;
    p->y = lv->playerStartY;
    p->vx = 0; p->vy = 0;
    p->isOnGround = false;
    p->health = 15;

    *enemyCountOut = lv->enemyCount;
    for (int i = 0; i < lv->enemyCount; i++)
    {
        EnemySpawn *s = &lv->enemySpawns[i];
        enemies[i] = (Enemy){
            .x = s->x, .y = s->y, .vx = s->vx,
            .health = 3, .maxHealth = 3, .isAlive = true,
            .state = PATROLING,
            .patrolLeft = s->patrolLeft, .patrolRight = s->patrolRight,
            .elevated = s->elevated,
            .facing = (s->vx >= 0) ? 1 : -1
        };
    }
}
int main(void)
{
    InitWindow(800, 450, "Silhouette");

    InitAudioDevice();
    Music bgm = LoadMusicStream("music/wolf.mp3");
    SetMusicVolume(bgm, 0.0f);
    PlayMusicStream(bgm);

    Texture2D background1 = LoadTexture("assets/1st level.png");
    Texture2D background2 = LoadTexture("assets/2nd level.png");

    Font century = LoadFont("assets/anime-ace.regular.ttf");

    SetupLevels(&background1, &background2);

    PlayerSpriteSystem playerSprites;
    EnemySpriteSystem enemySprites;
    InitPlayerSprites(&playerSprites);
    InitEnemySprites(&enemySprites);

    SetTargetFPS(60);

    int enemyCount = 3;
    Enemy enemies[8];
    Player player;

    ResetGame(&player, enemies, enemyCount);
    LoadLevel(0, &player, enemies, &enemyCount);

    float speed = 200.0f;
    float gravity = 1000.0f;
    float attackDuration = 0.25f;
    float enemyAttackDur = 0.20f;
    float attackRange = 90.0f;
    float alertRange = 200.0f;
    float chaseSpeed = 90.0f;
    float enemyAttackCDMax = 0.8f;

    bool gameOver = false;
    bool gameWon = false;

    bool showMenu = true;
    int score = 0;

    Camera2D camera = {0};
    camera.offset = (Vector2){400, 225};
    camera.target = (Vector2){player.x, player.y};
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        UpdateMusicStream(bgm);

        float dt = GetFrameTime();

        if (showMenu)
        {
            if (IsKeyPressed(KEY_ENTER))
            {
                showMenu = false;
                score = 0;
                currentLevel = 0;
                ResetGame(&player, enemies, enemyCount);
                LoadLevel(currentLevel, &player, enemies, &enemyCount);
                camera.target = (Vector2){ player.x + 20.0f, player.y + 30.0f };
            }

            BeginDrawing();
            ClearBackground((Color){20, 20, 30, 255});

            const char *title = "SILHOUETTE";
            int titleFs = 60;
            int titleW = MeasureText(title, titleFs);
            DrawTextEx(century, title, (Vector2){(800 - titleW) / 2, 140}, titleFs, 1, (Color){230, 230, 230, 255});

            const char *prompt = "Press ENTER to start";
            int promptFs = 24;
            int promptW = MeasureText(prompt, promptFs);
            DrawTextEx(century, prompt, (Vector2){(800 - promptW) / 2, 240}, promptFs, 1, (Color){180, 180, 180, 255});

            EndDrawing();
            continue;
        }

        if ((gameOver || gameWon) && IsKeyPressed(KEY_R))
        {
        currentLevel = 0;

        score = 0;

        ResetGame(&player, enemies, enemyCount);

        LoadLevel(currentLevel, &player, enemies, &enemyCount);

        gameOver = false;
        gameWon = false;

        camera.target = (Vector2){ player.x + 20.0f, player.y + 30.0f };
        }

        if (!gameOver && !gameWon)
        {

           player.vx = 0;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
    {
        player.vx = speed;
        player.facing = 1;
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
    {
        player.vx = -speed;
        player.facing = -1;
    }
    player.x += player.vx * dt;

    ResolvePlatformCollisionX(&player.x, player.y, 40, 60, &player.vx, currentPlatforms, currentPlatformCount);

    if (player.x < 0)
        player.x = 0;

    if (IsKeyPressed(KEY_SPACE) && player.jumps_left > 0){
        player.vy = -500.0f;
        player.jumps_left--;}
        
    player.isOnGround = false;
    player.vy += gravity * dt;
    player.y += player.vy * dt;

    if (ResolvePlatformCollisionY(player.x, &player.y, 40, 60, &player.vy, currentPlatforms, currentPlatformCount))
    {
        player.isOnGround = true;
        player.jumps_left = player.max_jumps;
    }
    float mapPixelW = currentLevelWidth;
    float mapPixelH = currentLevelHeight;

    if (player.x < 0) player.x = 0;
    if (player.x + 40 > mapPixelW) player.x = mapPixelW - 40;

    if (player.y < 0) 
        { player.y = 0; 
        player.vy = 0; }
    if (player.y + 60 > mapPixelH) 
        { player.y = mapPixelH - 60; 
        player.vy = 0; player.isOnGround = true; 
        player.jumps_left = player.max_jumps; }

           if ((IsKeyPressed(KEY_Z) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) && !player.isAttacking && player.attackCooldown <= 0) 
            {
                player.isAttacking = true;
                player.attackTimer = attackDuration;
                player.hasHit = false;
                player.attackCooldown = 0.5f;
            }
            if (player.attackCooldown > 0)
            {
                player.attackCooldown -= dt;
            }
            if (player.isAttacking)
            {
                player.attackTimer -= dt;
                if (player.attackTimer <= 0)
                    player.isAttacking = false;
            }

            if (player.hitFlashTimer > 0)
                player.hitFlashTimer -= dt;
            if (player.invincibleTimer > 0)
                player.invincibleTimer -= dt;

            int aliveCount = 0;
            for (int i = 0; i < enemyCount; i++)
            {
                Enemy *e = &enemies[i];
                if (!e->isAlive)
                    continue;
                aliveCount++;
                UpdateEnemy(e, &player, dt, attackRange, alertRange, enemyAttackDur, enemyAttackCDMax, chaseSpeed);              


                if (player.isAttacking && !player.hasHit)
                {
                    Rectangle hitbox = GetAttackHitbox(player);
                    Rectangle enemyR = {e->x, e->y, 40, 40};
                    if (CheckCollisionRecs(hitbox, enemyR))
                    {
                        e->health--;
                        e->hitFlashTimer = 0.15f;
                        player.hasHit = true;
                        if (e->health <= 0)
                        {
                            e->isAlive = false;
                            score += 100;
                        }
                    }
                }
                Rectangle pRect = {player.x, player.y, 40, 60};
                Rectangle eRect = {e->x, e->y, 40, 40};
                if (CheckCollisionRecs(pRect, eRect) && player.invincibleTimer <= 0)
                {
                    player.health--;
                    player.hitFlashTimer = 0.35f;
                    player.invincibleTimer = 0.7f;
                    if (player.health <= 0)
                    {
                        player.health = 0;
                        gameOver = true;
                    }
                }

                if (e->state == ATTACKING)
{
    if (player.invincibleTimer <= 0)
    {
        Rectangle ehit = GetEnemyAttackHitbox(*e);
        if (CheckCollisionRecs(ehit, pRect))
        {
            player.health--;
            player.hitFlashTimer = 0.35f;
            player.invincibleTimer = 0.7f;
            if (player.health <= 0)
            {
                player.health = 0;
                gameOver = true;
            }
        }
    }
}
                   
                
            }
            Vector2 playerCenter = (Vector2){ player.x + 20.0f, player.y + 30.0f };
            camera.target.x += (playerCenter.x - camera.target.x) * 5.0f * dt;
            camera.target.y += (playerCenter.y - camera.target.y) * 5.0f * dt;
            float halfW = camera.offset.x / camera.zoom;
            float halfH = camera.offset.y / camera.zoom;

            if (camera.target.x < halfW) camera.target.x = halfW;
            if (camera.target.x > mapPixelW - halfW) camera.target.x = mapPixelW - halfW;
            if (camera.target.y < halfH) camera.target.y = halfH;
            if (camera.target.y > mapPixelH - halfH) camera.target.y = mapPixelH - halfH;
            if (aliveCount == 0)
            {
                if (currentLevel + 1 < LEVEL_COUNT)
                {
                    currentLevel++;
                    LoadLevel(currentLevel, &player, enemies, &enemyCount);
                }
                else
                {
                    gameWon = true;
                }
            }
        }

        UpdatePlayerSprites(&playerSprites, &player, dt);
        UpdateEnemySprites(&enemySprites, enemies, enemyCount, dt);

        BeginDrawing();

        ClearBackground(WHITE);

        
        BeginMode2D(camera);

        DrawTexture(*currentBackground, 0, 0, WHITE);

        for (int i = 0; i < enemyCount; i++)
        {
            Enemy *e = &enemies[i];
            if (!e->isAlive)
                continue;

           DrawEnemySprite(&enemySprites, i, e);

            DrawHealthBar(e->x, e->y - 16, 40, 9,
                          e->health, e->maxHealth,
                          (Color){230, 50, 50, 255},
                          (Color){60, 15, 15, 255});
        }

        DrawPlayerSprites(&playerSprites, &player);

        EndMode2D();

        DrawTextEx(century, "HP", (Vector2){10, 12}, 18, 1, RED);
        DrawHealthBar(40, 13, 160, 17, player.health, 15, (Color){70, 210, 100, 255}, (Color){40, 40, 40, 255});

        DrawTextEx(century, TextFormat("Score: %d", score), (Vector2){650, 15}, 20, 1, BLACK);

        if (gameWon || gameOver)
        {
            DrawRectangle(0, 0, 800, 450, (Color){0, 0, 0, 165});

            const char *msg = gameWon ? "Winner!" : "Game Over!";
            Color col = gameWon ? (Color){80, 255, 130, 255} : (Color){220, 50, 50, 255};
            int fs = 56;
            int tw = MeasureText(msg, fs);
            DrawTextEx(century, msg, (Vector2){(800 - tw) / 2, 155}, fs, 1, col);

            const char *sub = "Press  R  to play again";
            int stw = MeasureText(sub, 22);
            DrawTextEx(century, sub, (Vector2){(800 - stw) / 2, 240}, 22, 1, WHITE);

            const char *scoreMsg = TextFormat("Score: %d", score);
            int smw = MeasureText(scoreMsg, 20);
            DrawTextEx(century, scoreMsg, (Vector2){(800 - smw) / 2, 280}, 20, 1, (Color){220, 220, 220, 255});
        }

        EndDrawing();
    }

    UnloadPlayerSprites(&playerSprites);
    UnloadEnemySprites(&enemySprites);
    UnloadMusicStream(bgm);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}