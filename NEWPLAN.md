# Plan: zones docking and stacking cleanup

## Goals

- Keep the zones drag-to-zone behavior.
- Treat a floating window assigned to a zone as a docked floating window: it keeps its own size, but is associated with the zone.
- Remove the separate `togglefloatabove` user-facing action/state before it becomes API.
- Keep `toggleoverlay` as the explicit always-on-top mechanism.
- Make docked floating windows stack with normal tiled content by default, so oversized docked windows do not obscure overlapping tiles unless explicitly overlayed.

## Short-term implementation

1. Remove `isfloatabove` from `Client` and initialization.
2. Remove `client_should_float_above()` and restore the simpler layer rules.
3. Remove `togglefloatabove` declaration, parser entry, implementation, and default keybind.
4. Add a zones helper for identifying docked floating windows.
5. For layer placement:
   - `isoverlay` -> `LyrOverlay`
   - fullscreen -> `LyrTop`
   - floating + not docked -> `LyrTop`
   - floating + docked -> `LyrTile`
   - tiled -> `LyrTile`
   - XWayland `_NET_WM_STATE_ABOVE` remains top-layer behavior.
6. Ensure drag-to-zone assigns the zone but preserves the user's/configuration's floating-vs-tiled choice. Tiled clients may use temporary floating geometry while dragging, but must be tiled again after drop.
7. On layout transitions:
   - leaving `zones` clears visible clients' zone assignments
   - entering `zones` force-reassigns visible tiled clients from their current geometry/best zone match
   - new windows continue to use `defaultzone`, with `current` inheriting the selected client's zone

## Follow-up design

- Generalize docking beyond zones: a floating window could be anchored to a layout region/tile while retaining independent size.
- Replace boolean stacking flags with a single stack mode if more control is needed:
  - `normal`
  - `top`
  - `overlay`
- Consider config defaults such as:
  - `floating_stack_mode=top`
  - `docked_floating_stack_mode=normal`
