#include "cliffs.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    max_vertical_extension = 20,
    max_horizontal_extension = 10,
    limb_step = 10,
    active_man_dougal = 0,
    active_man_peter = 1
};

static bool file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        return false;
    }

    fclose(fp);
    return true;
}

static const char *basename_ptr(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *base = path;

    if (slash != NULL && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }

    return base;
}

static bool has_extension(const char *path)
{
    return strchr(basename_ptr(path), '.') != NULL;
}

static void add_candidate(char candidates[][512], int *count, const char *value)
{
    int i;

    for (i = 0; i < *count; ++i) {
        if (strcmp(candidates[i], value) == 0) {
            return;
        }
    }

    if (*count >= 8) {
        return;
    }

    snprintf(candidates[*count], 512, "%s", value);
    (*count)++;
}

bool resolve_level_path(const char *requested, char *out, size_t out_size)
{
    char candidates[8][512];
    int count = 0;
    char buffer[512];
    const char *name = (requested != NULL && *requested != '\0') ? requested : "ledges";
    const char *base = basename_ptr(name);
    int i;

    add_candidate(candidates, &count, name);

    if (!has_extension(name)) {
        snprintf(buffer, sizeof(buffer), "%s.ldg", name);
        add_candidate(candidates, &count, buffer);
    }

    snprintf(buffer, sizeof(buffer), "levels/%s", base);
    add_candidate(candidates, &count, buffer);

    if (!has_extension(base)) {
        snprintf(buffer, sizeof(buffer), "levels/%s.ldg", base);
        add_candidate(candidates, &count, buffer);
    }

    for (i = 0; i < count; ++i) {
        if (file_exists(candidates[i])) {
            snprintf(out, out_size, "%s", candidates[i]);
            return true;
        }
    }

    return false;
}

static bool load_level(Level *level, const char *requested, char *error, size_t error_size)
{
    char resolved[512];
    FILE *fp;
    char line[256];

    if (!resolve_level_path(requested, resolved, sizeof(resolved))) {
        snprintf(error, error_size, "could not find level file '%s'", requested != NULL ? requested : "ledges");
        return false;
    }

    fp = fopen(resolved, "rb");
    if (fp == NULL) {
        snprintf(error, error_size, "failed to open '%s': %s", resolved, strerror(errno));
        return false;
    }

    memset(level, 0, sizeof(*level));
    snprintf(level->path, sizeof(level->path), "%s", resolved);

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *cursor = line;
        char *end = NULL;
        long values[3];
        int parsed = 0;

        while (*cursor != '\0' && parsed < 3) {
            long value = strtol(cursor, &end, 10);
            if (end != cursor) {
                values[parsed++] = value;
                cursor = end;
            } else {
                cursor++;
            }
        }

        if (parsed == 3 && level->count < mx_ledges) {
            level->ledges[level->count].height = (int)values[0];
            level->ledges[level->count].left_end = (int)values[1];
            level->ledges[level->count].right_end = (int)values[2];
            level->count++;
        }
    }

    fclose(fp);

    if (level->count == 0) {
        snprintf(error, error_size, "no ledges found in '%s'", resolved);
        return false;
    }

    return true;
}

static void animate(Game *game, uint32_t delay_ms)
{
    if (game->hooks.present != NULL) {
        game->hooks.present(game->hooks.userdata, game);
    }
    if (delay_ms > 0 && game->hooks.delay != NULL) {
        game->hooks.delay(game->hooks.userdata, delay_ms);
    }
}

static Man *current_man(Game *game)
{
    return &game->men[game->active_man];
}

static void copy_limb(Limb *dest, const Limb *source)
{
    dest->lr = source->lr;
    dest->du = source->du;
    dest->x = source->x;
    dest->y = source->y;
}

static void copy_all_limbs(Man *man)
{
    copy_limb(&man->old_rh, &man->rh);
    copy_limb(&man->old_lh, &man->lh);
    copy_limb(&man->old_lf, &man->lf);
    copy_limb(&man->old_rf, &man->rf);
}

