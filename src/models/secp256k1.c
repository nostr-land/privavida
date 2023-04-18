// I was getting into all sorts of trouble fighting
// CMake and XCode in order to build secp256k1.
// The only thing I found that works reasonably is
// to include secp256k1 into the libprivavida-core.a
// object. So that's what's happening here.

#define ENABLE_MODULE_EXTRAKEYS
#define ENABLE_MODULE_SCHNORRSIG

#include <secp256k1/src/secp256k1.c>
#include <secp256k1/src/precomputed_ecmult.c>
#include <secp256k1/src/precomputed_ecmult_gen.c>
