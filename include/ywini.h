/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWINI_H_
#define INCLUDE_YWINI_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <string.h>

#include <map>
#include <string>

class ywINI{
    static const int32_t MAX_LINE = 1024;
    static const int32_t MAX_SECTION = 1024;
    static const int32_t MAX_NAME = 1024;
    static const int32_t MAX_VALUE = 1024;
    static const int32_t MAX_FN = 1024;

 public:
    explicit ywINI(const char *fn) {
        (void)load(fn);
        update_by_env();
    }

    const char *get_str(const char *key) {
        return get_str(std::string(key));
    }
    const char *get_str(std::string key) {
        return kv_map[key].data();
    }
    int32_t get_int(std::string key) {
        return atoi(get_str(key));
    }
    bool    get_bool(std::string key) {
        const char *ret = get_str(key);

        if (strcasecmp(ret, "true")) return true;
        if (strcasecmp(ret, "yes")) return true;
        if (strcasecmp(ret, "y")) return true;
        if (strcasecmp(ret, "ok")) return true;
        if (strcasecmp(ret, "false")) return false;
        return atoi(ret);
    }

    bool save(const char *fn);

    void print() {
        std::map<std::string, std::string>::iterator iter;

        for (iter = kv_map.begin(); iter != kv_map.end(); ++iter) {
            printf("%-40s %s\n",
                   iter->first.c_str(),
                   iter->second.c_str());
        }
    }

    bool equal(ywINI *dest) {
        std::map<std::string, std::string>::iterator iter;

        for (iter = kv_map.begin(); iter != kv_map.end(); ++iter) {
            const char * key = iter->first.c_str();
            const char * val1 = get_str(key);
            const char * val2 = dest->get_str(key);
            if (strcmp(val1, val2)) {
                return false;
            }
        }

        return true;
    }

 private:
    bool load(const char *fn);
    void update_by_env();

    char *search(char *ptr, const char *targets);
    bool  exist(char *ptr, const char *targets);
    char  get_token(char **ptr, const char *seperate, char *buf, size_t size);
    char *trim(char *src);

    std::map<std::string, std::string> kv_map;
};

extern void ini_test();

#endif  // INCLUDE_YWINI_H_
