#include "hmi_buttons.h"
#include "hmi_events.h"

#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUTTONS";

typedef struct {
    buttons_config_t cfg;
    QueueHandle_t out_queue;

    // debounce interno
    int stable_level_1, last_level_1; TickType_t last_change_1;
    int stable_level_2, last_level_2; TickType_t last_change_2;
    int stable_level_3, last_level_3; TickType_t last_change_3;
} buttons_ctx_t;

static void gpio_buttons_init(const buttons_config_t *cfg)
{
    gpio_config_t io = {0};
    io.pin_bit_mask =
        (1ULL << cfg->btn1_gpio) |
        (1ULL << cfg->btn2_gpio) |
        (1ULL << cfg->btn3_gpio);
    io.mode = GPIO_MODE_INPUT;
    io.intr_type = GPIO_INTR_DISABLE;

    if (cfg->active_low) {
        io.pull_up_en = GPIO_PULLUP_ENABLE;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else {
        io.pull_up_en = GPIO_PULLUP_DISABLE;
        io.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }

    gpio_config(&io);
}

static inline void process_btn(int raw, int *last_level, int *stable_level, TickType_t *last_change, TickType_t now,
                               TickType_t debounce_ticks,bool active_low,QueueHandle_t q, hmi_event_t evt)
{
    if (raw != *last_level) {
        *last_level = raw;
        *last_change = now;
    }

    if ((now - *last_change) >= debounce_ticks && (*stable_level != *last_level)) {
        *stable_level = *last_level;

        // Detectamos “pressed” cuando llega al nivel de presionado estable
        const int pressed_level = active_low ? 0 : 1;

        if (*stable_level == pressed_level) {
            (void)xQueueSend(q, &evt, 0);
        }
    }
}

static void buttons_task(void *arg)
{
    buttons_ctx_t *ctx = (buttons_ctx_t *)arg;

    //pdMS_TO_TICKS - devuelve el tiempo expresado en ticks.
    const TickType_t poll_ticks = pdMS_TO_TICKS(ctx->cfg.poll_ms);
    const TickType_t debounce_ticks = pdMS_TO_TICKS(ctx->cfg.debounce_ms);

    // niveles por defecto
    const int idle_level = ctx->cfg.active_low ? 1 : 0;

    ctx->stable_level_1 = idle_level; ctx->last_level_1 = idle_level; ctx->last_change_1 = 0;
    ctx->stable_level_2 = idle_level; ctx->last_level_2 = idle_level; ctx->last_change_2 = 0;
    ctx->stable_level_3 = idle_level; ctx->last_level_3 = idle_level; ctx->last_change_3 = 0;

    ESP_LOGI(TAG, "Task started (poll=%dms, debounce=%dms, active_low=%d)",
             ctx->cfg.poll_ms, ctx->cfg.debounce_ms, (int)ctx->cfg.active_low);

    while (1) {
        vTaskDelay(poll_ticks);

        const int r1 = gpio_get_level(ctx->cfg.btn1_gpio);
        const int r2 = gpio_get_level(ctx->cfg.btn2_gpio);
        const int r3 = gpio_get_level(ctx->cfg.btn3_gpio);

        const TickType_t now = xTaskGetTickCount();

        process_btn(r1, &ctx->last_level_1, &ctx->stable_level_1, &ctx->last_change_1,
                    now, debounce_ticks, ctx->cfg.active_low, ctx->out_queue, HMI_EVT_BTN1_SHORT);

        process_btn(r2, &ctx->last_level_2, &ctx->stable_level_2, &ctx->last_change_2,
                    now, debounce_ticks, ctx->cfg.active_low, ctx->out_queue, HMI_EVT_BTN2_SHORT);

        process_btn(r3, &ctx->last_level_3, &ctx->stable_level_3, &ctx->last_change_3,
                    now, debounce_ticks, ctx->cfg.active_low, ctx->out_queue, HMI_EVT_BTN3_SHORT);
    }
}

void buttons_start(const buttons_config_t *cfg, QueueHandle_t out_queue)
{
    if (!cfg || !out_queue) return;

    gpio_buttons_init(cfg);

    static buttons_ctx_t ctx; // estático para evitar malloc (simple y robusto)
    ctx.cfg = *cfg;
    ctx.out_queue = out_queue;

    if (ctx.cfg.task_core == 0 || ctx.cfg.task_core == 1) {
        xTaskCreatePinnedToCore(buttons_task,"buttons_task",
                                ctx.cfg.task_stack,&ctx,
                                ctx.cfg.task_priority,NULL,
                                ctx.cfg.task_core);
    } else {
        xTaskCreate(buttons_task, "buttons_task",
                    ctx.cfg.task_stack, &ctx,
                    ctx.cfg.task_priority, NULL);
    }
}
