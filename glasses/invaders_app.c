#include "invaders_internal.h"

static example_call_ui_t s_call_ui;

static void handle_config(const uint8_t *data, uint32_t length)
{
    if (length < CONFIG_PAYLOAD || data[0] != PROTOCOL_VERSION) return;
    if (data[1] == COMMAND_CONFIG) {
        iv.sensitivity = (uint8_t)clamp_i32((int32_t)data[2], 1,
                                            (int32_t)MAX_SENSITIVITY);
        iv.fire_mode = (uint8_t)(data[3] != 0U ? FIRE_MODE_BUTTON :
                                 FIRE_MODE_AUTO);
        iv.fire_rate = (uint8_t)clamp_i32((int32_t)data[4], 1,
                                          (int32_t)MAX_FIRE_RATE);
        send_status(&iv);
    } else if (data[1] == COMMAND_NEW_GAME) {
        new_game(&iv);
    }
}

static gm_plugin_result_t on_start(void *opaque)
{
    invaders_t *self = opaque;
    gm_plugin_result_t result;
    gm_plugin_display_info_t display;
    char language_tag[GM_PLUGIN_LOCALE_TAG_MAX] = {0};
    result = example_call_ui_start(&s_call_ui);
    if (result != GM_PLUGIN_OK) return result;
    if (self->host->display_get_info(&display) != GM_PLUGIN_OK ||
        display.width <= BOARD_X * 2U ||
        display.height <= BOARD_Y + BOARD_BOTTOM_MARGIN)
        return GM_PLUGIN_ENOTSUP;
    self->screen_width = (int16_t)display.width;
    self->screen_height = (int16_t)display.height;
    self->board_width = (int16_t)(self->screen_width - BOARD_X * 2);
    self->board_height = (int16_t)(self->screen_height - BOARD_Y -
                                   BOARD_BOTTOM_MARGIN);
    if (self->board_width < MIN_BOARD_W ||
        self->board_height < MIN_BOARD_H) return GM_PLUGIN_ENOTSUP;
    self->ship_y = (int16_t)(self->board_height - 22);
    if (self->host->locale_get(language_tag) != GM_PLUGIN_OK)
        language_tag[0] = '\0';
    self->language = detect_language(language_tag);
    if (self->host->imu_enable(GM_PLUGIN_IMU_ENABLE_RAW) != GM_PLUGIN_OK)
        return GM_PLUGIN_ESTATE;
    self->state = STATE_IDLE;
    result = create_ui(self);
    if (result != GM_PLUGIN_OK) {
        (void)self->host->imu_enable(GM_PLUGIN_IMU_ENABLE_NONE);
        self->ui->obj_clean(self->ui->root_get());
        return result;
    }
    send_status(self);
    return GM_PLUGIN_OK;
}

