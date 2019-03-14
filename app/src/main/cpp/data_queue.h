//
// Created by Administrator on 2019/3/5.
//
#ifndef MYVIDEO_DATA_QUEUE_H
#define MYVIDEO_DATA_QUEUE_H

extern "C" {

#include <stdint.h>
#include <string.h>
#include <android/log.h>
#include <malloc.h>

struct data_queue {
    data_queue *next;
    uint8_t *data;
};

static void add(data_queue *&head, data_queue *&tail, uint8_t *data, int data_size) {
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

static data_queue *pop(data_queue *&head, data_queue *&tail) {
    if (!head) {
        return NULL;
    } else {
        data_queue *ret = head;
        head = head->next;
        return ret;
    }
}

static void free_data(data_queue *&node) {
    if (node) {
        free(node->data);
        node->data = NULL;
        free(node);
        node = NULL;
    }
}

}

#endif //MYVIDEO_DATA_QUEUE_H