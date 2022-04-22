#include <gtest/gtest.h>
#include "ThreadPool.h"

class ThreapoolTest :
        public testing::TestWithParam<int> {
};

TEST_P(ThreapoolTest, SubmitTest){
    ThreadPool tp(4);
    std::atomic<int> value{0};
    for(size_t i = 0; i < GetParam(); ++i){
        tp.Submit([&value](){++value;});
    }
    tp.Join();
    ASSERT_EQ(value.load(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(ThreapoolTestSuit,
                         ThreapoolTest,
                         testing::Values(100, 200, 500));

TEST(JUST_WORKS, Submit_Test){
    ThreadPool tp(4);
    std::atomic<int> value{0};
    tp.Submit([&value](){++value;});
    tp.Join();
    ASSERT_EQ(value.load(), 1);
}



int main(int argc, char** args) {
    testing::InitGoogleTest(&argc, args);
    return RUN_ALL_TESTS();
}
