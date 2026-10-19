#include "invaders_internal.h"

invaders_t iv;

uint8_t detect_language(const char *locale)
{
    if (locale[0] == 'e' && locale[1] == 'n') return 1U;
    if (locale[0] == 'e' && locale[1] == 's') return 2U;
    if (locale[0] == 'i' && locale[1] == 't') return 3U;
    if (locale[0] == 'd' && locale[1] == 'e') return 4U;
    if (locale[0] == 'z' && locale[1] == 'h') return 5U;
    return 0U;
}

int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void style_box(invaders_t *self, gm_plugin_lvgl_obj_t *object,
                      uint8_t opacity, uint8_t border, uint8_t radius)
{
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                        color(0xFFU), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_BG_OPA,
                        number(opacity), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_BORDER_COLOR,
                        color(0xFFU), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_BORDER_OPA,
                        number(255), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_BORDER_WIDTH,
                        number(border), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(object, GM_PLUGIN_LVGL_STYLE_RADIUS,
                        number(radius), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->obj_clear_flag(object, GM_PLUGIN_LVGL_FLAG_SCROLLABLE);
}

void style_panel(invaders_t *self, gm_plugin_lvgl_obj_t *label)
{
    style_box(self, label, GM_PLUGIN_LVGL_OPA_80, 1, 4);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                        color(0), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_TEXT_COLOR,
                        color(0xFFU), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_TEXT_ALIGN,
                        number(GM_PLUGIN_LVGL_TEXT_ALIGN_CENTER),
                        GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_PAD_TOP,
                        number(14), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_PAD_BOTTOM,
                        number(14), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_PAD_LEFT,
                        number(14), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(label, GM_PLUGIN_LVGL_STYLE_PAD_RIGHT,
                        number(14), GM_PLUGIN_LVGL_SELECTOR_MAIN);
}

void refresh_header(invaders_t *self)
{
    char text[48];
    char status[24];
    self->libc->snprintf(text, sizeof(text), "%s%u  %s%u",
                         L("Score ", "Score ", "Puntos ", "Punti ", "Punkte ",
                           "\xe5\xbe\x97\xe5\x88\x86 "),
                         (unsigned int)self->score,
                         L("Vie ", "Life ", "Vida ", "Vite ", "Leben ",
                           "\xe5\x91\xbd "),
                         (unsigned int)self->lives);
    self->ui->label_set_text(self->score_label, text);
    if (self->state == STATE_IDLE) {
        self->ui->label_set_text(self->status_label,
            L("Pret", "Ready", "Listo", "Pronto", "Bereit",
              "\xe5\xb0\xb1\xe7\xbb\xaa"));
    } else if (self->state == STATE_PAUSED) {
        self->ui->label_set_text(self->status_label,
            L("Pause", "Pause", "Pausa", "Pausa", "Pause",
              "\xe6\x9a\x82\xe5\x81\x9c"));
    } else if (self->state == STATE_OVER) {
        self->ui->label_set_text(self->status_label,
            L("Fini", "Over", "Fin", "Fine", "Ende",
              "\xe7\xbb\x93\xe6\x9d\x9f"));
    } else {
        self->libc->snprintf(status, sizeof(status), "%s%u",
                             L("Vague ", "Wave ", "Oleada ", "Ondata ",
                               "Welle ", "\xe6\xb3\xa2\xe6\xac\xa1 "),
                             (unsigned int)self->wave);
        self->ui->label_set_text(self->status_label, status);
    }
}

void set_message(invaders_t *self, const char *text, uint8_t visible)
{
    self->ui->label_set_text(self->message_label, text);
    if (visible != 0U) {
        gm_plugin_lvgl_point_t panel_size = {0, 0};
        int16_t width = self->screen_width - MESSAGE_SIDE_MARGIN * 2;
        int16_t height;
        if (width < MESSAGE_MIN_W) width = MESSAGE_MIN_W;
        self->ui->text_get_size(&panel_size, text, self->ui->font_default,
                                0, 0, width - 28,
                                GM_PLUGIN_LVGL_TEXT_FLAG_NONE);
        height = (int16_t)panel_size.y + 34;
        if (height < MESSAGE_MIN_H) height = MESSAGE_MIN_H;
        if (height > self->screen_height - 56)
            height = self->screen_height - 56;
        self->ui->obj_set_size(self->message_label, width, height);
        self->ui->obj_align(self->message_label,
                            GM_PLUGIN_LVGL_ALIGN_CENTER, 0, 0);
        self->ui->obj_clear_flag(self->message_label,
                                 GM_PLUGIN_LVGL_FLAG_HIDDEN);
    } else {
        self->ui->obj_add_flag(self->message_label,
                               GM_PLUGIN_LVGL_FLAG_HIDDEN);
    }
}

void send_status(invaders_t *self)
{
    uint8_t payload[8];
    payload[0] = PROTOCOL_VERSION;
    payload[1] = EVENT_STATUS;
    payload[2] = (uint8_t)(self->score & 0xFFU);
    payload[3] = (uint8_t)((self->score >> 8) & 0xFFU);
    payload[4] = self->wave;
    payload[5] = self->lives;
    payload[6] = (uint8_t)(self->state == STATE_PLAYING ||
                           self->state == STATE_PAUSED);
    payload[7] = self->state;
    (void)self->host->bt_send(EVENT_CHANNEL, payload, sizeof(payload));
    self->status_accumulator = 0U;
}

void place_formation(invaders_t *self)
{
    uint8_t row;
    uint8_t col;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            self->ui->obj_set_pos(self->aliens[row][col],
                (int16_t)(self->alien_origin_x + self->alien_offset +
                          col * (ALIEN_W + ALIEN_GAP_X)),
                (int16_t)(self->alien_origin_y + row * (ALIEN_H + ALIEN_GAP_Y)));
        }
    }
}

void build_formation(invaders_t *self)
{
    uint8_t row;
    uint8_t col;
    self->aliens_remaining = COLS * ROWS;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            self->alive[row][col] = 1U;
            self->ui->obj_clear_flag(self->aliens[row][col],
                                     GM_PLUGIN_LVGL_FLAG_HIDDEN);
        }
    }
    self->alien_origin_x = ALIEN_START_X;
    self->alien_origin_y = ALIEN_TOP;
    self->alien_offset = 0;
    self->alien_direction = 1U;
    self->enemy_cursor = 0U;
    self->descent_accumulator = 0U;
    self->march_accumulator = 0U;
    place_formation(self);
}

void reset_shot(invaders_t *self)
{
    self->bullet_active = 0U;
    self->enemy_bullet_active = 0U;
    self->fire_accumulator = 0U;
    self->enemy_fire_accumulator = 0U;
    self->ui->obj_add_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    self->ui->obj_add_flag(self->enemy_bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
}

void layout_ship(invaders_t *self)
{
    self->ship_x = ((self->board_width - SHIP_W) / 2) * Q;
    self->ship_vx = 0;
    self->ui->obj_set_pos(self->ship, (int16_t)(self->ship_x / Q),
                          (int16_t)(self->ship_y - 2));
}

void new_game(invaders_t *self)
{
    self->score = 0;
    self->wave = 1U;
    self->lives = START_LIVES;
    self->state = STATE_PLAYING;
    build_formation(self);
    reset_shot(self);
    layout_ship(self);
    set_message(self, "", 0U);
    refresh_header(self);
    send_status(self);
}
