#include "DensityCoder.h"

#include "DensityBridge.h"

#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace NDensity7z {
namespace {

constexpr Byte kMagic[4] = {'D', '7', 'D', '1'};
constexpr Byte kFormatVersion = 1;
constexpr Byte kDefaultChunkLog2 = 22;  // 4 MiB
constexpr Byte kMinimumChunkLog2 = 16;  // 64 KiB
constexpr Byte kMaximumChunkLog2 = 26;  // 64 MiB safety ceiling
constexpr UInt32 kStoredFlag = static_cast<UInt32>(1) << 31;
constexpr UInt32 kPackedSizeMask = ~kStoredFlag;
constexpr UInt32 kHeaderSize = 8;
constexpr UInt32 kFrameHeaderSize = 8;

void StoreUInt32LE(Byte *destination, UInt32 value) {
  destination[0] = static_cast<Byte>(value);
  destination[1] = static_cast<Byte>(value >> 8);
  destination[2] = static_cast<Byte>(value >> 16);
  destination[3] = static_cast<Byte>(value >> 24);
}

UInt32 LoadUInt32LE(const Byte *source) {
  return static_cast<UInt32>(source[0]) |
         (static_cast<UInt32>(source[1]) << 8) |
         (static_cast<UInt32>(source[2]) << 16) |
         (static_cast<UInt32>(source[3]) << 24);
}

HRESULT ReadUpTo(ISequentialInStream *stream,
                 Byte *buffer,
                 UInt32 capacity,
                 UInt32 &size,
                 UInt64 &processed_total) {
  size = 0;
  while (size < capacity) {
    UInt32 processed = 0;
    const UInt32 remaining = capacity - size;
    const HRESULT result = stream->Read(buffer + size, remaining, &processed);
    if (processed > remaining)
      return E_FAIL;

    size += processed;
    processed_total += processed;

    if (result != S_OK)
      return result;
    if (processed == 0)
      return S_OK;
  }
  return S_OK;
}

HRESULT ReadExact(ISequentialInStream *stream,
                  Byte *buffer,
                  UInt32 size,
                  UInt64 &processed_total) {
  UInt32 position = 0;
  while (position < size) {
    UInt32 processed = 0;
    const UInt32 remaining = size - position;
    const HRESULT result = stream->Read(buffer + position, remaining, &processed);
    if (processed > remaining)
      return E_FAIL;

    position += processed;
    processed_total += processed;

    if (result != S_OK)
      return result;
    if (position == size)
      return S_OK;
    if (processed == 0)
      return S_FALSE;
  }
  return S_OK;
}

HRESULT WriteAll(ISequentialOutStream *stream,
                 const Byte *buffer,
                 UInt32 size,
                 UInt64 &processed_total) {
  UInt32 position = 0;
  while (position < size) {
    UInt32 processed = 0;
    const UInt32 remaining = size - position;
    const HRESULT result = stream->Write(buffer + position, remaining, &processed);
    if (processed > remaining)
      return E_FAIL;

    position += processed;
    processed_total += processed;

    if (result != S_OK)
      return result;
    if (processed == 0)
      return E_FAIL;
  }
  return S_OK;
}

HRESULT ReportProgress(ICompressProgressInfo *progress,
                       const UInt64 &input_size,
                       const UInt64 &output_size) {
  return progress ? progress->SetRatioInfo(&input_size, &output_size) : S_OK;
}

HRESULT EncodeStream(UInt32 algorithm,
                     ISequentialInStream *in_stream,
                     ISequentialOutStream *out_stream,
                     ICompressProgressInfo *progress) {
  const UInt32 chunk_size = static_cast<UInt32>(1) << kDefaultChunkLog2;
  const size_t packed_capacity = density_plugin_bound(algorithm, chunk_size);
  if (packed_capacity == 0 ||
      packed_capacity > static_cast<size_t>(kPackedSizeMask) ||
      packed_capacity > static_cast<size_t>(std::numeric_limits<UInt32>::max()))
    return E_FAIL;

  std::vector<Byte> input(chunk_size);
  std::vector<Byte> packed(packed_capacity);

  UInt64 input_processed = 0;
  UInt64 output_processed = 0;

  Byte header[kHeaderSize] = {
      kMagic[0], kMagic[1], kMagic[2], kMagic[3],
      kFormatVersion, static_cast<Byte>(algorithm), kDefaultChunkLog2, 0};
  HRESULT result = WriteAll(out_stream, header, kHeaderSize, output_processed);
  if (result != S_OK)
    return result;

  for (;;) {
    UInt32 input_size = 0;
    result = ReadUpTo(in_stream, input.data(), chunk_size, input_size, input_processed);
    if (result != S_OK)
      return result;
    if (input_size == 0)
      break;

    size_t packed_size = 0;
    const int32_t status = density_plugin_encode(
        algorithm,
        input.data(),
        input_size,
        packed.data(),
        packed.size(),
        &packed_size);
    if (status != DENSITY_STATUS_OK || packed_size == 0 ||
        packed_size > packed.size())
      return E_FAIL;

    const bool store_raw = packed_size >= input_size;
    const Byte *payload = store_raw ? input.data() : packed.data();
    const UInt32 payload_size = store_raw
        ? input_size
        : static_cast<UInt32>(packed_size);
    const UInt32 packed_field = payload_size | (store_raw ? kStoredFlag : 0);

    Byte frame_header[kFrameHeaderSize];
    StoreUInt32LE(frame_header, input_size);
    StoreUInt32LE(frame_header + 4, packed_field);

    result = WriteAll(out_stream, frame_header, kFrameHeaderSize, output_processed);
    if (result != S_OK)
      return result;
    result = WriteAll(out_stream, payload, payload_size, output_processed);
    if (result != S_OK)
      return result;

    result = ReportProgress(progress, input_processed, output_processed);
    if (result != S_OK)
      return result;
  }

  Byte terminator[kFrameHeaderSize] = {};
  result = WriteAll(out_stream, terminator, kFrameHeaderSize, output_processed);
  if (result != S_OK)
    return result;
  return ReportProgress(progress, input_processed, output_processed);
}

HRESULT DecodeStream(UInt32 algorithm,
                     bool finish_mode,
                     ISequentialInStream *in_stream,
                     ISequentialOutStream *out_stream,
                     const UInt64 *in_size,
                     const UInt64 *out_size,
                     ICompressProgressInfo *progress) {
  UInt64 input_processed = 0;
  UInt64 output_processed = 0;

  // In partial mode, a caller requesting no output does not require stream
  // validation or input consumption. Full mode still validates the framing.
  if (!finish_mode && out_size && *out_size == 0)
    return ReportProgress(progress, input_processed, output_processed);

  Byte header[kHeaderSize];
  HRESULT result = ReadExact(in_stream, header, kHeaderSize, input_processed);
  if (result != S_OK)
    return result == S_FALSE ? S_FALSE : result;

  if (std::memcmp(header, kMagic, sizeof(kMagic)) != 0 ||
      header[4] != kFormatVersion ||
      header[5] != static_cast<Byte>(algorithm) ||
      header[6] < kMinimumChunkLog2 ||
      header[6] > kMaximumChunkLog2 ||
      header[7] != 0)
    return S_FALSE;

  const UInt32 chunk_size = static_cast<UInt32>(1) << header[6];
  const size_t packed_capacity = density_plugin_bound(algorithm, chunk_size);
  if (packed_capacity == 0 ||
      packed_capacity > static_cast<size_t>(kPackedSizeMask) ||
      packed_capacity > static_cast<size_t>(std::numeric_limits<UInt32>::max()))
    return S_FALSE;

  std::vector<Byte> raw(chunk_size);
  std::vector<Byte> packed(packed_capacity);

  for (;;) {
    // Partial decoding is allowed unless the caller requested finish mode.
    if (!finish_mode && out_size && output_processed == *out_size)
      return ReportProgress(progress, input_processed, output_processed);

    Byte frame_header[kFrameHeaderSize];
    result = ReadExact(in_stream, frame_header, kFrameHeaderSize, input_processed);
    if (result != S_OK)
      return result == S_FALSE ? S_FALSE : result;

    const UInt32 raw_size = LoadUInt32LE(frame_header);
    const UInt32 packed_field = LoadUInt32LE(frame_header + 4);
    if (raw_size == 0 && packed_field == 0)
      break;
    if (raw_size == 0 || packed_field == 0 || raw_size > chunk_size)
      return S_FALSE;

    const bool stored = (packed_field & kStoredFlag) != 0;
    const UInt32 packed_size = packed_field & kPackedSizeMask;
    if (packed_size == 0)
      return S_FALSE;
    if (stored) {
      if (packed_size != raw_size)
        return S_FALSE;
    } else if (packed_size > packed_capacity) {
      return S_FALSE;
    }

    UInt32 write_size = raw_size;
    bool stop_after_frame = false;
    if (out_size) {
      if (output_processed > *out_size)
        return S_FALSE;
      const UInt64 remaining = *out_size - output_processed;
      if (static_cast<UInt64>(raw_size) > remaining) {
        if (finish_mode)
          return S_FALSE;
        write_size = static_cast<UInt32>(remaining);
        stop_after_frame = true;
      }
    }

    if (stored) {
      result = ReadExact(in_stream, raw.data(), raw_size, input_processed);
      if (result != S_OK)
        return result == S_FALSE ? S_FALSE : result;
    } else {
      result = ReadExact(in_stream, packed.data(), packed_size, input_processed);
      if (result != S_OK)
        return result == S_FALSE ? S_FALSE : result;

      size_t decoded_size = 0;
      const int32_t status = density_plugin_decode(
          algorithm,
          packed.data(),
          packed_size,
          raw.data(),
          raw_size,
          &decoded_size);
      if (status != DENSITY_STATUS_OK || decoded_size != raw_size)
        return S_FALSE;
    }

    result = WriteAll(out_stream, raw.data(), write_size, output_processed);
    if (result != S_OK)
      return result;

    result = ReportProgress(progress, input_processed, output_processed);
    if (result != S_OK)
      return result;
    if (stop_after_frame)
      return S_OK;
  }

  if (out_size && output_processed != *out_size)
    return S_FALSE;
  if (in_size && input_processed != *in_size)
    return S_FALSE;

  return ReportProgress(progress, input_processed, output_processed);
}

}  // namespace

