#include "invaders_internal.h"

void move_ship(invaders_t *self, const gm_plugin_imu_sample_t *imu)
{
    /* Horizontal head turns come from the rotated X/Y axes; gyro Z is the
     * vertical (nod) axis. */
    int32_t horizontal = (int32_t)imu->gyro_raw[0] - (int32_t)imu->gyro_raw[1];
    int32_t magnitude = horizontal < 0 ? -horizontal : horizontal;
    int32_t target = 0;
    int32_t maximum = (self->board_width - SHIP_W) * Q;
    int32_t speed;
    int32_t divisor;
    if (magnitude > GYRO_THRESHOLD) {
        /* Higher sensitivity means a smaller gyro divisor. */
        divisor = 26 - (int32_t)self->sensitivity * 2;
        speed = magnitude / divisor + 3;
        if (speed < 4) speed = 4;
        if (speed > 20) speed = 20;
        target = horizontal < 0 ? speed * Q : -speed * Q;
    }
    self->ship_vx += (target - self->ship_vx) / 2;
    self->ship_x += self->ship_vx;
    if (self->ship_x < 0) {
        self->ship_x = 0;
        self->ship_vx = 0;
    } else if (self->ship_x > maximum) {
        self->ship_x = maximum;
        self->ship_vx = 0;
    }
    self->ui->obj_set_pos(self->ship, (int16_t)(self->ship_x / Q),
                          (int16_t)(self->ship_y - 2));
}

void start_shot(invaders_t *self)
{
    if (self->bullet_active != 0U) return;
    self->bullet_active = 1U;
    self->bullet_y = (int32_t)(self->ship_y - BULLET_H) * Q;
    self->ui->obj_set_pos(self->bullet,
                          (int16_t)(self->ship_x / Q + SHIP_W / 2),
                          (int16_t)(self->ship_y - BULLET_H));
    self->ui->obj_clear_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
}

static uint8_t column_has_alien(const invaders_t *self, uint8_t col)
{
    uint8_t row;
    for (row = 0; row < ROWS; ++row)
        if (self->alive[row][col] != 0U) return 1U;
    return 0U;
}

void start_enemy_shot(invaders_t *self)
{
    uint8_t attempt;
    if (self->enemy_bullet_active != 0U || self->aliens_remaining == 0U)
        return;
    for (attempt = 0; attempt < COLS; ++attempt) {
        uint8_t col = (uint8_t)((self->enemy_cursor + attempt) % COLS);
        int8_t row;
        int16_t alien_y;
        int32_t x;
        int32_t maximum = self->board_width - ENEMY_SHOT_W;
        if (column_has_alien(self, col) == 0U) continue;
        row = (int8_t)(ROWS - 1);
        while (row > 0 && self->alive[row][col] == 0U) --row;
        alien_y = (int16_t)(self->alien_origin_y +
                            row * (ALIEN_H + ALIEN_GAP_Y));
        x = self->alien_origin_x + self->alien_offset +
            col * (ALIEN_W + ALIEN_GAP_X) + ALIEN_W / 2;
        if (x > maximum) x = maximum;
        if (x < 0) x = 0;
        self->enemy_cursor = (uint8_t)((col + 1U) % COLS);
        self->enemy_bullet_active = 1U;
        self->enemy_bullet_x = x;
        self->enemy_bullet_y = (int32_t)(alien_y + ALIEN_H) * Q;
        self->ui->obj_set_pos(self->enemy_bullet, (int16_t)x,
                              (int16_t)(alien_y + ALIEN_H));
        self->ui->obj_clear_flag(self->enemy_bullet,
                                 GM_PLUGIN_LVGL_FLAG_HIDDEN);
        return;
    }
}

