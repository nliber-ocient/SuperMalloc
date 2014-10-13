#include <assert.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <limits.h>
#include <unistd.h>
#include <immintrin.h>
#include <errno.h>
#include <thread>
#include <time.h>

#include "futex_mutex.h"

// The lock field is the mutex as described in "futexes are tricky"
// The wait field is 1 if someone is waiting for the lock to zero (with the intention of running a transaction)

static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
  return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

static long futex_wait(volatile int *addr, int val) {
  return sys_futex((void*)addr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
}
static long futex_wake1(volatile int *addr) {
  return sys_futex((void*)addr, FUTEX_WAKE_PRIVATE, 1,   NULL, NULL, 0);
}
static long futex_wakeN(volatile int *addr) {
  return sys_futex((void*)addr, FUTEX_WAKE_PRIVATE, INT_MAX,   NULL, NULL, 0);
}

static const int lock_spin_count = 20;
static const int unlock_spin_count = 20;

// Return 0 if it's a fast acquiistion, 1 if slow
extern "C" int futex_mutex_lock(futex_mutex_t *m) {
  {
    int count = 0;
    while (count < lock_spin_count) {
      int old_c = m->lock;
      if (old_c != 0) {
	// someone else has the lock, so spin.
	_mm_pause();
	count++;
      } else if (__sync_bool_compare_and_swap(&m->lock, 0, 1)) {
	// got it
	return 0;
      } else {
	continue; // Don't pause, we were in a tight race on the compare-and-swap
      }
    }
  }
  // We got here without getting the lock, so we must do the futex thing.
  int c = __sync_val_compare_and_swap(&m->lock, 0, 1);
  if (c == 0)  return 0;
  if (c == 1) {
    c = __atomic_exchange_n(&m->lock, 2, __ATOMIC_SEQ_CST);
  }
  while (c != 0) {
    futex_wait(&m->lock, 2);
    c = __atomic_exchange_n(&m->lock, 2, __ATOMIC_SEQ_CST);
  }
  return 1;
}
 
extern "C" void futex_mutex_unlock(futex_mutex_t *m) {
  if (__sync_fetch_and_add(&m->lock, -1) != 1) {
    m->lock = 0;
    futex_wake1(&m->lock);
  }
  if (m->wait) {
    m->wait = 0;
    futex_wakeN(&m->wait);
  }
}
extern "C" int futex_mutex_subscribe(futex_mutex_t *m) {
  return m->lock;
}

extern "C" int futex_mutex_wait(futex_mutex_t *m) {
  int did_futex = 0;
  while (1) {
    for (int i = 0; i < lock_spin_count; i++) {
      if (m->lock == 0) return did_futex;
      _mm_pause();
    }
    // Now we have to do the relatively heavyweight thing.
    m->wait = 1;
    futex_wait(&m->wait, 1);
    did_futex = 1;
  }
}
  
#ifdef TESTING
futex_mutex_t m;
static void foo() {
  futex_mutex_lock(&m);
  printf("foo sleep\n");
  sleep(2);
  printf("foo slept\n");
  futex_mutex_unlock(&m);
}

static void simple_test() {
  std::thread a(foo);
  std::thread b(foo);
  std::thread c(foo);
  a.join();
  b.join();
  c.join();
}

static bool time_less(const struct timespec &a, const struct timespec &b) {
  if (a.tv_sec < b.tv_sec) return true;
  if (a.tv_sec > b.tv_sec) return false;
  return a.tv_nsec < b.tv_nsec;
}

volatile int exclusive_is_locked=0;
volatile uint64_t exclusive_count=0;

static void stress() {
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  start.tv_sec ++;
  uint64_t locked_fast=0, locked_slow=0, sub_locked=0, sub_unlocked=0, wait_long=0, wait_short=0, wait_was_one=0, wait_was_zero=0;
  while (1) {
    clock_gettime(CLOCK_MONOTONIC, &end);
    if (time_less(start, end)) break;
    for (uint64_t i = 0; i < 100; i++) {
      switch (i%3) {
	case 0: {
	  int lock_kind = futex_mutex_lock(&m);
	  if (0) {
	    assert(!exclusive_is_locked);
	    exclusive_is_locked=1;
	    exclusive_count++;
	    assert(exclusive_is_locked);	  
	    exclusive_is_locked=0;
	  }
	  futex_mutex_unlock(&m);
	  if (lock_kind==0) locked_fast++;
	  else              locked_slow++;
	  break;
	}
	case 1:
	  if (futex_mutex_subscribe(&m)) {
	    sub_locked++;
	  } else {
	    sub_unlocked++;
	  }
	  break;
	case 2:
	  if  (futex_mutex_wait(&m)) {
	    wait_long++;
	  } else {
	    wait_short++;
	  }
	  if (futex_mutex_subscribe(&m)) {
	    wait_was_one++;
	  } else {
	    wait_was_zero++;
	  }
	  break;
      }
    }
  }
  printf("locked_fast=%8ld locked_slow=%8ld sub_locked=%8ld sub_unlocked=%8ld wait_long=%8ld wait_short=%8ld was1=%8ld was0=%ld\n", locked_fast, locked_slow, sub_locked, sub_unlocked, wait_long, wait_short, wait_was_one, wait_was_zero);
}

static void stress_test() {
  const int n = 8;
  std::thread x[n];
  for (int i = 0; i < n; i++) { 
    x[i] = std::thread(stress);
  }
  for (int i = 0; i < n; i++) {
    x[i].join();
  }
}
  

extern "C" void test_futex() {
  stress_test();
  simple_test();
}
#endif

