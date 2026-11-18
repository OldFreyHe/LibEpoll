#include "MessageCodec.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{

TEST(MessageCodecTest, RoundTripPreservesFrameFields)
{
    MessageFrame original;
    original.head = {'L', 'I', 'B', 'E', 'P', 'O', 'L', 'L'};
    original.sequence = 0x123456789ABCDEF0ULL;
    original.messageId = 0x0102;
    original.payload = {0x00, 0x01, 0x7F, 0x80, 0xFF};

    std::vector<uint8_t> encoded;
    ASSERT_TRUE(MessageCodec::encode(original, encoded));
    ASSERT_EQ(encoded.size(), MessageCodec::HEADER_LEN + original.payload.size());

    MessageFrame decoded;
    ASSERT_TRUE(MessageCodec::decodeFrame(encoded.data(), encoded.size(), decoded));
    EXPECT_EQ(decoded.head, original.head);
    EXPECT_EQ(decoded.length, encoded.size());
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.messageId, original.messageId);
    EXPECT_EQ(decoded.payload, original.payload);
}

TEST(MessageCodecTest, RejectsIncompleteFrame)
{
    MessageFrame frame;
    frame.head = {'L', 'I', 'B', 'E', 'P', 'O', 'L', 'L'};
    frame.payload = {1, 2, 3};

    std::vector<uint8_t> encoded;
    ASSERT_TRUE(MessageCodec::encode(frame, encoded));
    ASSERT_GT(encoded.size(), MessageCodec::HEADER_LEN);

    MessageFrame decoded;
    EXPECT_FALSE(MessageCodec::decodeFrame(encoded.data(), encoded.size() - 1, decoded));
}

TEST(MessageCodecTest, RejectsNullInput)
{
    MessageFrame decoded;
    EXPECT_FALSE(MessageCodec::decodeFrame(nullptr, 0, decoded));
}

} // namespace
