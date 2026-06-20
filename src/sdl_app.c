#include "sdl_app.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    color_black = 0,
    color_blue = 1,
    color_green = 2,
    color_red = 4,
    color_grey = 8,
    color_light_blue = 11,
    color_light_red = 12,
    color_yellow = 14,
    color_white = 15
};

typedef struct {
    char ch;
    uint8_t rows[7];
} Glyph;

typedef struct {
    unsigned pitch;
    unsigned duration;
} Tone;

static const SDL_Color ega_palette[16] = {
    {0, 0, 0, 255},
    {0, 0, 170, 255},
    {0, 170, 0, 255},
    {0, 170, 170, 255},
    {170, 0, 0, 255},
    {170, 0, 170, 255},
    {170, 85, 0, 255},
    {170, 170, 170, 255},
    {85, 85, 85, 255},
    {85, 85, 255, 255},
    {85, 255, 85, 255},
    {85, 255, 255, 255},
    {255, 85, 85, 255},
    {255, 85, 255, 255},
    {255, 255, 85, 255},
    {255, 255, 255, 255}
};

static const Glyph glyphs[] = {
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'B', {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}},
    {'C', {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e}},
    {'D', {0x1c, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1c}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'F', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}},
    {'G', {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'J', {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0e}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'Q', {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e}},
    {'6', {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x08}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'?', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {'=', {0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00}},
    {'/', {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}},
    {'<', {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}},
    {'>', {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}},
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {'\'', {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}},
    {'"', {0x0a, 0x0a, 0x04, 0x00, 0x00, 0x00, 0x00}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

static const Tone intro_tune[] = {
    {8000, 5}, {6000, 10}, {5400, 10}, {4750, 5}, {4000, 5}, {3500, 5}, {3000, 5},
    {4000, 10}, {4750, 5}, {4500, 5}, {4000, 10}, {4750, 5}, {4000, 5},
    {3500, 10}, {4750, 10}, {4000, 30},
    {100, 25}, {4750, 5},
    {3500, 10}, {3750, 10}, {4750, 5}, {5650, 5}, {5400, 5}, {4250, 5},
    {3500, 10}, {2825, 10}, {4750, 10}, {100, 5}, {3500, 5},
    {3150, 20}, {3500, 10}, {100, 10},
    {3150, 25}, {3500, 5}, {4500, 5}, {6400, 5}, {6000, 10},
    {24000, 5}, {8000, 5}, {6000, 5}, {4750, 10}, {3000, 40},
    {0, 0}
};

static const uint8_t *glyph_rows(char c)
{
    size_t i;
    unsigned char upper = (unsigned char)toupper((unsigned char)c);

    for (i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); ++i) {
        if ((unsigned char)glyphs[i].ch == upper) {
            return glyphs[i].rows;
        }
    }

    return glyphs[sizeof(glyphs) / sizeof(glyphs[0]) - 1].rows;
}

static void set_color(App *app, int index)
{
    SDL_Color color = ega_palette[index & 15];

    SDL_SetRenderDrawColor(app->renderer, color.r, color.g, color.b, 255);
}

static void clear_screen(App *app, int color_index)
{
    set_color(app, color_index);
    SDL_RenderClear(app->renderer);
}

static void draw_ellipse(App *app, int x1, int y1, int x2, int y2, bool filled)
{
    int center_x = (x1 + x2) / 2;
    int center_y = (y1 + y2) / 2;
    int rx = abs(x2 - x1) / 2;
    int ry = abs(y2 - y1) / 2;
    int y;

    if (rx <= 0 && ry <= 0) {
        SDL_RenderDrawPoint(app->renderer, center_x, center_y);
        return;
    }

    if (rx <= 0) {
        SDL_RenderDrawLine(app->renderer, center_x, y1, center_x, y2);
        return;
    }
    if (ry <= 0) {
        SDL_RenderDrawLine(app->renderer, x1, center_y, x2, center_y);
        return;
    }

    for (y = -ry; y <= ry; ++y) {
        double ratio = 1.0 - ((double)(y * y) / (double)(ry * ry));
        int span = (int)lround((double)rx * sqrt(ratio < 0.0 ? 0.0 : ratio));

        if (filled) {
            SDL_RenderDrawLine(app->renderer, center_x - span, center_y + y, center_x + span, center_y + y);
        } else {
            SDL_RenderDrawPoint(app->renderer, center_x - span, center_y + y);
            SDL_RenderDrawPoint(app->renderer, center_x + span, center_y + y);
        }
    }
}

static void draw_char(App *app, int x, int y, char c, int color_index)
{
    const uint8_t *rows = glyph_rows(c);
    int row;
    int col;

    set_color(app, color_index);
    for (row = 0; row < 7; ++row) {
        for (col = 0; col < 5; ++col) {
            if (rows[row] & (1 << (4 - col))) {
                SDL_RenderDrawPoint(app->renderer, x + col, y + row);
            }
        }
    }
}

static void draw_text(App *app, int col, int row, int color_index, const char *text)
{
    int x = (col - 1) * 8;
    int y = (row - 1) * 8;
    int start_x = x;
    const unsigned char *cursor = (const unsigned char *)text;

    while (*cursor != '\0') {
        if (*cursor == '\n') {
            y += 8;
            x = start_x;
        } else {
            draw_char(app, x, y, (char)*cursor, color_index);
            x += 8;
        }
        cursor++;
    }
}

static void draw_ledge(App *app, const Ledge *ledge)
{
    set_color(app, color_grey);
    SDL_RenderDrawLine(app->renderer, ledge->left_end, ledge->height, ledge->right_end, ledge->height);
    SDL_RenderDrawLine(app->renderer, ledge->left_end, ledge->height - 1, ledge->right_end, ledge->height - 1);
}

static void draw_man(App *app, const Man *man, int color_index)
{
    int body_top_x = man->x;
    int body_top_y = man->y - 5;
    int body_bottom_y = man->y;

    set_color(app, color_index);
    SDL_RenderDrawLine(app->renderer, body_top_x, body_top_y, body_top_x, body_bottom_y);
    SDL_RenderDrawLine(app->renderer, body_top_x, body_top_y, body_top_x + man->lh.lr * man->lh.x, body_bottom_y - man->lh.du * man->lh.y);
    SDL_RenderDrawLine(app->renderer, body_top_x, body_top_y, body_top_x + man->rh.lr * man->rh.x, body_bottom_y - man->rh.du * man->rh.y);
    SDL_RenderDrawLine(app->renderer, body_top_x, body_bottom_y, body_top_x + man->lf.lr * man->lf.x, body_bottom_y - man->lf.du * man->lf.y);
    SDL_RenderDrawLine(app->renderer, body_top_x, body_bottom_y, body_top_x + man->rf.lr * man->rf.x, body_bottom_y - man->rf.du * man->rf.y);
    draw_ellipse(app, body_top_x - 2, body_top_y - 8, body_top_x + 2, body_top_y - 3, false);
}

static void draw_rope(App *app, const Rope *rope)
{
    const Tie *tie = &rope->ties[0];
    const Tie *next = tie->post;

    set_color(app, color_light_red);
    while (next != NULL) {
        SDL_RenderDrawLine(app->renderer, *tie->x, *tie->y, *next->x, *next->y);
        tie = next;
        next = next->post;
    }
}

static void draw_arrow(App *app, int direction)
{
    int x = 535;
    int y = 445;
    int tip_x;
    int tip_y;

    set_color(app, color_yellow);
    switch (direction) {
        case dir_up:
            tip_x = x + 10;
            tip_y = y + 3;
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, tip_x, y + 20);
            SDL_RenderDrawLine(app->renderer, tip_x + 1, tip_y, tip_x + 1, y + 20);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 3, y + 10);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 17, y + 10);
            break;
        case dir_down:
            tip_x = x + 10;
            tip_y = y + 17;
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, tip_x, y);
            SDL_RenderDrawLine(app->renderer, tip_x + 1, tip_y, tip_x + 1, y);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 3, y + 10);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 17, y + 10);
            break;
        case dir_left:
            tip_x = x + 3;
            tip_y = y + 10;
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 20, tip_y);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y + 1, x + 20, tip_y + 1);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 10, y + 3);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 10, y + 17);
            break;
        case dir_right:
        default:
            tip_x = x + 17;
            tip_y = y + 10;
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x, tip_y);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y + 1, x, tip_y + 1);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 10, y + 3);
            SDL_RenderDrawLine(app->renderer, tip_x, tip_y, x + 10, y + 17);
            break;
    }
}

