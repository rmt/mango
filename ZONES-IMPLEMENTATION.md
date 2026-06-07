# Zones implementation notes

This document describes the `zones` layout work in this branch, including commits from `c2015127ede3c161988174cfd95b6b1b0b6191f9` onward.

## Commit history covered

### `c2015127` — initial manual positioning layout

Initial implementation of a manual compass-like positioning layout. It added:

- a new layout implementation, originally named around manual positioning
- keybind actions for focusing and placing windows into named regions
- docs and binding docs for the new actions
- config/parser support for the initial feature

This commit established the core idea: a layout can place tiled clients into user-addressable regions rather than deriving all positions from insertion order or master/stack rules.

### `e159fff` — rename to zones and make zones configurable

Renamed the feature to `zones` and made the regions configurable. Major changes:

- replaced the old fixed positioning implementation with `src/layout/zones.h`
- added configurable `zone=` entries with `name`, `x`, `y`, `w`, and `h`
- added `defaultzone`
- added default fallback zones if none are configured
- added docs and sample config for `zones`
- added `focuszone` and `movetozone`
- updated layout registration to include `zones`

This is the point where the feature became a general tiling layout: users define arbitrary rectangular zones, and tiled windows are assigned to those zones.

### `9192bde` — swap `zone_name` when swapping clients

Updated client-position swapping so `zone_name` follows the logical client placement. This keeps zones consistent when existing window exchange/swap operations are used.

### `e7b83fa` — refine zones docking and transitions

Introduced the main follow-up behavior for zones transitions, docked floating windows, best-overlap drag preview, and documentation.

### `fba50be` — fix `focuszone` cycling

Fixed `focuszone` so it preserves the original top-candidate behavior when the focused window is not in the target zone set, but cycles from the current client when the focused window is already in the requested zone set.

### `6035df7` — align floating clients when assigning zones

When entering zones or otherwise assigning visible clients by geometry, floating clients are now actually aligned to their assigned zone. Previously they could receive `zone_name` metadata without moving to the docked position.

### `12652ba` — use zones layout transition for `movetozone`

Changed `movetozone` to enter `zones` through the same transition helper used by `setlayout` and `switch_layout`, then reapply the user-requested target zone to the selected client.

### `16545c6` — guard `zones_box()` before monitor dereference

Moved the `zones_box()` null guard before `zones_monitor_base(m)` so future null monitor call sites cannot dereference `m` before validation.

### `edb257e` — allow docked floating windows to redock

Allowed floating clients that are already docked to a zone to participate in zone drag/drop. Dropping in the same zone preserves the manual position; dropping in a different best-overlap zone updates `zone_name` and snap-aligns to the new zone.

### `7e7355a` — preserve tiled drag state and snap out-of-zone docked moves

Added explicit `drag_was_tiled` state so temporary floating during tiled zone drags can be distinguished from real docked floating drags. Tiled drags restore the previous floating geometry metadata after drop. Docked floating drops outside all zones now snap back to the current/default zone instead of preserving an out-of-zone position with stale `zone_name`.

### `2c16a1e` — assign zones again after clearing fullscreen state

Entering zones assigns visible clients before fullscreen/maximized state is cleared. Clients in those states are skipped by the first assignment pass, so a second `zones_assign_missing_visible()` pass now runs after `clear_fullscreen_and_maximized_state()` whenever the resulting layout is zones.

### `0bf50f3` — centralize client stack reparenting

Added `client_reparent_by_stack()` to centralize stack/layer policy for overlay, top, undocked floating, docked floating, and tiled clients. This removes duplicated layer-selection logic and prevents overlay clients from being accidentally moved out of `LyrOverlay` by later state changes.

### `87f8f66` — delay `movetozone` mutation until prerequisites pass

Changed `movetozone` so tiled clients do not mutate `zone_name` until the command has located the zones layout and is ready to enter/apply it. Floating clients still assign directly because they do not need to switch layouts.

