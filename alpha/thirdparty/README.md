# Third-party sources

These are the pinned upstream sources bundled by the alpha reconstruction.
manifest.json records archive URLs, hashes, source lists and include directories.
FreeType 2.4.1, libogg 1.3.0, libvorbis 1.3.3, SQLite 3.7.15.2 and pugixml 0.9
are built as static libraries. pugixml uses wide-character mode.

SQLite's compressed amalgamation is expanded into the build directory by Python.
The pugixml header uses standard library headers instead of obsolete declarations
inside namespace std, for compatibility with current C++ compilers. Size and offset assertions and two unused FreeType documentation headers are
omitted. Other bundled library files retain their upstream contents and licenses.

FreeImage 3.18.0 and zlib 1.3.1 are fetched from SHA-256-pinned upstream archives
at configure time; their sources and licenses live in the build directory.
