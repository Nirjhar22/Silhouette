#include <raylib.h>
#include <math.h>

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

DrawText(TextFormat("%.0f", dist/10), e->x, e->y-50, 30, WHITE);
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

void ResetGame(Player *p, Enemy enemies[])
{
    p->x = 100;
    p->y = 300;
    p->vx = 0;
    p->vy = 0;
    p->isOnGround = false;
    p->isAttacking = false;
    p->attackTimer = 0;
    p->facing = 1;
    p->hasHit = false;
    p->health = 10;
    p->hitFlashTimer = 0;
    p->invincibleTimer = 0;

    enemies[0] = (Enemy){
        .x = 420, .y = 170, .vx = 80, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .state = PATROLING, .patrolLeft = 400, .patrolRight = 550, .elevated = true, .attackTimer = 0, .attackCooldown = 0, .facing = 1};
    enemies[1] = (Enemy){
        .x = 480, .y = 360, .vx = -100, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .state = PATROLING, .patrolLeft = 320, .patrolRight = 620, .elevated = false, .attackTimer = 0, .attackCooldown = 0, .facing = -1};
    enemies[2] = (Enemy){
        .x = 680, .y = 360, .vx = 90, .health = 3, .maxHealth = 3, .isAlive = true, .hitFlashTimer = 0, .state = PATROLING, .patrolLeft = 610, .patrolRight = 760, .elevated = false, .attackTimer = 0, .attackCooldown = 0, .facing = 1};
}

int main(void)
{
    InitWindow(800, 450, "Silhouette");
    SetTargetFPS(60);

    int platformCount = 4;
    Rectangle platforms[] = {
        {0, 400, 800, 50},
        {150, 300, 150, 20},
        {400, 210, 150, 20},
        {600, 320, 150, 20}
    };

    int enemyCount = 3;
    Enemy enemies[3];
    Player player;
    ResetGame(&player, enemies);

    float speed = 200.0f;
    float gravity = 1000.0f;
    float attackDuration = 0.25f;
    float enemyAttackDur = 0.20f;
    float attackRange = 90.0f;
    float enemyAttackCDMax = 0.8f;
    float alertRange = 200.0f;
    float chaseSpeed = 90.0f;

    bool gameOver = false;
    bool gameWon = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if ((gameOver || gameWon) && (IsKeyPressed(KEY_R) || (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))))
        {
            ResetGame(&player, enemies);
            gameOver = false;
            gameWon = false;
        }

        if (!gameOver && !gameWon)
        {
            player.vx = 0;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) || (IsGamepadAvailable(0) && GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > 0.2f))
            {
                player.vx = speed;
                player.facing = 1;
            }
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || (IsGamepadAvailable(0) && GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -0.2f))
            {
                player.vx = -speed;
                player.facing = -1;
            }
            player.x += player.vx * dt;

            Rectangle pr = {player.x, player.y, 40, 60};
            for (int i = 1; i < platformCount; i++)
            {
                if (CheckCollisionRecs(pr, platforms[i]))
                {
                    if (player.vx > 0)
                        player.x = platforms[i].x - 40;
                    else
                        player.x = platforms[i].x + platforms[i].width;
                    player.vx = 0;
                }
            }
            if (player.x < 0)
                player.x = 0;
            if (player.x + 40 > 800)
                player.x = 760;

            if (player.isOnGround && (IsKeyPressed(KEY_SPACE) || (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))))
                player.vy = -500.0f;
            player.isOnGround = false;
            player.vy += gravity * dt;
            player.y += player.vy * dt;

            pr = (Rectangle){player.x, player.y, 40, 60};
            for (int i = 0; i < platformCount; i++)
            {
                if (CheckCollisionRecs(pr, platforms[i]))
                {
                    if (player.vy > 0)
                    {
                        player.y = platforms[i].y - 60;
                        player.vy = 0;
                        player.isOnGround = true;
                    }
                    else
                    {
                        player.y = platforms[i].y + platforms[i].height;
                        player.vy = 0;
                    }
                }
            }

            if ((!player.isAttacking && player.attackCooldown <= 0) && (IsKeyPressed(KEY_Z) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1))))
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
                    Rectangle ehit = GetEnemyAttackHitbox(*e);
                    if (player.invincibleTimer <= 0 && CheckCollisionRecs(ehit, pRect))
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
                    if (e->attackTimer <= 0)
                    {
                        e->state = CHASE;
                        e->attackCooldown = enemyAttackCDMax;
                    }
                }

            if (aliveCount == 0)
                gameWon = true;
            }
        }

        BeginDrawing();
        ClearBackground((Color){24, 24, 36, 255});

        for (int i = 0; i < platformCount; i++)
            DrawRectangleRec(platforms[i], (Color){70, 70, 95, 255});

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

        DrawText("HP", 10, 12, 18, WHITE);
        DrawHealthBar(40, 13, 160, 17,
                      player.health, 5,
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