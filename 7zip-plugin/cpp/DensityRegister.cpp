#include "DensityCoder.h"
#include "DensityBridge.h"

#include "7zip/Common/RegisterCodec.h"

namespace NDensity7z {
namespace {

// Private method IDs following 7-Zip's random-ID layout:
// 3F + developer ID 3C EF AE 04 60 + two-byte method number.
// Freeze these values once archives are distributed.
constexpr CMethodId kMethodIdChameleon = 0x3F3CEFAE04600001ULL;
constexpr CMethodId kMethodIdCheetah = 0x3F3CEFAE04600002ULL;
constexpr CMethodId kMethodIdLion = 0x3F3CEFAE04600003ULL;

void *CreateChameleonDecoder() {
  return static_cast<ICompressCoder *>(
      new CDensityDecoder(DENSITY_ALGORITHM_CHAMELEON));
}
void *CreateChameleonEncoder() {
  return static_cast<ICompressCoder *>(
      new CDensityEncoder(DENSITY_ALGORITHM_CHAMELEON));
}
void *CreateCheetahDecoder() {
  return static_cast<ICompressCoder *>(
      new CDensityDecoder(DENSITY_ALGORITHM_CHEETAH));
}
void *CreateCheetahEncoder() {
  return static_cast<ICompressCoder *>(
      new CDensityEncoder(DENSITY_ALGORITHM_CHEETAH));
}
void *CreateLionDecoder() {
  return static_cast<ICompressCoder *>(
      new CDensityDecoder(DENSITY_ALGORITHM_LION));
}
void *CreateLionEncoder() {
  return static_cast<ICompressCoder *>(
      new CDensityEncoder(DENSITY_ALGORITHM_LION));
}

}  // namespace

REGISTER_CODECS_VAR {
    {CreateChameleonDecoder, CreateChameleonEncoder,
     kMethodIdChameleon, "DensityChameleon", 1, false},
    {CreateCheetahDecoder, CreateCheetahEncoder,
     kMethodIdCheetah, "DensityCheetah", 1, false},
    {CreateLionDecoder, CreateLionEncoder,
     kMethodIdLion, "DensityLion", 1, false},
};

REGISTER_CODECS(Density)

}  // namespace NDensity7z
