# Changelog

## v1.4.3

- X/O markers no longer show a darker patch where their own strokes crossed/overlapped — the X is now built from non-overlapping pieces instead of two crossing bars laid on top of each other, and the O is smoother (more segments)
- Added a Marker Size setting, so markers can be scaled up or down independent of everything else

## v1.4.2

- X/O markers are drawn shapes again, not text — the "text glyph" fix from the last update looked like a stray font character instead of an actual X/O, so they're back to real filled shapes, just drawn in a way that actually respects opacity this time
- The opacity slider kept rendering in the wrong spot (up near the color row) no matter where it was positioned — it turned out to be GD's native slider widget, which isn't built to be repositioned freely. Swapped it for Geode's own slider widget, which behaves normally
- Both the settings popup and the color picker are shorter now instead of stretching taller than they need to
- Color picker is laid out sideways (wheel + live preview swatch next to each other) instead of stacked in one tall column

## v1.4.1

- Fixed the settings popup being taller than the screen, cutting off the title and top row
- X/O markers no longer look fully opaque once placed regardless of the opacity setting — they're drawn as text glyphs now instead of hand-drawn shapes, which respects opacity properly
- The color picker now shows a live preview swatch, instead of only finding out what you picked after closing it

## v1.4.0

- Marker color now opens GD's own color wheel instead of cycling between just red and green
- Removed "clear markers every retry" — replaced with a "Clear All Markers" button you press whenever you actually want to wipe them
- Fixed the opacity slider visually overlapping the row above it

## v1.3.1

- Fixed the settings popup text rendering broken — GD's built-in fonts don't have Korean glyphs, so anything Korean in there was drawing as blank space. Popup text is English now.
- Marker shape/color pickers are now a left/right cycle instead of one button per option

## v1.3.0

- The pause-menu button now opens a settings popup instead of toggling on/off directly
- Marker shape can now be your character icon, an X, or an O
- X/O markers can be red or green
- Marker opacity is now adjustable from the popup, not just the settings page
- Added a "keep only the last N markers" option, so old ones fall off instead of piling up forever

## v1.2.0

- Fixed the mod ID (it wasn't in a format the index would accept)
- Small wording tweaks here and there

## v1.1.0

- Death markers are now a real copy of your character instead of a generic dot — they show your actual icon, colors, vehicle (ship/ball/UFO/wave/robot/spider/swing), size, gravity, angle, and facing direction, exactly as you died
- Removed the noclip-through-hazards behavior — the mod now marks your real deaths instead of ghosting through obstacles
- Fixed a bug where the level's spawn point could get falsely marked as a death every single attempt
- Fixed the Death Stamp ON/OFF button in the pause menu sometimes not responding to clicks
- New icon

## v1.0.0

- Initial release
