// Linked only into the numerical game test when a candidate pow is supplied.
extern double DStarsTestPow(double x, double y);
double __wrap_Sf64Pow(double x, double y) {
    return DStarsTestPow(x, y);
}
