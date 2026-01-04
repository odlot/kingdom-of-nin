# Repository Guidelines

## Product Direction

- Goal: build a cooperative, roguelike multiplayer RPG inspired by World of Warcraft, Warcraft 3, and Maplestory.
- Current scope: all gameplay is singleplayer while the architecture prepares for future multiplayer.

## Project Structure & Module Organization

- `main.cc` defines the entry point and wires the game loop.
- `src/` contains game implementation modules (`game.cc`, `camera.cc`, `ecs/`, `world/`).
- `include/` holds public headers used across the project.
- `assets/` contains runtime assets copied into the build output.
- `build/` is the CMake build directory (generated).

## Build, Test, and Development Commands

- Configure and build:
  - `cmake -S . -B build`
  - `cmake --build build`
- Run the game: `./build/kingdom_of_nin`
- Build a container image: `docker build . -t kingdom-of-nin:0.1.1`
- Run container and attach:
  - `docker container run -d -it --rm --mount type=bind,src=./,dst=/app kingdom-of-nin:0.1.1 bash`
  - `docker attach <CONTAINER_ID/NAME>`

## Coding Style & Naming Conventions

- Language: C++20 (see `CMakeLists.txt`).
- Indentation: 2 spaces; brace style follows K&R in existing files.
- Prefer explicit ownership with `std::unique_ptr` as shown in `main.cc`.
- Keep headers in `include/` and source in `src/`; match filenames (`game.h` ↔ `game.cc`).

## Testing Guidelines

- No automated tests are configured yet.
- If you add tests, document the command in `README.md` and wire them into CMake/CTest; place sources in a `tests/` directory and use a consistent suffix like `_test.cc`.

## Validation

- Always run `scripts/check-build.sh` after code changes.
- Format C++ sources with `clang-format` before committing.
- Run clang-tidy builds when touching core systems or refactors: `cmake -S . -B build -DKINGDOM_OF_NIN_ENABLE_CLANG_TIDY=ON` then `cmake --build build`.
- Configure git hooks with `scripts/setup-hooks.sh` to enable shared pre-commit validation.

## Commit & Pull Request Guidelines

- Commit history only contains an initial commit; no established message convention yet.
- Use short, imperative summaries (e.g., “Add world generator seed”) and include context in the body when behavior changes.
- Keep commits atomic: commit only the files you touched and list each path explicitly in commit commands.
- PRs should include: a concise summary, how to run or verify changes, and screenshots or short clips for gameplay/UI changes when relevant.
