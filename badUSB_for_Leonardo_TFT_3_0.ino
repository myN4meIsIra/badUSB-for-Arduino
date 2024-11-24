#include <SD.h>
#include <SeeedTouchScreen.h>
#include "Keyboard.h"
#include "TFTv2.h"
#include <stdint.h>
#include <SPI.h>

// for touchscreen, change a few details dependant of which board is being used
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)  // mega
#define YP A2                                                   // must be an analog pin, use "An" notation!
#define XM A1                                                   // must be an analog pin, use "An" notation!
#define YM 54                                                   // can be a digital pin, this is A0
#define XP 57                                                   // can be a digital pin, this is A3
#elif defined(__AVR_ATmega32U4__)                               // leonardo
#define YP A2                                                   // must be an analog pin, use "An" notation!
#define XM A1                                                   // must be an analog pin, use "An" notation!
#define YM 18                                                   // can be a digital pin, this is A0
#define XP 21                                                   // can be a digital pin, this is A3
#elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)                  // samd21
#define YP A2                                                   // must be an analog pin, use "An" notation!
#define XM A1                                                   // must be an analog pin, use "An" notation!
#define YM A4                                                   // can be a digital pin, this is A0
#define XP A3                                                   // can be a digital pin, this is A3
#undef Serial
#define Serial SerialUSB
#else          //168, 328, something else
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A1  // must be an analog pin, use "An" notation!
#define YM 14  // can be a digital pin, this is A0
#define XP 17  // can be a digital pin, this is A3
#endif

//Measured ADC values for (0,0) and (210-1,320-1)
//TS_MINX corresponds to ADC value when X = 0
//TS_MINY corresponds to ADC value when Y = 0
//TS_MAXX corresponds to ADC value when X = 240 -1
//TS_MAXY corresponds to ADC value when Y = 320 -1
#define TS_MINX 116 * 2
#define TS_MAXX 890 * 2
#define TS_MINY 83 * 2
#define TS_MAXY 913 * 2


// for << output notation
template<typename T>
Print& operator<<(Print& printer, T value) {
  printer.print(value);
  return printer;
}


// variables for screen
const int chipSelect = 4;
int screenWidth = 240;
int screenHeight = 320;
int boxSize = round(screenWidth / 3);

short outsideBoxColor = 0xffff;
short insideBoxColor = 0x007C41;
short selectedBoxColor = 0x138A36;
short fileNameColor = 0x0000;

// variables for SD card
int maxNumberOfFiles = 25;
int numberOfFilesInArray = 0;
File* fileArray = new File[maxNumberOfFiles];
File root;



int selectedScript = 10;

void setup() {
  delay(1000);

  Serial.begin(9600);
  Keyboard.begin();

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);

  Tft.TFTinit();

  Sd2Card card;
  card.init(SPI_FULL_SPEED, chipSelect);

  // give error if SD card has error
  if (!SD.begin(chipSelect)) {
    Serial.println("error in SD card read");
    Tft.fillRectangle(0, 0, screenHeight, screenWidth, RED);
    Tft.drawString("error in SD card", screenHeight / 2, screenWidth / 2, 3, RED);
    while (true)
      ;
  }

  delay(10);
  root = SD.open("/");
  printDirectory(root, 0);
  root.close();

  delay(10);
  //printFilesArray();

  displayAvailableScripts();
  //runFile(fileArray[0]);



  Serial.println("done!");
}

TouchScreen ts = TouchScreen(XP, YP, XM, YM);
void loop() {

  while (1) {

    touchSelection(ts);
  }
}


