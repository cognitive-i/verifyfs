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

#include "VerifyFS.h"

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdio.h>

using namespace std;

namespace {
const char* currentDir = ".";

inline const char* correctPath(const char* fusePath)
{
    return (0 == strcmp(fusePath, "/"))? currentDir : fusePath + 1;
}

} // namespace

VerifyFS::VerifyFS(const string& untrustedPath, const IFileVerifier& fileVerifier, IPosixFileSystem& filesystem) :
    mUntrustedPath(untrustedPath),
    mFileVerifier(fileVerifier),
    mFS(filesystem)
{
    // maintain a FD link with source folder to use openat later on
    mSourceFolderFd = mFS.open(mUntrustedPath.c_str(), O_RDONLY);
    if(-1 == mSourceFolderFd)
        throw runtime_error("Unable to open source folder");
}

VerifyFS::~VerifyFS()
{
    mFS.close(mSourceFolderFd);
}

int VerifyFS::fuseStat(const char* path, struct stat* stbuf)
{
    const char* correctedPath = correctPath(path);
    return mFS.fstatat(mSourceFolderFd, correctedPath, stbuf, 0);
}

int VerifyFS::fuseOpendir(const char* path, struct fuse_file_info* fi)
{
    const char* correctedPath = correctPath(path);
    DIR* dh = fdopendir(mFS.openat(mSourceFolderFd, correctedPath, O_RDONLY));
    if(dh)
    {
        fi->fh = dirfd(dh);
        fdDir[fi->fh] = dh;
        return 0;
    }
    else
        return -ENOENT;
}

int VerifyFS::fuseReaddir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
    auto i = fdDir.find(fi->fh);
    if(fdDir.end() != i)
    {
        DIR* fdir = i->second;
        rewinddir(fdir);

        dirent dentry;
        dirent* pDentry = &dentry;
        while((0 == readdir_r(fdir, pDentry, &pDentry)) && pDentry)
            filler(buf, dentry.d_name, NULL, 0);

        return 0;
    }
    else
        return -ENOENT;
}

int VerifyFS::fuseReleasedir (const char* path, struct fuse_file_info* fi)
{
    auto i = fdDir.find(fi->fh);
    if(fdDir.end() != i)
    {
        closedir(i->second);
        fdDir.erase(i);
    }

    return 0;
}


int VerifyFS::fuseOpen(const char* path, struct fuse_file_info* fi)
{
    const char* correctedPath = correctPath(path);
    const int accessMode = fi->flags & O_ACCMODE;

    // only permit readonly
    if(O_RDONLY != accessMode)
        return -EACCES;

    if(! mFileVerifier.isValidFilePath(correctedPath))
        return -EACCES;

    return openAndVerify(correctedPath);
}

int VerifyFS::fuseRead(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    int result = -EACCES;
    string fullpath = mUntrustedPath + path;
    auto f = mTrustedFiles.find(fullpath);
    if(mTrustedFiles.end() != f)
    {
        vector<uint8_t>& fileData = f->second;
        if(offset < fileData.size())
        {
            size_t bytesRead = min((size_t)(fileData.size() - offset), (size_t) size);
            memcpy(buf, fileData.data()+offset, bytesRead);
            result = bytesRead;
        }
    }

    return result;
}

int VerifyFS::fuseRelease(const char* path, struct fuse_file_info* fi)
{
    string fullpath = mUntrustedPath + path;
    mTrustedFiles.erase(fullpath);
    return 0;
}

int VerifyFS::openAndVerify(const string& path)
{
    string fullpath = mUntrustedPath + '/' + path;

    int result = -ENOENT;
    int fh = mFS.openat(mSourceFolderFd, path.c_str(), O_RDONLY);
    if(-1 != fh)
    {
        struct stat details;
        mFS.fstat(fh, &details);

        vector<uint8_t> buffer;
        buffer.resize(details.st_size);

        const off_t bytesRead = mFS.read(fh, buffer.data(), details.st_size);
        mFS.close(fh);

        if(bytesRead == details.st_size)
        {
            bool isGood = mFileVerifier.isValidFileBlob(path, buffer.data(), buffer.size());
            if(isGood)
            {
                mTrustedFiles[fullpath] = buffer;
                result = 0;
            }
            else
                cerr << "Failed validation:  " << fullpath << endl;

        }
    }

    return result;
}


