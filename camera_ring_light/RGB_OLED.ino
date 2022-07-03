// Potentiometer config
#define PONTPIN 0
// Button  Config
#define BUTTONPIN 2
#define switched true                 // value if the button switch has been pressed
#define triggered true                // controls interrupt handler
#define interrupt_trigger_type RISING // interrupt triggered on a RISING input
#define debounce 10                   // time to wait in milli secs

volatile bool interrupt_process_status = {
    !triggered // start with no switch press pending, ie false (!triggered)
};
bool initialisation_complete = false;
int buttonState = 0; // variable for reading the pushbutton status

#include <Adafruit_NeoPixel.h>
#define LEDPIN 6
#define NUMPIXELS 60

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 200 // Time (in milliseconds) to pause between pixels
// OLED Setup
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);

String colors[3] = {"RED", "BLUE", "GREEN", "BRIGHT"};
int colorValues[3] = {100, 100, 100, 255};
int currentColor = 0;

void updateSelectedColorValue(float percent)
{
  int updatedColor = 255 * percent;
  if (updatedColor < 0)
  {
    updatedColor = 0;
  }
  if (updatedColor > 255)
  {
    updatedColor = 255;
  }
  colorValues[currentColor] = updatedColor;
}

void updateSelectedColor()
{
  currentColor += 1;
  if (currentColor > 2)
  {
    currentColor = 0;
  }
}

void displaySelectedColor(int currentColor)
{
  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(colors[currentColor]);
  display.setCursor(80, 0);
  display.println(colorValues[currentColor]);
  display.display(); // Show initial text
}

float convertPotValToPerc(int potValue)
{
  float _max = 950.00;
  return (((float)potValue - 27) / _max);
}

void button_interrupt_handler()
{
  if (initialisation_complete == true)
  {
    //  all variables are initialised so we are okay to continue to process this interrupt
    if (interrupt_process_status == !triggered)
    {
      // new interrupt so okay start a new button read process -
      // now need to wait for button release plus debounce period to elapse
      // this will be done in the button_read function
      if (digitalRead(BUTTONPIN) == HIGH)
      {
        // button pressed, so we can start the read on/off + debounce cycle wich will
        // be completed by the button_read() function.
        interrupt_process_status = triggered; // keep this ISR 'quiet' until button read fully completed
      }
    }
  }
} // end of button_interrupt_handler

bool read_button()
{
  int button_reading;
  // static variables because we need to retain old values between function calls
  static bool switching_pending = false;
  static long int elapse_timer;
  if (interrupt_process_status == triggered)
  {
    // interrupt has been raised on this button so now need to complete
    // the button read process, ie wait until it has been released
    // and debounce time elapsed
    button_reading = digitalRead(BUTTONPIN);
    if (button_reading == HIGH)
    {
      // switch is pressed, so start/restart wait for button relealse, plus end of debounce process
      switching_pending = true;
      elapse_timer = millis(); // start elapse timing for debounce checking
    }
    if (switching_pending && button_reading == LOW)
    {
      // switch was pressed, now released, so check if debounce time elapsed
      if (millis() - elapse_timer >= debounce)
      {
        // dounce time elapsed, so switch press cycle complete
        switching_pending = false;             // reset for next button press interrupt cycle
        interrupt_process_status = !triggered; // reopen ISR for business now button on/off/debounce cycle complete
        return switched;                       // advise that switch has been pressed
      }
    }
  }
  return !switched; // either no press request or debounce period not elapsed
} // end of read_button function

void setup()
{
  Serial.begin(9600);
  Serial.println("Hello world!");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.begin();
  pixels.setBrightness(255);
  pixels.fill(pixels.Color(10, 10, 10), 0, 60);
  pixels.show();

  pinMode(BUTTONPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTONPIN),
                  button_interrupt_handler,
                  interrupt_trigger_type);
  initialisation_complete = true;
}

void loop()
{
  if (read_button() == switched)
  {
    updateSelectedColor();
  }
  else
  {
    int sensorValueRaw = analogRead(A0);
    float sensorValue = convertPotValToPerc(sensorValueRaw);
    Serial.print("Pot Value: ");
    Serial.println(sensorValueRaw);
    Serial.println(sensorValue);

    updateSelectedColorValue(sensorValue);

    for (int i = 0; i < 3; i++)
    {
      Serial.println(colors[i] + ": " + colorValues[i]);
    }
    pixels.clear();
    pixels.fill(pixels.Color(colorValues[0], colorValues[1], colorValues[2]));
    pixels.show(); // Send the updated pixel colors to the hardware.
  }
  displaySelectedColor(currentColor);
  //  delay(DELAYVAL);
}
