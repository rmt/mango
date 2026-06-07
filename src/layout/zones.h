#include <ctype.h>
#include <math.h>
#include <string.h>

static bool zones_name_equals(const char *a, const char *b) {
	size_t i;

	if (!a || !b)
		return false;
	if (strlen(a) != strlen(b))
		return false;

	for (i = 0; a[i] != '\0'; i++) {
		if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i]))
			return false;
	}

	return true;
}

static const ConfigZone *zones_find(const char *name) {
	int32_t i;

	if (!name)
		return NULL;

	for (i = 0; i < config.zones_count; i++) {
		if (zones_name_equals(config.zones[i].name, name))
			return &config.zones[i];
	}

	return NULL;
}

static const ConfigZone *zones_first(void) {
	return config.zones_count > 0 ? &config.zones[0] : NULL;
}

static bool zones_client_has_valid_zone(Client *c) {
	return c && c->zone_name && zones_find(c->zone_name);
}

static bool zones_client_is_docked_floating(Client *c) {
	return c && c->isfloating && zones_client_has_valid_zone(c);
}

static bool zones_set_client_zone(Client *c, const ConfigZone *zone) {
	char *name = NULL;

	if (!c || !zone || !zone->name)
		return false;

	name = strdup(zone->name);
	if (!name)
		return false;

	free(c->zone_name);
	c->zone_name = name;
	return true;
}

static const ConfigZone *zones_default_for_monitor(Monitor *m) {
	const ConfigZone *zone = NULL;

	if (!m)
		return zones_first();

	if (config.defaultzone &&
		zones_name_equals(config.defaultzone, "current")) {
		if (zones_client_has_valid_zone(m->sel))
			return zones_find(m->sel->zone_name);
		return zones_first();
	}

	zone = zones_find(config.defaultzone);
	if (zone)
		return zone;

	return zones_first();
}

static struct wlr_box zones_monitor_base(Monitor *m) {
	struct wlr_box base = m->w;

	if (enablegaps) {
		base.x += m->gappoh;
		base.y += m->gappov;
		base.width = MAX(base.width - 2 * m->gappoh, 1);
		base.height = MAX(base.height - 2 * m->gappov, 1);
	}

	return base;
}

static struct wlr_box zones_box(Monitor *m, const ConfigZone *zone) {
	struct wlr_box base;
	struct wlr_box box;
	int32_t max_x, max_y;

	if (!m || !zone)
		return (struct wlr_box){0};

	base = zones_monitor_base(m);
	box = base;

	box.x = base.x + (int32_t)round(base.width * zone->x);
	box.y = base.y + (int32_t)round(base.height * zone->y);
	box.width = MAX((int32_t)round(base.width * zone->w), 1);
	box.height = MAX((int32_t)round(base.height * zone->h), 1);

	max_x = base.x + base.width;
	max_y = base.y + base.height;

	if (box.x < base.x)
		box.x = base.x;
	if (box.y < base.y)
		box.y = base.y;
	if (box.x >= max_x)
		box.x = max_x - 1;
	if (box.y >= max_y)
		box.y = max_y - 1;
	if (box.x + box.width > max_x)
		box.width = MAX(max_x - box.x, 1);
	if (box.y + box.height > max_y)
		box.height = MAX(max_y - box.y, 1);

	return box;
}

static bool zones_starts_left(const ConfigZone *zone) {
	return zone && zone->x <= 0.0;
}

static bool zones_ends_right(const ConfigZone *zone) {
	return zone && zone->x + zone->w >= 1.0;
}

static bool zones_starts_top(const ConfigZone *zone) {
	return zone && zone->y <= 0.0;
}

static bool zones_ends_bottom(const ConfigZone *zone) {
	return zone && zone->y + zone->h >= 1.0;
}

static struct wlr_box zones_align_floating(Client *c, const ConfigZone *zone) {
	struct wlr_box zone_box = zones_box(c->mon, zone);
	struct wlr_box geom = c->geom;

	if (zones_starts_left(zone))
		geom.x = zone_box.x;
	else if (zones_ends_right(zone))
		geom.x = zone_box.x + zone_box.width - geom.width;
	else
		geom.x = zone_box.x + (zone_box.width - geom.width) / 2;

	if (zones_starts_top(zone))
		geom.y = zone_box.y;
	else if (zones_ends_bottom(zone))
		geom.y = zone_box.y + zone_box.height - geom.height;
	else
		geom.y = zone_box.y + (zone_box.height - geom.height) / 2;