static void draw_dot(App *app, int x, int y, int fill_color, bool fill)
{
    set_color(app, fill_color);
    draw_ellipse(app, x - 2, y - 2, x + 2, y + 2, fill);
}

static void draw_belay_status(App *app, const Belay *belay, int origin_x, int origin_y)
{
    int x = origin_x;

    if (belay->l.status == tie_off) {
        draw_dot(app, x, origin_y, color_yellow, false);
    } else {
        draw_dot(app, x, origin_y, color_yellow, true);
    }

    x += 7;
    if (belay->b.status == tie_off) {
        draw_dot(app, x, origin_y, color_yellow, false);
    } else if (belay->b.status == tie_on_man_sliding) {
        draw_dot(app, x, origin_y, color_light_blue, true);
    } else {
        draw_dot(app, x, origin_y, color_yellow, true);
    }

    x += 7;
    if (belay->r.status == tie_off) {
        draw_dot(app, x, origin_y, color_yellow, false);
    } else {
        draw_dot(app, x, origin_y, color_yellow, true);
    }
}

static void draw_pitons(App *app, const TiePoint pitons[mx_pitons])
{
    int i;

    for (i = 0; i < mx_pitons; ++i) {
        if (pitons[i].status == tie_on_ledge) {
            draw_dot(app, pitons[i].x, pitons[i].y, color_yellow, true);
        } else if (pitons[i].status == tie_on_ledge_sliding) {
            draw_dot(app, pitons[i].x, pitons[i].y, color_light_blue, true);
        }
    }
}

