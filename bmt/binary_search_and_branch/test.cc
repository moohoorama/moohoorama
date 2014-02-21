/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <stdio.h>
#include <gtest/gtest.h>
#include <math.h>

#include <vector>
#include <algorithm>

static const int32_t DATA_SIZE = 1024*1024;
static int32_t       data[DATA_SIZE];

typedef bool (*compareFunc)(int32_t left, int32_t right);

bool compare(int32_t left, int32_t right) {
    return left < right;
}

TEST(BinarySearch, GenerateData) {
    std::vector<int32_t> _data(DATA_SIZE);
    int32_t i;
    int32_t rnd = 2;

    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd)) % DATA_SIZE;
        _data[i]=rnd;
    }
    _data[0]=0;
    _data[DATA_SIZE-1]=INT32_MAX;
    std::sort(_data.begin(), _data.end());
    std::copy(_data.begin(), _data.end(), data);
}

TEST(BinarySearch, BranchMethod) {
    int32_t min, mid, max;
    int32_t rnd = 2;
    int32_t i;
    compareFunc func = compare;

    for (i = 0; i < DATA_SIZE; ++i) {
        min = 0;
        max = DATA_SIZE-1;

        do {
            mid = (min + max)/2;
            if (func(rnd, data[mid])) {
                max = mid - 1;
            } else {
                min = mid + 1;
            }
        } while (min <= max);

        if (!((data[max] <= rnd) && (rnd < data[max+1]))) {
            printf("%d [%d, %d, %d]\n", i, min, mid, max);
            printf("[%d] %12d %12d %12d\n", min, data[min], rnd, data[min+1]);
            printf("[%d] %12d %12d %12d\n", mid, data[mid], rnd, data[mid+1]);
            printf("[%d] %12d %12d %12d\n", max, data[max], rnd, data[max+1]);
            assert(false);
        }
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd)) % DATA_SIZE;
    }
}

TEST(BinarySearch, NoBranchMethod) {
    int32_t i;
    int32_t j;
    int32_t log_size;
    int32_t idx;
    int32_t mid;
    int32_t size;
    int32_t sign;
    int32_t rnd = 2;
    compareFunc func = compare;

    log_size = ffsl(DATA_SIZE) - 1;
    for (i = 0; i < DATA_SIZE; ++i) {
        idx  = 0;
        size = DATA_SIZE;

        for (size = DATA_SIZE; size > 1; size /= 2) {
            mid = idx*size+size/2;
            sign = func(i+1, data[mid]);
            idx = idx*2 + 1-sign;
        }

        if (!((data[idx] <= rnd) && (rnd < data[idx+1]))) {
            printf("[%d, %d] %12d %12d %12d\n",
                   i, idx, data[idx], i+1, data[idx+1]);
            assert(false);
        }
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd)) % DATA_SIZE;
    }
}

int main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

