/*! \file src/utilities/coefficient.c   
 *  
 *  Created by James Adler and Xiaozhe Hu on 2/22/16.
 *  Copyright 2015__HAZMAT__. All rights reserved.
 *
 */

/* This code create generic functions for coefficients that are commonly used
 * such as zero scalars and vectors and coefficients of 1. */

#include "hazmat.h"

/****************************************************************************************/
void constant_coeff_scal(REAL *val,REAL* x,REAL constval) 
{
  // Assigns constant value (uses "time" slot in assembly routines).
    *val = constval;
}
/****************************************************************************************/

/****************************************************************************************/
void constant_coeff_vec2D(REAL *val,REAL* x,REAL constval) 
{
  // Assigns constant value (uses "time" slot in assembly routines).
    val[0] = constval;
    val[1] = constval;
}
/****************************************************************************************/

/****************************************************************************************/
void constant_coeff_vec3D(REAL *val,REAL* x,REAL constval) 
{
  // Assigns constant value (uses "time" slot in assembly routines).
    val[0] = constval;
    val[1] = constval;
    val[2] = constval;
}
/****************************************************************************************/

/****************************************************************************************/
void zero_coeff_scal(REAL *val,REAL* x,REAL time) 
{
  *val = 0.0;
}
/****************************************************************************************/

/****************************************************************************************/
void zero_coeff_vec2D(REAL *val,REAL* x,REAL time) 
{
  val[0] = 0.0;
  val[1] = 0.0;
}
/****************************************************************************************/

/****************************************************************************************/
void zero_coeff_vec3D(REAL *val,REAL* x,REAL time) 
{
  val[0] = 0.0;
  val[1] = 0.0;
  val[2] = 0.0;
}
/****************************************************************************************/

/****************************************************************************************/
void one_coeff_scal(REAL *val,REAL* x,REAL time) 
{
  *val = 1.0;
}
/****************************************************************************************/

/****************************************************************************************/
void one_coeff_vec2D(REAL *val,REAL* x,REAL time) 
{
  val[0] = 1.0;
  val[1] = 1.0;
}
/****************************************************************************************/

/****************************************************************************************/
void one_coeff_vec3D(REAL *val,REAL* x,REAL time) 
{
  val[0] = 1.0;
  val[1] = 1.0;
  val[2] = 1.0;
}
/****************************************************************************************/
