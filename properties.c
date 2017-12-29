//
//  Properties.c
//  读写配置文件
//
//  Created by Hale on 17-4-24.
//  Copyright (c) 2017年 Hale. All rights reserved.
//
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "properties.h"
 
#define KEY_SIZE        128 // key缓冲区大小
#define VALUE_SIZE      128 // value缓冲区大小
 
#define LINE_BUF_SIZE   256 // 读取配置文件中每一行的缓冲区大小
 
typedef struct Properties {
    char *key;
    char *value;
    struct Properties *pNext;
}Properties;
 
typedef struct PROPS_HANDLE {
    Properties *pHead;  // 属性链表头节点
    char *filepath;     // 属性文件路径
}PROPS_HANDLE;
 
static int createPropsNode(Properties **props);         // 创建一个节点
static int trimeSpace(const char *src,char *dest);      // 去空格
static int saveConfig(const char *filepath,Properties *head);   // 将修改或保存后的配置项保存到文件
 
// 初始化环境，成功返回0，失败返回非0值
int prop_init(const char *filepath,void **handle)
{
    int ret = 0;
    FILE *fp = NULL;
    Properties *pHead = NULL,*pCurrent = NULL, *pMalloc = NULL;
    PROPS_HANDLE *ph = NULL;
    char line[LINE_BUF_SIZE];               // 存放读取每一行的缓冲区
    char keybuff[KEY_SIZE] = { 0 };         // 存放key的缓冲区
    char valuebuff[VALUE_SIZE] = { 0 };     // 存放value的缓冲区
    char *pLine = NULL;                     // 每行缓冲区数据的指针
     
    if(filepath == NULL || handle == NULL)
    {
        ret = -1;
        printf("fun init error:%d from (filepath == NULL || handler == NULL)\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)malloc(sizeof(PROPS_HANDLE));
    if (ph == NULL) {
        ret = -2;
        printf("fun init malloc handle error:%d",ret);
        return ret;
    }
    memset(ph, 0, sizeof(PROPS_HANDLE));
     
    // 打开文件
    fp = fopen(filepath, "r");
    if (!fp) {
        ret = -3;
        printf("fun init open file error:%d from %s\n",ret,filepath);
        return ret;
    }
     
    // 创建头节点
    ret = createPropsNode(&pHead);
    if (ret != 0) {
        fclose(fp);  // 关闭文件
        printf("fun init create head node error:%d\n",ret);
        return ret;
    }
    memset(pHead, 0, sizeof(Properties));
     
    // 保存链表头节点和文件路径到handle中
    ph->pHead = pHead;
    ph->filepath = (char *)malloc(strlen(filepath) + 1);
    strcpy(ph->filepath, filepath);
     
    pCurrent = pHead;
 
    // 读取配置文件中的所有数据
    while (!feof(fp)) {
        if(fgets(line, LINE_BUF_SIZE, fp) == NULL)
        {
            break;
        }
         
        // 找等号
        if ((pLine = strstr(line, "=")) == NULL) {   // 没有等号，继续读取下一行
            continue;
        }
         
        // 循环创建节点
        ret = createPropsNode(&pMalloc);
        if (ret != 0) {
            fclose(fp);  // 关闭文件
            prop_release((void **)&ph);  // 创建节点失败，释放所有资源
            printf("create new node error:%d\n",ret);
            return ret;
        }
 
        // 设置Key
        memcpy(keybuff, line, pLine-line);
        trimeSpace(keybuff, pMalloc->key);    // 将keybuff去空格后放到pMallock.key中
     
        // 设置Value
        pLine += 1;
        trimeSpace(pLine, valuebuff);
        strcpy(pMalloc->value, valuebuff);
         
        // 将新节点入链表
        pMalloc->pNext = NULL;
        pCurrent->pNext = pMalloc;
        pCurrent = pMalloc; // 当前节点下移
         
        // 重置key,value
        memset(keybuff, 0, KEY_SIZE);
        memset(valuebuff, 0, VALUE_SIZE);
    }
     
    // 设置环境句柄给调用者
    *handle = ph;
     
    // 关闭文件
    fclose(fp);
     
    return ret;
}
 
// 获取属性数量，成功返回0，失败返回非0值
int prop_getCount(void *handle, int *count)
{
    int ret = 0,cn = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    if (handle == NULL || count == NULL) {
        ret = -1;
        printf("fun getCount error:%d from (handle == NULL || count == NULL)\n",ret);
        return ret;
    }
    ph = (PROPS_HANDLE *)handle;
    pCurrent = ph->pHead->pNext;
    while (pCurrent != NULL) {
        cn++;
        pCurrent = pCurrent->pNext;
    }
     
    *count = cn;
     
    return ret;
}
 
// 根据KEY获取值，找到返回0，如果未找到返回非0值
int prop_getValue(void *handle, const char *key, char *value)
{
    int ret = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    if (handle == NULL || key == NULL || value == NULL) {
        ret = -1;
        printf("getValue error:%d from (handle == NULL || key == NULL || value == NULL)\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)handle;
    pCurrent = ph->pHead->pNext;
    while (pCurrent != NULL) {
        if (strcmp(pCurrent->key,key) == 0) {
            break;
        }
        pCurrent = pCurrent->pNext;
    }
     
    if (pCurrent == NULL) {
        ret = -2;
        printf("fun getValue warning: not found the key:%s\n",key);
        return ret;
    }
    
    char * tempValue  = (char *)malloc(strlen(pCurrent->value)+1);
    memset(tempValue, 0, strlen(pCurrent->value)+1);
    trimeSpace(pCurrent->value, tempValue);

    //strcpy(value,pCurrent->value);
    strcpy(value, tempValue);
    free(tempValue);
     
    return ret;
}
 
// 修改key对应的属性值，修改成功返回0，失败返回非0值
int prop_setValue(void *handle, const char *key, const char *value)
{
    int ret = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    if (handle == NULL || key == NULL || value == NULL) {
        ret = -1;
        printf("fun setValue error:%d from (handle == NULL || key == NULL || value == NULL)\n",ret);
        return ret;
    }
     
    // 获得环境句柄
    ph = (PROPS_HANDLE *)handle;
     
    // 从环境句柄中获取头节点
    pCurrent = ph->pHead->pNext;
    while (pCurrent != NULL) {
        if (strcmp(pCurrent->key, key) == 0) {  // 找到
            break;
        }
        pCurrent = pCurrent->pNext;
    }
     
    if (pCurrent == NULL) { // 未找到key
        ret = -2;
        printf("fun setValue warning: not found the key:%s\n",key);
        return ret;
    }
     
    // 修改key的value
    strcpy(pCurrent->value, value);
    if (strchr(value, '\n') == NULL) {  // 加一个换行符
        strcat(pCurrent->value, "\n");
    }
     
    // 将修改的配置项写入到文件
    ret = saveConfig(ph->filepath, ph->pHead);
   
    return ret;
}
 
// 添加一个属性，添加成功返回0，失败返回非0值
int prop_add(void *handle, const char *key, const char *value)
{
    int ret = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    if (handle == NULL || key == NULL || value == NULL) {
        ret = -1;
        printf("fun add error:%d from (handle == NULL || key == NULL || value == NULL)\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)handle;
     
    //-----------如果key存在链表中，则直接修改，否则添加到链表中-----------//
    pCurrent = ph->pHead;
    while (pCurrent->pNext != NULL) {
        if (strcmp(pCurrent->pNext->key,key) == 0) {
            break;
        }
        pCurrent = pCurrent->pNext;
    }
     
    if (pCurrent->pNext != NULL) {
        return prop_setValue(handle, key, value);
    }
     
    //-----------key不存在，创建一个新的配置项，添加到链表中-----------//
    Properties *pMalloc;
    ret = createPropsNode(&pMalloc);
    if (ret != 0) {
        printf("fun add error:%d from malloc new node.",ret);
        return ret;
    }
     
    strcpy(pMalloc->key, key);
    if (strchr(pCurrent->value,'\n') == NULL) {
        strcat(pCurrent->value, "\n");
    }
    strcpy(pMalloc->value, value);
    if (strchr(value, '\n') == NULL) {  // 加一个换行符
        strcat(pMalloc->value, "\n");
    }
    pCurrent->pNext = pMalloc;  // 新配置项入链表
     
    // 将新配置项写入到文件
    ret = saveConfig(ph->filepath, ph->pHead);
     
    return ret;
}
 
// 删除一个属性，删除成功返回0，失败返回非0值
int prop_del(void *handle, const char *key)
{
    int ret = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL, *pPrev = NULL;
    if (handle == NULL || key == NULL) {
        ret = -1;
        printf("fun del error:%d from (handle == NULL || key == NULL)\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)handle;
    pPrev = ph->pHead;
    pCurrent = ph->pHead->pNext;
     
    while (pCurrent != NULL) {
        if (strcmp(pCurrent->key, key) == 0) {
            break;
        }
        pPrev = pCurrent;           // 上一个节点下移
        pCurrent = pCurrent->pNext; // 当前节点下移
    }
     
    if (pCurrent == NULL) { // 没有找到
        ret = -2;
        printf("fun del warning:not found the key:%s\n",key);
        return  ret;
    }
     
    pPrev->pNext = pCurrent->pNext; // 从链表中删除
    free(pCurrent); // 释放内存
    pCurrent = NULL;
     
    // 保存到文件
    ret = saveConfig(ph->filepath, ph->pHead);
     
    return ret;
}
 
// 获取属性文件中所有的key，获取成功返回0，失败返回非0值
int prop_getKeys(void *handle, char ***keys, int *keyscount)
{
    int ret = 0, count = 0, index = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    char **pKeys = NULL;
    if (handle == NULL || keys == NULL || keyscount == NULL) {
        ret = -1;
        printf("fun getKeys error:%d from (handle == NULL || keys == NULL || keyscount == NULL) \n",ret);
        return ret;
    }
     
    // 获取配置项数量
    ret = prop_getCount(handle, &count);
    if (ret != 0) {
        printf("fun getKeys error:%d from getCount \n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)handle;
    pCurrent = ph->pHead->pNext;
     
    // 根据链表长度，申请内存空间
    pKeys = (char **)malloc(sizeof(char *) * count);
    if (pKeys == NULL) {
        ret = -2;
        printf("fun getKeys error:%d from malloc keys\n",ret);
        return ret;
    }
     
    pCurrent = ph->pHead->pNext;
    while (pCurrent != NULL) {
        pKeys[index] = pCurrent->key;
        pCurrent = pCurrent->pNext;
        index++;
    }
     
    *keys = pKeys;
    *keyscount = count;
     
    return ret;
}
 
// 释放所有key的内存空间，成功返回0，失败返回非0值
int prop_free_keys(char ***keys,int *keyscount)
{
    int ret = 0;
    if (keys == NULL || keyscount == NULL) {
        ret = -1;
        printf("fun free_keys error:%d from (keys == NULL || keyscount == NULL) \n",ret);
        return ret;
    }
     
    free(*keys);
    *keys = NULL;
    *keyscount = 0;
     
    return ret;
}
 
// 获取属性文件中所有的值，成功返回0，失败返回非0值
int prop_getValues(void *handle, char ***values, int *valuescount)
{
    int ret = 0, count = 0, index = 0;
    PROPS_HANDLE *ph = NULL;
    Properties *pCurrent = NULL;
    char **pValues = NULL;
    if (handle == NULL || values == NULL || valuescount == NULL) {
        ret = -1;
        printf("fun getValues error:%d from (handle == NULL || values == NULL || valuescount == NULL)\n",ret);
        return ret;
    }
     
    // 获取配置项数量
    ret = prop_getCount(handle, &count);
    if (ret != 0) {
        printf("fun getValues error:%d from getCount \n",ret);
        return ret;
    }
     
    // 申请内存空间，存放所有的value
    pValues = (char **)malloc(sizeof(char *) * count);
    if (pValues == NULL) {
        ret = -2;
        printf("fun getValues error:%d from malloc values\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)handle;
    pCurrent = ph->pHead->pNext;
    while (pCurrent != NULL) {
        pValues[index] = pCurrent->value;
        pCurrent = pCurrent->pNext;
        index++;
    }
     
    *values = pValues;
    *valuescount = count;
     
    return ret;
}
 
// 释放所有value的内存空间，成功返回0，失败返回非0值
int prop_free_values(char ***values, int *valuescount)
{
    int ret = 0;
    if (values == NULL || valuescount == NULL) {
        ret = -1;
        printf("fun free_values error:%d from (values == NULL || valuescount == NULL) \n",ret);
        return ret;
    }
     
    free(*values);
    *values = NULL;
    *valuescount = 0;
     
    return ret;
}
 
// 释放环境资源，成功返回0，失败返回非0值
int prop_release(void **handle)
{
    int ret = 0;
    PROPS_HANDLE *ph = NULL;
    if(handle == NULL)
    {
        ret = -1;
        printf("release error:%d from (handler == NULL)\n",ret);
        return ret;
    }
     
    ph = (PROPS_HANDLE *)*handle;
     
    // 释放链表内存资源
    Properties *pCurr = ph->pHead;
    Properties *pTemp = NULL;
     
    while (pCurr != NULL) {
        if (pCurr->key != NULL) {
            free(pCurr->key);
            pCurr->key = NULL;
        }
         
        if (pCurr->value != NULL) {
            free(pCurr->value);
            pCurr->value = NULL;
        }
         
        pTemp = pCurr->pNext;
         
        free(pCurr);
         
        pCurr = pTemp;
    }
     
    // 释放存放配置文件路径分配的内存空间
    if(ph->filepath != NULL)
    {
        free(ph->filepath);
        ph->filepath = NULL;
    }
     
    // 释放环境句柄本身
    free(ph);
    *handle = NULL;    // 避免野指针
         
    return ret;
}
 
// 去空格
static int trimeSpace(const char *src,char *dest)
{
    int ret = 0;
    if (src == NULL || dest == NULL) {
        ret = -1;
        printf("trimeSpace error:%d from (src == NULL || dest == NULL)\n",ret);
        return ret;
    }
     
    const char *psrc = src;
    unsigned long i = 0,j = strlen(psrc) - 1,len;
    while (psrc[i] == ' ' || psrc[i] == '\n' || psrc[i] == '\r')
    {
        i++;
    }
     
    while (psrc[j] == ' ' || psrc[j] == '\n' || psrc[j] =='\r' ) {
        j--;
    }
     
    len = j - i + 1;
     
    memcpy(dest,psrc+i,len);
    *(dest+len) = '\0';
     
    return ret;
}
 
// 创建一个节点
static int createPropsNode(Properties **props)
{
    int ret = 0;
    Properties *p = NULL;
    if (props == NULL) {
        ret = -100;
        printf("createProps error:%d from (props == NULL)\n",ret);
        return ret;
    }
     
    p = (Properties *)malloc(sizeof(Properties));
    if (p == NULL) {
        ret = -200;
        printf("createProps malloc %ld bytes error:%d\n",sizeof(Properties),ret);
        return ret;
    }
    p->key = (char *)malloc(KEY_SIZE);
    p->value = (char *)malloc(VALUE_SIZE);
    p->pNext = NULL;
     
    *props = p;
     
    return ret;
}
 
// 保存到文件
static int saveConfig(const char *filepath,Properties *head)
{
    int ret = 0,writeLen = 0;
    FILE *fp = NULL;
    Properties *pCurrent = NULL;
    if (filepath == NULL || head == NULL) {
        ret = -100;
        printf("fun saveConfig error:%d from (filepath == NULL || head == NULL)\n",ret);
        return ret;
    }
     
    fp = fopen(filepath,"w");
    if (fp == NULL) {
        ret = -200;
        printf("fun saveConfig:open file error:%d from %s\n",ret,filepath);
        return ret;
    }
     
    pCurrent = head->pNext;
    while (pCurrent != NULL) {
        writeLen = fprintf(fp, "%s=%s",pCurrent->key,pCurrent->value);    // 返回写入的字节数，出现错误返回一个负值
        if (writeLen < 0) {  //TODO 如果写入失败，如何将写入的数据回退？？？
            ret = -300;
            printf("fun saveConfig err:%d from (%s=%s)\n",ret,pCurrent->key,pCurrent->value);
            break;
        }
        pCurrent = pCurrent->pNext;
    }
 
    fclose(fp); // 关闭文件
     
    return ret;
}
