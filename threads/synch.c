/* 이 파일은 Nachos 교육용 운영체제의 소스 코드에서 파생된 것입니다.
   Nachos 저작권 고지는 아래에 완전하게 재현되었습니다. */

/* 저작권 (c) 1992-1996 캘리포니아 대학교 리젠츠.
   모든 권리 보유.

   이 소프트웨어 및 관련 문서를 어떤 목적으로든 무료로, 서면 계약 없이
   사용, 복사, 수정 및 배포할 수 있는 권한이 부여되며,
   위의 저작권 고지와 다음 두 문단이 이 소프트웨어의 모든 복사본에
   포함되어야 합니다.

   캘리포니아 대학교는 이 소프트웨어 및 관련 문서의 사용으로 인해 발생한
   직간접적인, 특수한, 부수적인, 또는 결과적인 손해에 대해 어떠한 당사자에게도
   책임을 지지 않습니다. 캘리포니아 대학교는 이러한 손해의 가능성을 사전에
   통보받은 경우에도 마찬가지입니다.

   캘리포니아 대학교는 상품성 및 특정 목적에 대한 적합성을 포함하되
   이에 국한되지 않는 어떠한 보증도 명시적으로 부인합니다.
   여기에서 제공되는 소프트웨어는 "있는 그대로" 제공되며,
   캘리포니아 대학교는 유지보수, 지원, 업데이트, 향상 또는
   수정에 대한 의무가 없습니다.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* 세마포어 SEMA를 VALUE로 초기화합니다. 세마포어는
   두 가지 원자적 연산과 함께 사용하는 비음수 정수입니다:

   - down 또는 "P": 값이 양수가 될 때까지 기다린 후
   감소시킵니다.

   - up 또는 "V": 값을 증가시키고 (대기 중인 스레드가 있다면)
   하나의 대기 중인 스레드를 깨웁니다. */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* 세마포어에서 down 또는 "P" 연산. SEMA의 값이 양수가 될 때까지 기다린 후
   원자적으로 값을 감소시킵니다.

   이 함수는 잠들 수 있으므로 인터럽트 처리기 내에서 호출하면 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출될 수 있지만,
   만약 잠들면 다음 예약된 스레드가 아마도 인터럽트를 다시 활성화할 것입니다.
   이것은 sema_down 함수입니다. */
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		//PDG 웨이트리스트 정렬
		list_insert_ordered (&sema->waiters, &thread_current ()->elem, compare_priority, NULL);
		//list_push_back (&sema->waiters, &thread_current ()->elem);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* 세마포어에서 down 또는 "P" 연산을 수행하지만,
   세마포어가 이미 0이 아닌 경우에만 수행합니다.
   세마포어가 감소되면 true를 반환하고, 그렇지 않으면 false를 반환합니다.

   이 함수는 인터럽트 처리기에서 호출될 수 있습니다. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* 세마포어에서 up 또는 "V" 연산. SEMA의 값을 증가시키고,
   대기 중인 스레드가 있다면 하나의 스레드를 깨웁니다.

   이 함수는 인터럽트 처리기에서 호출될 수 있습니다. */
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (!list_empty (&sema->waiters))
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					struct thread, elem));
	sema->value++;
	intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* 세마포어의 자가 테스트로 두 개의 스레드 간에 제어를
   "핑퐁"하게 만듭니다. 진행 상황을 확인하기 위해 printf() 호출을 삽입하세요. */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* sema_self_test()에서 사용되는 스레드 함수. */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}
