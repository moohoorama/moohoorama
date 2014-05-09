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

class ywChunk {
    static const size_t  CHUNK_SIZE = 16*MB;

 public:
    Byte  body[CHUNK_SIZE];
};

static_assert(sizeof(ywCnkID) == 4, "invalid ywCnkID Size");

class ywCbuffMgr {
    static const uint32_t CBUFF_MAX  = 256;
    static const uint32_t CHUNK_SIZE = sizeof(ywChunk);

 public:
    ywCbuffMgr(int32_t _count):count(_count) {
        init();
    }
    ~ywCbuffMgr() {
        dest();
    }

    bool alloc(ywCnkID newID) {
        int32_t cbuff_id = get_cbuff_id(newID);

        if (cnk_id[cbuff_id].is_null()) {
            return true;
        }

        cnk_id[cbuff_id] = newID;

        return true;
    }
    bool free(ywCnkID id) {
        int32_t cbuff_id = get_cbuff_id(newID);

        if (cnk_id[cbuff_id] == id) {
            cnk_id[cbuff_id].set_null();
            return true;
        }

        return false;
    }

    ywChunk *get_chunk(ywCnkID id) {
        int32_t cbuff_id = get_cbuff_id(id);
        if (cnk_id[cbuff_id] == id) {
            return chunk[cbuff_id];
        }

        return NULL;
    }

 private:
    void init();
    void dest();

    int32_t get_cbuff_id(ywCnkID id) {
        return id.idx % count;
    }

    int32_t                 count;
    Byte                   *buffer;
    ywChunk                *chunk[CBUFF_MAX];
    ywCnkID                 cnk_id[CBUFF_MAX];
};

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

 public:
    explicit ywLogStore(const char * fn, int32_t _io):fd(-1),
    io(_io), done(false), running(false) {
        init(fn, _io);
    }

    ~ywLogStore() {
        done = true;
        while (running) { usleep(100); }
        if (fd != -1)   { close(fd); }
        printf("wait_count : %" PRId64 "\n", wait_count.get());
    }

    template<typename T>
    auto append(T value) {
        return append(&value);
    }

    template<typename T>
    ywPos append(T *value) {
        ywPos     pos = append_pos.alloc<CHUNK_SIZE>(value->get_size());
        Byte     *ptr = get_chunk_ptr(pos);

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

    bool reserve_space();
    bool flush();
    void wait_flush();

    bool read(ssize_t offset, int32_t size, Byte * buf) {
        return (pread(fd, buf, size, offset) == offset);
    }

    uint64_t get_append_pos() { return append_pos.get(); }
    uint64_t get_write_pos()  { return write_pos.get(); }

 private:
    void init(const char * fn, int32_t _io);
    bool create_flush_thread();
    void init_and_read_master();

    ywPos create_chunk(int32_t type) {
        ywCnkID cnk_id = cnk_mgr.alloc_chunk(type);
        while(!cbuff_mgr.alloc(cnk_id));
        return ywPos(cnk_id.idx * CHUNK_SIZE);
    }

    static void *log_flusher(void *arg_ptr);

    /* lpos to chunk ptr */
    Byte *get_chunk_ptr(ywPos pos) {
        int32_t   idx    = pos / CHUNK_SIZE;
        int32_t   offset = pos % CHUNK_SIZE;
        uint32_t  _wait_count = 0;
        ywCnkID   id = cnk_mgr.get_cnk_id(idx);
        ywChunk  *cnk;

        cnk = cbuff_mgr.get_chunk(id);
        while (cnk == NULL) {
            ++_wait_count;
            cnk = cbuff_mgr.get_chunk(id);
        }
        wait_count.mutate(_wait_count);

        return &cnk->body[offset];
    }

    int32_t                 fd;
    int32_t                 io;

    ywPos                   append_pos;
    ywPos                   write_pos;
    int32_t                 reserve_mem_idx;

    ywCbuffMgr              cbuff_mgr;
    ywChunkMgr              cnk_mgr;

    ywAccumulator<int64_t>  wait_count;
    bool                    done;
    bool                    running;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