static void init_man(Man *man, int x, int y)
{
    memset(man, 0, sizeof(*man));
    man->x = x;
    man->y = y;

    man->rh.x = max_horizontal_extension;
    man->rh.y = 0;
    man->rh.lr = 1;
    man->rh.du = 1;

    man->lh.x = max_horizontal_extension;
    man->lh.y = 0;
    man->lh.lr = -1;
    man->lh.du = 1;

    man->rf.x = max_horizontal_extension;
    man->rf.y = max_vertical_extension;
    man->rf.lr = 1;
    man->rf.du = -1;

    man->lf.x = max_horizontal_extension;
    man->lf.y = max_vertical_extension;
    man->lf.lr = -1;
    man->lf.du = -1;

    copy_all_limbs(man);
}

static void update_belay(Belay *belay, const Man *man)
{
    belay->b.x = man->x;
    belay->b.y = man->y;
    belay->l.x = man->x + man->lh.lr * man->lh.x;
    belay->l.y = man->y - man->lh.du * man->lh.y;
    belay->r.x = man->x + man->rh.lr * man->rh.x;
    belay->r.y = man->y - man->rh.du * man->rh.y;
}

static void init_belay(Belay *belay, const Man *man)
{
    memset(belay, 0, sizeof(*belay));
    belay->l.status = tie_off;
    belay->b.status = tie_off;
    belay->r.status = tie_off;
    update_belay(belay, man);
}

static void init_pitons(TiePoint pitons[mx_pitons])
{
    int i;

    for (i = 0; i < mx_pitons; ++i) {
        pitons[i].status = tie_off;
        pitons[i].x = -1;
        pitons[i].y = -1;
        pitons[i].tie = NULL;
    }
}

static void init_rope(Rope *rope, int x0, int y0, int xl, int yl)
{
    int i;

    memset(rope, 0, sizeof(*rope));
    rope->x0 = x0;
    rope->y0 = y0;
    rope->xl = xl;
    rope->yl = yl;
    rope->end0 = 10;
    rope->endl = 10;

    for (i = 0; i < mx_ties; ++i) {
        rope->ties[i].status = tie_off;
        rope->ties[i].x = NULL;
        rope->ties[i].y = NULL;
        rope->ties[i].pre = NULL;
        rope->ties[i].post = NULL;
    }

    rope->ties[0].x = &rope->x0;
    rope->ties[0].y = &rope->y0;
    rope->ties[1].x = &rope->xl;
    rope->ties[1].y = &rope->yl;
    rope->ties[0].post = &rope->ties[1];
    rope->ties[1].pre = &rope->ties[0];
}

static void dangle_ends(Rope *rope)
{
    Tie *tie = rope->ties[0].post;

    rope->x0 = *tie->x;
    rope->y0 = *tie->y + rope->end0;

    tie = rope->ties[1].pre;
    rope->xl = *tie->x;
    rope->yl = *tie->y + rope->endl;
}

static int get_free_index(Rope *rope)
{
    int i;

    for (i = 2; i < mx_ties; ++i) {
        if (rope->ties[i].pre == NULL && rope->ties[i].post == NULL) {
            return i;
        }
    }

    return mx_ties;
}

static bool insert_tie(Tie *at, Tie *tie)
{
    if (at->post != NULL) {
        tie->pre = at;
        tie->post = at->post;
        at->post->pre = tie;
        at->post = tie;
        return true;
    }

    if (at->pre != NULL) {
        tie->post = at;
        tie->pre = at->pre;
        at->pre->post = tie;
        at->pre = tie;
        return true;
    }

    return false;
}

static bool delete_tie(Tie *tie)
{
    if (tie == NULL || tie->pre == NULL || tie->post == NULL) {
        return false;
    }

    tie->post->pre = tie->pre;
    tie->pre->post = tie->post;
    tie->status = tie_off;
    tie->x = NULL;
    tie->y = NULL;
    tie->pre = NULL;
    tie->post = NULL;
    return true;
}

static bool add_tiepoint(Tie *at, Tie *tie, TiePoint *point, int status)
{
    if (!insert_tie(at, tie)) {
        return false;
    }

    tie->x = &point->x;
    tie->y = &point->y;
    tie->status = status;
    point->status = status;
    point->tie = tie;
    return true;
}

static bool is_near_rope(int a, int b, int x1, int y1, int x2, int y2)
{
    int projected;
    const int tolerance = 4;

    if (x1 == x2) {
        if (a != x1) {
            return false;
        }
        return (b - y1) * (b - y2) <= 0;
    }

    if ((a - x1) * (a - x2) > 0) {
        return false;
    }

    projected = y1 + (int)(((double)(a - x1) * (double)(y2 - y1)) / (double)(x2 - x1));
    return projected - tolerance <= b && projected + tolerance >= b;
}

