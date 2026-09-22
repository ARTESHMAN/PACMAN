#include <raylib.h>
#include <raymath.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 720

#define MAP_COLS 19
#define MAP_ROWS 15
#define TILE_SIZE 40
#define PACMAN_SIZE 32
#define OFFSET_X ((SCREEN_WIDTH - (MAP_COLS * TILE_SIZE)) / 2)
#define OFFSET_Y 86

#define MAX_RECORDS 100
#define MAX_NAME_LEN 24
#define MAX_PARTICLES 200
#define MAX_FLOATING_TEXTS 25
#define MAX_GHOSTS 4
#define TOTAL_LEVELS 3
#define MAX_BONUS_ITEMS 4

#define PELLET_SCORE 10
#define POWER_BERRY_SCORE 50
#define LEVEL_CLEAR_BASE_SCORE 1000
#define GHOST_SCORE_START 200

#define PACMAN_DEFAULT_SPEED 140.0f
#define PACMAN_BOOST_SPEED 220.0f
#define PACMAN_BOOST_DURATION 7.0f

#define GHOST_BASE_SPEED 95.0f
#define GHOST_EATEN_MULTIPLIER 1.8f
#define GHOST_FRIGHTENED_MULTIPLIER 0.55f
#define FRIGHTENED_DURATION 8.5f

#define BONUS_RESPAWN_TIME 18.0f

#define COLOR_BG ((Color){6, 8, 18, 255})
#define COLOR_PANEL ((Color){8, 10, 22, 255})
#define COLOR_WALL ((Color){12, 18, 52, 255})
#define COLOR_WALL_EDGE ((Color){42, 85, 240, 255})
#define COLOR_FLOOR ((Color){5, 7, 16, 255})

typedef enum {
    STATE_MENU,
    STATE_NAME_INPUT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_RECORDS
} GameState;

typedef enum {
    GHOST_BLINKY = 0,
    GHOST_PINKY,
    GHOST_INKY,
    GHOST_CLYDE
} GhostType;

typedef enum {
    ITEM_PEPPER = 1,
    ITEM_APPLE = 2,
    ITEM_MUSHROOM = 3,
    ITEM_BERRY = 4
} PowerItemType;

typedef struct {
    char name[MAX_NAME_LEN];
    int score;
    int level;
    char date[20];
} Record;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float alpha;
    float life;
    bool active;
} Particle;

typedef struct {
    Vector2 pos;
    char text[32];
    Color color;
    float alpha;
    float timer;
    bool active;
} FloatingText;

typedef struct {
    Vector2 pos;
    int col;
    int row;
    int targetCol;
    int targetRow;
    Vector2 dir;
    Vector2 nextDir;
    float speed;
    float defaultSpeed;
    float boostSpeed;
    float boostTimer;
    float mouthAngle;
    float mouthSpeed;
    float rotation;
    int lives;
} Pacman;

typedef struct {
    GhostType type;
    Vector2 pos;
    int col;
    int row;
    int targetCol;
    int targetRow;
    Vector2 dir;
    float speed;
    bool isFrightened;
    bool isEaten;
    Color defaultColor;
} Ghost;

typedef struct {
    Vector2 pos;
    PowerItemType type;
    bool active;
    float timer;
} PowerItem;

static GameState currentState = STATE_MENU;
static int currentLevel = 0;
static int score = 0;
static int ghostComboScore = GHOST_SCORE_START;
static float frightenedTimer = 0.0f;
static float screenShakeTime = 0.0f;
static char playerName[MAX_NAME_LEN] = "ARTESHMAN";
static int menuSelection = 0;
static bool audioMuted = false;
static bool applicationRunning = true;

static Record records[MAX_RECORDS];
static int recordCount = 0;

static Pacman pacman;
static Ghost ghosts[MAX_GHOSTS];
static PowerItem bonusItems[MAX_BONUS_ITEMS];
static Particle particles[MAX_PARTICLES];
static FloatingText floatingTexts[MAX_FLOATING_TEXTS];

static int totalPellets = 0;
static int pelletsRemaining = 0;
static char activeMap[MAP_ROWS][MAP_COLS + 1];
static int playerSpawnCol = 9;
static int playerSpawnRow = 11;

static Texture2D texPacman;
static Texture2D texGhost;
static Texture2D texGhostChaser;
static Texture2D texPizza;
static Texture2D texPepper;
static Texture2D texApple;
static Texture2D texBerry;
static Texture2D texMushroom;
static Texture2D texBackground;

static Music musicMenu;
static Music musicGame;
static bool hasMenuMusic = false;
static bool hasGameMusic = false;

static const char MAP_PRESETS[TOTAL_LEVELS][MAP_ROWS][MAP_COLS + 1] = {
    {
        "###################",
        "#o.......#.......o#",
        "#.###.##.#.##.###.#",
        "#.#.............#.#",
        "#.###.#.###.#.###.#",
        "#.....#.....#.....#",
        "#####.##   ##.#####",
        "     ...   ...     ",
        "#####.#######.#####",
        "#........#........#",
        "#.###.##.#.##.###.#",
        "#...#....P....#...#",
        "###.#.#.###.#.#.###",
        "#o....#.....#....o#",
        "###################"
    },
    {
        "###################",
        "#o......###......o#",
        "#.#####.....#####.#",
        "#.#...#######...#.#",
        "#.#.#....#....#.#.#",
        "#...#.#     #.#...#",
        "###.#.#     #.#.###",
        "    ...  P  ...    ",
        "###.#.#######.#.###",
        "#...#.........#...#",
        "#.###.#######.###.#",
        "#.#.............#.#",
        "#.###.##.#.##.###.#",
        "#o.......#.......o#",
        "###################"
    },
    {
        "###################",
        "#o..#.........#..o#",
        "###.#.#######.#.###",
        "#...#....#....#...#",
        "#.#####.###.#####.#",
        "#.#.............#.#",
        "#.#.###.   .###.#.#",
        " ...#    P    #... ",
        "#.#.###.   .###.#.#",
        "#.#.............#.#",
        "#.#####.###.#####.#",
        "#...#.........#...#",
        "###.#.#######.#.###",
        "#o..#.........#..o#",
        "###################"
    }
};

static const Vector2 GHOST_SPAWNS[MAX_GHOSTS] = {
    {9, 6}, {8, 6}, {10, 6}, {9, 7}
};

static const Color GHOST_COLORS[MAX_GHOSTS] = {
    RED, PINK, SKYBLUE, ORANGE
};

static int WrapColumn(int col) {
    if (col < 0) return MAP_COLS - 1;
    if (col >= MAP_COLS) return 0;
    return col;
}

static bool IsInsideMap(int col, int row) {
    return row >= 0 && row < MAP_ROWS && col >= 0 && col < MAP_COLS;
}

