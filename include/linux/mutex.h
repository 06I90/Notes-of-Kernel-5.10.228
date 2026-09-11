/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mutexes: blocking mutual exclusion locks
 *
 * started by Ingo Molnar:
 *
 *  Copyright (C) 2004, 2005, 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * This file contains the main data structure and API definitions.
 */
#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <asm/current.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/lockdep.h>
#include <linux/atomic.h>
#include <asm/processor.h>
#include <linux/osq_lock.h>
#include <linux/debug_locks.h>

struct ww_acquire_ctx;

/*
 * Simple, straightforward mutexes with strict semantics:
 * 简单、直接的互斥锁，具有严格的语义：
 *
 * - only one task can hold the mutex at a time
 *   同一时刻只能有一个任务持有这个互斥锁
 * - only the owner can unlock the mutex
 *   只有锁的拥有者才能解锁
 * - multiple unlocks are not permitted
 *   不允许重复解锁
 * - recursive locking is not permitted
 *   不允许递归加锁（同一线程不能重复获取同一把锁）
 * - a mutex object must be initialized via the API
 *   必须通过互斥锁的API函数初始化
 * - a mutex object must not be initialized via memset or copying
 *   mutex 对象不能通过 memset 或内存拷贝来初始化
 * - task may not exit with mutex held
 *   任务在退出时不能仍然持有 mutex
 * - memory areas where held locks reside must not be freed
 *   存放已持有锁的内存区域不能被释放
 * - held mutexes must not be reinitialized
 *   被持有的互斥锁不能被再次初始化
 * - mutexes may not be used in hardware or software interrupt
 *   contexts such as tasklets and timers
 *   mutex 不能在硬件或软件中断上下文中使用，比如 tasklet 和定时器
 *
 * These semantics are fully enforced when DEBUG_MUTEXES is
 * enabled. Furthermore, besides enforcing the above rules, the mutex
 * debugging code also implements a number of additional features
 * that make lock debugging easier and faster:
 * 当启用 DEBUG_MUTEXES 时，这些语义会被完全强制执行。
 * 此外，mutex 调试代码还提供了一些附加功能，使得锁调试更容易、更高效：
 *
 * - uses symbolic names of mutexes, whenever they are printed in debug output
 *   在调试输出中使用 mutex 的符号名称
 * - point-of-acquire tracking, symbolic lookup of function names
 *   记录获取锁的位置，并能进行函数名的符号查找
 * - list of all locks held in the system, printout of them
 *   维护系统中所有锁的列表，并支持打印输出
 * - owner tracking
 *   跟踪锁的拥有者
 * - detects self-recursing locks and prints out all relevant info
 *   检测自递归加锁，并打印所有相关信息
 * - detects multi-task circular deadlocks and prints out all affected
 *   locks and tasks (and only those tasks)
 *   检测多任务间的循环死锁，并打印所有受影响的锁和任务（且只打印相关任务）
 */

struct mutex {
	/* 表示当前持有锁的线程 */
	atomic_long_t		owner;
	/* 自旋锁保护等待队列 wait_list */
	spinlock_t		wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
#endif
	struct list_head	wait_list;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};

struct ww_class;
struct ww_acquire_ctx;

struct ww_mutex {
	struct mutex base;
	struct ww_acquire_ctx *ctx;
#ifdef CONFIG_DEBUG_MUTEXES
	struct ww_class *ww_class;
#endif
};

/*
 * This is the control structure for tasks blocked on mutex,
 * which resides on the blocked task's kernel stack:
 */
struct mutex_waiter {
	struct list_head	list;
	struct task_struct	*task;
	struct ww_acquire_ctx	*ww_ctx;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
};

#ifdef CONFIG_DEBUG_MUTEXES

#define __DEBUG_MUTEX_INITIALIZER(lockname)				\
	, .magic = &lockname

extern void mutex_destroy(struct mutex *lock);

#else

# define __DEBUG_MUTEX_INITIALIZER(lockname)

static inline void mutex_destroy(struct mutex *lock) {}

#endif

/**
 * mutex_init - initialize the mutex
 * @mutex: the mutex to be initialized
 *
 * Initialize the mutex to unlocked state.
 *
 * It is not allowed to initialize an already locked mutex.
 */
