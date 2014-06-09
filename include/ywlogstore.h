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

    static const ywCnkID  MASTER_CNK_ID = 0;
    static const int32_t  MAX_PREPARE_LOG_CNK_CNT = 16;
    static const ssize_t  FLUSH_UNIT = 256*KB;
    static const uint32_t FLUSH_COUNT = 32;

 public:
    explicit ywLogStore(const char * fn, int32_t _io):fd(-1),
    io(_io), buff_cache(DIO_ALIGN, MAX_PREPARE_LOG_CNK_CNT),
    done(false), running(false) {
        init(fn, _io);
    }

    ~ywLogStore() {
        done = true;
        while (running) { usleep(100); }
        if (fd != -1)   { close(fd); }
        printf("wait_count : %" PRId64 "\n", wait_count.sum());
    }

    template<typename T>
    ywPos append(T value) {
        return append(&value);
    }

    template<typename T>
    ywPos append(T *value) {
        ywPos     pos = append_pos.go_forward<CHUNK_SIZE>(value->get_size());
        Byte     *ptr = _get_chunk_ptr(pos);

        value->write(ptr);

        return pos;
    }

    bool  write_chunk(ywChunk *chunk, int32_t idx) {
        ssize_t ret = pwrite(fd, chunk->body, CHUNK_SIZE, idx * CHUNK_SIZE);
        return ret == CHUNK_SIZE;
    }

    void sync() {
        fsync(fd);
    }

    void wait_flush();

    bool read(ssize_t offset, int32_t size, Byte * buf) {
        return (pread(fd, buf, size, offset) == offset);
    }

    uint64_t get_append_pos() { return append_pos.get(); }
    uint64_t get_write_pos()  { return write_pos.get(); }

 private:
    void init(const char * fn, int32_t _io);
    void init_and_read_master();
    bool create_flush_thread();
    void reserve_space();
    void write_master_chunk();
    bool flush();

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

        while (!buff_cache.ht_regist(bcid, cnk_id)) { }

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

        bcid = buff_cache.ht_find(id);
        while (buff_cache.is_null(bcid)) {
            ++_wait_count;
            usleep(1);
            bcid = buff_cache.ht_find(id);
        }

        wait_count.mutate(_wait_count);

        ywChunk  *chunk = buff_cache.get_body(bcid);

        return &chunk->body[offset];
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
    bool                    done;
    bool                    running;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
