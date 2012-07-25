/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FILE_SOURCE_H_

#define FILE_SOURCE_H_

#include <stdio.h>

#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaErrors.h>
#include <utils/threads.h>
#include <drm/DrmManagerClient.h>

namespace android {

#define FILE_SOURCE_META_BUFFER_NUMBER                6
// must be power of 2
#define FILE_SOURCE_META_BUFFER_DATA_SIZE             (4*1024)//0x1000 // 4096
#define FILE_SOURCE_META_BUFFER_DATA_LOCAL_OFFSET     (FILE_SOURCE_META_BUFFER_DATA_SIZE - 1)
#define FILE_SOURCE_META_BUFFER_DATA_OFFSET           ~(FILE_SOURCE_META_BUFFER_DATA_LOCAL_OFFSET)
#define FILE_SOURCE_META_BUFFER_LRU_COUNT_LIMIT       100
struct FileSourceMetaBuffer {
    char data[FILE_SOURCE_META_BUFFER_DATA_SIZE];
    off_t offset;
    char valid;
    int count; // for LRU calculation
    size_t len; // actual bytes in buffer
};

class FileSourceBuffer {
public:
    FileSourceBuffer();
    ssize_t readFromBuffer(off64_t offset, void *data, size_t size, int file);

private:
    struct FileSourceMetaBuffer mMetaBuffer[FILE_SOURCE_META_BUFFER_NUMBER];
    int mNextBuffer;
    Mutex mLock;
};


class FileSource : public DataSource {
public:
    FileSource(const char *filename);
    FileSource(int fd, int64_t offset, int64_t length);

    virtual status_t initCheck() const;

    virtual ssize_t readAt(off64_t offset, void *data, size_t size);

    virtual status_t getSize(off64_t *size);

    virtual sp<DecryptHandle> DrmInitialization(const char *mime);

    virtual void getDrmInfo(sp<DecryptHandle> &handle, DrmManagerClient **client);

protected:
    virtual ~FileSource();

private:
    int mFd;
    int64_t mOffset;
    int64_t mLength;
    Mutex mLock;
    FileSourceBuffer mBuffer;

    /*for DRM*/
    sp<DecryptHandle> mDecryptHandle;
    DrmManagerClient *mDrmManagerClient;
    int64_t mDrmBufOffset;
    int64_t mDrmBufSize;
    unsigned char *mDrmBuf;

    ssize_t readAtDRM(off64_t offset, void *data, size_t size);

    FileSource(const FileSource &);
    FileSource &operator=(const FileSource &);
};

}  // namespace android

#endif  // FILE_SOURCE_H_

