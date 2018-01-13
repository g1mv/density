DENSITY
========
Superfast compression library

DENSITY is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible. **All three** of DENSITY's algorithms are currently at the **pareto frontier** of compression ratio vs speed (cf. [here](https://github.com/inikep/lzbench/blob/master/lzbench171_sorted.md) for an independent benchmark).

DENSITY features a simple API to enable quick integration in any project.

Why is it so fast ?
-------------------

One of the biggest assets of DENSITY is that its work unit is **not a byte** like other libraries, but **a group of 4 bytes**.

When other libraries consume one byte of data and then apply an algorithmic processing to it, DENSITY consumes 4 bytes and then applies its algorithmic processing.

That's why DENSITY's algorithms were designed from scratch. They have to alleviate for 4-byte work units and still provide interesting compression ratios.

**Speed pedigree traits**

*   4-byte work units
*   heavy use of registers as opposed to memory for processing
*   avoidance of or use of minimal branching when possible
*   use of low memory data structures to favor processor cache Lx accesses
*   library wide inlining
*   specific unrollings
*   prefetching and branching hints
*   restricted pointers to maximize compiler optimizations

A "blowup protection" is provided, dramatically increasing the processing speed of incompressible input data. Also, the output, compressed data size will **never exceed** the original uncompressed data size by more than 1% in case of incompressible, reasonably-sized inputs.

Benchmarks
----------

**Quick benchmark**

