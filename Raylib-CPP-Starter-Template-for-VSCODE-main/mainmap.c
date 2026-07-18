#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include "tilemap.h"



typedef struct
{
    float x, y;
    float vx, vy;
    bool isOnGround;
    bool isAttacking;
    float attackTimer;
    int facing;
    bool hasHit;
    int health;
    float hitFlashTimer;
    float invincibleTimer;
    float attackCooldown;
    int max_jumps;
    int jumps_left;
} Player;

typedef enum
{
    PATROLING,
    CHASE,
    ATTACKING,
    RETURN
} EnemyState;

typedef struct
{
    float x, y;
    float vx;
    float health;
    int maxHealth;
    bool isAlive;
    float hitFlashTimer;
    EnemyState state;        
    float patrolLeft;
    float patrolRight;
    float alertRange;
    float chaseSpeed;
    bool elevated;
    float attackTimer;
    float attackCooldown;
    int facing;
} Enemy;

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
    TraceLog(LOG_INFO, "state=%d dist=%.1f alertRange=%.1f", e->state, dist, alertRange);
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
    const char *tmjPath;
    const char *collisionLayer;
    const char *decorationLayer;
    int tilesetFirstgids[MAX_TILESETS];
    Texture2D *tilesetTextures[MAX_TILESETS];
    int tilesetRegCount;
    Texture2D *background;
    float playerStartX, playerStartY;
    EnemySpawn enemySpawns[8];
    int enemyCount;
} LevelDef;

#define LEVEL_COUNT 2
LevelDef levels[LEVEL_COUNT];
int currentLevel = 0;
Texture2D *currentBackground;

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

