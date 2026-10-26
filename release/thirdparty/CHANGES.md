# Local changes

`manifest.json` records upstream provenance and original file hashes. These
changes support the portable build and the requested source cleanup:

- pugixml's `pugixml.hpp` includes the standard string, stream forward declaration
  and iterator headers instead of declaring implementation types inside `std`.
- Three pugixml `STATIC_ASSERT` checks involving `sizeof` or `offsetof` were
  removed; its algorithmic assertions remain.
- Steamworks header include spelling matches the case of the bundled filenames.
  Its Windows integer typedefs use standard C++ types, and GCC on Windows no
  longer selects its POSIX calling-convention branch.
- The comment-only FreeType headers `ftchapters.h` and `afangles.h` were removed.
  Neither was included by the built source.

FreeImage is fetched into the build directory. `cmake/FreeImage.cmake` adapts
its upstream source list, excludes OpenEXR table-generator programs from the
library, retains the SDK's GUID header, replaces POSIX aligned
allocation with the Windows equivalent in OpenEXR, uses a pointer-sized cast for
an OpenEXR alignment check, and selects the Windows CRT byte-swap function in
JPEG-XR. FreeImage uses C++14 to support the bundled OpenEXR exception declarations.
The game zlib is built with upstream `Z_PREFIX` plus a private name for its old
`z_errmsg` table, keeping its symbols separate from FreeImage's newer zlib.
The game uses its bundled zlib's portable C implementation instead of the
optional MASM optimizations. Upstream assembler source is retained for reference.
