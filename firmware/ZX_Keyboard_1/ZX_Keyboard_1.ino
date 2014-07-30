/*
 * ZX Keyboard
 *
 * Arduino code to use a ZX Spectrum keyboard as a USB keyboard
 *
 * Alistair MacDonald 2014
 *
 */

/*
 * We use the VUSB for Arduino library from http://code.google.com/p/vusb-for-arduino/
 * Download vusb-for-arduino-005 and copy UsbKeyboard from the libraries folder to your libraries folder
 *
 * The Arduino's LED on pin 13 needs to be removed or a pullup resistor to VCC added. This is becuase we use the pin in in INPUT_PULLUP mode.
 *
 * The hardware is as follows....
 *
 * D2 -> 68R Resistor - > USB D+
 * D4 -> 68R Resistor - > USB D-
 * D5 -> 2K2 Resistor - > USB D-
 *
 * USB D+ and D- should be attached to two 3V6 0.5W Zener diodes wiht the anodes connected to ground
 *
 * D3 -> Keyboard Column 1
 * D6 -> Keyboard Column 2
 * D7 -> Keyboard Column 3
 * D8 -> Keyboard Column 4
 * D9 -> Keyboard Column 5
 * Keyboard Row 1 -> A3
 * Keyboard Row 2 -> A2
 * Keyboard Row 3 -> A1
 * Keyboard Row 4 -> A0
 * Keyboard Row 5 -> D10
 * Keyboard Row 6 -> D11
 * Keyboard Row 7 -> D12
 * Keyboard Row 8 -> D13
 *
 * Full background at http://www.agm.me.uk/blog/2014/07/zx-keyboard.php
 * Master source repository at https://github.com/alistairuk/ZX-Keyboard (please feed back and contribute)
 * Original build instructions at http://hackaday.io/project/2076-ZX-Keyboard
 *
 * Apologies for this not being as readable as I would like. Some parts are a little more complex than  they might at first appear.
 *
 */
 
#include "UsbKeyboard.h"

// Global varables
byte currentPressed[5]; // Stores the bitmask for currently pressed buttons
byte lastPressed[5];    // Stores the bitmask for the buttons pressed last time
uchar reportBuffer[4];  // Buffer for HID data (1 modifier byte and up to 3 keys)

