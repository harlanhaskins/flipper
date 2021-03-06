#include <flipper.h>

LF_MODULE(_user, "user", "User module description", NULL, NULL);

const struct _user user {
    user_test
};

LF_WEAK int user_test(int a, char b, long int c) {
    int result = lf_invoke(&_user, _user_test, lf_uint32_t, lf_args(lf_infer(a), lf_infer(b), lf_infer(c)));
}