static bool IsTileWalkable(int col, int row) {
    if (row < 0 || row >= MAP_ROWS) return false;
    if (col < 0 || col >= MAP_COLS) return true;
    return activeMap[row][col] != '#';
}

static Vector2 GetTileCenter(int col, int row) {
    col = WrapColumn(col);
    return (Vector2){
        OFFSET_X + col * TILE_SIZE + TILE_SIZE * 0.5f,
        OFFSET_Y + row * TILE_SIZE + TILE_SIZE * 0.5f
    };
}

static bool IsOppositeDir(Vector2 a, Vector2 b) {
    return (a.x == -b.x && a.y == -b.y) && (a.x != 0.0f || a.y != 0.0f);
}

static bool IsZeroDir(Vector2 dir) {
    return dir.x == 0.0f && dir.y == 0.0f;
}

static void GetDateString(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm localTime = {0};
    struct tm *timeInfo = localtime(&now);

    if (timeInfo) localTime = *timeInfo;
    strftime(buffer, size, "%Y-%m-%d", &localTime);
}

static void ResetVisualEffects(void) {
    memset(particles, 0, sizeof(particles));
    memset(floatingTexts, 0, sizeof(floatingTexts));
    screenShakeTime = 0.0f;
}

static void SpawnParticles(Vector2 position, Color color, int count, float speed) {
    int spawned = 0;

    for (int i = 0; i < MAX_PARTICLES && spawned < count; ++i) {
        if (particles[i].active) continue;

        particles[i].active = true;
        particles[i].pos = position;
        float angle = (float)GetRandomValue(0, 359) * DEG2RAD;
        float minSpeed = fmaxf(20.0f, speed * 0.25f);
        float randomSpeed = GetRandomValue((int)minSpeed, (int)fmaxf(minSpeed + 1.0f, speed));
        particles[i].vel = (Vector2){cosf(angle) * randomSpeed, sinf(angle) * randomSpeed};
        particles[i].color = color;
        particles[i].size = (float)GetRandomValue(2, 5);
        particles[i].alpha = 1.0f;
        particles[i].life = (float)GetRandomValue(30, 55) / 60.0f;
        ++spawned;
    }
}

static void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;

        particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, dt));
        particles[i].vel = Vector2Scale(particles[i].vel, 0.97f);
        particles[i].life -= dt;
        particles[i].alpha = fmaxf(0.0f, particles[i].life);

        if (particles[i].life <= 0.0f) {
            particles[i].active = false;
        }
    }
}

static void DrawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;
        DrawCircleV(particles[i].pos, particles[i].size, Fade(particles[i].color, particles[i].alpha));
    }
}

static void AddFloatingText(Vector2 position, const char *text, Color color) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; ++i) {
        if (floatingTexts[i].active) continue;

        floatingTexts[i].active = true;
        floatingTexts[i].pos = position;
        snprintf(floatingTexts[i].text, sizeof(floatingTexts[i].text), "%s", text);
        floatingTexts[i].color = color;
        floatingTexts[i].alpha = 1.0f;
        floatingTexts[i].timer = 1.0f;
        return;
    }
}

static void UpdateAndDrawFloatingTexts(float dt) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; ++i) {
        if (!floatingTexts[i].active) continue;

        floatingTexts[i].pos.y -= 28.0f * dt;
        floatingTexts[i].timer -= dt;
        floatingTexts[i].alpha = fmaxf(0.0f, floatingTexts[i].timer);

        int width = MeasureText(floatingTexts[i].text, 18);
        DrawText(floatingTexts[i].text,
                 (int)floatingTexts[i].pos.x - width / 2,
                 (int)floatingTexts[i].pos.y,
                 18,
                 Fade(floatingTexts[i].color, floatingTexts[i].alpha));

        if (floatingTexts[i].timer <= 0.0f) {
            floatingTexts[i].active = false;
        }
    }
}

static int RecordCompare(const void *a, const void *b) {
    const Record *ra = (const Record *)a;
    const Record *rb = (const Record *)b;

    if (ra->score < rb->score) return 1;
    if (ra->score > rb->score) return -1;
    if (ra->level < rb->level) return 1;
    if (ra->level > rb->level) return -1;
    return strcmp(ra->date, rb->date);
}

static void LoadRecords(void) {
    recordCount = 0;

    FILE *file = fopen("records.txt", "r");
    if (!file) return;

    while (recordCount < MAX_RECORDS) {
        Record record = {0};
        if (fscanf(file, "%23s %d %d %19s",
                   record.name,
                   &record.score,
                   &record.level,
                   record.date) != 4) {
            break;
        }
        records[recordCount++] = record;
    }

    fclose(file);
    qsort(records, (size_t)recordCount, sizeof(records[0]), RecordCompare);
}

static void SaveRecord(const char *name, int finalScore, int level) {
    LoadRecords();

    if (recordCount < MAX_RECORDS) {
        Record *record = &records[recordCount++];
        memset(record, 0, sizeof(*record));
        snprintf(record->name, sizeof(record->name), "%s", (name && name[0]) ? name : "PILOT");
        record->score = finalScore;
        record->level = level;
        GetDateString(record->date, sizeof(record->date));
    } else if (finalScore > records[recordCount - 1].score) {
        Record *record = &records[recordCount - 1];
        snprintf(record->name, sizeof(record->name), "%s", (name && name[0]) ? name : "PILOT");
        record->score = finalScore;
        record->level = level;
        GetDateString(record->date, sizeof(record->date));
    } else {
        return;
    }

    qsort(records, (size_t)recordCount, sizeof(records[0]), RecordCompare);

    FILE *file = fopen("records.txt", "w");
    if (!file) return;

    for (int i = 0; i < recordCount; ++i) {
        fprintf(file, "%s %d %d %s\n",
                records[i].name,
                records[i].score,
                records[i].level,
                records[i].date);
    }

    fclose(file);
}

static bool IsReservedBonusTile(int col, int row) {
    return col >= 7 && col <= 11 && row >= 5 && row <= 7;
}

static bool IsSafeBonusPosition(Vector2 pos) {
    float minDistance = TILE_SIZE * 1.6f;
    if (Vector2Distance(pos, pacman.pos) < minDistance) return false;

    for (int i = 0; i < MAX_GHOSTS; ++i) {
        if (Vector2Distance(pos, ghosts[i].pos) < minDistance) return false;
    }

    for (int i = 0; i < MAX_BONUS_ITEMS; ++i) {
        if (bonusItems[i].active && Vector2Distance(pos, bonusItems[i].pos) < TILE_SIZE * 1.25f) {
            return false;
        }
    }

    return true;
}

