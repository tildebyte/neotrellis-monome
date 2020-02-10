/*
    NeoTrellis Grid
    Many thanks to scanner_darkly, Szymon Kaliski, John Park, todbot, Juanma and others
*/
#include "MonomeSerialDevice.h"
#include <Adafruit_NeoTrellis.h>
#include "debug.h"


#define NUM_ROWS 8 // DIM_Y number of rows of keys down
#define NUM_COLS 16 // DIM_X number of columns of keys across

uint32_t hexColor;

// This assumes you are using a USB breakout board to route power to the board
// If you are plugging directly into the teensy, you will need to adjust this brightness to a much lower value
#define BRIGHTNESS 128 // overall grid brightness - use gamma table below to adjust levels
uint8_t R;
uint8_t G;
uint8_t B;
//  amber? {255,191,0}
//  warmer white? {255,255,200}

// set your monome device name here
String deviceID = "m128";
bool isInited = false;
int selected_palette = 0;
elapsedMillis monomeRefresh;

// Monome class setup
MonomeSerialDevice mdp;
int prevLedBuffer[mdp.MAXLEDCOUNT];

// NeoTrellis setup
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    { Adafruit_NeoTrellis(0x2e), Adafruit_NeoTrellis(0x2f), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31) }, // top row
    { Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x33), Adafruit_NeoTrellis(0x34), Adafruit_NeoTrellis(0x35) } // bottom row
};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, NUM_ROWS / 4, NUM_COLS / 4);

// gamma table for 16 levels of brightness
//const uint8_t gammaTable[16] = {0, 13, 15, 23, 32, 41, 52, 63, 76, 91, 108, 129, 153, 185, 230, 255};
const uint8_t gammaTable[16] = {0, 3, 4, 5, 8, 15, 24, 35, 50, 68, 89, 115, 143, 176, 213, 255};

