// Software arithmetic. Native float/double carry IEEE bits; they do not evaluate operations.
// Keep SoftFloat's nearest-even mode and extF80_roundingPrecision == 80.
#ifndef STARS_SFNUM_H
#define STARS_SFNUM_H
#include <float.h>
#include <stdint.h>
#include <string.h>
#include "softfloat.h"
typedef extFloat80_t SF80;
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53, "binary64 carrier required");
_Static_assert(sizeof(float) == 4 && FLT_MANT_DIG == 24, "binary32 carrier required");
static inline float32_t Sf32Bits(float d) {
    float32_t r;
    memcpy(&r.v, &d, sizeof(d));
    return r;
}
static inline float Sf32Value(float32_t r) {
    float d;
    memcpy(&d, &r.v, sizeof(d));
    return d;
}
static inline float64_t Sf64Bits(double d) {
    float64_t r;
    memcpy(&r.v, &d, sizeof(d));
    return r;
}
static inline double Sf64Value(float64_t r) {
    double d;
    memcpy(&d, &r.v, sizeof(d));
    return d;
}
static inline float Sf32Add(float a, float b) { return Sf32Value(f32_add(Sf32Bits(a), Sf32Bits(b))); }
static inline float Sf32Sub(float a, float b) { return Sf32Value(f32_sub(Sf32Bits(a), Sf32Bits(b))); }
static inline float Sf32Mul(float a, float b) { return Sf32Value(f32_mul(Sf32Bits(a), Sf32Bits(b))); }
static inline float Sf32Div(float a, float b) { return Sf32Value(f32_div(Sf32Bits(a), Sf32Bits(b))); }
static inline int   Sf32Eq(float a, float b) { return f32_eq(Sf32Bits(a), Sf32Bits(b)); }
static inline int   Sf32Lt(float a, float b) { return f32_lt(Sf32Bits(a), Sf32Bits(b)); }
static inline int   Sf32Le(float a, float b) { return f32_le(Sf32Bits(a), Sf32Bits(b)); }
static inline float Sf32Neg(float a) {
    float32_t r = Sf32Bits(a);
    r.v ^= UINT32_C(0x80000000);
    return Sf32Value(r);
}
static inline float    Sf32From64(double a) { return Sf32Value(f64_to_f32(Sf64Bits(a))); }
static inline float    Sf32From80(SF80 a) { return Sf32Value(extF80_to_f32(a)); }
static inline float    Sf32FromI32(int32_t a) { return Sf32Value(i32_to_f32(a)); }
static inline int32_t  Sf32ToI32(float a) { return f32_to_i32_r_minMag(Sf32Bits(a), false); }
static inline float    Sf32FromU32(uint32_t a) { return Sf32Value(ui32_to_f32(a)); }
static inline uint32_t Sf32ToU32(float a) { return f32_to_ui32_r_minMag(Sf32Bits(a), false); }
static inline float    Sf32FromI64(int64_t a) { return Sf32Value(i64_to_f32(a)); }
static inline int64_t  Sf32ToI64(float a) { return f32_to_i64_r_minMag(Sf32Bits(a), false); }
static inline float    Sf32FromU64(uint64_t a) { return Sf32Value(ui64_to_f32(a)); }
static inline uint64_t Sf32ToU64(float a) { return f32_to_ui64_r_minMag(Sf32Bits(a), false); }
static inline double   Sf64Add(double a, double b) { return Sf64Value(f64_add(Sf64Bits(a), Sf64Bits(b))); }
static inline double   Sf64Sub(double a, double b) { return Sf64Value(f64_sub(Sf64Bits(a), Sf64Bits(b))); }
static inline double   Sf64Mul(double a, double b) { return Sf64Value(f64_mul(Sf64Bits(a), Sf64Bits(b))); }
static inline double   Sf64Div(double a, double b) { return Sf64Value(f64_div(Sf64Bits(a), Sf64Bits(b))); }
static inline int      Sf64Eq(double a, double b) { return f64_eq(Sf64Bits(a), Sf64Bits(b)); }
static inline int      Sf64Lt(double a, double b) { return f64_lt(Sf64Bits(a), Sf64Bits(b)); }
static inline int      Sf64Le(double a, double b) { return f64_le(Sf64Bits(a), Sf64Bits(b)); }
static inline double   Sf64Neg(double a) {
    float64_t r = Sf64Bits(a);
    r.v ^= UINT64_C(0x8000000000000000);
    return Sf64Value(r);
}
static inline double   Sf64From32(float a) { return Sf64Value(f32_to_f64(Sf32Bits(a))); }
static inline double   Sf64From80(SF80 a) { return Sf64Value(extF80_to_f64(a)); }
static inline double   Sf64FromI32(int32_t a) { return Sf64Value(i32_to_f64(a)); }
static inline int32_t  Sf64ToI32(double a) { return f64_to_i32_r_minMag(Sf64Bits(a), false); }
static inline double   Sf64FromU32(uint32_t a) { return Sf64Value(ui32_to_f64(a)); }
static inline uint32_t Sf64ToU32(double a) { return f64_to_ui32_r_minMag(Sf64Bits(a), false); }
static inline double   Sf64FromI64(int64_t a) { return Sf64Value(i64_to_f64(a)); }
static inline int64_t  Sf64ToI64(double a) { return f64_to_i64_r_minMag(Sf64Bits(a), false); }
static inline double   Sf64FromU64(uint64_t a) { return Sf64Value(ui64_to_f64(a)); }
static inline uint64_t Sf64ToU64(double a) { return f64_to_ui64_r_minMag(Sf64Bits(a), false); }
static inline SF80     Sf80Add(SF80 a, SF80 b) { return extF80_add(a, b); }
static inline SF80     Sf80Sub(SF80 a, SF80 b) { return extF80_sub(a, b); }
static inline SF80     Sf80Mul(SF80 a, SF80 b) { return extF80_mul(a, b); }
static inline SF80     Sf80Div(SF80 a, SF80 b) { return extF80_div(a, b); }
static inline int      Sf80Eq(SF80 a, SF80 b) { return extF80_eq(a, b); }
static inline int      Sf80Lt(SF80 a, SF80 b) { return extF80_lt(a, b); }
static inline int      Sf80Le(SF80 a, SF80 b) { return extF80_le(a, b); }
static inline SF80     Sf80Neg(SF80 a) {
    a.signExp ^= 0x8000;
    return a;
}
static inline SF80     Sf80From32(float a) { return f32_to_extF80(Sf32Bits(a)); }
static inline SF80     Sf80From64(double a) { return f64_to_extF80(Sf64Bits(a)); }
static inline SF80     Sf80FromI32(int32_t a) { return i32_to_extF80(a); }
static inline int32_t  Sf80ToI32(SF80 a) { return extF80_to_i32_r_minMag(a, false); }
static inline SF80     Sf80FromU32(uint32_t a) { return ui32_to_extF80(a); }
static inline uint32_t Sf80ToU32(SF80 a) { return extF80_to_ui32_r_minMag(a, false); }
static inline SF80     Sf80FromI64(int64_t a) { return i64_to_extF80(a); }
static inline int64_t  Sf80ToI64(SF80 a) { return extF80_to_i64_r_minMag(a, false); }
static inline SF80     Sf80FromU64(uint64_t a) { return ui64_to_extF80(a); }
static inline uint64_t Sf80ToU64(SF80 a) { return extF80_to_ui64_r_minMag(a, false); }
double                 Sf64Sqrt(double d);
double                 Sf64Pow(double x, double y);
double                 Sf64Floor(double d);
double                 Sf64Hypot(double x, double y);
double                 Sf64Atan2(double y, double x);
// UI angle functions: |d| <= 2^20 radians; larger arguments return NaN.
double                 Sf64Sin(double d);
double                 Sf64Cos(double d);
#endif
