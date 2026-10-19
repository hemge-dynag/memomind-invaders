#include "invaders_internal.h"

static gm_plugin_result_t create_sprite(invaders_t *self,
                                        gm_plugin_lvgl_obj_t **out,
                                        gm_plugin_lvgl_obj_t *parent,
                                        uint8_t opacity, uint8_t border,
                                        uint8_t radius, int16_t width,
                                        int16_t height)
{
    gm_plugin_lvgl_obj_t *object = self->ui->obj_create(parent);
    if (object == 0) return GM_PLUGIN_ENOMEM;
    self->ui->obj_set_size(object, width, height);
    style_box(self, object, opacity, border, radius);
    *out = object;
    return GM_PLUGIN_OK;
}

static gm_plugin_result_t create_alien(invaders_t *self, uint8_t row,
                                       uint8_t col)
{
    gm_plugin_result_t result;
    gm_plugin_lvgl_obj_t *alien;
    static const uint8_t opacities[ROWS] = {225U, 160U, 255U};
    result = create_sprite(self, &alien, self->board, opacities[row],
                           (uint8_t)(row == 2U ? 2U : 1U),
                           (uint8_t)(row == 2U ? 2U : 6U), ALIEN_W, ALIEN_H);
    if (result != GM_PLUGIN_OK) return result;
    self->aliens[row][col] = alien;
    if (row == 1U) {
        gm_plugin_lvgl_obj_t *core;
        result = create_sprite(self, &core, alien, 255, 0, 2,
                               ALIEN_W - 14, ALIEN_H - 8);
        if (result != GM_PLUGIN_OK) return result;
        self->ui->obj_set_pos(core, 7, 4);
        self->ui->style_set(core, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                            color(0), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    }
    return GM_PLUGIN_OK;
}

static gm_plugin_result_t create_player(invaders_t *self)
{
    gm_plugin_result_t result;
    gm_plugin_lvgl_obj_t *nose;
    result = create_sprite(self, &self->ship, self->board, 255, 1, 3,
                           SHIP_W, SHIP_H);
    if (result != GM_PLUGIN_OK) return result;
    result = create_sprite(self, &self->ship_body, self->ship, 255, 0, 2,
                           SHIP_W - 10, SHIP_H - 4);
    if (result != GM_PLUGIN_OK) return result;
    self->ui->obj_set_pos(self->ship_body, 5, 2);
    result = create_sprite(self, &nose, self->ship, 255, 0, 3, 8, 6);
    if (result != GM_PLUGIN_OK) return result;
    self->ui->obj_set_pos(nose, (SHIP_W - 8) / 2, 0);
    result = create_sprite(self, &self->bullet, self->board, 255, 0, 1,
                           BULLET_W, BULLET_H);
    if (result != GM_PLUGIN_OK) return result;
    self->ui->obj_set_pos(self->bullet, 0, 0);
    result = create_sprite(self, &self->enemy_bullet, self->board, 200, 0, 1,
                           ENEMY_SHOT_W, ENEMY_SHOT_H);
    if (result != GM_PLUGIN_OK) return result;
    self->ui->obj_set_pos(self->enemy_bullet, 0, 0);
    self->ui->obj_add_flag(self->bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    self->ui->obj_add_flag(self->enemy_bullet, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    return GM_PLUGIN_OK;
}

gm_plugin_result_t create_ui(invaders_t *self)
{
    gm_plugin_lvgl_obj_t *host_root = self->ui->root_get();
    gm_plugin_lvgl_obj_t *ground;
    uint8_t row;
    uint8_t col;
    gm_plugin_result_t result;
    if (host_root == 0) return GM_PLUGIN_ESTATE;
    self->ui->obj_clean(host_root);
    self->root = self->ui->obj_create(host_root);
    if (self->root == 0) return GM_PLUGIN_ENOMEM;
    self->ui->obj_set_size(self->root, self->screen_width, self->screen_height);
    style_box(self, self->root, 255, 0, 0);
    self->ui->style_set(self->root, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                        color(0), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->board = self->ui->obj_create(self->root);
    self->score_label = self->ui->label_create(self->root);
    self->status_label = self->ui->label_create(self->root);
    if (self->board == 0 || self->score_label == 0 ||
        self->status_label == 0) return GM_PLUGIN_ENOMEM;
    self->ui->obj_set_pos(self->board, BOARD_X, BOARD_Y);
    self->ui->obj_set_size(self->board, self->board_width, self->board_height);
    style_box(self, self->board, GM_PLUGIN_LVGL_OPA_COVER, 2, 3);
    self->ui->style_set(self->board, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                        color(0), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->board, GM_PLUGIN_LVGL_STYLE_OUTLINE_COLOR,
                        color(0x40U), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->board, GM_PLUGIN_LVGL_STYLE_OUTLINE_WIDTH,
                        number(4), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->board, GM_PLUGIN_LVGL_STYLE_OUTLINE_OPA,
                        number(GM_PLUGIN_LVGL_OPA_50),
                        GM_PLUGIN_LVGL_SELECTOR_MAIN);

    ground = self->ui->obj_create(self->board);
    if (ground == 0) return GM_PLUGIN_ENOMEM;
    self->ui->obj_set_pos(ground, 0, self->board_height - 6);
    self->ui->obj_set_size(ground, self->board_width, 1);
    style_box(self, ground, 90, 0, 0);
    self->ui->style_set(ground, GM_PLUGIN_LVGL_STYLE_BG_COLOR,
                        color(0x50U), GM_PLUGIN_LVGL_SELECTOR_MAIN);

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            result = create_alien(self, row, col);
            if (result != GM_PLUGIN_OK) return result;
        }
    }
    result = create_player(self);
    if (result != GM_PLUGIN_OK) return result;

    self->ui->obj_set_pos(self->score_label, BOARD_X + 4, 6);
    self->ui->obj_set_size(self->score_label, HEADER_SIDE_W, HEADER_H);
    self->ui->label_set_long_mode(self->score_label, GM_PLUGIN_LVGL_LABEL_CLIP);
    self->ui->style_set(self->score_label, GM_PLUGIN_LVGL_STYLE_TEXT_COLOR,
                        color(0xF0U), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->score_label, GM_PLUGIN_LVGL_STYLE_TEXT_OPA,
                        number(GM_PLUGIN_LVGL_OPA_COVER),
                        GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->obj_set_size(self->status_label, HEADER_SIDE_W, HEADER_H);
    self->ui->obj_align(self->status_label, GM_PLUGIN_LVGL_ALIGN_TOP_RIGHT,
                        -12, 6);
    self->ui->label_set_long_mode(self->status_label, GM_PLUGIN_LVGL_LABEL_CLIP);
    self->ui->style_set(self->status_label, GM_PLUGIN_LVGL_STYLE_TEXT_ALIGN,
                        number(GM_PLUGIN_LVGL_TEXT_ALIGN_RIGHT),
                        GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->status_label, GM_PLUGIN_LVGL_STYLE_TEXT_COLOR,
                        color(0xFFU), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->status_label, GM_PLUGIN_LVGL_STYLE_TEXT_OPA,
                        number(GM_PLUGIN_LVGL_OPA_COVER),
                        GM_PLUGIN_LVGL_SELECTOR_MAIN);

    self->message_label = self->ui->label_create(self->root);
    if (self->message_label == 0) return GM_PLUGIN_ENOMEM;
    style_panel(self, self->message_label);
    self->ui->label_set_long_mode(self->message_label,
                                  GM_PLUGIN_LVGL_LABEL_WRAP);
    set_message(self, "", 0U);

    self->exit_arc = self->ui->arc_create(self->root);
    if (self->exit_arc == 0) return GM_PLUGIN_ENOMEM;
    self->ui->obj_set_size(self->exit_arc, 48, 48);
    self->ui->obj_align(self->exit_arc, GM_PLUGIN_LVGL_ALIGN_CENTER, 0, 0);
    self->ui->arc_set_range(self->exit_arc, 0, BUTTON_EXIT_MS);
    self->ui->arc_set_value(self->exit_arc, 0);
    self->ui->style_set(self->exit_arc, GM_PLUGIN_LVGL_STYLE_ARC_WIDTH,
                        number(4), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->exit_arc, GM_PLUGIN_LVGL_STYLE_ARC_COLOR,
                        color(0x20), GM_PLUGIN_LVGL_SELECTOR_MAIN);
    self->ui->style_set(self->exit_arc, GM_PLUGIN_LVGL_STYLE_ARC_WIDTH,
                        number(4), GM_PLUGIN_LVGL_SELECTOR_INDICATOR);
    self->ui->style_set(self->exit_arc, GM_PLUGIN_LVGL_STYLE_ARC_COLOR,
                        color(0xF0), GM_PLUGIN_LVGL_SELECTOR_INDICATOR);
    self->ui->style_set(self->exit_arc, GM_PLUGIN_LVGL_STYLE_OPA,
                        number(0), GM_PLUGIN_LVGL_SELECTOR_KNOB);
    self->ui->obj_add_flag(self->exit_arc, GM_PLUGIN_LVGL_FLAG_HIDDEN);

    place_formation(self);
    layout_ship(self);
    refresh_header(self);
    return GM_PLUGIN_OK;
}
