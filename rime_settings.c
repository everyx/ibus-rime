#include "rime_config.h"
#include "rime_settings.h"
#include <rime_api.h>
#include <string.h>

extern RimeApi *rime_api;

static struct ColorSchemeDefinition preset_color_schemes[] = {
  { "aqua", 0xffffff, 0x0a3dfa },
  { "azure", 0xffffff, 0x0a3dea },
  { "ink", 0xffffff, 0x000000 },
  { "luna", 0x000000, 0xffff7f },
  { NULL, 0, 0 }
};

static struct IBusRimeSettings ibus_rime_settings_default = {
  .embed_preedit_text = TRUE,
  .preedit_style = PREEDIT_STYLE_COMPOSITION,
  .cursor_type = CURSOR_TYPE_INSERT,
  .lookup_table_orientation = IBUS_ORIENTATION_SYSTEM,
  .color_scheme = NULL,
};

struct IBusRimeSettings g_ibus_rime_settings;
GHashTable* g_app_settings_cache = NULL;

static void
app_settings_destroy(gpointer data) {
    struct IBusRimeAppSettings* app_settings = (struct IBusRimeAppSettings*)data;
    if (app_settings) {
        g_free(app_settings->app_id);
        g_free(app_settings);
    }
}

static void
select_color_scheme(struct IBusRimeSettings* settings,
		    const char* color_scheme_id)
{
  struct ColorSchemeDefinition* c;
  for (c = preset_color_schemes; c->color_scheme_id; ++c) {
    if (!strcmp(c->color_scheme_id, color_scheme_id)) {
      settings->color_scheme = c;
      g_debug("selected color scheme: %s", color_scheme_id);
      return;
    }
  }
  // fallback to default
  settings->color_scheme = NULL;
}

static void
load_settings_from_path(struct IBusRimeSettings* settings, RimeConfig *config, const char *base_path)
{
  Bool inline_preedit = False;
  gchar *inline_preedit_path = g_strdup_printf("%s%s%s",
                                base_path ? base_path : "",
                                (base_path && strlen(base_path) > 0) ? "/" : "",
                                "style/inline_preedit");
  if (rime_api->config_get_bool(config, inline_preedit_path, &inline_preedit)) {
    settings->embed_preedit_text = !!inline_preedit;
  }
  g_free(inline_preedit_path);

  gchar *predict_style_path = g_strdup_printf("%s%s%s",
                                  base_path ? base_path : "",
                                  (base_path && strlen(base_path) > 0) ? "/" : "",
                                  "style/preedit_style");
  const char* preedit_style_str = rime_api->config_get_cstring(config, predict_style_path);
  g_free(predict_style_path);
  if(preedit_style_str) {
    if(!strcmp(preedit_style_str, "composition")) {
      settings->preedit_style = PREEDIT_STYLE_COMPOSITION;
    } else if(!strcmp(preedit_style_str, "preview")) {
      settings->preedit_style = PREEDIT_STYLE_PREVIEW;
    }
  }

  gchar *cursor_type_path = g_strdup_printf("%s%s%s",
                                  base_path ? base_path : "",
                                  (base_path && strlen(base_path) > 0) ? "/" : "",
                                  "style/cursor_type");
  const char* cursor_type_str = rime_api->config_get_cstring(config, cursor_type_path);
  g_free(cursor_type_path);
  if (cursor_type_str) {
    if (!strcmp(cursor_type_str, "insert")) {
      settings->cursor_type = CURSOR_TYPE_INSERT;
    } else if (!strcmp(cursor_type_str, "select")) {
      settings->cursor_type = CURSOR_TYPE_SELECT;
    }
  }

  Bool horizontal = False;
  gchar *horizontal_path = g_strdup_printf("%s%s%s",
                                base_path ? base_path : "",
                                (base_path && strlen(base_path) > 0) ? "/" : "",
                                "style/horizontal");
  if (rime_api->config_get_bool(config, horizontal_path, &horizontal)) {
    settings->lookup_table_orientation =
      horizontal ? IBUS_ORIENTATION_HORIZONTAL : IBUS_ORIENTATION_VERTICAL;
  }
  g_free(horizontal_path);

  gchar *color_scheme_path = g_strdup_printf("%s%s%s",
                                base_path ? base_path : "",
                                (base_path && strlen(base_path) > 0) ? "/" : "",
                                "style/color_scheme");
  const char* color_scheme = rime_api->config_get_cstring(config, color_scheme_path);
  g_free(color_scheme_path);
  if (color_scheme) {
    select_color_scheme(settings, color_scheme);
  }
}

void ibus_rime_load_settings(const gchar *app_id, gboolean force_reload)
{
  const gchar* current_app_id = (app_id && strlen(app_id) > 0) ? app_id : "";

  if (!g_app_settings_cache) {
      g_app_settings_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, app_settings_destroy);
  }

  if (force_reload) {
      g_hash_table_remove_all(g_app_settings_cache);
  } else {
      if (g_hash_table_contains(g_app_settings_cache, current_app_id)) {
          struct IBusRimeAppSettings *cached = g_hash_table_lookup(g_app_settings_cache, current_app_id);
          g_ibus_rime_settings = cached->settings;
          return;
      }
  }

  RimeConfig config = {0};
  if (!rime_api->config_open("ibus_rime", &config)) {
    g_error("error loading settings for ibus_rime");
    return;
  }

  struct IBusRimeSettings new_settings = ibus_rime_settings_default;
  load_settings_from_path(&new_settings, &config, "");

  if (strlen(current_app_id) > 0) {
      gchar *app_path = g_strdup_printf("app_options/%s", current_app_id);
      if (rime_api->config_get_item(&config, app_path, NULL)) {
          load_settings_from_path(&new_settings, &config, app_path);
      }
      g_free(app_path);
  }

  rime_api->config_close(&config);

  g_ibus_rime_settings = new_settings;

  struct IBusRimeAppSettings *app_settings_to_cache = g_malloc(sizeof(struct IBusRimeAppSettings));
  app_settings_to_cache->app_id = g_strdup(current_app_id);
  app_settings_to_cache->settings = g_ibus_rime_settings;
  g_hash_table_replace(g_app_settings_cache, g_strdup(current_app_id), app_settings_to_cache);
}
