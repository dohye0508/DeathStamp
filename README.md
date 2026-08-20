# Death Stamp

A [Geode](https://geode-sdk.org/) mod for Geometry Dash that lets you test your own custom levels without dying — and automatically marks every spot you *would* have died, so you can see exactly where your level is too hard.

> Built for level design iteration, not for cheating on rated/verified runs. See [Disclaimer](#disclaimer).

## What it does

While playing your own level in Normal Mode, Death Stamp intercepts every death and:

1. Lets you pass straight through the hazard instead of resetting the level (noclip)
2. Drops an X-shaped marker at the exact point of impact
3. Keeps a debounced edge-trigger so overlapping the same hazard for several frames only stamps once

Run the level a few times with markers accumulating (instead of clearing each attempt) and you get a rough death heatmap of your own level — no guesswork about which section is actually the hard one.

A toggle button is also added to the pause menu (Esc), so you can flip it on/off mid-playtest without leaving the level.

## Installation

1. Download `DeathStamp.geode` from [Releases](../../releases)
2. Drop it into your Geode mods folder: `<Geometry Dash>/geode/mods/`
3. Launch the game and enable **Death Stamp** from the mod list

## Usage

1. Open your level in the editor and start a Normal Mode playtest
2. Enable the mod from its settings popup, or toggle it live from the pause menu (**Esc → Death Stamp: ON/OFF**)
3. Play through — hazards no longer kill you, but each one gets stamped
4. Repeat a few runs to build up a heatmap of the roughest sections
5. **Turn it off before doing an actual verify run** — Death Stamp intentionally bypasses the game's normal death handling, so it does not (and should not) count as a legitimate completion

## Settings

| Setting | Type | Default | Description |
|---|---|---|---|
| `enabled` | bool | `false` | Master switch for noclip + stamping |
| `noclip` | bool | `true` | If off, you still die normally — only the stamp is recorded |
| `stamp-cooldown` | float | `0.25` | Minimum seconds between stamps, so lingering on one hazard doesn't spam markers |
| `marker-opacity` | int | `160` | Opacity of the death markers (0–255) |
| `clear-on-new-attempt` | bool | `false` | Clear markers on every retry instead of accumulating a heatmap |

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

## Disclaimer

Death Stamp is a level-design testing tool for your own creations. It deliberately skips the game's death handling while noclip is active, so:

- Levels played with it enabled are **not** legitimately verified
- Always turn it off and do a clean run before claiming a level as verified
- Not intended for use on other people's levels, ranked/leaderboard play, or anything beyond your own private testing

## License

MIT