void touchSelection(TouchScreen ts) {
  Point p = ts.getPoint();

  if (p.z > __PRESSURE && p.z < 1000) {
    Serial.print("Raw X = ");
    Serial.print(p.x);
    Serial.print("\tRaw Y = ");
    Serial.print(p.y);
    Serial.print("\tPressure = ");
    Serial.println(p.z);
  }


  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 320);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if (p.z > __PRESSURE && p.z < 1000) {
    Serial.print("X = ");
    Serial.print(p.x);
    Serial.print("\tY = ");
    Serial.print(p.y);
    Serial.print("\tPressure = ");
    Serial.println(p.z);
  }



  // up down execute buttons
  if (p.y >= screenWidth && (p.z > __PRESSURE && p.z < 1000)) {
    // top third
    if (screenWidth - boxSize < p.x) {
      Serial.println("up");
      delay(500);
    }
    // middle third
    if (boxSize < p.x && p.x < screenWidth - boxSize) {
      Serial.println("execute");
      runFile(fileArray[selectedScript]);
      delay(500);
    }
    // bottom third
    if (p.x < boxSize) {
      Serial.println("down");
      delay(500);
    }
  }

  // for the files
  else if (p.z > __PRESSURE && p.z < 1000) {
    // right
    if (p.y <= screenWidth && p.y >= screenWidth - boxSize && (p.z > __PRESSURE)) {
      // top third
      if (screenWidth - boxSize < p.x) {
        selectedScript = 2;
        delay(500);
      }
      // middle third
      if (boxSize < p.x && p.x < screenWidth - boxSize) {
        selectedScript = 5;
        delay(500);
      }
      // bottom third
      if (p.x < boxSize) {
        selectedScript = 8;
        delay(500);
      }
    }

    // middle
    if (p.y >= boxSize && p.y <= screenWidth - boxSize && (p.z > __PRESSURE)) {
      // top third
      if (screenWidth - boxSize < p.x) {
        selectedScript = 1;
        delay(500);
      }
      // middle third
      if (boxSize < p.x && p.x < screenWidth - boxSize) {
        selectedScript = 4;
        delay(500);
      }
      // bottom third
      if (p.x < boxSize) {
        selectedScript = 9;
        delay(500);
      }
    }

    // left
    if (p.y <= boxSize && (p.z > __PRESSURE)) {
      // top third
      if (screenWidth - boxSize < p.x) {
        selectedScript = 0;
        delay(500);
      }
      // middle third
      if (boxSize < p.x && p.x < screenWidth - boxSize) {
        selectedScript = 3;
        delay(500);
      }
      // bottom third
      if (p.x < boxSize) {
        selectedScript = 6;
        delay(500);
      }
    }

    Serial << "script: " << selectedScript << "\n";
    displayAvailableScripts();
  }
}


// visual stuffs
void displayAvailableScripts() {
  // text orientation
  TextOrientation orientation = LANDSCAPE;

  int positionInFilesArray = 0;

  //fillScreen(INT16U XL,INT16U XR,INT16U YU,INT16U YD,INT16U color);
  Tft.fillScreen(0, screenWidth, 0, screenHeight, BLACK);

  //fillRectangle(INT16U poX, INT16U poY, INT16U length, INT16U width, INT16U color);
  Tft.fillRectangle(0, 0, screenWidth, screenWidth, RED);

  // the row
  int term = 0;
  for (int i = 0; i < 3; i++) {
    // the column
    for (int j = 0; j < 3; j++) {
      //Serial << "generating block " << i << "," << j << "\n";
      // outside block
      Tft.fillRectangle(screenWidth - ((i + 1) * boxSize), boxSize * j, boxSize, boxSize, outsideBoxColor);

      // inside block
      int normColor = insideBoxColor;
      if (selectedScript == term++) insideBoxColor = selectedBoxColor;
      Tft.fillRectangle(5 + screenWidth - ((i + 1) * boxSize), 5 + boxSize * j, boxSize - 10, boxSize - 10, insideBoxColor);
      insideBoxColor = normColor;
    }
  }
  for (int i = 0; i < 3; i++) {
    // the column
    for (int j = 0; j < 3; j++) {
      Serial << "position {" << j << "," << i << "} \t = " << fileArray[positionInFilesArray].name() << "\n";
      Tft.drawString(fileArray[positionInFilesArray++].name(), 5 + boxSize * (j), screenWidth - (boxSize * i) - (boxSize / 2), 1, fileNameColor, orientation);
    }
  }

  //the run and scroll circles
  // scroll up
  Tft.fillCircle(screenWidth - ((screenHeight - screenWidth) / 2),
                 screenHeight - (screenHeight - screenWidth) / 2,
                 (screenHeight - screenWidth) / 2 - 5, WHITE);
  Tft.drawString("UP",
                 screenHeight - (screenHeight - screenWidth) + 32,
                 screenWidth - 1 * ((screenHeight - screenWidth) / 2) - 5,
                 1, BLACK, orientation);

  // execute
  Tft.fillCircle(screenWidth - 3 * ((screenHeight - screenWidth) / 2),
                 screenHeight - (screenHeight - screenWidth) / 2,
                 (screenHeight - screenWidth) / 2 - 5, RED);
  Tft.drawString("EXECUTE",
                 screenHeight - (screenHeight - screenWidth) + 18,
                 screenWidth - 3 * ((screenHeight - screenWidth) / 2) - 5,
                 1, WHITE, orientation);

  // scroll down
  Tft.fillCircle(((screenHeight - screenWidth) / 2),
                 screenHeight - (screenHeight - screenWidth) / 2,
                 (screenHeight - screenWidth) / 2 - 5, WHITE);
  Tft.drawString("DOWN",
                 screenHeight - (screenHeight - screenWidth) + 27,
                 screenWidth - 5 * ((screenHeight - screenWidth) / 2) - 5,
                 1, BLACK, orientation);
}


