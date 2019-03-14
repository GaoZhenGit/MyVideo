#include "data_queue.h"

data_queue::data_queue() {

}

void data_queue::set_copy_fun(void (*copy_fun)(uint8_t *, uint8_t *)) {
    this->copy_fun = copy_fun;
}

void data_queue::add(uint8_t *data, int data_size) {
    data_node *self = new data_node();
    self->data = new uint8_t[data_size];
    self->next = NULL;
    if (copy_fun) {
        copy_fun(data, self->data);
    } else {
        memcpy(self->data, data, data_size);
    }
    if (!tail) {
        tail = self;
    } else {
        tail->next = self;
        tail = self;
    }
    if (!head) {
        head = self;
    }
    count++;
}

data_node *data_queue::pop() {
    if (!head) {
        return NULL;
    } else {
        data_node *ret = head;
        head = head->next;
        count--;
        return ret;
    }
}

void data_queue::free(data_node *&node) {
    if (node) {
        delete[] node->data;
        node->data = NULL;
        delete node;
        node = NULL;
    }
}

data_queue *create_data_queue() {
    return new data_queue();
}

void free_data_queue(data_queue *& dq) {
    data_node *node = dq->pop();
    while (node) {
        dq->free(node);
        node = dq->pop();
    }
    delete dq;
    dq = NULL;
}
