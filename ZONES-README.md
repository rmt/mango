# Zones layout

`zones` is a tiling layout that places clients into named rectangular regions of the monitor work area. A zone is configured as a percentage rectangle, and any number of windows may be assigned to the same zone. Windows assigned to the same or overlapping zones intentionally overlap.

## Basic configuration

```ini
# Two default-style zones
zone=name:left,x:0%,y:0%,w:50%,h:100%
zone=name:right,x:50%,y:0%,w:50%,h:100%
defaultzone=current

bind=SUPER,z,setlayout,zones
bind=SUPER,bracketleft,focuszone,left
bind=SUPER+SHIFT,bracketleft,movetozone,left
bind=SUPER,bracketright,focuszone,right
bind=SUPER+SHIFT,bracketright,movetozone,right
```

Each `zone=` entry requires:

- `name`
- `x`
- `y`
- `w`
- `h`

Coordinates and sizes are percentages of the usable monitor area. If no zones are configured, Mango installs default `left` and `right` zones.

## Default zone behavior

```ini
defaultzone=current
```

`defaultzone` controls where new or otherwise unassigned windows go while the `zones` layout is active.

- `defaultzone=current`: inherit the currently selected window's zone when possible; otherwise use the first configured zone.
- `defaultzone=<name>`: assign new/unset windows to that named zone.
- invalid or missing default zone: fall back to the first configured zone.

## Layout transitions

When switching **to** `zones`, visible windows are assigned zones using their current geometry before the layout resizes them.

Best-zone selection currently prefers:

1. greatest intersection with the window's current geometry
2. smallest zone area when overlap ties, so a window fully inside a corner zone should prefer that corner over a larger half-screen zone
3. lowest current tiled occupancy as a final tie-break

When switching **away from** `zones`, visible clients' zone assignments are cleared. This prevents stale zone state from unexpectedly affecting later layouts.

## Actions

### `focuszone`

```ini
bind=SUPER,bracketleft,focuszone,left
bind=SUPER,space,focuszone,left|right
```

Focuses/cycles through visible clients assigned to one or more zones. Multiple zones may be separated with `|`.

### `movetozone`

```ini
bind=SUPER+SHIFT,bracketleft,movetozone,left
```

Assigns the selected client to the named zone.

- tiled clients are resized by the zones layout
- floating clients keep their own size and are aligned to the zone
- if the current layout is not `zones`, moving a tiled client to a zone switches the tag to the `zones` layout

## Floating and docked floating windows

Floating windows can be assigned to zones. A floating window with a valid zone assignment is treated as a **docked floating** window:

- it remains floating
- it keeps its own size
- it is aligned to its assigned zone
- it is stacked with normal tiled content unless `toggleoverlay` is set

This is useful for large or irregular windows inside a zone. If a docked floating window is larger than its zone, it may overlap neighboring zones, but it should not permanently obscure overlapping tiled windows unless it is focused or explicitly overlayed.

When a client is turned floating while in `zones`, for example via `togglefloating`, Mango explicitly aligns it to its current/default zone.

Dragging a tiled window while in `zones` reassigns it to the zone under the cursor instead of performing the usual drag-to-tile insertion used by layouts such as scroller or dwindle. The drag uses temporary floating geometry internally, but dropping the window restores its original tiled state; dragging must not change whether the window is floating.

## Overlay behavior

`toggleoverlay` remains the explicit always-on-top mechanism. Overlay clients are placed in the overlay layer regardless of whether they are tiled, floating, or docked floating.

The experimental separate `togglefloatabove` idea was removed from this plan because floating geometry and stacking policy should not be exposed as independent ad-hoc booleans. The current model is:

- normal tiled clients: tiled layer
- undocked floating clients: top layer, as before
- docked floating clients: tiled layer by default
- overlay clients: overlay layer

A future cleanup may replace this with an explicit stack mode such as `normal`, `top`, and `overlay`.

## Why this is still a tiling layout

Although zones can overlap, `zones` is still a tiling layout because the layout owns the placement of tiled clients. Each tiled client is assigned to a configured layout region, and `arrange()` resizes it to that region.

This is similar in spirit to fullscreen/monocle layouts: those layouts assign every tiled client to the same full-monitor rectangle, so all tiled clients geometrically overlap. They are still tiling layouts because the layout, not the user, computes the tiled clients' geometry.

`zones` generalizes that idea from one implicit full-screen rectangle to multiple user-defined rectangles. If the user defines overlapping zones, then tiled clients assigned to those zones overlap by design, but their geometry is still produced by the layout.

Floating/docked floating windows are different: they retain user-controlled size, but may be anchored/aligned to a zone for convenience.

## Trade-offs

- Overlapping zones are powerful but can be confusing if zone definitions are too broad or ambiguous.
- Best-zone matching uses geometry heuristics; unusual layouts may still need manual `movetozone` correction.
- Docked floating windows blur the line between tiling and floating: they are anchored to a layout region but keep their own size.
- Stacking is intentionally conservative: docked floating windows are not automatically always-on-top, so oversized docked windows do not permanently hide adjacent tiled windows.
- Clearing zone assignments when leaving `zones` avoids stale state, but it also means returning to `zones` recomputes assignments from current geometry rather than preserving old zone choices.