static bool check_rope_touch(int x, int y, Rope *rope, Tie **at)
{
    Tie *tie = &rope->ties[0];
    Tie *next = tie->post;

    while (next != NULL) {
        if (is_near_rope(x, y, *tie->x, *tie->y, *next->x, *next->y)) {
            *at = tie;
            return true;
        }
        tie = next;
        next = next->post;
    }

    return false;
}

static int limb_world_x(const Man *man, const Limb *limb)
{
    return man->x + limb->lr * limb->x;
}

static int limb_world_y(const Man *man, const Limb *limb)
{
    return man->y - limb->du * limb->y;
}

static bool is_attached(const Man *man, const Limb *limb, const Ledge ledges[mx_ledges], int ledge_count)
{
    int x = limb_world_x(man, limb);
    int y = limb_world_y(man, limb);
    int i;

    for (i = 0; i < ledge_count; ++i) {
        if (y == ledges[i].height && x >= ledges[i].left_end && x <= ledges[i].right_end) {
            return true;
        }
    }

    return false;
}

static int ledge_support(const Man *man, const Ledge ledges[mx_ledges], int ledge_count)
{
    int count = 0;

    if (is_attached(man, &man->rh, ledges, ledge_count)) {
        count += 1;
    }
    if (is_attached(man, &man->lh, ledges, ledge_count)) {
        count += 1;
    }
    if (is_attached(man, &man->lf, ledges, ledge_count)) {
        count += 2;
    }
    if (is_attached(man, &man->rf, ledges, ledge_count)) {
        count += 2;
    }

    return count;
}

static void swing_body(int *dx, int *dy, const Limb *new_limb, const Limb *old_limb, int direction)
{
    int delta_x = (new_limb->x - old_limb->x) * new_limb->lr;
    int delta_y = (new_limb->y - old_limb->y) * new_limb->du;

    if ((delta_x > 0 && direction == dir_left) || (delta_x < 0 && direction == dir_right)) {
        *dx = delta_x;
    }
    if ((delta_y > 0 && direction == dir_down) || (delta_y < 0 && direction == dir_up)) {
        *dy = delta_y;
    }
}

static void move_limb(Limb *limb, int sign, int direction)
{
    int value;

    if (direction == dir_left || direction == dir_right) {
        value = limb->x + sign * limb_step;
        if (value < 0) {
            value = 0;
        }
        if (value > max_horizontal_extension) {
            value = max_horizontal_extension;
        }
        limb->x = value;
    } else {
        value = limb->y + sign * limb_step;
        if (value < 0) {
            value = 0;
        }
        if (value > max_vertical_extension) {
            value = max_vertical_extension;
        }
        limb->y = value;
    }
}

static void move_limb_key(int move, Man *man, int direction)
{
    Limb *limb = NULL;
    int sign = 0;

    switch (move) {
        case 'p':
            limb = &man->rh;
            sign = 1;
            break;
        case 'l':
            limb = &man->rh;
            sign = -1;
            break;
        case 'q':
            limb = &man->lh;
            sign = 1;
            break;
        case 'a':
            limb = &man->lh;
            sign = -1;
            break;
        case 'k':
            limb = &man->rf;
            sign = (direction == dir_up || direction == dir_down) ? -1 : 1;
            break;
        case 'm':
            limb = &man->rf;
            sign = (direction == dir_up || direction == dir_down) ? 1 : -1;
            break;
        case 's':
            limb = &man->lf;
            sign = (direction == dir_up || direction == dir_down) ? -1 : 1;
            break;
        case 'x':
            limb = &man->lf;
            sign = (direction == dir_up || direction == dir_down) ? 1 : -1;
            break;
    }

    if (limb != NULL) {
        move_limb(limb, sign, direction);
    }
}

