#include <SoftwareSerial.h>
#include <Arduino.h>
#include <command.h>

SoftwareSerial hm10(10, 9);
bool send = false;
unsigned long previous = 0;

void setup()
{
	Serial.begin(9600);
	hm10.begin(9600);
	previous = millis();
}

void loop()
{
	// if (Serial.available()){
	//   hm10.write(Serial.read());
	// }
	if (!send)
	{
		send = true;
		hm10.write(CONNECT);
	}

	if (hm10.available())
	{
		int receive = hm10.read();
		Serial.print("Println: ");
		Serial.println(receive);
		Serial.print("Write: ");
		Serial.write(receive);
		Serial.println();
	}

	if (previous + 2000 > millis())
	{
		send = false;
		previous = millis();
	}
}