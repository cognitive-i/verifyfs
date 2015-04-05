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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FileVerifier.h"
#include "IPosixFileSystem.h"
#include "VerifyFS.h"

using namespace std;
using namespace testing;

class MockFileVerifier : public IFileVerifier
{
public:
    MOCK_CONST_METHOD1(isValidDirectoryPath, bool(const std::string& path));
    MOCK_CONST_METHOD1(isValidFilePath, bool(const std::string& path));
    MOCK_CONST_METHOD3(isValidFileBlob, bool(const std::string& path, const uint8_t* data, const size_t length));
};

class MockPosixFileSystem : public IPosixFileSystem
{
public:
    MOCK_METHOD3(open, int(const char* filename, int oflag, mode_t mode));
    MOCK_METHOD4(openat, int(int fd, const char *path, int oflag, mode_t mode));
    MOCK_METHOD1(close, int(int fd));
    MOCK_METHOD3(read, ssize_t(int fildes, void* buf, size_t nbyte));

    MOCK_METHOD2(fstat, int(int fildes, struct stat *buf));
    MOCK_METHOD4(fstatat, int(int fd, const char *path, struct stat *buf, int flag));

    MOCK_METHOD1(fdopendir, DIR*(int fd));
    MOCK_METHOD3(readdir_r, int(DIR* dirp, struct dirent* entry, struct dirent** result));
    MOCK_METHOD1(rewinddir, void(DIR *dirp));
    MOCK_METHOD1(closedir, int(DIR *dirp));
};

TEST(TestVerifyFs, OpensSourceDirectory) {

    const string SOURCE("SOME_DIRECTORY");
    MockFileVerifier mockFV;
    StrictMock<MockPosixFileSystem> mockFS;

    const int FAKE_FD = 666;
    Sequence s;
    EXPECT_CALL(mockFS, open(SOURCE.c_str(), O_RDONLY, _))
            .InSequence(s)
            .WillOnce(Return(FAKE_FD));

    EXPECT_CALL(mockFS, close(FAKE_FD))
            .InSequence(s);

    VerifyFS sut(SOURCE, mockFV, mockFS);
}


TEST(TestVerifyFs, CheckFileIsntInManifest){

    const string SOURCE("SOME_DIRECTORY");
    StrictMock<MockFileVerifier> mockFV;
    NiceMock<MockPosixFileSystem> mockFS;

    const int FAKE_FD = 666;
    Sequence s;
    EXPECT_CALL(mockFS, open(SOURCE.c_str(), O_RDONLY, _))
            .InSequence(s)
            .WillOnce(Return(FAKE_FD));

    EXPECT_CALL(mockFS, close(FAKE_FD));

    const string BADFILE1("/badfile");
    const string BADFILE2("/some/path/badfile");

    VerifyFS sut(SOURCE, mockFV, mockFS);

    // note FileVerifer APIs expect relative paths, so omit leading /
    EXPECT_CALL(mockFV, isValidFilePath(BADFILE1.substr(1)))
            .WillOnce(Return(false));

    EXPECT_CALL(mockFV, isValidFilePath(BADFILE2.substr(1)))
            .WillOnce(Return(false));

    struct fuse_file_info fi;
    fi.flags = O_RDONLY;
    EXPECT_TRUE(-EACCES == sut.fuseOpen(BADFILE1.c_str(), &fi));
    EXPECT_TRUE(-EACCES == sut.fuseOpen(BADFILE2.c_str(), &fi));

}


TEST(TestVerifyFs, ReadOnlyFileAccess){

    const string SOURCE("SOME_DIRECTORY");
    NiceMock<MockFileVerifier> mockFV;
    NiceMock<MockPosixFileSystem> mockFS;

    const int FAKE_FD = 666;
    Sequence s;
    EXPECT_CALL(mockFS, open(SOURCE.c_str(), O_RDONLY, _))
            .InSequence(s)
            .WillOnce(Return(FAKE_FD));

    EXPECT_CALL(mockFS, close(FAKE_FD));

    const string GOODFILE1("/goodfile");
    const string GOODFILE2("/some/path/goodfile");

    VerifyFS sut(SOURCE, mockFV, mockFS);

    ON_CALL(mockFV, isValidFilePath(_))
            .WillByDefault(Return(true));

    struct fuse_file_info fi;
    fi.flags = O_WRONLY;
    EXPECT_TRUE(-EACCES == sut.fuseOpen(GOODFILE1.c_str(), &fi));
    EXPECT_TRUE(-EACCES == sut.fuseOpen(GOODFILE2.c_str(), &fi));

    fi.flags = O_RDWR;
    EXPECT_TRUE(-EACCES == sut.fuseOpen(GOODFILE1.c_str(), &fi));
    EXPECT_TRUE(-EACCES == sut.fuseOpen(GOODFILE2.c_str(), &fi));
}



