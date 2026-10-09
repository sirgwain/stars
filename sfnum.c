#include "sfnum.h"

/* Transcendentals use software binary128 guard digits, then round once to
 * binary64. These are our own series, not code from MPFR or a system libm.
 * They are validated against the offline oracle; this is not a proof of
 * correct rounding for every possible binary64 argument. */
#ifdef LITTLEENDIAN
#define IQLO 0
#define IQHI 1
#else
#define IQLO 1
#define IQHI 0
#endif

static float128_t QBits(uint64_t hi, uint64_t lo) {
    float128_t q;
    q.v[IQHI] = hi;
    q.v[IQLO] = lo;
    return q;
}

static float128_t QInt(int32_t i) { return i32_to_f128(i); }
static float128_t QNeg(float128_t q) {
    q.v[IQHI] ^= UINT64_C(0x8000000000000000);
    return q;
}
static float128_t QLn2(void) { return QBits(UINT64_C(0x3ffe62e42fefa39e), UINT64_C(0xf35793c7673007e6)); }
static float128_t QPi(void) { return QBits(UINT64_C(0x4000921fb54442d1), UINT64_C(0x8469898cc51701b8)); }
static double     DQ(float128_t q) { return Sf64Value(f128_to_f64(q)); }
static double     DBits(uint64_t u) {
    float64_t d;
    d.v = u;
    return Sf64Value(d);
}

static float128_t QLog(float128_t q) {
    float128_t z, zz, term, sum, next;
    int32_t    e, i;

    e = (int32_t)((q.v[IQHI] >> 48) & 0x7fff) - 16383;
    q.v[IQHI] = (q.v[IQHI] & UINT64_C(0x0000ffffffffffff)) | UINT64_C(0x3fff000000000000);
    /* Keep arguments just below one close to zero without subtracting two
     * approximations to ln(2). This matters when pow's exponent is large. */
    if (f128_lt(f128_div(QInt(3), QInt(2)), q)) {
        q = f128_div(q, QInt(2));
        e++;
    }
    z = f128_div(f128_sub(q, QInt(1)), f128_add(q, QInt(1)));
    zz = f128_mul(z, z);
    term = sum = z;
    for (i = 3; i < 301; i += 2) {
        term = f128_mul(term, zz);
        next = f128_add(sum, f128_div(term, QInt(i)));
        if (f128_eq(next, sum))
            break;
        sum = next;
    }
    return f128_add(f128_mul(QInt(2), sum), f128_mul(QInt(e), QLn2()));
}

static float128_t QExp(float128_t q) {
    float128_t r, term, sum, next, scale;
    int32_t    n, i;

    /* Only a binary64 result is needed. These bounds keep range reduction
     * and the binary128 scale normal even for extreme pow inputs. */
    if (f128_lt(QInt(1024), q))
        return QBits(UINT64_C(0x7fff000000000000), 0);
    if (f128_lt(q, QInt(-1024)))
        return QInt(0);
    n = (int32_t)f128_to_i32(f128_div(q, QLn2()), softfloat_round_near_even, false);
    r = f128_sub(q, f128_mul(QInt(n), QLn2()));
    term = sum = QInt(1);
    for (i = 1; i < 100; i++) {
        term = f128_div(f128_mul(term, r), QInt(i));
        next = f128_add(sum, term);
        if (f128_eq(next, sum))
            break;
        sum = next;
    }
    scale = QBits((uint64_t)(n + 16383) << 48, 0);
    return f128_mul(sum, scale);
}

double Sf64Sqrt(double d) { return Sf64Value(f64_sqrt(Sf64Bits(d))); }
double Sf64Floor(double d) { return Sf64Value(f64_roundToInt(Sf64Bits(d), softfloat_round_min, false)); }

double Sf64Pow(double x, double y) {
    uint64_t       ux = Sf64Bits(x).v, uy = Sf64Bits(y).v;
    uint64_t       ax = ux & UINT64_C(0x7fffffffffffffff), ay = uy & UINT64_C(0x7fffffffffffffff);
    const uint64_t inf = UINT64_C(0x7ff0000000000000), one = UINT64_C(0x3ff0000000000000);
    float128_t     qx, qy, q;
    uint32_t       c;
    int            fInteger = 0, fOdd = 0, fNegative, e;

    if (ay == 0 || ux == one)
        return DBits(one);
    if (ax > inf || ay > inf)
        return DBits(inf | UINT64_C(0x0008000000000000));
    if (ay == inf) {
        if (ax == one)
            return DBits(one);
        return DBits(((ax > one) != ((uy >> 63) != 0)) ? inf : 0);
    }
    e = (int)(ay >> 52) - 1023;
    if (e >= 52) {
        fInteger = 1;
        fOdd = e == 52 && (ay & 1);
    } else if (e >= 0 && (ay & ((UINT64_C(1) << (52 - e)) - 1)) == 0) {
        fInteger = 1;
        fOdd = ((ay | (UINT64_C(1) << 52)) >> (52 - e)) & 1;
    }
    fNegative = (ux >> 63) && fOdd;
    if (ax == 0 || ax == inf) {
        return DBits((((ax == inf) != ((uy >> 63) != 0)) ? inf : 0) | (fNegative ? UINT64_C(0x8000000000000000) : 0));
    }
    if ((ux >> 63) && !fInteger)
        return DBits(inf | UINT64_C(0x0008000000000000));
    if (uy == one)
        return x;
    qx = f64_to_f128(Sf64Bits(DBits(ax)));
    qy = f64_to_f128(Sf64Bits(y));
    /* Exact small powers must retain ties at the binary64 boundary. A
     * log/exp round trip can nudge an exact midpoint to the wrong side. */
    if (fInteger && ay <= UINT64_C(0x40b0000000000000)) {
        c = (uint32_t)f128_to_ui32_r_minMag(f64_to_f128(Sf64Bits(DBits(ay))), false);
        q = QInt(1);
        while (c != 0) {
            if (c & 1)
                q = f128_mul(q, qx);
            c >>= 1;
            if (c != 0)
                qx = f128_mul(qx, qx);
        }
        if (uy >> 63)
            q = f128_div(QInt(1), q);
    } else {
        q = QExp(f128_mul(qy, QLog(qx)));
    }
    if (fNegative)
        q = QNeg(q);
    return DQ(q);
}

