// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/shaping/HarfBuzzShaper.h"

#include <unicode/uscript.h>

#include "build/build_config.h"
#include "platform/fonts/Font.h"
#include "platform/fonts/FontCache.h"
#include "platform/fonts/FontTestUtilities.h"
#include "platform/fonts/shaping/ShapeResultInlineHeaders.h"
#include "platform/fonts/shaping/ShapeResultTestInfo.h"
#include "platform/testing/FontTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/text/TextBreakIterator.h"
#include "platform/text/TextRun.h"
#include "platform/wtf/Vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class HarfBuzzShaperTest : public ::testing::Test {
 protected:
  void SetUp() override {
    font_description.SetComputedSize(12.0);
    font = Font(font_description);
    font.Update(nullptr);
  }

  void TearDown() override {}

  FontCachePurgePreventer font_cache_purge_preventer;
  FontDescription font_description;
  Font font;
  unsigned start_index = 0;
  unsigned num_characters = 0;
  unsigned num_glyphs = 0;
  hb_script_t script = HB_SCRIPT_INVALID;
};

static inline ShapeResultTestInfo* TestInfo(RefPtr<ShapeResult>& result) {
  return static_cast<ShapeResultTestInfo*>(result.Get());
}

TEST_F(HarfBuzzShaperTest, MutableUnique) {
  RefPtr<ShapeResult> result =
      ShapeResult::Create(&font, 0, TextDirection::kLtr);
  EXPECT_EQ(1, result->RefCount());

  // At this point, |result| has only one ref count.
  RefPtr<ShapeResult> result2 = result->MutableUnique();
  EXPECT_EQ(result.Get(), result2.Get());
  EXPECT_EQ(2, result2->RefCount());

  // Since |result| has 2 ref counts, it should return a clone.
  RefPtr<ShapeResult> result3 = result->MutableUnique();
  EXPECT_NE(result.Get(), result3.Get());
  EXPECT_EQ(1, result3->RefCount());
  EXPECT_EQ(2, result->RefCount());
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsLatin) {
  String latin_common = To16Bit("ABC DEF.", 8);
  HarfBuzzShaper shaper(latin_common.Characters16(), 8);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(8u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsLeadingCommon) {
  String leading_common = To16Bit("... test", 8);
  HarfBuzzShaper shaper(leading_common.Characters16(), 8);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(8u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsUnicodeVariants) {
  struct {
    const char* name;
    UChar string[4];
    unsigned length;
    hb_script_t script;
  } testlist[] = {
      {"Standard Variants text style", {0x30, 0xFE0E}, 2, HB_SCRIPT_COMMON},
      {"Standard Variants emoji style", {0x203C, 0xFE0F}, 2, HB_SCRIPT_COMMON},
      {"Standard Variants of Ideograph", {0x4FAE, 0xFE00}, 2, HB_SCRIPT_HAN},
      {"Ideographic Variants", {0x3402, 0xDB40, 0xDD00}, 3, HB_SCRIPT_HAN},
      {"Not-defined Variants", {0x41, 0xDB40, 0xDDEF}, 3, HB_SCRIPT_LATIN},
  };
  for (auto& test : testlist) {
    HarfBuzzShaper shaper(test.string, test.length);
    RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

    EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting()) << test.name;
    ASSERT_TRUE(
        TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script))
        << test.name;
    EXPECT_EQ(0u, start_index) << test.name;
    if (num_glyphs == 2) {
// If the specified VS is not in the font, it's mapped to .notdef.
// then hb_ot_hide_default_ignorables() swaps it to a space with zero-advance.
// http://lists.freedesktop.org/archives/harfbuzz/2015-May/004888.html
#if !defined(OS_MACOSX)
      EXPECT_EQ(TestInfo(result)->FontDataForTesting(0)->SpaceGlyph(),
                TestInfo(result)->GlyphForTesting(0, 1))
          << test.name;
#endif
      EXPECT_EQ(0.f, TestInfo(result)->AdvanceForTesting(0, 1)) << test.name;
    } else {
      EXPECT_EQ(1u, num_glyphs) << test.name;
    }
    EXPECT_EQ(test.script, script) << test.name;
  }
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsDevanagariCommon) {
  UChar devanagari_common_string[] = {0x915, 0x94d, 0x930, 0x28, 0x20, 0x29};
  String devanagari_common_latin(devanagari_common_string, 6);
  HarfBuzzShaper shaper(devanagari_common_latin.Characters16(), 6);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(2u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsDevanagariCommonLatinCommon) {
  UChar devanagari_common_latin_string[] = {0x915, 0x94d, 0x930, 0x20,
                                            0x61,  0x62,  0x2E};
  HarfBuzzShaper shaper(devanagari_common_latin_string, 7);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(3u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_DEVANAGARI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(2, start_index, num_glyphs, script));
  EXPECT_EQ(4u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabicThaiHanLatin) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  EXPECT_EQ(4u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_ARABIC, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(1, start_index, num_glyphs, script));
  EXPECT_EQ(3u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_THAI, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(2, start_index, num_glyphs, script));
  EXPECT_EQ(4u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_HAN, script);

  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(3, start_index, num_glyphs, script));
  EXPECT_EQ(5u, start_index);
  EXPECT_EQ(1u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_LATIN, script);
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabicThaiHanLatinTwice) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  EXPECT_EQ(4u, TestInfo(result)->NumberOfRunsForTesting());

  // Shape again on the same shape object and check the number of runs.
  // Should be equal if no state was retained between shape calls.
  RefPtr<ShapeResult> result2 = shaper.Shape(&font, TextDirection::kLtr);
  EXPECT_EQ(4u, TestInfo(result2)->NumberOfRunsForTesting());
}