static Vector2 GetRandomWalkableCenter(void) {
    for (int attempt = 0; attempt < 600; ++attempt) {
        int col = GetRandomValue(0, MAP_COLS - 1);
        int row = GetRandomValue(1, MAP_ROWS - 2);

        if (!IsTileWalkable(col, row)) continue;
        if (IsReservedBonusTile(col, row)) continue;

        Vector2 candidate = GetTileCenter(col, row);
        if (IsSafeBonusPosition(candidate)) return candidate;
    }

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            if (!IsTileWalkable(col, row) || IsReservedBonusTile(col, row)) continue;
            return GetTileCenter(col, row);
        }
    }

    return GetTileCenter(1, 1);
}

static void CopyCurrentMap(void) {
    for (int row = 0; row < MAP_ROWS; ++row) {
        memcpy(activeMap[row], MAP_PRESETS[currentLevel][row], MAP_COLS + 1);
    }
}

static void RespawnAllFood(void) {
    totalPellets = 0;
    pelletsRemaining = 0;

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            char tile = activeMap[row][col];
            if (tile == '.' || tile == 'o') {
                ++totalPellets;
            }
        }
    }

    pelletsRemaining = totalPellets;

    for (int i = 0; i < MAX_BONUS_ITEMS; ++i) {
        bonusItems[i].type = (PowerItemType)(i + 1);
        bonusItems[i].active = true;
        bonusItems[i].timer = BONUS_RESPAWN_TIME + i * 2.0f;
        bonusItems[i].pos = GetRandomWalkableCenter();
    }
}

static void ResetEntityToSpawn(Pacman *p) {
    p->col = playerSpawnCol;
    p->row = playerSpawnRow;
    p->targetCol = playerSpawnCol;
    p->targetRow = playerSpawnRow;
    p->pos = GetTileCenter(playerSpawnCol, playerSpawnRow);
    p->dir = (Vector2){0, 0};
    p->nextDir = (Vector2){0, 0};
    p->boostTimer = 0.0f;
    p->speed = p->defaultSpeed;
    p->rotation = 0.0f;
}

static void ResetGhosts(void) {
    for (int i = 0; i < MAX_GHOSTS; ++i) {
        Ghost *g = &ghosts[i];
        g->type = (GhostType)i;
        g->col = (int)GHOST_SPAWNS[i].x;
        g->row = (int)GHOST_SPAWNS[i].y;
        g->targetCol = g->col;
        g->targetRow = g->row - 1;
        g->pos = GetTileCenter(g->col, g->row);
        g->dir = (Vector2){0, -1};
        g->speed = GHOST_BASE_SPEED + currentLevel * 10.0f;
        g->isFrightened = false;
        g->isEaten = false;
        g->defaultColor = GHOST_COLORS[i];
    }
}

static void InitLevel(int levelIndex) {
    int preservedLives = pacman.lives;
    if (preservedLives <= 0) preservedLives = 3;

    currentLevel = ((levelIndex % TOTAL_LEVELS) + TOTAL_LEVELS) % TOTAL_LEVELS;
    CopyCurrentMap();

    playerSpawnCol = 9;
    playerSpawnRow = 11;

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            if (activeMap[row][col] == 'P') {
                playerSpawnCol = col;
                playerSpawnRow = row;
                activeMap[row][col] = ' ';
            }
        }
    }

    memset(&pacman, 0, sizeof(pacman));
    pacman.defaultSpeed = PACMAN_DEFAULT_SPEED;
    pacman.boostSpeed = PACMAN_BOOST_SPEED;
    pacman.speed = pacman.defaultSpeed;
    pacman.mouthAngle = 10.0f;
    pacman.mouthSpeed = 250.0f;
    pacman.lives = preservedLives;
    ResetEntityToSpawn(&pacman);

    ResetGhosts();

    frightenedTimer = 0.0f;
    ghostComboScore = GHOST_SCORE_START;
    ResetVisualEffects();
    RespawnAllFood();
}

static void StartNewGame(void) {
    score = 0;
    pacman.lives = 3;
    InitLevel(0);
}

static bool CanMoveFrom(int col, int row, Vector2 dir) {
    if (IsZeroDir(dir)) return false;
    return IsTileWalkable(col + (int)dir.x, row + (int)dir.y);
}

static void AdvancePacmanTarget(void) {
    int nextCol = WrapColumn(pacman.col + (int)pacman.dir.x);
    int nextRow = pacman.row + (int)pacman.dir.y;

    if (IsTileWalkable(nextCol, nextRow)) {
        pacman.targetCol = nextCol;
        pacman.targetRow = nextRow;
    } else {
        pacman.dir = (Vector2){0, 0};
        pacman.targetCol = pacman.col;
        pacman.targetRow = pacman.row;
    }
}

static void UpdatePacman(float dt) {
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) pacman.nextDir = (Vector2){1, 0};
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) pacman.nextDir = (Vector2){-1, 0};
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) pacman.nextDir = (Vector2){0, -1};
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) pacman.nextDir = (Vector2){0, 1};

    pacman.mouthAngle += pacman.mouthSpeed * dt;
    if (pacman.mouthAngle > 55.0f || pacman.mouthAngle < 8.0f) {
        pacman.mouthSpeed = -pacman.mouthSpeed;
    }

    if (pacman.boostTimer > 0.0f) {
        pacman.boostTimer = fmaxf(0.0f, pacman.boostTimer - dt);
        pacman.speed = pacman.boostSpeed;
    } else {
        pacman.speed = pacman.defaultSpeed;
    }

    if (IsOppositeDir(pacman.nextDir, pacman.dir)) {
        // Reverse without teleporting: swap the current segment endpoints and keep the
        // exact interpolated position. This preserves smooth 180-degree turns.
        int oldCol = pacman.col;
        int oldRow = pacman.row;
        pacman.col = WrapColumn(pacman.targetCol);
        pacman.row = pacman.targetRow;
        pacman.targetCol = oldCol;
        pacman.targetRow = oldRow;
        pacman.dir = pacman.nextDir;
    }

    if (IsZeroDir(pacman.dir)) {
        if (CanMoveFrom(pacman.col, pacman.row, pacman.nextDir)) {
            pacman.dir = pacman.nextDir;
            pacman.targetCol = WrapColumn(pacman.col + (int)pacman.dir.x);
            pacman.targetRow = pacman.row + (int)pacman.dir.y;
        }
        return;
    }

    Vector2 targetPos = GetTileCenter(pacman.targetCol, pacman.targetRow);
    float step = pacman.speed * dt;
    float distance = Vector2Distance(pacman.pos, targetPos);

    if (distance <= step) {
        pacman.pos = targetPos;
        pacman.col = WrapColumn(pacman.targetCol);
        pacman.row = pacman.targetRow;

        if (!IsZeroDir(pacman.nextDir) && CanMoveFrom(pacman.col, pacman.row, pacman.nextDir)) {
            pacman.dir = pacman.nextDir;
        }
        AdvancePacmanTarget();
    } else {
        Vector2 move = Vector2Normalize(Vector2Subtract(targetPos, pacman.pos));
        pacman.pos = Vector2Add(pacman.pos, Vector2Scale(move, step));
    }

    if (pacman.dir.x > 0) pacman.rotation = 0.0f;
    else if (pacman.dir.x < 0) pacman.rotation = 180.0f;
    else if (pacman.dir.y > 0) pacman.rotation = 90.0f;
    else if (pacman.dir.y < 0) pacman.rotation = 270.0f;
}

