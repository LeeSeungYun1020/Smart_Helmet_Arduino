#include <Arduino.h>
#include <command.h>

void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
}

void loop()
{
    //   if (Serial.available()){
    //     Serial1.write(Serial.read());
    //   }
    if (Serial1.available())
    {
        int receive = Serial1.read();
        Serial.print("Println: ");
        Serial.println(receive);
        Serial.print("Write: ");
        Serial.write(receive);
        Serial.println();
        Serial1.write(RIGHT);
    }
}
