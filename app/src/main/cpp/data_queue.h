//
// Created by Administrator on 2019/3/5.
//
#include <stdint.h>
#include <string.h>
#include <android/log.h>
#define TAG "ffnative" // 这个是自定义的LOG的标识
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型

extern "C" {
struct data_queue {
    data_queue *next;
    uint8_t *data;
};

void add(data_queue *&head, data_queue *&tail, uint8_t *data, int data_size) {
    data_queue *self = (data_queue *) (malloc(sizeof(data_queue)));
    self->data = (uint8_t *) malloc(data_size * sizeof(uint8_t));
    self->next = NULL;
    memcpy(self->data, data, data_size);
    if (!tail) {
        tail = self;
    } else {
        tail->next = self;
        tail = self;
    }
    if (!head) {
        head = self;
    }
}

data_queue *pop(data_queue *&head, data_queue *&tail) {
    if (!head) {
        return NULL;
    } else {
        data_queue *ret = head;
        head = head->next;
        return ret;
    }
}

void free_data(data_queue *&node) {
    if (node) {
        LOGI("node:%d", node);
        free(node->data);
        node->data = NULL;
        free(node);
        node = NULL;
    }
}

}