DENSITY
========
Superfast compression library

DENSITY is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible. Streaming is fully supported.
DENSITY features a buffer and a stream API to enable quick integration in any project.

Benchmarks
----------

**Quick bench**

File used : enwik8 (100 MB)

Platform : MacBook Pro, OSX 10.10.3, 2.3 GHz Intel Core i7, 8Gb 1600 MHz DDR, SSD, compiling with Clang/LLVM 6.1.0

Timing : using the *time* function, and taking the best *user* output after multiple runs. In the case of density, the in-memory integrated benchmark's best value (which uses the same usermode CPU timing) is used.

<sub>Library</sub> | <sub>Compress</sub> | <sub>Decompress</sub> | <sub>Size</sub> | <sub>Ratio</sub> | <sub>Round trip</sub>
--- | --- | --- | --- | --- | ---
<sub>**density** 0.12.2 beta</sub> | <sub>0.103s (974 MB/s)</sub> | <sub>0.071s (1416 MB/s)</sub> | <sub>61 525 242</sub> | <sub>61,53%</sub> | <sub>0.174s</sub>
<sub>lz4 r126</sub> | <sub>0.461s (217 MB/s)</sub> | <sub>0.091s (1099 MB/s)</sub> | <sub>56 995 497</sub> | <sub>57,00%</sub> | <sub>0.552s</sub>
<sub>lzo 2.08</sub> | <sub>0.367s (272 MB/s)</sub> | <sub>0.309s (324 MB/s)</sub> | <sub>56 709 096</sub> | <sub>56,71%</sub> | <sub>0.676s</sub>
<sub>**density** 0.12.2 beta</sub> | <sub>0.183s (547 MB/s)</sub> | <sub>0.144s (692 MB/s)</sub> | <sub>53 157 514</sub> | <sub>53,16%</sub> | <sub>0.327s</sub>
<sub>**density** 0.12.2 beta</sub> | <sub>0.360s (278 MB/s)</sub> | <sub>0.352s (284 MB/s)</sub> | <sub>48 048 466</sub> | <sub>48,05%</sub> | <sub>0.712s</sub>
<sub>lz4 r126</sub> | <sub>1.520s (66 MB/s)</sub> | <sub>0.087s (1149 MB/s)</sub> | <sub>47 082 421</sub> | <sub>47,08%</sub> | <sub>1.607s</sub>
<sub>lzo 2.08</sub> | <sub>9.562s (10 MB/s)</sub> | <sub>0.319s (313 MB/s)</sub> | <sub>41 720 721</sub> | <sub>41,72%</sub> | <sub>9.881s</sub>

**Squash**

Squash is an abstraction layer for compression algorithms, and has an extremely exhaustive set of benchmark results, including density's, [available here](https://quixdb.github.io/squash-benchmark/?dataset=dickens&machine=s-desktop).
You can choose between system architecture and compressed file type. There are even ARM boards tested ! A great tool for selecting a compression library.

