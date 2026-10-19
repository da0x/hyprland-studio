# Hyprland Studio

A visual configuration and management studio for [Hyprland](https://hypr.land).

See what your compositor is doing, browse every option and keybinding, try changes
live, and save them safely, all without giving up control of your configuration files.

> Hyprland Studio is an independent open-source project for configuring and managing
> Hyprland. It is not affiliated with or endorsed by the Hyprland project.

## Mission

Make Hyprland easier to understand and configure in the least intrusive way possible.

Hyprland Studio never changes your system behind your back. It installs nothing, runs
nothing with `sudo`, and touches no file you have not approved. When something would
help, like a status bar, a dock or a notification daemon, it recommends an existing,
well-known open-source program, tells you exactly what to install and why, and lets you
install it yourself. After that it can help you configure it, through the same
reviewed-diff process as everything else.

If you ever wonder "what did this do to my system?", that is a bug. Every change it makes
is listed, explained and reversible.

## Principles

- **You decide every change.** No package installs, no `sudo`, no replaced or moved
  files, no new autostart entries, nothing outside what you approved in a diff. Every
  change is recorded in a log you can read and undo.
- **Your config file is the source of truth.** Hyprland Studio reads and writes your own
  `hyprland.lua`. It keeps no database and no hidden copy. Stop using it and you lose
  nothing.
- **Edits are lossless.** When it changes a value, it changes only that value. Your
  comments, formatting, `require` calls and anything it does not understand are left
  exactly as they were.
- **Safe before clever.** The first releases only read from Hyprland. Writing to your
  config comes later, and when it does, every change is shown as a diff, backed up, and
  rolled back automatically if Hyprland rejects it.

## Status

Pre-0.1. The project is in research: see [`docs/research.md`](docs/research.md).
There is nothing to build yet.

## Roadmap

1. **0.1, read-only and live.** A keybinding browser with search and conflict detection,
   an option browser with documentation and "changed from default" highlighting, a live
   tree of monitors, workspaces and windows, and an event log with an IPC console.
2. **0.2, try before you write.** Change options and monitor settings at runtime, with
   one-click revert. Nothing is written to disk.
3. **0.3, the configuration engine.** Lossless reading and editing of `hyprland.lua`.
4. **0.4, save.** Write what you tried back to your config: diff, backup, atomic write,
   reload, automatic rollback on failure.

After that: help choosing and configuring a status bar and a dock from existing
open-source programs (such as Waybar), which you install yourself, an editable monitor
layout, keybinding and window-rule editors, an animation and bezier
editor, and support for the older `hyprland.conf` format.

## Building

Not yet available. Hyprland Studio will be written in modern C++ (C++26) with Qt 6,
built with CMake and Ninja, and target Linux on Wayland only. It depends only on
mainstream libraries that major distributions already package.

## License

GPL-3.0-or-later. See [`LICENSE`](LICENSE).

`SPDX-License-Identifier: GPL-3.0-or-later`
