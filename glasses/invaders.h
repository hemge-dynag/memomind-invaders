#ifndef INVADERS_H
#define INVADERS_H

#include "call_ui.h"
#include "gm_plugin_lvgl_api.h"
#include "gm_plugin_libc.h"
#include "gm_plugin_input.h"

#define CONFIG_CHANNEL UINT16_C(0x4943) /* 'IC': phone -> glasses */
#define EVENT_CHANNEL UINT16_C(0x4945)  /* 'IE': glasses -> phone */
#define PROTOCOL_VERSION 1U
#define COMMAND_CONFIG 1U
#define COMMAND_NEW_GAME 2U
#define CONFIG_PAYLOAD 5U
#define EVENT_STATUS 1U
#define STATUS_INTERVAL_MS 400U

#define STATE_IDLE 0U
#define STATE_PLAYING 1U
#define STATE_PAUSED 2U
#define STATE_OVER 3U

#define FIRE_MODE_AUTO 0U
#define FIRE_MODE_BUTTON 1U

#define COLS 7
#define ROWS 3
#define FRAME_MS 40U
#define Q 256
#define BOARD_X 10
#define BOARD_Y 36
#define BOARD_BOTTOM_MARGIN 12
#define MIN_BOARD_W 120
#define MIN_BOARD_H 120
#define HEADER_H 22
#define HEADER_SIDE_W 150

#define SHIP_W 28
#define SHIP_H 14
#define GYRO_THRESHOLD 15

#define ALIEN_W 34
#define ALIEN_H 16
#define ALIEN_GAP_X 6
#define ALIEN_GAP_Y 6
#define ALIEN_TOP 12
#define ALIEN_START_X 3
#define ALIEN_STEP_Y 6
#define ALIEN_STEP_MS 620U
#define ALIEN_CYCLE_MS 9000U
#define MAX_FIRE_RATE 10U
#define MAX_SENSITIVITY 10U

#define BULLET_W 3
#define BULLET_H 10
#define SHOT_SPEED 15
#define ENEMY_SHOT_W 3
#define ENEMY_SHOT_H 9
#define ENEMY_SHOT_SPEED 5
#define ENEMY_SHOT_INTERVAL_MS 1200U

#define START_LIVES 3
#define EXIT_PITCH 30
#define EXIT_MS 3000U
#define BUTTON_EXIT_MS 2000U
#define MESSAGE_SIDE_MARGIN 36
#define MESSAGE_MIN_W 320
#define MESSAGE_MIN_H 110

typedef struct {
    const gm_plugin_host_api_t *host;
    const gm_plugin_lvgl_api_t *ui;
    const gm_plugin_libc_extension_api_t *libc;
    gm_plugin_lvgl_obj_t *root;
    gm_plugin_lvgl_obj_t *board;
    gm_plugin_lvgl_obj_t *aliens[ROWS][COLS];
    gm_plugin_lvgl_obj_t *ship;
    gm_plugin_lvgl_obj_t *ship_body;
    gm_plugin_lvgl_obj_t *bullet;
    gm_plugin_lvgl_obj_t *enemy_bullet;
    gm_plugin_lvgl_obj_t *score_label;
    gm_plugin_lvgl_obj_t *status_label;
    gm_plugin_lvgl_obj_t *message_label;
    gm_plugin_lvgl_obj_t *exit_arc;

    int32_t ship_x;
    int32_t ship_vx;
    int32_t bullet_y;
    int32_t enemy_bullet_y;
    int32_t enemy_bullet_x;
    int32_t alien_offset;

    uint32_t frame_accumulator;
    uint32_t fire_accumulator;
    uint32_t enemy_fire_accumulator;
    uint32_t march_accumulator;
    uint32_t descent_accumulator;
    uint32_t status_accumulator;
    uint32_t exit_start_ms;
    uint32_t button_hold_ms;
    uint32_t resume_ms;

    int16_t screen_width;
    int16_t screen_height;
    int16_t board_width;
    int16_t board_height;
    int16_t ship_y;
    int16_t alien_origin_x;
    int16_t alien_origin_y;

    uint16_t score;
    uint8_t wave;
    uint8_t lives;
    uint8_t alive[ROWS][COLS];
    uint8_t aliens_remaining;
    uint8_t alien_direction;
    uint8_t enemy_cursor;
    uint8_t bullet_active;
    uint8_t enemy_bullet_active;
    uint8_t state;
    uint8_t sensitivity;
    uint8_t fire_mode;
    uint8_t fire_rate;
    uint8_t exiting;
    uint8_t exit_seconds;
    uint8_t button_holding_exit;
    uint8_t language;
} invaders_t;

#endif
