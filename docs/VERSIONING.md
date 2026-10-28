# Versioning

Stars! has a product version and a separate file format version.

## Product version

`main` uses the original-style revision letters. The next release is
`2.6k-x64`, tagged `v2.6k-x64`. Later releases advance the letter:
`v2.6l-x64`, `v2.6m-x64`, and so on. Earlier tags (`v2.8.0`, `v2.8.1`,
`v2.9.0`) remain historical releases. The former `2.8` maintenance branch
is named `2.6j-linux`.

`cmake/version.cmake` generates `<build>/generated/version.h` on every build.
`STARS_VERSION_STRING` is the exact product version; `STARS_VERSION_DISPLAY`
adds the build platform for a non-Windows host. Examples:

| Checkout | Version string | Windows display | Linux host display | Build |
| --- | --- | --- | --- | --- |
| Clean `v2.6k-x64` tag | `2.6k-x64` | `2.6k-x64` | `2.6k-linux-x64` | 0 |
| 12 commits after `v2.6k-x64` | `2.6l-x64-dev.12+g1a2b3c` | `2.6l-x64 (2.6l-x64-dev.12+g1a2b3c)` | `2.6l-linux-x64 (2.6l-x64-dev.12+g1a2b3c)` | 12 |
| Same, with tracked changes | `2.6l-x64-dev.12+g1a2b3c.dirty` | `2.6l-x64 (2.6l-x64-dev.12+g1a2b3c.dirty)` | `2.6l-linux-x64 (2.6l-x64-dev.12+g1a2b3c.dirty)` | 12 |
| Before the first lettered tag | `2.6k-x64-dev.N+g…` | `2.6k-x64 (2.6k-x64-dev.N+g…)` | `2.6k-linux-x64 (2.6k-x64-dev.N+g…)` | N, counted from `v2.9.0` |

The Windows `VERSIONINFO` numeric fields are `2,6,11,BUILD` for revision k;
the third field advances with the letter. The string fields carry the
original-style name. `SzVersion` shows it in the About box, splash screen
and report headers. `stars-host --version` uses the host display string.
The platform name is supplied by CMake as `STARS_VERSION_PLATFORM`.

For a source archive without Git history, set `STARS_VERSION=2.6k-x64` in
the environment to stamp a release build. Without that setting, the version
is `2.6k-x64-dev.0+gunknown`.

## Releasing

After the release commit is on `main`:

```sh
git tag v2.6k-x64
git push origin v2.6k-x64
```

The release workflow checks that the tag builds the exact version string
`2.6k-x64` and publishes the Windows executable, help file and Linux host.
Every push to `main` updates the rolling `latest` prerelease. The legacy
tags and the `2.6j-linux` branch remain available.

## File format version

Saves, turn files and race files carry their own version in `RTBOF`
(`verMajor`/`verMinor`/`verInc`). The original wrote 2.83 (`save.c`) and
refused files from 2.84 or later (`file.c`, `mdi.c`, `save.c`). The native
builds write 2.84 and read 2.49 through 2.84: they pick up games, turns
and races from 2.6j/2.7, while 2.6j/2.7 refuse the newer files. The records
themselves are unchanged. This number is independent of the product name.
