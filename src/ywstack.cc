/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywstack.h>
#include <gtest/gtest.h>

void stack_basic_test() {
    ywStack<int32_t, -1> int_stack;

    ASSERT_EQ(-1, int_stack.get(-1));
    ASSERT_EQ(-1, int_stack.get(0));
    ASSERT_EQ(-1, int_stack.get(1));
    ASSERT_EQ(-1, int_stack.get(1000000));

    int_stack.push(3);
    int_stack.push(2);
    int_stack.push(1);

    ASSERT_EQ(+3, int_stack.get_count());

    ASSERT_EQ(-1, int_stack.get(3));
    ASSERT_EQ(+1, int_stack.get(2));
    ASSERT_EQ(+2, int_stack.get(1));
    ASSERT_EQ(+3, int_stack.get(0));
    ASSERT_EQ(-1, int_stack.get(-1));

    ASSERT_EQ(-1, int_stack.find(4));
    ASSERT_EQ(-1, int_stack.find(-1));
    ASSERT_EQ(-1, int_stack.find(0));

    ASSERT_EQ(2, int_stack.find(1));
    ASSERT_EQ(1, int_stack.find(2));
    ASSERT_EQ(0, int_stack.find(3));

    ASSERT_EQ(+1, int_stack.pop());
    ASSERT_EQ(+2, int_stack.pop());
    ASSERT_EQ(+3, int_stack.pop());
    ASSERT_EQ(-1, int_stack.pop());
    ASSERT_EQ(-1, int_stack.pop());
    ASSERT_EQ(-1, int_stack.pop());
    ASSERT_EQ(+0, int_stack.get_count());

    int_stack.push(1);
    ASSERT_EQ(+1, int_stack.get_count());
    int_stack.clear();
    ASSERT_EQ(-1, int_stack.pop());
    ASSERT_EQ(+0, int_stack.get_count());
}
