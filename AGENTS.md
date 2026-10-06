# AGENTS.md

How to work on Hyprland Studio. This file is for everyone who changes the code, people
and assistants alike, and it is kept current: if something here is wrong, fix it in the
same change.

## The rules that shape everything else

**The user's config file is the source of truth.** The studio never stores settings of
its own, never keeps a private copy of the config, and never asks the user to migrate to
anything. Someone who uninstalls it must be left with a config that works and reads the
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

**Live screens update from events.** Compositor state comes from Hyprland's event
socket. Do not poll. If the socket drops, the screen says so and reconnects by itself,
because a dead connection otherwise looks exactly like a quiet compositor.

## Code

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
