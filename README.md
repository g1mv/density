libssc <a href="http://www.centaurean.com"><img src="http://www.centaurean.com/images/centaurean.png" width="140" align="right" title="Centaurean"/></a>
========
Superfast Symmetric Compression library

<b>libssc</b> is a free C99, open-source, BSD licensed compression library.

It is focused on high-speed compression, at the best ratio possible.
The word "symmetric" means that compression and decompression speeds are expected to be similar (i.e. very fast).
<b>libssc</b> features two APIs to enable quick integration in any project.

Benchmark
---------
Here is a benchmark of <b>libssc</b>'s fastest mode compared to other libraries, on an Intel® Core™ i3-2105	(x86 64), Asus P8H61-H motherboard with Fedora 19. It is possible to run yours using <a href=https://github.com/quixdb/squash>this project</a>.
<table><tr><td><img src=http://www.libssc.net/images/ratio.png /></td><td><img src=http://www.libssc.net/images/i3.png /></td></tr></table>

API
---
<b>libssc</b> features a *stream API* and a *buffer API* which are very simple to use, yet powerful enough to keep users' creativity unleashed. They are <a href=http://www.libssc.net/apis.html>described here</a>.

Output
------
<b>libssc</b> outputs compressed data in a simple format, which enables file storage and parallelization for both compression and decompression.
![libssc output format](http://www.libssc.net/images/ssc_output_format.png)
More <a href=http://www.libssc.net/format.html>details here</a>.

Build
-----
<b>libssc</b> is fully C99 compliant and can therefore be built on a number of platforms. Build instructions are <a href=http://www.libssc.net/build.html>detailed here</a>.

FAQ
---
The FAQ can be <a href=http://www.libssc.net/faq.html>found here</a>.
