#include "ui.h"

#include "screens/screen_loading.h"
#include "screens/screen_main.h"
#include "screens/screen_mode.h"
#include "screens/screen_select.h"

static lv_obj_t *screens[5];

void ui_init(void)
{
    screens[UI_SCREEN_LOADING] = screen_loading_create();
    screens[UI_SCREEN_MAIN] = screen_main_create();
    screens[UI_SCREEN_SELECT] = screen_select_create();

    // cung tro cung 1 screen
    screens[UI_SCREEN_PERSONAL] = screen_mode_create();
    screens[UI_SCREEN_TEAM] = screens[UI_SCREEN_PERSONAL];

    screen_mode_set_mode(SCREEN_MODE_PERSONAL);
    lv_scr_load(screens[UI_SCREEN_LOADING]);
}

void ui_switch_screen(ui_screen_t screen)
{
    if (screen == UI_SCREEN_PERSONAL)
        screen_mode_set_mode(SCREEN_MODE_PERSONAL);
    else if (screen == UI_SCREEN_TEAM)
        screen_mode_set_mode(SCREEN_MODE_TEAM);

    lv_scr_load(screens[screen]);
}