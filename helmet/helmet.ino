#include <Wire.h>
#include <Arduino.h>
#include <command.h>
#define MPU 0x68 // mpu6050 칩 주소.

const int lightSensor = A0;      // 광센서
const int lightSensorDigital = 2; // 광센서 디지털
const int lightStandard = 700;    // 광센서 기준
const int pressureSensor = A1;    // 압력 센서
const int pressureStandard = 650; // 압력 기준
const int vibrationSensor = A2;   // 진동 센서
const int vibrationStandard = 800; // 진동 기준 테스트 필요
const int vibrationSensor2 = 9; // 진동(충격) 센서 digital(T/F)
const int relayCenter = 3;      // 중간등
const int relayLeft = 4;      // 왼쪽등
const int relayRight = 5;     // 오른쪽등
const int relayNightLight = 6;    // 야간등
boolean isPassed = false;     // 이동수단-안전모간 블루투스 연결 확인용

long acX, acY, acZ, gyX, gyY, gyZ; //acc, gyro data (acc, gyro 계산 수식)
double angle = 0, deg;         // angle, deg data (각도계산)
double dgyX;             // double type acc data
long int normalX, normalY, normalZ;
long int delthaX[3], delthaY[3], delthaZ[3], deltha; // 정규화 데이터 값, 가속도 변화량 표시, deltha는 가속도 전체 변화량(x, y, z 합계)
long int angleVal;

const int mapping_value = 10000;

const int delayMain = 2;   // 전체 시스템 구동하는 데, 걸리는 시간 (즉 구동 시간)이다. (ex) 1000 = 1000ms = 1s = 1초
const long int sumCount = 2; // 평균 내는 횟수

unsigned long centerDetector = 0;   // 중간등 켜는 시간 설정용
unsigned long leftDetector = 0;     // 왼쪽등 켜는 시간 설정용
unsigned long rightDetector = 0;    // 오른쪽등 켜는 시간 설정용
unsigned long nightLightDetector = 0; // 야간등 켜는 시간 설정용

void setLED();
void manageCommand(int command);
void checkWeared();
bool isWeared();
void checkLight();
void checkVibration();

void mpu6050_init();  // 가속도 센서 초기 제어
void accel_calculate(); // 가속도 센서 측정하기
void value_init();    // 측정 변수 초기화 하기
void checkAccel();

// 좌우 조명.
void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  mpu6050Init(); // 가속도 센서 초기 설정
  pinMode(lightSensor, INPUT);
  pinMode(lightSensorDigital, INPUT);
  pinMode(pressureSensor, INPUT);
  pinMode(vibrationSensor, INPUT);
  pinMode(vibrationSensor2, INPUT);
  // 릴레이
  pinMode(relayCenter, OUTPUT);   // center
  pinMode(relayLeft, OUTPUT);     // left
  pinMode(relayRight, OUTPUT);    // right
  pinMode(relayNightLight, OUTPUT); // 광센서 (blue)

  digitalWrite(relayLeft, HIGH);
  digitalWrite(relayRight, HIGH);
  digitalWrite(relayCenter, HIGH);
  digitalWrite(relayNightLight, HIGH);
}

void loop()
{
  if (isPassed == false && Serial1.available())
  {
    int command = Serial1.read();
    if (command == PASS)
    {
      isPassed = true;
      Serial1.write(CONNECT);
    }
  }
  if (isPassed)
  {
    checkLight();
    checkViberation();
    checkWeared();
    //checkAccel();
    if (Serial1.available())
    {
      int command = Serial1.read();
      Serial.println(String("명령: ") + command);
      if (command == QUIT)
      {
        isPassed = false;
        digitalWrite(relayLeft, HIGH);
        digitalWrite(relayNightLight, HIGH);
        digitalWrite(relayRight, HIGH);
        digitalWrite(relayCenter, HIGH);
      }
      manageCommand(command);
    }
    setLED();
  }
}

void manageCommand(int command)
{
  unsigned long minimum = 250;
  unsigned long current = millis();
  if (command == CENTER && centerDetector < current)
    centerDetector = current + minimum;
  if (command == LEFT && leftDetector < current)
    leftDetector = current + minimum;
  if (command == RIGHT && rightDetector < current)
    rightDetector = current + minimum;
}

void setLED()
{
  unsigned long current = millis();
  unsigned long blinkDuration = 250;
  if (centerDetector + 500 > current)
  {
    digitalWrite(relayCenter, LOW);
  }
  else
  {
    digitalWrite(relayCenter, HIGH);
  }
  if (leftDetector > current)
    if ((current / blinkDuration) % 2 == 1)
      digitalWrite(relayLeft, HIGH);
    else
      digitalWrite(relayLeft, LOW);
  else
    digitalWrite(relayLeft, HIGH);

  if (rightDetector > current)
    if ((current / blinkDuration) % 2 == 1)
      digitalWrite(relayRight, HIGH);
    else
      digitalWrite(relayRight, LOW);
  else
    digitalWrite(relayRight, HIGH);

  if (nightLightDetector > current)
    digitalWrite(relayNightLight, LOW);
  else
    digitalWrite(relayNightLight, HIGH);
}

