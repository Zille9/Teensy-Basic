//############################################################################################
//#         RunCpm 6.9 für Teensy 4.x und VGA_t4 mit 16-Color Terminalemulation              #
//#         Anpassung an Teensy 4.x mit VGA-Terminal : Zille9 05/2026                        #
//#         RunCpm original : Marcelo Dantas                                                 #
//#         VGA_t4          : Jean-MarcHarvengt                                              #
//#         USBHost_t36     : Paul Stoffregen                                                #
//#                                                                                          #
//#         -- erste funktionierende Version --                                              #
//#         Terminalemulation ANSI(VT-100), KEYPROII                                         #
//#         -Terminalemulation noch nicht ganz perfekt aber benutzbar                        #
//#         -VGA-Connections (siehe MCUME-Projekt von Jean-MarcHarvengt)                     #
//#                                                                                          #
//#         -EXIT oder F12 beendet RunCPM und startet Teensy-Basic                           #
//#                                                                                          #
//#         HINWEIS: zum compilieren ist die Lib USBHost_t36 durch USBHost_t36_patched       #
//#                  zu ersetzen                                                             #
//############################################################################################

#include "globals.h"

#include <SPI.h>

#define SDFAT_FILE_TYPE 1 // Uncomment for Due, Teensy and Pico
#define DISABLE_FS_H_WARNING

#include <SdFat.h>  // One SD library to rule them all - Greinman SdFat from Library Manager

// Board definitions go into the "hardware" folder, if you use a board different than the
// Arduino DUE, choose/change a file from there and reference that file here
#include "hardware/teensy/teensy41.h"
//################################## VGA-Ausgabe #############################################
#include <VGA_t4.h>                     //VGA Anzeige-Treiber
VGA_T4 vga;

#define MAX_C 80
#define MAX_R 55
// Cursor-Koordinaten
int x_pos = 0;
int y_pos = 0;
static int fb_width, fb_height;

// Startwerte setzen
uint8_t fColor = 0xFF;
uint8_t bColor = 0x00;          // Schwarz

// Die Text-Matrix für das Scrolling im RAM
char screenBuffer[MAX_R][MAX_C];

// Zustände für den ANSI-Parser
#define STATE_NORMAL   0
#define STATE_ESC      1
#define STATE_ROW      2
#define STATE_COL      3
#define STATE_ATTR     4
#define STATE_ANSI_CSI 5
#define STATE_KAYPRO_CMD  6

int kayproState = STATE_NORMAL;
int targetRow = 0;

#define STATE_ATTR   4  // States!
char lastAttrCmd = 0; 
// Parameter-Puffer für numerische Werte (z.B. bei ESC[25;80H)
#define MAX_ANSI_PARAMS 5
int ansiParams[MAX_ANSI_PARAMS];
int ansiParamIdx = 0;

#include "USBHost_t36.h"                    //USB-Keyboard-treiber
USBHost myusb;
USBHub hub1(myusb);                         // Optional, falls ein Hub verwendet wird
USBHIDParser hid1(myusb);                   // Der "Dolmetscher" für moderne Tastaturen
KeyboardController keyboard1(myusb);

volatile int lastUsbChar = -1;              //Merker für die letzte gedrückte Taste

// Timing-Konstanten
const int REPEAT_DELAY = 500;               // Millisekunden bis zur ersten Wiederholung
const int REPEAT_RATE = 80;                 // Geschwindigkeit der Wiederholung danach

// Zustandsvariablen
uint8_t currentKey = 0;                     // Die aktuell gehaltene Taste
uint8_t currentMod = 0;                     // Shift, Ctrl, etc.
elapsedMillis repeatTimer;                  // Interner Timer
bool repeatPhase = false;                   // Ob wir bereits in der Wiederholungsphase sind
// Globale Variablen für das Gedächtnis des Repeat-Systems
int lastRepeatUnicode = 0;                  // Speichert den Buchstaben (z.B. 'A' oder 65)
uint8_t lastRepeatMod = 0;                  // Speichert Shift/AltGr Zustand
uint8_t lastRepeatKeycode = 0;              // Speichert die physische Taste (z.B. 40 für Enter)
// Warteschlange für Mehrbyte-Tastaturbefehle (ANSI-Pfeiltasten)
volatile uint8_t keyQueue[4] = {0};
volatile int keyQueueIdx = 0;
volatile int keyQueueLen = 0;

int saved_x = 0;
int saved_y = 0;

//############################################################################################

// Delays for LED blinking
#define sDELAY 50
#define DELAY 100

#include "abstraction_arduino.h"

// Serial port speed
#define SERIALSPD 9600

// PUN: device configuration
#ifdef USE_PUN
File32 pun_dev;
int pun_open = FALSE;
#endif

// LST: device configuration
#ifdef USE_LST
File32 lst_dev;
int lst_open = FALSE;
#endif

#include "ram.h"
#include "console.h"
#include CPU
#include "disk.h"
#include "host.h"
#include "cpm.h"
#ifdef CCP_INTERNAL
#include "ccp.h"
#endif


