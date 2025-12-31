#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<limits.h>

#define PROCESS_NAME_LEN 32 /*进程名长度*/
#define MIN_SLICE 10 /*最小碎片的大小*/
#define DEFAULT_MEM_SIZE 1024 /*内存大小*/
#define DEFAULT_MEM_START 0 /*起始位置*/
/* 内存分配算法 */
#define MA_FF 1
#define MA_BF 2
#define MA_WF 3

int mem_size = DEFAULT_MEM_SIZE; /*内存大小*/
int ma_algorithm = MA_FF; /*当前分配算法*/
static int pid = 0; /*初始 pid*/
int flag = 0; /*设置内存大小标志*/

/*描述每一个空闲块的数据结构*/
struct free_block_type{
    int size;
    int start_addr;
    struct free_block_type *next; 
};

/*指向内存中空闲块链表的首指针*/
struct free_block_type *free_block;

/*每个进程分配到的内存块的描述*/
struct allocated_block{
    int pid;
    int size;  
    int start_addr;   
    char process_name[PROCESS_NAME_LEN];
    struct allocated_block *next;
};

/*进程分配内存块链表的首指针*/
struct allocated_block *allocated_block_head = NULL;

//声明
struct free_block_type* init_free_block(int mem_size);

void display_menu();
int set_mem_size();
void set_algorithm();
void rearrange(int algorithm);
void rearrange_FF();
void rearrange_BF();
void rearrange_WF();
void new_process();
int allocate_mem(struct allocated_block *ab);
void kill_process();

struct allocated_block* find_process(int pid);

int free_mem(struct allocated_block *ab);
int dispose(struct allocated_block *free_ab);

int total_free_space();
void compact_memory();

void insert_free_block_sorted_by_addr(struct free_block_type *node);
void insert_free_block_sorted_by_size_asc(struct free_block_type *node);
void insert_free_block_sorted_by_size_desc(struct free_block_type *node);

void clear_free_list();

void clear_allocated_list();

void do_exit();

int compare_free_by_addr(const void *a, const void *b); //排序算法
int compare_free_by_size_asc(const void *a, const void *b);
int compare_free_by_size_desc(const void *a, const void *b);

void display_mem_usage();

/* main */
int main(){
    char choice;
    pid = 0;
    free_block = init_free_block(mem_size); //初始化空闲区
    while(1) {
        display_menu(); //显示菜单
        fflush(stdout);  //刷新输出缓冲区
        choice = getchar(); //获取用户输入
        while(getchar()!='\n'); // 清掉输入行余下字符
        switch(choice){
            case '1': set_mem_size(); break; //设置内存大小
            case '2': set_algorithm(); flag = 1; break;//设置算法
            case '3': new_process(); flag = 1; break;//创建新进程
            case '4': kill_process(); flag = 1; break;//删除进程
            case '5': display_mem_usage(); flag = 1; break; //显示内存使用
            case '0': do_exit(); return 0; //释放链表并退出
            default: break;
        }
    }
    return 0;
}

/*初始化空闲块，默认为一块，可以指定大小及起始地址*/
struct free_block_type* init_free_block(int mem_size){
    struct free_block_type *fb;
    fb = (struct free_block_type *)malloc(sizeof(struct free_block_type));
    if(fb == NULL){
        printf("No mem\n");
        return NULL;
    }
    fb->size = mem_size;
    fb->start_addr = DEFAULT_MEM_START;
    fb->next = NULL;
    return fb;
}

/*显示菜单*/
void display_menu(){
    printf("\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Display memory usage \n");
    printf("0 - Exit\n");
}

/*设置内存的大小*/
int set_mem_size(){  //输入为1
    int size;
    if(flag!=0){ //防止重复设置
        printf("Cannot set memory size again\n");
        return 0;
    }
    printf("Total memory size = ");
    if(scanf("%d", &size) != 1){
        while(getchar()!='\n');
        printf("Invalid input\n");
        return 0;
    }
    while(getchar()!='\n'); // 清掉残留输入
    if(size>0) {
        mem_size = size;
        if(free_block) {
            free_block->size = mem_size;
            free_block->start_addr = DEFAULT_MEM_START;
            free_block->next = NULL;
        } else {
            free_block = init_free_block(mem_size);
        }
    }
    flag=1; //已经设置完了
    return 1;
}

/* 设置当前的分配算法 */
void set_algorithm(){
    int algorithm;
    printf("\t1 - First Fit\n");
    printf("\t2 - Best Fit \n");
    printf("\t3 - Worst Fit \n");
    if(scanf("%d", &algorithm) != 1){  //输入失败退出
        while(getchar()!='\n');
        return;
    }
    while(getchar()!='\n');
    if(algorithm>=1 && algorithm <=3)  ma_algorithm = algorithm;
    //按指定算法重新排列空闲区链表
    rearrange(ma_algorithm);
}

