/*
 * Centaurean Density
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

#ifndef DENSITY_API_H
#define DENSITY_API_H

#include "density_api_data_structures.h"


/***********************************************************************************************************************
 *                                                                                                                     *
 * Density version information                                                                                         *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Returns the major density version
 */
uint8_t density_version_major(void);

/*
 * Returns the minor density version
 */
uint8_t density_version_minor(void);

/*
 * Returns the density revision
 */
uint8_t density_version_revision(void);



/***********************************************************************************************************************
 *                                                                                                                     *
 * Density stream API functions                                                                                        *
 *                                                                                                                     *
 ***********************************************************************************************************************/

/*
 * Allocate a stream in memory, with a user-defined memory allocation function.
 * This function will then be used throughout the library to allocate memory.
 * If NULL is specified, the standard malloc will be used.
 *
 * @param mem_alloc the memory allocation function
 * @param mem_free the memory freeing function
 */
density_stream *density_stream_create(void *(*mem_alloc)(size_t), void (*mem_free)(void *));

/*
 * Frees a stream from memory. This method uses a supplied memory freeing function.
 * If NULL is specified, the standard free will be used.
 *
 * @param stream the stream
 */
void density_stream_destroy(density_stream *stream);

/*
 * Prepare a stream with the encapsulated input/output buffers. This function *must* be called upon changing either buffer pointers / sizes.
 *
 * @param stream the stream
 * @param input_buffer a buffer of bytes
 * @param input_size the size in bytes of input_buffer
 * @param output_buffer a buffer of bytes
 * @param output_size the size of output_buffer, must be at least DENSITY_STREAM_MINIMUM_OUT_BUFFER_SIZE
 * @param mem_alloc a pointer to a memory allocation function. If NULL, the standard malloc(size_t) is used.
 * @param mem_free a pointer to a memory freeing function. If NULL, the standard free(void*) is used.
 */
DENSITY_STREAM_STATE density_stream_prepare(density_stream *stream, uint8_t* input_buffer, const uint_fast64_t input_size, uint8_t* output_buffer, const uint_fast64_t output_size);

/*
 * Update the stream's input
 *
 * @param stream the stream
 * @param in a byte array
 * @param availableIn the size of the byte array
 */
DENSITY_STREAM_STATE density_stream_update_input(density_stream *stream, uint8_t *in, const uint_fast64_t availableIn);

/*
 * Update the stream's output
 *
 * @param stream the stream
 * @param out a byte array
 * @param availableOut the size of the byte array
 */
DENSITY_STREAM_STATE density_stream_update_output(density_stream *stream, uint8_t *out, const uint_fast64_t availableOut);

/*
 * Returns the usable bytes (bytes that have been written by Density) on the output memory location
 *
 * @param stream the stream
 */
uint_fast64_t density_stream_output_available_for_use(density_stream* stream);

/*
 * Initialize compression
 *
 * @param stream the stream
 * @param compression_mode the compression mode
 * @param output_type the format of data output by encoding.
 *      EXPERTS ONLY ! If unsure, use DENSITY_ENCODE_OUTPUT_TYPE_DEFAULT.
 *      Any other option will create output data which is *NOT* directly decompressible by the API. This can be used for parallelizing Density.
 * @param block_type the type of data blocks Density will generate.
 *      EXPERTS ONLY ! If you're unsure use DENSITY_BLOCK_TYPE_DEFAULT.
 *      The option DENSITY_BLOCK_TYPE_NO_HASHSUM_INTEGRITY_CHECK basically makes the block footer size zero, and removes data integrity checks in the encoded output.
 *      It can be useful in network streaming situations, where data integrity is already checked by the protocol (TCP/IP for example), and the flush option in density_stream_compress is often set,
 *      as the absence of block footer will enhance compression ratio.
 */
DENSITY_STREAM_STATE density_stream_compress_init(density_stream *stream, const DENSITY_COMPRESSION_MODE compression_mode, const DENSITY_BLOCK_TYPE block_type);

/*
 * Stream decompression initialization
 *
 * @param stream the stream
 * @param header_information stream header information, use NULL if you don't need it
 */
DENSITY_STREAM_STATE density_stream_decompress_init(density_stream *stream, density_stream_header_information *header_information);

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
 *      the block_type parameter of density_stream_compress_init to enable a better compression ratio. It is also worth noting that
 *      the *best* input buffer size for compression ratio matters should be a multiple of 256, any other size will also work but will
 *      incur a less than optimal compression ratio.
 */
DENSITY_STREAM_STATE density_stream_compress_continue(density_stream *stream);

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
DENSITY_STREAM_STATE density_stream_decompress_continue(density_stream *stream);

/*
 * Call once processing is finished, to clear up the environment and release eventual allocated memory.
 *
 * @param stream the stream
 */
DENSITY_STREAM_STATE density_stream_compress_finish(density_stream *stream);

/*
 * Call once processing is finished, to clear up the environment and release eventual allocated memory.
 *
 * @param stream the stream
 */
DENSITY_STREAM_STATE density_stream_decompress_finish(density_stream *stream);

#endif