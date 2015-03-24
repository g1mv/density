DENSITY
========
Superfast compression library

DENSITY is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible.
DENSITY features a buffer and stream API to enable quick integration in any project.

Benchmarks
----------

**Quick bench**

File used : enwik8 (100 MB)

Platform : MacBook Pro, OSX 10.10.1, 2.3 GHz Intel Core i7, 8Go 1600 MHz DDR, SSD

Timing : using the *time* function, and taking the best *user* output after multiple runs

Note : *sharc -c1* uses the chameleon algorithm, *sharc -c2* the cheetah algorithm, and *sharc -c3* the lion algorithm (see **About the algorithms** further down).

<sub>Program</sub> | <sub>Library</sub> | <sub>Compress</sub> | <sub>Decompress</sub> | <sub>Size</sub> | <sub>Ratio</sub> | <sub>Round trip</sub>
--- | --- | --- | --- | --- | --- | ---
<sub>**[sharc](https://github.com/centaurean/sharc)** -c1</sub> | <sub>**density** 0.12.0</sub> | <sub>0.111s (900 MB/s)</sub> | <sub>0.085s (1175 MB/s)</sub> | <sub>61 524 502</sub> | <sub>61,52%</sub> | <sub>0.196s</sub>
<sub>[lz4](https://github.com/Cyan4973/lz4) -1</sub> | <sub>lz4 r126</sub> | <sub>0.461s (217 MB/s)</sub> | <sub>0.091s (1099 MB/s)</sub> | <sub>56 995 497</sub> | <sub>57,00%</sub> | <sub>0.552s</sub>
<sub>[lzop](http://www.lzop.org) -1</sub> | <sub>lzo 2.08</sub> | <sub>0.367s (272 MB/s)</sub> | <sub>0.309s (324 MB/s)</sub> | <sub>56 709 096</sub> | <sub>56,71%</sub> | <sub>0.676s</sub>
<sub>**[sharc](https://github.com/centaurean/sharc)** -c2</sub> | <sub>**density** 0.12.0</sub> | <sub>0.212s (472 MB/s)</sub> | <sub>0.217s (460 MB/s)</sub> | <sub>53 156 782</sub> | <sub>53,16%</sub> | <sub>0.429s</sub>
<sub>**[sharc](https://github.com/centaurean/sharc)** -c3</sub> | <sub>**density** 0.12.0</sub> | <sub>0.361s (277 MB/s)</sub> | <sub>0.396s (253 MB/s)</sub> | <sub>47 991 605</sub> | <sub>47,99%</sub> | <sub>0.757s</sub>
<sub>[lz4](https://github.com/Cyan4973/lz4) -3</sub> | <sub>lz4 r126</sub> | <sub>1.520s (66 MB/s)</sub> | <sub>0.087s (1149 MB/s)</sub> | <sub>47 082 421</sub> | <sub>47,08%</sub> | <sub>1.607s</sub>
<sub>[lzop](http://www.lzop.org) -7</sub> | <sub>lzo 2.08</sub> | <sub>9.562s (10 MB/s)</sub> | <sub>0.319s (313 MB/s)</sub> | <sub>41 720 721</sub> | <sub>41,72%</sub> | <sub>9.881s</sub>

**Squash**

Squash is an abstraction layer for compression algorithms, and has an extremely exhaustive set of benchmark results, including density's, [available here](https://quixdb.github.io/squash-benchmark/).
You can choose between system architecture and compressed file type. There are even ARM boards tested ! A great tool for selecting a compression library.

**FsBench**

FsBench is a command line utility that enables real-time testing of compression algorithms, but also hashes and much more. A fork with the latest density releases is [available here](https://github.com/centaurean/fsbench-density) for easy access.
The original author's repository [can be found here](https://chiselapp.com/user/Justin_be_my_guide/repository/fsbench/). Very informative tool as well.

Here are the results of a couple of test runs on a MacBook Pro, OSX 10.10.1, 2.3 GHz Intel Core i7, 8Go 1600 MHz DDR, SSD :

*enwik8 (100,000,000 bytes)*

    Codec                                   version      args
    C.Size      (C.Ratio)        E.Speed   D.Speed      E.Eff. D.Eff.
    density::chameleon                      2015-03-22   
       61524474 (x 1.625)      903 MB/s 1248 MB/s       347e6  480e6
    density::cheetah                        2015-03-22   
       53156746 (x 1.881)      468 MB/s  482 MB/s       219e6  225e6
    density::lion                           2015-03-22   
       47991569 (x 2.084)      285 MB/s  271 MB/s       148e6  140e6
    LZ4                                     r127         
       56973103 (x 1.755)      258 MB/s 1613 MB/s       111e6  694e6
    LZF                                     3.6          very
       53945381 (x 1.854)      192 MB/s  370 MB/s        88e6  170e6
    LZO                                     2.08         1x1
       55792795 (x 1.792)      287 MB/s  371 MB/s       126e6  164e6
    QuickLZ                                 1.5.1b6      1
       52334371 (x 1.911)      281 MB/s  351 MB/s       134e6  167e6
    Snappy                                  1.1.0        
       56539845 (x 1.769)      244 MB/s  788 MB/s       106e6  342e6
    wfLZ                                    r10          
       63521804 (x 1.574)      150 MB/s  513 MB/s        54e6  187e6
       
*silesia (211,960,320 bytes)*

    Codec                                   version      args
    C.Size      (C.Ratio)        E.Speed   D.Speed      E.Eff. D.Eff.
    density::chameleon                      2015-03-22   
      133118910 (x 1.592)     1040 MB/s 1281 MB/s       386e6  476e6
    density::cheetah                        2015-03-22   
      101751474 (x 2.083)      531 MB/s  493 MB/s       276e6  256e6
    density::lion                           2015-03-22   
       89433997 (x 2.370)      304 MB/s  275 MB/s       175e6  159e6
    LZ4                                     r127         
      101634462 (x 2.086)      365 MB/s 1815 MB/s       189e6  944e6
    LZF                                     3.6          very
      102043866 (x 2.077)      254 MB/s  500 MB/s       131e6  259e6
    LZO                                     2.08         1x1
      100592662 (x 2.107)      429 MB/s  578 MB/s       225e6  303e6
    QuickLZ                                 1.5.1b6      1
       94727961 (x 2.238)      370 MB/s  432 MB/s       204e6  238e6
    Snappy                                  1.1.0        
      101385885 (x 2.091)      356 MB/s 1085 MB/s       185e6  565e6
    wfLZ                                    r10          
      109610020 (x 1.934)      196 MB/s  701 MB/s        94e6  338e6

Build
-----
DENSITY is fully C99 compliant and can therefore be built on a number of platforms. You need a C compiler (gcc, clang ...), and a *make* utility.

Just *cd* into the density directory, then run the following command :
> make

And that's it !

Output format
-------------
DENSITY outputs compressed data in a simple format, which enables file storage and optional parallelization for both compression and decompression.
Inside the main header and footer, a number of blocks can be found, each having its own header and footer.
Inside each block, compressed data has a structure determined by the compression algorithm used.

It is possible to add an integrity checksum to the compressed output by using the *DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK* block type.
The 128-bit checksum is calculated using the excellent [SpookyHash algorithm](https://github.com/centaurean/spookyhash), which is extremely fast and offers a near-zero performance penalty.
An additional integrity check will then be automatically performed during decompression.

APIs
----
DENSITY features a *buffer API* and a *stream API* which are very simple to use, yet powerful enough to keep users' creativity unleashed.
Please see the *quick start* at the bottom of this page.

About the algorithms
--------------------

**Copy** ( *DENSITY_COMPRESSION_MODE_COPY* )

This is not a so-to-speak algorithm as the name implies. It embeds data inside the density block structures.
It can be used to quickly add integrity checks to input data, but it has another important purpose inside each block : if data is marked as incompressible using the target algorithm, a mode reversion occurs and copy mode is instead used for the remainder of the block.
On the next block the target algorithm is tried again.

**Chameleon** ( *DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM* )

Chameleon is a dictionary lookup based compression algorithm. It is designed for absolute speed and usually reaches a 60% compression ratio on compressible data.
Decompression is just as fast. This algorithm is a great choice when main concern is speed.

**Cheetah** ( *DENSITY_COMPRESSION_MODE_CHEETAH_ALGORITHM* )

Cheetah was developed in conjunction with Piotr Tarsa (https://github.com/tarsa).
It is derived from chameleon and uses swapped double dictionary lookups and predictions. It can be extremely good with highly compressible data (ratio reaching 10% or less).
On typical compressible data compression ratio is about 50% or less. It is still extremely fast for both compression and decompression and is a great, efficient all-rounder algorithm.

**Lion** ( *DENSITY_COMPRESSION_MODE_LION_ALGORITHM* )

Lion is a multiform compression algorithm derived from cheetah. It goes further in the areas of dynamic adaptation and fine-grained analysis. 
It uses swapped double dictionary lookups, multiple predictions, shifting sub-word dictionary lookups and forms rank entropy coding. 
Lion provides the best compression ratio of all three algorithms under any circumstance, and is still very fast.

Quick start (a simple example using the APIs)
---------------------------------------------
Using DENSITY in your application couldn't be any simpler.

First you need to include the 2 following files in your project :

* density_api.h
* density_api_data_structures.h

When this is done you can start using the **DENSITY APIs** :

    #include "density_api.h"

    #define MY_TEXT "This is a simple example on how to use Density API bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla"
    #define BUFFER_SIZE 16384
    
    ...
    
    uint8_t *outCompressed = (uint8_t *) malloc(BUFFER_SIZE * sizeof(uint8_t));
    uint8_t *outDecompressed = (uint8_t *) malloc(BUFFER_SIZE * sizeof(uint8_t));
    
    /**************
     * Buffer API *
     **************/
     
    density_buffer_processing_result result;
    result = density_buffer_compress((uint8_t *) TEXT, strlen(TEXT), outCompressed, BUFFER_SIZE, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, DENSITY_BLOCK_TYPE_DEFAULT, NULL, NULL);
    if(!result.state)
        printf("%llu bytes >> %llu bytes\n", result.bytesRead, result.bytesWritten);

    result = density_buffer_decompress(outCompressed, result.bytesWritten, outDecompressed, BUFFER_SIZE, NULL, NULL);
    if(!result.state)
        printf("%llu bytes >> %llu bytes\n", result.bytesRead, result.bytesWritten);
    
    /**************
     * Stream API *
     **************/

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
    printf("%llu bytes >> %llu bytes\n", *stream->totalBytesRead, *stream->totalBytesWritten);

    // Now let's decompress it, using the density_stream_output_available_for_use() method to know how many bytes were made available
    if ((streamState = density_stream_prepare(stream, outCompressed, density_stream_output_available_for_use(stream), outDecompressed, BUFFER_SIZE)))
        fprintf(stderr, "Error %i when preparing decompression\n", streamState);
    if ((streamState = density_stream_decompress_init(stream, NULL)))
        fprintf(stderr, "Error %i when initializing decompression\n", streamState);
    if ((streamState = density_stream_decompress_continue(stream))) if (streamState != DENSITY_STREAM_STATE_STALL_ON_INPUT)
        fprintf(stderr, "Error %i occured during decompression\n", streamState);
    if ((streamState = density_stream_decompress_finish(stream)))
        fprintf(stderr, "Error %i occured while finishing compression\n", streamState);
    printf("%llu bytes >> %llu bytes\n", *stream->totalBytesRead, *stream->totalBytesWritten);

    // Free memory
    density_stream_destroy(stream);
    
    free(outCompressed);
    free(outDecompressed);

And that's it ! We've done two compression/decompression round trips with a few lines !

If you want a more elaborate example you can checkout [the SHARC project](https://github.com/centaurean/sharc).
