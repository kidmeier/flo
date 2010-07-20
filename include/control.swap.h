#ifndef __control_swap_h__
#define __control_swap_h__

#define swap( type, a, b )	  \
	do { \
		type tmp; \
		tmp = a; \
		a = b; \
		b = tmp; \
	} while( 0 )

#endif
