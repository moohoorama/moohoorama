/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywini.h>
#include <stdlib.h>
#include <gtest/gtest.h>

bool ywINI::exist(char *ptr, const char *targets) {
    while (*targets != '\0') {
        if (*ptr == *targets) {
            return true;
        }
        ++targets;
    }

    return false;
}

char *ywINI::search(char *ptr, const char *targets) {
    while (*ptr && exist(ptr, targets) == false) {
        ++ptr;
    }
    return ptr;
}

char ywINI::get_token(char **ptr, const char *sep, char *buf, size_t size) {
    char *end = search(*ptr, sep);
    char  org = *end;

    *end = '\0';
    strncpy(buf, *ptr, size);
    *ptr = end + 1;
    return org;
}

char *ywINI::trim(char *src) {
    char *start = src;
    char *end   = src + strlen(src) - 1;

    while (*start && isspace(*start)) {
        ++start;
    }

    while (*end && isspace(*end)) {
        *end = '\0';
        --end;
    }

    return start;
}

bool ywINI::load(const char *fn) {
    FILE *fp;
    char  line[MAX_LINE] = "";
    char  section[MAX_SECTION] = "";
    char  name[MAX_NAME] = "";
    char  value[MAX_VALUE] = "";
    char *ptr;
    char  org;


    if (!(fp = fopen(fn, "r"))) {
        return false;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        ptr = line;

        switch (ptr[0]) {
        case ';':
        case '#':
            break;
        case '[':   /* section */
            ++ptr; /* skip '[' */
            (void)get_token(&ptr, "];#:=", section, sizeof(section));
            break;
        default:
            org = get_token(&ptr, ":=;#", name, sizeof(name));

            if (exist(&org, ":= ")) {
                org = get_token(&ptr, ";#", value, sizeof(value));

                if (strlen(trim(name))) {
                    std::string _name =
                        std::string(trim(section))
                        + std::string(".")
                        + std::string(trim(name));
                    kv_map[_name] = trim(value);
                }
            }
            break;
        }
    }

    fclose(fp);

    return true;
}

bool ywINI::save(const char *fn) {
    FILE *fp;
    char  section[MAX_SECTION] = "";
    char  prev_section[MAX_SECTION] = "";
    char  key[MAX_NAME] = "";
    char  name[MAX_NAME] = "";
    char  value[MAX_VALUE] = "";
    char *ptr;
    std::map<std::string, std::string>::iterator iter;

    if (!(fp = fopen(fn, "w"))) {
        return false;
    }

    for (iter = kv_map.begin(); iter != kv_map.end(); ++iter) {
        strncpy(key, iter->first.c_str(), sizeof(key));
        strncpy(value, iter->second.c_str(), sizeof(value));
        ptr = key;
        get_token(&ptr, ".", section, sizeof(section));
        strncpy(name, ptr, sizeof(key));
        if (strncmp(prev_section, section, sizeof(section))) {
            strncpy(prev_section, section, sizeof(section));
            fprintf(fp, "\n[%s]\n", section);
        }
        fprintf(fp, "%-40s= %s\n",
                name,
                value);
    }

    fclose(fp);

    return true;
}

void ywINI::update_by_env() {
    std::map<std::string, std::string>::iterator iter;
    char * ret;

    for (iter = kv_map.begin(); iter != kv_map.end(); ++iter) {
        if (NULL != (ret = getenv(iter->first.c_str()))) {
            kv_map[iter->first] = ret;
        }
    }
}

void ini_test() {
    ywINI test_ini("test.ini");

    test_ini.save("ret.ini");
    ywINI copy_ini("ret.ini");
    ASSERT_TRUE(test_ini.equal(&copy_ini));

    unlink("ret.ini");
    ywINI rm_ini("ret.ini");
    ASSERT_FALSE(test_ini.equal(&rm_ini));

    ywINI bin_test("ywtest");

    ASSERT_EQ(0, test_ini.get_int("tttt"));
}
