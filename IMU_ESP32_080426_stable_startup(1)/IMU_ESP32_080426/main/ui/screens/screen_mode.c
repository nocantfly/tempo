#include "screen_mode.h"
#include "ui.h"
#include "ui_header.h"
#include "application.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"

#define SCREEN_WIDTH 410
#define SCREEN_HEIGHT 502
#define SAFE_PADDING 30

#define MATCH_THRESHOLD_ENTER_PERCENT 10.0f
#define MATCH_THRESHOLD_EXIT_PERCENT 12.0f
#define FLOAT_EPSILON 0.0001f

static const char *TAG = "SCREEN_MODE";

typedef enum
{
    SAVE_TARGET_NONE = 0,
    SAVE_TARGET_BEFORE,
    SAVE_TARGET_AFTER
} save_target_t;

static lv_obj_t *screen_root;
static lv_obj_t *card;
static lv_obj_t *panel_grid;

static lv_obj_t *label_title;
static lv_obj_t *label_status;

static lv_obj_t *label_x_value;
static lv_obj_t *label_y_value;
static lv_obj_t *label_z_value;
static lv_obj_t *label_d_value;

static lv_obj_t *label_x_sub;
static lv_obj_t *label_y_sub;
static lv_obj_t *label_z_sub;
static lv_obj_t *label_d_sub;

static lv_obj_t *cell_x;
static lv_obj_t *cell_y;
static lv_obj_t *cell_z;
static lv_obj_t *cell_d;

// popup
static lv_obj_t *popup_overlay = NULL;
static lv_obj_t *popup_box = NULL;

// current live data
static float s_current_roll = 0.0f;
static float s_current_pitch = 0.0f;
static float s_current_yaw = 0.0f;
static float s_current_distance = 0.0f;

static screen_mode_type_t s_mode = SCREEN_MODE_PERSONAL;
static save_target_t s_pending_target = SAVE_TARGET_NONE;
static screen_mode_save_cb_t s_save_cb = NULL;

// cache state to avoid redraw/flicker
static bool s_theme_is_ok = false;
static bool s_theme_initialized = false;
static char s_last_status[128] = {0};

// ===== Back =====
static void on_back(lv_event_t *e)
{
    LV_UNUSED(e);

    if (popup_overlay)
    {
        ESP_LOGW(TAG, "Ignore back because popup is open");
        return;
    }

    ESP_LOGI(TAG, "Back button clicked");
    ui_switch_screen(UI_SCREEN_MAIN);
}

// ===== Style helpers =====
static void style_card_base(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1A2318), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x244020), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);

    lv_obj_set_style_radius(obj, 24, 0);
    lv_obj_set_style_border_width(obj, 7, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x39FF5A), 0);

    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x39FF5A), 0);
    lv_obj_set_style_shadow_width(obj, 46, 0);
    lv_obj_set_style_shadow_spread(obj, 12, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_80, 0);

    lv_obj_set_style_pad_all(obj, 6, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_cell_base(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1A2318), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x243020), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(obj, 3, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x567050), 0);
    lv_obj_set_style_radius(obj, 14, 0);

    lv_obj_set_style_shadow_width(obj, 18, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_40, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x567050), 0);

    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_popup_box(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x39FF5A), 0);
    lv_obj_set_style_radius(obj, 20, 0);
    lv_obj_set_style_pad_all(obj, 16, 0);

    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x39FF5A), 0);
    lv_obj_set_style_shadow_width(obj, 24, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_40, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_popup_btn(lv_obj_t *btn, lv_color_t bg)
{
    lv_obj_set_style_radius(btn, 18, 0);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *create_popup_button(lv_obj_t *parent,
                                     const char *text,
                                     lv_event_cb_t cb,
                                     void *user_data,
                                     lv_color_t bg,
                                     lv_coord_t w,
                                     lv_coord_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    style_popup_btn(btn, bg);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(lbl);

    return btn;
}

static lv_obj_t *create_imu_cell(lv_obj_t *parent,
                                 const char *title,
                                 const char *value,
                                 const char *sub,
                                 lv_obj_t **out_value_label,
                                 lv_obj_t **out_sub_label)
{
    lv_obj_t *cell = lv_obj_create(parent);
    style_cell_base(cell);

    lv_obj_t *lbl_title = lv_label_create(cell);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xE7D07A), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *lbl_value = lv_label_create(cell);
    lv_label_set_text(lbl_value, value);
    lv_obj_set_style_text_color(lbl_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_value, &lv_font_montserrat_40, 0);
    lv_obj_align(lbl_value, LV_ALIGN_CENTER, 0, 4);

    lv_obj_t *lbl_sub = lv_label_create(cell);
    lv_label_set_text(lbl_sub, sub);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0xFFD54F), 0);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_22, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_BOTTOM_MID, 0, -8);

    if (out_value_label)
        *out_value_label = lbl_value;

    if (out_sub_label)
        *out_sub_label = lbl_sub;

    return cell;
}

