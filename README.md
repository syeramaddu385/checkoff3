# Checkoff 3 - Compiler Optimization and Vectorization

I wrote a loop that sums a million-element array in C at every compiler optimization level. The `int` version got roughly 13× faster while the `float` version barely changed, until I used the -Ofast flag which made it about 12× faster. However, this flag ended up changing the answer of the addition.

## [Slides](https://docs.google.com/presentation/d/1uc9v5L3NSnS4ucjrBhWW2MMrlNGyYSGqLonuty-zfgg/edit?usp=sharing)

## [Full AI Conversation](https://claude.ai/share/c5f28d71-c7eb-4616-8ada-c5844eae1b83)

## What I explored

- What the optimization levels `-O0`, `-O1`, `-O2`, `-O3` and `-Ofast` actually do
- What vectorization is: one instruction splitting up the math and doing it all concurrently.
- Why a simple sum of ints gets vectorized automatically, but a sum of floats doesn't
- How floating-point rounding decides what the compiler is allowed to do

## My results

Time per sum of 1,000,000 elements, averaged over 1,000 runs.

| Level | int sum | float sum | float result (exact: 100000) |
|---|---|---|---|
| `-O0` | 0.529 ms | 0.597 ms | 100958.3438 |
| `-O1` | 0.248 ms | 0.591 ms | 100958.3438 |
| `-O2` | 0.041 ms | 0.502 ms | 100958.3438 |
| `-O3` | 0.036 ms | 0.500 ms | 100958.3438 |
| `-Ofast` | 0.035 ms | 0.041 ms | 100060.0000 |

The int sum was exactly 4500000 at every level when summing 0, 1, 2, ..., 9 repeatedly.

## Findings

### 1. Ints can be regrouped, floats can't

To use SIMD on a sum, the compiler has to keep several separate running totals and combine them at the end. That adds the same numbers in a different grouping.

- Integer addition is associative, so `(a + b) + c` always equals `a + (b + c)`. The compiler regroups on its own, and it speeds up a lot at `-O2`.
- Float addition adds rounds after each add, so a different grouping can give a different answer. No `-O` level is allowed to change your program's result, so the compiler keeps the exact order.
- `-Ofast` gives the compiler permission to break that rule. With this permission the float sum speeds up significantly.

### 2. The fast-math sum was actually closer to the true answer

Every element was `0.1f`, so the exact answer is 100000. The sequential sum resulted in 100958. The fast-math sum resulted in 100060.

One big total rounds badly on every add and the error is always in the same direction. Sixteen small totals round about 16× less. But this is luck for this data. Reordering isn't actually reliably more accurate. That's why the compiler won't do it unless you give it specific permissions.

### 3. My Mac vectorized at -O3 but it wasn't actually faster

Clang reported the float loop as vectorized even without fast-math. In the assembly, it loads 16 floats at once but still adds them one after the other, keeping the exact IEEE order. Each add still waits for the previous one, so it's barely any faster.

With fast-math, it uses `fadd.4s` on 4 separate vector registers, which are 16 independent totals that don't wait on each other. So it's a lot faster.

This behavior is specific to Clang on ARM (Apple Silicon). On x86, Clang reports that the loop was not vectorized. On ARM it only vectorized the loading, so loading the integers was faster but not the actual addition.

## Running the code

You need a C compiler (`gcc` or `clang`). On macOS, `gcc` is actually Clang.

```sh
sh run_all.sh               # build and run both programs at every level
CC=clang sh run_all.sh      # use clang explicitly
```

To build one program at one level:

```sh
gcc -std=c11 -Wall -O3 -o sum_float_O3 sum_float.c
./sum_float_O3              # default 1000 reps
./sum_float_O3 200          # custom rep count
```

The times will differ by machine, and each run varies a bit, so run each binary more than once. `-Ofast` is the same as `-O3 -ffast-math`.

### Checking what the compiler actually did

```sh
# Clang: which loops were vectorized, and why others weren't
clang -O3 -c -o /dev/null -Rpass=loop-vectorize sum_float.c
clang -O3 -c -o /dev/null -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize sum_float.c

# GCC equivalents
gcc -O3 -c -o /dev/null -fopt-info-vec-optimized sum_float.c
gcc -O3 -c -o /dev/null -fopt-info-vec-missed sum_float.c

# Compare the add instructions with and without fast-math
clang -O3 -S -o - sum_float.c | grep fadd
clang -O3 -ffast-math -S -o - sum_float.c | grep fadd
```

## Repository contents

| File | What it is |
|---|---|
| `sum_int.c` | Sums a `malloc`'d array of 1M ints, many times, and times it |
| `sum_float.c` | The same benchmark with floats; prints the sum so you can see rounding |
| `run_all.sh` | Builds and runs both programs at `-O0` through `-Ofast` |

## Sources

- [Auto-Vectorization in LLVM](https://llvm.org/docs/Vectorizers.html): the Clang/LLVM docs. The "Reductions" section explains why float sums need permission to be reordered.
- [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html): what each `-O` level does, and what `-Ofast` and `-ffast-math` actually change.