void setup(void) {

  Serial.begin(SERIALSPD);
  delay(200);
  myusb.begin();
  
  keyboard1.attachPress(OnPress);
  keyboard1.attachRelease(OnRelease); // Stoppt das Repeat-System
  vga.begin(VGA_MODE_640x480);
  vga.get_frame_buffer_size(&fb_width, &fb_height);

#ifdef DEBUGLOG
  _sys_deletefile((uint8 *)LogName);
#endif

  _clrscr();
  _puts("\e_F49;4$");
  _puts("\e[92m######## \e[1m #######    #####   \e[0m");
  _puts("\e_F49;5$");
  _puts("\e[92m     ##  \e[1m##     ##  ##   ##  \e[0m");
  _puts("\e_F49;6$");
  _puts("\e[92m    ##   \e[1m##     ## ##     ## \e[0m");
  _puts("\e_F49;7$");
  _puts("\e[92m   ##    \e[1m #######  ##     ## \e[0m");
  _puts("\e_F49;8$");
  _puts("\e[92m  ##     \e[1m##     ## ##     ## \e[0m");
  _puts("\e_F49;9$");
  _puts("\e[92m ##      \e[1m##     ##  ##   ##  \e[0m");
  _puts("\e_F49;10$");
  _puts("\e[92m######## \e[1m #######    #####   \e[0m");
  _puts("\e_F1;3$");


  _puts("\e[1m");
  _puts("CP/M 2.2 Emulator v" VERSION " by Marcelo Dantas\r\n");
  _puts("Arduino read/write support by Krzysztof Klis\r\n");
  _puts("VGA_t4 output support by Jean-MarcHarvengt\r\n");
  _puts("      Built " __DATE__ " - " __TIME__ "\r\n");
  _puts("   ** VGA-Version by Zille9 - May 2026 **\r\n");
// VGA16 Colored Text
  _puts("\e[96m \e[101m V \e[30m\e[102m G \e[96m\e[104m A \e[30m\e[103m 16 \e[96m\e[45m Controller \e[0m [\e[101m \e[41m \e[102m \e[42m \e[104m \e[44m \e[103m \e[43m \e[105m \e[45m \e[106m \e[46m \e[107m \e[47m \e[0m]\r\n");
  _puts("\e[1m");
  _puts("--------------------------------------------\r\n");
  _puts("\e[0m\r\n");

  _puts("\e[93m");

  _puts("CPU is ");
  _puts(CPU_IS);
  _puts("\r\n");
  Z80estimateClock();
  _puts("BIOS at 0x");
  _puthex16(BIOSjmppage);
  _puts(" - ");
  _puts("BDOS at 0x");
  _puthex16(BDOSjmppage);
  _puts("\r\n");
  _puts("CCP " CCPname " at 0x");
  _puthex16(CCPaddr);
  _puts("\r\n");

#if defined board_esp32
  _puts("Initializing SPI.\r\n");
  SPI.begin(SPIINIT);
#endif
  _puts("Initializing SD card.\r\n");
  if (SD.begin(SDINIT)) {
    if (VersionCCP >= 0x10 || SD.exists(CCPname)) {
#ifdef ABDOS
      _PatchBIOS();
#endif
      while (true) {
        _puts(CCPHEAD);
        _PatchCPM();
        Status = STATUS_RUNNING;
#ifdef CCP_INTERNAL
        _ccp();
#else
        if (!_RamLoad((uint8 *)CCPname, CCPaddr , 0)) {
          _puts("Unable to load the CCP.\r\nCPU halted.\r\n");
          break;
        }
        // Loads an autoexec file if it exists and this is the first boot
        // The file contents are loaded at ccpAddr+8 up to 126 bytes then the size loaded is stored at ccpAddr+7
        if (firstBoot) {
          if (_sys_exists((uint8*)AUTOEXEC)) {
            uint16 cmd = CCPaddr + 8;
            uint8 bytesread = (uint8)_RamLoad((uint8*)AUTOEXEC, cmd, 125);
            uint8 blen = 0;
            while (blen < bytesread && _RamRead(cmd + blen) > 31)
              blen++;
            _RamWrite(cmd + blen, 0x00);
            _RamWrite(--cmd, blen);
          }
          if (BOOTONLY)
            firstBoot = FALSE;
        }
        Z80reset();
        SET_LOW_REGISTER(BC, _RamRead(DSKByte));
        PC = CCPaddr;
        Z80run(cpuDelayInstructions);
#endif
        if (Status == STATUS_EXIT)
#ifdef DEBUG
#ifdef DEBUGONHALT
          Debug = 1;
        Z80debug();
#endif
#endif
        break;
#ifdef USE_PUN
        if (pun_dev)
          _sys_fflush(pun_dev);
#endif
#ifdef USE_LST
        if (lst_dev)
          _sys_fflush(lst_dev);
#endif
      }
    } else {
      _puts("Unable to load CP/M CCP.\r\nCPU halted.\r\n");
    }
  } else {
    _puts("Unable to initialize SD card.\r\nCPU halted.\r\n");
  }
}

void loop(void) {
  if (Status == STATUS_EXIT) load_hex();
  while (1) {
  }
}
