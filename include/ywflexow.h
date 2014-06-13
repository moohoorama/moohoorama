/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWFLEXOW_H_
#define INCLUDE_YWFLEXOW_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <ywtype.h>

typedef int16_t MCVersion;
typedef int8_t  MCColID;

#define COLUMN_MAX 128

/* MC multi_column */
typedef struct {
    ssize_t     column_count;
    ssize_t     key_count;
    ssize_t     fix_count;
    ssize_t     fix_size;
    MCVersion   cre_ver[COLUMN_MAX];
    TestModule *modules[COLUMN_MAX];
} ywtMCInfo;

extern const TypeModule MC_module;

class ywtMCMeta {
    static const MCVersion VER_MAX = 16;
 public:
    bool add_column() { return false; }
    bool modify_column() { return false; }
    bool drop_column() { return false; }

    ywtMCInfo *get_info(MCVersion ver) {
        return &_infos[ver];
    }
    ywtMCInfo *get_latest_info() {
        return &_infos[_ver];
    }

 private:
    MCVersion   _ver;
    int32_t     _info_count;
    ywtMCInfo   _infos[VER_MAX];
};

#endif  // INCLUDE_YWFLEXOW_H_