/*按指定的算法整理内存空闲块链表*/
void rearrange(int algorithm){
    switch(algorithm){
        case MA_FF: rearrange_FF(); break;
        case MA_BF: rearrange_BF(); break;
        case MA_WF: rearrange_WF(); break;
        default: break;
    }
}

/*辅助：将当前 free list 转换为数组并基于比较函数重建链表*/
void rebuild_free_list_with_cmp(int (*cmp)(const void *, const void *)){
    // count
    int cnt = 0;
    struct free_block_type *p = free_block;
    while(p){ cnt++; p = p->next; } //空闲块个数计算
    if(cnt <= 1) return; // 无需重建
    struct free_block_type **arr = malloc(sizeof(struct free_block_type*) * cnt);  //指针数组
    if(!arr) return;
    p = free_block;
    for(int i=0;i<cnt;i++){ arr[i] = p; p = p->next; }
    qsort(arr, cnt, sizeof(struct free_block_type*), cmp);
    // rebuild
    free_block = arr[0];
    p = free_block;
    for(int i=1;i<cnt;i++){
        p->next = arr[i];
        p = p->next;
    }
    p->next = NULL;
    free(arr);
}

//按 FF（按地址升序）
void rearrange_FF(){
    rebuild_free_list_with_cmp(compare_free_by_addr);
}

//按 BF（按大小升序）
void rearrange_BF(){
    rebuild_free_list_with_cmp(compare_free_by_size_asc);
}

//按 WF（按大小降序）
void rearrange_WF(){
    rebuild_free_list_with_cmp(compare_free_by_size_desc);
}

/*创建新的进程，主要是获取内存的申请数量*/
void new_process(){
    struct allocated_block *ab;
    int size; int ret;
    ab = (struct allocated_block *)malloc(sizeof(struct allocated_block));
    if(!ab) exit(-5);
    ab->next = NULL;
        pid++;
        sprintf(ab->process_name, "PROCESS-%02d", pid);
    ab->pid = pid;
    printf("Memory for %s: ", ab->process_name);
    if(scanf("%d", &size) != 1){
        while(getchar()!='\n');
        printf("Invalid input\n");
        free(ab);
        return;
    }
    while(getchar()!='\n');
    if(size>0) ab->size = size;
    else { free(ab); return; }

    ret = allocate_mem(ab); /* 从空闲区分配内存，ret==1 表示分配 ok*/

    //如果此时 allocated_block_head 尚未赋值，则赋值
    if((ret==1) && (allocated_block_head == NULL)){
        ab->next = NULL;
        allocated_block_head = ab;
        return;
    }
    //分配成功，将该已分配块的描述插入已分配链表
    else if (ret==1) {
        ab->next = allocated_block_head;
        allocated_block_head = ab;
        return;
    }
    else if(ret==-1){ //分配失败
        printf("Allocation fail\n");
        free(ab);
        return;
    }
    return;
}

