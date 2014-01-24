#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include <ywdlist.h>
#include <ywq.h>
#include <ywutil.h>
#include <ywserializer.h>
#include <ywmain.h>
#include <gtest/gtest.h>

TEST(Queue, Basic) {
    ywqTest();
}

TEST(Serialization, Basic) {
    EXPECT_TRUE( ywsTest() );
}

TEST(Dump, Basic) {
    char test[]="ABCDEF";
    printHex( sizeof(test), (char*)test );
    dump_stack();
}


int main( int argc, char ** argv )
{
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