## Implementation details

The follow-up changes refine zones semantics, especially around layout transitions, floating windows, drag/drop, and stacking.

### 1. Layout transition semantics

Added a layout transition helper in `src/dispatch/bind_define.h`:

```c
static void set_monitor_layout(Monitor *m, const Layout *layout)
```

This centralizes behavior for `setlayout` and `switch_layout`.

When switching **to** `zones`:

- visible clients are assigned zones based on their current geometry before the layout resizes them
- assignment is forced, so stale previous assignments do not override best geometry matching

When switching **away from** `zones`:

- visible clients' zone assignments are cleared
- docked floating clients are restored to the normal floating/top layer if they are not overlay windows

Reasoning: zone membership should be layout-local state. Leaving zones should not leave invisible metadata that later surprises the user. Entering zones should infer the best zone from what the user currently sees.

### 2. Best-zone matching

Added/refined zone matching in `src/layout/zones.h`.

Current matching prefers:

1. greatest intersection area between the window geometry and the zone rectangle
2. smallest zone area when intersection ties
3. lowest tiled occupancy as a final tie-break during layout-entry assignment

The smallest-zone tie-break matters when a window is fully inside multiple zones. For example, a window inside a NE corner zone may also be fully inside a top-half zone. Both have equal overlap, so the smaller NE zone should win.

Drag preview now uses the dragged window's full floating geometry rather than just the cursor point:

```c
zones_pick_for_box(m, grabc->float_geom)
```

Reasoning: users expect the target zone to be determined by the window they are dragging, not by a single cursor coordinate. If a 25% corner-sized window overlaps a 50% center zone more than a corner zone, the center zone should be highlighted and selected.

### 3. Floating windows can be docked to zones

A floating client with a valid `zone_name` is treated as a **docked floating** client:

```c
static bool zones_client_is_docked_floating(Client *c) {
    return c && c->isfloating && zones_client_has_valid_zone(c);
}
```

Docked floating behavior:

- the client remains floating
- it keeps its own size
- it is aligned to its assigned zone
- it is stacked with tiled content by default unless overlay is enabled

`movetozone` on a floating client:

- assigns the zone
- aligns the floating geometry to that zone
- keeps the floating size
- reparents non-overlay floating clients to the tiled layer

`movetozone` on a tiled client outside the zones layout enters zones through `set_monitor_layout()`, so it uses the same transition semantics as `setlayout,zones` and `switch_layout`. Because entering zones force-assigns visible clients by geometry, `movetozone` then reapplies the user-requested target zone to the selected client.

`togglefloating` while in zones:

- does not arbitrarily choose a new floating state; it only responds to the user's explicit toggle
- when a client becomes floating in zones, it is explicitly assigned/aligned to its current/default zone

Reasoning: floating-vs-tiled is a user or rules decision. Zone docking is an anchoring/placement feature, not a hidden request to change the user's chosen window mode.

### 4. Drag-to-zone behavior

Dragging a tiled window in `zones` uses temporary floating geometry internally, but dropping the window must not change whether it is floating.

The tiled drop path now:

- assigns the drop zone
- restores the client to tiled with `setfloating(c, 0)`
- hides the zone drop preview

Docked floating clients can also use zone drag/drop:

- moving within the same zone preserves the user's new manual position
- dropping onto a different best-overlap zone updates `zone_name`
- redocking to a different zone snap-aligns through `zones_align_floating()`
- dropping outside all zones snaps back to the current/default zone
- the client remains floating

Tiled zone drags are tracked with `drag_was_tiled` because they use temporary floating geometry while dragging. On drop, they are tiled again and their previous floating geometry metadata is restored so a later `togglefloating` does not resurrect the drag preview geometry.