void checkWeared()
{
  if (isWeared())
    Serial1.write(WEARED);
  else
    Serial1.write(UNWEARED);
}

bool isWeared()
{
  if (analogRead(pressureSensor) > pressureStandard)
    return true;
  else
    return false;
}

void checkLight()
{
  int lightValue = analogRead(lightSensor);
  if (lightValue > lightStandard)
    nightLightDetector = millis() + 500;
}

void checkViberation()
{
  int viberation = analogRead(vibrationSensor);
  bool viberation2 = digitalRead(vibrationSensor2);
  unsigned long current = millis();
  
  if (viberation > vibrationStandard || !viberation2)
  {
    leftDetector = current + 4000;
    rightDetector = current + 4000;
  }
}
// 가속도 센서(mpu6050) 초기설정 함수
void mpu6050Init()
{
  Wire.begin();        //I2C통신 시작
  Wire.beginTransmission(MPU); // 0x68(주소) 찾아가기
  Wire.write(0x6B);      // 초기 설정 제어 값
  Wire.write(0);
  Wire.endTransmission(true);
}
// 가속도 연산 함수
void accelCalculate()
{
  acX = 0;
  acY = 0;
  acZ = 0;
  normalX = 0;
  normalY = 0;
  normalZ = 0;

  Wire.beginTransmission(MPU);  // 번지수 찾기
  Wire.write(0x3B);       // 가속도 데이터 보내달라고 컨트롤 신호 보내기
  Wire.endTransmission(false);  // 기달리고,
  Wire.requestFrom(MPU, 6, true); // 데이터를 받아 처리

  // Data SHIFT
  acX = Wire.read() << 8 | Wire.read();
  acY = Wire.read() << 8 | Wire.read();
  acZ = Wire.read() << 8 | Wire.read();

  //맵핑화 시킨 것 - 즉 10000으로 맵핑시킴
  normalX = map(int(acX), -16384, 16384, 1000, 3000);
  normalY = map(int(acY), -16384, 16384, 1000, 3000);
  normalZ = map(int(acZ), -16384, 16384, 1000, 3000);

  //각도계산 deg -> 각도
  deg = atan2(acX, acZ) * 180 / PI; //rad to deg
  dgyX = gyY / 131.;          //16-bit data to 250 deg/sec
  angle = (0.95 * (angle + (dgyX * 0.001))) + (0.05 * deg);
}

// 연산 변수 초기화 함수
void valueInit()
{ // 연산 변수들을 초기화 시켜줌
  normalX = 0;
  normalY = 0;
  normalZ = 0;
  for (int i = 0; i < 3; i++)
  {
    delthaX[i] = 0;
    delthaY[i] = 0;
    delthaZ[i] = 0;
    angle = 0;
    angleVal = 0;
  }
}

void checkAccel()
{
  valueInit(); //가속도-각도 관련 초기값 선언

  //첫번째 감지
  for (int i = 0; i < sumCount; i++)
  {
    accelCalculate();
    delthaX[1] = delthaX[1] + normalX;
    delthaY[1] = delthaY[1] + normalY;
    delthaZ[1] = delthaZ[1] + normalZ;
    angleVal = angleVal + angle;
  }
  delthaX[1] = int(delthaX[1] / sumCount);
  delthaY[1] = int(delthaY[1] / sumCount);
  delthaZ[1] = int(delthaZ[1] / sumCount);

  //두번째 감지
  for (int i = 0; i < sumCount; i++)
  {
    accelCalculate();
    delthaX[2] = delthaX[2] + normalX;
    delthaY[2] = delthaY[2] + normalY;
    delthaZ[2] = delthaZ[2] + normalZ;
    angleVal = angleVal + angle;
  }
  delthaX[2] = int(delthaX[2] / sumCount);
  delthaY[2] = int(delthaY[2] / sumCount);
  delthaZ[2] = int(delthaZ[2] / sumCount);
  /*
   //3축 변화량 비교 - 가속도 변화량, 각도 평균 값
   delthaX[0] = abs(delthaX[1] - delthaX[2]); // 각각 변화량.
   delthaY[0] = abs(delthaY[1] - delthaY[2]);
   delthaZ[0] = abs(delthaZ[1] - delthaZ[2]);
   deltha = delthaX[0] + delthaY[0] + delthaZ[0];
   angleValue = abs(int(angleValue / (sumCount)));
*/

  // x축 변화량 test
  boolean stopping = true; // 멈추는 중.
  // delthaX[1]이 이전, delthaX[2] 가 이후
  if (delthaX[1] > delthaX[2] + 50)
    stopping = true;
  else
    stopping = false;

  Serial.print(String("x : ") + delthaX[1] + "\t" + delthaX[2]);
  Serial.print(String("y : ") + delthaY[1] + "\t" + delthaY[2]);

  Serial.print(" ");
  if (stopping)
  {
    Serial.println("stopping");
    centerDetector = millis() + 2000;
  }
  else
    Serial.println("");
}
