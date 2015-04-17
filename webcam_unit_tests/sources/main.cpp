
#include <iostream>
#include "gtest/gtest.h"


using namespace std;


TEST(FirstCase, FirstTest) {
    EXPECT_EQ(2, 2);
    ASSERT_EQ(1, 10);
}

TEST(FirstCase, SecondTest) {
    EXPECT_EQ(2, 2);
    ASSERT_EQ(1, 1);
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