static void screen_mode_set_status_if_changed(const char *text)
{
    if (!label_status || !text)
        return;

    if (strncmp(s_last_status, text, sizeof(s_last_status) - 1) == 0)
        return;

    strncpy(s_last_status, text, sizeof(s_last_status) - 1);
    s_last_status[sizeof(s_last_status) - 1] = '\0';
    lv_label_set_text(label_status, s_last_status);
}

static float percent_error(float current, float ref)
{
    float base = fabsf(ref);
    if (base < FLOAT_EPSILON)
        base = FLOAT_EPSILON;

    return (fabsf(current - ref) * 100.0f) / base;
}
static void log_pose_compare_detail(const char *name,
                                    const app_pose_t *current,
                                    const app_pose_t *ref)
{
    if (!current || !ref)
        return;

    ESP_LOGI(TAG,
             "[%s] cur(r=%.2f p=%.2f y=%.2f d=%.2f) "
             "save(r=%.2f p=%.2f y=%.2f d=%.2f) "
             "err%%(r=%.2f p=%.2f y=%.2f d=%.2f)",
             name,
             current->roll, current->pitch, current->yaw, current->distance_m,
             ref->roll, ref->pitch, ref->yaw, ref->distance_m,
             percent_error(current->roll, ref->roll),
             percent_error(current->pitch, ref->pitch),
             percent_error(current->yaw, ref->yaw),
             percent_error(current->distance_m, ref->distance_m));
}
static bool is_value_match_with_hysteresis(float current, float ref, bool currently_ok)
{
    float err = percent_error(current, ref);

    if (currently_ok)
        return (err <= MATCH_THRESHOLD_EXIT_PERCENT);

    return (err < MATCH_THRESHOLD_ENTER_PERCENT);
}

static bool pose_match_with_hysteresis(const app_pose_t *cur, const app_pose_t *ref, bool currently_ok)
{
    if (!cur || !ref)
        return false;

    bool ok_roll = is_value_match_with_hysteresis(cur->roll, ref->roll, currently_ok);
    bool ok_pitch = is_value_match_with_hysteresis(cur->pitch, ref->pitch, currently_ok);
    bool ok_yaw = is_value_match_with_hysteresis(cur->yaw, ref->yaw, currently_ok);
    bool ok_distance = is_value_match_with_hysteresis(cur->distance_m, ref->distance_m, currently_ok);

    return ok_roll && ok_pitch && ok_yaw && ok_distance;
}

static void apply_result_theme(bool is_ok)
{
    lv_color_t card_bg;
    lv_color_t card_grad;
    lv_color_t border_color;
    lv_color_t shadow_color;
    lv_color_t cell_bg;
    lv_color_t cell_grad;

    if (is_ok)
    {
        card_bg = lv_color_hex(0x0E3A22);
        card_grad = lv_color_hex(0x1F7A47);
        border_color = lv_color_hex(0x55FF99);
        shadow_color = lv_color_hex(0x39FF7A);

        cell_bg = lv_color_hex(0x146C3A);
        cell_grad = lv_color_hex(0x22A354);
    }
    else
    {
        card_bg = lv_color_hex(0x3A0E12);
        card_grad = lv_color_hex(0x8B1E28);
        border_color = lv_color_hex(0xFF6677);
        shadow_color = lv_color_hex(0xFF4C5E);

        cell_bg = lv_color_hex(0x8E1F2E);
        cell_grad = lv_color_hex(0xC53B4A);
    }

    if (card)
    {
        lv_obj_set_style_bg_color(card, card_bg, 0);
        lv_obj_set_style_bg_grad_color(card, card_grad, 0);
        lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);

        lv_obj_set_style_border_color(card, border_color, 0);
        lv_obj_set_style_border_width(card, 7, 0);

        lv_obj_set_style_shadow_color(card, shadow_color, 0);
        lv_obj_set_style_shadow_width(card, 46, 0);
        lv_obj_set_style_shadow_spread(card, 12, 0);
        lv_obj_set_style_shadow_opa(card, LV_OPA_80, 0);
    }

    lv_obj_t *cells[4] = {cell_x, cell_y, cell_z, cell_d};
    for (int i = 0; i < 4; i++)
    {
        if (!cells[i])
            continue;

        lv_obj_set_style_bg_color(cells[i], cell_bg, 0);
        lv_obj_set_style_bg_grad_color(cells[i], cell_grad, 0);
        lv_obj_set_style_bg_grad_dir(cells[i], LV_GRAD_DIR_VER, 0);

        lv_obj_set_style_border_color(cells[i], border_color, 0);
        lv_obj_set_style_border_width(cells[i], 3, 0);

        lv_obj_set_style_shadow_color(cells[i], border_color, 0);
        lv_obj_set_style_shadow_width(cells[i], 18, 0);
        lv_obj_set_style_shadow_spread(cells[i], 2, 0);
        lv_obj_set_style_shadow_opa(cells[i], LV_OPA_40, 0);
    }

    lv_obj_t *subs[4] = {label_x_sub, label_y_sub, label_z_sub, label_d_sub};
    for (int i = 0; i < 4; i++)
    {
        if (subs[i])
        {
            lv_obj_set_style_text_color(subs[i], lv_color_hex(0xFFD54F), 0);
        }
    }
}

