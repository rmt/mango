---
title: Layouts
description: Configure and switch between different window layouts.
---

## Supported Layouts

mangowm supports a variety of layouts that can be assigned per tag.

- `tile`
- `scroller`
- `monocle`
- `grid`
- `deck`
- `center_tile`
- `vertical_tile`
- `right_tile`
- `vertical_scroller`
- `vertical_grid`
- `vertical_deck`
- `dwindle`
- `fair`
- `vertical_fair`
- `zones`

---

## Scroller Layout

The Scroller layout positions windows in a scrollable strip, similar to PaperWM.

### Configuration

| Setting | Default | Description |
| :--- | :--- | :--- |
| `scroller_structs` | `20` | Width reserved on sides when window ratio is 1. |
| `scroller_default_proportion` | `0.9` | Default width proportion for new windows. |
| `scroller_focus_center` | `0` | Always center the focused window (1 = enable). |
| `scroller_prefer_center` | `0` | Center focused window only if it was outside the view. |
| `scroller_prefer_overspread` | `1` | Allow windows to overspread when there's extra space. |
| `edge_scroller_pointer_focus` | `1` | Focus windows even if partially off-screen. |
| `edge_scroller_focus_allow_speed` | `0.0` | Allow pointer focus to happen if the pointer moves at a speed greater than this value. |
| `scroller_proportion_preset` | `0.5,0.8,1.0` | Presets for cycling window widths. |
| `scroller_ignore_proportion_single` | `1` | Ignore proportion adjustments for single windows. |
| `scroller_default_proportion_single` | `1.0` | Default proportion for single windows in scroller. **Requires `scroller_ignore_proportion_single=0` to take effect.** |

> **Warning:** `scroller_prefer_overspread`, `scroller_focus_center`, and `scroller_prefer_center` interact with each other. Their priority order is:
>
> **scroller_prefer_overspread > scroller_focus_center > scroller_prefer_center**
>
> To ensure a lower-priority setting takes effect, you must set all higher-priority options to `0`.

```ini
# Example scroller configuration
scroller_structs=20
scroller_default_proportion=0.9
scroller_focus_center=0
scroller_prefer_center=0
scroller_prefer_overspread=1
edge_scroller_pointer_focus=1
edge_scroller_focus_allow_speed=0.0
scroller_default_proportion_single=1.0
scroller_proportion_preset=0.5,0.8,1.0
```

---

## Master-Stack Layouts

These settings apply to layouts like `tile` and `center_tile`.

| Setting | Default | Description |
| :--- | :--- | :--- |
| `new_is_master` | `1` | New windows become the master window. |
| `default_mfact` | `0.55` | The split ratio between master and stack areas. |
| `default_nmaster` | `1` | Number of allowed master windows. |
| `smartgaps` | `0` | Disable gaps when only one window is present. |
| `center_master_overspread` | `0` | (Center Tile) Master spreads across screen if no stack exists. |
| `center_when_single_stack` | `1` | (Center Tile) Center master when only one stack window exists. |

```ini
# Example master-stack configuration
new_is_master=1
smartgaps=0
default_mfact=0.55
default_nmaster=1
```

---

## Dwindle Layout

The Dwindle layout arranges windows as a binary tree of recursive splits. Each new window splits the focused window's container, producing a spiral-like tiling.

### Configuration

| Setting | Default | Description |
| :--- | :--- | :--- |
| `dwindle_split_ratio` | `0.5` | Ratio used for new splits (`0.05`–`0.95`). |
| `dwindle_smart_split` | `0` | Pick the split axis from the cursor's position inside the focused window. The new window appears on the cursor's side. |
| `dwindle_hsplit` | `1` | Side-by-side splits: where the new window goes. `0` = follow cursor, `1` = right, `2` = left. |
| `dwindle_vsplit` | `1` | Top/bottom splits: where the new window goes. `0` = follow cursor, `1` = below, `2` = above. |
| `dwindle_preserve_split` | `0` | Keep the sibling's split orientation when a window is closed. |
| `dwindle_smart_resize` | `0` | When dragging to resize, move the split toward the cursor regardless of which side was grabbed. |
| `dwindle_drop_simple_split` | `1` | Drag-to-tile drop preview. `1` = 2-zone preview matching `dwindle_split_ratio`, `0` = 4-quadrant preview. |
| `dwindle_manual_split` | `0` | Manually split windows mode. |