void DrawHealthBar(float x, float y, float w, float h,
                   float hp, float maxHp,
                   Color fill, Color bg)
{
    DrawRectangle((int)x, (int)y, (int)w, (int)h, bg);
    float ratio = hp / maxHp;
    if (ratio < 0)
        ratio = 0;
    DrawRectangle((int)x, (int)y, (int)(w * ratio), (int)h, fill);
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, WHITE);
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
   /* enemies[0] = (Enemy){
        .x = 420, .y = 170, .vx = 80, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .patrolLeft = 405, .patrolRight = 505, .isAttacking = false, .attackTimer = 0, .attackCooldown = 0, .facing = 1};
    enemies[1] = (Enemy){
        .x = 480, .y = 360, .vx = -100, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .patrolLeft = 320, .patrolRight = 620, .isAttacking = false, .attackTimer = 0, .attackCooldown = 0, .facing = -1};
    enemies[2] = (Enemy){
        .x = 680, .y = 360, .vx = 90, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .patrolLeft = 610, .patrolRight = 760, .isAttacking = false, .attackTimer = 0, .attackCooldown = 0, .facing = 1};*/
}
void SetupLevels(Texture2D *ground, Texture2D *tree, Texture2D *iceVungvang,
                  Texture2D *iceMountain2, Texture2D *bg1, Texture2D *bg2)
{
    levels[0] = (LevelDef){
        .tmjPath = "assets/1st level.tmj",
        .collisionLayer = "Collision", .decorationLayer = "Decorations",
        .tilesetFirstgids = {65, 385}, .tilesetTextures = {ground, tree}, .tilesetRegCount = 2,
        .background = bg1,
        .playerStartX = 100, .playerStartY = 300,
        .enemySpawns= {
            {420, 170, 80, 405, 505, false},
            {480, 360, -100, 320, 620, false},
            {680, 360, 90, 610, 760, false},
        },
        .enemyCount = 3
    };

    levels[1] = (LevelDef){
    .tmjPath = "assets/2nd level.tmj",
    .collisionLayer = "Collision", .decorationLayer = "Trees",
    .tilesetFirstgids = {1, 1001, 2001},
    .tilesetTextures = {iceVungvang, iceMountain2, tree}, .tilesetRegCount = 3,
    .background = bg2,
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

    ClearTilesets();
    for (int i = 0; i < lv->tilesetRegCount; i++)
        RegisterTileset(lv->tilesetFirstgids[i], *lv->tilesetTextures[i]);

    LoadMapFromTMJ(lv->tmjPath, lv->collisionLayer, lv->decorationLayer);
    currentBackground = lv->background;

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
    .alertRange = 200.0f,
    .chaseSpeed = 90.0f,
    .facing = (s->vx >= 0) ? 1 : -1
};
}
}
int main(void)
{
    InitWindow(800, 450, "Silhouette");
    Texture2D groundTileset = LoadTexture("assets/pixel_platform_01_tileset_final.png"); 
    Texture2D treeTileset = LoadTexture("assets/FreePack.png"); 
    Texture2D iceVungvang = LoadTexture("assets/Ice mountain vungvang.png");
    Texture2D iceMountain2 = LoadTexture("assets/Ice mountain tileset.png"); 
    Texture2D background1 = LoadTexture("assets/SUN AND MOUNTAIN 2.png");
    Texture2D background2 = LoadTexture("assets/ICY MOUNTAIN (1).png");

SetupLevels(&groundTileset, &treeTileset, &iceVungvang, &iceMountain2, &background1, &background2);
    TraceLog(LOG_INFO, "Map loaded: %d cols x %d rows", mapCols, mapRows);
    SetTargetFPS(60);

   

    /*int platformCount = 4;
    Rectangle platforms[] = {
        {0, 400, 800, 50},
        {150, 300, 150, 20},
        {400, 210, 150, 20},
        {650, 300, 150, 20}
    };*/
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
    Camera2D camera = {0};
    camera.offset = (Vector2){400, 225}; // screen center
    camera.target = (Vector2){player.x, player.y};
    camera.zoom = 1.0f;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

       if ((gameOver || gameWon) && IsKeyPressed(KEY_R))
        {
        currentLevel = 0;
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

ResolveTileCollisionX(&player.x, player.y, 40, 60, &player.vx);

if (player.x < 0)
    player.x = 0;

if (IsKeyPressed(KEY_SPACE) && player.jumps_left > 0){
    player.vy = -500.0f;
    player.jumps_left--;}
player.isOnGround = false;
player.vy += gravity * dt;
player.y += player.vy * dt;

if (ResolveTileCollisionY(player.x, &player.y, 40, 60, &player.vy))
{
    player.isOnGround = true;
    player.jumps_left = player.max_jumps;
}
float mapPixelW = mapCols * (float)TILE_SIZE;
float mapPixelH = mapRows * (float)TILE_SIZE;

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
                            e->isAlive = false;
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

        BeginDrawing();
        //ClearBackground((Color){24, 24, 36, 255});
        ClearBackground(WHITE);

        
        BeginMode2D(camera);
        DrawTexture(*currentBackground, 0, 0, WHITE);
        DrawMap(); 
      
        /*for (int i = 0; i < platformCount; i++)
            DrawRectangleRec(platforms[i], (Color){70, 70, 95, 255});*/

        for (int i = 0; i < enemyCount; i++)
        {
            Enemy *e = &enemies[i];
            if (!e->isAlive)
                continue;

            Color bodyCol = (e->hitFlashTimer > 0) ? WHITE : RED;
            DrawRectangle((int)e->x, (int)e->y, 40, 40, bodyCol);

            float eyeX = (e->facing == 1) ? e->x + 30 : e->x + 10;
            DrawCircle((int)eyeX, (int)e->y + 10, 5, (Color){20, 20, 20, 255});

            if (e->state == ATTACKING)
{
    Rectangle ehit = GetEnemyAttackHitbox(*e);
    DrawRectangleRec(ehit, (Color){160, 0, 0, 210});
}

            DrawHealthBar(e->x, e->y - 16, 40, 9,
                          e->health, e->maxHealth,
                          (Color){230, 50, 50, 255},
                          (Color){60, 15, 15, 255});
        }

        bool flashOn = (player.hitFlashTimer > 0 && (int)(player.hitFlashTimer * 14) % 2 == 0);
        DrawRectangle((int)player.x, (int)player.y, 40, 60,
                      flashOn ? (Color){255, 80, 80, 255} : BLUE);

        DrawCircle((int)player.x + (player.facing == 1 ? 30 : 10), (int)player.y + 10, 5, (Color){20, 20, 20, 255});

        if (player.isAttacking)
        {
            Rectangle hitbox = GetAttackHitbox(player);
            DrawRectangleRec(hitbox, Fade(YELLOW, 0.55f));
        }
        EndMode2D();
        DrawText("HP", 10, 12, 18, WHITE);
        DrawHealthBar(40, 13, 160, 17,
                      player.health, 15,
                      (Color){70, 210, 100, 255},
                      (Color){40, 40, 40, 255});

        DrawText("Arrows: Move  |  Space: Jump  |  Z/LMB: Attack  |  R: Restart",
                 10, 430, 13, (Color){140, 140, 140, 255});

        if (gameWon || gameOver)
        {
            DrawRectangle(0, 0, 800, 450, (Color){0, 0, 0, 165});

            const char *msg = gameWon ? "Winner!" : "Game Over!";
            Color col = gameWon ? (Color){80, 255, 130, 255} : (Color){220, 50, 50, 255};
            int fs = 56;
            int tw = MeasureText(msg, fs);
            DrawText(msg, (800 - tw) / 2, 155, fs, col);

            const char *sub = "Press  R  to play again";
            int stw = MeasureText(sub, 22);
            DrawText(sub, (800 - stw) / 2, 240, 22, WHITE);
        }
           

        EndDrawing();
    }

    CloseWindow();
    return 0;
}