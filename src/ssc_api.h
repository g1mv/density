/*
 * Centaurean libssc
 * http://www.libssc.net
 *
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Centaurean nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 18/10/13 22:41
 */

#ifndef SSC_API_H
#define SSC_API_H

#include <stdint.h>
#include <stdbool.h>



/***********************************************************************************************************************
 *                                                                                                                     *
 * SSC structures useful for the API                                                                                   *
 *                                                                                                                     *
 ***********************************************************************************************************************/

typedef uint8_t ssc_byte;
typedef bool ssc_bool;

typedef enum {
    SSC_COMPRESSION_MODE_COPY = 0,
    SSC_COMPRESSION_MODE_CHAMELEON = 1,
    SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON = 2,
} SSC_COMPRESSION_MODE;

typedef enum {
    SSC_ENCODE_OUTPUT_TYPE_DEFAULT = 0,
    SSC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER = 1,
    SSC_ENCODE_OUTPUT_TYPE_WITHOUT_FOOTER = 2,
    SSC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER = 3
} SSC_ENCODE_OUTPUT_TYPE;

typedef enum {
    SSC_BLOCK_TYPE_DEFAULT = 0,
    SSC_BLOCK_TYPE_NO_HASHSUM_INTEGRITY_CHECK = 1
} SSC_BLOCK_TYPE;

typedef enum {
    SSC_BUFFERS_STATE_OK = 0,                                       // ready to continue
    SSC_BUFFERS_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL,                // output buffer size is too small
    SSC_BUFFERS_STATE_ERROR_INVALID_STATE                           // error during processing
} SSC_BUFFERS_STATE;

typedef enum {
    SSC_STREAM_STATE_READY = 0,                                     // ready to continue
    SSC_STREAM_STATE_STALL_ON_INPUT_BUFFER,                         // input buffer has been completely read
    SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER,                        // there is not enought space left in the output buffer to continue
    SSC_STREAM_STATE_ERROR_INPUT_BUFFER_SIZE_NOT_MULTIPLE_OF_32,    // size of input buffer is no a multiple of 32
    SSC_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL,                 // output buffer size is too small
    SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE                   // error during processing
} SSC_STREAM_STATE;

typedef struct {
    ssc_byte version[3];
    ssc_byte compressionMode;
    ssc_byte blockType;
    ssc_byte parameters[7];
} ssc_main_header;

typedef struct {
    ssc_byte* pointer;
    uint_fast64_t position;
    uint_fast64_t size;
} ssc_byte_buffer;

typedef struct {
    ssc_byte_buffer in;
    uint_fast64_t* in_total_read;

    ssc_byte_buffer out;
    uint_fast64_t* out_total_written;

    void* internal_state;
} ssc_stream;



/***********************************************************************************************************************
 *                                                                                                                     *
 * SSC version information                                                                                             *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Returns the major ssc version
 */
uint8_t ssc_version_major();

/*
 * Returns the minor ssc version
 */
uint8_t ssc_version_minor();

/*
 * Returns the ssc revision
 */
uint8_t ssc_version_revision();



/***********************************************************************************************************************
 *                                                                                                                     *
 * SSC byte buffer utilities                                                                                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Rewind an SSC byte buffer
 *
 * @param byte_buffer the SSC byte buffer to rewind (its position is set to zero)
 */
void ssc_byte_buffer_rewind(ssc_byte_buffer* byte_buffer);



/***********************************************************************************************************************
 *                                                                                                                     *
 * SSC stream API functions                                                                                            *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Prepare a stream with the encapsulated input/output buffers. This function *must* be called upon changing either buffer pointers / sizes.
 *
 * @param stream the stream
 * @param input_buffer a buffer of bytes
 * @param input_size the size in bytes of input_buffer
 * @param output_buffer a buffer of bytes
 * @param output_size the size of output_buffer, must be at least SSC_STREAM_MINIMUM_OUT_BUFFER_SIZE
 * @param mem_alloc a pointer to a memory allocation function. If NULL, the standard malloc(size_t) is used.
 * @param mem_free a pointer to a memory freeing function. If NULL, the standard free(void*) is used.
 */
SSC_STREAM_STATE ssc_stream_prepare(ssc_stream *stream, uint8_t* input_buffer, const uint_fast64_t input_size, uint8_t* output_buffer, const uint_fast64_t output_size, void *(*mem_alloc)(), void (*mem_free)(void *));

/*
 * Initialize compression
 *
 * @param stream the stream
 * @param compression_mode the compression mode, can take the following values :
 *      SSC_COMPRESSION_MODE_COPY
 *      SSC_COMPRESSION_MODE_FASTEST
 *      SSC_COMPRESSION_MODE_DUAL_PASS
 * @param output_type the format of data output by encoding.
 *      EXPERTS ONLY ! If unsure, use SSC_ENCODE_OUTPUT_TYPE_DEFAULT.
 *      Any other option will create output data which is *NOT* directly decompressible by the API. This can be used for parallelizing SSC.
 * @param block_type the type of data blocks SSC will generate.
 *      EXPERTS ONLY ! If you're unsure use SSC_BLOCK_TYPE_DEFAULT.
 *      The option SSC_BLOCK_TYPE_NO_HASHSUM_INTEGRITY_CHECK basically makes the block footer size zero, and removes data integrity checks in the encoded output.
 *      It can be useful in network streaming situations, where data integrity is already checked by the protocol (TCP/IP for example), and the flush option in ssc_stream_compress is often set,
 *      as the absence of block footer will enhance compression ratio.
 */