static Vector2 GetGhostTargetTile(const Ghost *g) {
    if (g->isEaten) return (Vector2){9.0f, 6.0f};

    if (g->isFrightened) {
        int attempts = 0;
        while (attempts++ < 30) {
            int c = GetRandomValue(0, MAP_COLS - 1);
            int r = GetRandomValue(0, MAP_ROWS - 1);
            if (IsTileWalkable(c, r)) return (Vector2){(float)c, (float)r};
        }
        return (Vector2){(float)pacman.col, (float)pacman.row};
    }

    switch (g->type) {
        case GHOST_BLINKY:
            return (Vector2){(float)pacman.col, (float)pacman.row};

        case GHOST_PINKY: {
            int c = pacman.col + (int)pacman.dir.x * 3;
            int r = pacman.row + (int)pacman.dir.y * 3;
            return (Vector2){(float)WrapColumn(c), (float)Clamp(r, 0, MAP_ROWS - 1)};
        }

        case GHOST_INKY: {
            float dist = Vector2Distance(g->pos, pacman.pos);
            if (dist > 5.0f * TILE_SIZE) {
                return (Vector2){(float)pacman.col, (float)pacman.row};
            }
            return (Vector2){1.0f, 1.0f};
        }

        case GHOST_CLYDE: {
            float dist = Vector2Distance(g->pos, pacman.pos);
            if (dist > 6.0f * TILE_SIZE) {
                return (Vector2){(float)pacman.col, (float)pacman.row};
            }
            return (Vector2){1.0f, (float)(MAP_ROWS - 2)};
        }
    }

    return (Vector2){(float)pacman.col, (float)pacman.row};
}

static int TileIndex(int col, int row) {
    return row * MAP_COLS + col;
}

static int GetShortestPathNextDirection(const Ghost *ghost, Vector2 targetTile) {
    int startCol = WrapColumn(ghost->col);
    int startRow = ghost->row;
    int targetCol = WrapColumn((int)targetTile.x);
    int targetRow = Clamp((int)targetTile.y, 0, MAP_ROWS - 1);

    if (!IsInsideMap(startCol, startRow) || !IsTileWalkable(startCol, startRow)) return -1;
    if (!IsTileWalkable(targetCol, targetRow)) targetCol = startCol, targetRow = startRow;
    if (startCol == targetCol && startRow == targetRow) return -1;

    bool visited[MAP_ROWS][MAP_COLS] = {0};
    int firstDir[MAP_ROWS * MAP_COLS];
    for (int i = 0; i < MAP_ROWS * MAP_COLS; ++i) {
        firstDir[i] = -1;
    }

    const Vector2 dirs[4] = {
        {0, -1}, {-1, 0}, {0, 1}, {1, 0}
    };

    int queue[MAP_ROWS * MAP_COLS];
    int head = 0;
    int tail = 0;

    int startIndex = TileIndex(startCol, startRow);
    visited[startRow][startCol] = true;
    queue[tail++] = startIndex;

    while (head < tail) {
        int current = queue[head++];
        int currentRow = current / MAP_COLS;
        int currentCol = current % MAP_COLS;

        if (currentCol == targetCol && currentRow == targetRow) break;

        for (int d = 0; d < 4; ++d) {
            int nextCol = WrapColumn(currentCol + (int)dirs[d].x);
            int nextRow = currentRow + (int)dirs[d].y;
            if (!IsInsideMap(nextCol, nextRow)) continue;
            if (!IsTileWalkable(nextCol, nextRow)) continue;
            if (visited[nextRow][nextCol]) continue;

            visited[nextRow][nextCol] = true;
            int nextIndex = TileIndex(nextCol, nextRow);
            firstDir[nextIndex] = (current == startIndex) ? d : firstDir[current];
            queue[tail++] = nextIndex;
        }
    }

    int targetIndex = TileIndex(targetCol, targetRow);
    if (!visited[targetRow][targetCol] || firstDir[targetIndex] < 0) return -1;
    return firstDir[targetIndex];
}

static void SetGhostNextMove(Ghost *g) {
    Vector2 target = GetGhostTargetTile(g);
    int preferred = GetShortestPathNextDirection(g, target);

    const Vector2 dirs[4] = {
        {0, -1}, {-1, 0}, {0, 1}, {1, 0}
    };

    if (preferred >= 0) {
        Vector2 preferredDir = dirs[preferred];
        bool isReverse = IsOppositeDir(preferredDir, g->dir);
        bool hasAlternative = false;

        for (int d = 0; d < 4; ++d) {
            if (d == preferred) continue;
            if (IsOppositeDir(dirs[d], g->dir)) continue;
            if (CanMoveFrom(g->col, g->row, dirs[d])) {
                hasAlternative = true;
                break;
            }
        }

        if (!isReverse || !hasAlternative) {
            g->dir = preferredDir;
        }
    }

    if (!CanMoveFrom(g->col, g->row, g->dir)) {
        for (int d = 0; d < 4; ++d) {
            if (CanMoveFrom(g->col, g->row, dirs[d])) {
                g->dir = dirs[d];
                break;
            }
        }
    }

    if (CanMoveFrom(g->col, g->row, g->dir)) {
        g->targetCol = WrapColumn(g->col + (int)g->dir.x);
        g->targetRow = g->row + (int)g->dir.y;
    } else {
        g->targetCol = g->col;
        g->targetRow = g->row;
    }
}

static void ResetAfterLifeLost(void) {
    ResetEntityToSpawn(&pacman);
    ResetGhosts();
    frightenedTimer = 0.0f;
    ghostComboScore = GHOST_SCORE_START;
}

static void CollectPellets(void) {
    int col = pacman.col;
    int row = pacman.row;
    if (!IsInsideMap(col, row)) return;

    if (activeMap[row][col] == '.') {
        activeMap[row][col] = ' ';
        score += PELLET_SCORE;
        --pelletsRemaining;
        SpawnParticles(pacman.pos, YELLOW, 2, 25.0f);
    } else if (activeMap[row][col] == 'o') {
        activeMap[row][col] = ' ';
        score += POWER_BERRY_SCORE;
        --pelletsRemaining;
        frightenedTimer = FRIGHTENED_DURATION;
        ghostComboScore = GHOST_SCORE_START;

        for (int i = 0; i < MAX_GHOSTS; ++i) {
            if (!ghosts[i].isEaten) ghosts[i].isFrightened = true;
        }

        AddFloatingText(pacman.pos, "GHOST HUNT!", SKYBLUE);
        SpawnParticles(pacman.pos, SKYBLUE, 25, 80.0f);
    }
}

