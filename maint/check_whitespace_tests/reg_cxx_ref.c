/* expect: A6 A9 */
/* C++ reference parameters (test/mpi/include/mpitestcxx.h): a '&' before ',' or ')' */
int f(MPI::Comm &, int);
void g(MPI::Win &);
int h(MPI::Comm & , int);
int k(MPI::Comm & c, int);
int m(MPI::Comm &c, int);
int n(MPI::Comm&, int &);