[![Screenshot of density results on Squash](http://i.imgur.com/mszWTEl.png)](https://quixdb.github.io/squash-benchmark/?dataset=dickens&machine=s-desktop)

**FsBench**

FsBench is a command line utility that enables real-time testing of compression algorithms, but also hashes and much more. A fork with the latest density releases is [available here](https://github.com/gpnuma/fsbench-density) for easy access.
The original author's repository [can be found here](https://chiselapp.com/user/Justin_be_my_guide/repository/fsbench/). Very informative tool as well.

Here are the results of a couple of test runs on a MacBook Pro, OSX 10.10.3, 2.3 GHz Intel Core i7, 8Gb 1600 MHz DDR, SSD, compiling with Clang/LLVM 6.1.0 :

*enwik8 (100,000,000 bytes)*

    Codec                                   version      args
    C.Size      (C.Ratio)        E.Speed   D.Speed      E.Eff. D.Eff.
    density::chameleon                      0.12.2 beta  
       61525242 (x 1.625)      958 MB/s 1445 MB/s       368e6  555e6
    density::cheetah                        0.12.2 beta  
       53157514 (x 1.881)      528 MB/s  660 MB/s       247e6  309e6
    density::lion                           0.12.2 beta  
       48048466 (x 2.081)      223 MB/s  278 MB/s       115e6  144e6
    LZ4                                     r127         
       56973103 (x 1.755)      246 MB/s 1004 MB/s       106e6  431e6
    LZF                                     3.6          very
       53945381 (x 1.854)      192 MB/s  367 MB/s        88e6  168e6
    LZO                                     2.08         1x1
       55792795 (x 1.792)      288 MB/s  369 MB/s       127e6  163e6
    QuickLZ                                 1.5.1b6      1
       52334371 (x 1.911)      279 MB/s  351 MB/s       133e6  167e6
    Snappy                                  1.1.0        
       56539845 (x 1.769)      237 MB/s  800 MB/s       102e6  347e6
    wfLZ                                    r10          
       63521804 (x 1.574)      149 MB/s  505 MB/s        54e6  184e6
       
*silesia (211,960,320 bytes)*

    Codec                                   version      args
    C.Size      (C.Ratio)        E.Speed   D.Speed      E.Eff. D.Eff.
    density::chameleon                      0.12.2 beta  
      133120534 (x 1.592)     1117 MB/s 1570 MB/s       415e6  583e6
    density::cheetah                        0.12.2 beta  
      101753098 (x 2.083)      590 MB/s  706 MB/s       306e6  366e6
    density::lion                           0.12.2 beta  
       89220526 (x 2.376)      248 MB/s  278 MB/s       143e6  161e6
    LZ4                                     r127         
      101634462 (x 2.086)      348 MB/s 1244 MB/s       181e6  647e6
    LZF                                     3.6          very
      102043866 (x 2.077)      252 MB/s  487 MB/s       130e6  252e6
    LZO                                     2.08         1x1
      100592662 (x 2.107)      425 MB/s  572 MB/s       223e6  300e6
    QuickLZ                                 1.5.1b6      1
       94727961 (x 2.238)      368 MB/s  428 MB/s       203e6  236e6
    Snappy                                  1.1.0        
      101385885 (x 2.091)      346 MB/s 1109 MB/s       180e6  578e6
    wfLZ                                    r10          
      109610020 (x 1.934)      196 MB/s  692 MB/s        94e6  334e6

Build
-----
DENSITY is fully C99 compliant and can therefore be built on a number of platforms. It uses the [premake](http://premake.github.io/) build system.
The library was developped ang optimized against Clang/LLVM, it is therefore *highly* recommended to compile using Clang/LLVM for best performance, but if that's not possible GCC is also supported.
The following assumes you already have *git* installed.

**Mac OS X**

On OS X, Clang/LLVM is the default compiler, which makes things simpler.

1) Download and install [premake](http://premake.github.io/) for OS X and make it available in your path (if you have [homebrew](https://github.com/Homebrew/homebrew) installed, typing "brew install premake" in a command line should do the trick).

2) Run the following from the command line :

    cd build
    premake4 gmake
    make

**Linux**

On Linux, Clang/LLVM is not always available by default.

1) Install Clang/LLVM if you don't have it already (on the Debian distribution for example, sudo apt-get install clang-3.5 should do the trick)

2) Download and install [premake](http://premake.github.io/) for Linux and make it available in your path.

3) Run the following from the command line :

    cd build
    premake4 gmake
    make
    
**Windows**

On Windows, things are a (little) bit more complicated.

1) First, check you have git installed, if not you can [get it here](https://www.git-scm.com/download/win). Make it available in your path.

2) Microsoft Visual Studio is required as we will use its linker, [get it here](https://www.visualstudio.com/en-us/products/free-developer-offers-vs.aspx).

3) Now we proceed with the installation of Clang/LLVM for windows, thanks to the **ClangOnWin** project it's easy, you can [download it here](http://sourceforge.net/projects/clangonwin/).

4) Download and install [premake](http://premake.github.io/) for Windows and make it available in your path (edit the Path environment variable).

5) Run the following from the command line :
   
       cd build
       premake4.exe gmake
       make

And that's it ! You can now use the integrated in-memory benchmark to test your files (the *benchmark* or *benchmark.exe* binary).

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

Cheetah was developed in conjunction with [Piotr Tarsa](https://github.com/tarsa).
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

```C
#include "density_api.h"

#define MY_TEXT "This is a simple example on how to use Density API bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla"
#define BUFFER_SIZE DENSITY_MINIMUM_OUT_BUFFER_SIZE

...

uint8_t *outCompressed   = malloc(BUFFER_SIZE * sizeof(uint8_t));
uint8_t *outDecompressed = malloc(BUFFER_SIZE * sizeof(uint8_t));

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
```

And that's it ! We've done two compression/decompression round trips with a few lines !

If you want a more elaborate example you can checkout [the SHARC project](https://github.com/centaurean/sharc).
