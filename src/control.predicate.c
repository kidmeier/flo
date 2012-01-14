#include "control.predicate.h"

bool fallacyp( pointer arg ) {
	return false;
}

bool tautologyp( pointer arg ) {
	return true;
}

bool nullp( pointer arg ) {
	return NULL == arg;
}
