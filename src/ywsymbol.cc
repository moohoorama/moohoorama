/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywsymbol.h>
#include <gtest/gtest.h>

const char ywSymbol::EMPTY_STRING;

void symbol_basic_test() {
    {
        ywSymbol symbol;
        char     line_example[][32]={
            "000AAAA T TEST_A",
            "000BBBB T TEST_B",
            "000CCCC T TEST_C"};
        intptr_t i;

        symbol.readline(line_example[0]);
        symbol.readline(line_example[1]);
        symbol.readline(line_example[2]);

        for (i = 0xaaaa; i <  0xbbbb; ++i) {
            assert(strcmp(symbol.find(i), "TEST_A") == 0);
        }
        for (i = 0xbbbb; i <  0xcccc; ++i) {
            assert(strcmp(symbol.find(i), "TEST_B") == 0);
        }
        assert(symbol.get_count() == 3);
    }

    {
        ywSymbol symbol;
        intptr_t addr;

        symbol.load("ywtest.map", false);
        addr = reinterpret_cast<intptr_t>(symbol_basic_test);
        assert(memcmp(symbol.find(addr), __func__, strlen(__func__)) == 0);
    }
}