static void execute_moves(
    int *dx,
    int *dy,
    Man *man,
    const int *moves,
    int move_count,
    int direction,
    const Ledge ledges[mx_ledges],
    int ledge_count
)
{
    int i;
    int rh_dx = 0;
    int rh_dy = 0;
    int lh_dx = 0;
    int lh_dy = 0;
    int rf_dx = 0;
    int rf_dy = 0;
    int lf_dx = 0;
    int lf_dy = 0;

    for (i = 0; i < move_count; ++i) {
        move_limb_key(moves[i], man, direction);
    }

    if (is_attached(man, &man->old_rh, ledges, ledge_count)) {
        swing_body(&rh_dx, &rh_dy, &man->rh, &man->old_rh, direction);
    }
    if (is_attached(man, &man->old_lh, ledges, ledge_count)) {
        swing_body(&lh_dx, &lh_dy, &man->lh, &man->old_lh, direction);
    }
    if (is_attached(man, &man->old_rf, ledges, ledge_count)) {
        swing_body(&rf_dx, &rf_dy, &man->rf, &man->old_rf, direction);
    }
    if (is_attached(man, &man->old_lf, ledges, ledge_count)) {
        swing_body(&lf_dx, &lf_dy, &man->lf, &man->old_lf, direction);
    }

    *dx = rh_dx != 0 ? rh_dx : lh_dx != 0 ? lh_dx : rf_dx != 0 ? rf_dx : lf_dx;
    *dy = rh_dy != 0 ? rh_dy : lh_dy != 0 ? lh_dy : rf_dy != 0 ? rf_dy : lf_dy;
}

static int next_tie(Tie **tie, int direction)
{
    if (direction == 1) {
        *tie = (*tie)->post;
    } else {
        *tie = (*tie)->pre;
    }
    return 0;
}

static int on_belay(const Belay *belay)
{
    int count = 0;

    if (belay->l.status == tie_on_man) {
        count += 1;
    }
    if (belay->r.status == tie_on_man) {
        count += 1;
    }
    if (belay->b.status == tie_on_man) {
        count += 2;
    }

    return count;
}

static int test_pull(
    Rope *rope,
    Belay *belay,
    TiePoint *other_body,
    int *pivot_x,
    int *pivot_y,
    int direction,
    int *lift
)
{
    int anchored = 0;
    Tie *left_tie = belay->l.tie;
    Tie *right_tie = belay->r.tie;
    Tie *body_tie = belay->b.tie;
    Tie *cursor = direction == 1 ? &rope->ties[0] : &rope->ties[1];

    while (cursor != NULL) {
        if (cursor == left_tie || cursor == right_tie || cursor == body_tie) {
            int old_y = *cursor->y;
            next_tie(&cursor, -direction);
            if (cursor == NULL) {
                return 0;
            }
            *pivot_x = *cursor->x;
            *pivot_y = *cursor->y;
            *lift = *pivot_y < old_y ? 1 : 0;
            break;
        }

        if ((cursor == other_body->tie && cursor->status == tie_on_man) || cursor->status == tie_on_ledge) {
            anchored = 1;
        }

        next_tie(&cursor, direction);
    }

    if (cursor == NULL) {
        return 0;
    }

    return anchored;
}

static int is_pivot_forward(int pivot_x, int pivot_y, int hand_x, int hand_y, int direction)
{
    switch (direction) {
        case dir_up:
            return hand_y > pivot_y;
        case dir_down:
            return hand_y < pivot_y;
        case dir_right:
            return hand_x < pivot_x;
        case dir_left:
            return hand_x > pivot_x;
        default:
            return 0;
    }
}

static void adjust_rope_ends(Rope *rope, int length, int end, int sign)
{
    if (end == 1) {
        rope->end0 += sign * length;
        if (rope->end0 < 10) {
            rope->end0 = 10;
        }
    } else {
        rope->endl += sign * length;
        if (rope->endl < 10) {
            rope->endl = 10;
        }
    }
}

static bool check_limb_centering(const TiePoint *tie, const Limb *hand, int ledgesupport)
{
    if (tie->status != tie_on_man) {
        return false;
    }

    return (ledgesupport == 0 && hand->x == 0) || ledgesupport > 0;
}

