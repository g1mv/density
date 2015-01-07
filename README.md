DENSITY
========
Superfast compression library

DENSITY is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible.
DENSITY features two APIs to enable quick integration in any project.

Benchmark
---------
[Click here for a benchmark](http://quixdb.github.io/squash/benchmarks/core-i3-2105.html) of DENSITY's fastest mode compared to other libraries (look for the [sharc](https://github.com/centaurean/sharc) line), on an Intel® Core™ i3-2105 (x86 64), Asus P8H61-H motherboard with Fedora 19. It is possible to run yours using [this project](https://github.com/quixdb/squash).

API
---
DENSITY features a *stream API* which is very simple to use, yet powerful enough to keep users' creativity unleashed.

Quick start : a simple example using the API
--------------------------------------------
Using DENSITY in your application couldn't be any simpler.

First you need to include the 2 following files in your project :

* density_api.h
* density_api_data_structures.h

When this is done you can start using the **DENSITY API** :

    #include "density_api.h"
    #include "density_api_data_structures.h"

    #define MY_TEXT "This is a simple example on how to use Density API bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla"
    #define BUFFER_SIZE 16384
    
    ...
    
    uint8_t *outCompressed = (uint8_t *) malloc(BUFFER_SIZE * sizeof(uint8_t));
    uint8_t *outDecompressed = (uint8_t *) malloc(BUFFER_SIZE * sizeof(uint8_t));

    // We create the stream using the standard malloc and free functions
    density_stream* stream = density_stream_create(NULL, NULL);
    DENSITY_STREAM_STATE streamState;
    
    // Let's compress our text, using the Chameleon algorithm (extremely fast compression and decompression)
    if ((streamState = density_stream_prepare(stream, (uint8_t *) TEXT, strlen(TEXT), outCompressed, BUFFER_SIZE)))
        fprintf(stderr, "Error %i when preparing compression\n", streamState);
    if ((streamState = density_stream_compress_init(stream, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, DENSITY_BLOCK_TYPE_DEFAULT)))
        fprintf(stderr, "Error %i when initializing compression\n", streamState);
    if ((streamState = density_stream_compress(stream, true)))
        fprintf(stderr, "Error %i occured during compression\n", streamState);
    if ((streamState = density_stream_compress_finish(stream)))
        fprintf(stderr, "Error %i occured while finishing compression\n", streamState);
    printf("%llu bytes >> %llu bytes\n", *stream->in_total_read, *stream->out_total_written);

    // Now let's decompress it, using the density_stream_output_available_for_use() method to know how many bytes were made available
    if ((streamState = density_stream_prepare(stream, outCompressed, density_stream_output_available_for_use(stream), outDecompressed, BUFFER_SIZE)))
        fprintf(stderr, "Error %i when preparing decompression\n", streamState);
    if ((streamState = density_stream_decompress_init(stream, NULL)))
        fprintf(stderr, "Error %i when initializing decompression\n", streamState);
    if ((streamState = density_stream_decompress(stream, true)))
        fprintf(stderr, "Error %i occured during decompression\n", streamState);
    if ((streamState = density_stream_decompress_finish(stream)))
        fprintf(stderr, "Error %i occured while finishing compression\n", streamState);
    printf("%llu bytes >> %llu bytes\n", *stream->in_total_read, *stream->out_total_written);

    // Free memory
    density_stream_destroy(stream);
    
    free(outCompressed);
    free(outDecompressed);

And that's it ! We've done a compression/decompression round trip with a few lines !

If you want a more elaborate example you can checkout [the SHARC project](https://github.com/centaurean/sharc).

Build
-----
DENSITY is fully C99 compliant and can therefore be built on a number of platforms. You need a C compiler (gcc, clang ...), and a *make* utility.

Just *cd* into the density directory, then run the following command :
> make

And that's it !

Output
------
DENSITY outputs compressed data in a simple format, which enables file storage and optional parallelization for both compression and decompression.
