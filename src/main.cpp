#include <Arduino.h>
#include <Servo.h>
#include <IRremote.h>

// pins definitions
#define PIR_PIN 5
#define MONITOR 2
#define ACCESS 11
#define DENIED 10
#define AUTH 8
#define TOUCH 4

// configw
IRrecv irrecv(7);
decode_results results;
Servo servo;
uint32_t codes[] = {
    16738455, 16724175, 16718055,
    16743045, 16716015, 16726215, 16734885,
    16728765, 16730805, 16732845};

// values states
const int startPos = 170, endPos = 68;
int turnMode = 0, ledMode = 0, systemMode = 0;
int servoPos = 0, increment = 1;
int password[] = {1, 2, 3, 4};
int userPass[4] = {-1};
int passTrack = 0;
int systemState = LOW;

// led blink
unsigned long prevMillis = 0;
long interval = 1000;
int ledState = LOW;
int touchState = LOW;
int pirState = LOW;

// conditinal states
unsigned long doorOpen = 0;
bool opening = false, opened = false, closing = false;
bool auth = false;

// method declarations
void processCode(uint32_t);
void BlinkInterval(unsigned long, int);
void trackMotion();
void trackMotionLed();
void trackMotionState();
void trackLed();
int processPassword(uint32_t);
bool checkAuth();
void resetPassword();

void setup()
{
  pinMode(MONITOR, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ACCESS, OUTPUT);
  pinMode(DENIED, OUTPUT);
  pinMode(AUTH, OUTPUT);
  pinMode(TOUCH, INPUT);

  irrecv.enableIRIn();
  servo.write(startPos);
  servo.attach(9);
  Serial.begin(9600);
}

void loop()
{
  servoPos = servo.read();
  trackLed();

  if (irrecv.decode(&results))
  {
    if (results.value == 16769055)
    {
      systemState = LOW;
      digitalWrite(AUTH, LOW);
      digitalWrite(ACCESS, LOW);
      digitalWrite(DENIED, LOW);
      auth = false;
      ledMode = 0;
      resetPassword();
    }
    else if (results.value == 16754775)
    {
      systemState = HIGH;
      ledMode = 1;
      digitalWrite(AUTH, HIGH);
      digitalWrite(ACCESS, LOW);
      digitalWrite(DENIED, LOW);
    }

    if (systemState == HIGH)
    {
      processCode(results.value);
    }
    irrecv.resume();
  }

  if (systemState == HIGH)
  {
    if (systemMode == 1)
    {
      digitalWrite(AUTH, HIGH);
      digitalWrite(ACCESS, LOW);
      digitalWrite(DENIED, LOW);
    }
    else
    {
      if (auth)
      {
        pirState = (systemMode == 0) ? digitalRead(PIR_PIN) : LOW;
        touchState = (systemMode == 3) ? digitalRead(TOUCH) : LOW;
        trackMotionState();
        trackMotion();
        trackMotionLed();
      }
      else
      {
        digitalWrite(ACCESS, LOW);
        digitalWrite(DENIED, HIGH);
        digitalWrite(AUTH, LOW);
      }
    }
  }

  delay(20);
}

void trackMotionLed()
{

  switch (turnMode)
  {
  case 0:
    if (opened)
    {
      digitalWrite(ACCESS, HIGH);
      digitalWrite(DENIED, LOW);
      digitalWrite(AUTH, LOW);
    }
    else
    {
      if (systemMode == 3)
      {
        digitalWrite(AUTH, HIGH);
        digitalWrite(DENIED, HIGH);
        digitalWrite(ACCESS, LOW);
      }
      else
      {
        digitalWrite(AUTH, LOW);
        digitalWrite(ACCESS, LOW);
        digitalWrite(DENIED, HIGH);
      }
    }
    break;
  case 1:
    digitalWrite(ACCESS, LOW);
    digitalWrite(AUTH, LOW);
    digitalWrite(DENIED, HIGH);
    break;
  case 2:
    digitalWrite(ACCESS, HIGH);
    digitalWrite(AUTH, LOW);
    digitalWrite(DENIED, HIGH);
    break;
  default:
    break;
  }
}

void trackMotion()
{
  switch (turnMode)
  {
  case 1:
    opened = false;
    if (servoPos + increment <= startPos)
    {
      servo.write(servoPos + increment);
    }
    else
    {
      turnMode = 0;
    }
    break;
  case 2:
    if (servoPos - increment >= endPos)
    {
      servo.write(servoPos - increment);
    }
    else
    {
      turnMode = 0;
      opened = true;
    }
    break;
  default:
    break;
  }
}