```ini
# Example dwindle configuration
dwindle_split_ratio=0.5
dwindle_smart_split=0
dwindle_hsplit=0
dwindle_vsplit=0
dwindle_preserve_split=0
dwindle_smart_resize=0
dwindle_drop_simple_split=1
```

---

## Zones Layout

The Zones layout places tiled windows into named rectangular regions of the usable monitor area. Multiple windows may share a zone, and configured zones may overlap.

### Configuration

```ini
zone=name:left,x:0%,y:0%,w:50%,h:100%
zone=name:right,x:50%,y:0%,w:50%,h:100%
defaultzone=current
```

Each `zone=` entry requires `name`, `x`, `y`, `w`, and `h`. Coordinates and sizes are percentages of the usable monitor area and may include a trailing `%`. If no valid zones are configured, mangowm installs default `left` and `right` zones.

`defaultzone` controls where new or unassigned windows are placed:

- `defaultzone=current` inherits the selected window's zone when possible, then falls back to the first zone.
- `defaultzone=<name>` selects that configured zone.
- An invalid or missing default falls back to the first configured zone.

### Layout Transitions

When entering `zones`, visible windows are assigned from their current geometry before the layout resizes them. Selection prefers the greatest intersection, then the smallest matching zone, then the zone with the lowest tiled occupancy. Clients that were fullscreen or maximized during the first pass are assigned after those states are cleared.

When leaving `zones`, visible clients' zone assignments are cleared. Returning to the layout therefore recomputes placement from current geometry rather than restoring stale assignments.

### Commands

```ini
bind=SUPER,bracketleft,focuszone,left
bind=SUPER+SHIFT,bracketleft,movetozone,left
bind=SUPER,bracketright,focuszone,right
bind=SUPER+SHIFT,bracketright,movetozone,right
```

`focuszone` focuses or cycles through visible clients assigned to a zone. It accepts a `|`-separated list such as `left|right`.

`movetozone` assigns the selected window to a named zone:

- Tiled clients are resized by the zones layout. If necessary, the current tag enters `zones` through the normal layout-transition path.
- Floating clients retain their size and are aligned within the target zone.

### Docked Floating Windows

A floating client with a valid zone assignment is treated as a docked floating window. It remains floating and keeps its dimensions, but is aligned to its zone and stacked with tiled content. Use `toggleoverlay` when it should remain explicitly above other windows.

When focus moves within the same zone, the previous docked floating window may be lowered to reveal the newly focused client. Focus changes to another zone leave it in place, allowing each zone to retain its visible top window.

Turning a client floating while `zones` is active aligns it to its current or default zone.

### Drag and Drop

Dragging a tiled client in `zones` shows the best-overlap zone as a drop preview. The client is temporarily floated while moving, but dropping restores its tiled state and prior floating geometry.

Docked floating clients can also be redocked:

- Dropping within the same zone preserves the new manual position.
- Dropping onto another zone updates the assignment and aligns the window there.
- Dropping outside all zones snaps back to the current or default zone.
- The client remains floating in all cases.

### Monitor Changes

When an output is temporarily disconnected, mangowm remembers its active tag state and restores it when the output returns. Zone-assigned floating clients are realigned when monitor ownership, geometry, or usable area changes.

---

## Switching Layouts
| Setting | Default | Description |
| :--- | :--- | :--- |
| `circle_layout` | - | A comma-separated list of layouts `switch_layout` cycles through,the value sample:`tile,scroller`. |

You can switch layouts dynamically or set a default for specific tags using [Tag Rules](/docs/window-management/rules#tag-rules).

**Keybinding Examples:**

```ini
# Cycle through layouts
circle_layout=grid,scroller,tile
bind=SUPER,n,switch_layout

# Set specific layout
bind=SUPER,t,setlayout,tile
bind=SUPER,s,setlayout,scroller
```
