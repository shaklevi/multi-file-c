# Tests

| What | Where | Needs a display? |
|---|---|---|
| **Lint** – every `src/*.c` compiles with `-Wall -Wextra -Werror` | `make -C tests lint` | no |
| **Unit tests** – the *real* game sources linked against a mock MiniLibX; map validation, movement, collisions, collectibles, exit rules, rendering, ESC; every shipped level is checked, solved with a BFS and played to the end; every texture pack is validated. Built with AddressSanitizer + UBSan. | `make -C tests test` → `tests/reports/unit.xml` | no |
| **Integration tests** – the compiled `./maze_escape`: help/usage, rejection of bad maps *before* any window opens, and a smoke run of every level × texture pack under Xvfb | `bash tests/integration/test_cli.sh` → `tests/reports/integration.xml` | uses `xvfb-run` |

Run locally:

```bash
make linux                       # build the game
make -C tests lint test          # lint + unit tests
bash tests/integration/test_cli.sh
```

Options: `SAN=0` (no sanitizers), `--filter STR` / `--list` on `tests/build/unit_tests`,
`STRICT_KNOWN_BUGS=1`, `REQUIRE_XVFB=1`, `SMOKE_SECONDS=N`.

## Known bugs (`KNOWN_BUG` / `XFAIL`)

Tests for behaviour that is currently broken are kept in the suite but flagged as
*expected failures*, so the pipeline starts green and you can fix them one by one.
When you fix one, the test reports **XPASS** — delete its `KNOWN_BUG(...)` marker
(or the `known_bug` call in `test_cli.sh`) to turn it into a normal, enforced test.
Run with `STRICT_KNOWN_BUGS=1` (Jenkins parameter) to make them fail the build.

## Adding tests

* Unit test: add a `TEST(suite, name) { ... }` in `tests/unit/test_*.c` and list it in
  that file's table with `T(suite, name)`. Each test runs in its own process, so
  `exit()`, crashes and hangs inside the game code are caught and reported.
* New level in `levels/` or texture pack in `textures/`: picked up automatically.
