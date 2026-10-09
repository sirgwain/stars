// Offline reference capture, never run by CTest. Requires MPFR and GMP.
// stdout: exact binary64 inputs, native result, correctly rounded result.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

static uint64_t Bits(double d) {
    uint64_t ull;
    memcpy(&ull, &d, sizeof(ull));
    return ull;
}

static void Emit(double x, double y) {
    mpfr_t a, b, lo, hi;
    mpfr_prec_t cBits;
    double dLo, dHi, dNative;
    double (*volatile pfnPow)(double, double) = pow;

    mpfr_inits2(128, a, b, lo, hi, (mpfr_ptr)0);
    mpfr_set_d(a, x, MPFR_RNDN);
    mpfr_set_d(b, y, MPFR_RNDN);
    for (cBits = 128; cBits <= 4096; cBits *= 2) {
        mpfr_set_prec(lo, cBits);
        mpfr_set_prec(hi, cBits);
        mpfr_pow(lo, a, b, MPFR_RNDD);
        mpfr_pow(hi, a, b, MPFR_RNDU);
        dLo = mpfr_get_d(lo, MPFR_RNDN);
        dHi = mpfr_get_d(hi, MPFR_RNDN);
        if (Bits(dLo) == Bits(dHi))
            break;
    }
    if (cBits > 4096) {
        fprintf(stderr, "Unresolved rounding: %a %a\n", x, y);
        exit(1);
    }
    dNative = pfnPow(x, y);
    printf("%016llx %016llx %016llx %016llx\n", (unsigned long long)Bits(x), (unsigned long long)Bits(y),
           (unsigned long long)Bits(dNative), (unsigned long long)Bits(dLo));
    mpfr_clears(a, b, lo, hi, (mpfr_ptr)0);
}

int main(void) {
    static const int rgAbility[] = {10, 20, 24, 30, 38};
    static const double rgBase[] = {0.75, 0.875};
    static const int rgDistance[] = {1, 7, 24, 25, 26, 80, 81, 99, 100, 101, 249, 250, 251, 999, 1000, 1200};
    static const int rgRange[] = {25, 81, 100, 144, 256};
    int i, j, k, n;
    double x, y;

    fprintf(stderr, "MPFR %s; directed bounds must round to identical binary64 values\n", mpfr_get_version());
    for (i = 0; i < 5; i++)
        for (j = 1; j <= 2; j++)
            for (n = 0; n <= 101; n++) {
                x = (double)(1.0L - (long double)rgAbility[i] / (1000 * j));
                Emit(x, n);
            }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 16; j++)
            for (k = 0; k < 5; k++) {
                y = (double)((long double)sqrt((double)(rgDistance[j] * rgDistance[j] + 1)) / rgRange[k]);
                Emit(rgBase[i], nextafter(y, -INFINITY));
                Emit(rgBase[i], y);
                Emit(rgBase[i], nextafter(y, INFINITY));
            }
        for (n = 0; n <= 100; n++) {
            Emit(rgBase[i], nextafter((double)n, -INFINITY));
            Emit(rgBase[i], n);
            Emit(rgBase[i], nextafter((double)n, INFINITY));
        }
    }
    // General positive-domain stress: unity, underflow and overflow boundaries.
    for (i = -1; i <= 1; i++) {
        x = i < 0 ? nextafter(1.0, 0.0) : i > 0 ? nextafter(1.0, 2.0) : 1.0;
        Emit(x, 0x1p52);
        Emit(x, -0x1p52);
    }
    Emit(2.0, -1074.0);
    Emit(2.0, -1075.0);
    Emit(2.0, nextafter(-1075.0, INFINITY));
    Emit(2.0, 1023.0);
    Emit(2.0, 1024.0);
    return 0;
}
