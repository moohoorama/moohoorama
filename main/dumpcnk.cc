/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <stdio.h>
#include <assert.h>

#include <ywcommon.h>
#include <ywutil.h>
#include <ywsymbol.h>

#include <ywlogstore.h>
#include <ywchunkmgr.h>

ywSymbol *get_default_symbol() {
    static ywSymbol default_symbol("dumpcnk.map");
    return &default_symbol;
}

void help()
{
    printf("Usage: ./testopt [OPTION] [FILE]\n"
           "  -h                도움말\n"
           "  -f [FILE]         파일출력\n"
           "  -v                버전출력\n");
}

void version()
{
    printf("Version : 1.01\n");
}

int main(int argc, char ** argv) {
    int     opt;
    int     opt_ok = 0;
    ywCnkID cnk_id = 0;
    char    file_name[16];

    ywuGlobalInit();

    while ((opt = getopt(argc, argv, "c:h")) != -1)  {
        switch(opt) 
        { 
            case 'c':
                cnk_id = atoi(optarg);
                break;
            case 'h':
                help(); 
                break; 
            case 'v':
                version();  
                break;
            case 'f':
                memcpy(file_name, optarg, 16);
                opt_ok = 1;
                break;
        }
    }

    if (cnk_id == 0) {
        ywLogStore  ls("test.txt", 1);

        ls.reload_master();
    }
    if (opt_ok != 1)
    { 
        help();
    } 

    return 0;
}
