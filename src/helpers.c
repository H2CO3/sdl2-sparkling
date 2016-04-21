//
// helpers.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 12/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "helpers.h"

// Constrain a floating-point value to the [0...1] closed interval.
double constrain_to_01(double x)
{
	return x < 1 ? x < 0 ? 0 : x : 1;
}

//
// Hashmap handling
//
void set_integer_property(SpnHashMap *hm, const char *name, long n)
{
	SpnValue val = spn_makeint(n);
	spn_hashmap_set_strkey(hm, name, &val);
}

void set_float_property(SpnHashMap *hm, const char *name, double x)
{
	SpnValue val = spn_makefloat(x);
	spn_hashmap_set_strkey(hm, name, &val);
}

void set_string_property(SpnHashMap *hm, const char *name, const char *str)
{
	SpnValue val = spn_makestring(str);
	spn_hashmap_set_strkey(hm, name, &val);
	spn_value_release(&val);
}

void set_string_property_nocopy(SpnHashMap *hm, const char *name, const char *str)
{
	SpnValue val = spn_makestring_nocopy(str);
	spn_hashmap_set_strkey(hm, name, &val);
	spn_value_release(&val);
}
