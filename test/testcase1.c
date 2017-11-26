struct A {
    struct B {
        int x = 1;
    } x;
} x;

struct D {
    struct C {
        int jj;
    } j;
} jj;

int func(struct A a) {
    jj = x;
    func(jj);
}

