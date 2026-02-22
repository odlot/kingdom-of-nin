# kingdom-of-nin

A cooperative, rogue-like RPG.

## Build and Run

```bash
cmake -S . -B build
cmake --build build
./build/kingdom_of_nin
```

## Validation

### Build check

```bash
scripts/check-build.sh
```

### Tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

### Git hooks

```bash
scripts/setup-hooks.sh
```

### Format C++ sources

```bash
clang-format -i $(rg --files -g "*.cc" -g "*.h" -g "*.hpp")
```

### Run clang-tidy during builds

```bash
cmake -S . -B build -DKINGDOM_OF_NIN_ENABLE_CLANG_TIDY=ON
cmake --build build
```

### Building the container image

```bash
docker build . -t kingdom-of-nin:0.1.1
```

### Running and attaching to the container image

```bash
docker container run -d -it --rm --mount type=bind,src=./,dst=/app kingdom-of-nin:0.1.1 bash
docker attach <CONTAINER_ID/NAME>
```

### Authenticating with Codex

```bash
codex login --device-auth
```

## Releases

- A GitHub Actions workflow (`.github/workflows/release.yml`) runs on every push to `main`.
- It uses semantic version tags in `vX.Y.Z` format and auto-increments the patch number
  (example: `v0.1.3` -> `v0.1.4`).
- The workflow builds and tests the project, then publishes a GitHub Release with a Linux
  build artifact and checksum.
- If no prior semantic tag exists, it starts from `v0.1.0` (configurable in the workflow via
  `INITIAL_VERSION`).
