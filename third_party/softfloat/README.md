# Berkeley SoftFloat 3e

Unmodified `source/` and `COPYING.txt` from https://www.jhauser.us/arithmetic/SoftFloat-3e.zip.
Archive SHA-256: `21130ce885d35c1fe73fc1e1bf2244178167e05c6747cad5f450cc991714c746`.
The local CMake list uses the upstream FAST_INT64 build with the 8086-SSE
specialization, portable integer primitives, process-global state and native
endianness. Stars uses nearest-even arithmetic and 80-bit extended precision.
No MPFR or GMP code is included. Distributions must include COPYING.txt.
The game is single-threaded; numerical modes must remain nearest-even with
`extF80_roundingPrecision == 80`. Exception flags are not used by the game.
