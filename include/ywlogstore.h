/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWLOGSTORE_H_
#define INCLUDE_YWLOGSTORE_H_

#include <ywcommon.h>
#include <ywsl.h>
#include <ywmempool.h>
#include <ywrcupool.h>
#include <ywkey.h>
#include <ywbarray.h>
#include <ywutil.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <ywchunkmgr.h>
#include <ywpos.h>
#include <ywbuffcache.h>

class ywChunk {
    static const size_t  CHUNK_SIZE = 16*MB;

 public:
    Byte  body[CHUNK_SIZE];
};

static_assert(sizeof(ywCnkInfo) == 4, "invalid ywCnkInfo Size");

class ywLogStore {
 public:
    static const uint32_t CHUNK_SIZE    = sizeof(ywChunk);
    static const intptr_t DIO_ALIGN     = 512;

    /* IO Type */
    static const int32_t  BUFFERED_IO = 0;
    static const int32_t  DIRECT_IO = 1;
    static const int32_t  NO_IO = 2;

    /*CnkID Type*/
    static const uint32_t MASTER_CNK = 1;
    static const uint32_t LOG_CNK    = 2;
    static const uint32_t INFO_CNK   = 3;
    static const uint32_t SORTED_CNK = 4;

    static const ywCnkID  MASTER_CNK_ID = ywChunkMgr::MASTER_CNK_ID;
    static const int32_t  MAX_PREPARE_LOG_CNK_CNT = 16;
    static const ssize_t  FLUSH_UNIT = 256*KB;
    static const uint32_t FLUSH_COUNT = 32;

    static const int64_t  VIEW_DEFAULT = INT64_MAX;

    typedef enum {
        STATUS_NONE,
        STATUS_PREACTIVE,
        STATUS_ACTIVE,
        STATUS_AFTERACTIVE,
    } Status;

    explicit ywLogStore(const char * fn, int32_t _io):fd(-1),
    io(_io), buff_cache(DIO_ALIGN, MAX_PREPARE_LOG_CNK_CNT),
    appender_view_pos(VIEW_DEFAULT), status(STATUS_NONE), flusher_enable(0) {
        init(fn, _io);
    }

    ~ywLogStore() {
        if (change_status(STATUS_ACTIVE, STATUS_AFTERACTIVE)) {
            while (load_consume(&flusher_enable)) {
                ywWaitEvent::u_sleep(100, get_pc());
            }
            if (fd != -1)   { close(fd); }
            printf("wait_count : %" PRId64 "\n", wait_count.sum());
            change_status(STATUS_AFTERACTIVE, STATUS_NONE);
        }
    }

    template<typename T>
    ywPos append(T value) {
        return append(&value);
    }

    template<typename T>
    ywPos append(T *value) {
        appender_view_pos.set(get_cnk_id(append_pos));
        ywPos     pos = append_pos.go_forward<CHUNK_SIZE>(value->get_size());
        Byte     *ptr = _get_chunk_ptr(pos);
        __sync_synchronize();
        int64_t   min = appender_view_pos.min();

        assert(min <= get_cnk_id(pos));

        value->write(ptr);
        appender_view_pos.set(VIEW_DEFAULT);
        return pos;
    }

    bool  read_chunk(ywChunk *chunk, int32_t idx) {
        ssize_t ret = pread(fd, chunk->body, CHUNK_SIZE, idx * CHUNK_SIZE);
        return ret == CHUNK_SIZE;
    }

    bool  write_chunk(ywChunk *chunk, int32_t idx) {
        ssize_t ret = pwrite(fd, chunk->body, CHUNK_SIZE, idx * CHUNK_SIZE);
        return ret == CHUNK_SIZE;
    }

    void sync() {
        fsync(fd);
    }

    void wait_flush();

    uint64_t get_append_pos() { return append_pos.get(); }
    uint64_t get_write_pos()  { return write_pos.get(); }

    bool create_master();
    bool reload_master();

 private:
    void init(const char * fn, int32_t _io);
    bool create_flush_thread();
    void reserve_space();
    bool flush();

    void read_master_chunk();
    void write_master_chunk();

    ywPos create_chunk(int32_t type) {
        ywPos      pos;
        ywCnkID    cnk_id;
        ywBCID     bcid;
        ywCnkInfo *info;
        ywChunk   *body;

        pos.set_null();

        bcid = buff_cache.alloc_cache();
        if (buff_cache.is_null(bcid)) {
            return pos;
        }

        cnk_id = cnk_mgr.alloc_chunk(type); /* unable rollback */
        if (cnk_mgr.is_null(cnk_id)) {
            assert(buff_cache.dealloc_cache(bcid));
            return pos;
        }

        info = buff_cache.get_info(bcid);
        body = buff_cache.get_body(bcid);

        *info = cnk_mgr.get_cnk_info(cnk_id);
        memset(body, 0, sizeof(*body));

        while (!buff_cache.ht_regist(bcid, cnk_id)) {
            ywWaitEvent::u_sleep(100, get_pc());
        }

        if (type == LOG_CNK) {
            ++prepare_log_cnk_cnt;
        }

        return ywPos(cnk_id * CHUNK_SIZE);
    }

    static void *log_flusher(void *arg_ptr);

    ywCnkID get_cnk_id(ywPos pos) {
        return pos.get() / CHUNK_SIZE;
    }
    int32_t get_offset(ywPos pos) {
        return pos.get() % CHUNK_SIZE;
    }

    /* lpos to chunk ptr */
    Byte *_get_chunk_ptr(ywPos pos) {
        ywCnkID   id     = get_cnk_id(pos);
        int32_t   offset = get_offset(pos);
        uint32_t  _wait_count = 0;
        ywBCID    bcid;

        bcid = buff_cache.find_cache(id);
        while (buff_cache.is_null(bcid)) {
            ++_wait_count;
            ywWaitEvent::u_sleep(10, get_pc());
            bcid = buff_cache.find_cache(id);
        }

        wait_count.mutate(_wait_count);

        ywChunk  *chunk = buff_cache.get_body(bcid);

        return &chunk->body[offset];
    }

    Status get_status() {
        return load_consume(&status);
    }

    bool change_status(Status prev_status, Status new_status) {
        return __sync_bool_compare_and_swap(&status, prev_status, new_status);
    }

    int32_t                 fd;
    int32_t                 io;

    ywPos                   append_pos;
    ywPos                   write_pos;
    int32_t                 prepare_log_cnk_cnt;

    ywChunkMgr              cnk_mgr;
    ywBuffCache<ywCnkID, ywCnkInfo, ywChunk>
                            buff_cache;

    ywAccumulator<int64_t>  wait_count;
    ywAccumulator<int64_t>  appender_view_pos;
    Status                  status;
    int32_t                 flusher_enable;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
