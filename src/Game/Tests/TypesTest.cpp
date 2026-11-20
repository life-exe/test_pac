#include <gtest/gtest.h>
#include "Types.h"

using namespace LifeExe;

TEST(GridPosTest, DefaultConstructedIsZero)
{
    const Core::GridPos pos{};
    EXPECT_EQ(pos.row, 0);
    EXPECT_EQ(pos.col, 0);
}

TEST(GridPosTest, EqualityComparesBothFields)
{
    const Core::GridPos pos{2, 3};
    EXPECT_EQ(pos, (Core::GridPos{2, 3}));

    // Positions that differ in a single field must not compare equal
    EXPECT_NE(pos, (Core::GridPos{9, 3}));
    EXPECT_NE(pos, (Core::GridPos{2, 9}));
    EXPECT_FALSE(pos == (Core::GridPos{3, 2}));
}

TEST(GridPosTest, AdditionSumsComponentwise)
{
    const Core::GridPos pos{1, 2};
    EXPECT_EQ((pos + Core::GridPos{3, -4}), (Core::GridPos{4, -2}));
    EXPECT_EQ(pos + Core::GridPos{}, pos);
}

TEST(GridPosTest, SubtractionSubtractsComponentwise)
{
    const Core::GridPos pos{5, 1};
    EXPECT_EQ((pos - Core::GridPos{2, 3}), (Core::GridPos{3, -2}));
    EXPECT_EQ(pos - pos, Core::GridPos{});
}

TEST(GridPosTest, ScalarMultiplicationScalesBothFields)
{
    const Core::GridPos pos{1, -2};
    EXPECT_EQ(3 * pos, (Core::GridPos{3, -6}));
    EXPECT_EQ(0 * pos, Core::GridPos{});
    EXPECT_EQ(-1 * pos, (Core::GridPos{-1, 2}));
}

TEST(GridPosTest, OperationsAreEvaluatedAtCompileTime)
{
    // Compilation itself is the check: these must stay usable in constant expressions
    static_assert(Core::GridPos{} == Core::GridPos{0, 0});
    static_assert(Core::GridPos{1, 2} + Core::GridPos{3, 4} == Core::GridPos{4, 6});
    static_assert(Core::GridPos{5, 5} - Core::GridPos{2, 1} == Core::GridPos{3, 4});
    static_assert(2 * Core::GridPos{3, -1} == Core::GridPos{6, -2});
    SUCCEED();
}

TEST(Vec2fTest, DefaultConstructedIsZero)
{
    const Core::Vec2f vec{};
    EXPECT_FLOAT_EQ(vec.x, 0.0f);
    EXPECT_FLOAT_EQ(vec.y, 0.0f);
}

TEST(Vec2fTest, FieldsCanBeAssigned)
{
    Core::Vec2f vec{};
    vec.x = 3.5f;
    EXPECT_FLOAT_EQ(vec.x, 3.5f);
    EXPECT_FLOAT_EQ(vec.y, 0.0f);

    vec.y = -1.25f;
    EXPECT_FLOAT_EQ(vec.x, 3.5f);
    EXPECT_FLOAT_EQ(vec.y, -1.25f);

    const Core::Vec2f designated{.x = 1.5f, .y = -2.25f};
    EXPECT_FLOAT_EQ(designated.x, 1.5f);
    EXPECT_FLOAT_EQ(designated.y, -2.25f);
}
