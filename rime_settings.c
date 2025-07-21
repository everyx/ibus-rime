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

struct IBusRimeSettings settings;

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

// 新增：初始化应用程序设置管理器
void
ibus_rime_init_app_settings_manager()
{
  g_app_settings_manager.app_settings = g_hash_table_new_full(
      g_str_hash, g_str_equal, g_free, g_free);
  g_app_settings_manager.default_settings = ibus_rime_settings_default;
}

// 新增：清理应用程序设置管理器
void
ibus_rime_cleanup_app_settings_manager()
{
  if (g_app_settings_manager.app_settings) {
    g_hash_table_destroy(g_app_settings_manager.app_settings);
    g_app_settings_manager.app_settings = NULL;
  }
}

// 新增：加载应用程序特定设置
void
ibus_rime_load_app_specific_settings(const gchar* app_id, struct IBusRimeSettings* settings)
{
  if (!app_id || !settings) {
    return;
  }

  // 先使用默认设置
  *settings = g_app_settings_manager.default_settings;

  RimeConfig config = {0};
  if (!rime_api->config_open("ibus_rime", &config)) {
    g_warning("error loading settings for app: %s", app_id);
    return;
  }

  // 构建应用程序特定的配置路径
  gchar* app_path = g_strdup_printf("applications/%s", app_id);
  
  // 检查是否存在应用程序特定配置
  Bool has_app_config = False;
  if (rime_api->config_get_bool(&config, app_path, &has_app_config) && !has_app_config) {
    // 如果没有应用程序特定配置，使用默认设置
    rime_api->config_close(&config);
    g_free(app_path);
    return;
  }

  // 加载应用程序特定的设置
  gchar* inline_preedit_path = g_strdup_printf("%s/style/inline_preedit", app_path);
  Bool inline_preedit = False;
  if (rime_api->config_get_bool(&config, inline_preedit_path, &inline_preedit)) {
    settings->embed_preedit_text = !!inline_preedit;
  }
  g_free(inline_preedit_path);

  gchar* preedit_style_path = g_strdup_printf("%s/style/preedit_style", app_path);
  const char* preedit_style_str = rime_api->config_get_cstring(&config, preedit_style_path);
  if (preedit_style_str) {
    if (!strcmp(preedit_style_str, "composition")) {
      settings->preedit_style = PREEDIT_STYLE_COMPOSITION;
    } else if (!strcmp(preedit_style_str, "preview")) {
      settings->preedit_style = PREEDIT_STYLE_PREVIEW;
    }
  }
  g_free(preedit_style_path);

  gchar* cursor_type_path = g_strdup_printf("%s/style/cursor_type", app_path);
  const char* cursor_type_str = rime_api->config_get_cstring(&config, cursor_type_path);
  if (cursor_type_str) {
    if (!strcmp(cursor_type_str, "insert")) {
      settings->cursor_type = CURSOR_TYPE_INSERT;
    } else if (!strcmp(cursor_type_str, "select")) {
      settings->cursor_type = CURSOR_TYPE_SELECT;
    }
  }
  g_free(cursor_type_path);

  gchar* horizontal_path = g_strdup_printf("%s/style/horizontal", app_path);
  Bool horizontal = False;
  if (rime_api->config_get_bool(&config, horizontal_path, &horizontal)) {
    settings->lookup_table_orientation =
      horizontal ? IBUS_ORIENTATION_HORIZONTAL : IBUS_ORIENTATION_VERTICAL;
  }
  g_free(horizontal_path);

  gchar* color_scheme_path = g_strdup_printf("%s/style/color_scheme", app_path);
  const char* color_scheme = rime_api->config_get_cstring(&config, color_scheme_path);
  if (color_scheme) {
    select_color_scheme(settings, color_scheme);
  }
  g_free(color_scheme_path);

  g_free(app_path);
  rime_api->config_close(&config);

  g_debug("loaded app-specific settings for: %s", app_id);
}

// 新增：获取应用程序设置
struct IBusRimeSettings*
ibus_rime_get_app_settings(const gchar* app_id)
{
  if (!app_id) {
    return &g_app_settings_manager.default_settings;
  }

  // 检查缓存
  struct IBusRimeSettings* cached_settings = 
      g_hash_table_lookup(g_app_settings_manager.app_settings, app_id);
  if (cached_settings) {
    return cached_settings;
  }

  // 加载并缓存新的应用程序设置
  struct IBusRimeSettings* new_settings = g_new(struct IBusRimeSettings, 1);
  ibus_rime_load_app_specific_settings(app_id, new_settings);
  
  gchar* app_id_copy = g_strdup(app_id);
  g_hash_table_insert(g_app_settings_manager.app_settings, app_id_copy, new_settings);
  
  return new_settings;
}

void
ibus_rime_load_settings()
{
  settings = ibus_rime_settings_default;

  RimeConfig config = {0};
  if (!rime_api->config_open("ibus_rime", &config)) {
    g_error("error loading settings for ibus_rime");
    return;
  }

  Bool inline_preedit = False;
  if (rime_api->config_get_bool(
          &config, "style/inline_preedit", &inline_preedit)) {
    settings.embed_preedit_text = !!inline_preedit;
  }

  const char* preedit_style_str =
      rime_api->config_get_cstring(&config, "style/preedit_style");
  if(preedit_style_str) {
    if(!strcmp(preedit_style_str, "composition")) {
      settings.preedit_style = PREEDIT_STYLE_COMPOSITION;
    } else if(!strcmp(preedit_style_str, "preview")) {
      settings.preedit_style = PREEDIT_STYLE_PREVIEW;
    }
  }

  const char* cursor_type_str =
      rime_api->config_get_cstring(&config, "style/cursor_type");
  if (cursor_type_str) {
    if (!strcmp(cursor_type_str, "insert")) {
      settings.cursor_type = CURSOR_TYPE_INSERT;
    } else if (!strcmp(cursor_type_str, "select")) {
      settings.cursor_type = CURSOR_TYPE_SELECT;
    }
  }

  Bool horizontal = False;
  if (rime_api->config_get_bool(&config, "style/horizontal", &horizontal)) {
    settings.lookup_table_orientation =
      horizontal ? IBUS_ORIENTATION_HORIZONTAL : IBUS_ORIENTATION_VERTICAL;
  }

  const char* color_scheme =
    rime_api->config_get_cstring(&config, "style/color_scheme");
  if (color_scheme) {
    select_color_scheme(&settings, color_scheme);
  }

  // 保存为默认设置  
  g_app_settings_manager.default_settings = settings;

  rime_api->config_close(&config);
}