static void UpdateBonusItems(float dt) {
    for (int i = 0; i < MAX_BONUS_ITEMS; ++i) {
        PowerItem *item = &bonusItems[i];
        if (!item->active) {
            item->timer -= dt;
            if (item->timer <= 0.0f) {
                item->active = true;
                item->timer = BONUS_RESPAWN_TIME;
                item->pos = GetRandomWalkableCenter();
            }
            continue;
        }

        item->timer -= dt;
        if (item->timer <= 0.0f) {
            item->active = false;
            item->timer = BONUS_RESPAWN_TIME;
            continue;
        }

        if (Vector2Distance(pacman.pos, item->pos) >= TILE_SIZE * 0.72f) continue;

        item->active = false;
        item->timer = BONUS_RESPAWN_TIME;

        switch (item->type) {
            case ITEM_PEPPER:
                pacman.boostTimer = PACMAN_BOOST_DURATION;
                score += 100;
                AddFloatingText(pacman.pos, "SPEED BOOST!", ORANGE);
                SpawnParticles(pacman.pos, ORANGE, 30, 90.0f);
                break;

            case ITEM_APPLE:
                if (pacman.lives < 3) {
                    pacman.lives++;
                    AddFloatingText(pacman.pos, "1UP! +1 LIFE", GREEN);
                } else {
                    AddFloatingText(pacman.pos, "+100", GREEN);
                }
                score += 100;
                SpawnParticles(pacman.pos, GREEN, 30, 90.0f);
                break;

            case ITEM_MUSHROOM:
                pacman.lives--;
                screenShakeTime = 0.40f;
                AddFloatingText(pacman.pos, "POISON! -1 LIFE", PURPLE);
                SpawnParticles(pacman.pos, PURPLE, 35, 100.0f);

                if (pacman.lives <= 0) {
                    SaveRecord(playerName, score, currentLevel + 1);
                    currentState = STATE_GAME_OVER;
                } else {
                    ResetAfterLifeLost();
                }
                break;

            case ITEM_BERRY:
                score += 300;
                AddFloatingText(pacman.pos, "+300 BONUS", GOLD);
                SpawnParticles(pacman.pos, GOLD, 25, 85.0f);
                break;
        }

        if (currentState != STATE_PLAYING) return;
    }
}

static void UpdateGhosts(float dt) {
    for (int i = 0; i < MAX_GHOSTS; ++i) {
        Ghost *g = &ghosts[i];
        float currentSpeed = g->speed;
        if (g->isEaten) currentSpeed *= GHOST_EATEN_MULTIPLIER;
        else if (g->isFrightened) currentSpeed *= GHOST_FRIGHTENED_MULTIPLIER;

        Vector2 targetPos = GetTileCenter(g->targetCol, g->targetRow);
        float step = currentSpeed * dt;
        float distance = Vector2Distance(g->pos, targetPos);

        if (distance <= step) {
            g->pos = targetPos;
            g->col = WrapColumn(g->targetCol);
            g->row = g->targetRow;

            if (g->isEaten && g->col == 9 && (g->row == 6 || g->row == 7)) {
                g->isEaten = false;
                g->isFrightened = false;
                g->dir = (Vector2){0, -1};
            }

            SetGhostNextMove(g);
        } else if (distance > 0.001f) {
            Vector2 move = Vector2Normalize(Vector2Subtract(targetPos, g->pos));
            g->pos = Vector2Add(g->pos, Vector2Scale(move, step));
        }

        if (Vector2Distance(pacman.pos, g->pos) < PACMAN_SIZE * 0.72f) {
            if (g->isEaten) continue;

            if (g->isFrightened) {
                g->isEaten = true;
                g->isFrightened = false;
                score += ghostComboScore;
                AddFloatingText(pacman.pos, TextFormat("+%d", ghostComboScore), SKYBLUE);
                SpawnParticles(g->pos, SKYBLUE, 30, 90.0f);
                ghostComboScore *= 2;
            } else {
                pacman.lives--;
                screenShakeTime = 0.45f;
                SpawnParticles(pacman.pos, RED, 40, 110.0f);

                if (pacman.lives <= 0) {
                    SaveRecord(playerName, score, currentLevel + 1);
                    currentState = STATE_GAME_OVER;
                } else {
                    ResetAfterLifeLost();
                }
                return;
            }
        }
    }
}

static void AdvanceLevel(void) {
    score += LEVEL_CLEAR_BASE_SCORE * (currentLevel + 1);

    currentLevel = (currentLevel + 1) % TOTAL_LEVELS;
    InitLevel(currentLevel);

    AddFloatingText(pacman.pos, "MAZE CLEARED!", GOLD);
    SpawnParticles(pacman.pos, GOLD, 50, 125.0f);
}

static void UpdateGameplay(float dt) {
    if (screenShakeTime > 0.0f) screenShakeTime = fmaxf(0.0f, screenShakeTime - dt);

    if (frightenedTimer > 0.0f) {
        frightenedTimer = fmaxf(0.0f, frightenedTimer - dt);
        if (frightenedTimer <= 0.0f) {
            for (int i = 0; i < MAX_GHOSTS; ++i) {
                ghosts[i].isFrightened = false;
            }
            ghostComboScore = GHOST_SCORE_START;
        }
    }

    UpdatePacman(dt);
    if (currentState != STATE_PLAYING) return;

    CollectPellets();
    if (pelletsRemaining <= 0) {
        AdvanceLevel();
        UpdateParticles(dt);
        return;
    }

    UpdateGhosts(dt);
    if (currentState != STATE_PLAYING) {
        UpdateParticles(dt);
        return;
    }

    UpdateBonusItems(dt);
    UpdateParticles(dt);
}

static void DrawMaze(void) {
    float pulseTime = (float)GetTime();

    for (int row = 0; row < MAP_ROWS; ++row) {
        for (int col = 0; col < MAP_COLS; ++col) {
            int x = OFFSET_X + col * TILE_SIZE;
            int y = OFFSET_Y + row * TILE_SIZE;
            char tile = activeMap[row][col];

            if (tile == '#') {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, COLOR_WALL);
                DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, COLOR_WALL_EDGE);
                DrawRectangle(x + 4, y + 4, TILE_SIZE - 8, TILE_SIZE - 8, (Color){6, 10, 32, 255});
            } else {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, COLOR_FLOOR);

                if (tile == '.') {
                    if (texPizza.id != 0) {
                        Rectangle source = {0, 0, (float)texPizza.width, (float)texPizza.height};
                        Rectangle dest = {x + TILE_SIZE * 0.5f, y + TILE_SIZE * 0.5f, 15.0f, 15.0f};
                        DrawTexturePro(texPizza, source, dest, (Vector2){7.5f, 7.5f}, 0.0f, WHITE);
                    } else {
                        DrawCircle(x + TILE_SIZE / 2, y + TILE_SIZE / 2, 3.5f, (Color){255, 230, 170, 255});
                    }
                } else if (tile == 'o') {
                    float pulse = sinf(pulseTime * 8.0f) * 2.0f;
                    Vector2 center = GetTileCenter(col, row);
                    DrawCircleGradient(center, 8.0f + pulse, WHITE, (Color){255, 170, 40, 255});
                }
            }
        }
    }
}

