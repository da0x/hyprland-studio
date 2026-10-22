# Lantern for Hyprland

Lantern for Hyprland is a settings app for Hyprland that makes your desktop easier to
see, read and use, and then gets out of the way.

**Accessibility first.** Make text larger, raise the contrast, correct the colours for
colour blindness across the whole desktop, calm the animations, and make the cursor easier
to find. Each one is set where your desktop already looks for it, so GTK apps, Hyprland
and the apps on top of it follow along.

**It never takes over.** Lantern installs nothing and runs nothing with `sudo`. It shows
every change as a diff before writing it, keeps a copy of what it replaced, and undoes it
in one click. What it writes is plain Hyprland, Waybar and GTK configuration, with no
markers and nothing of its own, so your desktop works the same if you stop using it.

**One look, everywhere.** Pick a theme and your window borders, Waybar, the launcher,
notifications and GTK apps all change together, and your terminal can match. Pick a
wallpaper and see it at once. Your bar follows your windows' corners, gaps and borders.

It also shows you what Hyprland is doing: every option with its documentation, every
keybinding and where it is set, a live map of your monitors and windows, an event log, and
a console that only reads. It is made for Hyprland alone, talking to Hyprland's own sockets
and reading and writing its Lua config.

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
  launcher, notifications, GTK apps and Lantern itself, shown on a picture of your
  desktop as it would look.
- **Wallpaper:** pick a picture, see it on the desktop at once, and keep it with
  hyprpaper or awww.
- **Windows:** corners, gaps, border thickness, and whether dragging a border resizes,
  with a preview of your screen. The bar's corners and gaps follow the windows'.
- **Displays:** each display's resolution, refresh rate, scale, rotation, variable
  refresh rate and place beside the others, and whether apps made for X11 stay sharp
  when a display is scaled. A change is put back after 15 seconds unless
  you keep it, so a display that goes dark fixes itself.
- **Keyboard and mouse:** keyboard layouts and the keys that switch them, what Caps Lock
  does, how held keys repeat, Num Lock at start, the pointer's speed, acceleration,
  buttons and scroll direction, and the touchpad's tap to click, right click, pause while
  typing, scrolling and three-finger swipe between workspaces.
- **Bar:** a Waybar style that matches the theme, with a border like your windows', the
  widgets you choose on the left, centre or right, such as the weather, media and
  processor, and a switch to start or stop it.
- **Terminal:** a colour scheme, font, text size and background opacity for kitty.
- **Cursors:** the mouse cursor's theme, each shown by its own cursors, and the
  distribution's with the command that installs one.
- **Accessibility:** text size, contrast, colour blindness, motion, and cursor size.
- **Essentials:** checks for what a Hyprland desktop needs beside Hyprland: the portals for
  screen sharing and file dialogs, a polkit agent for password prompts, a notification
  daemon, fonts for text and emoji, and Qt's Wayland support. Each one missing shows why it
  matters and the command that installs it; Lantern installs nothing. A polkit agent that
  is installed but not running can be started, now and with Hyprland.
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

This is everything Lantern touches. Nothing else is read, written or started.

### Reads

- **Hyprland:** your config folder, following links, and the running Hyprland's
  sockets. To learn which config Hyprland loaded, it reads Hyprland's lock file and
  command line. If they are installed, it also reads Hyprland's Lua type stubs in
  `/usr/share/hypr/stubs/`.
- **Bar page:** whether Waybar is installed, whether its icon font is, which config
  Waybar would read, and `/etc/os-release`, to show the install command for your
  distribution.
- **Wallpaper page:** the pictures in your Wallpapers and Pictures folders,
  `~/.local/share/backgrounds`, `/usr/share/hypr` and `/usr/share/backgrounds`, and
  `hyprpaper.conf`, or which picture awww shows.
- **Terminal page and Console:** kitty's `kitty.conf` and `current-theme.conf`. For the
  preview of a see-through terminal, the Terminal page asks awww which picture it shows.
- **Displays page:** each display Hyprland knows, with the modes it offers, from
  `hyprctl monitors all`.
- **Keyboard and mouse page:** the keyboard layouts xkeyboard-config lists in
  `/usr/share/X11/xkb/rules/evdev.lst`, and whether udev has found a touchpad, from
  `/run/udev/data`.
- **Essentials page:** the portals in `/usr/share/xdg-desktop-portal/portals`, the D-Bus
  services in `/usr/share/dbus-1/services`, the polkit agents and Qt plugins installed,
  your running programs, and fontconfig's fonts.
- **Cursors page:** the cursor themes in your icon folders and the system's, and their
  cursor files, to draw them.
- **Its own colours:** `~/.config/hypr/hyprtoolkit.conf`, like every hyprtoolkit app.

Reading your config never runs it. Lantern reads it in a sandbox of its own, where
commands, file writes and startup programs are noted but never carried out.

### Writes

Only after you review the change line by line. When your config is a link, Lantern
writes the file it points to, and the review says which one.

- **Theme:** the window border colours in your Hyprland config, Waybar's files, the
  colours in `hyprtoolkit.conf`, wofi's `style.css` and dunst's `dunstrc` if you have
  them, a `gtk.css` for GTK 3 and one for GTK 4 that give GTK apps the palette, GTK 3's
  dark or light preference in `settings.ini`, and GTK's dark or light preference and
  accent colour in GNOME's settings.
