# Changelog

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
