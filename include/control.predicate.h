#ifndef __control_predicate_H__
#define __control_predicate_H__

#include "core.types.h"

typedef bool (*predicate_f)( pointer );

// Some standard predicates
bool fallacyp( pointer );
bool tautologyp( pointer );
bool nullp( pointer );

#endif