TEST_F(HarfBuzzShaperTest, ResolveCandidateRunsArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  HarfBuzzShaper shaper(arabic_string, 3);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kRtl);

  EXPECT_EQ(1u, TestInfo(result)->NumberOfRunsForTesting());
  ASSERT_TRUE(
      TestInfo(result)->RunInfoForTesting(0, start_index, num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(3u, num_glyphs);
  EXPECT_EQ(HB_SCRIPT_ARABIC, script);
}

// This is a simplified test and doesn't accuratly reflect how the shape range
// is to be used. If you instead of the string you imagine the following HTML:
// <div>Hello <span>World</span>!</div>
// It better reflects the intended use where the range given to each shape call
// corresponds to the text content of a TextNode.
TEST_F(HarfBuzzShaperTest, ShapeLatinSegment) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  RefPtr<ShapeResult> combined = shaper.Shape(&font, direction);
  RefPtr<ShapeResult> first = shaper.Shape(&font, direction, 0, 6);
  RefPtr<ShapeResult> second = shaper.Shape(&font, direction, 6, 11);
  RefPtr<ShapeResult> third = shaper.Shape(&font, direction, 11, 12);

  ASSERT_TRUE(TestInfo(first)->RunInfoForTesting(0, start_index, num_characters,
                                                 num_glyphs, script));
  EXPECT_EQ(0u, start_index);
  EXPECT_EQ(6u, num_characters);
  ASSERT_TRUE(TestInfo(second)->RunInfoForTesting(
      0, start_index, num_characters, num_glyphs, script));
  EXPECT_EQ(6u, start_index);
  EXPECT_EQ(5u, num_characters);
  ASSERT_TRUE(TestInfo(third)->RunInfoForTesting(0, start_index, num_characters,
                                                 num_glyphs, script));
  EXPECT_EQ(11u, start_index);
  EXPECT_EQ(1u, num_characters);

  HarfBuzzShaper shaper2(string.Characters16(), 6);
  RefPtr<ShapeResult> first_reference = shaper2.Shape(&font, direction);

  HarfBuzzShaper shaper3(string.Characters16() + 6, 5);
  RefPtr<ShapeResult> second_reference = shaper3.Shape(&font, direction);

  HarfBuzzShaper shaper4(string.Characters16() + 11, 1);
  RefPtr<ShapeResult> third_reference = shaper4.Shape(&font, direction);

  // Width of each segment should be the same when shaped using start and end
  // offset as it is when shaping the three segments using separate shaper
  // instances.
  // A full pixel is needed for tolerance to account for kerning on some
  // platforms.
  ASSERT_NEAR(first_reference->Width(), first->Width(), 1);
  ASSERT_NEAR(second_reference->Width(), second->Width(), 1);
  ASSERT_NEAR(third_reference->Width(), third->Width(), 1);

  // Width of shape results for the entire string should match the combined
  // shape results from the three segments.
  float total_width = first->Width() + second->Width() + third->Width();
  ASSERT_NEAR(combined->Width(), total_width, 1);
}

