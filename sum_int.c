// sum_int.c -- sum a malloc'd array of 1M ints, many times, and time it.
//
// Build:  gcc -std=c11 -Wall -O2 -o sum_int_O2 sum_int.c
// Run:    ./sum_int_O2          (default 1000 reps)
//         ./sum_int_O2 200      (custom rep count)

#define _POSIX_C_SOURCE 199309L   // needed for clock_gettime with -std=c11
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000000
#define DEFAULT_REPS 1000

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);   // monotonic: unaffected by clock changes
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// volatile = "every write must actually happen." Writing each rep's sum here
// forces the compiler to really compute every sum instead of skipping
// reps whose results are never used.
volatile int sink;

int main(int argc, char **argv) {
    int reps = (argc > 1) ? atoi(argv[1]) : DEFAULT_REPS;

    int *a = malloc(N * sizeof *a);
    if (a == NULL) { perror("malloc"); return 1; }

    // Small values so the sum (max 9 * 1M) fits in an int.
    // Signed overflow is undefined behavior -- don't let that sneak in.
    for (int i = 0; i < N; i++)
        a[i] = i % 10;

    double start = now_sec();
    for (int r = 0; r < reps; r++) {
        int sum = 0;
        for (int i = 0; i < N; i++)
            sum += a[i];
        sink = sum;

        // Compiler barrier (GCC/Clang): "assume any memory may have changed."
        // Without it, -O2/-O3 can notice that every rep sums the same
        // unchanged array and compute it once -- then you're timing nothing.
        __asm__ volatile("" ::: "memory");
    }
    double elapsed = now_sec() - start;

    printf("sum = %d (exact 4500000) | %8.3f ms/sum | %.3f ns/element\n",
           sink, elapsed * 1e3 / reps, elapsed * 1e9 / ((double)reps * N));

    free(a);
    return 0;
}