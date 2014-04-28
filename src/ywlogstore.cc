/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <ywlogstore.h>
#include <ywarchive.h>
#include <gtest/gtest.h>

class ywLS_writer {
 public:
    explicit ywLS_writer(ywLogStore *_ls, int32_t _chunk_idx):ls(_ls),
        chunk_idx(_chunk_idx), offset(0), done(false) {
        buffer = reinterpret_cast<Byte*>(malloc(
                ls->CHUNK_SIZE + ls->DIO_ALIGN));
        chunk = reinterpret_cast<ywChunk*>(
            align(reinterpret_cast<intptr_t>(buffer), ls->DIO_ALIGN));
    }
    void finalize() {
        ls->write_chunk(chunk, 0);
        ls->sync();
        printHex(1024, chunk->body, true);
        free(buffer);
        buffer = NULL;
        offset = 0;
    }
    ~ywLS_writer() {
        assert(!buffer);
    }

    template<typename T>
    void dump(T val, const char * title = NULL) {
        val.write(&chunk->body[offset]);
        offset += val.get_size();
    }

 private:
    Byte       *buffer;
    ywChunk    *chunk;
    ywLogStore *ls;
    int32_t     chunk_idx;
    int32_t     offset;
    bool        done;
};

void ywLogStore::init_and_read_master() {
    int32_t init_cnk;

    assert(cnk_mgr.alloc_chunk(MASTER_CNK) == 0);
    init_cnk = cnk_mgr.alloc_chunk(LOG_CNK);
    assert(init_cnk == 1);

    append_pos.set(init_cnk, 0);
    write_pos.set(init_cnk, 0);
    assert(set_chunk_idx(init_cnk));

    ywar_print  print_ar;
    cnk_mgr.dump(&print_ar);

    ywLS_writer ar(this, 0);
    cnk_mgr.dump(&ar);
}

void ywLogStore::init(const char * fn, int32_t _io) {
    uint32_t  flag = O_RDWR | O_CREAT | O_LARGEFILE;
    uint32_t  i;
    Byte     *aligned;

    chunk_buffer = reinterpret_cast<Byte*>(malloc(
            MEM_CHUNK_COUNT*CHUNK_SIZE+DIO_ALIGN));
    aligned = reinterpret_cast<Byte*>(
        align(reinterpret_cast<intptr_t>(chunk_buffer), DIO_ALIGN));

    for (i = 0; i < MEM_CHUNK_COUNT; ++i) {
        chunk_ptr[i] = reinterpret_cast<ywChunk*>(aligned + i*CHUNK_SIZE);
        chunk_idx[i] = -1;
    }

    append_pos.set_null();
    write_pos.set_null();

    if (io == DIRECT_IO) flag |= O_DIRECT;

    fd = open(fn, flag, S_IRWXU);
    if (fd == -1) {
        perror("file open error:");
        assert(fd != -1);
    }

    init_and_read_master();

    assert(create_flush_thread());
}

bool ywLogStore::reserve_space() {
    if (chunk_idx[reserve_mem_idx] == -1) {
        assert(set_chunk_idx(cnk_mgr.alloc_chunk(LOG_CNK)));
    }

    return true;
}
bool ywLogStore::flush() {
    struct    iovec iov[MEM_CHUNK_COUNT];
    int32_t   old_idx[MEM_CHUNK_COUNT];
    size_t    ret;
    size_t    offset = write_pos.get_idx() * CHUNK_SIZE;
    uint64_t  old_val = write_pos.get();
    int32_t  *chunk_idx_ptr;
    uint32_t  cnt;
    uint32_t  mem_chunk_no = write_pos.get_idx() % MEM_CHUNK_COUNT;
    uint32_t  i;

    for (i = 0, cnt = 0; i < MEM_CHUNK_COUNT; ++i, ++cnt) {
        if (write_pos.get_idx() >= append_pos.get_idx()) {
            break;
        }

        old_idx[i]      = write_pos.get_idx();
        iov[i].iov_base = chunk_ptr[mem_chunk_no+i]->body;
        iov[i].iov_len  = CHUNK_SIZE;
        if (!write_pos.next_chunk()) {
            write_pos.set(old_val);
            return false;
        }
    }

    if (!cnt) return false;

    if (io != NO_IO) {
        ret = pwritev(fd, iov, i, offset);
        sync();
        if (!(ret == cnt * CHUNK_SIZE)) {
            write_pos.set(old_val);
            perror("flush error:");
            return false;
        }
    }

    for (i = 0; i < cnt; ++i) {
        chunk_idx_ptr = &chunk_idx[(mem_chunk_no + i) % MEM_CHUNK_COUNT];
        assert(__sync_bool_compare_and_swap(
                chunk_idx_ptr, old_idx[i], old_idx[i] + MEM_CHUNK_COUNT));
    }
    return true;
}

void ywLogStore::wait_flush() {
    if (io != NO_IO) {
        while (write_pos.get_idx() <
               append_pos.get_idx()) {
            usleep(100);
        }
    }
}

bool ywLogStore::create_flush_thread() {
    pthread_attr_t          attr;
    pthread_t               pt;
    assert(0 == pthread_attr_init(&attr) );

    assert(0 == pthread_create(&pt, &attr, log_flusher, this));
    return true;
}

void *ywLogStore::log_flusher(void *arg_ptr) {
    ywLogStore *log = reinterpret_cast<ywLogStore*>(arg_ptr);

    log->running = true;
    while (!log->done) {
        if (!log->reserve_space()) {
            if (!log->flush()) {
                usleep(100);
            }
        }
    }
    log->running = false;

    return NULL;
}

class logStoreConcTest {
 public:
    static const int32_t TEST_COUNT = 16*KB;

    void run();

    ywLogStore *ls;
    int32_t     operation;
};

void logStoreConcTest::run() {
    int32_t           i;
    Byte              buf[64*KB];

    for (i = 0; i < TEST_COUNT; ++i) {
        ywBarray   test(64*KB, buf);
//        ywLong test(i);
        ls->append(&test);
    }
}

void ls_stress_task(void *arg) {
//    logStoreConcTest *tc = reinterpret_cast<logStoreConcTest*>(arg);
    auto *tc = reinterpret_cast<logStoreConcTest*>(arg);

    tc->run();
}


void logstore_basic_test(int32_t io_type, int32_t thread_count) {
    ywWorkerPool     *tpool = ywWorkerPool::get_instance();
    logStoreConcTest  tc[MAX_THREAD_COUNT];
    ywLogStore        ls("test.txt", io_type);
//    Byte              buf[64*KB];
//    size_t            i;
    int32_t           i;
    ywPerfChecker     perf;

    for (i = 0; i < thread_count; ++i) {
        tc[i].operation = i;
        tc[i].ls        = &ls;
        assert(tpool->add_task(ls_stress_task, &tc[i]));
    }
    perf.start(logStoreConcTest::TEST_COUNT*thread_count*64/1024);
    tpool->wait_to_idle();

    printf("append : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_append_pos(),
           ls.get_append_pos()/1024/1024);
    printf("flushd : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    ls.wait_flush();
    printf("flushd : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    perf.finish("MPS");
//    ls.sync();
}