// print all the files that have been dropped into the file array
void printFilesArray() {
  Serial.println("printing files ...");
  for (int i = 0; i < (sizeof(fileArray) / sizeof(File)) + 1; i++) {
    Serial.println(fileArray[i].name());
  }
}


// print the text in a file
void runFile(File file) {

  TextOrientation orientation = LANDSCAPE;

  Serial << "\n\nprint file \"" << file.name() << "\"\n";

  Tft.fillRectangle(0, 0, screenHeight, screenWidth, WHITE);


  // re-open the file for reading:
  File myFile = SD.open(file.name());// SD.open(file.name());
  //File myFile = file;
  if (myFile) {

    char* fileName = myFile.name();
    Serial << fileName << "\n";
    if (fileName[0] == 'W') {
      //Pressing Win+r shortcut
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('r');
      delay(50);
      Keyboard.releaseAll();
      delay(50 * 5);

      //Inputting cmd command
      Keyboard.println("cmd");
      delay(1000);  //A delay to ensure that cmd window has been started

    } else if (fileName[0] == 'L') {
      //Pressing Win+r shortcut
      Keyboard.press(132);
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('t');
      delay(50);
      Keyboard.releaseAll();
    
      delay(1000);  //A delay to ensure that cmd window has been started
    }

    int position = 5;
    int iteration = 0;
    //char* text= (char *) malloc(sizeof(char*) * 50);
    char text[255] = "";
    while (myFile.available()) {

      char c[2];
      //getline(myFile, text);
      myFile.read(c, 1);
      text[iteration++] = c[0];

      //File text = myFile.read();

      if (c[0] == '\n') {
        Tft.drawString(text, 5, screenWidth - (position += 10), 1, GREEN, orientation);
        Keyboard.println(text);
        Serial.println(text);
        delay(50);
        memset(&text[0], 0, 255);
        iteration = 0;
        //break;
      }
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}


void printDirectory(File dir, int numTabs) {
  Serial.println("print directory");
  while (true) {
    File entry = dir.openNextFile();
    char* name = entry.name();
    //Serial << "entry anme: " << name << "\n";

    if (!entry) {
      // no more files
      break;
    }

    char fileExtension[4] = "abc";
    for (int i = 0; i < 3; i++) {
      int c = strlen(name) - 3 + i;
      fileExtension[i] = name[c];
    }
    if (strcmp(fileExtension, "TXT") == 0) {
      // append file to the file array and increment the counter
      fileArray[numberOfFilesInArray++] = entry;

      // break out of loop if we have too many files
      if (numberOfFilesInArray > maxNumberOfFiles) break;
    }

    entry.close();
  }

  delay(50);
}
