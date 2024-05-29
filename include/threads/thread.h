#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "synch.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#define FDT_PAGES 3			
#define FD_LIMIT FDT_PAGES * (1 << 9)		/* file descriptor table의 한계 index */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */
	
	/* PDG 스레드를 깨울 시간 set : 1tick 10ms */
	int64_t wakeup_tick;
	/* PDG 원본 우선순위*/
	int org_priority;
	/* PDG 대기하고 있는 락*/
	struct lock* wait_on_lock;			
	/* PDG 해당 쓰레드에 걸린 multi 락 */
	struct list donation;	 //d_elem1 d_elem2 d_elem3		 
	/* PDG 도네이션용 엘리먼트 */
	struct list_elem d_elem;              /* List element. */
	/* PDG 전체리스트 엘리먼트 */
	struct list_elem a_elem;              /* List element. */
	/* PDG MLFQ 상냥함 구현 */
	int nice;              /* List element. */
	/* PDG MLFQ CPU 사용계수 구현 */
	int recent_cpu;              /* List element. */

	/* 프로세스(쓰레드)의 파일 디스크립터 테이블 */
	struct file **fd_table;			// 프로세스가 파일을 열면 해당 파일의 포인터를 이 배열에 저장함
									// file descriptor table의 시작 주소 가리키도록 초기화
	/* fd table의 open한 곳의 index */
	int fd_idx;

	/* 부모의 interrupt frame */
	struct intr_frame parent_if;
	/* fork를 통해 만들어진 child의 load를 기다리는 semaphore */
	struct semaphore fork_sema;

	/* 자식 프로세스 list의 element */
	struct list_elem ch_elem;
	/* 자식 프로세스 list */
	struct list child;

	/* wait semaphore */
	struct semaphore wait_sema;
	/* free semaphore */
	struct semaphore free_sema;
	
	/* 자식 프로세스의 exit 상태를 나타내는 값 */
	int exit_status;

	struct file *running;

	
#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

int64_t global_ticks;


int64_t get_globalticks(void);
bool compare_priority(const struct list_elem *curr, const struct list_elem *new, void *aux UNUSED);
bool compare_donation_priority(const struct list_elem *curr, const struct list_elem *new, void *aux UNUSED);

void thread_init (void);
void thread_start (void);

/*PDG start*/
void thread_wakeup(void);
void thread_sleep(int64_t local_ticks);
/*PDG end*/

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

void priority_preemption(void);
#endif /* threads/thread.h */
