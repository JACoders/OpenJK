// Prints the network field descriptors of codemp/qcommon/msg.cpp with the offsets, sizes and C types the compiler computes.
// fields.inc is written by ../python/gen_fields.py; rebuild this program after regenerating it.
// Output: one tab-separated record per line, read by gen_fields.py --exe.
#include "qcommon/q_shared.h"
#include <cstddef>
#include <cstdio>
#include <type_traits>

// C type of a field: "bool" (qboolean), "int", "float", "enum" (other enums, such as trType_t), "?" (anything else)
template <class T> static const char *TypeName() {
	return std::is_same<T, qboolean>::value ? "bool"
		: std::is_same<T, int>::value ? "int"
		: std::is_same<T, float>::value ? "float"
		: std::is_enum<T>::value ? "enum" : "?";
}
// parameters must not be named "type": the preprocessor would also replace the ::type of the traits
#define TYPE_OF(st, mem) \
	TypeName<std::remove_cv<std::remove_reference<decltype(((st *)0)->mem)>::type>::type>()

#define S(type) std::printf("S\t" #type "\t%u\n", (unsigned)sizeof(type));
#define C(name) std::printf("C\t" #name "\t%d\n", (int)(name));
#define A(name, count) std::printf("A\t" #name "\t%d\t%u\t%u\n", (int)(count), \
	(unsigned)offsetof(playerState_t, name), (unsigned)sizeof(((playerState_t *)0)->name[0]));
#define F(tab, type, member, bits) std::printf("F\t" #tab "\t" #member "\t%d\t%u\t%u\t%s\n", (int)(bits), \
	(unsigned)offsetof(type, member), (unsigned)sizeof(((type *)0)->member), TYPE_OF(type, member));

int main() {
#include "fields.inc"
	return 0;
}
