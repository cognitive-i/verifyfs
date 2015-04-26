/*
 * Copyright (c) 2015, Cognitive-i Ltd (verifyfs@cognitive-i.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 *    list of conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef POSIXFILESYSTEM_H
#define POSIXFILESYSTEM_H

#include "IPosixFileSystem.h"

class PosixFileSystem : public IPosixFileSystem
{    
    // IPosixFileSystem interface
public:
    virtual int open(const char* filename, int oflag, mode_t mode);
    virtual int openat(int fd, const char* path, int oflag, mode_t mode);    
    virtual int close(int fd);
    virtual ssize_t read(int fildes, void* buf, size_t nbyte);

    virtual int fstat(int fildes, struct stat* buf);
    virtual int fstatat(int fd, const char *path, struct stat* buf, int flag);

    virtual DIR* fdopendir(int fd);
    virtual int readdir_r(DIR* dirp, dirent* entry, dirent** result);
    virtual void rewinddir(DIR *dirp);
    virtual int closedir(DIR* dirp);
};

#endif // POSIXFILESYSTEM_H