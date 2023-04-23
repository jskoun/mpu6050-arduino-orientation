//I2C HARDWARE COMMUNICATION WITH GY-521 (MPU6050)
#include "Wire.h" //minimum amount of libraries used, arduino built in I2C comms
const int MPU_ADDR = 0x68; //0b1101000
//I2C bus sensor address, returned from I2C scan
//also attainable through the MPU6000 datasheet p.36
//[http://cdn.sparkfun.com/datasheets/Components/General%20IC/PS-MPU-6000A.pdf]
//Register Map
//[http://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf]


float ACC_SCALE = 2048.0; //for 16g sensitivity
float GYR_SCALE = 65.5; //for 500 deg sensitivity
float g = 9.80665; //for m/s^2 conversion

//OFFSETS - [DEVICE SPECIFIC] CALCULATED BY SETTING TO 0 AND READING THE ERROR FROM EXPECTED VALUES//
int ACC_X_OFFSET = 900;
int ACC_Y_OFFSET = 2700;
int ACC_Z_OFFSET = 590;

int GYR_X_OFFSET = 88;
int GYR_Y_OFFSET = 260;
int GYR_Z_OFFSET = 190;

float ANGLE_DEAD_ZONE = 0.1;

float accX, accY, accZ, gyrX, gyrY, gyrZ;
float roll, pitch, yaw;
float t2, t1, dt;

void setup()
{
  Serial.begin(115200); //Begin Serial communication to acquire data in PC
  //while(!Serial){;} //wait for comms to establish
  Wire.begin();

  Wire.beginTransmission(MPU_ADDR); //Begin transmission to the sensor address
  Wire.write(0x6B); //0x6B is the Register 107 – Power Management 
  Wire.write(0); //setting to 0, wakes up the sensor
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR); 
  Wire.write(0x1A);             //register 0x1A: CONFIG
  Wire.write(0b00000100);       //[Digital Low Pass Filter 21Hz]
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR); 
  Wire.write(0x1B);             //register 0x1B: GYRO_CONFIG
  Wire.write(0b00001000);       //[Full Scale Gyro: 250deg/s]
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR); 
  Wire.write(0x1C);             //register 0x1C: ACCEL_CONFIG
  Wire.write(0b00011000);       //[Full Scale Accelerometer: +/-16g]
  Wire.endTransmission(true);
}

void loop()
{
  Wire.beginTransmission(MPU_ADDR);
  /*
  //Code to read config data

  Wire.write(0x1A);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 3, true); //Request 0x1A, 0x1B, 0x1C
  if (Wire.available()) 
  {
    byte DLPF_CFG = Wire.read() & 0b00000111; //masking last 3 bits
    byte FS_SEL = (Wire.read() >> 3) & 0b00000011; //FS_SEL in bits 3&4: bitshift+mask
    byte AFS_SEL = (Wire.read() >> 3) & 0b00000011; //

    Serial.print("DLPF configuration value: ");
    Serial.println(DLPF_CFG, DEC);

    Serial.print("FS_CEL configuration value: ");
    Serial.println(FS_SEL, DEC);

    Serial.print("AFS_CEL configuration value: ");
    Serial.println(AFS_SEL, DEC);
  }*/
  Wire.write(0x3B); 
  //0x3B is the ACCEL_XOUT_H register
  //combined with 0x3C (ACCEL_XOUT_L) we get the 16bit x acceleration value
  //Registers 0x3B to 0x40 are accelerometer x,y,z measurements
  //LINEAR ACCELERATION PARALLEL TO AXIS
  //Registers 0x41 to 0x42 are temperature measurements
  //Registers 0x43 to 0x48 are gyroscope ANGULAR VELOCITY measurements
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 7*2, true); //Request 14 bytes: 7 16bit measurements
  //Auto-increment takes care of moving the pointer to the next register
  int16_t ACCEL_X = (Wire.read() << 8 | Wire.read()); //Acceleration sensitivity in LSB/g
  int16_t ACCEL_Y = (Wire.read() << 8 | Wire.read());
  int16_t ACCEL_Z = (Wire.read() << 8 | Wire.read());

  int16_t tmp = (Wire.read() << 8 | Wire.read())/340+36.53; //Temperature sensitivity

  int16_t GYRO_X = (Wire.read() << 8 | Wire.read()); //Gyroscope sensitivity in LSB/deg/s
  int16_t GYRO_Y = (Wire.read() << 8 | Wire.read());
  int16_t GYRO_Z = (Wire.read() << 8 | Wire.read());

  getFloatAcceleration(ACCEL_X, ACCEL_Y, ACCEL_Z, accX, accY, accZ);
  getFloatGyroscope(GYRO_X, GYRO_Y, GYRO_Z, gyrX, gyrY, gyrZ);

  t1 = t2;
  t2 = millis();
  dt = (t2-t1)/1000; //integration time
  //Trigonometry to calculate pitch and roll by the gravity vector acting on the X,Y,Z axes
  float acc_angle_X = (atan(accY / sqrt(pow(accX, 2) + pow(accZ, 2))) * 180 / PI); 
  float acc_angle_Y = (atan(-1*accX / sqrt(pow(accY, 2) + pow(accZ, 2))) * 180 / PI);
  
  //Integration for orientation, correcting for drift by implementing a dead zone to ignore noise
  if (abs(gyrX)>ANGLE_DEAD_ZONE) pitch = pitch + dt*(gyrX);
  if (abs(gyrY)>ANGLE_DEAD_ZONE) roll = roll + dt*(gyrY);
  if (abs(gyrZ)>ANGLE_DEAD_ZONE) yaw = yaw + dt*(gyrZ);
	
  //wannabe sensor fusion
  pitch = 0.9*pitch+0.1*acc_angle_X;
  roll = 0.9*roll+0.1*acc_angle_Y;

  Serial.print(pitch);
  Serial.print(",");
  Serial.print(roll);
  Serial.print(",");
  Serial.print(yaw);
  Serial.print(",");

  Serial.print(acc_angle_X);
  Serial.print(",");
  Serial.print(acc_angle_Y);

  Serial.println("");

  delay(10);
}

void getFloatAcceleration(int16_t intX, int16_t intY, int16_t intZ, float& x, float& y, float& z) 
{
  //input: 3 axis int16_t MPU6050 acceleration measurements
  //output: 3 axis float acceleration measurements scaled to m/s^2
  x = (intX+ACC_X_OFFSET)/ACC_SCALE;
  y = (intY+ACC_Y_OFFSET)/ACC_SCALE;
  z = (intZ+ACC_Z_OFFSET)/ACC_SCALE;
}

void getFloatGyroscope(int16_t intX, int16_t intY, int16_t intZ, float& x, float& y, float& z) 
{
  //input: 3 axis int16_t MPU6050 gyroscope measurements
  //output: 3 axis float gyroscope measurements scaled to deg/s
  x = (intX+GYR_X_OFFSET)/GYR_SCALE;
  y = (intY+GYR_Y_OFFSET)/GYR_SCALE;
  z = (intZ+GYR_Z_OFFSET)/GYR_SCALE;
}