static void update_pose_compare_theme(void)
{
    app_pose_t current = {
        .roll = s_current_roll,
        .pitch = s_current_pitch,
        .yaw = s_current_yaw,
        .distance_m = s_current_distance,
    };

    app_pose_t pose_before = {0};
    app_pose_t pose_after = {0};

    bool has_before = application_get_pose_before(&pose_before);
    bool has_after = application_get_pose_after(&pose_after);

    if (has_before)
        log_pose_compare_detail("BEFORE", &current, &pose_before);

    if (has_after)
        log_pose_compare_detail("AFTER", &current, &pose_after);

    bool current_theme_ok = s_theme_initialized ? s_theme_is_ok : false;

    bool match_before = false;
    bool match_after = false;

    if (has_before)
        match_before = pose_match_with_hysteresis(&current, &pose_before, current_theme_ok);

    if (has_after)
        match_after = pose_match_with_hysteresis(&current, &pose_after, current_theme_ok);

    bool is_ok = match_before || match_after;

    if (!s_theme_initialized || (is_ok != s_theme_is_ok))
    {
        s_theme_is_ok = is_ok;
        s_theme_initialized = true;
        apply_result_theme(is_ok);
    }

    if (match_before && match_after)
    {
        screen_mode_set_status_if_changed("Tu the hien tai khop voi vi tri truoc hoac sau");
    }
    else if (match_before)
    {
        screen_mode_set_status_if_changed("Tu the hien tai khop voi vi tri truoc");
    }
    else if (match_after)
    {
        screen_mode_set_status_if_changed("Tu the hien tai khop voi vi tri sau");
    }
    else
    {
        if (!has_before && !has_after)
        {
            screen_mode_set_status_if_changed("Chua luu vi tri truoc va sau");
        }
        else if (!has_before)
        {
            screen_mode_set_status_if_changed("Khong khop, va chua luu vi tri truoc");
        }
        else if (!has_after)
        {
            screen_mode_set_status_if_changed("Khong khop, va chua luu vi tri sau");
        }
        else
        {
            screen_mode_set_status_if_changed("Tu the hien tai chua khop");
        }
    }
}

static void apply_mode_title(void)
{
    if (!label_title)
        return;

    if (s_mode == SCREEN_MODE_PERSONAL)
        lv_label_set_text(label_title, "Che do 1: Luyen tap ca nhan");
    else
        lv_label_set_text(label_title, "Che do 2: Luyen tap doi nhom");
}

static void destroy_popup(void)
{
    if (popup_overlay)
    {
        lv_obj_del(popup_overlay);
        popup_overlay = NULL;
        popup_box = NULL;
    }
}

static void popup_show_confirm(save_target_t target);

static void on_popup_cancel(lv_event_t *e)
{
    LV_UNUSED(e);
    ESP_LOGI(TAG, "Popup cancel");
    s_pending_target = SAVE_TARGET_NONE;
    destroy_popup();
}

static void on_popup_back_to_select(lv_event_t *e)
{
    LV_UNUSED(e);
    s_pending_target = SAVE_TARGET_NONE;
    destroy_popup();

    extern void popup_show_select(void);
    popup_show_select();
}