- **Wallpaper:** `hyprpaper.conf`, and the lines in your Hyprland config that start the
  wallpaper program at login and turn off Hyprland's own wallpaper. A picture you add
  is copied into your wallpapers folder under a new random name.
- **Windows:** the corners, gaps, border size and resizing in your Hyprland config, and
  Waybar's files, so the bar keeps matching.
- **Displays:** the `hl.monitor` call for each display you change in your Hyprland
  config, only the values that changed. A display your config sets only through a rule
  for every display gets a call of its own, by its make, model and serial number, so it
  keeps its settings on another port. Keeping X11 apps sharp sets XWayland's
  `force_zero_scaling` and, at a scale of 2 or more, `GDK_SCALE`.
- **Keyboard and mouse:** the layouts, variants, xkb options, key repeat, Num Lock,
  pointer speed, acceleration, left-handed buttons, scroll direction and touchpad settings
  in your Hyprland config's `input` table, and the `hl.gesture` call for a three-finger
  swipe between workspaces. xkb options Lantern has no choice for are kept as they are.
- **Bar:** Waybar's `config.jsonc` and `style.css`, and rules in your Hyprland config that
  float the volume control and network settings the bar opens. The weather widget, when
  you turn it on, has Waybar ask the website wttr.in for the temperature every half hour,
  which lets that site see roughly where you are.
- **Terminal:** kitty's `current-theme.conf`, and the font, text size, opacity and theme
  lines in `kitty.conf`.
- **Cursors:** `XCURSOR_THEME` in your Hyprland config, `~/.icons/default/index.theme`,
  which X11 apps read, and GTK's cursor theme in GNOME's settings. Hyprland shows the new
  cursor at once.
- **Accessibility:** animations, the cursor size and the colour-correction shader in your
  Hyprland config, the shader itself in `~/.config/hypr/shaders/`, the text sizes in
  `hyprtoolkit.conf`, and the desktop's contrast, motion, text size and cursor size
  settings.
- **Essentials:** only when you start a polkit agent from it: a line in your Hyprland
  config's start that runs the agent after you log in.

Desktop settings are changed through `gsettings`, and their old values are kept so Undo
can restore them. After any change to your Hyprland config, Lantern asks Hyprland whether
it found errors, and puts the files back if it did.

### Its own folder

Everything Lantern keeps for itself is in `~/.config/lantern-hyprland/`, and a menu item
deletes it for you:

- `settings.json`: the window size, the last page, which areas you allowed, and
  checklists you dismissed.
- `effects.lua`: the effects Activate applies, as plain Hyprland Lua you can copy into
  your own config.
- `activation.json`: what is active right now, so Deactivate works even after Lantern
  was closed.
- `backups/`: a record of each change, with copies of the files it replaced, for Undo.

There are no caches and no exports. Copying goes to the clipboard.

### Starts and stops

- **Waybar, hyprpaper and awww,** only when you press their On or Off switch. If one is
  running when Lantern writes its files, Lantern restarts it so it reads them.
- **A wallpaper preview.** Picking a picture shows it straight away, without writing
  anything, and leaving the page puts back the one you applied. Applying makes the
  chosen program your only wallpaper program, at login too.
- **Helpers you already have:** `wl-copy` for the Copy buttons, and your desktop's own
  file chooser for adding pictures.

### Changes while active

Only in the areas you allowed on the Activate page: the running Hyprland's options, rules
and keybindings, which a reload undoes, the programs you allowed it to start, and your
desktop theme settings.

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
   - Widgets for the bar, each on the side you choose: the window title, weather, media,
     privacy, processor, memory, temperature, keep awake, Bluetooth and battery, as well
     as the workspaces, clock, sound, network and tray.
   - A cursor theme page, a see-through terminal, and GTK apps in the theme's colours.
   - The settings a plain Hyprland install still lacks: displays, keyboard and touchpad,
     idle and lock, night light, a check for the desktop's essentials, autostart and
     default apps.

After that: changing any option from the Options page and saving it to your config,
keeping an activation across restarts (opt-in), dragging displays into place, keybinding and
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

hyprtoolkit 0.6.0 has three bugs Lantern runs into. Its scroll area crashes the app, so
with 0.6.0 the theme and wallpaper cards move a row at a time with the mouse wheel instead
of scrolling smoothly. Text that gets narrower can leave a piece of its old self past its
edge, which flickers as the mouse moves. And a change of layout can wait until the window
is resized, so after you make the text smaller, the sidebar keeps its width. All three are
fixed in
[da0x/hyprtoolkit](https://github.com/da0x/hyprtoolkit)'s `main` until a hyprtoolkit
release has them. Build that, install it somewhere of its own, and point Lantern at it:
`make HYPRTOOLKIT_PREFIX=/where/it/is/installed`.

`make install` builds nothing itself, so run `make` as yourself first. Packagers can use CMake directly:
the build takes the usual `CMAKE_BUILD_TYPE`, `CMAKE_INSTALL_PREFIX` and `DESTDIR`.

## Acknowledgements

Lantern is written with AI coding agents, under my direction and review.

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