static void limb_rope_move(
    Game *game,
    const Limb *limb,
    const Limb *old_limb,
    Belay *belay,
    TiePoint *tie,
    TiePoint *other_body,
    int *dx,
    int *dy,
    int direction
)
{
    int temp_dx = 0;
    int temp_dy = 0;
    int pivot_x = 0;
    int pivot_y = 0;
    int lift = 0;
    int in_pivot = 0;
    int out_pivot = 0;
    int in_pull = 0;
    int out_pull = 0;

    if (tie->status != tie_on_man) {
        return;
    }

    swing_body(&temp_dx, &temp_dy, limb, old_limb, direction);
    if (temp_dx == 0 && temp_dy == 0) {
        return;
    }

    in_pull = test_pull(&game->rope, belay, other_body, &pivot_x, &pivot_y, 1, &lift);
    in_pivot = is_pivot_forward(pivot_x, pivot_y, tie->x, tie->y, direction);
    out_pull = test_pull(&game->rope, belay, other_body, &pivot_x, &pivot_y, -1, &lift);
    out_pivot = is_pivot_forward(pivot_x, pivot_y, tie->x, tie->y, direction);

    if (in_pivot) {
        if (in_pull) {
            *dx = temp_dx;
            *dy = temp_dy;
        } else {
            adjust_rope_ends(&game->rope, abs(temp_dx - temp_dy), 1, -1);
        }

        if (out_pull == 0 && belay->b.status != tie_on_man) {
            adjust_rope_ends(&game->rope, abs(temp_dx - temp_dy), -1, 1);
        }
    }

    if (out_pivot) {
        if (out_pull) {
            *dx = temp_dx;
            *dy = temp_dy;
        } else {
            adjust_rope_ends(&game->rope, abs(temp_dx - temp_dy), -1, -1);
        }

        if (in_pull == 0 && belay->b.status != tie_on_man) {
            adjust_rope_ends(&game->rope, abs(temp_dx - temp_dy), 1, 1);
        }
    }
}

static void execute_rope_moves(Game *game, Man *man, int *dx, int *dy, int ledgesupport)
{
    TiePoint *other_body = &game->belays[(game->active_man + 1) % 2].b;
    Belay *belay = &game->belays[game->active_man];

    if (check_limb_centering(&belay->l, &man->lh, ledgesupport)) {
        limb_rope_move(
            game,
            &man->lh,
            &man->old_lh,
            belay,
            &belay->l,
            other_body,
            dx,
            dy,
            game->direction
        );
    }

    if (check_limb_centering(&belay->r, &man->rh, ledgesupport)) {
        limb_rope_move(
            game,
            &man->rh,
            &man->old_rh,
            belay,
            &belay->r,
            other_body,
            dx,
            dy,
            game->direction
        );
    }
}

static int circle_y(int x, int center_x, int center_y, double radius)
{
    double argument = radius * radius - (double)((x - center_x) * (x - center_x));

    if (argument > 0.0) {
        return center_y + (int)sqrt(argument);
    }
    return center_y;
}

static void replace_man(Game *game, Man *man, Belay *belay, int x, int y)
{
    man->x = x;
    man->y = y;
    update_belay(belay, man);
    dangle_ends(&game->rope);
    animate(game, 16);
}

static int start_fall(Game *game, Man *man, Belay *belay)
{
    int start_y = man->y;

    while ((man->y - start_y) < 100) {
        if (ledge_support(man, game->level.ledges, game->level.count) > 2) {
            return 0;
        }

        man->y += 10;
        update_belay(belay, man);
        dangle_ends(&game->rope);
        animate(game, 24);
    }

    return -1;
}

static int hang_drop(Game *game, Man *man, Belay *belay, TiePoint *other_body)
{
    int in_lift = 0;
    int out_lift = 0;
    int in_x = 0;
    int in_y = 0;
    int out_x = 0;
    int out_y = 0;
    int start_y = man->y;

    while ((in_lift == 0 || out_lift == 0) && (man->y - start_y) < 100) {
        man->y += 1;
        update_belay(belay, man);
        dangle_ends(&game->rope);
        animate(game, 8);

        if (in_lift + out_lift + ledge_support(man, game->level.ledges, game->level.count) > 2) {
            return 0;
        }

        test_pull(&game->rope, belay, other_body, &in_x, &in_y, 1, &in_lift);
        test_pull(&game->rope, belay, other_body, &out_x, &out_y, -1, &out_lift);
    }

    if ((man->y - start_y) == 100) {
        return -1;
    }

    man->x = 10 * ((man->x + 5) / 10);
    man->y = 10 * ((man->y + 5) / 10);
    return 0;
}

