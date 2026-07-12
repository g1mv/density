#ifndef DENSITY_7ZIP_BRIDGE_H
#define DENSITY_7ZIP_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum DensityPluginAlgorithm {
  DENSITY_ALGORITHM_CHAMELEON = 1,
  DENSITY_ALGORITHM_CHEETAH = 2,
  DENSITY_ALGORITHM_LION = 3
};

enum DensityPluginStatus {
  DENSITY_STATUS_OK = 0,
  DENSITY_STATUS_INVALID_ARGUMENT = 1,
  DENSITY_STATUS_UNSUPPORTED_ALGORITHM = 2,
  DENSITY_STATUS_CODEC_ERROR = 3,
  DENSITY_STATUS_PANIC = 4
};

size_t density_plugin_bound(uint32_t algorithm, size_t input_size);

int32_t density_plugin_encode(
    uint32_t algorithm,
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *written);

int32_t density_plugin_decode(
    uint32_t algorithm,
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *written);

#ifdef __cplusplus
}
#endif

#endif
