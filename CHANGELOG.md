0.9.12 beta
-----------
<i>December 2, 2013</i>

* Mandala kernel addition, replacing dual pass chameleon
* Simplified, faster hash function
* Fixed directMemoryLocation freeing issue during main encoding/decoding finish
* Implemented no footer encode output type
* Namespace migration, kernel structure reorganization
* Corrected copy mode problem
* Implemented efficiency checks and mode reversions
* Corrected lack of main header parameters retrieval
* Fixed stream not being properly ended when mode reversion occurred
* Updated metadata computations

0.9.11 beta
-----------
<i>November 2, 2013</i>

* First beta release of DENSITY, including all the compression code from SHARC in a standalone, BSD licensed library
* Added copy mode (useful for enhancing data security via the density block checksums for example)
* Makefile produces static and dynamic libraries