static int swing_baby(Game *game, Man *man, Belay *belay, int pivot_x, int pivot_y, int in_lift, int out_lift)
{
    int x = man->x;
    int y = man->y;
    int sign = 0;
    int limit;
    double u;
    double v;
    double radius;

    if (x < pivot_x) {
        sign = 1;
    } else if (x > pivot_x) {
        sign = -1;
    }

    limit = pivot_y + 20;
    while (y < limit) {
        y += 5;
        replace_man(game, man, belay, x, y);
        if (in_lift + out_lift + ledge_support(man, game->level.ledges, game->level.count) > 2) {
            return 0;
        }
    }

    u = (double)(x - pivot_x);
    v = (double)(y - pivot_y);
    radius = sqrt(u * u + v * v);

    if (sign == 0) {
        return 0;
    }

    limit = pivot_x + (pivot_x - x);
    if (sign > 0) {
        limit -= 20;
    } else {
        limit += 20;
    }

    while (sign * x < sign * limit) {
        x += sign * 5;
        y = circle_y(x, pivot_x, pivot_y, radius);
        replace_man(game, man, belay, x, y);
        if (in_lift + out_lift + ledge_support(man, game->level.ledges, game->level.count) > 2) {
            return 0;
        }
    }

    replace_man(game, man, belay, 10 * ((man->x + 5) / 10), 10 * ((man->y + 5) / 10));
    return 0;
}

static int new_settle_weight(Game *game, Man *man)
{
    int ledgesupport = ledge_support(man, game->level.ledges, game->level.count);
    int in_pull;
    int out_pull;
    int in_lift = 0;
    int out_lift = 0;
    int in_x = 0;
    int in_y = 0;
    int out_x = 0;
    int out_y = 0;
    TiePoint *other_body;
    Belay *belay = &game->belays[game->active_man];

    if (ledgesupport > 2) {
        return 0;
    }
    if (on_belay(belay) < 2) {
        return start_fall(game, man, belay);
    }

    other_body = &game->belays[(game->active_man + 1) % 2].b;
    in_pull = test_pull(&game->rope, belay, other_body, &in_x, &in_y, 1, &in_lift);
    out_pull = test_pull(&game->rope, belay, other_body, &out_x, &out_y, -1, &out_lift);

    if (in_lift + out_lift + ledgesupport > 2) {
        return 0;
    }
    if (in_pull == 1 && out_pull == 0) {
        return swing_baby(game, man, belay, in_x, in_y, in_lift, out_lift);
    }
    if (in_pull == 0 && out_pull == 1) {
        return swing_baby(game, man, belay, out_x, out_y, in_lift, out_lift);
    }
    if (in_pull == 0 && out_pull == 0) {
        return start_fall(game, man, belay);
    }
    if ((in_x <= man->x && man->x <= out_x) || (out_x <= man->x && man->x <= in_x)) {
        return hang_drop(game, man, belay, other_body);
    }
    if (abs(in_x - man->x) < abs(out_x - man->x)) {
        return swing_baby(game, man, belay, in_x, in_y, in_lift, out_lift);
    }
    return swing_baby(game, man, belay, out_x, out_y, in_lift, out_lift);
}

static int move_man(Game *game)
{
    int dx = 0;
    int dy = 0;
    int support = 0;
    int result = 0;
    Man *man = current_man(game);

    execute_moves(
        &dx,
        &dy,
        man,
        game->move_queue,
        game->queued_moves,
        game->direction,
        game->level.ledges,
        game->level.count
    );

    if (dx == 0 && dy == 0) {
        support = ledge_support(man, game->level.ledges, game->level.count);
        execute_rope_moves(game, man, &dx, &dy, support);
    }

    man->x -= dx;
    man->y += dy;
    copy_all_limbs(man);
    update_belay(&game->belays[game->active_man], man);
    dangle_ends(&game->rope);

    result = new_settle_weight(game, man);
    animate(game, 8);
    return result;
}

static void clear_move_batch(Game *game)
{
    game->queued_moves = 0;
    game->move_batch_active = false;
}

static bool man_on_top(const Game *game)
{
    const Man *man = &game->men[game->active_man];
    const Ledge *top = &game->level.ledges[0];

    return man->y + man->lf.y == top->height || man->y + man->rf.y == top->height;
}