const uint8_t allpalettes[9][3][16] = {
    {//inferno
        {0,16,29,67,107,117,163,183,231,245,251,251,251,243,242,244},
        {0,20,12,10,23,27,43,55,95,124,157,179,185,224,243,247},
        {0,112,69,104,110,109,97,83,44,21,6,24,30,86,129,141}
    },
    {//viridis
        {68, 72, 67, 57, 48, 42, 39, 34, 30, 33, 50, 83, 116,167,218,248},
        {3,33, 59, 84, 104, 117, 126,137, 156, 167, 181, 197, 208, 219, 226, 230},
        {87, 114, 131, 139, 141, 148, 142, 141, 137, 132, 122, 103, 84,  51,  24,  33}
    },
    {//terrain
        {44, 24, 1,0,65, 129,197,254,228,226,198,168,131,163,196,237},
        {64 ,104,149,192,217,229,243,253,220,217,182,143,96, 137,180,231},
        {166,206,251,136,115,127,141,152,138,137,122,106,88 ,131,177,230}
    },
    {//ocean
        {0,0,0,0,0,0,0,0,0,0,0,20, 96, 137,197,246},
        {0,124,82,99, 63, 26, 7,24, 48, 75, 116,137,175,196,226,250},
        {0,2,30,19, 43, 67, 89, 101,117,135,163,177,202,216,236,252}
    },
    {//cubehelix
        {0,35, 26, 22, 23, 49, 113,161,194,209,210,202,194,194,212,242},
        {0,21, 29, 55, 89, 114,122,121,121,128,149,169,194,216,239,251},
        {0,56, 59, 76, 73, 54, 50, 74, 114,156,207,230,242,242,238,246}
    },
    {//cividis
        {0,0,0,31, 62, 85, 101,117,134,154,175,190,206,229,239,253},
        {0,37, 47, 58, 75, 91, 104,117,131,147,163,176,188,207,216,231},
        {0,84, 109,110,107,109,112,117,120,118,112,106,98, 80, 70, 55}
    },
//    {//gnuplot
//        {0,57, 84, 102,121,136,148,158,168,179,196,211,225,234,250,255},
//        {0,0,0,1,3,5,9,14, 21, 30, 54, 82, 121,154,228,249},
//        {0,80, 162,215,252,248,217,164,100,9,0,0,0,0,0,0}
//    },
    {//copper
        {0,50, 54, 80, 97, 113,139,152,179,208,234,255,255,255,255,255},
        {0,32, 34, 50, 61, 71, 88, 96, 113,132,148,161,174,184,192,199},
        {0,20, 21, 32, 39, 45, 56, 61, 72, 84, 94, 102,110,117,122,126}
    },
    {//varibright
        {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
        {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
        {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
    }
};


// ***************************************************************************
// **                                HELPERS                                **
// ***************************************************************************
// Pad a string of length 'len' with nulls
void pad_with_nulls(char* s, int len) {
    int l = strlen(s);
    for ( int i = l; i < len; i++) {
        s[i] = '\0';
    }
}


// ***************************************************************************
// **                          FUNCTIONS FOR TRELLIS                        **
// ***************************************************************************
// define a callback for key presses
TrellisCallback keyCallback(keyEvent evt){
    uint8_t x;
    uint8_t y;
    x = evt.bit.NUM % NUM_COLS; // NUM_COLS;
    y = evt.bit.NUM / NUM_COLS; // NUM_COLS;
    if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
//      on
//        debugfln(INFO, "%d, %d pressed", x, y);
        mdp.sendGridKey(x, y, 1);
//------------color palette initial selection-------------------
    if (isInited == false){
        selected_palette = y;
        isInited=true;
        for(int i=0; i < 8; i++){
           colorpalettedisplay(y, i);
        }
        trellis.show();
        sendLeds();
        delay(1000);
        for(int i=0; i < 8; i++){
           colorpalettedisplay(8, i);
        }
        trellis.show();
        sendLeds();
      }
//--------------------------------------------------------------
    } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
//      off
//        debugfln(INFO, "%d, %d released", x, y);
        mdp.sendGridKey(x, y, 0);
    }
    return 0;
}

// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************
void setup(){
    Serial.begin(115200);
    R = 255;
    G = 255;
    B = 255;
    mdp.isMonome = true;
    mdp.deviceID = deviceID;
    mdp.setupAsGrid(NUM_ROWS, NUM_COLS);
    trellis.begin();

    // set overall brightness for all pixels
    uint8_t x, y;
    for (x = 0; x < NUM_COLS / 4; x++) {
        for (y = 0; y < NUM_ROWS / 4; y++) {
            trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
        }
    }

    // key callback
    for (x = 0; x < NUM_COLS; x++) {
        for (y = 0; y < NUM_ROWS; y++) {
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
            trellis.registerCallback(x, y, keyCallback);
        }
    }
    delay(300);
    mdp.setAllLEDs(0);
    sendLeds();
    monomeRefresh = 0;

    // blink one led to show it's started up
    trellis.setPixelColor(0, 0xFFFFFF);
    trellis.show();
    delay(100);
    trellis.setPixelColor(0, 0x000000);
    trellis.show();
    colorpaletteselector();
}

// ***************************************************************************
// **                                SEND LEDS                              **
// ***************************************************************************

void sendLeds(){
    uint8_t value, prevValue = 0;
    uint8_t Rvalue = 64, Gvalue = 0, Bvalue = 0;
    bool isDirty = false;

    for(int i=0; i < NUM_ROWS * NUM_COLS; i++){
        value = mdp.leds[i];
        prevValue = prevLedBuffer[i];
        Rvalue = allpalettes[selected_palette][0][value] * gammaTable[value] / 256;
        Gvalue = allpalettes[selected_palette][1][value] * gammaTable[value] / 256;
        Bvalue = allpalettes[selected_palette][2][value] * gammaTable[value] / 256;
        if (value != prevValue) {
            hexColor =  ((Rvalue << 16) + (Gvalue << 8) + (Bvalue << 0));
            trellis.setPixelColor(i, hexColor);
            prevLedBuffer[i] = value;
            isDirty = true;
        }
    }

    if (isDirty) {
        trellis.show();
    }
}


// ***************************************************************************
// **                       color palette selector                          **
// ***************************************************************************
void colorpaletteselector(){
    for (int i=0; i < 8; i++){
        colorpalettedisplay(i, i);
    }
    trellis.show();
}
void colorpalettedisplay(int selected, int row){
    for (int i=0; i<NUM_COLS; i++){
        uint8_t gvalue = gammaTable[i];
        uint8_t Rvalue = allpalettes[selected][0][i];
        uint8_t Gvalue = allpalettes[selected][1][i];
        uint8_t Bvalue = allpalettes[selected][2][i];
        trellis.setPixelColor((i + row * 16),
                              ((Rvalue * gvalue / 256 << 16)
                              + (Gvalue * gvalue / 256 << 8)
                              + (Bvalue * gvalue / 256 << 0))); //addressed with keynum
        delay(0);
    }
}


// ***************************************************************************
// **                                 LOOP                                  **
// ***************************************************************************
void loop() {
    mdp.poll(); // process incoming serial from Monomes

    // refresh every 16ms or so
    if (isInited && monomeRefresh > 16) {
        trellis.read();
        sendLeds();
        monomeRefresh = 0;
    }
    else if (isInited==false){
        trellis.read();
    }
}
