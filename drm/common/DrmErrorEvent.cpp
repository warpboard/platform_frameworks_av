/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <utils/String8.h>
#include <drm/DrmErrorEvent.h>
#include <stdlib.h>

using namespace android;

DrmErrorEvent::DrmErrorEvent(int uniqueId, int infoType, const String8 message)
    : mUniqueId(uniqueId),
      mInfoType(infoType),
      mMessage(message),
      mDrmBuffer() {

}

DrmErrorEvent::DrmErrorEvent(int uniqueId, int infoType, const String8 message,
        const DrmBuffer& drmBuffer)
        : mUniqueId(uniqueId), mInfoType(infoType), mMessage(message), mDrmBuffer() {
    setData(drmBuffer);
}

DrmErrorEvent::~DrmErrorEvent() {
    delete [] mDrmBuffer.data;
}


int DrmErrorEvent::getUniqueId() const {
    return mUniqueId;
}

int DrmErrorEvent::getType() const {
    return mInfoType;
}

const String8 DrmErrorEvent::getMessage() const {
    return mMessage;
}

int DrmErrorEvent::getCount() const {
    return mAttributes.size();
}

status_t DrmErrorEvent::put(const String8& key, String8& value) {
        mAttributes.add(key, value);
    return DRM_NO_ERROR;
}

const String8 DrmErrorEvent::get(const String8& key) const {
    if (mAttributes.indexOfKey(key) != NAME_NOT_FOUND) {
        return mAttributes.valueFor(key);
    }
    return String8("");
}

const DrmBuffer& DrmErrorEvent::getData() const {
    return mDrmBuffer;
}

void DrmErrorEvent::setData(const DrmBuffer& drmBuffer) {
    delete [] mDrmBuffer.data;
    mDrmBuffer.data = new char[drmBuffer.length];;
    mDrmBuffer.length = drmBuffer.length;
    memcpy(mDrmBuffer.data, drmBuffer.data, drmBuffer.length);
}

DrmErrorEvent::KeyIterator DrmErrorEvent::keyIterator() const {
    return KeyIterator(this);
}

DrmErrorEvent::Iterator DrmErrorEvent::iterator() const {
    return Iterator(this);
}

// KeyIterator implementation
DrmErrorEvent::KeyIterator::KeyIterator(const DrmErrorEvent::KeyIterator& keyIterator)
        : mDrmErrorEvent(keyIterator.mDrmErrorEvent), mIndex(keyIterator.mIndex) {
}

bool DrmErrorEvent::KeyIterator::hasNext() {
    return (mIndex < mDrmErrorEvent->mAttributes.size());
}

const String8& DrmErrorEvent::KeyIterator::next() {
    const String8& key = mDrmErrorEvent->mAttributes.keyAt(mIndex);
    mIndex++;
    return key;
}

DrmErrorEvent::KeyIterator& DrmErrorEvent::KeyIterator::operator=(
        const DrmErrorEvent::KeyIterator& keyIterator) {
    mDrmErrorEvent = keyIterator.mDrmErrorEvent;
    mIndex = keyIterator.mIndex;
    return *this;
}

// Iterator implementation
DrmErrorEvent::Iterator::Iterator(const DrmErrorEvent::Iterator& iterator)
        : mDrmErrorEvent(iterator.mDrmErrorEvent), mIndex(iterator.mIndex) {
}

DrmErrorEvent::Iterator& DrmErrorEvent::Iterator::operator=(const DrmErrorEvent::Iterator& iterator) {
    mDrmErrorEvent = iterator.mDrmErrorEvent;
    mIndex = iterator.mIndex;
    return *this;
}

bool DrmErrorEvent::Iterator::hasNext() {
    return mIndex < mDrmErrorEvent->mAttributes.size();
}

const String8& DrmErrorEvent::Iterator::next() {
    const String8& value = mDrmErrorEvent->mAttributes.editValueAt(mIndex);
    mIndex++;
    return value;
}