/*分配内存模块*/
int allocate_mem(struct allocated_block *ab){
    struct free_block_type *fbt, *pre;
    int request_size = ab->size;
    fbt = pre = free_block;

    // 1) 根据当前算法搜索合适的空闲分区
    struct free_block_type *chosen = NULL, *chosen_pre = NULL;
    if(ma_algorithm == MA_FF){
        // First Fit: 第一个满足的
        while(fbt){
            if(fbt->size >= request_size){
                chosen = fbt;
                chosen_pre = pre;
                break;
            }
            pre = fbt;
            fbt = fbt->next;
        }
    } else if(ma_algorithm == MA_BF){
        // Best Fit: 遍历找到最小的满足者
        int best_size = INT_MAX;
        while(fbt){
            if(fbt->size >= request_size && fbt->size < best_size){
                best_size = fbt->size;
                chosen = fbt;
                chosen_pre = pre;
            }
            pre = fbt;
            fbt = fbt->next;
        }
    } else if(ma_algorithm == MA_WF){
        // Worst Fit: 找最大的
        int worst_size = -1;
        while(fbt){
            if(fbt->size >= request_size && fbt->size > worst_size){
                worst_size = fbt->size;
                chosen = fbt;
                chosen_pre = pre;
            }
            pre = fbt;
            fbt = fbt->next;
        }
    }

    // 2) 如果找到了单块满足的空闲区
    if(chosen){
        // 如果分割后剩余大于等于 MIN_SLICE，则分割
        if(chosen->size - request_size >= MIN_SLICE){
            ab->start_addr = chosen->start_addr; 
            chosen->start_addr += request_size;
            chosen->size -= request_size;
            // 分配成功，按算法重排 free list（某些算法需要）
            rearrange(ma_algorithm);
            return 1;
        } else {
            // 剩余过小，直接整体分配
            ab->start_addr = chosen->start_addr;
            // 从链表中删除 chosen
            if(chosen == free_block){
                free_block = chosen->next;
            } else {
                // 找 chosen 的前驱并删除
                struct free_block_type *tmp = free_block;
                while(tmp && tmp->next != chosen) tmp = tmp->next;
                if(tmp) tmp->next = chosen->next;
            }
            free(chosen);
            rearrange(ma_algorithm);
            return 1;
        }
    }

    // 3) 没有单块满足，检查总空闲是否足够 -> 若足够，则进行紧缩后再分配
    if(total_free_space() >= request_size){
        // 紧缩内存
        compact_memory();
        // 紧缩后尝试再次分配：简单使用 FF 行为（因为紧缩后只有一个空闲块）
        if(free_block && free_block->size >= request_size){
            if(free_block->size - request_size >= MIN_SLICE){
                ab->start_addr = free_block->start_addr;
                free_block->start_addr += request_size;
                free_block->size -= request_size;
            } else {
                ab->start_addr = free_block->start_addr;
                struct free_block_type *tmp = free_block;
                free_block = free_block->next;
                free(tmp);
            }
            rearrange(ma_algorithm);
            return 1;
        } else {
            // 紧缩后仍不够
            return -1;
        }
    }

    // 4) 总空闲不足，分配失败
    return -1;
}

/*删除进程，归还分配的存储空间，并删除描述该进程内存分配的节点*/
void kill_process(){
    struct allocated_block *ab;
    int pid_in;
    printf("Kill Process, pid=");
    if(scanf("%d", &pid_in) != 1){
        while(getchar()!='\n');
        printf("Invalid input\n");
        return;
    }
    while(getchar()!='\n'); 
    ab = find_process(pid_in);
    if(ab!=NULL){
        free_mem(ab); /*释放 ab 所表示的分配区*/
        dispose(ab); /*释放 ab 数据结构节点*/
    } else {
        printf("Process %d not found\n", pid_in);
    }
}

//将 ab 所表示的已分配区归还，并进行可能的合并  归还空闲列表
int free_mem(struct allocated_block *ab){
    int algorithm = ma_algorithm;
    struct free_block_type *fbt;
    fbt = (struct free_block_type*) malloc(sizeof(struct free_block_type));
    if(!fbt) return -1;

    fbt->start_addr = ab->start_addr;
    fbt->size = ab->size;
    fbt->next = NULL;

    // 插入到空闲链表（先按地址插入末尾或末尾插入再排序）
    // 我们直接插入到链表头以方便实现，然后按地址排序并合并。
    if(!free_block){
        free_block = fbt;
    } else {
        // 插在链表末尾
        struct free_block_type *p = free_block;
        while(p->next) p = p->next;
        p->next = fbt;
    }

    // 对空闲链表按地址排序并合并相邻的分区
    rearrange_FF(); // 先按地址排序
    // 合并相邻块
    struct free_block_type *p = free_block;
    while(p && p->next){
        if(p->start_addr + p->size == p->next->start_addr){
            // 合并 p 和 p->next 
            struct free_block_type *to_remove = p->next;
            p->size += to_remove->size;
            p->next = to_remove->next;
            free(to_remove);
        } else {
            p = p->next;
        }
    }

    // 最后按当前算法重新整理 free list
    rearrange(algorithm);
    return 1;
}

//释放 ab 数据结构节点 释放分配链表
int dispose(struct allocated_block *free_ab){
    struct allocated_block *pre, *ab;
    if(free_ab == NULL) return -1; // 空指针
    if(allocated_block_head == NULL) return -1; // 空链表
    if(allocated_block_head == free_ab) { // 如果要释放第一个节点
        allocated_block_head = allocated_block_head->next;
        free(free_ab);
        return 1;
    }

    pre = allocated_block_head;  // 找到要删除的节点的前驱
    ab = allocated_block_head->next;  //当前节点
    while(ab && ab!=free_ab){ 
        pre = ab; 
        ab = ab->next; 
    }
    if(ab==NULL) return -1; // 未找到
    pre->next = ab->next; // 删除节点
    free(ab);
    return 2;
}

