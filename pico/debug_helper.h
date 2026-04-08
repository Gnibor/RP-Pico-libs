#ifndef _DEBUG_HELPER_H_
#define _DEBUG_HELPER_H_

#ifdef DEBUG
	#if defined(__arm__) || defined(__thumb__)
		#define BREAK() __asm volatile ("bkpt #0")
	#elif defined(__riscv)
		#define BREAK() __asm volatile ("ebreak")
	#else
		#define BREAK() do { while(1); } while(0)
	#endif
#else
	#define BREAK() ((void)0)
#endif

#ifdef DEBUG
	#ifdef LOG_E
		#define ASSERT(x) do { \
			if(!(x)) { \
				LOG_E("assert failed: %s", #x); \
				while(1); \
			} \
		} while(0)
	#else
		#define ASSERT(x) do { \
			if(!(x)) { \
				while(1); \
			} \
		} while(0)
	#endif
#else
	#define ASSERT(x) ((void)0)
#endif

#endif // _DEBUG_HELPER_H_