static void on_loop(void *opaque, uint32_t elapsed_ms)
{
    invaders_t *self = opaque;
    gm_plugin_imu_sample_t imu;
    uint32_t now = self->host->monotonic_ms();
    uint32_t chunk;
    if (elapsed_ms > 150U) elapsed_ms = 150U;
    self->status_accumulator += elapsed_ms;
    if (self->button_holding_exit != 0U) {
        self->button_hold_ms += elapsed_ms;
        if (self->button_hold_ms >= BUTTON_EXIT_MS) {
            self->host->app_exit();
            return;
        }
        self->ui->arc_set_value(self->exit_arc,
                                (int16_t)self->button_hold_ms);
    }
    if (self->host->imu_read(&imu) != GM_PLUGIN_OK) return;
    if (imu.pitch_degrees > EXIT_PITCH) {
        if (self->exiting == 0U) {
            self->exiting = 1U;
            self->exit_start_ms = now;
            self->exit_seconds = 3U;
            set_message(self, L("Tu vas quitter...",
                                "Exiting...",
                                "Saliendo...",
                                "Uscita...",
                                "Beende...",
                                "\xe6\xad\xa3\xe5\x9c\xa8\xe9\x80\x80\xe5\x87\xba..."),
                        1U);
        } else {
            uint32_t elapsed = now - self->exit_start_ms;
            if (elapsed >= EXIT_MS) {
                self->exiting = 2U;
                self->host->app_exit();
                return;
            }
            {
                uint8_t seconds = (uint8_t)((EXIT_MS - elapsed + 999U) / 1000U);
                if (seconds != self->exit_seconds) {
                    char text[64];
                    self->exit_seconds = seconds;
                    self->libc->snprintf(text, sizeof(text), "%s%u",
                        L("Tu vas quitter... ",
                          "Exiting... ",
                          "Saliendo... ",
                          "Uscita... ",
                          "Beende... ",
                          "\xe9\x80\x80\xe5\x87\xba\xef\xbc\x9a"),
                        (unsigned int)seconds);
                    set_message(self, text, 1U);
                }
            }
        }
    } else if (self->exiting != 0U) {
        self->exiting = 0U;
        self->exit_seconds = 0U;
        if (self->state == STATE_PLAYING) set_message(self, "", 0U);
    }
    if (self->state == STATE_PLAYING) move_ship(self, &imu);
    self->frame_accumulator += elapsed_ms;
    if (self->frame_accumulator > 200U) self->frame_accumulator = 200U;
    while (self->frame_accumulator >= FRAME_MS) {
        self->frame_accumulator -= FRAME_MS;
        chunk = FRAME_MS;
        step_frame(self, chunk, now);
    }
    if (self->status_accumulator >= STATUS_INTERVAL_MS) send_status(self);
}

static bool on_event(void *opaque, const gm_plugin_event_t *event)
{
    invaders_t *self = opaque;
    gm_plugin_button_action_t action;
    gm_plugin_button_t button;
    if (event == 0) return false;
    if (event->type == GM_PLUGIN_EVENT_CONNECTION) {
        if (event->data.connection.connected) send_status(self);
        return true;
    }
    if (event->type == GM_PLUGIN_EVENT_BT_MESSAGE) {
        if (event->data.bt.channel != CONFIG_CHANNEL ||
            event->data.bt.data == 0) return false;
        handle_config(event->data.bt.data, event->data.bt.length);
        return true;
    }
    if (event->type != GM_PLUGIN_EVENT_BUTTON) return false;
    action = event->data.button.action;
    button = event->data.button.button;
    if (action == GM_PLUGIN_BUTTON_ACTION_LONG ||
        action == GM_PLUGIN_BUTTON_ACTION_VERY_LONG) {
        if (self->button_holding_exit == 0U) {
            self->button_holding_exit = 1U;
            self->button_hold_ms = 0U;
            self->ui->arc_set_value(self->exit_arc, 0);
            self->ui->obj_clear_flag(self->exit_arc,
                                     GM_PLUGIN_LVGL_FLAG_HIDDEN);
        }
        return true;
    }
    if (action == GM_PLUGIN_BUTTON_ACTION_RELEASE) {
        self->button_holding_exit = 0U;
        self->button_hold_ms = 0U;
        self->ui->obj_add_flag(self->exit_arc, GM_PLUGIN_LVGL_FLAG_HIDDEN);
        return true;
    }
    if (action != GM_PLUGIN_BUTTON_ACTION_SINGLE) return false;
    if (button == GM_PLUGIN_BUTTON_PRIMARY) {
        if (self->state == STATE_PLAYING) {
            if (self->fire_mode == FIRE_MODE_BUTTON) start_shot(self);
            else {
                self->state = STATE_PAUSED;
                set_message(self, L("Pause", "Pause", "Pausa", "Pausa",
                                    "Pause", "\xe6\x9a\x82\xe5\x81\x9c"), 1U);
                refresh_header(self);
                send_status(self);
            }
        } else if (self->state == STATE_PAUSED) {
            self->state = STATE_PLAYING;
            self->resume_ms = self->host->monotonic_ms();
            self->frame_accumulator = 0U;
            set_message(self, "", 0U);
            refresh_header(self);
            send_status(self);
        } else if (self->state == STATE_OVER) {
            new_game(self);
        } else {
            new_game(self);
        }
        return true;
    }
    if (button == GM_PLUGIN_BUTTON_BACK) {
        self->exiting = 2U;
        self->host->app_exit();
        return true;
    }
    return false;
}

