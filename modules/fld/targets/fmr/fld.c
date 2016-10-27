#define __private_include__
#include <flipper/flipper.h>
#include <flipper/modules.h>

/* ~ Provide the definition for this standard module. ~ */
LF_MODULE(_fld, "fld", "Loads modules on the target device.", _fld_id);

int fld_configure(void) {
	return lf_invoke(&_fld, _fld_configure, NULL);
}
