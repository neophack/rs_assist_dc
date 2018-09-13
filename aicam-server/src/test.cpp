/***************************************************************************
 * 
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file test.cpp
 * @author work(com@baidu.com)
 * @date 2018/04/03 20:54:59
 * @brief 
 *  
 **/

#include <string.h>
#include <stdio.h>
#include <stdint.h>

int main() {
/*
    FILE * fp = fopen("tmp.txt","r");
    fseek(fp,0L,SEEK_END);
    int flen=ftell(fp);
    char *p=(char *)malloc(flen+1);
    fseek(fp,0L,SEEK_SET);
    fread(p,flen,1,fp);
    return 0;
*/
/*    
    uint32_t tmp_len = 10000;
    char * tmp_int = (char *) &tmp_len;
    for(int i=0; i<4; i++) {
        printf("%02X",tmp_int[i]);
    }
    return 0;
*/
    FILE * fp = fopen("tmp.txt","r");
    char tmp[2634488+1];

    memset(tmp,0,2634488);
    fseek(fp,0L,SEEK_SET);
    fread(tmp,2634488,1,fp);
    int imod = 2634488+1 /10000;
    int last_idx = -1;
    FILE * fout = NULL;
    char file_name[64];
    for(int i=0; i< 27; i++) {
        uint32_t ilen =*(uint32_t *)&tmp[100000*i];
        printf("ilen:%u\n",ilen);
    }
    return 0;
    for(int i=0; i< 2634488+1; i++) {
        imod = i/100000;
        if(imod != last_idx) {
            memset(file_name, 0, sizeof(file_name));
            sprintf(file_name,"file_idx/file_idx_%d",imod);
            fout = fopen(file_name,"a+");
            last_idx = imod;
        }
        fprintf(fout,"%02X",tmp[i]);
    }
    return 0;
    return 0;
}



















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
