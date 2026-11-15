# fibtool - C build practice for Jenkins

A small CLI (Fibonacci series / primes) deliberately split into several build
artifacts so the pipeline has real work to do. Triggered by a PR webhook.

```
include/   public headers
src/       main.c, fibonacci.c, primes.c, parse.c  -> libfibcore.a  (static)
           fibutil.c                               -> libfibutil.so (shared)
tests/     tiny test framework that emits JUnit XML
version.txt  "Build N" - overridden by Jenkins' BUILD_NUMBER
Makefile / Jenkinsfile
```

## Usage

```sh
make                       # release build  -> artifacts/release/bin/fibtool
make BUILD=debug           # separate dir   -> artifacts/debug/
make BUILD=asan            # sanitizers     -> artifacts/asan/
make test WERROR=1         # build, run unit + smoke tests, write JUnit XML
make lint                  # needs cppcheck
make dist                  # artifacts/fibtool-<ver>-<sha>-release.tar.gz
make help

artifacts/release/bin/fibtool -n 100       # fibonacci up to 100
artifacts/release/bin/fibtool -p -n 100    # primes up to 100
FIBTOOL_LOG=debug artifacts/release/bin/fibtool -V
```

## What the build exercises

| Concept                         | Where                                           |
|---------------------------------|-------------------------------------------------|
| Static library (`ar`)           | `libfibcore.a`                                  |
| Shared library + soname + symlinks, `-fPIC` | `libfibutil.so.1.0.0`               |
| Runtime linking (`rpath $ORIGIN`) | `ldd artifacts/release/bin/fibtool`           |
| Generated source (`version.h`)  | Makefile rule, uses `version.txt`, git SHA, `BUILD_NUMBER` |
| Header dependency tracking      | `-MMD -MP` / `.d` files                         |
| Build flavours in isolated dirs | `BUILD=release|debug|asan`                      |
| Warnings as errors              | `WERROR=1`                                      |
| Test reports Jenkins can parse  | `artifacts/<flavour>/test-reports/junit.xml`    |
| Static analysis                 | `make lint` (cppcheck)                          |
| Install staging / packaging     | `make install DESTDIR=... ` / `make dist`       |

## Practice exercises

1. **Break it on purpose.** Change an assertion in `tests/test_primes.c`; confirm the
   stage turns red and the JUnit report shows the failing test.
2. **Warning gate.** Add an unused variable; it passes locally but fails in CI
   because of `WERROR=1`. Why is that a useful policy?
3. **Missing dependency.** Remove `cppcheck` from the agent (or install it) and
   watch the Lint stage flip between UNSTABLE and SUCCESS.
4. **Shared library hell.** Copy only `bin/fibtool` elsewhere and run it. Fix it
   using `LD_LIBRARY_PATH`, then via `rpath`. Check with `ldd`.
5. **Dockerise the agent.** Switch the Jenkinsfile to `agent { docker { image 'gcc:13' } }`
   and install cppcheck in a custom image.
6. **Caching.** Run `make` twice - the 2nd run should do nothing. Then `touch include/primes.h`
   and note which objects rebuild. Which Jenkins strategy keeps `artifacts/` between builds, and
   what is the risk?
7. **Stretch:** add `--coverage` (gcov/lcov + Cobertura publisher), a `clang` matrix
   entry (`make CC=clang`), or a cross-compile with `CC=aarch64-linux-gnu-gcc`.
