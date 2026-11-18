// sum_float.c -- same benchmark as sum_int.c, but with floats.
//
// Build:  gcc -std=c11 -Wall -O3 -o sum_float_O3 sum_float.c
// Run:    ./sum_float_O3        (default 1000 reps)
//
// Watch the printed sum, not just the time: float addition isn't
// associative, so -Ofast (which reorders the additions to vectorize)
// can give a different answer than -O0 through -O3.

#define _POSIX_C_SOURCE 199309L   // needed for clock_gettime with -std=c11
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000000
#define DEFAULT_REPS 1000

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

volatile float sink;   // forces every rep's sum to really be computed

int main(int argc, char **argv) {
    int reps = (argc > 1) ? atoi(argv[1]) : DEFAULT_REPS;

    float *a = malloc(N * sizeof *a);
    if (a == NULL) { perror("malloc"); return 1; }

    // Every element is 0.1f, so the exact answer is 100000.
    // But 0.1 isn't exactly representable in binary, and once the running
    // sum gets large, each += rounds to float precision. One long chain of
    // additions (-O0 to -O3) piles up that error; -Ofast's several
    // partial sums stay smaller, so they round differently.
    for (int i = 0; i < N; i++)
        a[i] = 0.1f;

    double start = now_sec();
    for (int r = 0; r < reps; r++) {
        float sum = 0.0f;
        for (int i = 0; i < N; i++)
            sum += a[i];
        sink = sum;
        __asm__ volatile("" ::: "memory");   // compiler barrier (see sum_int.c)
    }
    double elapsed = now_sec() - start;

    printf("sum = %.4f (exact 100000) | %8.3f ms/sum | %.3f ns/element\n",
           sink, elapsed * 1e3 / reps, elapsed * 1e9 / ((double)reps * N));

    free(a);
    return 0;
}