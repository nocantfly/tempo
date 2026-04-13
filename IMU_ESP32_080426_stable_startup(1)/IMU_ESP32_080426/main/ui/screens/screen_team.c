#include "screen_team.h"
#include "ui.h"
#include "ui_header.h"
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH   410
#define SCREEN_HEIGHT  502
#define SAFE_PADDING   30

// ===== UI refs =====
static lv_obj_t *card;
static lv_obj_t *panel_grid;

static lv_obj_t *label_x_value;
static lv_obj_t *label_y_value;
static lv_obj_t *label_z_value;
static lv_obj_t *label_d_value;

static lv_obj_t *label_status;

// ===== Back =====
static void on_back(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_switch_screen(UI_SCREEN_MAIN);
}

// ===== Style helpers =====
static void style_card_base(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1A2318), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);

    lv_obj_set_style_radius(obj, 24, 0);
    lv_obj_set_style_border_width(obj, 3, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x39FF5A), 0);

    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x39FF5A), 0);
    lv_obj_set_style_shadow_width(obj, 28, 0);
    lv_obj_set_style_shadow_spread(obj, 3, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_60, 0);

    lv_obj_set_style_pad_all(obj, 6, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_cell_base(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x567050), 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *create_imu_cell(lv_obj_t *parent,
                                 const char *title,
                                 const char *value,
                                 const char *sub,
                                 lv_obj_t **out_value_label)
{
    lv_obj_t *cell = lv_obj_create(parent);
    style_cell_base(cell);

    // ===== Title X/Y/Z/D =====
    lv_obj_t *lbl_title = lv_label_create(cell);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xE7D07A), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 6);

    // ===== Value background =====
    lv_obj_t *value_bg = lv_obj_create(cell);
    lv_obj_set_size(value_bg, 150, 44);
    lv_obj_align(value_bg, LV_ALIGN_CENTER, 0, 4);
    lv_obj_clear_flag(value_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(value_bg, 6, 0);
    lv_obj_set_style_border_width(value_bg, 0, 0);
    lv_obj_set_style_bg_color(value_bg, lv_color_hex(0xD8C36A), 0);
    lv_obj_set_style_bg_opa(value_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(value_bg, 0, 0);

    // ===== Value label =====
    lv_obj_t *lbl_value = lv_label_create(value_bg);
    lv_label_set_text(lbl_value, value);
    lv_obj_set_style_text_color(lbl_value, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_font(lbl_value, &lv_font_montserrat_34, 0);
    lv_obj_center(lbl_value);

    // ===== Sub label =====
    lv_obj_t *lbl_sub = lv_label_create(cell);
    lv_label_set_text(lbl_sub, sub);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0x67D96C), 0);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_BOTTOM_MID, 0, -8);

    if (out_value_label)
        *out_value_label = lbl_value;

    return cell;
}

static void set_card_theme(lv_color_t main_color, lv_color_t bg_color)
{
    if (!card)
        return;

    lv_obj_set_style_border_color(card, main_color, 0);
    lv_obj_set_style_shadow_color(card, main_color, 0);
    lv_obj_set_style_bg_color(card, bg_color, 0);
}

// ===== Create =====
lv_obj_t *screen_team_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ===== Header =====
    ui_header_create(scr);

    // ===== Title =====
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Che do 2: Luyen tap doi nhom");
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_title, &font_noto_basic_20_4, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, SAFE_PADDING + 56);

    // ===== Card =====
    card = lv_obj_create(scr);
    lv_obj_set_size(card, 350, 290);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, SAFE_PADDING + 92);
    style_card_base(card);

    // ===== Grid full card =====
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

    // X
    lv_obj_t *cell_x = create_imu_cell(panel_grid, "X", "+15.3°", "Goc Pitch", &label_x_value);
    lv_obj_set_grid_cell(cell_x,
                         LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    // Y
    lv_obj_t *cell_y = create_imu_cell(panel_grid, "Y", "+42.5°", "Goc Roll", &label_y_value);
    lv_obj_set_grid_cell(cell_y,
                         LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    // Z
    lv_obj_t *cell_z = create_imu_cell(panel_grid, "Z", "210.8°", "Goc Yaw", &label_z_value);
    lv_obj_set_grid_cell(cell_z,
                         LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    // D
    lv_obj_t *cell_d = create_imu_cell(panel_grid, "D", "325.4m", "Khoang Cach", &label_d_value);
    lv_obj_set_grid_cell(cell_d,
                         LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    // ===== Status outside card =====
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Chuan hoa bo cam bien: hoan tat");
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_16, 0);
    lv_obj_align_to(label_status, card, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    // ===== Back button =====
    lv_obj_t *btn_back = lv_btn_create(scr);
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

    return scr;
}

//
// ===== API =====
//

void screen_team_set_status(const char *text)
{
    if (!label_status)
        return;

    lv_label_set_text(label_status, text);
}

void screen_team_set_card_color(lv_color_t main, lv_color_t bg)
{
    set_card_theme(main, bg);
}

void screen_team_set_distance(float distance_m)
{
    if (!label_d_value)
        return;

    static char buf[32];
    snprintf(buf, sizeof(buf), "%.1fm", distance_m);
    lv_label_set_text(label_d_value, buf);
}

void screen_team_update_imu(float roll, float pitch, float yaw)
{
    if (!label_x_value || !label_y_value || !label_z_value || !card)
        return;

    static char buf_x[32];
    static char buf_y[32];
    static char buf_z[32];

    snprintf(buf_x, sizeof(buf_x), "%+.1f°", pitch);
    snprintf(buf_y, sizeof(buf_y), "%+.1f°", roll);
    snprintf(buf_z, sizeof(buf_z), "%.1f°", yaw);

    lv_label_set_text(label_x_value, buf_x);
    lv_label_set_text(label_y_value, buf_y);
    lv_label_set_text(label_z_value, buf_z);

    float abs_pitch = fabsf(pitch);
    float abs_roll  = fabsf(roll);
    float err = (abs_pitch > abs_roll) ? abs_pitch : abs_roll;

    if (err < 5.0f)
    {
        screen_team_set_status("Chuan hoa bo cam bien: hoan tat");
        screen_team_set_card_color(
            lv_color_hex(0x39FF5A),
            lv_color_hex(0x1A2318));
    }
    else if (err < 15.0f)
    {
        screen_team_set_status("Chuan hoa bo cam bien: can chinh nhe");
        screen_team_set_card_color(
            lv_color_hex(0xFFD94D),
            lv_color_hex(0x2D2A16));
    }
    else
    {
        screen_team_set_status("Chuan hoa bo cam bien: lech nhieu");
        screen_team_set_card_color(
            lv_color_hex(0xFF5A5A),
            lv_color_hex(0x2A1616));
    }
}