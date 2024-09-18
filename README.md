density
========
Superfast compression library

**density** is a free **pure rust** (formerly C99) open-source compression library.

It is focused on high-speed compression, at the best ratio possible. **density**'s algorithms are currently at the **pareto frontier** of compression speed vs ratio (cf. [here](https://quixdb.github.io/squash-benchmark/?dataset=dickens&machine=s-desktop) for an independent
benchmark).

**density** features a simple API to enable quick integration to any project.

Why is it so fast ?
-------------------

One of the biggest assets of **density** is that its work unit, unlike most compression algorithms, is **not a byte**
but **a group of 4 bytes**.

When other libraries consume one byte of data to apply their algorithmic processing, **density** consumes 4 bytes.

That's why **density**'s algorithms have been designed from scratch. They have to cater for 4-byte work units and still
provide interesting compression ratios.

**Speed pedigree traits**

* 4-byte work units
* heavy use of registers as opposed to memory
* avoidance of or use of minimal branching when possible
* use of low memory data structures to favor processor cache Lx storage
* library wide inlining

A "blowup protection" is provided, dramatically increasing the processing speed of incompressible input data. The aim is
for compressed data to **never exceed** the original's size, even for incompressible inputs.

Benchmarks
----------

**Quick benchmark**

**density** features an integrated single-core **in-memory benchmark**.