// Represents the case where a part of a cluster has a different color.
// <div>0x647<span style="color: red;">0x64A</span></div>
// This test requires context-aware shaping which hasn't been implemented yet.
// See crbug.com/689155
TEST_F(HarfBuzzShaperTest, DISABLED_ShapeArabicWithContext) {
  UChar arabic_string[] = {0x647, 0x64A};
  HarfBuzzShaper shaper(arabic_string, 2);

  RefPtr<ShapeResult> combined = shaper.Shape(&font, TextDirection::kRtl);

  RefPtr<ShapeResult> first = shaper.Shape(&font, TextDirection::kRtl, 0, 1);
  RefPtr<ShapeResult> second = shaper.Shape(&font, TextDirection::kRtl, 1, 2);

  // Combined width should be the same when shaping the two characters
  // separately as when shaping them combined.
  ASSERT_NEAR(combined->Width(), first->Width() + second->Width(), 0.1);
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalUpright) {
  font_description.SetOrientation(FontOrientation::kVerticalUpright);
  font = Font(font_description);
  font.Update(nullptr);

  // This string should create 2 runs, ideographic and Latin, both in upright.
  String string(u"\u65E5\u65E5\u65E5lllll");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // Shape each run and merge them using CopyRange. Bounds() should match.
  RefPtr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 3);
  RefPtr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 3, string.length());

  RefPtr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 3, composite_result.Get());
  result2->CopyRange(3, string.length(), composite_result.Get());

  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
}

TEST_F(HarfBuzzShaperTest, ShapeVerticalMixed) {
  font_description.SetOrientation(FontOrientation::kVerticalMixed);
  font = Font(font_description);
  font.Update(nullptr);

  // This string should create 2 runs, ideographic in upright and Latin in
  // rotated horizontal.
  String string(u"\u65E5\u65E5\u65E5lllll");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // Shape each run and merge them using CopyRange. Bounds() should match.
  RefPtr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 3);
  RefPtr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 3, string.length());

  RefPtr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 3, composite_result.Get());
  result2->CopyRange(3, string.length(), composite_result.Get());

  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
}

TEST_F(HarfBuzzShaperTest, MissingGlyph) {
  // U+FFF0 is not assigned as of Unicode 10.0.
  String string(
      u"\uFFF0"
      u"Hello");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  EXPECT_EQ(0u, result->StartIndexForResult());
  EXPECT_EQ(string.length(), result->EndIndexForResult());
}

TEST_F(HarfBuzzShaperTest, PositionForOffsetLatin) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);
  RefPtr<ShapeResult> first = shaper.Shape(&font, direction, 0, 5);    // Hello
  RefPtr<ShapeResult> second = shaper.Shape(&font, direction, 6, 11);  // World

  EXPECT_EQ(0.0f, result->PositionForOffset(0));
  ASSERT_NEAR(first->Width(), result->PositionForOffset(5), 1);
  ASSERT_NEAR(second->Width(),
              result->PositionForOffset(11) - result->PositionForOffset(6), 1);
  ASSERT_NEAR(result->Width(), result->PositionForOffset(12), 0.1);
}

