#if 0

#pragma once

class debugPrint : public Print {
    uint8_t _debugLevel = 1;
  public:
    void setDebugLevel(byte newLevel) {
      _debugLevel = newLevel;
    }
    size_t write (uint8_t value)
    {
      if (_debugLevel)
      {
        Serial.write(value);
      }
      return 1;
    }
} Debug;

/*
void setup() {
  // put your setup code here, to run once:

  Serial.begin(4800);
  Serial.println(F("Debug Print Test"));

  Debug.println("Debug 1 is on");

  Debug.setDebugLevel(0);
  Debug.println("Debug 2 is off");

  Debug.setDebugLevel(1);
  Debug.println("Debug 3 is on again");

  Debug.setDebugLevel(0);
  Debug.println(F("Debug 4 is off"));

  Debug.setDebugLevel(1);
  Debug.println(F("Debug 5 is on again"));

  float afloat = 123.45;
  Debug.println(afloat);

  char achararray [] = {"bla bla\0"};
  Debug.println(achararray);

  String anArduinoStringObject = "bla blub";
  Debug.println(anArduinoStringObject);
}

void loop() {
  // put your main code here, to run repeatedly:

}
 */
#endif
