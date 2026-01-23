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
