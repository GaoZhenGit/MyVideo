//
// Created by Administrator on 2019/3/5.
//
#ifndef MYVIDEO_DATA_QUEUE_H
#define MYVIDEO_DATA_QUEUE_H

#include <stdint.h>
#include <string.h>
#include <android/log.h>

class data_node {
public:
    uint8_t *data;
    data_node *next;
};

class data_queue {
private:
    data_node *head = NULL;
    data_node *tail = NULL;
    void (*copy_fun)(uint8_t *, uint8_t *) = NULL;
public:
    uint16_t count = 0;

    data_queue();
    //设置增加节点时的复制函数，如果没有，就是用memcpy
    void set_copy_fun(void (*copy_fun)(uint8_t *, uint8_t *));
    //增加节点
    void add(uint8_t *data, int data_size);
    //弹出节点并移除
    data_node *pop();
    //节点内容回收
    void free(data_node *&node);
};

data_queue *create_data_queue();
void free_data_queue(data_queue *&);

#endif //MYVIDEO_DATA_QUEUE_H