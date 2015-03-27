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

#include <iostream>
#include <fstream>
#include <sstream>

#include "VerifyFS.h"
#include "FuseFSGlue.h"

using namespace std;

int verifyFSAdditionalArgs(void* data, const char* arg, int key, struct fuse_args* outargs)
{
    string& sourceMountPath = static_cast<string*>(data)[0];
    string& fileHashesPath = static_cast<string*>(data)[1];

    if(FUSE_OPT_KEY_NONOPT != key)
    {
        // we're only interested in positionals
        return 1;
    }
    else if(sourceMountPath.empty())
    {
        sourceMountPath = arg;
        return 0;
    }
    else if(fileHashesPath.empty())
    {
        fileHashesPath = arg;
        return 0;
    }
    else
        return 1;
}

map<string, string> parseFileHashes(const string& fileHashesPath)
{
    // filenames are expected to be normalised within the folder
    // so no leading ./ or absolute path prefixes.
    map<string, string> result;

    ifstream input(fileHashesPath);
    if(input.is_open())
    {
        string line;
        while(getline(input, line))
        {
            const string hash = line.substr(0, 64);
            const string filename = line.substr(66);
            result[filename] = hash;
        }
    }

    return result;
}

int main(int argc, char* argv[])
{
    // VerifyFS <sourcefolder> <hashesfile> <mountpoint>
    string sourceAndHash[2];
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    fuse_opt_parse(&args, &sourceAndHash, NULL, verifyFSAdditionalArgs);

    // create fuse filesystem
    const map<std::string, std::string> fileHashes = parseFileHashes(sourceAndHash[1]);
    VerifyFS verifyFS(sourceAndHash[0], fileHashes);

    // activate
    int result = startFuseFSProvider(args.argc, args.argv, &verifyFS);
    fuse_opt_free_args(&args);
    return result;
}

