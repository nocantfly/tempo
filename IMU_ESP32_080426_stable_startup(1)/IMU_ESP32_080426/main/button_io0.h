#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*button_click_cb_t)(void);

/**
 * @brief Khoi tao task doc nut IO0
 * @param cb ham se duoc goi khi phat hien click
 */
void button_io0_start(button_click_cb_t cb);

#ifdef __cplusplus
}
#endif