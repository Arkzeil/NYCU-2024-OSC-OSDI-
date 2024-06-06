#ifndef UARTFS_H
#define UARTFS_H

#include "kernel/vfs.h"

int dev_uart_write(struct file *file, const void *buf, my_uint64_t len);
int dev_uart_read(struct file *file, void *buf, my_uint64_t len);
int dev_uart_open(struct vnode *file_node, struct file **target);
int dev_uart_close(struct file *file);

struct file_operations dev_f_ops = {dev_uart_write, dev_uart_read, dev_uart_open, dev_uart_close, op_denied, op_denied};

#endif