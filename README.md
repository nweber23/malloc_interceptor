# malloc-fail-injector 🧨

A lightweight **malloc failure simulator** for Linux (Ubuntu 22.04+).  
It lets you **force specific allocations to fail** — perfect for testing error handling, robustness, and memory safety.

---

## 🚀 Features

- Works with **any binary** via `LD_PRELOAD`
- Specify which allocations to fail:
  - `MALLOC_FAIL_AT=35` → fail the 35th allocation
  - `MALLOC_FAIL_AT="10,20,30"` → fail those specific calls
  - `MALLOC_FAIL_AT="5-8"` → fail allocations 5 through 8
  - `MALLOC_FAIL_EVERY=100` → fail every 100th allocation
- Thread-safe and zero external dependencies
- No code changes needed in your project

---

## 🧱 Building

Requirements: `gcc`, `make`, `libdl` (standard on Ubuntu 22.04)

```bash
git clone https://github.com/yourname/malloc-fail-injector.git
cd malloc-fail-injector
make
