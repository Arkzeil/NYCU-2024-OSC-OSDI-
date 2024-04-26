.global call_get_pid
call_get_pid:
    mov w8, #0	
    svc #0
    ret

.global call_uart_read
call_uart_read:
    mov w8, #1	
    svc #0
    ret

.global call_uart_write
call_uart_write:
	mov w8, #2	
	svc #0
	ret

.global call_exec
call_exec:
	mov w8, #3	
	svc #0
	ret

.global call_fork
call_fork:
	mov w8, #4	
	svc #0
	ret

.global call_exit
call_exit:
	mov w8, #5	
	svc #0
	ret

.global call_mbox
call_mbox:
	mov w8, #5	
	svc #0
	ret

.global call_kill
call_kill:
	mov w8, #7
	svc #0
	ret

.global call_sys_clone
call_sys_clone:
	/* Save args for the child.  */
	mov	x10, x0					/*fn*/
	mov	x11, x1					/*arg*/
	mov	x12, x2					/*stack*/

	/* Do the system call.  */
	mov 	x0, x2					/* stack  */
	mov	x8, #4
	svc	0x0

	cmp	x0, #0
	beq	thread_start
	ret

thread_start:
	mov	x29, 0

	/* Pick the function arg and execute.  */
	mov	x0, x11
	blr	x10

	/* We are done, pass the return value through x0.  */
	mov	x8, #5
	svc	0x0