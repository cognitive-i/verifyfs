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

#include <functional>

using namespace std;
using namespace testing;
using namespace std::placeholders;

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




namespace {

int fakeStatInjector(struct stat* in, struct stat* out)
{
    memcpy(out, in, sizeof(struct stat));
    return 0;
}

int fakeReadInjector(void* in, void* out, const size_t count)
{
    memcpy(out, in, count);
    return count;
}

MATCHER_P(binaryMatcher, d, "") { return (0 == memcmp(arg, d.data(), d.length())); }

} // helper namespace

TEST(TestVerifyFs, OpenActualFile){

    const string SOURCE("SOME_DIRECTORY");
    StrictMock<MockFileVerifier> mockFV;
    StrictMock<MockPosixFileSystem> mockFS;


    const int FD_DIRECTORY = 1000;
    const int FD_UNTRUSTED_FILE = 1001;

    const string GOODFILE_ABS("/goodfile");
    const string GOODFILE_RELATIVE = GOODFILE_ABS.substr(1);
    const string GOODFILE_DATA = "hello world whoop whoop";
    struct stat GOODFILE_STAT;
    bzero(&GOODFILE_STAT, sizeof(struct stat));
    GOODFILE_STAT.st_size = GOODFILE_DATA.length();


    Sequence s;
    EXPECT_CALL(mockFS, open(StrEq(SOURCE.c_str()), O_RDONLY, _))
            .InSequence(s)
            .WillOnce(Return(FD_DIRECTORY));

    EXPECT_CALL(mockFV, isValidFilePath(GOODFILE_RELATIVE))
            .InSequence(s)
            .WillOnce(Return(true));

    EXPECT_CALL(mockFS, openat(FD_DIRECTORY, StrEq(GOODFILE_RELATIVE.c_str()), O_RDONLY, _))
            .InSequence(s)
            .WillOnce(Return(FD_UNTRUSTED_FILE));

    auto fstatter = bind(fakeStatInjector, &GOODFILE_STAT, _1);
    EXPECT_CALL(mockFS, fstat(FD_UNTRUSTED_FILE, _))
            .InSequence(s)
            .WillOnce(WithArgs<1>(Invoke(fstatter)));

    auto reader = bind(fakeReadInjector, (void*) GOODFILE_DATA.data(), _1, _2);
    EXPECT_CALL(mockFS, read(FD_UNTRUSTED_FILE, _, GOODFILE_STAT.st_size))
            .InSequence(s)
            .WillOnce(WithArgs<1,2>(Invoke(reader)));

    EXPECT_CALL(mockFS, close(FD_UNTRUSTED_FILE))
            .InSequence(s)
            .WillOnce(Return(0));

    EXPECT_CALL(mockFV, isValidFileBlob(GOODFILE_RELATIVE, binaryMatcher(GOODFILE_DATA), GOODFILE_DATA.length()))
            .InSequence(s)
            .WillOnce(Return(true));

    EXPECT_CALL(mockFS, close(FD_DIRECTORY))
            .InSequence(s)
            .WillOnce(Return(0));

    VerifyFS sut(SOURCE, mockFV, mockFS);

    struct fuse_file_info fi;
    fi.flags = O_RDONLY;
    EXPECT_EQ(0, sut.fuseOpen(GOODFILE_ABS.c_str(), &fi));
}




