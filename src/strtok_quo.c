#include "strtok_quo.h"

char * strtok_quo (char *s, char **save_ptr) {
	char *end;
	if (s == NULL) {
		s = *save_ptr;
	}
	if (*s == '\0') {
		*save_ptr = s;
		return NULL;
	}
	// Scan leading delimiters.
	s += strspn (s, " \t\n\v\f\r");
	if (*s == '\0') {
		*save_ptr = s;
		return NULL;
	}
	// Find the end of the token.
	end = strcspn_quo (&s) + s;
	if (*end == '\0') {
		*save_ptr = end;
		return s;
	}
	// Terminate the token and make *SAVE_PTR point past it.
	*end = '\0';
	*save_ptr = end + 1;
	return s;
}

size_t strcspn_quo (char **str) {
	bool quo1 = false;
	bool quo2 = false;
	char *s = *str;
	if (*s == '\'') {
		quo1 = true;
		s++;
	} else if (*s == '"') {
		quo2 = true;
		s++;
	}
	bool esc = false;
	while (*s != '\0') {
		if (!esc) {
			if (*s == '\\') {
				esc = true;
			} else if (!quo1 && !quo2) {
				if (isspace(*s)) {
					return s - (*str);
				}
			} else if ((quo1 && *s == '\'') || (quo2 && *s == '"')) {
				*s = '\0'; // nullify last quote
				(*str) ++; // skip first quote
				return s - (*str) + 1;
			}
		} else {
			esc = false;
		}
		s++;
	}
	return s - (*str);
}
