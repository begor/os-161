# Locks

Locks are implemented in `synch.h`/`sync.c`. 
Basically, Lock is a Sleep Lock. When you try acquire a Lock, and it's already locked by someone else,
instead of spinning on CPU while waiting (like in Spin Lock), you'll go to sleep on a wait channel.

## Implementation

Locks are implemented via volatile boolean field, which denotes lock's state: locked or not locked.
Critical sections (that is, sections of code that need to be protected in presence of multiple concurrent threads) are protected via Spin Lock. Threads that can not acquire Lock at the moment been put into wait channel and sleep there before Lock become available for a one of them.