TEST_F(HarfBuzzShaperTest, PositionForOffsetArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  TextDirection direction = TextDirection::kRtl;

  HarfBuzzShaper shaper(arabic_string, 3);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  EXPECT_EQ(0.0f, result->PositionForOffset(3));
  ASSERT_NEAR(result->Width(), result->PositionForOffset(0), 0.1);
}

// A Value-Parameterized Test class to test OffsetForPosition() with
// |include_partial_glyphs| parameter.
class IncludePartialGlyphs : public HarfBuzzShaperTest,
                             public ::testing::WithParamInterface<bool> {};

INSTANTIATE_TEST_CASE_P(OffsetForPositionTest,
                        IncludePartialGlyphs,
                        ::testing::Bool());

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetLatin) {
  String string = To16Bit("Hello World!", 12);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 12);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
  EXPECT_EQ(4u, result->OffsetForPosition(result->PositionForOffset(4),
                                          include_partial_glyphs));
  EXPECT_EQ(5u, result->OffsetForPosition(result->PositionForOffset(5),
                                          include_partial_glyphs));
  EXPECT_EQ(6u, result->OffsetForPosition(result->PositionForOffset(6),
                                          include_partial_glyphs));
  EXPECT_EQ(7u, result->OffsetForPosition(result->PositionForOffset(7),
                                          include_partial_glyphs));
  EXPECT_EQ(8u, result->OffsetForPosition(result->PositionForOffset(8),
                                          include_partial_glyphs));
  EXPECT_EQ(9u, result->OffsetForPosition(result->PositionForOffset(9),
                                          include_partial_glyphs));
  EXPECT_EQ(10u, result->OffsetForPosition(result->PositionForOffset(10),
                                           include_partial_glyphs));
  EXPECT_EQ(11u, result->OffsetForPosition(result->PositionForOffset(11),
                                           include_partial_glyphs));
  EXPECT_EQ(12u, result->OffsetForPosition(result->PositionForOffset(12),
                                           include_partial_glyphs));
}

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetArabic) {
  UChar arabic_string[] = {0x628, 0x64A, 0x629};
  TextDirection direction = TextDirection::kRtl;

  HarfBuzzShaper shaper(arabic_string, 3);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
}

