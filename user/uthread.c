#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

// 从 proc.h 中借鉴一下 context 结构体，用于保存 ra、sp 以及 callee-saved registers： 参考https://mit-public-courses-cn-translatio.gitbook.io/mit6-s081/lec11-thread-switching-robert/11.7-xv6-switch-function
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct context ctx; // 在 thread 中添加 context 结构体
};
struct thread all_thread[MAX_THREAD]; // definate the thread array which contains MAX_THREAD = 4 threads C语言中，数组名称即地址
struct thread *current_thread; 
// extern void thread_switch(uint64, uint64);  // 是声明，不是定义，没有分配内存。但是在别处定义了。
extern void thread_switch(struct context* old, struct context* new); // 修改 thread_switch 函数声明
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for(int i = 0; i < MAX_THREAD; i++){
    if(t >= all_thread + MAX_THREAD) // all_thread is the address in C language. It seems like the % operation
      t = all_thread;
    if(t->state == RUNNABLE) { 
      next_thread = t;
      break;
    }
    t = t + 1;
  }
  // 如果没有可运行的线程，exit退出
  if (next_thread == 0) {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }
  
  // 准备切换线程
  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
     // swtch函数会保存用户进程P1对应内核线程的寄存器至context对象。所以目前为止有两类寄存器：用户寄存器存在trapframe中，内核线程的寄存器存在context中。 
     // trapframe还是只包含进入和离开内核时的数据。而context结构体中包含的是在内核线程和 调度器线程 之间切换时，需要保存和恢复的数据。
     // swtch函数会保存内核线程的寄存器到进程的context中，然后切换到对应CPU的调度器线程，再让其他的线程运行。
     // context switching，他们通常说的是从一个线程切换到另一个线程，因为在切换的过程中需要先保存前一个线程的寄存器，然后再恢复之前保存的后一个线程的寄存器，这些寄存器都是保存在context对象中。
     // 在XV6的代码中，context对象总是由swtch函数产生，所以context总是保存了内核线程在执行swtch函数时的状态。当我们在恢复一个内核线程时，对于刚恢复的线程所做的第一件事情就是从之前的swtch函数中返回
     // swtch(&p->context, &c->context)含义如下：
     // swtch函数会将当前内核线程的寄存器保存到p->context中。swtch函数的另一个参数c->context，c表示当前CPU的结构体。CPU结构体中的context保存了当前CPU核的调度器线程的寄存器。所以swtch函数在保存完当前内核线程的内核寄存器之后，就会恢复当前CPU核的调度器线程的寄存器，并继续执行当前CPU核的调度器线程。
     thread_switch(&t->ctx, &next_thread->ctx);
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
  // 因为ra寄存器保存的是当前函数的返回地址，所以调度器线程中的代码会返回到ra寄存器中的地址。
  t->ctx.ra = (uint64)func; // 返回地址
  // thread_switch 的结尾会返回到 ra，从而运行线程代码
  t->ctx.sp = (uint64)&t->stack + (STACK_SIZE - 1);  // 栈指针
  // 将线程的栈指针指向其独立的栈，注意到栈的生长是从高地址到低地址，所以要将 sp 设置为指向 stack 的最高地址
  // 总之：添加的部分为设置上下文中 ra 指向的地址为线程函数的地址，这样在第一次调度到该线程，执行到 thread_switch 中的 ret 之后就可以跳转到线程函数从而开始执行了。设置 sp 使得线程拥有自己独有的栈，也就是独立的执行流。
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  thread_schedule();
  exit(0);
}
