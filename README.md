VerifyFS
==========================
VerifyFS is a FUSE driver that performs on demand integrity checking of files
prior to access.  Ideal for the quick loading and running applications distributed
within XML Dsig signed package formats such as .jar, .air and .wgt.


Installation
============
The project uses pkg-config for locating Fuse and OpenSSL

MacOS
port install osxfuse openssl


**GitHub Repository**
https://github.com/cognitive-i/verifyfs

Author
======
Atish Nazir, Cognitive-i Ltd verifyfs@cognitive-i.com
