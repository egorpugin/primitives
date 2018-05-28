#pragma once

// Concatenate preprocessor tokens A and B without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define CONCATENATE_NX(s1, s2) s1 ## s2

// Concatenate preprocessor tokens A and B after macro-expanding them.
#define CONCATENATE(s1, s2) CONCATENATE_NX(s1, s2)
