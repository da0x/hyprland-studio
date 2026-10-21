# Lantern for Hyprland

> Lantern is written with AI coding agents, under the direction and review of its
> human author.

A visual configuration and management app for [Hyprland](https://hypr.land). It is made
for Hyprland alone: it talks to Hyprland's own sockets and reads and writes its Lua config.

See what your compositor is doing, browse every option and keybinding, try changes
live, and save them safely, all without giving up control of your configuration files.

> Lantern for Hyprland is an independent open-source project for configuring and managing
> Hyprland. It is not affiliated with or endorsed by the Hyprland project.

## Mission

Make Hyprland easier to understand and configure, in the least intrusive way possible.
Three promises carry everything else:

- **It never changes your system behind your back.** It installs nothing, runs nothing
  with `sudo`, and touches no file you have not approved. When an outside program would
  help, like a status bar, a dock or a notification daemon, it names an existing,
  well-known open-source one, tells you exactly what to install and why, and lets you
  install it yourself.
- **You make the decisions.** It shows what it finds, explains the options and what each
  would change, and may recommend one, but it never picks for you. A keybinding conflict
  is reported, not fixed; a suggestion stays a suggestion until you accept it.
- **You can leave at any time, and leave nothing behind.** Every change it makes to your
  config is plain Hyprland configuration, with no markers, no extra files and no `require`
  lines of its own, so your config keeps working exactly the same without it. Everything
  Lantern keeps for itself (its settings, and a backup of every file it changed)
  lives in one folder, `~/.config/lantern-hyprland/`. To stop using it, uninstall the
  package and delete that folder, or let Lantern delete it for you first.

If you ever wonder "what did this do to my system?", that is a bug.

## What it does

Each page does one thing:

- **Theme:** one palette for the whole desktop: window borders, the bar, the app
  launcher, notifications, GTK apps and Lantern itself.
- **Wallpaper:** pick a picture, see it on the desktop at once, and keep it with
  hyprpaper or awww.
- **Windows:** corners, gaps, border thickness, and whether dragging a border resizes,
  with a preview of your screen. The bar's corners and gaps follow the windows'.
- **Bar:** a Waybar style that matches the theme, and a switch to start or stop it.
- **Terminal:** a colour scheme and font for kitty, wofi's and dunst's colours to match.
- **Accessibility:** text size, contrast, colour blindness, motion, and cursor size.
- **Keyboard shortcuts:** every binding, what it does, where in your config it is set,
  and which keys are bound more than once, with a search.
- **Options:** every Hyprland option with its value, default and description, and which
  ones your config sets or changes.
- **Live tree:** a map of your monitors and their windows, and the tree of monitors,
  workspaces and windows, kept current as things change.
- **Event log:** what Hyprland reports as it happens, said in words.
- **Activate:** applies Lantern's effects to the running desktop, in the areas you
  allowed; **Deactivate** puts everything back.
- **Console:** asks Hyprland what it knows, like `hyprctl`, but only with requests
  that read.

## Principles

- **You decide every change.** No package installs, no `sudo`, no replaced or moved
  files, no new autostart entries, nothing outside what you approved in a diff. Every
  change is recorded with a copy of the files it replaced, and can be undone.
- **Your config file is the source of truth.** Lantern reads and writes your own
  `hyprland.lua`. It keeps no database and no hidden copy, only its own window
  settings. Stop using it and you lose nothing.
- **Edits are lossless.** When it changes a value, it changes only that value. Your
  comments, formatting, `require` calls and anything it does not understand are left
  exactly as they were.
- **Safe before clever.** Activate never writes to your config: what it changes lives in
  the running session, only in areas you allowed, and one reload or one click on
  Deactivate undoes it. The pages that do write, like Theme, Windows and Bar, show every
  change as a diff first, back up what they replace, and put it back if Hyprland rejects
  it.

## What it reads and writes

**Reads:** your Hyprland config folder (following links), Hyprland's IPC sockets in
`$XDG_RUNTIME_DIR/hypr/`, the running instance's `hyprland.lock` and command line (to
find which config it loaded), and, if present, Hyprland's API stubs in
`/usr/share/hypr/stubs/`. For the Bar page it looks for Waybar on your `PATH`, asks
fontconfig whether the icon font is installed, finds the config Waybar would read, and
reads `/etc/os-release` to name the right install command. The Wallpaper page looks for
images in the Wallpapers folder of your pictures folder (from `~/.config/user-dirs.dirs`,
or `~/Pictures`), the pictures folder itself, `~/.local/share/backgrounds`,
`/usr/share/hypr` and `/usr/share/backgrounds`, and reads `~/.config/hypr/hyprpaper.conf`,
or, with awww, asks `awww query` which image it shows. Its Add from files button opens your
desktop's own file chooser, through xdg-desktop-portal. Its Copy button runs
`wl-copy`, from wl-clipboard, if you have it, since the toolkit offers no clipboard of its
own. The Terminal page and the Console read kitty's `kitty.conf` and `current-theme.conf`,
the Console to draw its replies in kitty's colours.

