# malloc-fail-injector

Force malloc to fail at specific points — test your C program's error handling without modifying code.

```bash
make
LD_PRELOAD=./interceptor.so MALLOC_FAIL_AT=10 ./your_program
```

---

## Usage

```bash
# Fail specific allocations
MALLOC_FAIL_AT=35              # Fail the 35th call
MALLOC_FAIL_AT="10,20,30"      # Fail calls 10, 20, and 30
MALLOC_FAIL_AT="5-8"           # Fail calls 5 through 8

# Fail periodically
MALLOC_FAIL_EVERY=100          # Fail every 100th allocation

# Advanced options
MALLOC_FAIL_OFFSET=-5          # Shift allocation numbering
MALLOC_FAIL_DEBUG=1            # Show decision for each allocation
```

## Examples

**Find unchecked malloc calls:**
```bash
for i in {1..100}; do
    LD_PRELOAD=./interceptor.so MALLOC_FAIL_AT=$i ./your_program || echo "Crash at $i"
done
```

**Test specific scenario:**
```bash
# Fail allocations 5, 10, and 15
LD_PRELOAD=./interceptor.so MALLOC_FAIL_AT="5,10,15" ./your_program
```

## Building

Requirements: GCC, Make (standard on Ubuntu 22.04+)

```bash
make           # Build
make clean     # Remove objects
make fclean    # Remove everything
```

## How It Works

Uses `LD_PRELOAD` to intercept malloc/calloc/realloc and return NULL at specified points. Thread-safe, zero dependencies, works with any dynamically-linked binary.

## Limitations

- Linux only (uses `LD_PRELOAD`)
- Doesn't work with statically-linked or setuid binaries
- No support for `posix_memalign` yet
