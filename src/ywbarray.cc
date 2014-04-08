/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbarray.h>
#include <gtest/gtest.h>

void barray_test() {
    int32_t  num  = 0x01;
    int32_t  num2 = 0x02;
    Byte     buf[1024];
    Byte     buf2[1024];

    {
        ywBarray test(
            static_cast<uint32_t>(sizeof(num)),
            reinterpret_cast<Byte*>(&num));

        test.write(buf);
        ASSERT_EQ(0, test.comp(buf));
        ASSERT_EQ(5, test.get_size());
        {
            ywBarray test2(buf);

            assert(test2.comp(buf) == 0);
        }
    }

    {
        ywBarray test(
            static_cast<uint32_t>(sizeof(num)),
            reinterpret_cast<Byte*>(&num));
        ywBarray test2(
            static_cast<uint32_t>(sizeof(num2)),
            reinterpret_cast<Byte*>(&num2));

        test.write(buf);
        test2.write(buf2);

        ASSERT_EQ(0, test.comp(buf));
        ASSERT_EQ(1, test2.comp(buf));

        ASSERT_EQ(-1, test.comp(buf2));
        ASSERT_EQ(0,  test2.comp(buf2));

        ASSERT_EQ(5, test.get_size());
        ASSERT_EQ(5, test2.get_size());
    }


    {
        ywBarray test(249, buf2);
        test.write(buf);
        ASSERT_EQ(0, test.comp(buf));
        ASSERT_EQ(250, test.get_size());
        {
            ywBarray test2(buf);

            assert(test2.comp(buf) == 0);
        }
    }

    {
        ywBarray test(250, buf2);
        test.write(buf);
        ASSERT_EQ(0, test.comp(buf));
        ASSERT_EQ(255, test.get_size());
        {
            ywBarray test2(buf);

            assert(test2.comp(buf) == 0);
        }
    }

    {
        ywBarray test(251, buf2);
        test.write(buf);
        ASSERT_EQ(0, test.comp(buf));
        ASSERT_EQ(256, test.get_size());
        {
            ywBarray test2(buf);

            assert(test2.comp(buf) == 0);
        }
    }
}
