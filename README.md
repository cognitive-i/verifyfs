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

XML DSig has a very wide variety of signing and hashing permutations, but it reduces
down to the same pattern of a manifest file of digests that is signed with certificate.
This driver assumes that the manifest file signature has been checked by the caller and
that the caller can pass the driver it a list of digests in an untamperable manner.
For simplicity sha256 digests are supported, however it a simple matter to extend to
support other digests.

The sha256_digests file should be formatted in same manner as the shasum command and
the filenames within the digest file should not contain any leading slashes etc.  See
the test/makeManifest for an example.

Todo
====
* support other common digest formats
* limit directory listings to files specified in the digest file
* support MMAP file access for enhanced performance
* decouple direct POSIX file system API calls to enable GMock and GTesting
* ensure implementation is thread safe

Ideas
=====
* Application file accesses will be quite deterministic, so add common usage paths and preemptively verify files


Author
======
Atish Nazir, Cognitive-i Ltd verifyfs@cognitive-i.com
