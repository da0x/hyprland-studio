# AGENTS.md

How to work on Hyprland Studio. This file is for everyone who changes the code, people
and assistants alike, and it is kept current: if something here is wrong, fix it in the
same change.

## The rules that shape everything else

**The mission is to enhance Hyprland in the least intrusive way possible.** This rule
outranks every feature. The studio must never leave a user asking "what did it do to my
system?":

- It never installs a package, never runs `sudo`, and never asks for a password.
- When an outside program would help (a status bar, a dock, a notification daemon), it
  prefers an existing, widely packaged open-source one, such as Waybar. It names the
  program, links its upstream, explains why, and shows the exact install command for the
  user to run themselves.
- It writes only files the user approved in a reviewed diff. It never replaces a symbolic
  link with a real file, never moves or deletes the user's files, and never adds
  autostart entries, shell startup lines or theme settings on its own.
- Every change it makes goes into a readable log, with a way to undo it.
- It does not ship its own replacements for programs the Linux desktop already has.
- It never makes the final decision. It reports, explains the options and what each
  would change, and may recommend one; the user chooses. Nothing is fixed automatically,
  not even a keybinding conflict.
- It leaves nothing behind. What it writes into the user's config is plain Hyprland
  configuration: no markers, no sidecar files, no `require` lines of its own, so the config
  works the same once the studio is gone. Everything it keeps for itself lives in
  `~/.config/hyprland-studio/`, which the app can remove on request. Uninstalling the
  package and deleting that folder must leave the system exactly as the user's own
  choices made it.

**The user's config file is the source of truth.** The studio never stores Hyprland
settings anywhere else, never keeps a private copy of the config, and never asks the user
to migrate to anything. The only file it keeps is its own UI settings (window size, last
page) in `~/.config/hyprland-studio/`, through `QSettings`. Anything else it would write
needs an entry in the README's *What it reads and writes* first. Someone who uninstalls it must be left with a config that works and reads the
way they wrote it.

**Edits are lossless.** A change replaces exactly the bytes of the value that changed.
Everything else in the file, including comments, blank lines, ordering, `require` calls,
and code the studio does not understand, stays byte for byte identical. Reading a file
and writing it back with no edits must produce the same bytes. A test proves this for
every fixture.

**A Lua config is a program, not a data file.** Only `hl.*` calls whose arguments are
literals are editable. A value that is computed (from a local, a function, a loop) is
shown read-only, with the file and line it comes from. Do not try to evaluate or rewrite
user code.

**Read-only features ship before ones that write.** Anything that only queries Hyprland
cannot break someone's desktop, so it comes first. Writing to disk always goes through a
diff the user reviews, a timestamped backup, an atomic write, a reload and a check, with
automatic rollback on failure.

**Never start the user's programs by accident.** `Hyprland --verify-config` runs the
whole config, including commands and file writes at its top level. Do not run it as a
background check. Reading a config for display goes through the studio's own sandboxed
interpreter, which runs nothing. See `docs/research.md`.

**Never switch the user's config format.** A `hyprland.lua` on the config path wins over
any `hyprland.conf`, so creating one next to a working `.conf` changes which config
Hyprland loads at the next reload. Only a migration the user asked for may do that.

**The running compositor is reached only through IPC.** When IPC does not give
something the studio needs, the workaround lives in the IPC layer and nowhere else, and
the gap is recorded in `docs/upstream.md`.

**Live screens update from events.** Compositor state comes from Hyprland's event
socket. Do not poll. If the socket drops, the screen says so and reconnects by itself,
because a dead connection otherwise looks exactly like a quiet compositor.

## Code

- **GPL-3.0-or-later.** Every source, header, CMake and script file starts with
  `SPDX-License-Identifier: GPL-3.0-or-later` in a comment, and CI checks it. Code from
  elsewhere comes in only under a GPL-compatible license, with its own notice kept
  (Hyprland's example config and stubs, for instance, are BSD-3-Clause).
- **Packageable as is.** A distribution, Arch Linux first, must be able to package any
  release with no patches. So the build respects the packager's flags: it works with
  `CMAKE_BUILD_TYPE=None`, adds no `-O` or `-march` of its own, and turns warnings into
  errors only when CI asks (`HYPRLAND_STUDIO_WARNINGS_AS_ERRORS`, off by default). It
  installs with `GNUInstallDirs` under any prefix and `DESTDIR`, downloads nothing during
  configure or build (no `FetchContent`), bundles no copy of a system library, runs its
  tests offline with `ctest`, and embeds no timestamps (`SOURCE_DATE_EPOCH` if one is ever
  needed). Releases are signed git tags that are never moved. The app ships a desktop
  file, AppStream metainfo and hicolor icons.
- **Mainstream libraries only.** Qt 6, KDE's LayerShellQt if a layer surface is needed,
  tree-sitter, Lua, and the C++ standard library: things every major distribution
  packages. No personal or niche libraries, so the studio builds from distribution
  packages alone and stays easy to package.
- **The latest C++ standard the shipping compilers support**, currently C++26. Modern
  code only: no raw `new` or `delete`, no C-style casts or arrays, no macros where a
  `constexpr` or a template will do. Use `std::expected` for errors, ranges, `std::format`,
  strong types instead of bare `int` and `std::string`, and value semantics. clang-tidy's
  `modernize-*` and `cppcoreguidelines-*` checks enforce this in CI.
- **Layers.** `configuration` (read, edit and diff config files) has no Qt dependency.
  Qt, the IPC sockets and the Lua syntax tree library are each wrapped once, so the rest
  of the code uses the project's own types.
- **Names are full words**: `configuration`, not `config`; `application`, not `app`;
  `function`, not `fn`. A folder, its namespace and its concept share one name:
  `src/configuration/` is `namespace configuration`. Folders and files are lowercase,
  with hyphens in file and folder names.
- **Every feature comes with tests.** Configuration code is tested against real config
  files in `tests/fixtures/`.

## Commits

- One logical change per commit. Prefer "Add the keybinding model" then "Detect
  conflicting keybindings" over one large commit.
- Subjects are plain sentences that start with a capital verb: "Add the IPC client". No
  `feat(scope):` prefixes.
- No attribution or `Co-Authored-By` trailers.

## Where things are

- `docs/research.md`: what we know about Hyprland's config formats and IPC, and what is
  still open. Read it before touching the configuration or IPC code.
- `docs/upstream.md`: gaps in Hyprland's IPC, and how the studio works around them.
