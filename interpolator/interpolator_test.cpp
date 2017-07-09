#include <gtest/gtest.h>

#include <iostream>
#include <limits>

#include "interpolator.hpp"

class my_interpolator : public interpolator<double, double>
{
public:
    virtual bool isValid(const double & y) const
    {
        return y != invalidValue();
    }

    virtual double invalidValue() const
    {
        return std::numeric_limits<double>::infinity();
    }
};

TEST(Interpolator, Initialize)
{
    my_interpolator intrp;

    ASSERT_TRUE(intrp.empty());
    ASSERT_EQ(0, intrp.size());
    ASSERT_EQ(0, intrp.valid_count());

    intrp.add(0.0, 1.0);
    intrp.add(1.0, 2.0);

    ASSERT_FALSE(intrp.empty());
    ASSERT_EQ(2, intrp.size());
    ASSERT_EQ(2, intrp.valid_count());
}

TEST(Interpolator, GetSimpleCase)
{
    my_interpolator intrp;

    intrp.add(0.0, 1.0);
    intrp.add(1.0, 2.0);

    // get valid points
    ASSERT_EQ(1.0, intrp.get(0.0).second);
    ASSERT_EQ(2.0, intrp.get(1.0).second);

    // get interpolated value
    ASSERT_EQ(1.1, intrp.get(0.1).second);
    ASSERT_EQ(1.5, intrp.get(0.5).second);
    ASSERT_EQ(1.95, intrp.get(0.95).second);
}

TEST(Interpolator, GetEdgeCase)
{
    my_interpolator intrp;

    // empty interpolator should return invalid pt for all requests
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.1).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.5).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.95).second);

    // still, everything should be invalid
    intrp.add(0.0, 1.0);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.1).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.5).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(0.95).second);

    // requesting points outside of the range
    intrp.add(1.0, 2.0);
    ASSERT_EQ(1.5, intrp.get(0.5).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(-1.0).second);
    ASSERT_EQ(intrp.invalidValue(), intrp.get(3.0).second);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
