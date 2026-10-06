# Research

What Hyprland gives us to build on, what we have confirmed, and what is still open.
Each section is a question. When one is answered, write the answer under it with the
Hyprland version it was checked against.

Checked against Hyprland 0.56.2 (v0.56.2, commit `efb5099`) on Arch-based Linux, with
Qt 6.11, GCC 16.2 and Clang 22.

## Which config formats does Hyprland read?

**Confirmed:** two. The newer Lua format (`hyprland.lua`) and the older hyprlang format
(`hyprland.conf`). The binary links both `libhyprlang` and `liblua`.

Hyprland ships an example Lua config at `/usr/share/hypr/hyprland.lua` (356 lines). It
is a Lua program that calls into an `hl` table:

```lua
hl.monitor({
    output   = "",
    mode     = "preferred",
    position = "auto",
    scale    = "auto",
})

local terminal = "kitty"
```

Many existing users still run `hyprland.conf`. We start with Lua and add `.conf` later.

**Open:** how Hyprland picks between the two when both exist, and how `require` resolves
files relative to the config directory.

## What is the shape of the `hl` API?

**Confirmed:** Hyprland ships LuaLS type annotations for the whole API at
`/usr/share/hypr/stubs/hl.meta.lua`, about 1,300 annotation lines with classes such as
`HL.BindOptions`, `HL.LayoutProvider` and `HL.Window`. This is our schema for which calls
the editor recognizes and what their arguments mean.

**Open:** a full list of the `hl.*` calls a config uses in practice, which of them take
plain literal tables, and which take functions (layouts, event handlers) that must stay
read-only.

## How do we read and write a Lua file without losing anything?

We need a Lua syntax tree that keeps every byte: comments, whitespace and exact source
positions. Edits then replace only the byte range of the changed value.

**Open:**
- tree-sitter-lua is the first candidate: it is C, keeps byte ranges for every node and
  tolerates broken input. Check its Lua 5.4 coverage and its license.
- Compare with alternatives before choosing.

## How do we check a config before Hyprland loads it?

**Confirmed:** `Hyprland --verify-config` checks a config without starting the
compositor, and `--config FILE` points it at a file. `hyprctl configerrors` lists the
errors in the config that is currently loaded.

**Open:** whether `--verify-config` handles Lua configs fully, including `require`d
files, and whether it can safely run while a session is already up.

## What does IPC give us?

**Confirmed:** each running instance has two Unix sockets in
`$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/`:
- `.socket.sock` answers requests. `hyprctl` is a client of it, and `-j` asks for JSON.
- `.socket2.sock` streams events, one per line.

Requests that matter for the first releases:
- `binds`: every registered keybinding, with modifier mask, key, keycode, submap,
  dispatcher, argument and flags (`locked`, `release`, `repeat`, `mouse` and others).
- `descriptions`: every option (353 here) with description, default, current value,
  minimum and maximum. This is the option catalogue; we do not maintain one by hand.
- `getoption <name>`: one option's current value.
- `monitors` (and `monitors all`, which includes disabled outputs), `workspaces`,
  `activeworkspace`, `clients`, `activewindow`, `layers`, `devices`, `animations`,
  `layouts`, `instances`.
- `configerrors`, `rollinglog`.

**Open:** the full event list and the format of each event on `.socket2.sock`; how
reconnects behave when Hyprland reloads or restarts.

## Can we change things at runtime without writing the file?

This decides whether "try before you write" is possible.

**Confirmed:** `hyprctl` offers `keyword <name> <value>` to set a config keyword
dynamically, `eval <code>` to run a Lua string, `repl` for an interactive Lua session,
and `reload` (with `config-only` to leave monitors alone).

**Open:** whether `keyword` still works under a Lua config or `eval` replaces it; whether
runtime changes are dropped on reload (which is what makes one-click revert simple); and
how to read back the value an option had before we changed it.

## What already exists?

**Open:** survey existing Hyprland GUI and TUI tools (settings apps, monitor layout
tools, keybinding viewers) so we build what is missing instead of a second copy of
something that works.

## Can the project be called "Hyprland Studio"?

**Open:** check Hyprland's guidance on using its name, and any existing project with the
same or a similar name. The README already says the project is independent and not
endorsed.