TEST_P(IncludePartialGlyphs, OffsetForPositionMatchesPositionForOffsetMixed) {
  UChar mixed_string[] = {0x628, 0x64A, 0x629, 0xE20, 0x65E5, 0x62};
  HarfBuzzShaper shaper(mixed_string, 6);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  bool include_partial_glyphs = GetParam();
  EXPECT_EQ(0u, result->OffsetForPosition(result->PositionForOffset(0),
                                          include_partial_glyphs));
  EXPECT_EQ(1u, result->OffsetForPosition(result->PositionForOffset(1),
                                          include_partial_glyphs));
  EXPECT_EQ(2u, result->OffsetForPosition(result->PositionForOffset(2),
                                          include_partial_glyphs));
  EXPECT_EQ(3u, result->OffsetForPosition(result->PositionForOffset(3),
                                          include_partial_glyphs));
  EXPECT_EQ(4u, result->OffsetForPosition(result->PositionForOffset(4),
                                          include_partial_glyphs));
  EXPECT_EQ(5u, result->OffsetForPosition(result->PositionForOffset(5),
                                          include_partial_glyphs));
  EXPECT_EQ(6u, result->OffsetForPosition(result->PositionForOffset(6),
                                          include_partial_glyphs));
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeIntoLatin) {
  String string = To16Bit("Testing ShapeResult::createSubRun", 33);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 33);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  RefPtr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result->CopyRange(0, 10, composite_result.Get());
  result->CopyRange(10, 20, composite_result.Get());
  result->CopyRange(20, 30, composite_result.Get());
  result->CopyRange(30, 33, composite_result.Get());

  EXPECT_EQ(result->NumCharacters(), composite_result->NumCharacters());
  EXPECT_EQ(result->SnappedWidth(), composite_result->SnappedWidth());
  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            composite_result->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(15),
            composite_result->SnappedStartPositionForOffset(15));
  EXPECT_EQ(result->SnappedStartPositionForOffset(30),
            composite_result->SnappedStartPositionForOffset(30));
  EXPECT_EQ(result->SnappedStartPositionForOffset(33),
            composite_result->SnappedStartPositionForOffset(33));
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeIntoArabicThaiHanLatin) {
  UChar mixed_string[] = {0x628, 0x20, 0x64A, 0x629, 0x20, 0xE20, 0x65E5, 0x62};
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(mixed_string, 8);
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".2" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .2);

  RefPtr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result->CopyRange(0, 4, composite_result.Get());
  result->CopyRange(4, 6, composite_result.Get());
  result->CopyRange(6, 8, composite_result.Get());

  EXPECT_EQ(result->NumCharacters(), composite_result->NumCharacters());
  EXPECT_EQ(result->SnappedWidth(), composite_result->SnappedWidth());
  EXPECT_EQ(result->Bounds(), composite_result->Bounds());
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            composite_result->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(1),
            composite_result->SnappedStartPositionForOffset(1));
  EXPECT_EQ(result->SnappedStartPositionForOffset(2),
            composite_result->SnappedStartPositionForOffset(2));
  EXPECT_EQ(result->SnappedStartPositionForOffset(3),
            composite_result->SnappedStartPositionForOffset(3));
  EXPECT_EQ(result->SnappedStartPositionForOffset(4),
            composite_result->SnappedStartPositionForOffset(4));
  EXPECT_EQ(result->SnappedStartPositionForOffset(5),
            composite_result->SnappedStartPositionForOffset(5));
  EXPECT_EQ(result->SnappedStartPositionForOffset(6),
            composite_result->SnappedStartPositionForOffset(6));
  EXPECT_EQ(result->SnappedStartPositionForOffset(7),
            composite_result->SnappedStartPositionForOffset(7));
  EXPECT_EQ(result->SnappedStartPositionForOffset(8),
            composite_result->SnappedStartPositionForOffset(8));
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeAcrossRuns) {
  // Create 3 runs:
  // [0]: 1 character.
  // [1]: 5 characters.
  // [2]: 2 character.
  String mixed_string(u"\u65E5Hello\u65E5\u65E5");
  TextDirection direction = TextDirection::kLtr;
  HarfBuzzShaper shaper(mixed_string.Characters16(), mixed_string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);

  // CopyRange(5, 7) should copy 1 character from [1] and 1 from [2].
  RefPtr<ShapeResult> target = ShapeResult::Create(&font, 0, direction);
  result->CopyRange(5, 7, target.Get());
  EXPECT_EQ(2u, target->NumCharacters());
}

TEST_F(HarfBuzzShaperTest, ShapeResultCopyRangeSegmentGlyphBoundingBox) {
  String string(u"THello worldL");
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result1 = shaper.Shape(&font, direction, 0, 6);
  RefPtr<ShapeResult> result2 =
      shaper.Shape(&font, direction, 6, string.length());

  RefPtr<ShapeResult> composite_result =
      ShapeResult::Create(&font, 0, direction);
  result1->CopyRange(0, 6, composite_result.Get());
  result2->CopyRange(6, string.length(), composite_result.Get());

  RefPtr<ShapeResult> result = shaper.Shape(&font, direction);
  EXPECT_EQ(result->Bounds(), composite_result->Bounds());

  // Check width and bounds are not too much different. ".1" is heuristic.
  EXPECT_NEAR(result->Width(), result->Bounds().Width(), result->Width() * .1);
}

