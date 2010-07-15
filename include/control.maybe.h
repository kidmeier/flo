#ifndef __control_maybe_h__
#define __control_maybe_h__

#define maybe( op, fail, expr )	  \
	((op) fail) ? (op) : (expr)

#endif