**Writes:** files you approved after reviewing the change line by line, such as Waybar's
config and stylesheet, the wallpaper in `hyprpaper.conf`, the lines in your Hyprland
config that start the wallpaper program at login and turn off Hyprland's own wallpaper, the window border colours in its
`hl.config` (only those values, nothing else in the file), the colours in
`~/.config/hypr/hyprtoolkit.conf`, wofi's and dunst's colours (below), and
from the Terminal page, kitty's `current-theme.conf` and the line in `kitty.conf` that
includes it, wofi's `style.css`, dunst's `dunstrc` (a first one starts
as a copy of the system's); from the Accessibility page, `animations.enabled` and
`decoration.screen_shader` in `hl.config` (added in a call of their own at the end when the
config does not set them), the `XCURSOR_SIZE` and `HYPRCURSOR_SIZE` lines, a colour-correction
shader in `~/.config/hypr/shaders/`, and the text sizes in `hyprtoolkit.conf`; and, outside
any file, GTK's `color-scheme` and `accent-color`, and the desktop's `high-contrast`,
`reduced-motion`, `enable-animations`, `text-scaling-factor` and `cursor-size`, through
`gsettings`, whose old values are kept with the change so Undo can restore them (if your config is a link, the file it
points at, which the review names); an image you add from files, copied into your
wallpapers folder under a new random name (a UUID, with the image's own extension); and its own folder, `~/.config/lantern-hyprland/`: its settings
(window size, last page, which areas you allowed, whether you dismissed a page's checklist), the effects you set up (plain Hyprland
Lua you can read and copy into your own config, in `effects.lua`), while anything is active a
record of what is (`activation.json`), so Deactivate can take it back even after Lantern
was closed, a record in `backups/` of each change it
wrote, with copies of the files it replaced, so any change can be undone, and, only while
the desktop-theme area is active, a record of the theme values it changed so they can
always be put back, even after a crash. No caches, no exports: copying goes to the
clipboard. A menu item removes that folder for you.

**Starts and stops Waybar, hyprpaper and awww** only when you press their On or Off switch,
on the Bar and Wallpaper pages. While one is on, Lantern restarts it after writing its
files, so it reads them. Applying a wallpaper makes the chosen program the only one: it
turns the other off and, as part of the change you review, starts the chosen one at login
instead of the other. Picking a wallpaper shows it on the desktop straight away, through
`hyprctl hyprpaper` or `awww img`, without writing anything; leaving the page puts back the
one applied, unless you apply the new one. awww remembers its image itself, so for awww
Lantern keeps only the fit you chose, in its settings. After a change to your Hyprland
config, Lantern asks Hyprland for config errors and puts the files back if it reports
any.

**Takes its colours** from hyprtoolkit, as every app built on it does: from
`hyprtoolkit.conf`, or hyprtoolkit's own defaults. When the Theme page applies a palette,
Lantern changes to it at once, without closing.

**Changes while active, only in areas you allowed:** the running Hyprland's options, rules
and keybindings (through IPC; a reload undoes them), the programs it started for you, and
your desktop theme settings.

Reading your config never runs it. Lantern reads Lua configs in a sandbox of its own,
where commands, file writes and startup programs are recorded but never carried out.

## Status

0.1. Every page above works.

It needs:

- **Hyprland 0.56.** It is tested with 0.56.2. Hyprland changes its config and its IPC
  often, so a newer release may need a newer Lantern.
- **A Lua config** (`hyprland.lua`). Lantern reads the older `hyprland.conf` too, but
  changes only a Lua config so far.

Research is in [`docs/research.md`](docs/research.md), the design in
[`docs/architecture.md`](docs/architecture.md), the gaps in Hyprland's IPC in
[`docs/upstream.md`](docs/upstream.md), and the work is planned on
[neotrac.org/hyprland-studio](https://neotrac.org/hyprland-studio).

## Roadmap

1. **0.1, see everything, and change the look safely.**
   - A keybinding browser with search and conflict detection, an option browser with
     documentation and "changed from default" highlighting, a live tree of monitors,
     workspaces and windows, an event log, and a read-only console.
   - Theme, wallpaper, windows, bar, terminal and accessibility pages that write your
     files only after you review the change, each with Undo.
   - **Activate** applies Lantern's effects to your running desktop: Hyprland options
     and rules, keybindings, and outside programs you installed (a bar, a dock).
     **Deactivate** puts everything back exactly as it was. Each area has its own "allow"
     switch, off until you turn it on.
2. **0.2, make it yours.**
   - Themes you author yourself, next to the built-in ones.
   - More widgets for the bar.

After that: changing any option from the Options page and saving it to your config,
keeping an activation across restarts (opt-in), an editable monitor layout, keybinding and
window-rule editors, an animation and bezier editor, and writing the older
`hyprland.conf` format.

## Building

Lantern is written in C++26 on [hyprtoolkit](https://github.com/hyprwm/hyprtoolkit),
Hyprland's own Wayland-native toolkit, and builds with CMake. On Arch Linux, the packages it
needs are:

```
sudo pacman -S --needed base-devel cmake ninja hyprtoolkit lua tree-sitter tree-sitter-lua nlohmann-json fontconfig systemd-libs gtest
```

Then, from the repository:

```
make                  # build into build/release
make test             # run the tests
sudo make install     # install under /usr/local
sudo make uninstall   # remove what make install put there
```

`make install PREFIX=~/.local` installs for your user only, without `sudo`.

hyprtoolkit 0.6.0 has two bugs Lantern runs into. Its scroll area crashes the app, so
with 0.6.0 the theme and wallpaper cards move a row at a time with the mouse wheel instead
of scrolling smoothly. And text that gets narrower can leave a piece of its old self past
its edge, which flickers as the mouse moves. Both are fixed in
[da0x/hyprtoolkit](https://github.com/da0x/hyprtoolkit)'s `main` until a hyprtoolkit
release has them. Build that, install it somewhere of its own, and point Lantern at it:
`make HYPRTOOLKIT_PREFIX=/where/it/is/installed`.

`make install` builds nothing itself, so run `make` as yourself first. Packagers can use CMake directly:
the build takes the usual `CMAKE_BUILD_TYPE`, `CMAKE_INSTALL_PREFIX` and `DESTDIR`.

## Acknowledgements

Lantern learned from [Noctalia](https://github.com/noctalia-dev/noctalia), an
MIT-licensed Wayland shell by noctalia-dev and contributors: in particular its direct use
of Hyprland's sockets, and its rule of never managing the user's compositor config. No
Noctalia code is included.

The Terminal page's themes are kitty's own colour schemes from
[kitty-themes](https://github.com/kovidgoyal/kitty-themes) (GPL-3.0), bundled unchanged in
`src/desktop/terminal-themes/`, each with its author and, for most, its MIT licence in its
header.

## License

GPL-3.0-or-later. See [`LICENSE`](LICENSE).

`SPDX-License-Identifier: GPL-3.0-or-later`
