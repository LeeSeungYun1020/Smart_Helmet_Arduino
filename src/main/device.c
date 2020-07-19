#include <Arduino.h>
#include <SoftwareSerial.h>
#include <pitches.h>
#include <command.h>
#include <Servo.h>
Servo myServo;
SoftwareSerial HM10(10, 9);	  // 호환보드에서 블루투스 모듈 사용 위해 가상 Serial 구성
const int alcoholSensor = A7; // 알코올 센서
const int deviceButton = 7;	  // 전원 버튼
const int motorPin = 8;		  // 서보 모터 핀 / 안전모 보관함 잠금
const int echoPin = 13;		  // 초음파 에코
const int trigPin = 12;		  // 초음파 트리거

const int alcoholStandard = 450; // 알코올 측정 기준
const long int interval = 8000;	 // 8초 대기
const int buttonCenter = 31;	 // 비상 버튼
const int buttonLeft = 33;		 // 좌회전 버튼
const int buttonRight = 35;		 // 우회전 버튼
const int buzzer = 2;			 // 경고 및 알림 부저

boolean checkAlcohol();
void manageCommand(int command);
void alcoholError();
void checkLEDButton();
boolean checkBluetoothConnection();
void openBox();
void closeBox();

boolean buttonLast = false;		 // 버튼 마지막 상태
boolean buttonCloseLast = false; // 버튼(잠금) 마지막 상태
boolean isOpen = false;			 // 잠금 상태

unsigned long wearAlert = 0; // 미착용 경고

void setup()
{
	Serial.begin(9600);
	pinMode(deviceButton, INPUT);
	pinMode(alcoholSensor, INPUT);
	pinMode(buzzer, OUTPUT);
	pinMode(buttonCenter, INPUT);
	pinMode(buttonLeft, INPUT);
	pinMode(buttonRight, INPUT);
	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);
	HM10.begin(9600);
	closeBox();
}

void loop()
{
	boolean buttonCurrent = digitalRead(deviceButton);
	if (buttonCurrent == true && buttonLast == false)
	{
		Serial.println("시작");
		buttonLast = true;
		wearAlert = millis();
		if (checkAlcohol())
		{
			if (checkBluetoothConnection())
			{
				openBox();
				while (true)
				{
					boolean buttonClose = digitalRead(deviceButton);
					if (buttonClose && buttonCloseLast == false)
					{
						Serial.println("잠금 시도");
						buttonCloseLast = true;
						digitalWrite(trigPin, HIGH);
						delay(10);
						digitalWrite(trigPin, LOW);
						float duration = pulseIn(echoPin, HIGH);
						float distance = duration * 340 / 10000 / 2;
						if (distance < 15)
						{
							Serial.println("잠금 확인 완료");
							HM10.write(QUIT);
							closeBox();
							break;
						}
					}
					else if (!buttonClose)
					{
						buttonCloseLast = false;
					}

					if (HM10.available())
					{
						int command = HM10.read();
						//Serial.println(String("<확인용> 명령: ") + command);
						if (command != -1)
							manageCommand(command);
					}
					checkLEDButton();
				}
			}
			else
			{
				Serial.println("<경고> 블루투스 연결 실패");
				tone(buzzer, 450, interval);
			}
			buttonLast = false;
			Serial.println("사용 종료 / 반납 처리됨.");
			delay(500);
		}
		else
		{
			Serial.println("알코올 기준 미충족으로 대기 시간(" + String(interval) + ") 이후 재시도 허가");
			delay(interval);
			buttonLast = false;
		}
	}
}

boolean checkAlcohol()
{
	Serial.println("알코올 검사 시작");
	unsigned long alcoholInitTime = millis();
	boolean pass = false;
	while (true)
	{
		int alcoholValue = analogRead(alcoholSensor);
		if (alcoholInitTime + interval < millis())
		{
			if (pass)
				return true;
			else
				return false;
		}
		else
		{
			if (alcoholValue > alcoholStandard)
			{
				alcoholError();
				return false;
			}
			else
				pass = true;
		}
	}
	return true;
}

void alcoholError()
{
	Serial.println(String("<경고> 알코올 값 초과 (") + analogRead(alcoholSensor) + ")");
	tone(buzzer, 450, interval);
} // 부저

void manageCommand(int command)
{
	if (command == CONNECT)
	{
		Serial.println("<확인> 블루투스 연결이 정상적으로 작동합니다.");
	}
	else if (command == WEARED)
	{
		noTone(buzzer);
		//Serial.println("weared");
		wearAlert = millis();
	}
	else if (command == UNWEARED)
	{
		//Serial.println("unweared");
		noTone(buzzer);
		if (wearAlert + interval + 3000 < millis())
		{
			tone(buzzer, 400, 100);
		}
	}
	else
	{
		Serial.println(String("<경고> 존재하지 않는 명령(코드: ") + command + ")이 호출되었습니다.");
	}
}

void checkLEDButton()
{
	if (digitalRead(buttonCenter))
	{
		HM10.write(CENTER);
	}
	if (digitalRead(buttonLeft))
	{
		HM10.write(LEFT);
	}
	if (digitalRead(buttonRight))
	{
		HM10.write(RIGHT);
	}
}

boolean checkBluetoothConnection()
{
	HM10.write(PASS);
	unsigned long start = millis();
	while (true)
	{
		if (HM10.available())
		{
			if (HM10.read() == CONNECT)
				return true;
		}
		if (millis() - start > 10000)
			break;
	}
	return false;
}

void openBox()
{
	Serial.println("open");
	myServo.attach(motorPin);
	myServo.write(0);
	delay(500);
	myServo.detach();
	isOpen = true;
}

void closeBox()
{
	Serial.println("close");
	myServo.attach(motorPin);
	myServo.write(90);
	delay(500);
	myServo.detach();
	isOpen = false;
}