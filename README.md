DENSITY
========
Superfast compression library

DENSITY is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible.
DENSITY features two APIs to enable quick integration in any project.

Benchmark
---------

File used : enwik8 (100 MB)

Platform : MacBook Pro, OSX 10.10.1, 2.3 GHz Intel Core i7, 8Go 1600 MHz DDR, SSD

Timing : using the *time* function, and taking the best *user* output after multiple runs

<sub>Program</sub> | <sub>Library</sub> | <sub>Compress</sub> | <sub>Decompress</sub> | <sub>Size</sub> | <sub>Ratio</sub> | <sub>Round trip</sub>
--- | --- | --- | --- | --- | --- | ---
<sub>**sharc** -c1</sub> | <sub>**density** 0.10.2</sub> | <sub>0,117s (854,70 MB/s)</sub> | <sub>0,096s (1041,67 MB/s)</sub> | <sub>61 525 266</sub> | <sub>61,53%</sub> | <sub>0,213s</sub>
<sub>**sharc** -c2</sub> | <sub>**density** 0.10.2</sub> | <sub>0,217s (460,83 MB/s)</sub> | <sub>0,231s (432,90 MB/s)</sub> | <sub>53 157 538</sub> | <sub>53,16%</sub> | <sub>0,448s</sub>
<sub>lz4 -1</sub> | <sub>lz4 r126</sub> | <sub>0,479s (208,77 MB/s)</sub> | <sub>0,091s (1098,90 MB/s)</sub> | <sub>56 995 497</sub> | <sub>57,00%</sub> | <sub>0,570s</sub>
<sub>lz4 -9</sub> | <sub>lz4 r126</sub> | <sub>3,925s (25,48 MB/s)</sub> | <sub>0,087s (1149,43 MB/s)</sub> | <sub>44 250 986</sub> | <sub>44,25%</sub> | <sub>4,012s</sub>
<sub>lzop -1</sub> | <sub>lzo 2.08</sub> | <sub>0,367s (272,48 MB/s)</sub> | <sub>0,309s (323,62 MB/s)</sub> | <sub>56 709 096</sub> | <sub>56,71%</sub> | <sub>0,676s</sub>
<sub>lzop -9</sub> | <sub>lzo 2.08</sub> | <sub>14,298s (6,99 MB/s)</sub> | <sub>0,315s 317,46 MB/s)</sub> | <sub>41 217 688</sub> | <sub>41,22%</sub> | <sub>14,613s</sub>

[Click here for a more exhaustive benchmark](http://quixdb.github.io/squash/benchmarks/core-i3-2105.html) of DENSITY's fastest mode compared to other libraries (look for the [sharc](https://github.com/centaurean/sharc) line), on an Intel® Core™ i3-2105 (x86 64), Asus P8H61-H motherboard with Fedora 19. It is possible to run yours using [this project](https://github.com/quixdb/squash).

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
    if ((streamState = density_stream_compress_continue(stream)))  if (streamState != DENSITY_STREAM_STATE_STALL_ON_INPUT)
        fprintf(stderr, "Error %i occured during compression\n", streamState);
    if ((streamState = density_stream_compress_finish(stream)))
        fprintf(stderr, "Error %i occured while finishing compression\n", streamState);
    printf("%llu bytes >> %llu bytes\n", *stream->in_total_read, *stream->out_total_written);

    // Now let's decompress it, using the density_stream_output_available_for_use() method to know how many bytes were made available
    if ((streamState = density_stream_prepare(stream, outCompressed, density_stream_output_available_for_use(stream), outDecompressed, BUFFER_SIZE)))
        fprintf(stderr, "Error %i when preparing decompression\n", streamState);
    if ((streamState = density_stream_decompress_init(stream, NULL)))
        fprintf(stderr, "Error %i when initializing decompression\n", streamState);
    if ((streamState = density_stream_decompress_continue(stream))) if (streamState != DENSITY_STREAM_STATE_STALL_ON_INPUT)
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
