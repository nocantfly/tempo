ui/
│
├── ui.h
├── ui.c                // init + navigator + state
│
├── components/
│   ├── ui_header.c
│   ├── ui_header.h
│
├── screens/
│   ├── screen_loading.c
│   ├── screen_loading.h
│
│   ├── screen_main.c
│   ├── screen_main.h
│
│   ├── screen_personal.c
│   ├── screen_personal.h
│
│   ├── screen_team.c
│   ├── screen_team.h


typedef enum {
    UI_SCREEN_LOADING,
    UI_SCREEN_MAIN,
    UI_SCREEN_PERSONAL,
    UI_SCREEN_TEAM
} ui_screen_t;