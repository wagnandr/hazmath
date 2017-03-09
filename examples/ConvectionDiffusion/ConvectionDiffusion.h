#ifdef WITH_SUITESPARSE
#undef WITH_SUITESPARSE
#endif

#ifndef M_PI
#define M_PI 3.141592653589793e+00;
#endif

// PDE Coefficients
void diffusion_coeff(REAL *val,REAL* x, REAL t) {
  // a(x)
  *val = 1.0;
  return;
}

// Exact Solution (if you have one)
// Change as needed for different dimensions
void exactsol(REAL *val,REAL* x, REAL t) {
  // 1D
  //*val = sin(M_PI*x[0])*exp(-M_PI*M_PI*time);
  // 2D
  *val = sin(M_PI*x[0])*sin(M_PI*x[1]);
  // 3D
  ///  *val=x[2]*x[2];
  //  *val=10.*x[0]*(1.-x[0]) *x[1]*(1.-x[1])*x[2];
  // *val= x[2]+x[0];
  return;
}

void advection(REAL* x, REAL *advcoeff, REAL t) {
  advcoeff[0] = 1e2; // or beta_1(x)
  advcoeff[1] = 0.; // or beta_2(x)
  advcoeff[2] = -1.; // or beta_3(x)
  return;
}

REAL bernoulli(const REAL z)
{
  // returns B(z) = z/(exp(z)-1)
  double tolb=1e-12,zlarge=256e+0;  
  if (fabs(z) < tolb)
    return (1.-z*0.5); // around 0 this is the asymptotic;
  else if(z<zlarge)
    return (z/(exp(z)-1.));
  else //z>tlarge this is zero pretty much
    return 0.;
}
// Right-hand Side
void myrhs(REAL *val,REAL* x, REAL t) {
  *val = 0.0;
}

// Boundary Conditions
void bc(REAL *val, REAL* x, REAL t) {
  *val= 0.0;
  return;
}