/* Returns the row of the alien hit, or -1. Hides the alien on a hit. */
static int32_t alien_at(invaders_t *self, int32_t x, int32_t y,
                        int32_t w, int32_t h)
{
    uint8_t row;
    uint8_t col;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            int32_t ax;
            int32_t ay;
            if (self->alive[row][col] == 0U) continue;
            ax = self->alien_origin_x + self->alien_offset +
                 col * (ALIEN_W + ALIEN_GAP_X);
            ay = self->alien_origin_y + row * (ALIEN_H + ALIEN_GAP_Y);
            if (x + w <= ax || x >= ax + ALIEN_W) continue;
            if (y + h <= ay || y >= ay + ALIEN_H) continue;
            self->alive[row][col] = 0U;
            self->ui->obj_add_flag(self->aliens[row][col],
                                   GM_PLUGIN_LVGL_FLAG_HIDDEN);
            --self->aliens_remaining;
            return row;
        }
    }
    return -1;
}

static void update_bullet(invaders_t *self)
{
    int32_t x;
    int32_t y;
    int32_t hit;
    if (self->bullet_active == 0U) return;
    self->bullet_y -= SHOT_SPEED * Q;
    y = self->bullet_y / Q;
    x = self->ship_x / Q + SHIP_W / 2;
    hit = alien_at(self, x, y, BULLET_W, BULLET_H);
    if (hit >= 0) {
        self->bullet_active = 0U;
        self->ui->obj_add_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
        self->score = (uint16_t)(self->score +
                                 (uint16_t)(ROWS - (uint32_t)hit) * 10U);
        refresh_header(self);
        return;
    }
    if (y < -BULLET_H) {
        self->bullet_active = 0U;
        self->ui->obj_add_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
        return;
    }
    self->ui->obj_set_pos(self->bullet, (int16_t)x, (int16_t)y);
}

static void finish_game(invaders_t *self, uint8_t won)
{
    char text[192];
    self->state = STATE_OVER;
    self->libc->snprintf(
        text, sizeof(text), "%s\n%s%u\n%s",
        won != 0U ?
            L("Victoire !", "You win!", "Victoria!", "Vittoria!",
              "Gewonnen!", "\xe8\x83\x9c\xe5\x88\xa9\xef\xbc\x81") :
            L("Partie terminee", "Game over", "Fin de la partida",
              "Partita finita", "Spiel vorbei",
              "\xe6\xb8\xb8\xe6\x88\x8f\xe7\xbb\x93\xe6\x9d\x9f"),
        L("Score ", "Score ", "Puntos ", "Punti ", "Punkte ",
          "\xe5\xbe\x97\xe5\x88\x86 "),
        (unsigned int)self->score,
        L("Bouton : rejouer", "Button: play again", "Boton: jugar",
          "Pulsante: rigioca", "Taste: nochmal",
          "\xe6\x8c\x89\xe9\x92\xae\xef\xbc\x9a\xe5\x86\x8d\xe7\x8e\xa9"));
    set_message(self, text, 1U);
    refresh_header(self);
    send_status(self);
}

void next_wave(invaders_t *self)
{
    if (self->wave < 255U) ++self->wave;
    build_formation(self);
    reset_shot(self);
    if (self->alien_origin_y > self->ship_y - ALIEN_H - 40)
        self->alien_origin_y = ALIEN_TOP;
    refresh_header(self);
    send_status(self);
}

