#include <stddef.h>

struct { char *name, *nmemonic; } base_data_names[] = {
	{ "foreign_nil_p",    "nil?" },
	{ "foreign_value_p",  "value?" },
	{ "foreign_sym_p",    "sym?" },
	{ "foreign_cons_p",   "cons?" },
	{ "foreign_func_p",   "func?" },
	{ "foreign_car",      "car" },
	{ "foreign_cdr",      "cdr" },
	{ "foreign_makesym",  "makesym" },
	{ "foreign_splitsym", "splitsym" },
	{ "foreign_cons",     "cons" },
	{ "foreign_symeq_p",  "symeq?" },
	{ "foreign_eq_p",     "=?" },
	{ "foreign_lt_p",     "<?" },
	{ "foreign_gt_p",     ">?" },
	{ "foreign_ult_p",    "<u?" },
	{ "foreign_ugt_p",    ">u?" },
	{ "foreign_addc_p",   "addc?" },
	{ "foreign_add",      "+" },
	{ "foreign_sub",      "-" },
	{ "foreign_mul",      "*" },
	{ "foreign_div",      "/" },
	{ "foreign_rem",      "%" },
	{ "foreign_bit_and",  "&" },
	{ "foreign_bit_or",   "|" },
	{ "foreign_bit_nand", "~&" },
	{ "foreign_bit_xor",  "^" },
	{ "foreign_bit_shl",  "<<" },
	{ "foreign_bit_shr",  ">>" },
	{ "foreign_bit_shra", ">>+" },
};

size_t base_data_count = sizeof(base_data_names) / sizeof(base_data_names[0]);

