# Death Stamp

A [Geode](https://geode-sdk.org/) mod for Geometry Dash that marks every spot you died in your own custom level, so you can see exactly where it's too hard.

## What it does

While playing your own level, Death Stamp intercepts every death and drops a frozen copy of your character — matching your colors, icon, vehicle (ship/ball/UFO/wave/robot/spider/swing), size, gravity, and angle at the moment of impact — pinned at the exact point you died, then lets the level reset as normal.

Play a few attempts with markers accumulating (instead of clearing each attempt) and you get a rough death heatmap of your own level — no guesswork about which section is actually the hard one.

A button in the pause menu (Esc) opens a settings popup, so you can adjust everything mid-playtest without leaving the level.

## Installation

1. Download `DeathStamp.geode` from [Releases](../../releases)
2. Drop it into your Geode mods folder: `<Geometry Dash>/geode/mods/`
3. Launch the game and enable **Death Stamp** from the mod list

## Usage

1. Open your level in the editor and start a Normal Mode playtest
2. Press **Esc** and tap the **Death Stamp** button to open the settings popup, or use the mod's settings page from the mod list
3. Turn it on, pick a marker style, and play — every death drops a marker at that spot
4. Repeat a few attempts to build up a heatmap of the roughest sections

## Settings

| Setting | Type | Default | Description |
|---|---|---|---|
| `enabled` | bool | `false` | Master switch for stamping |
| `marker-style` | string | `player` | `player` (your character icon), `x`, or `o` |
| `marker-size` | int | `100` | Scale of the markers, as a percentage |
| `marker-color-r/g/b` | int | `235`/`60`/`60` | Color for `x`/`o` markers — pick it visually via the popup's color wheel rather than editing these directly |
| `marker-opacity` | int | `160` | Opacity of the death markers (0–255) |
| `max-markers` | int | `0` | Only keep the most recent N markers; `0` keeps all of them |

All of these can be adjusted from the pause-menu settings popup, including a "Clear All Markers" button to wipe the current level's markers on demand.

## Building from source

Requires the [Geode CLI](https://docs.geode-sdk.org/getting-started/) and a Geode profile pointing at a Geometry Dash install (or any valid profile path — the build itself doesn't touch the game).

```bash
git clone https://github.com/dohye0508/DeathStamp.git
cd DeathStamp
geode build
```

Add `--install` to copy the built `.geode` straight into your active profile's mods folder:

```bash
geode build --install
```

## License

MIT
