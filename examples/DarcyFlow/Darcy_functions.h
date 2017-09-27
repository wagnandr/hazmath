/*! \file DarcyData.h
 *
 *  Created by Adler, Hu, Zikatanov on 8/30/16.
 *  Copyright 2016_HAZMATH__. All rights reserved.
 *
 * \brief This contains all the Data parameters and coefficients
 *        for the Darcy example.  This includes exact solutions,
 *        RHS functions, coefficients, and boundary conditions.
 *
 */

// Coefficients
// K
void porosity(REAL *val,REAL* x,REAL time,void *param) {
  // K = 3x3 matrix (assuming diagonal for now)
  // ordered column-wise
  val[0] = 1.0;
  val[1] = 0.0;
  val[2] = 0.0;
  val[3] = 0.0;
  val[4] = 1.0;
  val[5] = 0.0;
  val[6] = 0.0;
  val[7] = 0.0;
  val[8] = 1.0;
}

// W
void source(REAL *val,REAL* x,REAL time,void *param) {
  *val = 0.0;
}

// Ss : Negative because goes into the 2,2 block after time discretization
void Ss(REAL *val,REAL* x,REAL time,void *param) {
  *val = -1.0;
}

// g : Dirichlet conditions for h. 
void myg(REAL *val,REAL* x,REAL time,void *param) {
  *val = -20.0;
}

// Boundary Conditions
void bc_q(REAL *val,REAL* x,REAL time,void *param) {
  // Known flux on top and bottom
  INT bdrycode = *((INT *) param);
  if(bdrycode==22) { // Rainfall
    val[0] = 0.0; 
    val[1] = 0.0;
    val[2] = -1.0;
  } else { // No flux in the ground
    val[0] = 0.0;
    val[1] = 0.0;
    val[2] = 0.0;
  }
}
void bc_h(REAL *val,REAL* x,REAL time,void *param) {
  // it is not called
    *val = 0.0;
}
// combines the above. 
void bc(REAL *val,REAL* x, REAL time,void *param) {
  REAL mybc_q[3];
  REAL mybc_h;
  bc_q(mybc_q,x,time,param);
  bc_h(&mybc_h,x,time,param);
  // q
  val[0] = mybc_q[0];
  val[1] = mybc_q[1];
  val[2] = mybc_q[2];
  //h
  val[3] = mybc_h;
}

// Initial Conditions
void initial_q(REAL *val,REAL* x,REAL time,void *param) {
  INT bdrycode = *((INT *) param);
  // Known flux on top and bottom
  if(bdrycode==22) { // Rainfall
    val[0] = 0.0;
    val[1] = 0.0;
    val[2] = -1.0;
  } else { // No flux in the ground
    val[0] = 0.0;
    val[1] = 0.0;
    val[2] = 0.0;
  }
}
void initial_h(REAL *val,REAL* x,REAL time,void *param) {
    *val = 0.0;
}
void initial_conditions(REAL *val,REAL* x, REAL time,void *param) {
  REAL myinit_q[3];
  REAL myinit_h;
  initial_q(myinit_q,x,time,param);
  initial_h(&myinit_h,x,time,param);
  // q
  val[0] = myinit_q[0];
  val[1] = myinit_q[1];
  val[2] = myinit_q[2];
  //h
  val[3] = myinit_h;
}

void mgraph_wrap(dCSRmat A, dvector f, dvector *u);