static void DrawItemPro(Texture2D texture, Vector2 center, float radius, Color fallbackColor) {
    if (texture.id != 0) {
        Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
        Rectangle dest = {center.x, center.y, radius * 2.0f, radius * 2.0f};
        DrawTexturePro(texture, source, dest, (Vector2){radius, radius}, 0.0f, WHITE);
    } else {
        DrawCircleV(center, radius, fallbackColor);
        DrawCircleLines((int)center.x, (int)center.y, radius + 2.0f, WHITE);
    }
}

static void DrawGhost(const Ghost *ghost) {
    if (ghost->isEaten) {
        DrawCircle(ghost->pos.x - 5, ghost->pos.y - 2, 4.0f, WHITE);
        DrawCircle(ghost->pos.x + 5, ghost->pos.y - 2, 4.0f, WHITE);
        DrawCircle(ghost->pos.x - 5 + ghost->dir.x * 2.0f,
                   ghost->pos.y - 2 + ghost->dir.y * 2.0f,
                   2.0f,
                   DARKBLUE);
        DrawCircle(ghost->pos.x + 5 + ghost->dir.x * 2.0f,
                   ghost->pos.y - 2 + ghost->dir.y * 2.0f,
                   2.0f,
                   DARKBLUE);
        return;
    }

    Texture2D texture = (ghost->type == GHOST_BLINKY && texGhostChaser.id != 0) ? texGhostChaser : texGhost;

    if (texture.id != 0) {
        Color tint = WHITE;
        if (ghost->isFrightened) {
            bool flash = frightenedTimer < 2.5f && ((int)(frightenedTimer * 8.0f) % 2 == 0);
            tint = flash ? WHITE : (Color){60, 90, 255, 255};
        }

        Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
        Rectangle dest = {ghost->pos.x, ghost->pos.y, PACMAN_SIZE * 1.15f, PACMAN_SIZE * 1.15f};
        DrawTexturePro(texture,
                       source,
                       dest,
                       (Vector2){PACMAN_SIZE * 0.58f, PACMAN_SIZE * 0.58f},
                       0.0f,
                       tint);
        return;
    }

    Color body = ghost->isFrightened ? BLUE : ghost->defaultColor;
    DrawCircleSector(ghost->pos, PACMAN_SIZE / 2.0f, 180.0f, 360.0f, 24, body);
    DrawRectangle((int)(ghost->pos.x - PACMAN_SIZE / 2.0f),
                  (int)ghost->pos.y,
                  PACMAN_SIZE,
                  PACMAN_SIZE / 2,
                  body);

    DrawCircle(ghost->pos.x - 5, ghost->pos.y - 2, 4.0f, WHITE);
    DrawCircle(ghost->pos.x + 5, ghost->pos.y - 2, 4.0f, WHITE);
    DrawCircle(ghost->pos.x - 5 + ghost->dir.x * 2.0f,
               ghost->pos.y - 2 + ghost->dir.y * 2.0f,
               2.0f,
               DARKBLUE);
    DrawCircle(ghost->pos.x + 5 + ghost->dir.x * 2.0f,
               ghost->pos.y - 2 + ghost->dir.y * 2.0f,
               2.0f,
               DARKBLUE);
}

static void DrawPacman(void) {
    if (texPacman.id != 0) {
        Rectangle source = {0, 0, (float)texPacman.width, (float)texPacman.height};
        Rectangle dest = {pacman.pos.x, pacman.pos.y, PACMAN_SIZE * 1.15f, PACMAN_SIZE * 1.15f};
        DrawTexturePro(texPacman,
                       source,
                       dest,
                       (Vector2){PACMAN_SIZE * 0.58f, PACMAN_SIZE * 0.58f},
                       pacman.rotation,
                       WHITE);
        return;
    }

    float startAngle = pacman.rotation + pacman.mouthAngle * 0.5f;
    float endAngle = pacman.rotation + 360.0f - pacman.mouthAngle * 0.5f;
    DrawCircleSector(pacman.pos, PACMAN_SIZE / 2.0f, startAngle, endAngle, 28, YELLOW);
}

static void DrawProceduralHeart(float x, float y, float size) {
    float radius = size * 0.28f;
    DrawCircleV((Vector2){x - radius, y - radius * 0.4f}, radius, RED);
    DrawCircleV((Vector2){x + radius, y - radius * 0.4f}, radius, RED);
    DrawTriangle((Vector2){x - size * 0.52f, y - radius * 0.4f},
                 (Vector2){x + size * 0.52f, y - radius * 0.4f},
                 (Vector2){x, y + size * 0.52f},
                 RED);
}

static void DrawHUD(void) {
    DrawRectangle(0, 0, SCREEN_WIDTH, OFFSET_Y - 8, COLOR_PANEL);
    DrawLine(0, OFFSET_Y - 8, SCREEN_WIDTH, OFFSET_Y - 8, (Color){35, 45, 95, 255});

    const char *displayName = playerName[0] ? playerName : "PILOT";

    DrawText(TextFormat("PILOT: %s", displayName), 18, 16, 20, GOLD);
    DrawText(TextFormat("SCORE: %06d", score), 250, 16, 20, WHITE);
    DrawText(TextFormat("MAZE: %d/%d", currentLevel + 1, TOTAL_LEVELS), 470, 16, 20, SKYBLUE);

    DrawText("LIVES:", 610, 18, 18, LIGHTGRAY);
    for (int i = 0; i < pacman.lives; ++i) {
        DrawProceduralHeart(690.0f + i * 26.0f, 24.0f, 18.0f);
    }

    if (pacman.boostTimer > 0.0f) {
        DrawText(TextFormat("SPEED BOOST: %.1fs", pacman.boostTimer), 18, 48, 15, ORANGE);
    }

    if (frightenedTimer > 0.0f) {
        DrawText(TextFormat("GHOST HUNT: %.1fs (x%d)", frightenedTimer, ghostComboScore / 100),
                 250,
                 48,
                 15,
                 SKYBLUE);
    }

    Color foodColor = pelletsRemaining < 10 ? GREEN : LIGHTGRAY;
    DrawText(TextFormat("FOOD: %d/%d", pelletsRemaining, totalPellets), 470, 48, 15, foodColor);
    DrawText("[M] MUTE   [P/ESC] PAUSE", 616, 48, 11, GRAY);
}

