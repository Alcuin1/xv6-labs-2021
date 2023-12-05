	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch: //uthread_switch.S 中需要实现上下文切换的代码，这里借鉴 swtch.S：
	/* YOUR CODE HERE */
	ret    /* return to ra */
