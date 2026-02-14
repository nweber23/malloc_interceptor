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

# Size-based failure filtering
MALLOC_FAIL_SIZE_MIN=1024      # Only fail allocations >= 1024 bytes
MALLOC_FAIL_SIZE_MAX=512       # Only fail allocations <= 512 bytes

# Statistics and reporting
MALLOC_FAIL_STATS=1            # Print allocation statistics at program exit

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

**Test large allocation failures only:**
```bash
# Only fail allocations of 4KB or more
LD_PRELOAD=./interceptor.so MALLOC_FAIL_SIZE_MIN=4096 MALLOC_FAIL_EVERY=10 ./your_program
```

**View allocation statistics:**
```bash
# Print detailed statistics about all allocation attempts at exit
LD_PRELOAD=./interceptor.so MALLOC_FAIL_STATS=1 ./your_program
```

**Debug mode with detailed output:**
```bash
# Show what decision was made for each allocation
LD_PRELOAD=./interceptor.so MALLOC_FAIL_AT=5 MALLOC_FAIL_DEBUG=1 ./your_program 2>&1 | head -20
```

## Building and Testing

Requirements: GCC, Make (standard on Ubuntu 22.04+)

```bash
make           # Build the interceptor
make test      # Run test suite with various failure modes
make clean     # Remove objects
make fclean    # Remove everything
```

## Supported Allocation Functions

The interceptor intercepts:
- **Standard C**: malloc, calloc, realloc, free
- **POSIX**: posix_memalign
- **Aligned allocation**: aligned_alloc, memalign, valloc, pvalloc

All functions support the same failure modes and filtering options.

## How It Works

Uses `LD_PRELOAD` to intercept memory allocation functions and return NULL (or error codes) at specified points. Features include:

- **Configurable failure points**: Specify exact indices, ranges, or periodic patterns
- **Size-based filtering**: Selectively fail large or small allocations
- **Statistics tracking**: Monitor success/failure counts for each allocation type
- **Thread-safe**: Uses atomic operations for safe concurrent access
- **Debug mode**: Detailed logging of each allocation decision

Zero dependencies beyond libc, works with any dynamically-linked binary.

## Limitations

- Linux only (uses `LD_PRELOAD`)
- Doesn't work with statically-linked or setuid binaries
- Some systems may not have all alignment functions available
