#ifndef __IBUS_RIME_SETTINGS_H__
#define __IBUS_RIME_SETTINGS_H__

#include <ibus.h>
#include <libintl.h>

#define GETTEXT_PACKAGE "ibus-rime"
#define _(x) dgettext(GETTEXT_PACKAGE, x)

// colors
#define RIME_COLOR_LIGHT  0xd4d4d4
#define RIME_COLOR_DARK   0x606060
#define RIME_COLOR_BLACK  0x000000

// What a channel draws: the schema's pre-edit string, the keys the user typed,
// the text the current candidate would commit, or nothing.
enum PreeditChannel {
  CHANNEL_NONE,
  CHANNEL_COMPOSITION,
  CHANNEL_RAW,
  CHANNEL_PREVIEW,
};

// What happens to a composition when the focus leaves it: discard it, or hand
// it to the client that is losing the focus (IBUS_ENGINE_PREEDIT_CLEAR /
// IBUS_ENGINE_PREEDIT_COMMIT, the same switch ibus-hangul relies on).
enum FocusOutBehavior {
  FOCUS_OUT_CANCEL,
  FOCUS_OUT_COMMIT,
};

struct IBusRimeSettings {
  gint preedit;
  gint auxiliary;
  gint focus_out_behavior;
  gint lookup_table_orientation;
  gboolean status_hint;
};

extern struct IBusRimeSettings g_ibus_rime_settings;

void
ibus_rime_load_settings();

#endif