Just use ```cargo bench``` to assess the performance of the library on your platform, as well as lz4's and snappy's, using the
*dickens* file from the renowned [silesia corpus](https://sun.aei.polsl.pl//~sdeor/index.php?page=silesia).

It's also possible to run the benchmark using your own files, with the following command: ```FILE=... cargo bench``` (replace ... with a file path).
Popular compression test files include the [silesia corpus files](https://sun.aei.polsl.pl//~sdeor/index.php?page=silesia) or [enwik8](https://mattmahoney.net/dc/textdata.html).

<details>
<summary>A sample benchmark run on MacOS Sonoma (iMac, Apple M1 processor)</summary>

```
cargo bench
Finished `bench` profile [optimized] target(s) in 0.09s
 Running unittests src/lib.rs (target/release/deps/density-b591f7be0538b3e7)

running 2 tests
test tests::chameleon ... ignored
test tests::cheetah ... ignored

test result: ok. 0 passed; 0 failed; 2 ignored; 0 measured; 0 filtered out; finished in 0.00s

     Running benches/density.rs (target/release/deps/density-2306b5bd233c0ef1)
Using file ./benches/data/dickens (10192446 bytes)
Timer precision: 41 ns
density                            fastest       │ slowest       │ median        │ mean          │ samples │ iters
├─ chameleon                                     │               │               │               │         │
│  ├─ compress/raw      (1.749x)   4.651 ms      │ 5.113 ms      │ 4.697 ms      │ 4.79 ms       │ 25      │ 25
│  │                               2.191 GB/s    │ 1.993 GB/s    │ 2.169 GB/s    │ 2.127 GB/s    │         │
│  ╰─ decompress/raw               3.575 ms      │ 4.217 ms      │ 3.584 ms      │ 3.623 ms      │ 25      │ 25
│                                  2.85 GB/s     │ 2.416 GB/s    │ 2.843 GB/s    │ 2.813 GB/s    │         │
╰─ cheetah                                       │               │               │               │         │
   ├─ compress/raw      (1.860x)   8.459 ms      │ 8.592 ms      │ 8.479 ms      │ 8.493 ms      │ 25      │ 25
   │                               1.204 GB/s    │ 1.186 GB/s    │ 1.201 GB/s    │ 1.2 GB/s      │         │
   ╰─ decompress/raw               6.021 ms      │ 6.421 ms      │ 6.027 ms      │ 6.051 ms      │ 25      │ 25
                                   1.692 GB/s    │ 1.587 GB/s    │ 1.69 GB/s     │ 1.684 GB/s    │         │

     Running benches/lz4.rs (target/release/deps/lz4-335bcba24aa068f4)
Using file ./benches/data/dickens (10192446 bytes)
Timer precision: 41 ns
lz4                                fastest       │ slowest       │ median        │ mean          │ samples │ iters
╰─ default                                       │               │               │               │         │
   ├─ compress/raw      (1.585x)   21.35 ms      │ 21.65 ms      │ 21.39 ms      │ 21.41 ms      │ 25      │ 25
   │                               477.2 MB/s    │ 470.5 MB/s    │ 476.3 MB/s    │ 476 MB/s      │         │
   ╰─ decompress/raw               3.348 ms      │ 3.793 ms      │ 3.357 ms      │ 3.381 ms      │ 25      │ 25
                                   3.044 GB/s    │ 2.686 GB/s    │ 3.035 GB/s    │ 3.014 GB/s    │         │

     Running benches/snappy.rs (target/release/deps/snappy-8184ebab72727142)
Using file ./benches/data/dickens (10192446 bytes)
Timer precision: 41 ns
snappy                             fastest       │ slowest       │ median        │ mean          │ samples │ iters
╰─ default                                       │               │               │               │         │
   ├─ compress/stream   (1.607x)   28.65 ms      │ 29.9 ms       │ 28.76 ms      │ 28.82 ms      │ 25      │ 25
   │                               355.7 MB/s    │ 340.7 MB/s    │ 354.3 MB/s    │ 353.6 MB/s    │         │
   ╰─ decompress/stream            12.83 ms      │ 13.33 ms      │ 12.89 ms      │ 12.91 ms      │ 25      │ 25
                                   793.9 MB/s    │ 764.4 MB/s    │ 790.5 MB/s    │ 789.4 MB/s    │         │

     Running benches/utils.rs (target/release/deps/utils-5b512daa91267795)

running 0 tests

test result: ok. 0 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out; finished in 0.00s
```

</details>

**Other benchmarks**

Here are a couple other benchmarks featuring **density** (non-exhaustive list) :

* [**squash**](https://github.com/quixdb/squash) is an abstraction layer for compression algorithms, and has an
  extremely exhaustive set of benchmark results, including
  density's, [available here](https://quixdb.github.io/squash-benchmark/?dataset=dickens&machine=s-desktop).

* [**lzbench**](https://github.com/inikep/lzbench) is an in-memory benchmark of open-source LZ77/LZSS/LZMA compressors.

Build
-----
**density** can be built on a number of rust-compatible platforms. First use [rustup](https://rustup.rs) to install rust. The general process is then the following:

1) Get the source code :

```
    git clone https://github.com/g1mv/density.git
    cd density
```

2) Build and test :

```
    cargo build
    cargo test
```

3) Run benchmarks with your own files :

```
    FILE=... cargo bench
```

Output format
-------------
**density** outputs compressed data in a simple format, which enables file storage and optional parallelization for both
compression and decompression.

A very short header holding vital information (like **density** version and algorithm used) precedes the binary compressed
data.

APIs
----
**density** features a straightforward *API*, simple yet powerful enough to keep users' creativity unleashed.

For advanced developers, it allows use of custom dictionaries and export of generated dictionaries after a
compression session. Although using the default, blank dictionary is perfectly fine in most cases, setting up your own,
tailored dictionaries could somewhat improve compression ratio especially for low sized input datum.

Please see the [*quick start*](#quick-start-simple-example-using-the-api) at the bottom of this page.

About the algorithms
--------------------

**density**'s algorithms are general purpose, but empirically seem to have an edge with voluminous datasets and text-based datasets.
They are, however, less than ideal for numerous, very small data items as they usually need more data to ramp up and "learn" than classical LZ-based algorithms - a disadvantage of using 4-byte work units; however, this can be alleviated by grouping small data items into bigger ones (by using *tar* when handling files for example).

**Chameleon**

Chameleon is a dictionary lookup based compression algorithm. It is designed for absolute speed and usually reaches a
compression ratio not far from 2x on compressible data.
Decompression is just as fast. This algorithm is a great choice when main concern is speed.

**Cheetah**

Cheetah was developed with inputs from [Piotr Tarsa](https://github.com/tarsa).
It is derived from chameleon and uses swapped double dictionary lookups and predictions. It can be extremely efficient with
highly compressible data (compression ratios reaching 10x or more).
On typical compressible data, compression ratio is about 2x or more. It is still extremely fast both for compression and
decompression and is a great, efficient all-rounder algorithm.

Quick start (simple example using the API)
--------------------------------------------
Using **density** in your rust code is a straightforward process.

First include the required dependency in your *Cargo.toml* file :

```toml
[dependencies]
density = "0.16.0"
```

Use the **API** :

```rust
use std::fs::read;
use density::algorithms::chameleon::chameleon::Chameleon;

fn main() {
    let file_mem = read("file_path").unwrap();

    let mut encoded_mem = vec![0_u8; file_mem.len() << 1];
    let encoded_size = Chameleon::encode(&file_mem, &mut encoded_mem).unwrap();

    let mut decoded_mem = vec![0_u8; file_mem.len() << 1];
    let decoded_size = Chameleon::decode(&encoded_mem[0..encoded_size], &mut decoded_mem).unwrap();
}
```

And that's it ! We've done a compression/decompression round trip with a few lines !