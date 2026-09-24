#ifndef MANGO_LAYOUT_ZONES_H
#define MANGO_LAYOUT_ZONES_H

#include "mango/common/types.h"
#include <stdbool.h>
#include <wlr/util/box.h>

typedef struct ConfigZone ConfigZone;

const ConfigZone *zones_find(const char *name);
bool zones_client_has_valid_zone(Client *c);
bool zones_client_is_docked_floating(Client *c);
bool zones_clients_share_zone(Client *a, Client *b);
bool zones_set_client_zone(Client *c, const ConfigZone *zone);
const ConfigZone *zones_default_for_monitor(Monitor *m);
struct wlr_box zones_box(Monitor *m, const ConfigZone *zone);
struct wlr_box zones_align_floating(Client *c, const ConfigZone *zone);
const ConfigZone *zones_pick_for_box(Monitor *m, struct wlr_box geom);
void zones_assign_visible_by_geometry(Monitor *m, bool force);
void zones_assign_missing_visible(Monitor *m);
void zones_realign_visible_floating(Monitor *m);
void zones_clear_visible(Monitor *m);
void zones(Monitor *m);

#endif