void trackMotionState()
{
  if (pirState == HIGH || touchState == HIGH)
  {
    digitalWrite(MONITOR, HIGH);
    turnMode = 2;
    doorOpen = millis();
    opening = true;
  }
  else
  {
    if (millis() - doorOpen >= 5000 && opening)
    {
      digitalWrite(MONITOR, LOW);
      turnMode = 1;
      opening = false;
    }
  }
}

void trackLed()
{
  switch (ledMode)
  {
  case 0:
    digitalWrite(MONITOR, LOW);
    break;
  case 1:
    digitalWrite(MONITOR, HIGH);
    break;
  case 2:
    BlinkInterval(interval, MONITOR);
    break;
  default:
    break;
  }
}

int processPassword(uint32_t code)
{
  for (int i = 0; i < 10; i++)
  {
    if (code == codes[i])
    {
      return i;
    }
  }
  return -1;
}

bool checkAuth()
{
  for (int i = 0; i < 4; i++)
  {
    if (userPass[i] != password[i])
    {
      return false;
    }
  }
  return true;
}

void resetPassword()
{
  for (int i = 0; i < 4; i++)
  {
    userPass[i] = -1;
  }
}

void processCode(uint32_t code)
{
  if (systemMode == 1)
  {
    if (code == 16756815)
    {
      passTrack = 0;
      systemMode = 0;
      Serial.println("Mode 0");
      digitalWrite(AUTH, LOW);
      return;
    }

    digitalWrite(MONITOR, LOW);
    delay(300);
    userPass[passTrack++] = processPassword(code);
    Serial.print("key : ");
    Serial.print(userPass[passTrack - 1]);
    Serial.println("");
    if (checkAuth())
    {
      passTrack = 0;
      auth = true;
      ledMode = 0;
      systemMode = 2;
      digitalWrite(AUTH, LOW);
      return;
    }
    if (passTrack >= 4)
    {
      passTrack = 0;
      resetPassword();
      for (int i = 0; i < 3; i++)
      {
        digitalWrite(MONITOR, LOW);
        delay(100);
        digitalWrite(MONITOR, HIGH);
        delay(100);
      }
    }
  }
  else if (systemMode == 2)
  {
    switch (code)
    {
    case 16761405:
      if (auth)
      {
        ledMode = 0;
        systemMode = 3;
      }
    case 16753245:
      if (auth)
      {
        turnMode = 1;
        Serial.println("Turning Left");
      }
      break;
    case 16769565:
      if (auth)
      {
        turnMode = 2;
        Serial.println("Turning Right");
      }
      break;
    case 16748655:
      turnMode = 0;
      Serial.println("Stable now");
      break;
    case 16736925:
      Serial.println("Checking state: ");
      Serial.println(servo.read());
      break;
    case 16750695:
      if (!auth)
      {
        systemMode = 1;
        Serial.println("Mode 1");
      }
      break;
    case 16720605:
      if (auth)
      {
        ledMode = 2;
        systemMode = 0;
      }
      break;
    case 16756815:
      ledMode = 1;
      auth = false;
      resetPassword();
      Serial.println("Unauthenticate");
      break;
    default:
      break;
    }
  }
  else if (systemMode == 3)
  {
    switch (code)
    {
    case 16736925:
      Serial.println("Checking state: ");
      Serial.println(servo.read());
      break;
    case 16750695:
      if (!auth)
      {
        systemMode = 1;
        Serial.println("Mode 1");
      }
      break;
    case 16720605:
      if (auth)
      {
        ledMode = 2;
        systemMode = 0;
      }
      break;
    case 16712445:
      if (auth)
      {
        ledMode = 0;
        systemMode = 2;
      }
      break;
    case 16756815:
      ledMode = 1;
      auth = false;
      resetPassword();
      Serial.println("Unauthenticate");
      break;
    default:
      Serial.println("I dont understand you");
      break;
    }
  }
  else
  {
    switch (code)
    {
    case 16761405:
      if (auth)
      {
        ledMode = 0;
        systemMode = 3;
      }
      break;
    case 16736925:
      Serial.println("Checking state: ");
      Serial.println(servo.read());
      break;
    case 16750695:
      if (!auth)
      {
        systemMode = 1;
        Serial.println("Mode 1");
      }
      break;
    case 16712445:
      if (auth)
      {
        ledMode = 0;
        systemMode = 2;
      }
      break;
    case 16756815:
      ledMode = 1;
      auth = false;
      resetPassword();
      Serial.println("Unauthenticate");
      break;
    default:
      Serial.println("I dont understand you");
      break;
    }
  }
}

void BlinkInterval(unsigned long blinkTime, int pinNo)
{
  if (pinNo > 0 && pinNo < 14)
  {
    if (millis() - prevMillis >= blinkTime)
    {
      digitalWrite(pinNo, ledState);
      ledState = !ledState;
      prevMillis = millis();
    }
  }
}