SSC_STREAM_STATE ssc_stream_compress_init(ssc_stream *stream, const SSC_COMPRESSION_MODE compression_mode, const SSC_ENCODE_OUTPUT_TYPE output_type, const SSC_BLOCK_TYPE block_type);

/*
 * Stream decompression initialization
 *
 * @param stream the stream
 */
SSC_STREAM_STATE ssc_stream_decompress_init(ssc_stream *stream);

/*
 * Stream compression function, has to be called repetitively.
 * When the dataset in the input buffer is the last, flush has to be true. Otherwise it should be false at all times.
 *
 * @param stream the stream
 *      Please note that the input buffer size, if flush is false, *must* be a multiple of 32 otherwise an error will be returned.
 * @param flush a boolean indicating flush behaviour
 *      If set to true, this will ensure that every byte from the input buffer will have its counterpart in the output buffer.
 *      flush has to be true when the presented data is the last (end of a file for example).
 *      It can also be set to true multiple times to handle network streaming for example. In that case, please also check
 *      the block_type parameter of ssc_stream_compress_init to enable a better compression ratio. It is also worth noting that
 *      the *best* input buffer size for compression ratio matters should be a multiple of 256, any other size will also work but will
 *      incur a less than optimal compression ratio.
 */
SSC_STREAM_STATE ssc_stream_compress(ssc_stream *stream, const ssc_bool flush);

/*
 * Stream decompression function, has to be called repetitively.
 * When the dataset in the input buffer is the last, flush has to be true. Otherwise it should be false at all times.
 *
 * @param stream the stream
 * @param flush a boolean indicating flush behaviour
 *      If set to true, this will ensure that every byte from the input buffer will have its counterpart in the output buffer.
 *      flush has to be true when the presented data is the last (end of a file for example)
 *      It can also be set to true multiple times to handle network streaming for example.
 */
SSC_STREAM_STATE ssc_stream_decompress(ssc_stream *stream, const ssc_bool flush);

/*
 * Call once processing is finished, to clear up the environment and release eventual allocated memory.
 *
 * @param stream the stream
 */
SSC_STREAM_STATE ssc_stream_compress_finish(ssc_stream *stream);

/*
 * Call once processing is finished, to clear up the environment and release eventual allocated memory.
 *
 * @param stream the stream
 */
SSC_STREAM_STATE ssc_stream_decompress_finish(ssc_stream *stream);

/*
 * Returns the header that was read during ssc_stream_decompress_init.
 *
 * @param stream the stream
 * @param header the header returned
 */
SSC_STREAM_STATE ssc_stream_decompress_utilities_get_header(ssc_stream* stream, ssc_main_header* header);



/***********************************************************************************************************************
 *                                                                                                                     *
 * SSC buffers API functions                                                                                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Returns the max compressed length possible (with incompressible data)
 *
 * @param result a pointer to return the resulting length to
 * @param in_length, the length of the input data to compress
 * @param compression_mode the compression mode to be used
 */
SSC_BUFFERS_STATE ssc_buffers_max_compressed_length(uint_fast64_t * result, uint_fast64_t in_length, const SSC_COMPRESSION_MODE compression_mode);

/*
 * Buffers compression function
 *
 * @param total_written a pointer to return the total bytes written to, can be NULL for no return
 * @param in the input buffer to compress
 * @param in_size the size of the input buffer in bytes
 * @param out the output buffer
 * @param out_size the size of the output buffer in bytes
 * @param compression_mode the compression mode to use
 * @param output_type the output type to use (if unsure, SSC_ENCODE_OUTPUT_TYPE_DEFAULT), see the ssc_stream_compress documentation
 * @param block_type the block type to use (if unsure, SSC_BLOCK_TYPE_DEFAULT), see the ssc_stream_compress documentation
 * @param mem_alloc a pointer to a memory allocation function. If NULL, the standard malloc(size_t) is used.
 * @param mem_free a pointer to a memory freeing function. If NULL, the standard free(void*) is used.
 */
SSC_BUFFERS_STATE ssc_buffers_compress(uint_fast64_t* total_written, uint8_t *in, uint_fast64_t in_size, uint8_t *out, uint_fast64_t out_size, const SSC_COMPRESSION_MODE compression_mode, const SSC_ENCODE_OUTPUT_TYPE output_type, const SSC_BLOCK_TYPE block_type, void *(*mem_alloc)(), void (*mem_free)(void *));

/*
 * Buffers decompression function
 *
 * @param total_written a pointer to return the total bytes written to, can be NULL for no return
 * @param header a pointer to return the header read to, can be NULL for no return
 * @param in the input buffer to decompress
 * @param in_size the size of the input buffer in bytes
 * @param out the output buffer
 * @param out_size the size of the output buffer in bytes
 * @param mem_alloc a pointer to a memory allocation function. If NULL, the standard malloc(size_t) is used.
 * @param mem_free a pointer to a memory freeing function. If NULL, the standard free(void*) is used.
 */
SSC_BUFFERS_STATE ssc_buffers_decompress(uint_fast64_t * total_written, ssc_main_header* header, uint8_t *in, uint_fast64_t in_size, uint8_t *out, uint_fast64_t out_size, void *(*mem_alloc)(), void (*mem_free)(void *));

#endif