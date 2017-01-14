/*

MinIMU-9-Arduino-AHRS
Pololu MinIMU-9 + Arduino AHRS (Attitude and Heading Reference System)

Copyright (c) 2011-2016 Pololu Corporation.
http://www.pololu.com/

MinIMU-9-Arduino-AHRS is based on sf9domahrs by Doug Weibel and Jose Julio:
http://code.google.com/p/sf9domahrs/

sf9domahrs is based on ArduIMU v1.5 by Jordi Munoz and William Premerlani, Jose
Julio and Doug Weibel:
http://code.google.com/p/ardu-imu/

MinIMU-9-Arduino-AHRS is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your option)
any later version.

MinIMU-9-Arduino-AHRS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License along
with MinIMU-9-Arduino-AHRS. If not, see <http://www.gnu.org/licenses/>.

*/

#include "src/ahrs/MinIMU9AHRS.h"

// Uncomment the below line to use this axis definition:
   // X axis pointing forward
   // Y axis pointing to the right
   // and Z axis pointing down.
// Positive pitch : nose up
// Positive roll : right wing down
// Positive yaw : clockwise
int SENSOR_SIGN[9] = {1,1,1,-1,-1,-1,1,1,1}; //Correct directions x,y,z - gyro, accelerometer, magnetometer
// Uncomment the below line to use this axis definition:
   // X axis pointing forward
   // Y axis pointing to the left
   // and Z axis pointing up.
// Positive pitch : nose down
// Positive roll : right wing down
// Positive yaw : counterclockwise
//int SENSOR_SIGN[9] = {1,-1,-1,-1,1,1,1,-1,-1}; //Correct directions x,y,z - gyro, accelerometer, magnetometer

float G_Dt=0.02;    // Integration time (DCM algorithm)  We will run the integration loop at 50Hz if possible

long timer=0;   //general purpuse timer
long timer_old;
long timer24=0; //Second timer used to print values
int AN[6]; //array that stores the gyro and accelerometer data
int AN_OFFSET[6]={0,0,0,0,0,0}; //Array that stores the Offset of the sensors

int gyro_x;
int gyro_y;
int gyro_z;
int accel_x;
int accel_y;
int accel_z;
int magnetom_x;
int magnetom_y;
int magnetom_z;
float c_magnetom_x;
float c_magnetom_y;
float c_magnetom_z;
float MAG_Heading;

float Accel_Vector[3]= {0,0,0}; //Store the acceleration in a vector
float Gyro_Vector[3]= {0,0,0};//Store the gyros turn rate in a vector
float Omega_Vector[3]= {0,0,0}; //Corrected Gyro_Vector data
float Omega_P[3]= {0,0,0};//Omega Proportional correction
float Omega_I[3]= {0,0,0};//Omega Integrator
float Omega[3]= {0,0,0};

// Euler angles
float roll;
float pitch;
float yaw;

float errorRollPitch[3]= {0,0,0};
float errorYaw[3]= {0,0,0};

unsigned int counter=0;
uint8_t gyro_sat=0;

float DCM_Matrix[3][3]= {
  {
    1,0,0  }
  ,{
    0,1,0  }
  ,{
    0,0,1  }
};
float Update_Matrix[3][3]={{0,1,2},{3,4,5},{6,7,8}}; //Gyros here


float Temporary_Matrix[3][3]={
  {
    0,0,0  }
  ,{
    0,0,0  }
  ,{
    0,0,0  }
};

void sc_pololu_loop(float deltat, float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
{
  G_Dt = deltat;

  // *** DCM algorithm
  // Data adquisition
  gyro_x = gx;
  gyro_y = gy;
  gyro_z = gz;

  accel_x = ax;
  accel_y = ay;
  accel_z = az;

  magnetom_x = mx;
  magnetom_y = my;
  magnetom_z = mz;

  // FIXME: magnetometer units and scaling?
  Compass_Heading(); // Calculate magnetic heading

  // Calculations...
  // FIXME: accelerometer units and scaling?
  Matrix_update(deltat);
  Normalize();
  Drift_correction();
  Euler_angles();
  // ***

  //printdata();
}

void sc_pololu_get_euler(float *p, float *r, float *y)
{
  *p = pitch;
  *r = roll;
  *y = yaw;
}