static void lose_life(invaders_t *self)
{
    if (self->lives > 0U) --self->lives;
    self->ui->obj_add_flag(self->enemy_bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    self->ui->obj_add_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    self->enemy_bullet_active = 0U;
    self->bullet_active = 0U;
    if (self->lives == 0U) {
        finish_game(self, 0U);
        return;
    }
    reset_shot(self);
    layout_ship(self);
    refresh_header(self);
    send_status(self);
}

static void update_enemy_bullet(invaders_t *self)
{
    int32_t y;
    int32_t ship_left;
    int32_t ship_right;
    if (self->enemy_bullet_active == 0U) return;
    self->enemy_bullet_y += ENEMY_SHOT_SPEED * Q;
    y = self->enemy_bullet_y / Q;
    if (y > self->board_height) {
        self->enemy_bullet_active = 0U;
        self->ui->obj_add_flag(self->enemy_bullet,
                               GM_PLUGIN_LVGL_FLAG_HIDDEN);
        return;
    }
    ship_left = self->ship_x / Q;
    ship_right = ship_left + SHIP_W;
    if (y + ENEMY_SHOT_H >= self->ship_y &&
        y <= self->ship_y + SHIP_H &&
        self->enemy_bullet_x + ENEMY_SHOT_W > ship_left &&
        self->enemy_bullet_x < ship_right) {
        lose_life(self);
        return;
    }
    self->ui->obj_set_pos(self->enemy_bullet,
                          (int16_t)self->enemy_bullet_x, (int16_t)y);
}

/* One marching step: the formation slides sideways, reverses on the edge and
 * steps down when it does. Crossing the ship line costs a life. */
static void march(invaders_t *self)
{
    int32_t limit;
    uint8_t row;
    uint8_t col;
    int32_t lowest_y = 0;
    limit = self->board_width - ALIEN_W - ALIEN_START_X;
    if (self->alien_direction != 0U) self->alien_offset += 2;
    else self->alien_offset -= 2;
    if (self->alien_offset <= 0) {
        self->alien_offset = 0;
        self->alien_direction = 1U;
        self->alien_origin_y += ALIEN_STEP_Y;
    } else if (self->alien_offset >= limit) {
        self->alien_offset = limit;
        self->alien_direction = 0U;
        self->alien_origin_y += ALIEN_STEP_Y;
    }
    place_formation(self);
    for (row = 0; row < ROWS; ++row) {
        int32_t row_y = self->alien_origin_y + row * (ALIEN_H + ALIEN_GAP_Y);
        for (col = 0; col < COLS; ++col)
            if (self->alive[row][col] != 0U && row_y > lowest_y)
                lowest_y = row_y;
    }
    if (lowest_y + ALIEN_H >= self->ship_y) {
        self->state = STATE_OVER;
        finish_game(self, 0U);
    }
}

void step_frame(invaders_t *self, uint32_t elapsed_ms, uint32_t now)
{
    static const uint16_t fire_intervals[MAX_FIRE_RATE + 1U] = {
        0U, 900U, 780U, 670U, 570U, 480U, 400U, 330U, 270U, 220U, 180U,
    };
    uint32_t interval;
    (void)now;
    if (self->state != STATE_PLAYING) return;
    self->fire_accumulator += elapsed_ms;
    self->enemy_fire_accumulator += elapsed_ms;
    self->march_accumulator += elapsed_ms;
    self->descent_accumulator += elapsed_ms;

    interval = fire_intervals[self->fire_rate > MAX_FIRE_RATE ?
                              MAX_FIRE_RATE : self->fire_rate];
    if (self->fire_mode == FIRE_MODE_AUTO) {
        if (self->fire_accumulator >= interval) {
            self->fire_accumulator = 0U;
            start_shot(self);
        }
    } else {
        self->fire_accumulator = 0U;
    }
    if (self->enemy_fire_accumulator >= ENEMY_SHOT_INTERVAL_MS) {
        self->enemy_fire_accumulator = 0U;
        start_enemy_shot(self);
    }
    if (self->march_accumulator >= ALIEN_STEP_MS) {
        self->march_accumulator = 0U;
        march(self);
        if (self->state != STATE_PLAYING) return;
    }
    if (self->descent_accumulator >= ALIEN_CYCLE_MS) {
        self->descent_accumulator = 0U;
        self->alien_origin_y += ALIEN_STEP_Y;
        place_formation(self);
    }
    update_bullet(self);
    update_enemy_bullet(self);
    if (self->aliens_remaining == 0U) {
        self->score = (uint16_t)(self->score + 50U);
        next_wave(self);
    }
}
