/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywstack.h>
#include <gtest/gtest.h>

void stack_basic_test() {
    ywStack<int32_t> int_stack;
    int32_t          val;

    ASSERT_FALSE(int_stack.get(-1, &val));
    ASSERT_FALSE(int_stack.get(0,  &val));
    ASSERT_FALSE(int_stack.get(1,  &val));
    ASSERT_FALSE(int_stack.get(2,  &val));

    int_stack.push(3);
    int_stack.push(2);
    int_stack.push(1);

    ASSERT_EQ(+3, int_stack.get_count());

    ASSERT_FALSE(int_stack.get(3, &val));
    ASSERT_TRUE(int_stack.get(2,  &val)); ASSERT_EQ(val, 1);
    ASSERT_TRUE(int_stack.get(1,  &val)); ASSERT_EQ(val, 2);
    ASSERT_TRUE(int_stack.get(0,  &val)); ASSERT_EQ(val, 3);
    ASSERT_FALSE(int_stack.get(-1, &val));

    ASSERT_EQ(-1, int_stack.find(4));
    ASSERT_EQ(-1, int_stack.find(-1));
    ASSERT_EQ(-1, int_stack.find(0));

    ASSERT_EQ(2, int_stack.find(1));
    ASSERT_EQ(1, int_stack.find(2));
    ASSERT_EQ(0, int_stack.find(3));

    ASSERT_TRUE(int_stack.pop(&val)); ASSERT_EQ(val, 1);
    ASSERT_TRUE(int_stack.pop(&val)); ASSERT_EQ(val, 2);
    ASSERT_TRUE(int_stack.pop(&val)); ASSERT_EQ(val, 3);
    ASSERT_FALSE(int_stack.pop(&val));
    ASSERT_FALSE(int_stack.pop(&val));
    ASSERT_FALSE(int_stack.pop(&val));
    ASSERT_EQ(0, int_stack.get_count());

    int_stack.push(1);
    ASSERT_EQ(+1, int_stack.get_count());
    int_stack.clear();
    ASSERT_FALSE(int_stack.pop(&val));
    ASSERT_EQ(+0, int_stack.get_count());
}