static void DrawGameWorld(void) {
    DrawMaze();

    for (int i = 0; i < MAX_BONUS_ITEMS; ++i) {
        if (!bonusItems[i].active) continue;

        switch (bonusItems[i].type) {
            case ITEM_PEPPER: DrawItemPro(texPepper, bonusItems[i].pos, 14.0f, ORANGE); break;
            case ITEM_APPLE: DrawItemPro(texApple, bonusItems[i].pos, 14.0f, GREEN); break;
            case ITEM_MUSHROOM: DrawItemPro(texMushroom, bonusItems[i].pos, 14.0f, PURPLE); break;
            case ITEM_BERRY: DrawItemPro(texBerry, bonusItems[i].pos, 14.0f, SKYBLUE); break;
        }
    }

    for (int i = 0; i < MAX_GHOSTS; ++i) {
        DrawGhost(&ghosts[i]);
    }

    DrawPacman();
    DrawParticles();
}

static void DrawMenuBackground(void) {
    ClearBackground((Color){10, 12, 22, 255});
    if (texBackground.id != 0) {
        DrawTexturePro(texBackground,
                       (Rectangle){0, 0, (float)texBackground.width, (float)texBackground.height},
                       (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
                       (Vector2){0, 0},
                       0.0f,
                       Fade(WHITE, 0.25f));
    }
}

static void DrawMenuScreen(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menuSelection = (menuSelection + 2) % 3;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menuSelection = (menuSelection + 1) % 3;
    }

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) currentState = STATE_NAME_INPUT;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        LoadRecords();
        currentState = STATE_RECORDS;
    }
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) applicationRunning = false;

    if (IsKeyPressed(KEY_ENTER)) {
        if (menuSelection == 0) currentState = STATE_NAME_INPUT;
        else if (menuSelection == 1) {
            LoadRecords();
            currentState = STATE_RECORDS;
        } else {
            applicationRunning = false;
        }
    }

    BeginDrawing();
    DrawMenuBackground();

    const char *title = "PAC-MAN DELUXE";
    int titleWidth = MeasureText(title, 46);
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 120, 46, YELLOW);

    const char *subtitle = "Amirkabir University of Technology - BP Edition";
    DrawText(subtitle,
             SCREEN_WIDTH / 2 - MeasureText(subtitle, 16) / 2,
             180,
             16,
             LIGHTGRAY);

    const char *options[3] = {"1. START MISSION", "2. HALL OF FAME", "3. QUIT"};
    for (int i = 0; i < 3; ++i) {
        Color color = i == menuSelection ? YELLOW : RAYWHITE;
        if (i == menuSelection) {
            DrawText(">", SCREEN_WIDTH / 2 - 140, 320 + i * 55, 24, YELLOW);
        }
        DrawText(options[i], SCREEN_WIDTH / 2 - 110, 320 + i * 55, 24, color);
    }

    const char *hint = "Use [W/S] or [1/2/3] - Press [ENTER] to Select";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 16) / 2, 560, 16, GRAY);
    EndDrawing();
}

static void DrawNameInputScreen(void) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentState = STATE_MENU;
        return;
    }

    int key = GetCharPressed();
    while (key > 0) {
        size_t len = strlen(playerName);
        if (key >= 32 && key <= 126 && len < MAX_NAME_LEN - 1) {
            playerName[len] = (char)key;
            playerName[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t len = strlen(playerName);
        if (len > 0) playerName[len - 1] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (playerName[0] == '\0') strcpy(playerName, "ARTESHMAN");
        StartNewGame();
        currentState = STATE_PLAYING;
        return;
    }

    BeginDrawing();
    ClearBackground((Color){12, 14, 26, 255});

    const char *title = "ENTER PILOT CALLSIGN";
    DrawText(title,
             SCREEN_WIDTH / 2 - MeasureText(title, 30) / 2,
             180,
             30,
             YELLOW);

    const char *hint = "Type your name and press [ENTER] to launch (or [ESC] to back)";
    DrawText(hint,
             SCREEN_WIDTH / 2 - MeasureText(hint, 16) / 2,
             225,
             16,
             LIGHTGRAY);

    DrawRectangle(SCREEN_WIDTH / 2 - 180, 280, 360, 60, (Color){24, 28, 50, 255});
    DrawRectangleLines(SCREEN_WIDTH / 2 - 180, 280, 360, 60, SKYBLUE);

    bool showCursor = ((int)(GetTime() * 2.5f) % 2 == 0);
    const char *cursor = showCursor ? "_" : "";
    const char *value = TextFormat("%s%s", playerName, cursor);
    DrawText(value,
             SCREEN_WIDTH / 2 - MeasureText(value, 28) / 2,
             296,
             28,
             WHITE);

    DrawText("Press [ENTER] to Start Game",
             SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to Start Game", 18) / 2,
             380,
             18,
             GOLD);

    EndDrawing();
}

static void DrawRecordsScreen(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        currentState = STATE_MENU;
        return;
    }

    BeginDrawing();
    ClearBackground((Color){10, 12, 24, 255});

    const char *title = "HALL OF HEROES";
    DrawText(title,
             SCREEN_WIDTH / 2 - MeasureText(title, 36) / 2,
             50,
             36,
             GOLD);

    DrawText("RANK   PLAYER               SCORE     LVL   DATE", 130, 130, 18, SKYBLUE);
    DrawLine(130, 155, 670, 155, (Color){50, 65, 120, 255});

    int limit = recordCount < 10 ? recordCount : 10;
    for (int i = 0; i < limit; ++i) {
        Color rowColor;
        if (i == 0) rowColor = GOLD;
        else if (i == 1) rowColor = LIGHTGRAY;
        else if (i == 2) rowColor = (Color){205, 127, 50, 255};
        else rowColor = WHITE;

        DrawText(TextFormat("#%02d    %-18s  %06d    %02d    %s",
                            i + 1,
                            records[i].name,
                            records[i].score,
                            records[i].level,
                            records[i].date),
                 130,
                 175 + i * 38,
                 18,
                 rowColor);
    }

    if (recordCount == 0) {
        const char *empty = "No records recorded yet. Complete a game!";
        DrawText(empty,
                 SCREEN_WIDTH / 2 - MeasureText(empty, 18) / 2,
                 300,
                 18,
                 GRAY);
    }

    DrawText("Press [ESC] or [ENTER] to Return",
             SCREEN_WIDTH / 2 - MeasureText("Press [ESC] or [ENTER] to Return", 16) / 2,
             640,
             16,
             LIGHTGRAY);

    EndDrawing();
}

