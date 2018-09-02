# Locks

Locks are implemented in `synch.h`/`sync.c`. 

Basically, Lock is a Sleep Lock. When you try acquire a Lock, and it's already locked by someone else,
instead of spinning on CPU while waiting (like in Spin Lock), you'll go to sleep on a wait channel.

## Implementation

Locks are implemented via volatile boolean field, which denotes lock's state: locked or not locked.
Critical sections (that is, sections of code that need to be protected in presence of multiple concurrent threads) are protected via Spin Lock. Threads that can not acquire Lock at the moment been put into wait channel and sleep there before Lock become available for a one of them.


# Condition Variables

CVs are implemented in `synch.h`/`sync.c`.

CVs are used to sleep until some condition becomes true. 


## Implementation

CVs are implemented using Wait Channel to put awaiting for condition thread to sleep, and locks for ensuring atomic operations. Also, there is internal Spin Lock to protect critical sections of implementation.


# RW Locks

RWs are implemented in `synch.h`/`sync.c`.

RW lock is a lock that threads can acquire in one of two ways: read mode or write mode. Read mode does not conflict with read mode, but read mode conflicts with write mode and write mode conflicts with write mode. The result is that many threads can acquire the lock in read mode, or one thread can acquire the lock in write mode. 

One important issue of RW locks is the one of starvation: when there are a lot of readers, and one writer waits infinitely to acquire the lock. 

## Implementation

They're implemented using three locks: 
1) `rwlock_lock` is used in a classical sense, to protect critical sections of RW's code.
2) `rwlock_writer` is used for writers to acquire the lock. Since there is only one writer possible in a momemnt of time, simple lock is sufficient for that.
3) `rwlock_read_available` this is used to prevent starvation. If we have our RW lock acquired in read mode, and then come the writer to acquire it in write mode, we shouldn't allow any more reader to acquire it (even though it's not violates the API).