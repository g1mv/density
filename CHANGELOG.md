0.11.4 beta
-----------
*February 10, 2015*

* Removed unnecessary makefile, now using a single makefile
* Mandala kernel : reset prediction entries as required
* Mandala kernel : convert hash to 16-bit unsigned int before storing
* Updated SpookyHash to 1.0.2

0.11.3 beta
-----------
*February 5, 2015*

* Added integrity check system
* Corrected pointer usage and update on footer read/writes
* Now freeing kernel state memory only when compression mode is not copy
* Updated Makefiles
* Improved memory teleport
* Fixed sequencing problem after kernels request a new block

0.11.2 beta
-----------
*February 3, 2015*

* Added an algorithms overview in README
* Removed ssc references
* Now initializing last hash to zero on mandala kernel inits
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

0.10.2 beta
-----------
*January 7, 2015*

* Improved organization of compile-time switches and run-time options in the API
* Removed method density_stream_decompress_utilities_get_header from the API, header info is now returned in the density_stream_decompress_init function
* Corrected readme to reflect API changes

0.10.1 beta
-----------
*January 5, 2015*

* Re-added mandala kernel
* Corrected available bytes adjustment problem
* Added missing restrict keywords
* Cleaned unnecessary defines

0.10.0 beta
-----------
*January 2, 2015*

* Complete stream API redesign to greatly improve flexibility
* Only one supported algorithm for now : Chameleon

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