Reasoning: dragging a tiled window from zone A to zone B is a zone reassignment, not a request to float the window. Only explicit user actions such as `togglefloating`, or window match/rule configuration, should determine floating state. For already-floating docked windows, drag/drop is a redocking operation rather than a floating-state change, and out-of-zone drops should not leave a stale zone assignment paired with an out-of-zone position.

### 5. Zone drop preview

Added an overlay rectangle for zone drop feedback:

- `zone_droparea`
- `dropzone`
- `show_zone_droparea()`
- `hide_zone_droparea()`

During zone drags, Mango highlights the best matching zone. The highlighted `dropzone` is used when the mouse button is released.

Reasoning: zones can overlap, so the user needs visible feedback about which zone will receive the window.

### 6. Stacking model and overlay behavior

The branch previously experimented with a separate `togglefloatabove` concept. That was rejected/removed from the current plan.

Current stacking model is centralized in `client_reparent_by_stack()`:

- normal tiled clients: `LyrTile`
- undocked floating clients: `LyrTop`, preserving existing Mango behavior
- docked floating clients: `LyrTile` by default
- overlay clients: `LyrOverlay`
- explicit forced-top states such as fullscreen: `LyrTop`
- XWayland above/top hints still map to top-layer behavior

`toggleoverlay` remains the explicit always-on-top action. Docked floating windows are deliberately not always-on-top by default.

Reasoning: the problem was not that floating windows needed another independent boolean. The problem was that a floating window can be spatially docked to a zone while still retaining its floating size. Such windows should not permanently obscure neighboring tiled content unless the user explicitly asks for overlay behavior.

### 7. Focus behavior for docked floating windows

Focused docked floating windows may be raised through existing focus/lift behavior. When focus moves away from a docked floating window that is not overlayed, it is lowered again.

Reasoning: this allows a docked floating window to be usable while focused, without turning it into a permanent top-layer obstruction.

## Config and actions

### Config

```ini
zone=name:left,x:0%,y:0%,w:50%,h:100%
zone=name:right,x:50%,y:0%,w:50%,h:100%
defaultzone=current
```

`zone=` defines a named rectangle as percentages of the monitor usable area.

`defaultzone` controls assignment for new/unset clients:

- `current`: inherit the selected client's zone if possible, otherwise use the first zone
- `<name>`: use that named zone
- invalid/missing: use the first configured zone

### Actions

```ini
bind=SUPER,bracketleft,focuszone,left
bind=SUPER+SHIFT,bracketleft,movetozone,left
```

- `focuszone zone|zone...`: cycle focus through clients in one or more zones. If the focused client is already in the requested zone set, cycle from it; otherwise start with the top matching client.
- `movetozone zone`: assign the selected client to a zone

`setlayout,zones` and `switch_layout` now run the layout transition logic described above.

## Why zones is still a tiling layout

`zones` can create overlapping windows, but it is still a tiling layout because the layout owns tiled-client geometry.

A fullscreen/monocle-style layout is the simplest comparison: every tiled client is assigned to the same full-monitor rectangle, so all tiled clients geometrically overlap. It is still a tiling layout because the layout computes that rectangle and applies it to tiled clients.

`zones` generalizes this from one implicit full-screen rectangle to multiple user-defined rectangles. If the user configures overlapping zones, then tiled clients assigned to those zones overlap by design, but their geometry is still layout-managed.

Floating/docked floating clients are separate: they retain user-controlled dimensions but may be anchored/aligned to a layout zone.

## Trade-offs and open questions

- Clearing zones on layout exit avoids stale state, but means returning to zones recomputes assignments from current geometry.
- Geometry-based matching is intuitive but still heuristic; ambiguous overlaps may need manual `movetozone` correction.
- Docked floating windows intentionally blur layout anchoring and floating sizing.
- The current stacking policy avoids a new public `togglefloatabove` action, but a future explicit stack mode (`normal`, `top`, `overlay`) may be cleaner.
- Docking is currently zones-specific. The same concept could later be generalized to dock floating windows to layout regions in other layouts.
