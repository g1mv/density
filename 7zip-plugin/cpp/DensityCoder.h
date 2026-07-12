#ifndef DENSITY_7ZIP_CODER_H
#define DENSITY_7ZIP_CODER_H

#include "Common/MyCom.h"
#include "7zip/ICoder.h"

namespace NDensity7z {

Z7_CLASS_IMP_COM_1(
    CDensityEncoder,
    ICompressCoder)
  UInt32 _algorithm;

public:
  explicit CDensityEncoder(UInt32 algorithm) : _algorithm(algorithm) {}
};

Z7_CLASS_IMP_COM_2(
    CDensityDecoder,
    ICompressCoder,
    ICompressSetFinishMode)
  UInt32 _algorithm;
  bool _finishMode;

public:
  explicit CDensityDecoder(UInt32 algorithm)
      : _algorithm(algorithm), _finishMode(false) {}
};

}  // namespace NDensity7z

#endif
