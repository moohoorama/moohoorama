
/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcupool.h>
#include <gtest/gtest.h>

class test_type {
    ywDList  list;
    int32_t  val;
    char     pad[1024];
};


void rcu_pool_test() {
    ywRcuPool<test_type> pool;
    typedef ywRcuPoolGuard<test_type> guard_type;
    test_type *buf[32];
    int32_t    i;

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            assert(guard.free(buf[i]));
        }
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        guard.rollback();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        assert(guard.alloc() == NULL);
        guard.rollback();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        assert(guard.alloc() == NULL);
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            assert(guard.free(buf[i]));
        }
        guard.commit();
    }

    pool.report();
    pool.aging();
    pool.reclaim_to_pool();
    pool.report();

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            assert(guard.free(buf[i]));
        }
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        guard.rollback();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        assert(guard.alloc() == NULL);
        guard.rollback();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            buf[i] = guard.alloc();
        }
        assert(guard.alloc() == NULL);
        guard.commit();
    }

    {
        guard_type guard(&pool);

        for (i = 0 ; i < 32; ++i) {
            assert(guard.free(buf[i]));
        }
        guard.commit();
    }

    pool.report();
    pool.aging();
    pool.reclaim_to_pool();
    pool.report();
}