double Sf64Hypot(double x, double y) {
    float128_t qx = f64_to_f128(Sf64Bits(x)), qy = f64_to_f128(Sf64Bits(y));
    if ((Sf64Bits(x).v & UINT64_C(0x7fffffffffffffff)) == UINT64_C(0x7ff0000000000000) ||
        (Sf64Bits(y).v & UINT64_C(0x7fffffffffffffff)) == UINT64_C(0x7ff0000000000000))
        return DBits(UINT64_C(0x7ff0000000000000));
    return DQ(f128_sqrt(f128_add(f128_mul(qx, qx), f128_mul(qy, qy))));
}

static float128_t QAtan(float128_t q) {
    float128_t zz, term, sum, next, offset = QInt(0);
    int        fReciprocal = f128_lt(QInt(1), q), i;

    if (fReciprocal)
        q = f128_div(QInt(1), q);
    if (f128_lt(f128_div(QInt(2), QInt(5)), q)) {
        q = f128_div(f128_sub(q, QInt(1)), f128_add(q, QInt(1)));
        offset = f128_div(QPi(), QInt(4));
    }
    zz = QNeg(f128_mul(q, q));
    term = sum = q;
    for (i = 3; i < 401; i += 2) {
        term = f128_mul(term, zz);
        next = f128_add(sum, f128_div(term, QInt(i)));
        if (f128_eq(next, sum))
            break;
        sum = next;
    }
    sum = f128_add(sum, offset);
    return fReciprocal ? f128_sub(f128_div(QPi(), QInt(2)), sum) : sum;
}

double Sf64Atan2(double y, double x) {
    uint64_t       ux = Sf64Bits(x).v, uy = Sf64Bits(y).v;
    uint64_t       ax = ux & UINT64_C(0x7fffffffffffffff), ay = uy & UINT64_C(0x7fffffffffffffff);
    const uint64_t inf = UINT64_C(0x7ff0000000000000);
    float128_t     q;

    if (ax > inf || ay > inf)
        return DBits(inf | UINT64_C(0x0008000000000000));
    if (ay == 0)
        q = (ux >> 63) ? QPi() : QInt(0);
    else if (ax == 0 || ay == inf)
        q = f128_div(QPi(), QInt(ax == inf ? 4 : 2));
    else
        q = QAtan(f128_div(f64_to_f128(Sf64Bits(DBits(ay))), f64_to_f128(Sf64Bits(DBits(ax)))));
    if ((ux >> 63) && ay != 0)
        q = f128_sub(QPi(), q);
    if (uy >> 63)
        q = QNeg(q);
    return DQ(q);
}

static double DSinCos(double d, int fCos) {
    float128_t q, pi2, zz, term, sum, next;
    int32_t    n, i;

    if ((Sf64Bits(d).v & UINT64_C(0x7fffffffffffffff)) == 0)
        return fCos ? DBits(UINT64_C(0x3ff0000000000000)) : d;
    /* The UI supplies angles from atan2 and at most one full revolution.
     * Reduction also supports |angle| <= 2^20; this is not a general libm
     * huge-argument reducer. Fail closed outside that documented domain. */
    if ((Sf64Bits(d).v & UINT64_C(0x7fffffffffffffff)) > UINT64_C(0x4130000000000000))
        return DBits(UINT64_C(0x7ff8000000000000));
    q = f64_to_f128(Sf64Bits(d));
    pi2 = f128_mul(QPi(), QInt(2));
    n = (int32_t)f128_to_i32(f128_div(q, pi2), softfloat_round_near_even, false);
    q = f128_sub(q, f128_mul(QInt(n), pi2));
    zz = QNeg(f128_mul(q, q));
    term = sum = fCos ? QInt(1) : q;
    for (i = fCos ? 2 : 3; i < 160; i += 2) {
        term = f128_div(f128_mul(term, zz), QInt(i * (i - 1)));
        next = f128_add(sum, term);
        if (f128_eq(next, sum))
            break;
        sum = next;
    }
    return DQ(sum);
}

double Sf64Sin(double d) { return DSinCos(d, 0); }
double Sf64Cos(double d) { return DSinCos(d, 1); }
