	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch: //uthread_switch.S 中需要实现上下文切换的代码，这里借鉴 swtch.S：
	/* YOUR CODE HERE */
	// 这里看到的就是之前保存的当前CPU核的调度器线程的寄存器。在这些寄存器中，最有趣的就是ra（Return Address）寄存器，因为ra寄存器保存的是当前函数的返回地址，
	// 所以调度器线程中的代码会返回到ra寄存器中的地址。通过查看kernel.asm，我们可以知道这个地址的内容是什么。也可以在gdb中输入“x/i 0x80001f2e”进行查看。
	// 首先，ra寄存器被保存在了a0寄存器指向的地址。a0寄存器对应了swtch函数的第一个参数，从前面可以看出这是当前线程的context对象地址；
	sd ra, 0(a0) //含义：Store Doubleword，指令是RISC-V指令集中的一条存储指令,用于将一个双字(64位)数据存储到内存中。将ra存储到a0寄存器指向的地址
	sd sp, 8(a0)
	sd s0, 16(a0)
	sd s1, 24(a0)
	sd s2, 32(a0)
	sd s3, 40(a0)
	sd s4, 48(a0)
	sd s5, 56(a0)
	sd s6, 64(a0)
	sd s7, 72(a0)
	sd s8, 80(a0)
	sd s9, 88(a0)
	sd s10, 96(a0)
	sd s11, 104(a0)
	// a1寄存器对应了swtch函数的第二个参数，从前面可以看出这是即将要切换到的调度器线程的context对象地址。
	ld ra, 0(a1) // load a1寄存器中的值读到ra寄存器中
	ld sp, 8(a1) // 它实际是当前进程的内核栈地址，它由虚拟内存系统映射在了一个高地址。
	ld s0, 16(a1)
	ld s1, 24(a1)
	ld s2, 32(a1)
	ld s3, 40(a1)
	ld s4, 48(a1)
	ld s5, 56(a1)
	ld s6, 64(a1)
	ld s7, 72(a1)
	ld s8, 80(a1)
	ld s9, 88(a1)
	ld s10, 96(a1)
	ld s11, 104(a1)
	// 上半部分是将当前的寄存器保存在当前线程对应的context对象中，函数的下半部分是将调度器线程的寄存器，也就是我们将要切换到的线程的寄存器恢复到CPU的寄存器中。
	// 之后函数就返回了。所以调度器线程的ra寄存器的内容才显得有趣，因为它指向的是swtch函数返回的地址，也就是scheduler函数。
	ret    /* return to ra */
	// 补充：另一个问题是，为什么RISC-V中有32个寄存器，但是swtch函数中只保存并恢复了14个寄存器？
	// 学生回答：因为switch是按照一个普通函数来调用的，对于有些寄存器，swtch函数的调用者默认swtch函数会做修改，所以调用者已经在自己的栈上保存了这些寄存器，当函数返回时，这些寄存器会自动恢复。所以swtch函数里只需要保存Callee Saved Register就行
	// 完全正确！因为swtch函数是从C代码调用的，所以我们知道Caller Saved Register会被C编译器保存在当前的栈上。Caller Saved Register大概有15-18个，而我们在swtch函数中只需要处理C编译器不会保存，但是对于swtch函数又有用的一些寄存器。所以在切换线程的时候，我们只需要保存Callee Saved Register。
	// 即在调用本函数 uthread_switch() 的过程中，caller-saved registers 已经被调用者保存到栈帧中了，所以这里无需保存这一部分寄存器。 

	// 在我们实际返回之前，我们再来打印一些有趣的寄存器。首先sp寄存器有了一个不同的值，sp寄存器的值现在在内存中的stack0区域中。
	// 这个区域实际上是在启动顺序中非常非常早的一个位置，start.s在这个区域创建了栈，这样才可以调用第一个C函数。所以调度器线程运行在CPU对应的bootstack上。
	// 其次是ra寄存器，现在指向了scheduler函数，因为我们恢复了调度器线程的context对象中的内容。

	// 学生提问：为什么swtch函数要用汇编来实现，而不是C语言？
    // Robert教授：C语言中很难与寄存器交互。可以肯定的是C语言中没有方法能更改sp、ra寄存器。所以在普通的C语言中很难完成寄存器的存储和加载，唯一的方法就是在C中嵌套汇编语言。所以我们也可以在C函数中内嵌switch中的指令，但是这跟我们直接定义一个汇编函数是一样的。或者说swtch函数中的操作是在C语言的层级之下，所以并不能使用C语言。