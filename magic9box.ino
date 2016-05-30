#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int I2C_ADDR = 0x27;  // Define I2C Address where the PCF8574A is
const int xAxis = A3;
const int yAxis = A2;
const int zAxis = A1;

int axis[3] = { xAxis, yAxis, zAxis };

const int shakeThreshold = 20; // accelerometer values must exceed this

long accelReadMillis;  // general loop control
long accelReadLoop = 200;  // milliseconds between checks

#define BACKLIGHT_TIMER 10000 // ten seconds

unsigned int shakenCounter = 0;  // keep track of the number of times shaken
unsigned int doesNotComputeCounter = 0;  // keep track of the number of times in Does Not Compute Mode
unsigned int freakOutCounter = 0;  // keep track of the number of time in Freak Out Mode

unsigned long milliCount = 0;

char* firstLine[] = 
  { "The answer is...",
   "I'm thinking...",
   "I'll go with...",
   "Let's say...",
   "How about...",
   "What about...",
   "Consider...",
   "1 possibility...",
   "If I must..." };

char* secondLine[] = 
  { "42",
   "2 points",
   "It's not ready",
   "You need a spike",
   "Let John decide",
   "Thang's pickle",
   "No Wei Wei",
   "No Wei Bro",
   "Vodka first",
   "Break it down",
   "Delete it",
   "Invalid use case",
   "Swarm on this",
   "Vote on it",
   "Scrum Poker",
   "Move to ready",
   "3 points",
   "8 points",
   "Whatever",
   "No way",
   "That's easy",
   "It's blocking",
   "Bad user story",
   "I'm no Genie",
   "Need beer",
   "Need ACs",
   "Need ATs",
   "No test case",
   "Mike's right",
   "Nick's Wright",
   "No API",
   "Do the API",
   "CbQoS FTW",
   "Lenny Ha-Ha",
   "GFDISDTS",
   "Relax, its over",
   "Move to backlog",
   "Burn this down"};
unsigned int firstLineSize = (sizeof(firstLine) / sizeof(char*));
unsigned int secondLineSize = (sizeof(secondLine) / sizeof(char*));

LiquidCrystal_I2C	lcd(I2C_ADDR,16, 2);

void setup()
{
  pinMode(12, INPUT);
  lcd.begin();
  lcd.backlight();
  lcd.home();
  lcd.print("Magic 9 Box");
  lcd.setCursor(0, 1);
  lcd.print("version 0.92");
  delay(1750);
  lcd.noBacklight();
  accelReadMillis = millis();
}

void loop()
{
  long startMillis = millis();

  // if we are not currently displaying a message and it's time to see if we have been shaken
  if ((milliCount == 0) && ((startMillis - accelReadMillis) > accelReadLoop)) {
    if (HasBeenShaken())
      WasShaken();

    accelReadMillis = millis();
  }
  else {  // if we are displaying a message, see if it is time to clear it.
    if (milliCount != 0) {
      if ((startMillis - milliCount) >= BACKLIGHT_TIMER) {
        milliCount = 0;
        clearDisplay();
      }
    }
  }
}

boolean
HasBeenShaken() {
  // take 5 samples on each of the 3 axis
  int raw[3][5];

  for (int a = 0; a < 3; a++) {
    for (int i = 0; i < 5; i++) {
      raw[a][i] = analogRead(axis[a]);
      delay(200);
    }
  }

  // calculate the differences
  // 5 samples yields 4 differences on each of the 3 axis
  int diffs[3][4];
  for (int ad = 0; ad < 3; ad++) {
    int leftVal = raw[ad][0];
    for (int id = 1; id < 5; id++) {
      diffs[ad][id-1] = abs(leftVal - raw[ad][id]);
    }
  }
  
  // count the number above the shake threshold
  int overThreshold = 0;
  for (int ad11 = 0; ad11 < 3; ad11++) {
    for (int id11 = 0; id11 < 4; id11++) { 
      if (diffs[ad11][id11] > shakeThreshold)
        overThreshold++;
    }
  }

  if (overThreshold >= 6)  // six is just a threshold based on observation
    return true;
    
  return false;
}

void WasShaken()  {

  shakenCounter++; 
  boolean done = false;

  // Determine the Mode: 	1. Does Not Compute, 2. 2 answers, 3. 1 answer, 4. Freak out
  uint8_t ar = analogRead(A0);
  randomSeed((ar * millis()) / (millis() / 5));
  unsigned int mode = random(1, 5);
  
  // turn the backlight on
  lcd.backlight();  

  while (!done) {
    switch(mode) {

      case 1:
	  doesNotComputeCounter++;
	  // limit does not compute mode to 10% of all shakes
	  if (doesNotComputeCounter < (shakenCounter / 10)) {
	    DoesNotCompute();
	    done = true;
	  }
	  else mode = random(2, 5); // skip mode 1, does not compute
	  break;

      case 2: {
        unsigned int x = random(0, firstLineSize); // pick a random message for the first line
        char* msg = firstLine[x];
        lcd.home();
	    lcd.print(msg);
        delay(750);
	    // fall through to the next mode
      }

      case 3: {
        unsigned int x = random(0, secondLineSize);  // pick a random message for the second line
        char* msg = secondLine[x];
        lcd.setCursor(0, 1);
	    lcd.print(msg);
	    done = true;
        break;
      }

      case 4:
        freakOutCounter++;
	    // limit freak out mode to 10% of all shakes
	    if (freakOutCounter < (shakenCounter / 10)) {
	      FreakOut();
	      done = true;
	  }
	  else mode = random(1, 4);  // try another mode, not this one (1, 2, or 3)
      break;
    } // switch (mode)
  } // while (!done)
  // start a timer for 10 seconds
  milliCount = millis();
}

void DoesNotCompute() {
  lcd.home();
  lcd.print("    That...    ");
  delay(1200);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Does Not Compute");
  delay(1200);
  lcd.clear();
  lcd.home();
  lcd.print("Rebooting...");
  delay(200);
  for (int i = 0; i < 12; i++) {
    lcd.scrollDisplayLeft();
    delay(250);
  }  
}

void FreakOut() {
  // 0xa2 through 0xfe  these are some the lcd's whacky characters we will display
  // 162 through 254
  for (int numflashes = 0; numflashes < 6; numflashes++) { // 3 times through, 2 lines = 6

    if (numflashes && (numflashes % 2 != 0))
      lcd.setCursor(0, 1);
    else
      lcd.home();
    for (int j = 0; j < 16; j++)
      lcd.write(random(162, 255));
    delay(750);
  }
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("      Ugh       ");
}

// clear the display
void clearDisplay()  {
  lcd.clear();
  lcd.noBacklight();
}