static void belay_to_rope(Game *game, TiePoint *point, int state_count)
{
    int index;
    Tie *at = NULL;

    if (point->status == tie_on_man && state_count == 3) {
        point->status = tie_on_man_sliding;
        point->tie->status = tie_on_man_sliding;
        return;
    }

    if (point->status == tie_on_man || point->status == tie_on_man_sliding) {
        if (delete_tie(point->tie)) {
            point->status = tie_off;
            point->tie = NULL;
        }
        return;
    }

    if (!check_rope_touch(point->x, point->y, &game->rope, &at)) {
        return;
    }

    index = get_free_index(&game->rope);
    if (index == mx_ties) {
        return;
    }

    add_tiepoint(at, &game->rope.ties[index], point, tie_on_man);
}

static void process_belay_action(Game *game, int key)
{
    Belay *belay = &game->belays[game->active_man];
    TiePoint *point = NULL;
    int state_count = 0;

    switch (key) {
        case 'w':
            point = &belay->l;
            state_count = 2;
            break;
        case 'b':
            point = &belay->b;
            state_count = 3;
            break;
        case 'o':
            point = &belay->r;
            state_count = 2;
            break;
        default:
            return;
    }

    belay_to_rope(game, point, state_count);
    dangle_ends(&game->rope);
}

static int get_piton_index(TiePoint pitons[mx_pitons], int x, int y)
{
    int i;

    for (i = 0; i < mx_pitons; ++i) {
        if (pitons[i].x == x && pitons[i].y == y) {
            return i;
        }
    }

    for (i = 0; i < mx_pitons; ++i) {
        if (pitons[i].status == tie_off) {
            return i;
        }
    }

    return mx_pitons;
}

static void piton_to_rope(Game *game, TiePoint *piton, Tie *at, int x, int y)
{
    int index;

    switch (piton->status) {
        case tie_on_ledge:
            piton->status = tie_on_ledge_sliding;
            piton->tie->status = tie_on_ledge_sliding;
            return;
        case tie_on_ledge_sliding:
            delete_tie(piton->tie);
            piton->status = tie_off;
            piton->tie = NULL;
            return;
        default:
            index = get_free_index(&game->rope);
            if (index == mx_ties) {
                return;
            }
            piton->status = tie_on_ledge;
            piton->x = x;
            piton->y = y;
            add_tiepoint(at, &game->rope.ties[index], piton, tie_on_ledge);
            return;
    }
}

static void process_piton_action(Game *game, int key)
{
    Man *man = current_man(game);
    Tie *at = NULL;
    int x = 0;
    int y = 0;
    int index;

    if (key == 'i') {
        if (!is_attached(man, &man->rh, game->level.ledges, game->level.count)) {
            return;
        }
        x = limb_world_x(man, &man->rh);
        y = limb_world_y(man, &man->rh);
    } else if (key == 'e') {
        if (!is_attached(man, &man->lh, game->level.ledges, game->level.count)) {
            return;
        }
        x = limb_world_x(man, &man->lh);
        y = limb_world_y(man, &man->lh);
    } else {
        return;
    }

    if (!check_rope_touch(x, y, &game->rope, &at)) {
        return;
    }

    index = get_piton_index(game->pitons, x, y);
    if (index == mx_pitons) {
        return;
    }

    piton_to_rope(game, &game->pitons[index], at, x, y);
    dangle_ends(&game->rope);
}

static void process_pending_actions(Game *game)
{
    if (game->pending_man >= 0) {
        game->active_man = game->pending_man;
        game->pending_man = -1;
    }

    if (game->pending_belay_key != 0) {
        process_belay_action(game, game->pending_belay_key);
        game->pending_belay_key = 0;
    }

    if (game->pending_piton_key != 0) {
        process_piton_action(game, game->pending_piton_key);
        game->pending_piton_key = 0;
    }
}

static bool is_move_key(int key)
{
    switch (key) {
        case 'p':
        case 'l':
        case 'k':
        case 'm':
        case 'q':
        case 'a':
        case 's':
        case 'x':
            return true;
        default:
            return false;
    }
}

