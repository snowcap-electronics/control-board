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

#ifndef MINIMU9AHRS
#define MINIMU9AHRS

#include <math.h>
#include <stdint.h>

// Uncomment the following line to use a MinIMU-9 v5 or AltIMU-10 v5. Leave commented for older IMUs (up through v4).
//#define IMU_V5

extern int SENSOR_SIGN[9];

// tested with Arduino Uno with ATmega328 and Arduino Duemilanove with ATMega168

//#include <Wire.h>

// accelerometer: 8 g sensitivity
// 3.9 mg/digit; 1 g = 256
#if 0
#define GRAVITY 256  //this equivalent to 1G in the raw data coming from the accelerometer
#else
// Already in Gs
#define GRAVITY 1
#endif

#define ToRad(x) ((x)*0.01745329252)  // *pi/180
#define ToDeg(x) ((x)*57.2957795131)  // *180/pi

#if 0
// gyro: 2000 dps full scale
// 70 mdps/digit; 1 dps = 0.07
#define Gyro_Gain_X 0.07 //X axis Gyro gain
#define Gyro_Gain_Y 0.07 //Y axis Gyro gain
#define Gyro_Gain_Z 0.07 //Z axis Gyro gain
#define Gyro_Scaled_X(x) ((x)*ToRad(Gyro_Gain_X)) //Return the scaled ADC raw data of the gyro in radians for second
#define Gyro_Scaled_Y(x) ((x)*ToRad(Gyro_Gain_Y)) //Return the scaled ADC raw data of the gyro in radians for second
#define Gyro_Scaled_Z(x) ((x)*ToRad(Gyro_Gain_Z)) //Return the scaled ADC raw data of the gyro in radians for second
#else
// Already in rads/sec
#define Gyro_Scaled_X(x) (x)
#define Gyro_Scaled_Y(x) (x)
#define Gyro_Scaled_Z(x) (x)
#endif

// LSM303/LIS3MDL magnetometer calibration constants; use the Calibrate example from
// the Pololu LSM303 or LIS3MDL library to find the right values for your board

#if 0
#define M_X_MIN -1000
#define M_Y_MIN -1000
#define M_Z_MIN -1000
#define M_X_MAX +1000
#define M_Y_MAX +1000
#define M_Z_MAX +1000
#else
#define M_X_MIN -0.760
#define M_Y_MIN -0.282
#define M_Z_MIN -0.698
#define M_X_MAX +0.255
#define M_Y_MAX +0.669
#define M_Z_MAX +0.235
#endif

#define Kp_ROLLPITCH 0.02
#define Ki_ROLLPITCH 0.00002
#define Kp_YAW 1.2
#define Ki_YAW 0.00002

/*For debugging purposes*/
//OUTPUTMODE=1 will print the corrected data,
//OUTPUTMODE=0 will print uncorrected data of the gyros (with drift)
#define OUTPUTMODE 1

#define PRINT_DCM 0     //Will print the whole direction cosine matrix
#define PRINT_ANALOGS 0 //Will print the analog raw data
#define PRINT_EULER 1   //Will print the Euler angles Roll, Pitch and Yaw

#define STATUS_LED 13


void sc_pololu_loop(float deltat, float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
void sc_pololu_get_euler(float *p, float *r, float *y);
void Compass_Heading(void);
void Euler_angles(void);
void Matrix_update(float deltat);
void Drift_correction(void);
void Normalize(void);
float Vector_Dot_Product(float vector1[3],float vector2[3]);
void Vector_Cross_Product(float vectorOut[3], float v1[3],float v2[3]);
void Vector_Scale(float vectorOut[3],float vectorIn[3], float scale2);
void Vector_Add(float vectorOut[3],float vectorIn1[3], float vectorIn2[3]);
void Matrix_Multiply(float a[3][3], float b[3][3], float mat[3][3]);


extern long timer;   //general purpuse timer
extern long timer_old;
extern long timer24; //Second timer used to print values
extern int AN[6]; //array that stores the gyro and accelerometer data
extern int AN_OFFSET[6];

extern float gyro_x;
extern float gyro_y;
extern float gyro_z;
extern float accel_x;
extern float accel_y;
extern float accel_z;
extern float magnetom_x;
extern float magnetom_y;
extern float magnetom_z;
extern float c_magnetom_x;
extern float c_magnetom_y;
extern float c_magnetom_z;
extern float MAG_Heading;

extern float Accel_Vector[3];
extern float Gyro_Vector[3];
extern float Omega_Vector[3];
extern float Omega_P[3];
extern float Omega_I[3];
extern float Omega[3];

// Euler angles
extern float roll;
extern float pitch;
extern float yaw;

extern float errorRollPitch[3];
extern float errorYaw[3];

extern unsigned int counter;
extern uint8_t gyro_sat;

extern float DCM_Matrix[3][3];
extern float Update_Matrix[3][3];
extern float Temporary_Matrix[3][3];

#endif