TEST_F(HarfBuzzShaperTest, SafeToBreakLatinCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::testing::CreateTestFont(
      "MEgalopolis",
      blink::testing::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  String string = To16Bit("ffi ff", 6);
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);

  EXPECT_EQ(0u, result->NextSafeToBreakOffset(0));  // At start of string.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(1));  // At end of "ffi" ligature.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(2));  // At end of "ffi" ligature.
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(3));  // At end of "ffi" ligature.
  EXPECT_EQ(4u, result->NextSafeToBreakOffset(4));  // After space.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(5));  // At end of "ff" ligature.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(6));  // At end of "ff" ligature.

  // Verify safe to break information in copied results to ensure that both
  // copying and multi-run break information works.
  RefPtr<ShapeResult> copied_result =
      ShapeResult::Create(&testFont, 0, TextDirection::kLtr);
  result->CopyRange(0, 3, copied_result.Get());
  result->CopyRange(3, string.length(), copied_result.Get());

  EXPECT_EQ(0u, copied_result->NextSafeToBreakOffset(0));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(1));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(2));
  EXPECT_EQ(3u, copied_result->NextSafeToBreakOffset(3));
  EXPECT_EQ(4u, copied_result->NextSafeToBreakOffset(4));
  EXPECT_EQ(6u, copied_result->NextSafeToBreakOffset(5));
  EXPECT_EQ(6u, copied_result->NextSafeToBreakOffset(6));
}

TEST_F(HarfBuzzShaperTest, SafeToBreakPreviousLatinCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::testing::CreateTestFont(
      "MEgalopolis",
      blink::testing::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  String string = To16Bit("ffi ff", 6);
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);

  EXPECT_EQ(6u, result->PreviousSafeToBreakOffset(6));  // At end of "ff" liga.
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(5));  // At end of "ff" liga.
  EXPECT_EQ(4u, result->PreviousSafeToBreakOffset(4));  // After space.
  EXPECT_EQ(3u, result->PreviousSafeToBreakOffset(3));  // At end of "ffi" liga.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(2));  // At start of string.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(1));  // At start of string.
  EXPECT_EQ(0u, result->PreviousSafeToBreakOffset(0));  // At start of string.

  // Verify safe to break information in copied results to ensure that both
  // copying and multi-run break information works.
  RefPtr<ShapeResult> copied_result =
      ShapeResult::Create(&testFont, 0, TextDirection::kLtr);
  result->CopyRange(0, 3, copied_result.Get());
  result->CopyRange(3, string.length(), copied_result.Get());

  EXPECT_EQ(6u, copied_result->PreviousSafeToBreakOffset(6));
  EXPECT_EQ(4u, copied_result->PreviousSafeToBreakOffset(5));
  EXPECT_EQ(4u, copied_result->PreviousSafeToBreakOffset(4));
  EXPECT_EQ(3u, copied_result->PreviousSafeToBreakOffset(3));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(2));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(1));
  EXPECT_EQ(0u, copied_result->PreviousSafeToBreakOffset(0));
}

