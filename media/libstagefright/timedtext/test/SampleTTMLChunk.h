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
#ifndef TEST_SAMPLETTMLCHUNK_H_
#define TEST_SAMPLETTMLCHUNK_H_

namespace android {
namespace test {

static const char *kEmptyTTMLString =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\">"
    "  <head/>"
    "  <body style=\"normal\" region=\"bottom\">"
    "  </body></tt>";

static const char *kTTMLWithHeadTag =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
    "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
    "  <head>"
    "    <metadata>"
    "      <ttm:title>Document Metadata Example</ttm:title>"
    "    </metadata>"
    "    <styling>"
    "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\" />"
    "    </styling>"
    "  </head>"
    "  <body style=\"normal\" region=\"bottom\">"
    "    <div begin=\"00:10:00:00\">"
    "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 1-1</p>"
    "   </div>"
    "  </body></tt>";

static const char *kTTMLWithBrAndSpanTag =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
    "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
    "  <head>"
    "    <metadata>"
    "      <ttm:title>Document Metadata Example</ttm:title>"
    "    </metadata>"
    "    <styling>"
    "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\" />"
    "    </styling>"
    "  </head>"
    "  <body style=\"normal\" region=\"bottom\">"
    "    <div begin=\"00:10:00:00\">"
    "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 1-1<br />"
    "Subtitle <span style=\"italic\">1-2</span></p>"
    "   </div>"
    "  </body></tt>";

static const char *kTTMLWithMultipleSpaces =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
    "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">"
    "  <head>"
    "    <metadata>"
    "      <ttm:title>Document Metadata Example</ttm:title>"
    "    </metadata>"
    "    <styling>"
    "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\" />"
    "    </styling>"
    "  </head>"
    "  <body style=\"normal\" region=\"bottom\">"
    "    <div begin=\"00:10:00:00\">"
    "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">"
    "        Subtitle  1-1"
    "      </p>"
    "   </div>"
    "  </body></tt>";

static const char *kTTMLWithMetaData =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
    "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
    "     xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\""
    "     xmlns:smpte=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt\">"
    "  <head>"
    "    <metadata>"
    "      <ttm:title>Document Metadata Example</ttm:title>"
    "    </metadata>"
    "    <styling>"
    "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\" />"
    "    </styling>"
    "  </head>"
    "  <body style=\"normal\">"
    "    <div>"
    "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">"
    "         <metadata>"
    "           <smpte:image imagetype=\"PNG\" encoding=\"Base64\">"
    "             iVBO...gg=="
    "           </smpte:image>"
    "         </metadata>"
    "      </p>"
    "   </div>"
    "  </body></tt>";

static const char *kTTMLEmptyChunk[] = { kEmptyTTMLString };
static const char *kTTMLSimpleChunk[] = { kTTMLWithHeadTag };
static const char *kTTMLChunkWithBrAndSpan[] = { kTTMLWithBrAndSpanTag };
static const char *kTTMLChunkWithMultipleSpaces[] = { kTTMLWithMultipleSpaces };
static const char *kTTMLChunkWithMetaData[] = { kTTMLWithMetaData };
static const char *kTTMLChunksIncludingEmptyChunk[] = {
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\">"
      "  <head/>"
      "  <body>"
      "    <div>"
      "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 1-1</p>"
      "      <p begin=\"00:00:30:00\" end=\"00:00:40:10\">Subtitle 1-2</p>"
      "      <p begin=\"00:00:50:00\" end=\"00:01:00:10\">Subtitle 1-3</p>"
      "   </div>"
      "</body></tt>",
  kEmptyTTMLString,
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\">"
      "  <head/>"
      "  <body>"
      "    <div>"
      "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 3-1</p>"
      "      <p begin=\"00:00:30:00\" end=\"00:00:40:10\">Subtitle 3-2</p>"
      "      <p begin=\"00:00:50:00\" end=\"00:01:00:10\">Subtitle 3-3</p>"
      "   </div>"
      "</body></tt>"
};

static const char *kTTMLChunks[] = {
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
      "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
      "     xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\""
      "     xml:lang=\"zh-TW\" ttp:timeBase=\"smpte\""
      "     ttp:frameRate=\"24\" ttp:frameRateMultiplier=\"999 1000\""
      "     ttp:dropMode=\"nonDrop\" xmlns=\"http://www.w3.org/ns/ttml\">"
      "  <head>"
      "    <styling>"
      "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"normal\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "      <style xml:id=\"italic\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"italic\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "    </styling>"
      "    <layout>"
      "      <region xml:id=\"top\" tts:origin=\"0% 0%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"before\" />"
      "      <region xml:id=\"bottom\" tts:origin=\"0% 85%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"after\" />"
      "    </layout>"
      "  </head>"
      "  <body style=\"normal\" region=\"bottom\">"
      "    <div begin=\"00:00:00:00\">"
      "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 1-1</p>"
      "      <p begin=\"00:00:30:00\" end=\"00:00:40:10\">Subtitle 1-2</p>"
      "      <p begin=\"00:00:50:00\" end=\"00:01:00:10\">Subtitle 1-3</p>"
      "   </div>"
      "</body></tt>",

  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
      "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
      "     xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\""
      "     xml:lang=\"zh-TW\" ttp:timeBase=\"smpte\""
      "     ttp:frameRate=\"24\" ttp:frameRateMultiplier=\"999 1000\""
      "     ttp:dropMode=\"nonDrop\" xmlns=\"http://www.w3.org/ns/ttml\"> "
      "  <head>"
      "    <styling>"
      "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"normal\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "      <style xml:id=\"italic\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"italic\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "    </styling>"
      "    <layout>"
      "      <region xml:id=\"top\" tts:origin=\"0% 0%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"before\" />"
      "      <region xml:id=\"bottom\" tts:origin=\"0% 85%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"after\" />"
      "    </layout>"
      "  </head>"
      "  <body style=\"normal\" region=\"bottom\">"
      "    <div begin=\"00:01:00:00\">"
      "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 2-1</p>"
      "      <p begin=\"00:00:30:00\" end=\"00:00:40:10\">Subtitle 2-2</p>"
      "      <p begin=\"00:00:50:00\" end=\"00:01:00:10\">Subtitle 2-3</p>"
      "   </div>"
      "</body></tt>",

  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      " <tt xmlns:tt=\"http://www.w3.org/ns/ttml\""
      "     xmlns:tts=\"http://www.w3.org/ns/ttml#styling\""
      "     xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\""
      "     xml:lang=\"zh-TW\" ttp:timeBase=\"smpte\""
      "     ttp:frameRate=\"24\" ttp:frameRateMultiplier=\"999 1000\""
      "     ttp:dropMode=\"nonDrop\" xmlns=\"http://www.w3.org/ns/ttml\"> "
      "  <head>"
      "    <styling>"
      "      <style xml:id=\"normal\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"normal\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "      <style xml:id=\"italic\" tts:fontFamily=\"sansSerif\""
      "             tts:fontWeight=\"normal\" tts:fontStyle=\"italic\""
      "             tts:color=\"rgb(255,255,255)\" tts:fontSize=\"100%\" />"
      "    </styling>"
      "    <layout>"
      "      <region xml:id=\"top\" tts:origin=\"0% 0%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"before\" />"
      "      <region xml:id=\"bottom\" tts:origin=\"0% 85%\""
      "              tts:extent=\"100% 15%\" tts:textAlign=\"center\""
      "              tts:displayAlign=\"after\" />"
      "    </layout>"
      "  </head>"
      "  <body style=\"normal\" region=\"bottom\">"
      "    <div begin=\"00:02:00:00\">"
      "      <p begin=\"00:00:10:00\" end=\"00:00:20:10\">Subtitle 3-1</p>"
      "      <p begin=\"00:00:30:00\" end=\"00:00:40:10\">Subtitle 3-2</p>"
      "      <p begin=\"00:00:50:00\" end=\"00:01:00:10\">Subtitle 3-3</p>"
      "   </div>"
      "</body></tt>"
};

}  // namespace test
}  // namespace android
#endif  // TEST_SAMPLETTMLCHUNK_H_
