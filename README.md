# Hyprland Studio

> Hyprland Studio is written with AI coding agents, under the direction and review of its
> human author.

A visual configuration and management studio for [Hyprland](https://hypr.land).

See what your compositor is doing, browse every option and keybinding, try changes
live, and save them safely, all without giving up control of your configuration files.

> Hyprland Studio is an independent open-source project for configuring and managing
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
  the studio keeps for itself (its window settings, and later its change log and backups)
  lives in one folder, `~/.config/hyprland-studio/`. To stop using it, uninstall the
  package and delete that folder, or let the studio delete it for you first.

If you ever wonder "what did this do to my system?", that is a bug.

## Principles

- **You decide every change.** No package installs, no `sudo`, no replaced or moved
  files, no new autostart entries, nothing outside what you approved in a diff. Once
  the studio can write, every change is recorded in a log you can read and undo.
- **Your config file is the source of truth.** Hyprland Studio reads and writes your own
  `hyprland.lua`. It keeps no database and no hidden copy, only its own window
  settings. Stop using it and you lose nothing.
- **Edits are lossless.** When it changes a value, it changes only that value. Your
  comments, formatting, `require` calls and anything it does not understand are left
  exactly as they were.
- **Safe before clever.** The first release never writes to your config: what it changes
  lives in the running session, only in areas you allowed, and one reload or one click on
  Deactivate undoes it. Writing to your config comes later, and when it does, every change
  is shown as a diff, backed up, and rolled back automatically if Hyprland rejects it.

## What it reads and writes

**Reads:** your Hyprland config folder (following links), Hyprland's IPC sockets in
`$XDG_RUNTIME_DIR/hypr/`, the running instance's `hyprland.lock` and command line (to
find which config it loaded), and, if present, Hyprland's API stubs in
`/usr/share/hypr/stubs/`.

**Writes:** only into `~/.config/hyprland-studio/`: its settings (window size, last page,
which areas you allowed), and, only while the desktop-theme area is active, a record of
the theme values it changed so they can always be put back, even after a crash. No logs,
no caches, no exports: copying goes to the clipboard. A menu item removes that folder for
you.

**Changes while active, only in areas you allowed:** the running Hyprland's options, rules
and keybindings (through IPC; a reload undoes them), the programs it started for you, and
your desktop theme settings.

Reading your config never runs it. The studio reads Lua configs in a sandbox of its own,
where commands, file writes and startup programs are recorded but never carried out.

## Status

Pre-0.1. Research is in [`docs/research.md`](docs/research.md), the design in
[`docs/architecture.md`](docs/architecture.md), and the work is planned on
[neotrac.org/hyprland-studio](https://neotrac.org/hyprland-studio). There is nothing to
build yet.

## Roadmap

1. **0.1, see everything, and activate or deactivate in one click.**
   - A keybinding browser with search and conflict detection, an option browser with
     documentation and "changed from default" highlighting, a live tree of monitors,
     workspaces and windows, and an event log.
   - **Activate** applies the studio's effects to your running desktop: Hyprland options
     and rules, keybindings, outside programs you installed (a bar, a dock), and the
     desktop theme. **Deactivate** puts everything back exactly as it was. Each of these
     areas has its own "allow" switch, off until you turn it on.
   - Nothing is written to your config. Activation lives in the running session.
2. **0.2, the configuration engine.** Lossless reading and editing of `hyprland.lua`.
3. **0.3, save.** Write what you chose back to your config: diff, backup, atomic write,
   reload, automatic rollback on failure.

After that: keeping an activation across restarts (opt-in), an editable monitor
layout, keybinding and window-rule editors, an animation and bezier
editor, and support for the older `hyprland.conf` format.

## Building

Not yet available. Hyprland Studio will be written in modern C++ (C++26) on
[hyprtoolkit](https://github.com/hyprwm/hyprtoolkit), Hyprland's own Wayland-native
toolkit, built with CMake and Ninja, and target Linux on Wayland only. It depends only on
libraries that distributions already package.

## Acknowledgements

Hyprland Studio learned from [Noctalia](https://github.com/noctalia-dev/noctalia), an
MIT-licensed Wayland shell by noctalia-dev and contributors: in particular its direct use
of Hyprland's sockets, and its rule of never managing the user's compositor config. No
Noctalia code is included.

## License

GPL-3.0-or-later. See [`LICENSE`](LICENSE).

`SPDX-License-Identifier: GPL-3.0-or-later`
