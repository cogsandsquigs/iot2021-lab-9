#include <Wire.h>
#include <SparkFun_VCNL4040_Arduino_Library.h>
#include <oled-wing-adafruit.h>
#include <blynk.h>

SYSTEM_THREAD(ENABLED);

VCNL4040 vcnl;
OledWingAdafruit display;

const uint16_t dialInput = A0;
const uint16_t buttonInput = D8;
const uint16_t tempInput = A1;

const uint16_t blue = D7;
const uint16_t green = D6;
const uint16_t orange = D5;
const uint16_t blynkLight = A2;

uint minSetPoint = 0;
uint maxSetPoint = 0;
uint ambientLight = 0;

int TransitionAcrossPoints = 0;        // checks if we have transitioned accross set points or not. -1 for blue, 0 for green, 1 for orange
int previousTransition = 0;            // previous hasTransitionedAcrossPoints
bool setMin = true;                    // checks if we are setting min point or not
bool setPointMode = true;              // checks if we are setting the points or recording temp to OLED
bool wasSetPointButtonPressed = false; // checks if button was pressed

void setup()
{
  Wire.begin(); //Join i2c bus

  vcnl.begin();             // start monitoring + lux
  vcnl.powerOffProximity(); //Power down the proximity portion of the sensor
  vcnl.powerOnAmbient();    // Power up the ambient sensor

  Blynk.begin("avQ6vbh1dxKVUU6klLXq7vbFteKEGoCf", IPAddress(167, 172, 234, 162), 9090); // connecting to blynk server

  display.setup(); // starts the display
  display.clearDisplay();
  display.display();

  pinMode(dialInput, INPUT);
  pinMode(buttonInput, INPUT);

  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(orange, OUTPUT);
  pinMode(blynkLight, OUTPUT);
}

void loop()
{
  // init Blynk for this loop
  Blynk.run();

  // this chunk inits display
  display.loop();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  uint64_t reading = analogRead(tempInput);  // this whole chunk gets temp from sensor
  double voltage = (reading * 3.3) / 4095.0; // gets sensor voltage
  double tempC = (voltage - 0.5) * 100;      // gets celcius temp from voltage
  double tempF = (tempC * 9.0 / 5.0) + 32.0; // gets fahrenheit temp from celcius
  Blynk.virtualWrite(V0, tempF);             // write to blynk gauge

  ambientLight = vcnl.getAmbient();

  if (setPointMode) // checks if it is in setting point mode
  {
    uint setPoint = map(analogRead(dialInput), 0, 4095, 0, 65536); // gets point from potentiometer

    wasSetPointButtonPressed = digitalRead(buttonInput); // checks if button was pressed

    if (wasSetPointButtonPressed && setMin) // if button was pressed
    {
      minSetPoint = setPoint;
      setMin = false;
      wasSetPointButtonPressed = false;
      delay(1000); // delay to wait for no button press
    }
    else if (wasSetPointButtonPressed && !setMin)
    {
      maxSetPoint = setPoint;
      setPointMode = false;
      wasSetPointButtonPressed = false;
      delay(1000); // delay to wait for no button press
    }

    display.printlnf("CURRENT: %d\nmin: %d\nmax: %d", setPoint, minSetPoint, maxSetPoint); // print min and max points
    display.display();
    if (!setPointMode)
    {
      delay(1000);
    }
  }
  else
  {
    // this chunk prints the temperature out in a ✨pretty✨ format
    display.println("TEMPERATURE");
    display.setCursor(0, 8);
    display.println("Celsius: " + String(tempC));
    display.setCursor(0, 16);
    display.println("Fahrenheit: " + String(tempF));
    display.display();
  }

  if (display.pressedA()) // if A button pressed, go back to temperature point setting mode
  {
    setPointMode = true;
    setMin = true;
  }

  previousTransition = TransitionAcrossPoints;

  if (ambientLight <= minSetPoint)
  {
    TransitionAcrossPoints = -1;
    digitalWrite(blue, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(orange, LOW);
  }
  else if (ambientLight > minSetPoint && ambientLight < maxSetPoint)
  {
    TransitionAcrossPoints = 0;
    digitalWrite(blue, LOW);
    digitalWrite(green, HIGH);
    digitalWrite(orange, LOW);
  }
  else
  {
    TransitionAcrossPoints = 1;
    digitalWrite(blue, LOW);
    digitalWrite(green, LOW);
    digitalWrite(orange, HIGH);
  }

  if (previousTransition != TransitionAcrossPoints) // checks if there is a disparity btwn prev. and curr. ambient light points
  {
    if (TransitionAcrossPoints == -1)
    {
      Blynk.notify("Light has transitioned to Blue region!");
    }
    else if (TransitionAcrossPoints == 0)
    {
      Blynk.notify("Light has transitioned to Green region!");
    }
    else if (TransitionAcrossPoints == 1)
    {
      Blynk.notify("Light has transitioned to Orange region!");
    }
  }
}
// button activates LED
BLYNK_WRITE(V1)
{
  int pinData = param.asInt();
  if (pinData == 1)
  {
    digitalWrite(blynkLight, HIGH);
  }
  else
  {
    digitalWrite(blynkLight, LOW);
  }
}