DENSITY features an **integrated in-memory benchmark**. After building the project (see [build](#build)), a *benchmark* executable will be present in the build/bin/Release directory. If run without arguments, usage help will be displayed.

File used : enwik8 (100 MB)

Platform : MacBook Pro, OSX 10.13.2, 2.3 GHz Intel Core i7, 8Gb 1600 MHz DDR, SSD, compiling with Clang/LLVM 9.0.0

Timing : using the *time* function, and taking the best *user* output after multiple runs. In the case of density, the in-memory integrated benchmark's best value (which uses the same usermode CPU timing) is used.

<sub>Library</sub>|<sub>Algorithm</sub>|<sub>Compress</sub>|<sub>Decompress</sub>|<sub>Size</sub>|<sub>Ratio</sub>|<sub>Round trip</sub>
---|---|---|---|---|---|---
<sub>**density** 0.14.0</sub>|<sub>Chameleon</sub>|<sub>0.098s (1023 MB/s)</sub>|<sub>0.062s (1619 MB/s)</sub>|<sub>61 524 478</sub>|<sub>61,52%</sub>|<sub>0.160s</sub>
<sub>lz4 r129</sub>|<sub>-1</sub>|<sub>0.468s (214 MB/s)</sub>|<sub>0.115s (870 MB/s)</sub>|<sub>57 285 990</sub>|<sub>57,29%</sub>|<sub>0.583s</sub>
<sub>lzo 2.08</sub>|<sub>-1</sub>|<sub>0.367s (272 MB/s)</sub>|<sub>0.309s (324 MB/s)</sub>|<sub>56 709 096</sub>|<sub>56,71%</sub>|<sub>0.676s</sub>
<sub>**density** 0.14.0</sub>|<sub>Cheetah</sub>|<sub>0.179s (560 MB/s)</sub>|<sub>0.142 (706 MB/s)</sub>|<sub>53 156 750</sub>|<sub>53,16%</sub>|<sub>0.321s</sub>
<sub>**density** 0.14.0</sub>|<sub>Lion</sub>|<sub>0.356s (281 MB/s)</sub>|<sub>0.348s (288 MB/s)</sub>|<sub>47 818 076</sub>|<sub>47,82%</sub>|<sub>0.704s</sub>
<sub>lz4 r129</sub>|<sub>-3</sub>|<sub>1.685s (59 MB/s)</sub>|<sub>0.118s (847 MB/s)</sub>|<sub>44 539 940</sub>|<sub>44,54%</sub>|<sub>1.803s</sub>
<sub>lzo 2.08</sub>|<sub>-7</sub>|<sub>9.562s (10 MB/s)</sub>|<sub>0.319s (313 MB/s)</sub>|<sub>41 720 721</sub>|<sub>41,72%</sub>|<sub>9.881s</sub>

**Other benchmarks**

Here are a few other benchmarks featuring DENSITY (non exhaustive list) :

*   [**squash**](https://github.com/quixdb/squash) is an abstraction layer for compression algorithms, and has an extremely exhaustive set of benchmark results, including density's, [available here](https://quixdb.github.io/squash-benchmark/?dataset=dickens&machine=s-desktop).

*   [**lzbench**](https://github.com/inikep/lzbench) is an in-memory benchmark of open-source LZ77/LZSS/LZMA compressors. Although DENSITY is not LZ-based, results are well worth a [look here](https://github.com/inikep/lzbench/blob/master/lzbench171_sorted.md).

*   [**fsbench**](https://github.com/gpnuma/fsbench-density) is a command line utility that enables real-time testing of compression algorithms, but also hashes and much more. A fork with density releases is [available here](https://github.com/gpnuma/fsbench-density) for easy access.
The original author's repository [can be found here](https://chiselapp.com/user/Justin_be_my_guide/repository/fsbench/).

Build
-----
DENSITY can be built on a number of platforms. It uses the [premake](http://premake.github.io/) build system.

It was developed and optimized against Clang/LLVM, but GCC is also supported (please use a recent version for maximum performance).
At this time MSVC is *not* supported, but it's straightforward to install clang for windows.

The following assumes you already have *git* installed.

**Mac OS X**

On OS X, Clang/LLVM is the default compiler, which makes things simpler.

1) Download and install [premake5](http://premake.github.io/) for OS X and make it available in your path.

2) Run the following from the command line :

```
    cd build
    premake5 gmake
    make
```

**Linux**

On Linux, Clang/LLVM is not always available by default.

1) Install Clang/LLVM *(optional if GCC is installed)* if you don't have it already (on Debian distributions for example, *sudo apt-get install clang* should do the trick)

2) Download and install [premake5](http://premake.github.io/) for Linux and make it available in your path.

3) Run the following from the command line :

```
    cd build
    premake5 gmake
    make
```

**Windows**

On Windows, things are a (little) bit more complicated.

1) First, check if you have git installed, if not you can [get it here](https://www.git-scm.com/download/win). Make it available in your path.

2) We'll use *GNU make* to build the project, on Windows it's not part of the default toolset but you can [download a port here](http://gnuwin32.sourceforge.net/packages/make.htm). Make it available in your path.

3) Microsoft Visual Studio is required as we will use its linker, [get it here](https://www.visualstudio.com/en-us/products/free-developer-offers-vs.aspx).

4) Now we can install Clang/LLVM for Windows, and thanks to the **ClangOnWin** project it's easy ! You can [download it here](http://sourceforge.net/projects/clangonwin/). Make it available in your path.

5) Download and install [premake5](http://premake.github.io/) for Windows and make it available in your path (edit the Path environment variable).

6) Run the following from the command line :

```
    cd build
    premake5.exe gmake
    make
```

And that's it ! You can now use the integrated in-memory benchmark to test your files (the *benchmark* or *benchmark.exe* binary in the build/bin/Release/ folder).

Output format
-------------
DENSITY outputs compressed data in a simple format, which enables file storage and optional parallelization for both compression and decompression.

A very short header holding vital informations (like DENSITY version and algorithm used) precedes the binary compressed data.

APIs
----
DENSITY features a straightforward *API*, simple yet powerful enough to keep users' creativity unleashed.
Please see the [*quick start*](quickstart) at the bottom of this page.

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

Cheetah was developed with inputs from [Piotr Tarsa](https://github.com/tarsa).
It is derived from chameleon and uses swapped double dictionary lookups and predictions. It can be extremely good with highly compressible data (ratio reaching 10% or less).
On typical compressible data compression ratio is about 50% or less. It is still extremely fast for both compression and decompression and is a great, efficient all-rounder algorithm.

**Lion** ( *DENSITY_COMPRESSION_MODE_LION_ALGORITHM* )

Lion is a multiform compression algorithm derived from cheetah. It goes further in the areas of dynamic adaptation and fine-grained analysis.
It uses multiple swapped dictionary lookups and predictions, and forms rank entropy coding.
Lion provides the best compression ratio of all three algorithms under any circumstance, and is still very fast.

Quick start (a simple example using the API)
--------------------------------------------
Using DENSITY in your application couldn't be any simpler.

First you need to include this file in your project :

*   density_api.h

When this is done you can start using the **DENSITY API** :

```C
    #include <string.h>
    #include "density_api.h"

    char* text = "This is a simple example on how to use the simple Density API.  This is a simple example on how to use the simple Density API.";
    uint64_t text_length = (uint64_t)strlen(text);

    // Determine safe buffer sizes
    uint_fast64_t compress_safe_size = density_compress_safe_size(text_length);
    uint_fast64_t decompress_safe_size = density_decompress_safe_size(text_length);

    // Allocate required memory
    uint8_t *outCompressed   = malloc(compress_safe_size * sizeof(char));
    uint8_t *outDecompressed = malloc(decompress_safe_size * sizeof(char));
    density_buffer_processing_result result;

    // Compress
    result = density_buffer_compress(text, text_length, outCompressed, compress_safe_size, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, NULL);
    if(!result.state)
        printf("Compressed %llu bytes to %llu bytes\n", result.bytesRead, result.bytesWritten);

    // Decompress
    result = density_buffer_decompress(outCompressed, result.bytesWritten, outDecompressed, decompress_safe_size, NULL);
    if(!result.state)
        printf("Decompressed %llu bytes to %llu bytes\n", result.bytesRead, result.bytesWritten);

    // Free memory_allocated
    free(outCompressed);
    free(outDecompressed);
```

And that's it ! We've done a compression/decompression round trip with a few lines !
