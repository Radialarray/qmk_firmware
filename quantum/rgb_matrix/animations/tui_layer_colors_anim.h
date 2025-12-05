#ifdef RGB_MATRIX_ENABLE

// TUI-driven, layer-aware base colors effect.
//
// Uses the baked per-layer, per-LED RGB table emitted by
// keyboard_tui into the generated keymap.c:
//   const uint8_t PROGMEM layer_base_colors[NUM_LAYERS][RGB_MATRIX_LED_COUNT][3];
//
// For each LED, we:
//   1. Map LED index -> key row/col using g_led_config.matrix_co.
//   2. Use layer_switch_get_layer(key) to find the effective layer
//      according to QMK's normal layer resolution rules
//      (MO, LT, tri-layer, etc.).
//   3. Read the RGB triplet for [layer][led] from PROGMEM and set
//      that color for the LED.
//
#define RGB_MATRIX_EFFECT_TUI_LAYER_COLORS
RGB_MATRIX_EFFECT(TUI_LAYER_COLORS)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

// Forward declaration; implemented in action_layer.c
uint8_t layer_switch_get_layer(keypos_t key);

extern const uint8_t PROGMEM layer_base_colors[][RGB_MATRIX_LED_COUNT][3];
extern const uint8_t PROGMEM layer_base_colors_layer_count;

// Cache LED -> key matrix mapping so we only scan g_led_config once.
static bool     tui_layer_colors_mapping_initialized = false;
static keypos_t tui_layer_colors_led_to_key[RGB_MATRIX_LED_COUNT];

static void tui_layer_colors_init_mapping(void) {
    if (tui_layer_colors_mapping_initialized) {
        return;
    }

    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; led++) {
        keypos_t key = {0};
        bool     found = false;

        for (uint8_t row = 0; row < MATRIX_ROWS && !found; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                if (g_led_config.matrix_co[row][col] == led) {
                    key.row = row;
                    key.col = col;
                    found   = true;
                    break;
                }
            }
        }

        if (found) {
            tui_layer_colors_led_to_key[led] = key;
        } else {
            // Mark as unmapped: use 0xFF as sentinel.
            tui_layer_colors_led_to_key[led].row = 0xFF;
            tui_layer_colors_led_to_key[led].col = 0xFF;
        }
    }

    tui_layer_colors_mapping_initialized = true;
}

static uint8_t tui_layer_colors_layer_count(void) {
#ifdef LAYER_BASE_COLORS_LAYER_COUNT
    return LAYER_BASE_COLORS_LAYER_COUNT;
#else
    return 0;
#endif
}

bool TUI_LAYER_COLORS(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    tui_layer_colors_init_mapping();

    const uint8_t layer_count = tui_layer_colors_layer_count();
    if (layer_count == 0) {
        // No color data compiled in; nothing to draw.
        return rgb_matrix_check_finished_leds(led_max);
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();

        keypos_t key = tui_layer_colors_led_to_key[i];

        uint8_t layer_index = 0;
        if (key.row != 0xFF && key.col != 0xFF) {
            layer_index = layer_switch_get_layer(key);
        }

        if (layer_index >= layer_count) {
            layer_index = (uint8_t)(layer_count - 1);
        }

        uint8_t r = pgm_read_byte(&layer_base_colors[layer_index][i][0]);
        uint8_t g = pgm_read_byte(&layer_base_colors[layer_index][i][1]);
        uint8_t b = pgm_read_byte(&layer_base_colors[layer_index][i][2]);

        rgb_matrix_set_color(i, r, g, b);
    }

    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#endif // RGB_MATRIX_ENABLE
