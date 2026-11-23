#include "rime_config.h"
#include "rime_settings.h"
#include <rime_api.h>
#include <string.h>

extern RimeApi *rime_api;

static struct IBusRimeSettings ibus_rime_settings_default = {
  .preedit = CHANNEL_COMPOSITION,
  .auxiliary = CHANNEL_NONE,
  .focus_out_behavior = FOCUS_OUT_CANCEL,
  .lookup_table_orientation = IBUS_ORIENTATION_SYSTEM,
  .status_hint = FALSE,
};

static gint
parse_channel (const char* value)
{
  if (!value) {
    return CHANNEL_NONE;
  }
  if (!strcmp(value, "composition")) {
    return CHANNEL_COMPOSITION;
  }
  if (!strcmp(value, "raw")) {
    return CHANNEL_RAW;
  }
  if (!strcmp(value, "preview")) {
    return CHANNEL_PREVIEW;
  }
  return CHANNEL_NONE;
}

struct IBusRimeSettings g_ibus_rime_settings;

void
ibus_rime_load_settings()
{
  g_ibus_rime_settings = ibus_rime_settings_default;

  RimeConfig config = {0};
  if (!rime_api->config_open("ibus_rime", &config)) {
    g_error("error loading settings for ibus_rime");
    return;
  }

  const char* preedit = rime_api->config_get_cstring(&config, "style/preedit");
  if (preedit) {
    g_ibus_rime_settings.preedit = parse_channel(preedit);
  }

  const char* auxiliary =
      rime_api->config_get_cstring(&config, "style/auxiliary");
  if (auxiliary) {
    g_ibus_rime_settings.auxiliary = parse_channel(auxiliary);
  }

  const char* focus_out =
      rime_api->config_get_cstring(&config, "style/focus_out");
  if (focus_out && !strcmp(focus_out, "commit")) {
    g_ibus_rime_settings.focus_out_behavior = FOCUS_OUT_COMMIT;
  }

  Bool horizontal = False;
  if (rime_api->config_get_bool(&config, "style/horizontal", &horizontal)) {
    g_ibus_rime_settings.lookup_table_orientation =
      horizontal ? IBUS_ORIENTATION_HORIZONTAL : IBUS_ORIENTATION_VERTICAL;
  }

  Bool status_hint = False;
  if (rime_api->config_get_bool(&config, "style/status_hint", &status_hint)) {
    g_ibus_rime_settings.status_hint = !!status_hint;
  }

  rime_api->config_close(&config);
}