static void on_popup_ok(lv_event_t *e)
{
    LV_UNUSED(e);

    if (s_save_cb)
    {
        if (s_pending_target == SAVE_TARGET_BEFORE)
        {
            s_save_cb(SCREEN_MODE_SAVE_BEFORE);
            screen_mode_set_status_if_changed("Da luu vi tri truoc");
        }
        else if (s_pending_target == SAVE_TARGET_AFTER)
        {
            s_save_cb(SCREEN_MODE_SAVE_AFTER);
            screen_mode_set_status_if_changed("Da luu vi tri sau");
        }
    }

    s_pending_target = SAVE_TARGET_NONE;
    destroy_popup();
}

static void on_select_before(lv_event_t *e)
{
    LV_UNUSED(e);
    ESP_LOGI(TAG, "Select save before");
    s_pending_target = SAVE_TARGET_BEFORE;
    popup_show_confirm(SAVE_TARGET_BEFORE);
}

static void on_select_after(lv_event_t *e)
{
    LV_UNUSED(e);
    ESP_LOGI(TAG, "Select save after");
    s_pending_target = SAVE_TARGET_AFTER;
    popup_show_confirm(SAVE_TARGET_AFTER);
}

void popup_show_select(void)
{
    destroy_popup();

    popup_overlay = lv_obj_create(screen_root);
    lv_obj_remove_style_all(popup_overlay);
    lv_obj_set_size(popup_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(popup_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(popup_overlay, LV_OPA_60, 0);
    lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_SCROLLABLE);

    popup_box = lv_obj_create(popup_overlay);
    lv_obj_set_size(popup_box, 340, 300);
    lv_obj_center(popup_box);
    style_popup_box(popup_box);

    lv_obj_t *lbl = lv_label_create(popup_box);
    lv_label_set_text(lbl, "Chon vi tri can luu");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *btn1 = create_popup_button(popup_box, "Vi tri truoc", on_select_before, NULL,
                                         lv_color_hex(0x2E7D32), 260, 56);
    lv_obj_align(btn1, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *btn2 = create_popup_button(popup_box, "Vi tri sau", on_select_after, NULL,
                                         lv_color_hex(0x1565C0), 260, 56);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 128);

    lv_obj_t *btn3 = create_popup_button(popup_box, "Quay lai", on_popup_cancel, NULL,
                                         lv_color_hex(0x555555), 260, 56);
    lv_obj_align(btn3, LV_ALIGN_TOP_MID, 0, 196);
}

static void popup_show_confirm(save_target_t target)
{
    destroy_popup();

    popup_overlay = lv_obj_create(screen_root);
    lv_obj_remove_style_all(popup_overlay);
    lv_obj_set_size(popup_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(popup_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(popup_overlay, LV_OPA_60, 0);
    lv_obj_clear_flag(popup_overlay, LV_OBJ_FLAG_SCROLLABLE);

    popup_box = lv_obj_create(popup_overlay);
    lv_obj_set_size(popup_box, 350, 230);
    lv_obj_center(popup_box);
    style_popup_box(popup_box);

    lv_obj_t *lbl = lv_label_create(popup_box);
    if (target == SAVE_TARGET_BEFORE)
        lv_label_set_text(lbl, "Luu vao vi tri truoc?");
    else
        lv_label_set_text(lbl, "Luu vao vi tri sau?");

    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 18);

    lv_obj_t *btn_row = lv_obj_create(popup_box);
    lv_obj_set_size(btn_row, 300, 70);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 24, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    create_popup_button(btn_row, "Oke", on_popup_ok, NULL,
                        lv_color_hex(0x2E7D32), 138, 54);

    create_popup_button(btn_row, "Cancel", on_popup_back_to_select, NULL,
                        lv_color_hex(0x7A1F1F), 138, 54);
}

// ===== Create =====
lv_obj_t *screen_mode_create(void)
{
    screen_root = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen_root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_root, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *bg = lv_img_create(screen_root);
    lv_img_set_src(bg, &ui_img_bgr_png);
    lv_obj_center(bg);
    lv_obj_move_background(bg);

    ui_header_create(screen_root);
    ui_header_set_imu_border(false);
    label_title = lv_label_create(screen_root);
    lv_label_set_text(label_title, "Che do 1: Luyen tap ca nhan");
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_title, &font_noto_basic_20_4, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, SAFE_PADDING + 56);

    card = lv_obj_create(screen_root);
    lv_obj_set_size(card, 350, 290);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SAFE_PADDING + 92);
    style_card_base(card);

    panel_grid = lv_obj_create(card);
    lv_obj_set_size(panel_grid, lv_pct(100), lv_pct(100));
    lv_obj_align(panel_grid, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(panel_grid, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_opa(panel_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel_grid, 0, 0);
    lv_obj_set_style_pad_left(panel_grid, 0, 0);
    lv_obj_set_style_pad_right(panel_grid, 0, 0);
    lv_obj_set_style_pad_top(panel_grid, 0, 0);
    lv_obj_set_style_pad_bottom(panel_grid, 0, 0);
    lv_obj_set_style_pad_row(panel_grid, 4, 0);
    lv_obj_set_style_pad_column(panel_grid, 4, 0);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_layout(panel_grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(panel_grid, col_dsc, row_dsc);

    cell_x = create_imu_cell(panel_grid, "X", "+0.0°", "Goc Pitch", &label_x_value, &label_x_sub);
    lv_obj_set_grid_cell(cell_x,
                         LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    cell_y = create_imu_cell(panel_grid, "Y", "+0.0°", "Goc Roll", &label_y_value, &label_y_sub);
    lv_obj_set_grid_cell(cell_y,
                         LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    cell_z = create_imu_cell(panel_grid, "Z", "0.0°", "Goc Yaw", &label_z_value, &label_z_sub);
    lv_obj_set_grid_cell(cell_z,
                         LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    cell_d = create_imu_cell(panel_grid, "D", "0.0m", "Khoang Cach", &label_d_value, &label_d_sub);
    lv_obj_set_grid_cell(cell_d,
                         LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    label_status = lv_label_create(screen_root);
    lv_label_set_text(label_status, "Chua luu vi tri truoc va sau");
    strncpy(s_last_status, "Chua luu vi tri truoc va sau", sizeof(s_last_status) - 1);
    s_last_status[sizeof(s_last_status) - 1] = '\0';

    lv_obj_set_style_text_color(label_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);
    lv_obj_align_to(label_status, card, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    lv_obj_t *btn_back = lv_btn_create(screen_root);
    lv_obj_set_size(btn_back, 120, 45);
    lv_obj_align_to(btn_back, label_status, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);
    lv_obj_add_event_cb(btn_back, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_set_style_radius(btn_back, 25, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_80, 0);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_back);

    apply_mode_title();
    s_theme_initialized = false;
    update_pose_compare_theme();
    ui_header_set_imu_border(false);
    return screen_root;
}

void screen_mode_set_mode(screen_mode_type_t mode)
{
    s_mode = mode;
    apply_mode_title();
}

void screen_mode_set_title(const char *title)
{
    if (!label_title || !title)
        return;
    lv_label_set_text(label_title, title);
}

void screen_mode_set_status(const char *text)
{
    screen_mode_set_status_if_changed(text);
}

void screen_mode_set_card_color(lv_color_t main, lv_color_t bg)
{
    if (!card)
        return;

    lv_obj_set_style_border_color(card, main, 0);
    lv_obj_set_style_shadow_color(card, main, 0);
    lv_obj_set_style_bg_color(card, bg, 0);
}
void screen_mode_update_pose(float roll, float pitch, float yaw, float distance_m)
{
    if (!label_x_value || !label_y_value || !label_z_value || !label_d_value || !card)
        return;

    s_current_roll = roll;
    s_current_pitch = pitch;
    s_current_yaw = yaw;
    s_current_distance = distance_m;

    static char buf_x[32];
    static char buf_y[32];
    static char buf_z[32];
    static char buf_d[32];

    snprintf(buf_x, sizeof(buf_x), "%+.1f°", pitch);
    lv_label_set_text(label_x_value, buf_x);

    snprintf(buf_y, sizeof(buf_y), "%+.1f°", roll);
    lv_label_set_text(label_y_value, buf_y);

    snprintf(buf_z, sizeof(buf_z), "%.1f°", yaw);
    lv_label_set_text(label_z_value, buf_z);

    snprintf(buf_d, sizeof(buf_d), "%.1fm", distance_m);
    lv_label_set_text(label_d_value, buf_d);

    update_pose_compare_theme();
}

void screen_mode_handle_button_click(void)
{
    if (!screen_root)
        return;

    if (lv_scr_act() != screen_root)
    {
        ESP_LOGW(TAG, "Ignore IO0 click because screen_mode is not active");
        return;
    }

    if (popup_overlay)
    {
        ESP_LOGI(TAG, "Popup already open");
        return;
    }

    popup_show_select();
}

void screen_mode_register_save_callback(screen_mode_save_cb_t cb)
{
    s_save_cb = cb;
}