TEST_F(HarfBuzzShaperTest, SafeToBreakLatinDiscretionaryLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;
  ligatures.discretionary = FontDescription::kEnabledLigaturesState;

  // MEgalopolis Extra has a lot of ligatures which this test relies on.
  Font testFont = blink::testing::CreateTestFont(
      "MEgalopolis",
      blink::testing::PlatformTestDataPath(
          "third_party/MEgalopolis/MEgalopolisExtra.woff"),
      16, &ligatures);

  // RA and CA form ligatures, most glyph pairs have kerning.
  String string(u"ABRACADABRA");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&testFont, TextDirection::kLtr);
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(1));    // After CA ligature.
  EXPECT_EQ(6u, result->NextSafeToBreakOffset(6));    // After CA ligature.
  EXPECT_EQ(9u, result->NextSafeToBreakOffset(7));    // Before RA ligature.
  EXPECT_EQ(9u, result->NextSafeToBreakOffset(9));    // Before RA ligature.
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(10));  // At end of string.

  // Add zero-width spaces at the safe to break offsets.
  String refString(u"ABRACA\u200BDAB\u200BRA");
  HarfBuzzShaper refShaper(refString.Characters16(), refString.length());
  RefPtr<ShapeResult> referenceResult =
      refShaper.Shape(&testFont, TextDirection::kLtr);

  // Results should be identical if it truly is safe to break at the designated
  // safe-to-break offsets
  EXPECT_EQ(result->SnappedWidth(), referenceResult->SnappedWidth());
  EXPECT_EQ(result->Bounds(), referenceResult->Bounds());
  EXPECT_EQ(result->SnappedStartPositionForOffset(0),
            referenceResult->SnappedStartPositionForOffset(0));
  EXPECT_EQ(result->SnappedStartPositionForOffset(1),
            referenceResult->SnappedStartPositionForOffset(1));
  EXPECT_EQ(result->SnappedStartPositionForOffset(2),
            referenceResult->SnappedStartPositionForOffset(2));
  EXPECT_EQ(result->SnappedStartPositionForOffset(3),
            referenceResult->SnappedStartPositionForOffset(3));
  EXPECT_EQ(result->SnappedStartPositionForOffset(4),
            referenceResult->SnappedStartPositionForOffset(4));
  EXPECT_EQ(result->SnappedStartPositionForOffset(5),
            referenceResult->SnappedStartPositionForOffset(5));

  // First zero-width space is at position 6 so the the matching character in
  // the reference results is 7.
  EXPECT_EQ(result->SnappedStartPositionForOffset(6),
            referenceResult->SnappedStartPositionForOffset(7));
  EXPECT_EQ(result->SnappedStartPositionForOffset(7),
            referenceResult->SnappedStartPositionForOffset(8));
  EXPECT_EQ(result->SnappedStartPositionForOffset(8),
            referenceResult->SnappedStartPositionForOffset(9));

  // Second zero-width space is at position 9 so the the matching character in
  // the reference results is 11.
  EXPECT_EQ(result->SnappedStartPositionForOffset(9),
            referenceResult->SnappedStartPositionForOffset(11));
  EXPECT_EQ(result->SnappedStartPositionForOffset(10),
            referenceResult->SnappedStartPositionForOffset(12));
}

// TODO(layout-dev): This test fails on Mac due to AAT shaping.
TEST_F(HarfBuzzShaperTest, DISABLED_SafeToBreakArabicCommonLigatures) {
  FontDescription::VariantLigatures ligatures;
  ligatures.common = FontDescription::kEnabledLigaturesState;

  // كسر الاختبار
  String string(
      u"\u0643\u0633\u0631\u0020\u0627\u0644\u0627\u062E\u062A\u0628\u0627"
      u"\u0631");
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kRtl);

  // Safe to break at 0, 3, 4, 5, 7, and 11.
  EXPECT_EQ(0u, result->NextSafeToBreakOffset(0));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(1));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(2));
  EXPECT_EQ(3u, result->NextSafeToBreakOffset(3));
  EXPECT_EQ(4u, result->NextSafeToBreakOffset(4));
  EXPECT_EQ(5u, result->NextSafeToBreakOffset(5));
  EXPECT_EQ(7u, result->NextSafeToBreakOffset(6));
  EXPECT_EQ(7u, result->NextSafeToBreakOffset(7));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(8));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(9));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(10));
  EXPECT_EQ(11u, result->NextSafeToBreakOffset(11));
  EXPECT_EQ(12u, result->NextSafeToBreakOffset(12));
}

// TODO(layout-dev): Expand RTL test coverage and add tests for mixed
// directionality strings.

}  // namespace blink