#define mutex_init(mutex)						\
do {									\
	static struct lock_class_key __key;				\
									\
	__mutex_init((mutex), #mutex, &__key);				\
} while (0)

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define __DEP_MAP_MUTEX_INITIALIZER(lockname)			\
		, .dep_map = {					\
			.name = #lockname,			\
			.wait_type_inner = LD_WAIT_SLEEP,	\
		}
#else
# define __DEP_MAP_MUTEX_INITIALIZER(lockname)
#endif

#define __MUTEX_INITIALIZER(lockname) \
		{ .owner = ATOMIC_LONG_INIT(0) \
		, .wait_lock = __SPIN_LOCK_UNLOCKED(lockname.wait_lock) \
		, .wait_list = LIST_HEAD_INIT(lockname.wait_list) \
		__DEBUG_MUTEX_INITIALIZER(lockname) \
		__DEP_MAP_MUTEX_INITIALIZER(lockname) }

#define DEFINE_MUTEX(mutexname) \
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

extern void __mutex_init(struct mutex *lock, const char *name,
			 struct lock_class_key *key);

/**
 * mutex_is_locked - is the mutex locked
 * @lock: the mutex to be queried
 *
 * Returns true if the mutex is locked, false if unlocked.
 */
extern bool mutex_is_locked(struct mutex *lock);

/*
 * See kernel/locking/mutex.c for detailed documentation of these APIs.
 * Also see Documentation/locking/mutex-design.rst.
 */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
extern void mutex_lock_nested(struct mutex *lock, unsigned int subclass);
extern void _mutex_lock_nest_lock(struct mutex *lock, struct lockdep_map *nest_lock);

extern int __must_check mutex_lock_interruptible_nested(struct mutex *lock,
					unsigned int subclass);
extern int __must_check mutex_lock_killable_nested(struct mutex *lock,
					unsigned int subclass);
extern void mutex_lock_io_nested(struct mutex *lock, unsigned int subclass);

#define mutex_lock(lock) mutex_lock_nested(lock, 0)
#define mutex_lock_interruptible(lock) mutex_lock_interruptible_nested(lock, 0)
#define mutex_lock_killable(lock) mutex_lock_killable_nested(lock, 0)
#define mutex_lock_io(lock) mutex_lock_io_nested(lock, 0)

#define mutex_lock_nest_lock(lock, nest_lock)				\
do {									\
	typecheck(struct lockdep_map *, &(nest_lock)->dep_map);	\
	_mutex_lock_nest_lock(lock, &(nest_lock)->dep_map);		\
} while (0)

#else
extern void mutex_lock(struct mutex *lock);
extern int __must_check mutex_lock_interruptible(struct mutex *lock);
extern int __must_check mutex_lock_killable(struct mutex *lock);
extern void mutex_lock_io(struct mutex *lock);

# define mutex_lock_nested(lock, subclass) mutex_lock(lock)
# define mutex_lock_interruptible_nested(lock, subclass) mutex_lock_interruptible(lock)
# define mutex_lock_killable_nested(lock, subclass) mutex_lock_killable(lock)
# define mutex_lock_nest_lock(lock, nest_lock) mutex_lock(lock)
# define mutex_lock_io_nested(lock, subclass) mutex_lock_io(lock)
#endif

/*
 * NOTE: mutex_trylock() follows the spin_trylock() convention,
 *       not the down_trylock() convention!
 *
 * Returns 1 if the mutex has been acquired successfully, and 0 on contention.
 */
extern int mutex_trylock(struct mutex *lock);
extern void mutex_unlock(struct mutex *lock);

extern int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock);

/*
 * These values are chosen such that FAIL and SUCCESS match the
 * values of the regular mutex_trylock().
 */
enum mutex_trylock_recursive_enum {
	MUTEX_TRYLOCK_FAILED    = 0,
	MUTEX_TRYLOCK_SUCCESS   = 1,
	MUTEX_TRYLOCK_RECURSIVE,
};

/**
 * mutex_trylock_recursive - trylock variant that allows recursive locking
 * @lock: mutex to be locked
 *
 * This function should not be used, _ever_. It is purely for hysterical GEM
 * raisins, and once those are gone this will be removed.
 *
 * Returns:
 *  - MUTEX_TRYLOCK_FAILED    - trylock failed,
 *  - MUTEX_TRYLOCK_SUCCESS   - lock acquired,
 *  - MUTEX_TRYLOCK_RECURSIVE - we already owned the lock.
 */
extern /* __deprecated */ __must_check enum mutex_trylock_recursive_enum
mutex_trylock_recursive(struct mutex *lock);

#endif /* __LINUX_MUTEX_H */
