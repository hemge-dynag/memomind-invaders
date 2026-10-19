#ifndef INVADERS_INTERNAL_H
#define INVADERS_INTERNAL_H

#include "invaders.h"

extern invaders_t iv;

#define number gm_plugin_lvgl_style_number
#define color gm_plugin_lvgl_style_color

#define L(fr_str, en_str, es_str, it_str, de_str, zh_str) \
    (iv.language == 1U ? (en_str) : iv.language == 2U ? (es_str) : \
     iv.language == 3U ? (it_str) : iv.language == 4U ? (de_str) : \
     iv.language == 5U ? (zh_str) : (fr_str))

uint8_t detect_language(const char *locale);
int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum);
void style_box(invaders_t *self, gm_plugin_lvgl_obj_t *object,
               uint8_t opacity, uint8_t border, uint8_t radius);
void style_panel(invaders_t *self, gm_plugin_lvgl_obj_t *label);
void refresh_header(invaders_t *self);
void set_message(invaders_t *self, const char *text, uint8_t visible);
void send_status(invaders_t *self);
void place_formation(invaders_t *self);
void build_formation(invaders_t *self);
void reset_shot(invaders_t *self);
void layout_ship(invaders_t *self);
void new_game(invaders_t *self);
void move_ship(invaders_t *self, const gm_plugin_imu_sample_t *imu);
void start_shot(invaders_t *self);
void step_frame(invaders_t *self, uint32_t elapsed_ms, uint32_t now);
gm_plugin_result_t create_ui(invaders_t *self);

#endif
