#ifndef CLIFFS_H
#define CLIFFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    screen_width = 640,
    screen_height = 480,
    mx_ties = 20,
    mx_pitons = 20,
    mx_moves = 16,
    mx_ledges = 500
};

typedef enum {
    dir_up = 1,
    dir_down = 2,
    dir_left = 3,
    dir_right = 4
} Direction;

typedef enum {
    tie_off = 0,
    tie_on_man = 1,
    tie_on_man_sliding = 2,
    tie_on_ledge = 3,
    tie_on_ledge_sliding = 4
} TieStatus;

typedef enum {
    result_none = 0,
    result_fall = -1,
    result_win = 1,
    result_retreat = 2
} RoundResult;

typedef enum {
    key_none = 0,
    key_arrow_up = 1001,
    key_arrow_down,
    key_arrow_left,
    key_arrow_right,
    key_f1,
    key_f2,
    key_enter,
    key_escape
} KeyCode;

typedef struct {
    int lr;
    int du;
    int x;
    int y;
} Limb;

typedef struct {
    int x;
    int y;
    Limb lh;
    Limb rh;
    Limb lf;
    Limb rf;
    Limb old_lh;
    Limb old_rh;
    Limb old_lf;
    Limb old_rf;
} Man;

typedef struct {
    int height;
    int left_end;
    int right_end;
} Ledge;

typedef struct Tie {
    int status;
    int *x;
    int *y;
    struct Tie *pre;
    struct Tie *post;
} Tie;

typedef struct {
    int x0;
    int y0;
    int xl;
    int yl;
    int end0;
    int endl;
    Tie ties[mx_ties];
} Rope;

typedef struct {
    int status;
    int x;
    int y;
    Tie *tie;
} TiePoint;

typedef struct {
    TiePoint l;
    TiePoint b;
    TiePoint r;
} Belay;

typedef struct {
    char path[512];
    Ledge ledges[mx_ledges];
    int count;
} Level;

typedef struct Game Game;

typedef struct {
    void (*present)(void *userdata, const Game *game);
    void (*delay)(void *userdata, uint32_t ms);
    void *userdata;
} RenderHooks;

struct Game {
    Level level;
    Rope rope;
    TiePoint pitons[mx_pitons];
    Man men[2];
    Belay belays[2];
    int active_man;
    int direction;
    int move_count;
    int pending_man;
    int pending_belay_key;
    int pending_piton_key;
    int move_queue[mx_moves];
    int queued_moves;
    bool move_batch_active;
    uint64_t move_batch_started_ms;
    uint64_t started_ms;
    RoundResult result;
    bool round_over;
    RenderHooks hooks;
};

bool resolve_level_path(const char *requested, char *out, size_t out_size);
bool game_init(
    Game *game,
    const char *level_name,
    const RenderHooks *hooks,
    uint64_t now_ms,
    char *error,
    size_t error_size
);
void game_press_key(Game *game, int key, uint64_t now_ms);
void game_update(Game *game, uint64_t now_ms);
int game_elapsed_seconds(const Game *game, uint64_t now_ms);
const char *game_result_title(RoundResult result);
const char *game_result_line(RoundResult result, int line);

#endif