// Setup everything
void setup() {
  // Set column pins to floating
  pinMode(3, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  // Set row pins to pullup
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  // Disable timer0 overflow interrupt
  // This is done to prevent it interupting USB communication
  TIMSK0&=!(1<<TOIE0);
}

// A dleay rputeen with the same functionality as delay() but will work with timer0 disabled
void delay2(unsigned long inDelay) {
  for (int loopIt=0; loopIt<inDelay; loopIt++) {
    delayMicroseconds(1000);
  }
}

// Read the state of one row of keys
byte readRow(int rowPin) {
  // Seet the row pin to low
  pinMode(rowPin, OUTPUT);
  digitalWrite(rowPin, LOW);
  // Read the values from the columns
  // Sorry about it looking complex. It does the same as 8 digitalRead() calls.
  byte result = (~PINB>>2 & B00001111) | (~PINC<<4 & B11110000);
  // Set the row pin back to floating
  pinMode(rowPin, INPUT);
  // All done, return the value  
  return result;
}

// Clear the buffer for HID data
void clearReportBuffer() {
  memset(reportBuffer, 0, sizeof(reportBuffer));
}

// Insert one keycode to the buffer for HID data
void addReportBufferValue(int inKeystroke) {
  memcpy(reportBuffer+2, reportBuffer+1, sizeof(reportBuffer)-2);
  reportBuffer[1] = inKeystroke;
}

// Populate the HID buffer based on the currently pressed buttons in currentPressed[]
void fillReportBuffer() {
  // Check what keys are being pressed to work out shift state
  int isShiftPressed = currentPressed[0]&0x04;
  int isNumberPressed = ((currentPressed[0]&0x90) || (currentPressed[1]&0x90) || (currentPressed[2]&0x90) || (currentPressed[3]&0x90) || (currentPressed[4]&0x90));
  int isLetterPressed = ((currentPressed[0]&0x6B) || (currentPressed[1]&0x6F) || (currentPressed[2]&0x6F) || (currentPressed[3]&0x6F) || (currentPressed[4]&0x6F)); // Ignore shift here
  // Set the shift modifier is shift is pressed, but only if no numbers (used for navigation) are pressed or a letter (and other non-number keys) is pressed
  // As odd as it is this will work fully under normal circumstances but will also increase compatibility when used with ZX Spectrum emulators
  int isShifted = isShiftPressed && (!isNumberPressed || isLetterPressed);
  reportBuffer[0] = isShifted ? 0x02 : 0x00;
  // Add keycodes for any keys pressed to the HID buffer
  // Row 0
  // Note: We only chage the modifyer when Caps Shift is pressed
  if (currentPressed[0]&0x01) addReportBufferValue(KEY_SPACE);
  if (currentPressed[0]&0x02) addReportBufferValue(KEY_ENTER);
  if (currentPressed[0]&0x08) addReportBufferValue(KEY_P);
  if (!isShiftPressed && (currentPressed[0]&0x10)) addReportBufferValue(KEY_0);
  if (isShiftPressed && (currentPressed[0]&0x10)) addReportBufferValue(0x022A); // Backspace
  if (currentPressed[0]&0x20) addReportBufferValue(KEY_A);
  if (currentPressed[0]&0x40) addReportBufferValue(KEY_Q);
  if (currentPressed[0]&0x80) addReportBufferValue(KEY_1);
  // Row 1
  // Note: This is not called when Symbol Shift is pressed
  if (currentPressed[1]&0x02) addReportBufferValue(KEY_L);
  if (currentPressed[1]&0x08) addReportBufferValue(KEY_O);
  if (currentPressed[1]&0x10) addReportBufferValue(KEY_9);
  if (currentPressed[1]&0x04) addReportBufferValue(KEY_Z);
  if (currentPressed[1]&0x20) addReportBufferValue(KEY_S);
  if (currentPressed[1]&0x40) addReportBufferValue(KEY_W);
  if (!isShiftPressed && (currentPressed[1]&0x80)) addReportBufferValue(KEY_2);
  if (isShiftPressed && (currentPressed[1]&0x80)) addReportBufferValue(0x0039); // Caps Lock
  // Row 2
  if (currentPressed[2]&0x01) addReportBufferValue(KEY_M);
  if (currentPressed[2]&0x02) addReportBufferValue(KEY_K);
  if (currentPressed[2]&0x08) addReportBufferValue(KEY_I);
  if (!isShiftPressed && (currentPressed[2]&0x10)) addReportBufferValue(KEY_8);
  if (isShiftPressed && (currentPressed[2]&0x10)) addReportBufferValue(0x004F); // Right Arrow
  if (currentPressed[2]&0x04) addReportBufferValue(KEY_X);
  if (currentPressed[2]&0x20) addReportBufferValue(KEY_D);
  if (currentPressed[2]&0x40) addReportBufferValue(KEY_E);
  if (currentPressed[2]&0x80) addReportBufferValue(KEY_3);
  // Row 3
  if (currentPressed[3]&0x01) addReportBufferValue(KEY_N);
  if (currentPressed[3]&0x02) addReportBufferValue(KEY_J);
  if (currentPressed[3]&0x08) addReportBufferValue(KEY_U);
  if (!isShiftPressed && (currentPressed[3]&0x10)) addReportBufferValue(KEY_7);
  if (isShiftPressed && (currentPressed[3]&0x10)) addReportBufferValue(0x0052); // Down Arrow
  if (currentPressed[3]&0x04) addReportBufferValue(KEY_C);
  if (currentPressed[3]&0x20) addReportBufferValue(KEY_F);
  if (currentPressed[3]&0x40) addReportBufferValue(KEY_R);
  if (currentPressed[3]&0x80) addReportBufferValue(KEY_4);
  // Row 4
  if (currentPressed[4]&0x01) addReportBufferValue(KEY_B);
  if (currentPressed[4]&0x02) addReportBufferValue(KEY_H);
  if (currentPressed[4]&0x08) addReportBufferValue(KEY_Y);
  if (!isShiftPressed && (currentPressed[4]&0x10)) addReportBufferValue(KEY_6);
  if (isShiftPressed && (currentPressed[4]&0x10)) addReportBufferValue(0x0051); // Down Arrow
  if (currentPressed[4]&0x04) addReportBufferValue(KEY_V);
  if (currentPressed[4]&0x20) addReportBufferValue(KEY_G);
  if (currentPressed[4]&0x40) addReportBufferValue(KEY_T);
  if (!isShiftPressed && (currentPressed[4]&0x80)) addReportBufferValue(KEY_5);
  if (isShiftPressed && (currentPressed[4]&0x80)) addReportBufferValue(0x0050); // Left Arrow
}

// Lookup the key ID for a key when Symbol Shift is pressed
// The modifyer (the shift state) is returned in the upper byte
// Some keys remain unshifted to increase compatibility when used with ZX Spectrum emulators
word keyToSymbol(byte inCol, byte inRow) {
  word ourKeyCode = inCol<<8 | inRow;
  switch (ourKeyCode) {
    // Row 0
    case 0x0001: return KEY_SPACE; // Space
    case 0x0002: return KEY_ENTER; // Enter
    case 0x0008: return 0x021F; // "
    case 0x0010: return 0x022D; // _
    case 0x0004: return 0x00E1; // Caps Shift
    case 0x0020: return 0x0232; // ~
    case 0x0040: return KEY_Q;  // Q
    case 0x0080: return 0x021E; // !
    // Row 1
    case 0x0101: return 0x00E4; // Symbol Shift
    case 0x0102: return 0x002E; // =
    case 0x0108: return 0x0033; // ;
    case 0x0110: return 0x0227; // )
    case 0x0104: return 0x0233; // :
    case 0x0120: return 0x0264; // |
    case 0x0140: return KEY_W;  // W
    case 0x0180: return 0x0234; // @
    // Row 2
    case 0x0201: return 0x0037; // .
    case 0x0202: return 0x022E; // +
    case 0x0208: return KEY_I;  // I
    case 0x0210: return 0x0226; // (
    case 0x0204: return 0x0220; // £
    case 0x0220: return 0x0064; // Backslash
    case 0x0240: return KEY_E;  // E
    case 0x0280: return 0x0032; // #
    // Row 3
    case 0x0301: return 0x0036; // ,
    case 0x0302: return 0x002D; // -
    case 0x0308: return 0x0030; // ]
    case 0x0310: return 0x0034; // '
    case 0x0304: return 0x0238; // ?
    case 0x0320: return 0x022F; // {
    case 0x0340: return 0x0236; // <
    case 0x0380: return 0x0221; // $
    // Row 4
    case 0x0401: return 0x0225; // *
    case 0x0402: return 0x0223; // ^
    case 0x0408: return 0x002F; // [
    case 0x0410: return 0x0224; // &
    case 0x0404: return 0x0038; // /
    case 0x0420: return 0x0230; // }
    case 0x0440: return 0x0237; // >
    case 0x0480: return 0x0222; // %
    // Default
    default: return 0;
  }
}

// Populate and send the HID buffer based on currentPressed[]
void SendNormalStrokes() {
  // Wait until any past keystrokes are sent
  while (!usbInterruptIsReady());
  // Clear the current values before we add the new ones
  clearReportBuffer();
  // Fill the buffer for the keys pressed
  fillReportBuffer();
  // Send the buffer across the USB
  usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
}

// Build and send the HID buffer for a symbol
// onCol specified the column and inRow is a bitmask of the key that was pressed
word sendSymbolKeystroke(byte inCol, byte inRow) {
  // Wait until any past keystrokes are sent
  while (!usbInterruptIsReady());
  // Clear the current values before we add the new ones
  clearReportBuffer();
  // Lookup the keycode and put it in the HID buffer
  word usbKeycode = keyToSymbol(inCol, inRow);
  reportBuffer[0] = usbKeycode>>8 & 0xFF; // Modifyer stored in the upper byte
  reportBuffer[1] = usbKeycode & 0xFF;    // Keycode stored in the lower byte
  // Send the buffer across the USB
  usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
  // Wait until any past keystrokes are sent
  while (!usbInterruptIsReady());
  // Clear the current values to simulate the key being released
  clearReportBuffer();
  // Send the empty buffer across the USB
  usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
}

// Constants used for keyMode
#define KEYMODE_NORMAL 0
#define KEYMODE_SYMBOL 1

int keyMode = KEYMODE_NORMAL; // Has the Symbol Shift been pressed?
byte changedRow;              // Used to track what has changed in a row

void loop() {

  // Poll the USB so the computer knows we are alive
  usbPoll();

  // Read in the keyboard status
  currentPressed[0] = readRow(3);
  currentPressed[1] = readRow(6);
  currentPressed[2] = readRow(7);
  currentPressed[3] = readRow(8);
  currentPressed[4] = readRow(9);

  // Check if a key has changed
  if ((lastPressed[0]!=currentPressed[0]) ||
      (lastPressed[1]!=currentPressed[1]) ||
      (lastPressed[2]!=currentPressed[2]) ||
      (lastPressed[3]!=currentPressed[3]) ||
      (lastPressed[4]!=currentPressed[4])) {

    // Send symbol if Symbol Shift pressed
    if (currentPressed[1]&0x01) {
      // We are now in symbol mode until all the buttons are released
      keyMode = KEYMODE_SYMBOL;
      // Loop though each of the rows read
      for (int loopCol=0; loopCol<sizeof(currentPressed); loopCol++) {
        // Check if any of the buttons have just been pressed and send the keystroke if they have.
        changedRow = currentPressed[loopCol] & ~lastPressed[loopCol];
        if (changedRow) {
          sendSymbolKeystroke(loopCol, changedRow);
        }
      }
    }
    
    // Reset key mode if no key pressed (ignoring shift)
    // We wait for no keys to prevent sending a lot of keys when Symbol Shift is released
    if (((currentPressed[0]&0xFB) | currentPressed[1] | currentPressed[2] | currentPressed[3] | currentPressed[4])==0x00) {
      keyMode = KEYMODE_NORMAL;
    }
    
    // Send keystrokes if in normal operation
    if (keyMode==KEYMODE_NORMAL) {
      SendNormalStrokes();
    }

    // Remember that last state of the keyboard
    memcpy(lastPressed, currentPressed, sizeof(lastPressed));

    // Poor man's debounce
    // Yes it is a very long time but these old rubber keyboard have a lot of bounce 
    // We can't use delay() as we disabled timer0 se we use our delay2() replacement
    delay2(80);

  }
  else {
    // No key was pressed so save some power by sleeping a little
    // We can't use delay() as we disabled timer0 se we use our delay2() replacement
    delay2(20);
  }
}
