// Offline MPFR oracle. Never linked into the game or its ordinary tests.
#include <mpfr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t ullSeed = UINT64_C(0x53a27bcd01876f09);
static uint64_t Next(void) {
    ullSeed ^= ullSeed << 13;
    ullSeed ^= ullSeed >> 7;
    ullSeed ^= ullSeed << 17;
    return ullSeed;
}
static uint64_t Bits(double d) {
    uint64_t u;
    memcpy(&u, &d, 8);
    return u;
}
static double Value(uint64_t u) {
    double d;
    memcpy(&d, &u, 8);
    return d;
}

static void Eval(int i, mpfr_t r, mpfr_t x, mpfr_t y, mpfr_rnd_t rnd) {
    switch (i) {
    case 0:
        mpfr_sqrt(r, x, rnd);
        break;
    case 1:
        mpfr_floor(r, x);
        break;
    case 2:
        mpfr_hypot(r, x, y, rnd);
        break;
    case 3:
        mpfr_atan2(r, x, y, rnd);
        break;
    case 4:
        mpfr_sin(r, x, rnd);
        break;
    case 5:
        mpfr_cos(r, x, rnd);
        break;
    case 6:
        mpfr_pow(r, x, y, rnd);
        break;
    }
}

int main(void) {
    mpfr_t x, y, lo, hi;
    double dx, dy, dLo, dHi;
    int    i, j, cBits;

    mpfr_inits2(128, x, y, lo, hi, (mpfr_ptr)0);
    for (j = 0; j < 2000; j++) {
        for (i = 0; i < 7; i++) {
            dx = (int32_t)Next() / 65536.0;
            dy = (int32_t)Next() / 1048576.0;
            if (i == 0)
                dx = Value(Next() & UINT64_C(0x7fefffffffffffff));
            if (i == 6)
                dx = Value(UINT64_C(0x3fe0000000000000) | (Next() & UINT64_C(0x000fffffffffffff)));
            mpfr_set_d(x, dx, MPFR_RNDN);
            mpfr_set_d(y, dy, MPFR_RNDN);
            for (cBits = 128; cBits <= 4096; cBits *= 2) {
                mpfr_set_prec(lo, cBits);
                mpfr_set_prec(hi, cBits);
                Eval(i, lo, x, y, MPFR_RNDD);
                Eval(i, hi, x, y, MPFR_RNDU);
                dLo = mpfr_get_d(lo, MPFR_RNDN);
                dHi = mpfr_get_d(hi, MPFR_RNDN);
                if (Bits(dLo) == Bits(dHi))
                    break;
            }
            if (cBits > 4096) {
                fprintf(stderr, "Unresolved: %d %a %a\n", i, dx, dy);
                return 1;
            }
            printf("%d %016llx %016llx %016llx\n", i, (unsigned long long)Bits(dx), (unsigned long long)Bits(dy), (unsigned long long)Bits(dLo));
        }
    }
    mpfr_clears(x, y, lo, hi, (mpfr_ptr)0);
    return 0;
}