Z7_COM7F_IMF(CDensityEncoder::Code(
    ISequentialInStream *in_stream,
    ISequentialOutStream *out_stream,
    const UInt64 * /* in_size */,
    const UInt64 * /* out_size */,
    ICompressProgressInfo *progress)) {
  if (!in_stream || !out_stream)
    return E_INVALIDARG;
  try {
    return EncodeStream(_algorithm, in_stream, out_stream, progress);
  } catch (const std::bad_alloc &) {
    return E_OUTOFMEMORY;
  } catch (...) {
    return E_FAIL;
  }
}

Z7_COM7F_IMF(CDensityDecoder::Code(
    ISequentialInStream *in_stream,
    ISequentialOutStream *out_stream,
    const UInt64 *in_size,
    const UInt64 *out_size,
    ICompressProgressInfo *progress)) {
  if (!in_stream || !out_stream)
    return E_INVALIDARG;
  try {
    return DecodeStream(
        _algorithm, _finishMode, in_stream, out_stream, in_size, out_size, progress);
  } catch (const std::bad_alloc &) {
    return E_OUTOFMEMORY;
  } catch (...) {
    return S_FALSE;
  }
}

Z7_COM7F_IMF(CDensityDecoder::SetFinishMode(UInt32 finish_mode)) {
  _finishMode = finish_mode != 0;
  return S_OK;
}

}  // namespace NDensity7z