static void draw_round_hud(App *app, const Game *game, uint64_t now_ms)
{
    char buffer[64];

    draw_text(app, 15, 29, game->active_man == 0 ? color_yellow : color_light_blue, "DOUGALH:");
    draw_text(app, 1, 29, game->active_man == 1 ? color_yellow : color_light_blue, "PETERB:");

    snprintf(buffer, sizeof(buffer), "MOVE = %d", game->move_count);
    draw_text(app, 40, 29, color_yellow, buffer);

    snprintf(buffer, sizeof(buffer), "%d:%02d", game_elapsed_seconds(game, now_ms) / 60, game_elapsed_seconds(game, now_ms) % 60);
    draw_text(app, 56, 29, color_yellow, buffer);

    draw_belay_status(app, &game->belays[0], 191, 457);
    draw_belay_status(app, &game->belays[1], 71, 457);
    draw_arrow(app, game->direction);
}

static void draw_level_name(App *app, const char *path)
{
    const char *base = strrchr(path, '/');
    char buffer[64];

    base = base == NULL ? path : base + 1;
    snprintf(buffer, sizeof(buffer), "LEVEL: %s", base);
    draw_text(app, 1, 1, color_white, buffer);
}

bool app_init(App *app, bool hidden, char *error, size_t error_size)
{
    Uint32 window_flags = SDL_WINDOW_RESIZABLE;

    memset(app, 0, sizeof(*app));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        snprintf(error, error_size, "sdl init failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    if (hidden) {
        window_flags |= SDL_WINDOW_HIDDEN;
    }

    app->window = SDL_CreateWindow(
        "Attack the Cliffs",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        screen_width * 2,
        screen_height * 2,
        window_flags
    );
    if (app->window == NULL) {
        snprintf(error, error_size, "window creation failed: %s", SDL_GetError());
        app_shutdown(app);
        return false;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (app->renderer == NULL) {
        app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (app->renderer == NULL) {
        snprintf(error, error_size, "renderer creation failed: %s", SDL_GetError());
        app_shutdown(app);
        return false;
    }

    SDL_RenderSetLogicalSize(app->renderer, screen_width, screen_height);
    SDL_RenderSetIntegerScale(app->renderer, SDL_TRUE);

    {
        SDL_AudioSpec want;
        SDL_AudioSpec have;

        SDL_zero(want);
        want.freq = 48000;
        want.format = AUDIO_F32SYS;
        want.channels = 1;
        want.samples = 1024;

        app->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (app->audio_device != 0) {
            app->audio_ok = true;
            SDL_PauseAudioDevice(app->audio_device, 0);
        }
    }

    return true;
}

void app_shutdown(App *app)
{
    if (app->win_texture != NULL) {
        SDL_DestroyTexture(app->win_texture);
        app->win_texture = NULL;
    }
    if (app->audio_device != 0) {
        SDL_CloseAudioDevice(app->audio_device);
        app->audio_device = 0;
    }
    if (app->renderer != NULL) {
        SDL_DestroyRenderer(app->renderer);
        app->renderer = NULL;
    }
    if (app->window != NULL) {
        SDL_DestroyWindow(app->window);
        app->window = NULL;
    }
    SDL_Quit();
}

static void queue_tone(App *app, unsigned pitch, unsigned duration_units)
{
    float *samples;
    int sample_count;
    int i;
    const int sample_rate = 48000;
    const float seconds = (float)duration_units * 0.05f;

    if (!app->audio_ok || duration_units == 0) {
        return;
    }

    sample_count = (int)(seconds * (float)sample_rate);
    if (sample_count <= 0) {
        return;
    }

    samples = (float *)malloc((size_t)sample_count * sizeof(float));
    if (samples == NULL) {
        return;
    }

    if (pitch == 100) {
        memset(samples, 0, (size_t)sample_count * sizeof(float));
    } else {
        float frequency = 1193180.0f / (float)pitch;
        float phase = 0.0f;
        float phase_step = frequency / (float)sample_rate;

        for (i = 0; i < sample_count; ++i) {
            float envelope = 1.0f;

            if (i > sample_count - 400) {
                envelope = (float)(sample_count - i) / 400.0f;
                if (envelope < 0.0f) {
                    envelope = 0.0f;
                }
            }
            samples[i] = (phase < 0.5f ? 0.075f : -0.075f) * envelope;
            phase += phase_step;
            if (phase >= 1.0f) {
                phase -= 1.0f;
            }
        }
    }

    SDL_QueueAudio(app->audio_device, samples, (Uint32)((size_t)sample_count * sizeof(float)));
    free(samples);
}

void app_play_intro_tune(App *app)
{
    int i;

    if (!app->audio_ok) {
        return;
    }

    SDL_ClearQueuedAudio(app->audio_device);
    for (i = 0; intro_tune[i].pitch != 0 || intro_tune[i].duration != 0; ++i) {
        queue_tone(app, intro_tune[i].pitch, intro_tune[i].duration);
    }
}

void app_stop_audio(App *app)
{
    if (app->audio_ok) {
        SDL_ClearQueuedAudio(app->audio_device);
    }
}

bool app_load_win_image(App *app, const char *path, char *error, size_t error_size)
{
    FILE *fp = fopen(path, "rb");
    const int width = 400;
    const int height = 400;
    size_t count;
    unsigned char *indices;
    uint32_t *pixels;
    int i;

    if (fp == NULL) {
        snprintf(error, error_size, "could not open %s", path);
        return false;
    }

    indices = (unsigned char *)malloc((size_t)width * (size_t)height);
    pixels = (uint32_t *)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    if (indices == NULL || pixels == NULL) {
        free(indices);
        free(pixels);
        fclose(fp);
        snprintf(error, error_size, "not enough memory for %s", path);
        return false;
    }

    count = fread(indices, 1, (size_t)width * (size_t)height, fp);
    fclose(fp);
    if (count != (size_t)width * (size_t)height) {
        free(indices);
        free(pixels);
        snprintf(error, error_size, "unexpected size for %s", path);
        return false;
    }

    for (i = 0; i < width * height; ++i) {
        SDL_Color color = ega_palette[indices[i] & 15];
        pixels[i] = ((uint32_t)255 << 24) | ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
    }

    app->win_texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (app->win_texture != NULL) {
        SDL_UpdateTexture(app->win_texture, NULL, pixels, width * (int)sizeof(uint32_t));
        app->win_texture_width = width;
        app->win_texture_height = height;
    }

    free(indices);
    free(pixels);
    return app->win_texture != NULL;
}

void app_render_instructions(App *app, const char *level_name)
{
    clear_screen(app, color_black);
    draw_text(app, 21, 3, color_white, "ATTACK THE CLIFFS");
    draw_text(app, 17, 6, color_yellow, "PRESS ENTER TO START");
    draw_text(app, 19, 8, color_light_blue, "PRESS ESC TO QUIT");
    draw_text(app, 5, 11, color_white, "F1  = SELECT PETERB");
    draw_text(app, 5, 12, color_white, "F2  = SELECT DOUGALH");
    draw_text(app, 5, 13, color_white, "ARROWS = SELECT DIRECTION");
    draw_text(app, 5, 16, color_white, "LEFT HAND   Q / A");
    draw_text(app, 5, 17, color_white, "LEFT FOOT   S / X");
    draw_text(app, 5, 18, color_white, "RIGHT FOOT  K / M");
    draw_text(app, 5, 19, color_white, "RIGHT HAND  P / L");
    draw_text(app, 5, 22, color_white, "BELAY MAN   W / B / O");
    draw_text(app, 5, 23, color_white, "BELAY LEDGE E / I");
    draw_text(app, 5, 26, color_white, "HIT MOVEMENT KEYS QUICKLY TO COMBINE MOVES.");
    draw_text(app, 5, 27, color_white, "USE THE ROPE CREATIVELY.");
    draw_text(app, 5, 29, color_yellow, "LEVEL:");
    draw_text(app, 13, 29, color_white, level_name);
}

void app_render_game(App *app, const Game *game, uint64_t now_ms)
{
    int i;

    clear_screen(app, color_black);
    draw_level_name(app, game->level.path);

    for (i = 0; i < game->level.count; ++i) {
        draw_ledge(app, &game->level.ledges[i]);
    }

    draw_rope(app, &game->rope);
    draw_pitons(app, game->pitons);
    draw_man(app, &game->men[0], game->active_man == 0 ? color_yellow : color_light_blue);
    draw_man(app, &game->men[1], game->active_man == 1 ? color_yellow : color_light_blue);
    draw_round_hud(app, game, now_ms);
}

void app_render_result(App *app, const Game *game)
{
    SDL_Rect dst;
    int line;
    char buffer[64];
    int start_row = 24;

    clear_screen(app, color_black);
    draw_text(app, 25, 3, color_white, game_result_title(game->result));

    if (game->result == result_win && app->win_texture != NULL) {
        dst.w = 320;
        dst.h = 320;
        dst.x = (screen_width - dst.w) / 2;
        dst.y = 40;
//        SDL_RenderCopy(app->renderer, app->win_texture, NULL, &dst);
        SDL_RenderCopyEx(
            app->renderer,
            app->win_texture,
            NULL,
            &dst,
            90.0,
            NULL,
            SDL_FLIP_NONE);        
        start_row = 47;
    }

    for (line = 0; line < 4; ++line) {
        draw_text(app, 4, start_row + line, line == 3 ? color_light_blue : color_white, game_result_line(game->result, line));
    }

    snprintf(buffer, sizeof(buffer), "TOTAL NUMBER OF MOVES = %d", game->move_count);
    draw_text(app, 18, game->result == result_win ? 6 : 21, color_yellow, buffer);
}

void app_present(App *app)
{
    SDL_RenderPresent(app->renderer);
}

void app_render_hook(void *userdata, const Game *game)
{
    App *app = (App *)userdata;

    app_render_game(app, game, SDL_GetTicks64());
    app_present(app);
}

void app_delay_hook(void *userdata, uint32_t ms)
{
    (void)userdata;
    SDL_Delay(ms);
}
