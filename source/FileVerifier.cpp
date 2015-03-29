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

#include "FileVerifier.h"
#include <fstream>
#include <openssl/sha.h>
#include <exception>

using namespace std;

FileVerifier::FileVerifier(const string& digestsPath)
{
    ifstream input(digestsPath);
    if(input.is_open())
    {
        string line;
        while(getline(input, line))
        {
            const string hash = line.substr(0, 64);
            const string filename = line.substr(66);
            mDigests[filename] = hash;
        }
    }
    else
        throw runtime_error("Unable to open digests file");
}

bool FileVerifier::isValidDirectoryPath(const std::string& path) const
{
    return true;
}

bool FileVerifier::isValidFilePath(const std::string& path) const
{
    return (! getFileDigest(path).empty());
}

bool FileVerifier::isValidFileBlob(const string& path, const uint8_t* data, const size_t length) const
{
    const string expectedDigest = getFileDigest(path);
    if(expectedDigest.empty())
        return false;

    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(data, length, digest);

    string digestHex;
    int i;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        char byteBuffer[3];
        sprintf(byteBuffer, "%02x", digest[i]);
        digestHex += byteBuffer;
    }

    return (digestHex == expectedDigest);
}

const string FileVerifier::getFileDigest(const string& path) const
{
    auto h = mDigests.find(path);
    return (mDigests.end() != h) ? h->second : string();
}