static void DrawPauseScreen(void) {
    BeginDrawing();
    ClearBackground(COLOR_BG);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 190});

    const char *title = "GAME PAUSED";
    DrawText(title,
             SCREEN_WIDTH / 2 - MeasureText(title, 40) / 2,
             SCREEN_HEIGHT / 2 - 80,
             40,
             YELLOW);

    const char *resume = "Press [P] or [ESC] to Resume";
    DrawText(resume,
             SCREEN_WIDTH / 2 - MeasureText(resume, 20) / 2,
             SCREEN_HEIGHT / 2,
             20,
             WHITE);

    const char *menu = "Press [Q] or [M] to Return to Main Menu";
    DrawText(menu,
             SCREEN_WIDTH / 2 - MeasureText(menu, 18) / 2,
             SCREEN_HEIGHT / 2 + 40,
             18,
             LIGHTGRAY);

    EndDrawing();
}

static void DrawGameOverScreen(void) {
    BeginDrawing();
    ClearBackground((Color){25, 5, 5, 255});
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){25, 5, 5, 220});

    const char *title = "GAME OVER";
    DrawText(title,
             SCREEN_WIDTH / 2 - MeasureText(title, 44) / 2,
             SCREEN_HEIGHT / 2 - 80,
             44,
             RED);

    const char *finalScore = TextFormat("FINAL SCORE: %d", score);
    DrawText(finalScore,
             SCREEN_WIDTH / 2 - MeasureText(finalScore, 24) / 2,
             SCREEN_HEIGHT / 2 - 10,
             24,
             WHITE);

    const char *hint = "Press [ENTER] or [ESC] to Return to Menu";
    DrawText(hint,
             SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2,
             SCREEN_HEIGHT / 2 + 60,
             18,
             LIGHTGRAY);

    EndDrawing();
}

static void SetAudioForState(void) {
    if (audioMuted) {
        SetMasterVolume(0.0f);
    } else {
        SetMasterVolume(1.0f);
    }

    if (currentState == STATE_PLAYING || currentState == STATE_PAUSED) {
        if (hasMenuMusic && IsMusicStreamPlaying(musicMenu)) StopMusicStream(musicMenu);
        if (hasGameMusic && !IsMusicStreamPlaying(musicGame)) PlayMusicStream(musicGame);
        if (hasGameMusic) UpdateMusicStream(musicGame);
    } else {
        if (hasGameMusic && IsMusicStreamPlaying(musicGame)) StopMusicStream(musicGame);
        if (hasMenuMusic && !IsMusicStreamPlaying(musicMenu)) PlayMusicStream(musicMenu);
        if (hasMenuMusic) UpdateMusicStream(musicMenu);
    }
}

static Texture2D LoadTextureIfExists(const char *path) {
    return FileExists(path) ? LoadTexture(path) : (Texture2D){0};
}

static Music LoadMusicIfExists(const char *path, bool *loaded) {
    if (!FileExists(path)) {
        *loaded = false;
        return (Music){0};
    }

    *loaded = true;
    return LoadMusicStream(path);
}

static void LoadAssets(void) {
    texPacman = LoadTextureIfExists("pacman.png");
    texGhost = LoadTextureIfExists("ghost.png");
    texGhostChaser = LoadTextureIfExists("ghostc.png");
    texPizza = LoadTextureIfExists("pizza.png");
    texPepper = LoadTextureIfExists("pepper.png");
    texApple = LoadTextureIfExists("apple.png");
    texBerry = LoadTextureIfExists("berry.png");
    texMushroom = LoadTextureIfExists("Mushroom.png");
    texBackground = LoadTextureIfExists("background_menu.png");

    musicMenu = LoadMusicIfExists("menu.mp3", &hasMenuMusic);
    musicGame = LoadMusicIfExists("game.mp3", &hasGameMusic);
}

static void UnloadTextureIfLoaded(Texture2D *texture) {
    if (texture->id != 0) {
        UnloadTexture(*texture);
        *texture = (Texture2D){0};
    }
}

static void UnloadMusicIfLoaded(Music *music, bool *loaded) {
    if (*loaded) {
        StopMusicStream(*music);
        UnloadMusicStream(*music);
        *music = (Music){0};
        *loaded = false;
    }
}

static void Cleanup(void) {
    UnloadMusicIfLoaded(&musicMenu, &hasMenuMusic);
    UnloadMusicIfLoaded(&musicGame, &hasGameMusic);
    CloseAudioDevice();

    UnloadTextureIfLoaded(&texPacman);
    UnloadTextureIfLoaded(&texGhost);
    UnloadTextureIfLoaded(&texGhostChaser);
    UnloadTextureIfLoaded(&texPizza);
    UnloadTextureIfLoaded(&texPepper);
    UnloadTextureIfLoaded(&texApple);
    UnloadTextureIfLoaded(&texBerry);
    UnloadTextureIfLoaded(&texMushroom);
    UnloadTextureIfLoaded(&texBackground);

    CloseWindow();
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pac-Man Deluxe - Raylib Edition");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    SetRandomSeed((unsigned int)time(NULL));

    InitAudioDevice();
    LoadAssets();
    LoadRecords();
    StartNewGame();

    while (applicationRunning && !WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 1.0f / 30.0f);

        if (IsKeyPressed(KEY_M)) {
            audioMuted = !audioMuted;
        }

        switch (currentState) {
            case STATE_MENU:
                DrawMenuScreen();
                break;

            case STATE_NAME_INPUT:
                DrawNameInputScreen();
                break;

            case STATE_PLAYING:
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    currentState = STATE_PAUSED;
                    break;
                }

                UpdateGameplay(dt);
                {
                    Vector2 shake = {0, 0};
                    if (screenShakeTime > 0.0f) {
                        shake.x = (float)GetRandomValue(-4, 4);
                        shake.y = (float)GetRandomValue(-4, 4);
                    }

                    BeginDrawing();
                    ClearBackground(COLOR_BG);

                    Camera2D camera = {0};
                    camera.target = (Vector2){-shake.x, -shake.y};
                    camera.offset = (Vector2){0, 0};
                    camera.zoom = 1.0f;

                    BeginMode2D(camera);
                    DrawGameWorld();
                    UpdateAndDrawFloatingTexts(dt);
                    EndMode2D();

                    DrawHUD();
                    EndDrawing();
                }
                break;

            case STATE_PAUSED:
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    currentState = STATE_PLAYING;
                } else if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_M)) {
                    currentState = STATE_MENU;
                }
                DrawPauseScreen();
                break;

            case STATE_GAME_OVER:
                DrawGameOverScreen();
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                    currentState = STATE_MENU;
                }
                break;

            case STATE_RECORDS:
                DrawRecordsScreen();
                break;
        }

        SetAudioForState();
    }

    Cleanup();
    return 0;
}
