#pragma once
// CUBE_SCRIPTING: whether the build has the interface script engine, abstr::Machine and cube::System
// (GameController's constructor has it check db.dat). The script's values are 32-bit ints that
// also carry pointers and handles, so x64 builds go without it; /DCUBE_SCRIPTING=0 leaves it out
// of any build.
#ifndef CUBE_SCRIPTING
#ifdef _WIN64
#define CUBE_SCRIPTING 0
#else
#define CUBE_SCRIPTING 1
#endif
#endif