	return geom;
}

static int32_t zones_intersection_area(struct wlr_box a, struct wlr_box b) {
	int32_t x1 = MAX(a.x, b.x);
	int32_t y1 = MAX(a.y, b.y);
	int32_t x2 = MIN(a.x + a.width, b.x + b.width);
	int32_t y2 = MIN(a.y + a.height, b.y + b.height);

	if (x2 <= x1 || y2 <= y1)
		return 0;

	return (x2 - x1) * (y2 - y1);
}

static const ConfigZone *zones_pick_for_box(Monitor *m, struct wlr_box geom) {
	const ConfigZone *best_zone = NULL;
	int32_t i;
	int32_t best_overlap = 0;
	int64_t best_zone_area = INT64_MAX;

	if (!m)
		return NULL;

	for (i = 0; i < config.zones_count; i++) {
		const ConfigZone *zone = &config.zones[i];
		struct wlr_box box = zones_box(m, zone);
		int32_t overlap = zones_intersection_area(geom, box);
		int64_t area = (int64_t)box.width * box.height;

		if (overlap > best_overlap ||
			(overlap == best_overlap && overlap > 0 && area < best_zone_area)) {
			best_zone = zone;
			best_overlap = overlap;
			best_zone_area = area;
		}
	}

	return best_zone;
}

static int32_t zones_visible_tiled_occupancy(Monitor *m, const ConfigZone *zone) {
	Client *c;
	int32_t count = 0;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || !ISTILED(c) || !zones_client_has_valid_zone(c))
			continue;
		if (zones_name_equals(c->zone_name, zone->name))
			count++;
	}

	return count;
}

static void zones_assign_visible_by_geometry(Monitor *m, bool force) {
	Client *c;
	int32_t i;

	if (!m || config.zones_count <= 0)
		return;

	wl_list_for_each(c, &clients, link) {
		const ConfigZone *best_zone = NULL;
		int32_t best_overlap = -1;
		int32_t best_occupancy = INT_MAX;
		int64_t best_zone_area = INT64_MAX;

		if (!VISIBLEON(c, m) || client_is_unmanaged(c) ||
			c->iskilling || c->isfullscreen || c->ismaximizescreen ||
			(!force && zones_client_has_valid_zone(c)))
			continue;

		for (i = 0; i < config.zones_count; i++) {
			const ConfigZone *zone = &config.zones[i];
			struct wlr_box zone_box = zones_box(m, zone);
			int32_t overlap = zones_intersection_area(c->geom, zone_box);
			int32_t occupancy = zones_visible_tiled_occupancy(m, zone);
			int64_t zone_area = (int64_t)zone_box.width * zone_box.height;

			if (overlap > best_overlap ||
				(overlap == best_overlap && zone_area < best_zone_area) ||
				(overlap == best_overlap && zone_area == best_zone_area &&
				 occupancy < best_occupancy)) {
				best_zone = zone;
				best_overlap = overlap;
				best_occupancy = occupancy;
				best_zone_area = zone_area;
			}
		}

		if (!best_zone || best_overlap == 0)
			best_zone = zones_default_for_monitor(m);
		if (best_zone && zones_set_client_zone(c, best_zone) && c->isfloating) {
			c->geom = zones_align_floating(c, best_zone);
			c->iscustompos = 1;
			c->float_geom = c->geom;
			if (!c->isoverlay)
				wlr_scene_node_reparent(&c->scene->node, layers[LyrTile]);
			resize(c, c->geom, 0);
		}
	}
}

static void zones_assign_missing_visible(Monitor *m) {
	zones_assign_visible_by_geometry(m, false);
}

static void zones_clear_visible(Monitor *m) {
	Client *c;

	if (!m)
		return;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || !c->zone_name)
			continue;

		free(c->zone_name);
		c->zone_name = NULL;

		if (c->isfloating && !c->isoverlay)
			wlr_scene_node_reparent(&c->scene->node, layers[LyrTop]);
	}
}

void zones(Monitor *m) {
	Client *c = NULL;
	const ConfigZone *zone = NULL;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || !ISTILED(c))
			continue;

		zone = zones_client_has_valid_zone(c) ? zones_find(c->zone_name)
											  : zones_default_for_monitor(m);
		if (!zone)
			continue;

		client_tile_resize(c, zones_box(m, zone), 0);
	}
}
