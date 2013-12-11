# Installation Guide

claire-protorpc is a collection of utility for build modern rpc-based application. 

It is designed for used in x86-64 Linux. Beacause claire-nettyuse some new kernel api(timerfd, eventfd...), kernel version require at least more than 2.6.32.

Currently claire-protorpc use [CMake][1] build the library, require at least 2.8 version.

It depends claire-common and claire-netty (with all its dependent library) and serveral other library:
 
- [snappy][2]: claire-protorpc use c-ares support dns resolver.

After install all before, run ./build.sh and ./build.sh install.

Done.


  [1]: http://www.cmake.org/
  [2]: https://code.google.com/p/snappy/