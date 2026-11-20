#include <gtest/gtest.h>

#include <type_traits>

#include "Types.h"

using namespace LifeExe;
using Core::GridPos;
using Core::Vec2f;

namespace
{

// The center of a cell in tiles: the cell {row, col} occupies [col, col + 1) x [row, row + 1)
constexpr Vec2f centerOf(GridPos pos)
{
    return {static_cast<float>(pos.col) + 0.5f, static_cast<float>(pos.row) + 0.5f};
}

}  // namespace

TEST(KickoffAcceptance, GridPosIsZeroInitializedByDefault)
{
    constexpr GridPos pos{};
    static_assert(pos.row == 0);
    static_assert(pos.col == 0);
    EXPECT_EQ(pos.row, 0);
    EXPECT_EQ(pos.col, 0);
}

TEST(KickoffAcceptance, GridPosFieldsAreRowThenColumn)
{
    constexpr GridPos pos{3, 7};
    EXPECT_EQ(pos.row, 3);
    EXPECT_EQ(pos.col, 7);
}

TEST(KickoffAcceptance, GridPosEqualityComparesBothFields)
{
    EXPECT_EQ((GridPos{2, 5}), (GridPos{2, 5}));
    EXPECT_NE((GridPos{2, 5}), (GridPos{5, 2}));
    EXPECT_NE((GridPos{2, 5}), (GridPos{2, 6}));
    EXPECT_NE((GridPos{2, 5}), (GridPos{3, 5}));
}

TEST(KickoffAcceptance, GridPosAdditionAddsFields)
{
    constexpr GridPos sum{GridPos{1, 2} + GridPos{3, -4}};
    static_assert(sum == GridPos{4, -2});
    EXPECT_EQ(sum, (GridPos{4, -2}));
}

TEST(KickoffAcceptance, GridPosSubtractionSubtractsFields)
{
    constexpr GridPos difference{GridPos{5, 5} - GridPos{2, 7}};
    static_assert(difference == GridPos{3, -2});
    EXPECT_EQ(difference, (GridPos{3, -2}));
}

TEST(KickoffAcceptance, GridPosScalesByAnInteger)
{
    constexpr GridPos scaled{4 * GridPos{-1, 2}};
    static_assert(scaled == GridPos{-4, 8});
    EXPECT_EQ(scaled, (GridPos{-4, 8}));
    EXPECT_EQ((0 * GridPos{9, 9}), (GridPos{0, 0}));
}

TEST(KickoffAcceptance, GridPosArithmeticIsConsistent)
{
    const GridPos a{3, 4};
    const GridPos b{-2, 6};
    EXPECT_EQ((a + b) - b, a);
    EXPECT_EQ(2 * a, a + a);
}

TEST(KickoffAcceptance, GridPosIsATrivialValueType)
{
    static_assert(std::is_trivially_copyable_v<GridPos>);
    static_assert(std::is_standard_layout_v<GridPos>);
    SUCCEED();
}

TEST(KickoffAcceptance, Vec2fIsZeroInitializedByDefault)
{
    constexpr Vec2f position{};
    EXPECT_FLOAT_EQ(position.x, 0.0f);
    EXPECT_FLOAT_EQ(position.y, 0.0f);
}

TEST(KickoffAcceptance, Vec2fXCountsColumnsAndYCountsRows)
{
    constexpr Vec2f position{2.5f, 7.5f};
    EXPECT_FLOAT_EQ(position.x, 2.5f);
    EXPECT_FLOAT_EQ(position.y, 7.5f);
}

TEST(KickoffAcceptance, CellCenterMatchesTheConvention)
{
    // Column grows to the right, row grows downwards: the center of {row 5, col 3} is x = 3.5, y = 5.5
    const Vec2f center{centerOf({5, 3})};
    EXPECT_FLOAT_EQ(center.x, 3.5f);
    EXPECT_FLOAT_EQ(center.y, 5.5f);
}
