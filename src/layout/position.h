#include <ctype.h>
#include <stddef.h>

static bool position_zone_name_equals(const char *name, size_t len,
									  const char *zone_name) {
	size_t i = 0;

	if (!name || !zone_name || strlen(zone_name) != len)
		return false;

	for (i = 0; i < len; i++) {
		if (toupper((unsigned char)name[i]) !=
			toupper((unsigned char)zone_name[i]))
			return false;
	}

	return true;
}

static uint32_t parse_position_zone_span(const char *name, size_t len) {
	if (position_zone_name_equals(name, len, "C"))
		return POS_ZONE_C;
	if (position_zone_name_equals(name, len, "BC"))
		return POS_ZONE_BC;
	if (position_zone_name_equals(name, len, "N"))
		return POS_ZONE_N;
	if (position_zone_name_equals(name, len, "S"))
		return POS_ZONE_S;
	if (position_zone_name_equals(name, len, "E"))
		return POS_ZONE_E;
	if (position_zone_name_equals(name, len, "W"))
		return POS_ZONE_W;
	if (position_zone_name_equals(name, len, "NW"))
		return POS_ZONE_NW;
	if (position_zone_name_equals(name, len, "NE"))
		return POS_ZONE_NE;
	if (position_zone_name_equals(name, len, "SW"))
		return POS_ZONE_SW;
	if (position_zone_name_equals(name, len, "SE"))
		return POS_ZONE_SE;
	if (position_zone_name_equals(name, len, "BN"))
		return POS_ZONE_BN;
	if (position_zone_name_equals(name, len, "BS"))
		return POS_ZONE_BS;
	if (position_zone_name_equals(name, len, "BE"))
		return POS_ZONE_BE;
	if (position_zone_name_equals(name, len, "BW"))
		return POS_ZONE_BW;
	if (position_zone_name_equals(name, len, "BNW"))
		return POS_ZONE_BNW;
	if (position_zone_name_equals(name, len, "BNE"))
		return POS_ZONE_BNE;
	if (position_zone_name_equals(name, len, "BSW"))
		return POS_ZONE_BSW;
	if (position_zone_name_equals(name, len, "BSE"))
		return POS_ZONE_BSE;

	return POS_ZONE_NONE;
}

static uint32_t parse_position_zone(const char *name) {
	if (!name)
		return POS_ZONE_NONE;

	return parse_position_zone_span(name, strlen(name));
}

static bool position_zone_matches(uint32_t zone, uint32_t mask) {
	if (zone <= POS_ZONE_NONE || zone > POS_ZONE_BSE)
		return false;

	return (mask & (1u << zone)) != 0;
}

static uint32_t position_zone_mask_from_string(const char *list) {
	const char *cursor = NULL;
	const char *token = NULL;
	uint32_t mask = 0;

	if (!list)
		return 0;

	token = list;
	for (cursor = list; ; cursor++) {
		if (*cursor != '|' && *cursor != '\0')
			continue;

		if (cursor > token) {
			uint32_t zone =
				parse_position_zone_span(token, (size_t)(cursor - token));
			if (zone > POS_ZONE_NONE && zone <= POS_ZONE_BSE)
				mask |= (1u << zone);
		}

		if (*cursor == '\0')
			break;

		token = cursor + 1;
	}

	return mask;
}

static bool position_zone_left_aligned(uint32_t zone) {
	switch (zone) {
	case POS_ZONE_W:
	case POS_ZONE_BW:
	case POS_ZONE_NW:
	case POS_ZONE_BNW:
	case POS_ZONE_SW:
	case POS_ZONE_BSW:
		return true;
	default:
		return false;
	}
}

static bool position_zone_right_aligned(uint32_t zone) {
	switch (zone) {
	case POS_ZONE_E:
	case POS_ZONE_BE:
	case POS_ZONE_NE:
	case POS_ZONE_BNE:
	case POS_ZONE_SE:
	case POS_ZONE_BSE:
		return true;
	default:
		return false;
	}
}