bool game_init(
    Game *game,
    const char *level_name,
    const RenderHooks *hooks,
    uint64_t now_ms,
    char *error,
    size_t error_size
)
{
    memset(game, 0, sizeof(*game));

    if (!load_level(&game->level, level_name, error, error_size)) {
        return false;
    }

    if (hooks != NULL) {
        game->hooks = *hooks;
    }

    init_man(&game->men[active_man_dougal], 110, 250);
    init_belay(&game->belays[active_man_dougal], &game->men[active_man_dougal]);

    init_man(&game->men[active_man_peter], 70, 250);
    init_belay(&game->belays[active_man_peter], &game->men[active_man_peter]);

    init_rope(&game->rope, 70, 250, 110, 250);
    init_pitons(game->pitons);

    game->active_man = active_man_dougal;
    game->direction = dir_right;
    game->pending_man = -1;
    game->started_ms = now_ms;

    belay_to_rope(game, &game->belays[active_man_dougal].b, 3);
    belay_to_rope(game, &game->belays[active_man_peter].b, 3);
    dangle_ends(&game->rope);

    return true;
}

void game_press_key(Game *game, int key, uint64_t now_ms)
{
    if (game->round_over) {
        return;
    }

    if (key >= 0 && key < 256) {
        key = tolower(key);
    }

    switch (key) {
        case key_escape:
            game->result = result_retreat;
            game->round_over = true;
            clear_move_batch(game);
            return;
        case key_f1:
            game->pending_man = active_man_peter;
            return;
        case key_f2:
            game->pending_man = active_man_dougal;
            return;
        case key_arrow_left:
            game->direction = dir_left;
            return;
        case key_arrow_right:
            game->direction = dir_right;
            return;
        case key_arrow_up:
            game->direction = dir_up;
            return;
        case key_arrow_down:
            game->direction = dir_down;
            return;
        case 'w':
        case 'b':
        case 'o':
            game->pending_belay_key = key;
            return;
        case 'e':
        case 'i':
            game->pending_piton_key = key;
            return;
        default:
            break;
    }

    if (!is_move_key(key)) {
        return;
    }

    if (game->queued_moves < mx_moves) {
        game->move_queue[game->queued_moves++] = key;
    }
    if (!game->move_batch_active) {
        game->move_batch_active = true;
        game->move_batch_started_ms = now_ms;
    }
}

void game_update(Game *game, uint64_t now_ms)
{
    if (game->round_over) {
        return;
    }

    if (game->move_batch_active && now_ms - game->move_batch_started_ms >= 150) {
        int move_result;

        game->move_count++;
        move_result = move_man(game);
        clear_move_batch(game);

        if (move_result == -1) {
            game->result = result_fall;
            game->round_over = true;
            return;
        }

        if (move_result == 0 && man_on_top(game)) {
            game->result = result_win;
            game->round_over = true;
            return;
        }
    }

    if (!game->move_batch_active) {
        process_pending_actions(game);
    }
}

int game_elapsed_seconds(const Game *game, uint64_t now_ms)
{
    return (int)((now_ms - game->started_ms) / 1000);
}

const char *game_result_title(RoundResult result)
{
    switch (result) {
        case result_fall:
            return "A LONG WAY DOWN";
        case result_win:
            return "TO THE TOP";
        case result_retreat:
            return "RETREAT";
        default:
            return "";
    }
}

const char *game_result_line(RoundResult result, int line)
{
    static const char *fall_lines[] = {
        "YOUR CLIMBING COMPANION WATCHES IN SHOCKED HORROR.",
        "YOU SLIDE DOWN A SHORT ICEFIELD, THEN CARTWHEEL",
        "OUT OVER THE 3,000 FOOT ABYSS.",
        "PRESS ENTER TO CLIMB AGAIN OR ESC TO QUIT."
    };
    static const char *win_lines[] = {
        "IT IS AMAZING. YOU MADE IT TO THE TOP.",
        "THE VIEW OF THE SURROUNDING PEAKS IS SPECTACULAR.",
        "YOU BARELY NOTICE THE FROSTBITE SETTING IN.",
        "PRESS ENTER TO CLIMB AGAIN OR ESC TO QUIT."
    };
    static const char *retreat_lines[] = {
        "YOU DECIDE TO RETREAT FOR TODAY,",
        "BUT YOU ARE HAPPY KNOWING YOU'LL BE BACK.",
        "",
        "PRESS ENTER TO CLIMB AGAIN OR ESC TO QUIT."
    };
    const char **lines = NULL;

    switch (result) {
        case result_fall:
            lines = fall_lines;
            break;
        case result_win:
            lines = win_lines;
            break;
        case result_retreat:
            lines = retreat_lines;
            break;
        default:
            return "";
    }

    if (line < 0 || line > 3) {
        return "";
    }

    return lines[line];
}
