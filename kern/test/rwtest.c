/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

#define NTHREADS        32
#define CREATELOOPS		8
#define TESTLOOPS       20

static bool test_status = TEST161_FAIL;
static struct semaphore *donesem = NULL;
static struct rwlock *testlock = NULL;

struct spinlock status_lock;
static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;
static volatile unsigned long testval4;

static
bool
failif(bool condition) {
	if (condition) {
		spinlock_acquire(&status_lock);
		test_status = TEST161_FAIL;
		spinlock_release(&status_lock);
	}
	return condition;
}

static
void
rwlocktestreadthread(void *junk, unsigned long num)
{
	(void)junk;
	(void)num;

	int i;

	kprintf_n("Read Thread: %21lu\n", num);

	for (i=0; i<TESTLOOPS; i++) {
		random_yielder(4);
		rwlock_acquire_read(testlock);

		testval4 += 1;

		KASSERT(testlock->readers > 0);
		
		random_yielder(4);
		rwlock_release_read(testlock);
	}

	V(donesem);
	return;
}

static
void
rwlocktestwritethread(void *junk, unsigned long num)
{
	(void)junk;
	(void)num;

	int i;

	kprintf_n("Write Thread: %21lu\n", num);

	for (i=0; i<TESTLOOPS; i++) {
		rwlock_acquire_write(testlock);
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		testval1 = num;
		testval2 = num*num;
		testval3 = num%3;

		if (testval2 != testval1*testval1) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		if (testval2%3 != (testval3*testval3)%3) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		if (testval3 != testval1%3) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		if (testval1 != num) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		if (testval2 != num*num) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		if (testval3 != num%3) {
			goto fail;
		}
		random_yielder(4);
		KASSERT(testlock->readers == 0);

		rwlock_release_write(testlock);
	}

	V(donesem);
	return;

fail:
	rwlock_release_write(testlock);
	failif(true);
	V(donesem);
	return;
}

// Tests core reader-writer lock functionality by reading and writing shared state. 
int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;

	int i, result;

	kprintf_n("Starting rwt1...\n");
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");
		testlock = rwlock_create("testlock");
		if (testlock == NULL) {
			panic("rwt1: lock_create failed\n");
		}
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwt1: sem_create failed\n");
		}
		if (i != CREATELOOPS - 1) {
			rwlock_destroy(testlock);
			sem_destroy(donesem);
		}
	}
	spinlock_init(&status_lock);
	test_status = TEST161_SUCCESS;

	testval1 = 0;
	testval2 = 0;
	testval3 = 0;
	testval4 = 0;

	for (i=1; i<NTHREADS+1; i++) {
		kprintf_t(".");
		result = thread_fork("synchtest", NULL, rwlocktestreadthread, NULL, i);
		if (result) {
			panic("rwt1: read thread_fork failed: %s\n", strerror(result));
		}
		result = thread_fork("synchtest", NULL, rwlocktestwritethread, NULL, i);
		if (result) {
			panic("rwt1: write thread_fork failed: %s\n", strerror(result));
		}
	}

	for (i=0; i<NTHREADS * 2; i++) {
		kprintf_t(".");
		P(donesem);
	}

	KASSERT(testlock->readers == 0);
	KASSERT(testval1 > 0);
	KASSERT(testval4 == NTHREADS * TESTLOOPS);

	rwlock_destroy(testlock);
	sem_destroy(donesem);
	testlock = NULL;
	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt1");

	return 0;
}

// Tests that reader-writer locks allow maximum read concurrency when no writers are waiting.    
int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	int i, result;

	kprintf_n("Starting rwt2...\n");
	testlock = rwlock_create("testlock");
	if (testlock == NULL) {
		panic("rwt2: lock_create failed\n");
	}
	donesem = sem_create("donesem", 0);
	if (donesem == NULL) {
		panic("rwt2: sem_create failed\n");
	}
	spinlock_init(&status_lock);
	test_status = TEST161_SUCCESS;
	testval4 = 0;

	// Acquiring lock a couple of times
	rwlock_acquire_read(testlock);
	rwlock_acquire_read(testlock);
	rwlock_acquire_read(testlock);

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		result = thread_fork("rwtest", NULL, rwlocktestreadthread, NULL, i);
		if (result) {
			panic("rwt2: thread_fork failed: %s\n", strerror(result));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		kprintf_t(".");
		P(donesem);
	}


	// RWLock can still be acquired for reading
	KASSERT(testval4 == NTHREADS * TESTLOOPS);
	// ...even though it's already been acquired 3 times
	KASSERT(testlock->readers == 3);

	rwlock_release_read(testlock);
	rwlock_release_read(testlock);
	rwlock_release_read(testlock);

	KASSERT(testlock->readers == 0);


	rwlock_destroy(testlock);
	sem_destroy(donesem);
	testlock = NULL;
	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt2");

	return 0;
}

// Tests reader-writer lock error handling. Panics on success.
int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("Starting rwt3...\n");
	testlock = rwlock_create("testlock");
	if (testlock == NULL) {
		panic("rwt3: lock_create failed\n");
	}
	
	secprintf(SECRET, "Should panic...", "rwt3");
	rwlock_release_read(testlock);

	success(TEST161_FAIL, SECRET, "rwt3");

	/* Don't do anything that could panic. */

	testlock = NULL;

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("Starting rwt4...\n");
	testlock = rwlock_create("testlock");
	if (testlock == NULL) {
		panic("rwt4: lock_create failed\n");
	}

	rwlock_acquire_read(testlock);
	rwlock_release_read(testlock);
	
	secprintf(SECRET, "Should panic...", "rwt4");
	rwlock_release_read(testlock);

	success(TEST161_FAIL, SECRET, "rwt4");

	/* Don't do anything that could panic. */

	testlock = NULL;

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("Starting rwt5...\n");
	testlock = rwlock_create("testlock");
	if (testlock == NULL) {
		panic("rwt5: lock_create failed\n");
	}
	
	secprintf(SECRET, "Should panic...", "rwt5");
	rwlock_release_write(testlock);

	success(TEST161_FAIL, SECRET, "rwt5");

	/* Don't do anything that could panic. */

	testlock = NULL;

	return 0;
}
