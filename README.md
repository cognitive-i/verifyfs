VerifyFS
==========================
VerifyFS is a FUSE driver that performs on demand integrity checking of files
prior to access.  Ideal for the quick loading and running applications distributed
within XML Dsig signed package formats such as .jar, .air and .wgt.

**GitHub Repository**
https://github.com/cognitive-i/verifyfs

Installation
============
The project uses CMake and pkg-config for locating Fuse and OpenSSL.

**MacOS**
port install osxfuse openssl

**Debian/Ubuntu/Xubuntu**
apt-get install libfuse-dev libssl-dev


Usage
=====
VerifyFS source_folder sha256_digests mount_point

Note
====
The sha256_digests file should be formatted in same manner as the shasum command and
the filenames within the digest file should not contain any leading slashes etc.  See
the test/makeManifest for an example.

Author
======
Atish Nazir, Cognitive-i Ltd verifyfs@cognitive-i.com