/* LOCK을 초기화합니다. 락은 언제든지 최대 하나의 스레드만 소유할 수 있습니다.
   우리의 락은 "재귀적"이지 않으며, 즉 현재 락을 소유한 스레드가
   다시 그 락을 획득하려고 시도하면 오류입니다.

   락은 초기 값이 1인 세마포어의 특수화입니다. 락과 세마포어의 차이는 두 가지입니다.
   첫째, 세마포어는 1보다 큰 값을 가질 수 있지만, 락은 한 번에
   하나의 스레드만 소유할 수 있습니다. 둘째, 세마포어에는 소유자가 없지만,
   락은 소유자가 있어야 하므로 한 스레드가 세마포어를 "down"하고
   다른 스레드가 "up"할 수 있지만, 락은 동일한 스레드가 획득하고
   해제해야 합니다. 이러한 제한이 부담이 될 경우, 세마포어를 사용해야 하는
   좋은 신호입니다. */
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* LOCK을 획득하고 필요하면 사용할 수 있을 때까지 잠듭니다.
   현재 스레드가 이미 락을 소유하고 있어서는 안 됩니다.

   이 함수는 잠들 수 있으므로 인터럽트 처리기 내에서 호출하면 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출될 수 있지만,
   필요하면 잠들기 때문에 인터럽트가 다시 활성화될 것입니다. */
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));
	sema_down (&lock->semaphore);
	lock->holder = thread_current ();
	//PDG 현재 락의 대기열의 처음의 우선순위를 설정
	//thread_current()->priority = list_entry(list_begin(&lock->semaphore.waiters), struct thread, elem)->priority;
}

/* LOCK을 획득하려고 시도하고 성공하면 true를 반환하고 실패하면 false를 반환합니다.
   현재 스레드가 이미 락을 소유하고 있어서는 안 됩니다.

   이 함수는 잠들지 않으므로 인터럽트 처리기 내에서 호출될 수 있습니다. */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* 현재 스레드가 소유한 LOCK을 해제합니다.
   이것은 lock_release 함수입니다.

   인터럽트 처리기는 락을 획득할 수 없으므로,
   락을 해제하려고 시도해서는 안 됩니다. */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));
	//PDG 도네이션으로 변경된 경우 원상복구
	//thread_set_priority(thread_current()->org_priority);
	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* 현재 스레드가 LOCK을 소유하고 있으면 true를 반환하고, 그렇지 않으면 false를 반환합니다.
   (다른 스레드가 락을 소유하고 있는지 테스트하는 것은 경쟁 조건이 발생할 수 있습니다.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* 목록에 있는 하나의 세마포어. */
struct semaphore_elem {
	struct list_elem elem;              /* 목록 요소. */
	struct semaphore semaphore;         /* 이 세마포어. */
};

/* 조건 변수 COND를 초기화합니다. 조건 변수는 한 코드 조각이 조건을 신호하고,
   협력하는 코드가 신호를 받고 이에 따라 행동할 수 있게 합니다. */
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* LOCK을 원자적으로 해제하고 다른 코드 조각이 COND에 신호를 보낼 때까지 기다립니다.
   COND가 신호를 받은 후, 반환하기 전에 LOCK을 다시 획득합니다.
   이 함수를 호출하기 전에 LOCK이 소유되어야 합니다.

   이 함수가 구현하는 모니터는 "Mesa" 스타일이며 "Hoare" 스타일이 아닙니다.
   즉, 신호를 보내고 받는 것이 원자적 연산이 아닙니다. 따라서 일반적으로
   대기 완료 후 조건을 다시 확인하고, 필요하면 다시 대기해야 합니다.

   주어진 조건 변수는 단일 락과만 연관됩니다. 하지만 하나의 락은 여러 조건 변수와
   연관될 수 있습니다. 즉, 락에서 조건 변수로 일대다 매핑이 가능합니다.

   이 함수는 잠들 수 있으므로 인터럽트 처리기 내에서 호출하면 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출될 수 있지만,
   필요하면 잠들기 때문에 인터럽트가 다시 활성화될 것입니다. */
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	//list_push_back (&cond->waiters, &waiter.elem);
	// PDG
	list_insert_ordered (&cond->waiters, &waiter.elem, compare_priority, NULL);	
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/* 만약 COND(LOCK으로 보호됨)에 대기 중인 스레드가 있다면,
   이 함수는 그들 중 하나에게 신호를 보내 대기에서 깨웁니다.
   이 함수를 호출하기 전에 LOCK이 소유되어야 합니다.

   인터럽트 처리기는 락을 획득할 수 없으므로,
   조건 변수 내에서 신호를 보내려고 시도해서는 안 됩니다. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters))
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);
}

/* COND(LOCK으로 보호됨)에서 대기 중인 모든 스레드를 깨웁니다.
   이 함수를 호출하기 전에 LOCK이 소유되어야 합니다.

   인터럽트 처리기는 락을 획득할 수 없으므로,
   조건 변수 내에서 신호를 보내려고 시도해서는 안 됩니다. */
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