static bool position_zone_top_aligned(uint32_t zone) {
	switch (zone) {
	case POS_ZONE_N:
	case POS_ZONE_BN:
	case POS_ZONE_NW:
	case POS_ZONE_BNW:
	case POS_ZONE_NE:
	case POS_ZONE_BNE:
		return true;
	default:
		return false;
	}
}

static bool position_zone_bottom_aligned(uint32_t zone) {
	switch (zone) {
	case POS_ZONE_S:
	case POS_ZONE_BS:
	case POS_ZONE_SW:
	case POS_ZONE_BSW:
	case POS_ZONE_SE:
	case POS_ZONE_BSE:
		return true;
	default:
		return false;
	}
}

static struct wlr_box position_zone_box(Monitor *m, uint32_t zone) {
	struct wlr_box base = m->w;
	struct wlr_box box = m->w;
	int32_t width_percent = 70;
	int32_t height_percent = 70;

	if (enablegaps) {
		base.x += m->gappoh;
		base.y += m->gappov;
		base.width = MAX(base.width - 2 * m->gappoh, 1);
		base.height = MAX(base.height - 2 * m->gappov, 1);
	}

	switch (zone) {
	case POS_ZONE_BC:
		width_percent = 90;
		height_percent = 90;
		break;
	case POS_ZONE_N:
	case POS_ZONE_S:
		width_percent = 100;
		height_percent = 50;
		break;
	case POS_ZONE_E:
	case POS_ZONE_W:
		width_percent = 50;
		height_percent = 100;
		break;
	case POS_ZONE_NW:
	case POS_ZONE_NE:
	case POS_ZONE_SW:
	case POS_ZONE_SE:
		width_percent = 50;
		height_percent = 50;
		break;
	case POS_ZONE_BN:
	case POS_ZONE_BS:
		width_percent = 100;
		height_percent = 70;
		break;
	case POS_ZONE_BE:
	case POS_ZONE_BW:
		width_percent = 70;
		height_percent = 100;
		break;
	case POS_ZONE_BNW:
	case POS_ZONE_BNE:
	case POS_ZONE_BSW:
	case POS_ZONE_BSE:
		width_percent = 70;
		height_percent = 70;
		break;
	case POS_ZONE_C:
	default:
		width_percent = 70;
		height_percent = 70;
		break;
	}

	box.width = MAX((base.width * width_percent + 50) / 100, 1);
	box.height = MAX((base.height * height_percent + 50) / 100, 1);

	if (position_zone_left_aligned(zone))
		box.x = base.x;
	else if (position_zone_right_aligned(zone))
		box.x = base.x + base.width - box.width;
	else
		box.x = base.x + (base.width - box.width) / 2;

	if (position_zone_top_aligned(zone))
		box.y = base.y;
	else if (position_zone_bottom_aligned(zone))
		box.y = base.y + base.height - box.height;
	else
		box.y = base.y + (base.height - box.height) / 2;

	return box;
}

static struct wlr_box position_align_floating(Client *c, uint32_t zone) {
	struct wlr_box zone_box = position_zone_box(c->mon, zone);
	struct wlr_box geom = c->geom;

	if (position_zone_left_aligned(zone))
		geom.x = zone_box.x;
	else if (position_zone_right_aligned(zone))
		geom.x = zone_box.x + zone_box.width - geom.width;
	else
		geom.x = zone_box.x + (zone_box.width - geom.width) / 2;

	if (position_zone_top_aligned(zone))
		geom.y = zone_box.y;
	else if (position_zone_bottom_aligned(zone))
		geom.y = zone_box.y + zone_box.height - geom.height;
	else
		geom.y = zone_box.y + (zone_box.height - geom.height) / 2;

	return geom;
}

void position(Monitor *m) {
	Client *c = NULL;
	uint32_t zone = POS_ZONE_NONE;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || !ISTILED(c))
			continue;

		zone = c->position_zone;
		if (zone == POS_ZONE_NONE)
			zone = POS_ZONE_C;

		client_tile_resize(c, position_zone_box(m, zone), 0);
	}
}
