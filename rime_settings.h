#ifndef __IBUS_RIME_SETTINGS_H__
#define __IBUS_RIME_SETTINGS_H__

#include <ibus.h>
#include <glib.h>

// colors
#define RIME_COLOR_LIGHT  0xd4d4d4
#define RIME_COLOR_DARK   0x606060
#define RIME_COLOR_BLACK  0x000000

enum PreeditStyle {
  PREEDIT_STYLE_COMPOSITION,
  PREEDIT_STYLE_PREVIEW,
};

enum CursorType {
  CURSOR_TYPE_INSERT,
  CURSOR_TYPE_SELECT,
};

struct ColorSchemeDefinition {
  const char* color_scheme_id;
  int text_color;
  int back_color;
};

struct IBusRimeSettings {
  gboolean embed_preedit_text;
  gint preedit_style;
  gint cursor_type;
  gint lookup_table_orientation;
  struct ColorSchemeDefinition* color_scheme;
};

struct AppSpecificSettings {  
  gchar* app_id;  
  struct IBusRimeSettings settings;  
};

struct AppSettingsManager {  
  GHashTable* app_settings;  // app_id -> IBusRimeSettings  
  struct IBusRimeSettings default_settings;  
};

extern struct IBusRimeSettings settings;

void
ibus_rime_load_settings();

#endif
