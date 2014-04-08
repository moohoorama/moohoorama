/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbarray.h>
#include <ywkey.h>
#include <gtest/gtest.h>

void key_test() {
    typedef  ywKey<ywBarray, ywBarray> key_type;

    int32_t  num  = 0x01;
//    int32_t  num2 = 0x02;
    Byte     buf[1024];
//    Byte     buf2[1024];

    {
        ywBarray test(
            static_cast<uint32_t>(sizeof(num)),
            reinterpret_cast<Byte*>(&num));
        key_type key(test, test);

        key.write(buf);
        ASSERT_EQ(0, key.comp(buf));
        ASSERT_EQ(10, key.get_size());
        {
            key_type key2(test, test);

            assert(key2.comp(buf) == 0);
        }
    }
}