/* 显示当前内存的使用情况，包括空闲区的情况和已经分配的情况 */
void display_mem_usage(){
    struct free_block_type *fbt = free_block;
    struct allocated_block *ab = allocated_block_head;
    if(fbt==NULL) {
        printf("No free block\n");
    }
    printf("----------------------------------------------------------\n");
    /* 显示空闲区 */
    printf("Free Memory:\n");
    printf("%20s %20s\n", " start_addr", " size");
    while(fbt!=NULL){
        printf("%20d %20d\n", fbt->start_addr, fbt->size);
        fbt=fbt->next;
    }
    /* 显示已分配区 */
    printf("\nUsed Memory:\n");
    printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
    while(ab!=NULL){
        printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name,
               ab->start_addr, ab->size);
        ab=ab->next;
    }
    printf("----------------------------------------------------------\n");
    return 0;
}

/* 辅助：查找进程 */
struct allocated_block* find_process(int pid_in){
    struct allocated_block *p = allocated_block_head;
    while(p){
        if(p->pid == pid_in) return p;
        p = p->next;
    }
    return NULL;
}

/* 计算总空闲空间 */
int total_free_space(){
    int total = 0;
    struct free_block_type *p = free_block;
    while(p){
        total += p->size;
        p = p->next;
    }
    return total;
}

//  紧缩内存：将所有已分配块压缩到起始地址，保持进程之间的相对地址顺序（按 start_addr 升序） 
void compact_memory(){
    // 若没有已分配块则整个内存空闲
    if(allocated_block_head == NULL){
        // 释放现有 free list，创建单一空闲块
        clear_free_list();
        free_block = init_free_block(mem_size);
        return;
    }

    // 把 allocated blocks 放入数组并按 start_addr 升序排列
    int count = 0;
    struct allocated_block *p = allocated_block_head;
    while(p) { count++; p = p->next; }
    struct allocated_block **arr = malloc(sizeof(struct allocated_block*) * count);
    if(!arr) return;
    p = allocated_block_head;
    for(int i=0;i<count;i++){ arr[i] = p; p = p->next; }
    for(int i=0;i<count-1;i++){
        for(int j=0;j<count-1-i;j++){
            if(arr[j]->start_addr > arr[j+1]->start_addr){
                struct allocated_block *tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
            }
        }
    }
    // 将进程紧凑到内存起始处，保持顺序
    int cur = DEFAULT_MEM_START;
    for(int i=0;i<count;i++){
        arr[i]->start_addr = cur;
        cur += arr[i]->size;
    }
    // 现在构建 free_block 为单一空闲块（剩余的）
    clear_free_list();
    if(cur < mem_size){
        free_block = init_free_block(mem_size - cur);
        if(free_block){
            free_block->start_addr = cur;
        }
    } else {
        free_block = NULL;
    }
    free(arr);
    // 最终按当前算法重排（这里紧缩后通常只有一个空闲块）
    rearrange(ma_algorithm);
}

//  清空 free list（释放所有节点） 
void clear_free_list(){
    struct free_block_type *p = free_block;
    while(p){
        struct free_block_type *n = p->next;
        free(p);
        p = n;
    }
    free_block = NULL;
}

// 清空 allocated list（释放所有描述节点） 
void clear_allocated_list(){
    struct allocated_block *p = allocated_block_head;
    while(p){
        struct allocated_block *n = p->next;
        free(p);
        p = n;
    }
    allocated_block_head = NULL;
}

// 退出时释放链表资源 
void do_exit(){
    clear_free_list();
    clear_allocated_list();
}

// 比较函数用于 qsort（按地址）  适用于FF
int compare_free_by_addr(const void *a, const void *b){
    struct free_block_type *pa = *(struct free_block_type **)a;
    struct free_block_type *pb = *(struct free_block_type **)b;
    if(pa->start_addr < pb->start_addr) return -1;  //pa在前 pb在后
    else if(pa->start_addr > pb->start_addr) return 1;
    return 0;
}

//  比较函数（按 size 升序）   BF
int compare_free_by_size_asc(const void *a, const void *b){
    struct free_block_type *pa = *(struct free_block_type **)a;
    struct free_block_type *pb = *(struct free_block_type **)b;
    if(pa->size < pb->size) return -1;
    else if(pa->size > pb->size) return 1;
    return 0;
}

// 比较函数（按 size 降序）    WF
int compare_free_by_size_desc(const void *a, const void *b){
    struct free_block_type *pa = *(struct free_block_type **)a;
    struct free_block_type *pb = *(struct free_block_type **)b;
    if(pa->size > pb->size) return -1;
    else if(pa->size < pb->size) return 1;
    return 0;
}
