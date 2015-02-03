0.11.2 beta
-----------
February 3, 2015

* Added an algorithms overview in README
* Removed ssc references
* Now Initializing last hash to zero on mandala kernel inits
* Reimplemented the buffer API
* Various corrections and improvements

0.11.1 beta
-----------
*January 19, 2015*

* Added a sharc benchmark in README
* Stateless memory teleport
* Improved event management and dispatching
* Improved compression/decompression finishes
* Improved streams API
* Various bug fixes, robustness improvements

0.9.12 beta
-----------
*December 2, 2013*

* Mandala kernel addition, replacing dual pass chameleon
* Simplified, faster hash function
* Fixed memory freeing issue during main encoding/decoding finish
* Implemented no footer encode output type
* Namespace migration, kernel structure reorganization
* Corrected copy mode problem
* Implemented efficiency checks and mode reversions
* Corrected lack of main header parameters retrieval
* Fixed stream not being properly ended when mode reversion occurred
* Updated metadata computations

0.9.11 beta
-----------
*November 2, 2013*

* First beta release of DENSITY, including all the compression code from SHARC in a standalone, BSD licensed library
* Added copy mode (useful for enhancing data security via the density block checksums for example)
* Makefile produces static and dynamic libraries
