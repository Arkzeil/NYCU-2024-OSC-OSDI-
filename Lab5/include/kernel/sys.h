#ifndef SYS_H
#define SYS_H
// If __ASSEMBLER__ is not defined, it means that the file is being compiled by a C/C++ compiler, and the code between #ifndef __ASSEMBLER__ and #endif will be included.
#ifndef __ASSEMBLER__

int call_get_pid();
unsigned int call_uart_read(char buf[], unsigned int size);
unsigned int call_uart_write(char buf[], unsigned int size);
int call_exec(const char* name, char *const argv[]);
int call_fork();
void call_exit();
int call_mbox(unsigned char ch, unsigned int *mbox);
void call_kill(int pid);

#endif
// If __ASSEMBLER__ is defined, it means that the file is being processed by an assembler, and the code between #ifndef __ASSEMBLER__ and #endif will be skipped.
#endif