static void on_suspend(void *opaque)
{
    invaders_t *self = opaque;
    self->button_holding_exit = 0U;
    self->button_hold_ms = 0U;
    self->ui->obj_add_flag(self->exit_arc, GM_PLUGIN_LVGL_FLAG_HIDDEN);
    (void)self->host->imu_enable(GM_PLUGIN_IMU_ENABLE_NONE);
}

static void on_resume(void *opaque)
{
    invaders_t *self = opaque;
    (void)self->host->imu_enable(GM_PLUGIN_IMU_ENABLE_RAW);
    self->frame_accumulator = 0U;
    self->resume_ms = self->host->monotonic_ms();
}

static void on_stop(void *opaque)
{
    invaders_t *self = opaque;
    example_call_ui_stop(&s_call_ui);
    (void)self->host->imu_enable(GM_PLUGIN_IMU_ENABLE_NONE);
    self->ui->obj_clean(self->ui->root_get());
}

gm_plugin_result_t gm_plugin_entry(const gm_plugin_host_api_t *host,
                                   gm_plugin_descriptor_t *plugin)
{
    const gm_plugin_capabilities_t required =
        GM_PLUGIN_CAP_BUTTON | GM_PLUGIN_CAP_IMU_RAW |
        GM_PLUGIN_CAP_LOCALE | GM_PLUGIN_CAP_BLUETOOTH;
    if (host == 0 || plugin == 0 ||
        host->struct_size < GM_PLUGIN_HOST_API_MIN_SIZE ||
        !GM_PLUGIN_VERSION_COMPATIBLE(host->abi_version,
                                      GM_PLUGIN_ABI_MIN_VERSION) ||
        host->graphics.lvgl == 0 || host->display_get_info == 0 ||
        host->monotonic_ms == 0 || host->imu_enable == 0 ||
        host->imu_read == 0 || host->app_exit == 0 || host->locale_get == 0 ||
        host->bt_send == 0 ||
        (host->capabilities & required) != required ||
        plugin->struct_size < GM_PLUGIN_DESCRIPTOR_MIN_SIZE)
        return GM_PLUGIN_EVERSION;
    iv.host = host;
    iv.ui = host->graphics.lvgl;
    if (gm_plugin_libc_get(host, &iv.libc) != GM_PLUGIN_OK)
        return GM_PLUGIN_ENOTSUP;
    if (iv.ui == 0 ||
        iv.ui->struct_size < GM_PLUGIN_LVGL_API_MIN_SIZE ||
        !GM_PLUGIN_VERSION_COMPATIBLE(iv.ui->api_version,
                                      GM_PLUGIN_LVGL_API_MIN_VERSION))
        return GM_PLUGIN_EVERSION;
    iv.sensitivity = 5U;
    iv.fire_mode = FIRE_MODE_AUTO;
    iv.fire_rate = 4U;
    iv.language = 0U;
    iv.exiting = 0U;
    plugin->abi_version = GM_PLUGIN_ABI_MIN_VERSION;
    plugin->context = &iv;
    s_call_ui.host = host;
    plugin->on_start = on_start;
    plugin->on_resume = on_resume;
    plugin->on_loop = on_loop;
    plugin->on_event = on_event;
    plugin->on_suspend = on_suspend;
    plugin->on_stop = on_stop;
    return GM_PLUGIN_OK;
}
