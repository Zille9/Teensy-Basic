////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    LUA für Teensy 4.1  für TEENSY 4.1                                                                          //
//                                    for VGA monitor output - Juni 2026                                                                          //
//      mit folgenden Features: Fliesskomma-Arithmetik mit double-Präzision                                                                       //
//                              Editor für Skripte mit :-Farb-Syntax                                                                              //
//                                                      -Suchfunktion                                                                             //
//                                                      -Block-Kopier und Einfügefunktion                                                         //
//                                                      -Block-Löschfunktion                                                                      //
//                              Farb- und Grafikfunktionen                                                                                        //
//                              mathematische Funktionen                                                                                          //
//                              SD-Card-Funktionen                                                                                                //
//                              Flashloader um zum Basic zurückzukehren                                                                           //
//                                                                                                                                                //
//      von:Reinhard Zielinski <zille09@gmail.com>                                                                                                //
//                                                                                                                                                //
//      Connections: SD-Card -> Builtin                                                                                                           //
//                   VGA-Beschaltung: R: 3(2k), 4(1k), 33(470) | G:11(2k), 13(1k), 2(470) | B:10(820), 12(390) | HSync:15(82) | VSync:8 (82)      //
//                   PCM5102 : BCK: 21, DIN 7, LCK 20  - Kompatibilität zum MCUME-Projekt                                                         //
//                                                                                                                                                //
//                                                                                                                                                //
//                                                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <VGA_t4.h>
#include <USBHost_t36.h> // Offizielle USB-Host Bibliothek für Teensy 4.1
#include <SD.h>
#include <SPI.h>
#include <cmath>

#include "FXUtil.h"     // Für die originale update_firmware() Funktion
extern "C" {
#include "FlashTxx.h"   // Für firmware_buffer_init() und firmware_buffer_free()
}
// SD-Karte des Teensy 4.1
const int chipSelect = BUILTIN_SDCARD;
// Das Arbeitsverzeichnis standardmässig auf /lua/
String currentWorkDir = "/lua/";


extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
// Dummy-Implementierungen für fehlende POSIX-Systemaufrufe (Newlib-Stubs)
extern "C" {
  int _open(const char *path, int flags, ...) {
    return -1;
  }
  int _getpid(void) {
    return 1;
  }
  int _times(void *buf) {
    return -1;
  }
  int _unlink(const char *pathname) {
    return -1;
  }
  int _link(const char *oldpath, const char *newpath) {
    return -1;
  }
  int _kill(int pid, int sig) {
    return -1;
  }
}
//***************************************** Tastatur ********************************************

// 1. USB Host und Keyboard Controller initialisieren
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
KeyboardController keyboard1(myusb);
USBHIDParser hid1(myusb);

volatile int lastUsbChar = -1;              //Merker für die letzte gedrückte Taste

// Timing-Konstanten
const int REPEAT_DELAY = 500; // Millisekunden bis zur ersten Wiederholung
const int REPEAT_RATE = 80;   // Geschwindigkeit der Wiederholung danach

// Zustandsvariablen
uint8_t currentKey = 0;       // Die aktuell gehaltene Taste
uint8_t currentMod = 0;       // Shift, Ctrl, etc.
elapsedMillis repeatTimer;    // Interner Timer
bool repeatPhase = false;     // Ob wir bereits in der Wiederholungsphase sind
// Globale Variablen für das Gedächtnis des Repeat-Systems
int lastRepeatUnicode = 0;    // Speichert den Buchstaben (z.B. 'A' oder 65)
uint8_t lastRepeatMod = 0;    // Speichert Shift/AltGr Zustand
uint8_t lastRepeatKeycode = 0;// Speichert die physische Taste (z.B. 40 für Enter)
bool break_marker = false;    // ESC Taste


//***************************************** VGA *************************************************

VGA_T4 vga;
lua_State *L;
String inputBuffer = "";
static int fb_width, fb_height;

unsigned long lastCursorBlink = 0;
const int blinkInterval = 250; // Blink-Geschwindigkeit in Millisekunden (250ms an / 250ms aus)
bool cursorVisible = false;


// Terminal-Größe
const int TERM_COLS = 80;
const int TERM_ROWS = 60;
EXTMEM char termBuffer[TERM_ROWS][TERM_COLS];
int cursorX = 0;
int cursorY = 0;

#define BLUE       11
#define LIGHT_BLUE 83
#define WHITE      255
#define YELLOW     252
#define MAGENTA    163
#define CYAN       127
#define GREEN      24
#define RED        196
#define GRAY       114
#define ORANGE     240
#define DARKGRAY   73

int fColor = WHITE;
int bColor = DARKGRAY;

const int MAX_C = 640 / 8;                 //Anzahl Textspalten
const int MAX_R = (480 / 8) - 1;           //Anzahl Textzeilen

//***************************************** EDITOR **********************************************
#define EDITOR_MAX_SIZE (128 * 1024)          // 128 KB im PSRAM reservieren
EXTMEM char editorBuffer[EDITOR_MAX_SIZE];

#define CLIPBOARD_SIZE (2 * 1024)             // 2 KB Puffer kopierte Zeile reservieren
EXTMEM char editorClipboard[CLIPBOARD_SIZE];
//***********************************************************************************************

// 5. Hilfsfunktion: Macht aus "skript.lua" automatisch "/pfad/skript.lua"
FLASHMEM String resolve_lua_path(String filename) {
  if (filename.startsWith("/")) {
    return filename; // Wenn es absolut mit / beginnt, so lassen
  }
  return currentWorkDir + filename; // Ansonsten das aktuelle Verzeichnis davorhängen
}

//********************************** Lua-Datei ausführen ****************************************
void executeLuaFile(const char* filename) {
  // 1. KORREKTUR: Den Dateinamen durch den Pfad-Resolver jagen (Arbeitsverzeichnis ergänzen)
  String vollerPfad = resolve_lua_path(filename);

  if (!SD.exists(vollerPfad.c_str())) {
    String errMsg = "Fehler: " + vollerPfad + " existiert nicht!\n\r";
    Serial.print(errMsg);
    vga_print_str(errMsg.c_str());
    return;
  }

  File luaFile = SD.open(vollerPfad.c_str(), FILE_READ);
  if (luaFile) {
    size_t fileSize = luaFile.size();

    // 2. KORREKTUR: Keinen String verwenden! Speicher blockweise reservieren (keine Fragmentierung)
    char* fileBuffer = (char*)malloc(fileSize + 1);
    if (fileBuffer == NULL) {
      String errMsg = "Fehler: Zu wenig RAM zum Laden von " + vollerPfad + "!\n\r";
      Serial.print(errMsg);
      vga_print_str(errMsg.c_str());
      luaFile.close();
      return;
    }

    // Datei in einem Rutsch in den Puffer einlesen
    luaFile.readBytes(fileBuffer, fileSize);
    fileBuffer[fileSize] = '\0';
    luaFile.close();

    if (luaL_loadbuffer(L, fileBuffer, fileSize, vollerPfad.c_str()) == LUA_OK) {
      if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
        String errMsg = "Laufzeitfehler in " + vollerPfad + ": " + String(lua_tostring(L, -1)) + "\n\r";
        Serial.print(errMsg);
        vga_print_str(errMsg.c_str());
        lua_pop(L, 1);
      }
    } else {
      String errMsg = "Syntaxfehler in " + vollerPfad + ": " + String(lua_tostring(L, -1)) + "\n\r";
      Serial.print(errMsg);
      vga_print_str(errMsg.c_str());
      lua_pop(L, 1);
    }

    // Speicher freigeben
    free(fileBuffer);

  } else {
    String errMsg = "Fehler beim Oeffnen von " + vollerPfad + "\n\r";
    Serial.print(errMsg);
    vga_print_str(errMsg.c_str());
  }
}

// C++ Wrapper für den Lua-Befehl: dofile("name.lua")
static int lua_dofile(lua_State *L) {
  const char* filename = luaL_checkstring(L, 1);
  executeLuaFile(filename);
  return 0;
}

//************************************* Bildschirmsteuerung ***************************************
void scrollTerminal() {
  for (int r = 0; r < TERM_ROWS - 1; r++) {
    memcpy(termBuffer[r], termBuffer[r + 1], TERM_COLS);
  }
  memset(termBuffer[TERM_ROWS - 1], '\0', TERM_COLS);
  for (int r = 0; r < TERM_ROWS; r++) {
    for (int c = 0; c < TERM_COLS; c++) {
      char singleCharStr[] = { termBuffer[r][c] != '\0' ? termBuffer[r][c] : ' ', '\0' };
      vga.drawText(c * 8, r * 8, singleCharStr, fColor, bColor, false);
    }
  }
  cursorY = TERM_ROWS - 1;
}


void vga_print_str(const char* str) {
  while (*str) {
    if (*str == '\n') {
      cursorX = 0;
      cursorY++;
      if (cursorY >= TERM_ROWS) scrollTerminal();
    } else if (*str == '\r') {
      cursorX = 0;
    } else {
      termBuffer[cursorY][cursorX] = *str;
      char echo[] = { *str, '\0' };
      vga.drawText(cursorX * 8, cursorY * 8, echo, fColor, bColor, false);

      cursorX++;
      if (cursorX >= TERM_COLS) {
        cursorX = 0;
        cursorY++;
        if (cursorY >= TERM_ROWS) scrollTerminal();
      }
    }
    str++;
  }
}




// ************** Zentrale Funktion zur Verarbeitung aller Zeichen (Egal ob USB oder Serial) ****************

void handleIncomingChar(char c) {

  vga.drawText(cursorX * 8, cursorY * 8, " ", fColor, bColor, false);   // Cursor an der alten Position löschen
  if (c == '\n' || c == '\r') {                                         // Enter / Return
    vga_print_str("\n");
    Serial.println();

    if (inputBuffer.length() > 0) {
      int status = luaL_dostring(L, inputBuffer.c_str());
      if (status != LUA_OK) {
        String errMsg = "Fehler: " + String(lua_tostring(L, -1)) + "\n";
        Serial.print(errMsg);
        vga_print_str(errMsg.c_str());
        lua_pop(L, 1);
      }
      inputBuffer = "";
    }
    vga_print_str("> ");
  }

  else if (c == 8 || c == 127) {                                        // Backspace
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      if (cursorX > 0) {
        cursorX--;
        termBuffer[cursorY][cursorX] = '\0';
        vga.drawText(cursorX * 8, cursorY * 8, " ", fColor, bColor, false);
      }
    }
  }
  else if (c >= 32 && c <= 126) {
    inputBuffer += c;
    termBuffer[cursorY][cursorX] = c;

    // SOFORT ZEICHNEN: Das Zeichen direkt auf den Schirm bringen
    char echo[] = { c, '\0' };
    vga.drawText(cursorX * 8, cursorY * 8, echo, fColor, bColor, false);
    Serial.print(c);

    cursorX++;
    if (cursorX >= TERM_COLS) {
      cursorX = 0;
      cursorY++;
      if (cursorY >= TERM_ROWS) scrollTerminal();
    }
  }
}

FLASHMEM static uint16_t wait_key(bool modes) {
  if (modes) {
    vga_print_str("\n\r");
    vga_print_str("SPACE<Continue> / ESC <Exit>\n\r");
  }
  while (1) {
    if (lastUsbChar != -1) {
      uint16_t c = lastUsbChar;
      lastUsbChar = -1;
      return c;
    }
  }
}

void OnPress(int unicode, uint8_t modifier, uint8_t keycode) {

  if (lastRepeatKeycode != 0) {   //verhindert weiterlaufen der Repeatfunktion, wenn sich zwei Tasten überlappen
    lastRepeatKeycode = 0;
    repeatPhase = false;
  }
  // Speichern für Auto-Repeat
  lastRepeatUnicode = unicode;
  lastRepeatMod = modifier;
  lastRepeatKeycode = keycode;
  repeatTimer = 0;
  repeatPhase = false;

  // Deine bestehende Logik ausführen
  process_keyboard_logic(unicode, modifier, keycode);

}

void OnRelease(int unicode, uint8_t modifier, uint8_t keycode) {
  lastRepeatKeycode = 0; // Stop Repeat
}

void handleRepeat() {
  if (lastRepeatKeycode == 0) return;

  if (!repeatPhase) {
    if (repeatTimer >= REPEAT_DELAY) {
      repeatPhase = true;
      repeatTimer = 0;
      process_keyboard_logic(lastRepeatUnicode, lastRepeatMod, lastRepeatKeycode);
    }
  } else {
    if (repeatTimer >= REPEAT_RATE) {
      repeatTimer = 0;
      process_keyboard_logic(lastRepeatUnicode, lastRepeatMod, lastRepeatKeycode);
    }
  }
}

void process_keyboard_logic(int unicode, uint8_t mod, uint8_t keycode) {
  bool shift = (mod & 0x02) || (mod & 0x20);
  bool altGr = (mod & 0x40);

  if (altGr) {
    switch (keycode) {
      case 36: lastUsbChar = 0x7B; return;
      case 37: lastUsbChar = 0x5B; return;
      case 38: lastUsbChar = 0x5D; return;
      case 39: lastUsbChar = 0x7D; return;
      case 100: lastUsbChar = 0x7C; return;
    }
  }

  if (unicode > 31 || unicode == 9) {   //normale Textzeichen oder Tab-Taste
    lastUsbChar = unicode;
    return;
  }

  switch (keycode) {
    case 40: lastUsbChar = 13; return;
    case 42: lastUsbChar = 8;  return;
    case 41: lastUsbChar = 27; break_marker = true; return;
    case 50: lastUsbChar = 35; return;
    case 53: lastUsbChar = '^'; return;
    case 100: lastUsbChar = (shift ? '>' : '<'); return;
    // Hier kannst du die Pfeiltasten ergänzen:
    case 82: lastUsbChar = 11; return; // Up
    case 81: lastUsbChar = 10; return; // Down
    case 80: lastUsbChar = 21; return; // Left
    case 79: lastUsbChar = 6;  return; // Right
  }
}

//************************************* Lua-Print *************************************
static int lua_custom_print(lua_State *L) {
  int n = lua_gettop(L);
  String outStr = "";
  for (int i = 1; i <= n; i++) {
    if (i > 1) outStr += "\t";
    if (lua_isstring(L, i)) outStr += lua_tostring(L, i);
    else if (lua_isboolean(L, i)) outStr += (lua_toboolean(L, i) ? "true" : "false");
    else outStr += lua_typename(L, lua_type(L, i));
  }
  outStr += "\n";
  Serial.print(outStr);
  vga_print_str(outStr.c_str());
  return 0;
}



//************************************* Lua-Delay ***********************************************

static int lua_delay(lua_State *L) {
  delay(luaL_checkinteger(L, 1));
  return 0;
}

//************************************* Lua-Inkey ***********************************************
// Globale Lua-Funktion: inkey() - Gibt den gedrückten Tastencode zurück oder -1
FLASHMEM int lua_global_inkey(lua_State* L) {
  int taste = inchar(); // Nutzt Ihre funktionierende, entblockte inchar()-Logik
  lua_pushinteger(L, taste);
  return 1; // 1 Rückgabewert an Lua
}


FLASHMEM void load_hex() {
  uint32_t buffer_addr, buffer_size;

  vga_print_str("INITIALISIERE MULTIBOOT-PUFFER...\r");
  delay(500);

  // 1. Offiziellen FlasherX-Speicherpuffer anfordern
  firmware_buffer_init(&buffer_addr, &buffer_size);

  // KRITISCH: Wenn die Größe 0 ist, abbrechen!
  if (buffer_size == 0) {
    vga_print_str("Fehler: Flash voll! buffer_size ist 0.\r");
    return;
  }


  // 2. Datei über die bereits laufende SD-Instanz öffnen
  File hexFile = SD.open("basic.hex", FILE_READ);
  Serial.printf("Datei Groesse: %d Bytes\n", hexFile.size());
  if (!hexFile) {
    vga_print_str("HEX-Datei nicht gefunden!\r");
    firmware_buffer_free(buffer_addr, buffer_size);
    return;
  }

  vga_print_str("FIRMWARE-TRANSFER STARTET...\r");
  delay(200);

  // 3. Das originale FlasherX übernimmt: Einlesen, RAM-Kopieren, Reboot
  update_firmware(&hexFile, &Serial, buffer_addr, buffer_size);

  // Fallback: Nur wenn die HEX-Datei fehlerhaft/unvollständig war, läuft der Code hier weiter
  hexFile.close();
  vga_print_str("FEHLER: Hex-Struktur ungueltig. Reboot...\r");

  firmware_buffer_free(buffer_addr, buffer_size);
  delay(1000);

  REBOOT; // Zurück ins stabile BASIC booten
}

//***************************************** EDITOR **********************************************

FLASHMEM int lua_cmd_edit(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushstring(L, "Fehler: Dateiname fehlt! Nutzen Sie: edit(\"skript.lua\")");
    lua_error(L);
    return 0;
  }
  String rawName = lua_tostring(L, 1);

  // KORREKTUR: Aus "test.lua" wird im Hintergrund "/lua/test.lua"
  String vollerPfad = resolve_lua_path(rawName);

  //String fileNameStr = lua_tostring(L, 1);

  open_fullscreen_editor(vollerPfad);
  vga.clear(bColor);
  delay(10);

  if (strlen(editorBuffer) > 0) {
    vga_print_str("Kompiliere und starte Lua-Skript aus PSRAM...\n\r");

    if (luaL_loadbuffer(L, editorBuffer, strlen(editorBuffer), vollerPfad.c_str()) == LUA_OK) {
      if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {   // Code ausführen

        const char* error_msg = lua_tostring(L, -1);
        vga_print_str("Lua-Laufzeitfehler: ");
        vga_print_str(error_msg);
        vga_print_str("\n\r");
        lua_pop(L, 1);                                  // Fehler vom Stack entfernen
      }

    } else {
      const char* error_msg = lua_tostring(L, -1);
      vga_print_str("Lua-Syntaxfehler: ");
      vga_print_str(error_msg);
      vga_print_str("\n\r");
      lua_pop(L, 1);                                    // Fehler vom Stack entfernen
    }
  } else {
    vga_print_str("Skript ist leer, keine Ausfuehrung.\n\r");
  }

  return 0;
}

static int inchar()
{
  myusb.Task();
  handleRepeat();
  if (lastUsbChar != -1) {
    int c = lastUsbChar;
    lastUsbChar = -1;
    return c;
  }
  delayNanoseconds(500);
  yield();
  return -1; // Sofort zurück
}


FLASHMEM void open_fullscreen_editor(String filename) {
  int tastenCode = -1;

  extern uint8_t external_psram_size;
  if (external_psram_size == 0) {
    vga_print_str("Fehler: Kein PSRAM verbaut!\r\n");
    delay(2000);
    return;
  }

  // 1. Puffer leeren und Datei von SD laden
  memset(editorBuffer, 0, EDITOR_MAX_SIZE);
  if (SD.exists(filename.c_str())) {
    File file = SD.open(filename.c_str(), FILE_READ);
    if (file) {
      size_t bytesRead = file.readBytes(editorBuffer, EDITOR_MAX_SIZE - 1);
      editorBuffer[bytesRead] = '\0';
      file.close();
    }
  }

  // 2. Editor-Bildschirm-Variablen initialisieren
  vga.clear(bColor);

  int textLength = strlen(editorBuffer);
  int cursorIdx = 0; // Synchroner Start am Dateianfang
  bool isEditing = true;

  int cursorX = 0;
  int cursorY = 16;
  int logischeZeile = 1;
  int logischeSpalte = 1;
  int blockStartIdx = -1; // -1 Aktuell kein Textblock markiert
  
  // VISUELLES SEITENFENSTER (X/Y-Scrolling)
  int startLine = 1;
  int maxSichtbareZeilen = MAX_R - 2;
  int startCol = 1;
  int maxSichtbareSpalten = MAX_C;

  // NEU: Ermittelt blitzschnell die exakte Syntax-Farbe für ein bestimmtes Zeichen im Puffer
  auto getCharColorAt = [&](int targetIdx) -> int {
    if (targetIdx < 0 || targetIdx >= textLength) return fColor;

    // 1. Den Anfang der aktuellen Zeile finden, um den Lexer zu starten
    int startIdx = targetIdx;
    while (startIdx > 0 && editorBuffer[startIdx - 1] != '\n') {
      startIdx--;
    }

    // 2. Die Zeile von vorne bis zu unserem Ziel-Index analysieren
    int charIdx = startIdx;
    bool insideString = false;
    bool insideComment = false;
    char quoteChar = '\0';
    int detectedColor = fColor;

    while (charIdx <= targetIdx) {
      char ch = editorBuffer[charIdx];
      detectedColor = fColor; // Standard

      // Kommentare prüfen
      if (!insideString && !insideComment && ch == '-' && editorBuffer[charIdx + 1] == '-') {
        insideComment = true;
      }
      if (insideComment) {
        detectedColor = 24; // GREEN
      }
      // Strings prüfen
      else if (!insideComment) {
        if ((ch == '"' || ch == '\'') && !insideString) {
          insideString = true; quoteChar = ch; detectedColor = 240;
        } else if (insideString && ch == quoteChar) {
          insideString = false; detectedColor = 240;
        } else if (insideString) {
          detectedColor = 240; // ORANGE
        }
      }

      // Lua-Keywords prüfen (Kategorie A)
      if (!insideString && !insideComment) {
        bool wordStart = (charIdx == startIdx || isspace((unsigned char)editorBuffer[charIdx - 1]) || ispunct((unsigned char)editorBuffer[charIdx - 1]));
        if (wordStart) {
          const char* primaryKeywords[] = {"function", "local", "if", "then", "else", "elseif", "end", "for", "while", "do", "return", "break", "true", "false", "nil", "and", "or", "not", "in", "repeat", "until", "require"};
          for (const char* kw : primaryKeywords) {
            size_t kwLen = strlen(kw);
            if (strncmp(&editorBuffer[charIdx], kw, kwLen) == 0) {
              char nextChar = editorBuffer[charIdx + kwLen];
              if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                if (targetIdx >= charIdx && targetIdx < (int)(charIdx + kwLen)) {
                  return 196; // RED (Es liegt auf einem Keyword)
                }
              }
            }
          }

          // System-Keywords prüfen (Kategorie B)
          const char* systemKeywords[] = {"vga", "sd", "math", "hardware_control", "delay", "print", "type", "pairs", "ipairs", "tostring", "tonumber", "error", "assert", "sqrt", "sin", "cos", "tan", "abs", "floor", "ceil", "random", "min", "max", "pi", "inkey"};
          for (const char* skw : systemKeywords) {
            size_t skwLen = strlen(skw);
            if (strncmp(&editorBuffer[charIdx], skw, skwLen) == 0) {
              char nextChar = editorBuffer[charIdx + skwLen];
              if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                if (targetIdx >= charIdx && targetIdx < (int)(charIdx + skwLen)) {
                  return 127; // CYAN (Es liegt auf einer Funktion)
                }
              }
            }
          }
        }
      }

      charIdx++;
    }
    return detectedColor;
  };
  // HILFSFUNKTION A: Berechnet die mathematischen Textpositionen live aus dem cursorIdx
  auto berechneLogischePosition = [&]() {
    int tempZeile = 1;
    int tempSpalte = 1;
    for (int i = 0; i <= textLength; i++) {
      if (i == cursorIdx) {
        logischeZeile = tempZeile;
        logischeSpalte = tempSpalte;
        break;
      }
      char ch = editorBuffer[i];
      if (ch == '\n') {
        tempZeile++;
        tempSpalte = 1;
      }
      else if (ch != '\0') {
        tempSpalte++;
      }
    }

    // Vertikales Scrolling (Y) anpassen
    if (logischeZeile >= startLine + maxSichtbareZeilen) {
      startLine = logischeZeile - maxSichtbareZeilen + 1;
    }
    if (logischeZeile < startLine) {
      startLine = logischeZeile;
    }

    // Horizontales X-Scrolling für überlange Zeilen anpassen
    if (logischeSpalte >= startCol + maxSichtbareSpalten) {
      startCol = logischeSpalte - maxSichtbareSpalten + 1;
    }
    if (logischeSpalte < startCol) {
      startCol = logischeSpalte;
    }
  };

  // HILFSFUNKTION B: Das komplette Fenster mit X/Y-Ausschnitt auf den Monitor zeichnen
  auto redrawScreen = [&]() {
    berechneLogischePosition();

    // Textfeld komplett löschen
    vga.drawRect(0, 16, MAX_C * 8, (MAX_R - 2) * 8, bColor);

    // Obere Statuszeile zusammenbauen (128 Byte Array-Fix)
    char statusBuf[128];
    float kbSize = (float)textLength / 1024.0f;
    snprintf(statusBuf, sizeof(statusBuf), " LUA-EDIT | Ln: %-3d Col: %-2d | Size: %.1f KB | Datei: %s", logischeZeile, logischeSpalte, kbSize, filename.c_str());
    int len = strlen(statusBuf); while (len < MAX_C) {
      statusBuf[len++] = ' ';
    } statusBuf[len] = '\0';
    vga.drawText(0, 0, statusBuf, bColor, fColor, false);

    // Untere Infoleiste zeichnen
    char infoBuf[128];
    snprintf(infoBuf, sizeof(infoBuf), " F1:Save/Exit  F2:Find  F3:Copy  F4:Paste  ESC:Abort ");
    int infoLen = strlen(infoBuf); while (infoLen < MAX_C) {
      infoBuf[infoLen++] = ' ';
    } infoBuf[infoLen] = '\0';
    int infoY = (MAX_R) * 8;
    vga.drawText(0, infoY, infoBuf, bColor, fColor, false);

    // Sichtbaren Text zeilenweise im X/Y-Ausschnitt rendern
    int currentY = 16;
    int aktuelleDruckZeile = 1;
    char* lineStart = editorBuffer;

    while (*lineStart && currentY < infoY) {
      char* lineEnd = strchr(lineStart, '\n');
      if (lineEnd != NULL) *lineEnd = '\0'; // Temporär kappen

      // Befindet sich diese Textzeile im vertikalen Sichtfenster?
      if (aktuelleDruckZeile >= startLine && aktuelleDruckZeile < startLine + maxSichtbareZeilen) {
        int zeilenLaenge = strlen(lineStart);

        // Hat die Zeile überhaupt genug Zeichen für den aktuellen horizontalen Scroll-Ausschnitt?
        if (zeilenLaenge >= startCol) {
          char* sichtbarerText = &lineStart[startCol - 1];

          char tempRowBuf[128]; // Auf 128 Byte dimensioniertes Zeichen-Array
          strncpy(tempRowBuf, sichtbarerText, maxSichtbareSpalten);
          tempRowBuf[maxSichtbareSpalten] = '\0';

          // SYNTAX HIGHLIGHTING (Zeichenweise verarbeiten)
          int printX = 0;
          int charIdx = 0;
          bool insideString = false;
          bool insideComment = false;
          char quoteChar = '\0';

          while (tempRowBuf[charIdx] != '\0') {
            char ch = tempRowBuf[charIdx];
            int activeColor = fColor;

            if (!insideString && !insideComment && ch == '-' && tempRowBuf[charIdx + 1] == '-') {
              insideComment = true;
            }
            if (insideComment) {
              activeColor = 24;  // GREEN
            }
            else if (!insideComment) {
              if ((ch == '"' || ch == '\'') && !insideString) {
                insideString = true;
                quoteChar = ch;
                activeColor = 240;
              }
              else if (insideString && ch == quoteChar) {
                insideString = false;
                activeColor = 240;
              }
              else if (insideString) {
                activeColor = 240;  // ORANGE
              }
            }

            if (!insideString && !insideComment) {
              bool wordStart = (charIdx == 0 || isspace((unsigned char)tempRowBuf[charIdx - 1]) || ispunct((unsigned char)tempRowBuf[charIdx - 1]));
              if (wordStart) {
                const char* primaryKeywords[] = {"function", "local", "if", "then", "else", "elseif", "end", 
                "for", "while", "do", "return", "break", "true", "false", "nil", "and", "or", "not", "in", 
                "repeat", "until", "require"};
                
                for (const char* kw : primaryKeywords) {
                  size_t kwLen = strlen(kw);
                  if (strncmp(&tempRowBuf[charIdx], kw, kwLen) == 0) {
                    char nextChar = tempRowBuf[charIdx + kwLen];
                    if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                      vga.drawText(printX, currentY, kw, 196, bColor, false); // RED
                      printX += kwLen * 8; charIdx += kwLen - 1; goto char_processed;
                    }
                  }
                }

                const char* systemKeywords[] = {"vga", "sd", "math", "hardware_control", "delay", "print", 
                "type", "pairs", "ipairs", "tostring", "tonumber", "error", "assert", "sqrt", "sin", 
                "cos", "tan", "abs", "floor", "ceil", "random", "min", "max", "pi", "inkey"};
                
                for (const char* skw : systemKeywords) {
                  size_t skwLen = strlen(skw);
                  if (strncmp(&tempRowBuf[charIdx], skw, skwLen) == 0) {
                    char nextChar = tempRowBuf[charIdx + skwLen];
                    if (nextChar == '\0' || isspace((unsigned char)nextChar) || ispunct((unsigned char)nextChar)) {
                      vga.drawText(printX, currentY, skw, 127, bColor, false); // CYAN
                      printX += skwLen * 8; charIdx += skwLen - 1; goto char_processed;
                    }
                  }
                }
              }
            }

            {
              char singleCharStr[2] = { ch, '\0' };
              vga.drawText(printX, currentY, singleCharStr, activeColor, bColor, false);
              printX += 8;
            }
char_processed:
            charIdx++;
          }
        }

        // Visuelle Pixelkoordinaten des Cursors relativ zum Scroll-Fenster bestimmen
        if (aktuelleDruckZeile == logischeZeile) {
          cursorX = (logischeSpalte - startCol) * 8;
          cursorY = currentY;
        }
        currentY += 8;
      } else if (aktuelleDruckZeile == logischeZeile) {
        cursorX = (logischeSpalte - startCol) * 8;
        cursorY = currentY;
      }

      if (lineEnd != NULL) *lineEnd = '\n'; // Wiederherstellen
      if (lineEnd == NULL) break;
      lineStart = lineEnd + 1;
      aktuelleDruckZeile++;
    }

    if (textLength == 0) {
      cursorX = 0;
      cursorY = 16;
    }

    // --- CURSOR ZEICHNEN ---
    //int infoY = (MAX_R - 1) * 8;
    if (cursorY >= 16 && cursorY < infoY && cursorX >= 0 && cursorX < (MAX_C * 8)) {
      char retroCursor[2] = { 127, '\0' };
      vga.drawText(cursorX, cursorY, retroCursor, fColor, bColor, false);
    }
  };

  // HILFSFUNKTION C: Berechnet nach einer Bewegung die neue Lage und prüft auf Scrolling
  auto updateCursorPositionWithWeiche = [&]() {
    int alterStartLine = startLine;
    int alterStartCol = startCol;

    // KORREKTUR: Das alte Löschen hier oben wurde restlos entfernt!

    berechneLogischePosition();

    // Wenn das Fenster scrollen muss, erzwingen wir ein volles Neuzeichnen
    if (startLine != alterStartLine || startCol != alterStartCol) {
      redrawScreen();
    } else {
      // Wenn der Ausschnitt gleich bleibt, versetzen wir nur den Cursor-Block flackerfrei
      cursorX = (logischeSpalte - startCol) * 8;
      cursorY = 16 + (logischeZeile - startLine) * 8;

      // Schnelles Update der Zahlen in der Statusleiste oben
      char statusBuf[128];
      float kbSize = (float)textLength / 1024.0f;
      snprintf(statusBuf, sizeof(statusBuf), " LUA-EDIT | Ln: %-3d Col: %-2d | Size: %.1f KB ", logischeZeile, logischeSpalte, kbSize);
      int len = strlen(statusBuf); while (len < MAX_C) {
        statusBuf[len++] = ' ';
      } statusBuf[len] = '\0';
      vga.drawText(0, 0, statusBuf, bColor, fColor, false);

      int infoY = (MAX_R) * 8;
      if (cursorY >= 16 && cursorY < infoY && cursorX >= 0 && cursorX < (MAX_C * 8)) {
        char retroCursor[2] = { 127, '\0' };
        vga.drawText(cursorX, cursorY, retroCursor, fColor, bColor, false);
      }
    }
  };

  // Den Bildschirm das erste Mal komplett zeichnen
  redrawScreen();
  lastUsbChar = -1;

  // 3. Editor-Hauptschleife
  while (isEditing) {
    tastenCode = inchar();
    if (tastenCode == -1) {
      delay(5);
      continue;
    }

    switch (tastenCode) {
      //----------- ESC ---------------------   
      case 27:  // ESC
        isEditing = false;
        break;
      //----------- F1 ----------------------  
      case 194: // F1: Speichern auf SD-Karte
        if (filename.length() > 0) {
          SD.remove(filename.c_str());
          File file = SD.open(filename.c_str(), FILE_WRITE);
          if (file) {
            file.print(editorBuffer); file.close();
            vga.clear(bColor);
            vga_print_str("Datei gespeichert.\r\n");
            delay(800);
          }
        } isEditing = false;
        break;
      //------------- F2 --------------------  
      case 195: // --- F2-TASTE: SUCHEN (FIND) ---
        {
          // 1. Obere Statuszeile für die Eingabe temporär löschen
          vga.drawRect(0, 0, MAX_C * 8, 8, bColor);
          vga.drawText(0, 0, " SUCHEN: ", bColor, fColor, false);
          
          char suchPuffer[32];
          memset(suchPuffer, 0, sizeof(suchPuffer));
          int sLen = 0;
          bool sEditing = true;
          
          // Blockierende Eingabeschleife nur für den Suchbegriff
          while (sEditing) {
            int sKey = inchar();
            if (sKey == 13 || sKey == 10) { // ENTER -> Suche starten
              sEditing = false;
            } else if (sKey == 27) { // ESC -> Abbrechen
              suchPuffer[0] = '\0';
              sEditing = false;
            } else if ((sKey == 8 || sKey == 127) && sLen > 0) { // Backspace
              sLen--;
              suchPuffer[sLen] = '\0';
              vga.drawRect(64, 0, (MAX_C - 8) * 8, 8, bColor);
              vga.drawText(64, 0, suchPuffer, bColor, fColor, false);
            } else if (sKey >= 32 && sKey <= 126 && sLen < 30) { // Zeichen tippen
              suchPuffer[sLen++] = (char)sKey;
              suchPuffer[sLen] = '\0';
              vga.drawText(64, 0, suchPuffer, bColor, fColor, false);
            }
            delay(10);
          }
          
          // 2. Die eigentliche Suche im PSRAM-Puffer ausführen
          if (strlen(suchPuffer) > 0) {
            // Sucht ab der aktuellen Position nach vorne (Find Next)
            char* fundStelle = strstr(&editorBuffer[cursorIdx + 1], suchPuffer);
            
            if (fundStelle == NULL) {
              // Falls nach dem Cursor nichts mehr kam, suche vom Anfang des Skripts
              fundStelle = strstr(editorBuffer, suchPuffer);
            }
            
            if (fundStelle != NULL) {
              // Speicheradresse in den logischen Puffer-Index umrechnen
              cursorIdx = (int)(fundStelle - editorBuffer);
              blockStartIdx = -1; // Eventuelle Markierung aufheben
            } else {
              // Fehler-Feedback unaufdringlich unten rechts ausgeben
              int infoY = MAX_R * 8;
              vga.drawText(60 * 8, infoY, "[NICHT GEFUNDEN]", bColor, fColor, false);
              delay(1000);
            }
          }
          
          // 3. Bildschirm komplett neu aufbauen (berechnet X/Y-Scrolling zum Wort automatisch!)
          redrawScreen();
        }
        break;
      //--------------- F3 ---------------------  
      case 196: // --- F3-TASTE: BLOCK MARKIEREN ODER KOPIEREN ---
        {
          int infoY = MAX_R * 8;
          
          if (blockStartIdx == -1) {
            // 1. SCHRITT: Startpunkt des Blocks auf den aktuellen logischen Index setzen
            blockStartIdx = cursorIdx;

            // Feedback unten rechts in Ihrer Palette einblenden
            vga.drawText(60 * 8, infoY, "[MARK: START]", bColor, fColor, false);
            delay(1000);
          } else {
            // 2. SCHRITT: Endpunkt setzen und kopieren
            int start = (blockStartIdx < cursorIdx) ? blockStartIdx : cursorIdx;
            int ende = (blockStartIdx < cursorIdx) ? cursorIdx : blockStartIdx;
            int blockLaenge = ende - start;

            if (blockLaenge > 0 && blockLaenge < CLIPBOARD_SIZE - 2) {
              strncpy(editorClipboard, &editorBuffer[start], blockLaenge);
              editorClipboard[blockLaenge] = '\0'; // Sicher terminieren

              vga.drawText(60 * 8, infoY, "[BLOCK KOPIERT]", bColor, fColor, false);
              delay(1000);
            }
            // Hinweis: Wir nullen blockStartIdx hier nicht, um anschließendes Ausschneiden per ENTF zu erlauben!
          }
          redrawScreen();
        }
        break;
      //-------------- F4 ---------------------
      case 197: // --- F4-TASTE: BLOCK EINFÜGEN (Paste) ---
        {
          int clipLen = strlen(editorClipboard);
          
          // Nur einfügen, wenn Inhalt im Clipboard ist und ins PSRAM passt
          if (clipLen > 0 && (textLength + clipLen) < EDITOR_MAX_SIZE - 2) {
            
            // 1. Platz schaffen im Textstream
            memmove(&editorBuffer[cursorIdx + clipLen], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
            
            // 2. Daten aus der Zwischenablage einsetzen
            memcpy(&editorBuffer[cursorIdx], editorClipboard, clipLen);
            
            // 3. Indizes und Längen um die Blockgröße nach vorne schieben
            cursorIdx += clipLen;
            textLength += clipLen;
            
            blockStartIdx = -1; // Markierung aufheben
            redrawScreen();     // Komplettes Bild inklusive Scroll-Ausschnitt neu berechnen
          }
        }
        break;

      //----------- Pfeil links --------------  
      case 216: 
        if (cursorIdx > 0) {
          // KORREKTUR: Ermittelt die exakte Syntax-Farbe für die Wiederherstellung!
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          cursorIdx--;
          updateCursorPositionWithWeiche();
        }
        break;
      //----------- Pfeil rechts -------------
      case 215: 
        if (cursorIdx < textLength) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          cursorIdx++;
          updateCursorPositionWithWeiche();
        }
        break;
      //----------- Pfeil hoch ---------------
      case 218: 
        if (cursorIdx > 0) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          int targetCol = 0; int i = cursorIdx;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            targetCol++;
            i--;
          }
          if (i > 0 && editorBuffer[i - 1] == '\n') i--;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            i--;
          }
          int currentCol = 0;
          while (editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
            currentCol++;
            i++;
          }

          cursorIdx = i;
          updateCursorPositionWithWeiche();
        }
        break;
      //------------ Pfeil runter ---------------
      case 217: 
        if (cursorIdx < textLength) {
          int korrekteFarbe = getCharColorAt(cursorIdx);
          char altChar[2] = { (char)(editorBuffer[cursorIdx] ? editorBuffer[cursorIdx] : ' '), '\0' };
          vga.drawText(cursorX, cursorY, altChar, korrekteFarbe, bColor, false);

          int targetCol = 0; int i = cursorIdx;
          while (i > 0 && editorBuffer[i - 1] != '\n') {
            targetCol++;
            i--;
          }
          i = cursorIdx;
          while (i < textLength && editorBuffer[i] != '\n') i++;
          if (i < textLength && editorBuffer[i] == '\n') i++;
          int currentCol = 0;
          while (i < textLength && editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) {
            currentCol++;
            i++;
          }

          cursorIdx = i;
          updateCursorPositionWithWeiche();
        }
        break;
      //------------- Home -----------------
      case 210:
        while (cursorIdx > 0 && editorBuffer[cursorIdx - 1] != '\n')
        {
          cursorIdx--;
        }
        updateCursorPositionWithWeiche();
        break; 
      //-------------- END -----------------
      case 213:
        while (cursorIdx < textLength && editorBuffer[cursorIdx] != '\n')
        {
          cursorIdx++;
        }
        updateCursorPositionWithWeiche();
        break; 
      //-------------- ENTER ---------------
      case 13: 
      case 10:
        if (textLength < EDITOR_MAX_SIZE - 2)
        { memmove(&editorBuffer[cursorIdx + 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = '\n';
          cursorIdx++;
          textLength++;
          redrawScreen();
        }
        break;
      //-------------- TAB -----------------
      case 9: // Fügt 2 Leerzeichen ein
        if (textLength < EDITOR_MAX_SIZE - 3)
        { memmove(&editorBuffer[cursorIdx + 2], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = ' ';
          editorBuffer[cursorIdx + 1] = ' ';
          cursorIdx += 2;
          textLength += 2;
          redrawScreen();
        }
        break;
      //------------ BACKSPACE -------------
      case 8: 
      case 127:
        if (cursorIdx > 0)
        { memmove(&editorBuffer[cursorIdx - 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          cursorIdx--;
          textLength--;
          redrawScreen();
        }
        break;
      //------------ DELETE ----------------
      case 212: // --- ENTF / DELETE: Zeichen ODER ganzen Block löschen ---
        {
          if (blockStartIdx == -1) {
            // Herkömmliches Verhalten: Einzelnes Zeichen unter dem Cursor löschen
            if (cursorIdx < textLength) {
              memmove(&editorBuffer[cursorIdx], &editorBuffer[cursorIdx + 1], textLength - cursorIdx);
              textLength--;
              redrawScreen();
            }
          } else {
            // ============================================================================
            // GANZEN MARKIERTEN BLOCK LÖSCHEN (Oder Ausschneiden abschließen)
            // ============================================================================
            int start = (blockStartIdx < cursorIdx) ? blockStartIdx : cursorIdx;
            int ende = (blockStartIdx < cursorIdx) ? cursorIdx : blockStartIdx;
            int blockLaenge = ende - start;

            if (blockLaenge > 0) {
              // 1. Text im PSRAM-Stream nahtlos zusammenziehen
              memmove(&editorBuffer[start], &editorBuffer[ende], textLength - ende + 1);
              textLength -= blockLaenge;
              cursorIdx = start; // Cursor an die Schnittstelle setzen

              // Feedback unten rechts ausgeben
              int infoY = MAX_R * 8;
              vga.drawText(60 * 8, infoY, "[BLOCK ENTFERNT]", bColor, fColor, false);
              delay(1000);
            }
            
            blockStartIdx = -1; // Markierung zurücksetzen
            redrawScreen();     // Fensterlage und X/Y-Scrolling komplett frisch ermitteln
          }
        }
        break;
      //------------ PAGE-UP ----------------  
      case 211: // --- PAGE UP / BILD AUF ---
        {
          // Wir versuchen, den Cursor um 'maxSichtbareZeilen' nach oben zu bewegen
          for (int z = 0; z < maxSichtbareZeilen; z++) {
            int targetCol = 0; int i = cursorIdx;
            while (i > 0 && editorBuffer[i - 1] != '\n') { targetCol++; i--; }
            if (i > 0 && editorBuffer[i - 1] == '\n') i--; 
            while (i > 0 && editorBuffer[i - 1] != '\n') i--;
            int currentCol = 0;
            while (editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) { currentCol++; i++; }
            cursorIdx = i;
          }
          redrawScreen(); // Kompletten Ausschnitt mit X/Y-Scrolling neu aufbauen
        }
        break;
      //-------------- PAGE-DOWN -------------
      case 214: // --- PAGE DOWN / BILD AB ---
        {
          // Wir versuchen, den Cursor um 'maxSichtbareZeilen' nach unten zu bewegen
          for (int z = 0; z < maxSichtbareZeilen; z++) {
            int targetCol = 0; int i = cursorIdx;
            while (i > 0 && editorBuffer[i - 1] != '\n') { targetCol++; i--; }
            i = cursorIdx; while (i < textLength && editorBuffer[i] != '\n') i++;
            if (i < textLength && editorBuffer[i] == '\n') i++; 
            int currentCol = 0;
            while (i < textLength && editorBuffer[i] != '\n' && editorBuffer[i] != '\0' && currentCol < targetCol) { currentCol++; i++; }
            cursorIdx = i;
          }
          redrawScreen(); // Kompletten Ausschnitt mit X/Y-Scrolling neu aufbauen
        }
        break;
        
      //------- normale Zeichen ------------
      default: 
        if (tastenCode >= 32 && tastenCode <= 126 && textLength < EDITOR_MAX_SIZE - 2)
        { memmove(&editorBuffer[cursorIdx + 1], &editorBuffer[cursorIdx], textLength - cursorIdx + 1);
          editorBuffer[cursorIdx] = (char)tastenCode;
          cursorIdx++; textLength++;
          redrawScreen();
        }
        break;
    }
  }
  vga.clear(bColor);
}


//****************************************** LUA-Brückenfunktion **********************************************
FLASHMEM int lua_hardware_control(lua_State* L) {
  // 1. Parameter-Prüfung: Wir erwarten 3 Argumente (Zahl, String, String/Zahl)
  if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
    lua_pushstring(L, "Fehler: Ungueltige Argumente! Nutzen Sie: hardware_control(pin, \"MODUS\", \"WERT\")");
    lua_error(L);
    return 0;
  }

  // 2. Werte vom Lua-Stack abholen
  int pin = (int)lua_tonumber(L, 1);
  String modus = lua_tostring(L, 2);

  // 3. Pin-Modus einstellen
  if (modus == "OUTPUT") {
    pinMode(pin, OUTPUT);
  } else if (modus == "INPUT") {
    pinMode(pin, INPUT);
  } else if (modus == "INPUT_PULLUP") {
    pinMode(pin, INPUT_PULLUP);
  }

  // 4. Wert verarbeiten (falls vorhanden)
  if (lua_gettop(L) >= 3) {
    // Wert kann als String ("HIGH"/"LOW") oder Zahl (0-255 für PWM) kommen
    if (lua_isnumber(L, 3)) {
      int analogWert = (int)lua_tonumber(L, 3);
      analogWrite(pin, analogWert); // Für PWM-Ausgabe (z.B. LEDs dimmen)
    }
    else if (lua_isstring(L, 3)) {
      String wertStr = lua_tostring(L, 3);
      if (wertStr == "HIGH" || wertStr == "1") {
        digitalWrite(pin, HIGH);
      } else if (wertStr == "LOW" || wertStr == "0") {
        digitalWrite(pin, LOW);
      }
    }
  }

  // 5. Wenn der Pin im INPUT-Modus ist, geben wir den aktuellen Zustand an Lua zurück
  if (modus == "INPUT" || modus == "INPUT_PULLUP") {
    int sensorWert = digitalRead(pin);
    lua_pushinteger(L, sensorWert); // Schiebt das Ergebnis (0 oder 1) zurück auf den Lua-Stack
    return 1; // Signalisiert Lua: Es gibt 1 Rückgabewert
  }

  return 0; // Kein Rückgabewert bei OUTPUT
}
//****************************************** LUA-Brückenfunktion SD**********************************************

// ============================================================================
// WILDCARD HILFSFUNKTION (Für sd.ls-Filterung)
// ============================================================================

FLASHMEM bool wildcard_match(const char* pattern, const char* str) {
  while (*pattern) {
    if (*pattern == '*') {
      if (!*(++pattern)) return true; // Ein einzelner Stern am Ende passt auf alles restliche
      while (*str) {
        if (wildcard_match(pattern, str)) return true;
        str++;
      }
      return false;
    } else if (*pattern == '?') {
      if (!*str) return false;
      pattern++; str++;
    } else {
      if (tolower((unsigned char)*pattern) != tolower((unsigned char)*str)) return false;
      pattern++; str++;
    }
  }
  return !*pattern && !*str;
}

// ============================================================================
// SD-KARTEN INTERFACE (Modul: sd)
// ============================================================================
// 1. Datei mit Wildcard suchen sd.ls("*lua")
FLASHMEM int lua_sd_ls(lua_State* L) {
  String searchPattern = "*"; // Standardmäßig alles anzeigen
  String path = currentWorkDir;

  // 1. Parameter auswerten
  if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
    String arg = lua_tostring(L, 1);

    // Prüfen, ob das Argument ein Wildcard (* oder ?) enthält
    if (arg.indexOf('*') != -1 || arg.indexOf('?') != -1) {
      int lastSlash = arg.lastIndexOf('/');
      if (lastSlash != -1) {
        path = arg.substring(0, lastSlash + 1);
        searchPattern = arg.substring(lastSlash + 1);
      } else {
        path = currentWorkDir;
        searchPattern = arg;
      }
    } else {
      // Wenn ein Ordnerpfad ohne Wildcard übergeben wurde, jagen wir ihn durch den Pfad-Resolver
      path = resolve_lua_path(arg);
      // Sicherstellen, dass der Pfad für die SD-Bibliothek mit einem / endet
      if (!path.endsWith("/")) path += "/";
      // Kein Wildcard -> Es ist wie bisher ein reiner Ordnerpfad
      //path = arg;
    }
  }

  File dir = SD.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    lua_pushboolean(L, false);
    return 1;
  }

  // Header ausgeben (zeigt nun auch den aktiven Filter)
  vga_print_str("--- Verzeichnis: ");
  vga_print_str(path.c_str());
  if (searchPattern != "*") {
    vga_print_str(" [Filter: ");
    vga_print_str(searchPattern.c_str());
    vga_print_str("]");
  }
  vga_print_str(" ---\n\r");

  vga_print_str("Name                     Typ      Groesse\n\r");
  vga_print_str("-----------------------------------------------\n\r");

  int zeilenZaehler = 3;

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    const char* rawName = entry.name();

    // 2. SYSTEM-FILTER: Unsichtbare Dateien immer ausblenden
    if (rawName[0] == '.' ||
        strcasecmp(rawName, "System Volume Information") == 0 ||
        strcasecmp(rawName, "FOUND.000") == 0 ||
        strncasecmp(rawName, "._", 2) == 0) {
      entry.close();
      continue;
    }

    // 3. WILDCARD-FILTER: Prüfen, ob die Datei auf das Suchmuster passt
    // (Ordner werden vom Filter ausgenommen und immer angezeigt, um navigieren zu können)
    if (!entry.isDirectory() && !wildcard_match(searchPattern.c_str(), rawName)) {
      entry.close();
      continue; // Passt nicht zum Filter -> Überspringen
    }

    // 4. BEGRENZUNG: Dateinamen formatieren und bei Bedarf kürzen
    char displayName[25];
    size_t nameLen = strlen(rawName);

    if (nameLen > 20) {
      strncpy(displayName, rawName, 18);
      displayName[18] = '.';
      displayName[19] = '.';
      displayName[20] = '\0';
    } else {
      strcpy(displayName, rawName);
    }

    // Zeilenpuffer für das saubere Spaltenlayout
    char spaltenBuf[80];
    if (entry.isDirectory()) {
      snprintf(spaltenBuf, sizeof(spaltenBuf), "%-24s <DIR>    ---", displayName);
    } else {
      char groesseStr[20];
      snprintf(groesseStr, sizeof(groesseStr), "%lu Bytes", (unsigned long)entry.size());
      snprintf(spaltenBuf, sizeof(spaltenBuf), "%-24s FILE     %s", displayName, groesseStr);
    }

    // Zeile auf VGA ausgeben
    vga_print_str(spaltenBuf);
    vga_print_str("\n\r");
    entry.close();

    zeilenZaehler++;

    // Seitenumbruch-Logik (MAX_R - 4)
    if (zeilenZaehler >= MAX_R - 4) {
      vga_print_str("-- WEITER MIT TASTE | ESC ZUM ABBRECHEN --\r");
      int taste = wait_key(1);
      delay(150);
      lastUsbChar = -1;
      vga_print_str("                                          \r");

      if (taste == 27) { // ESC gedrückt
        dir.close();
        lua_pushboolean(L, true);
        return 1;
      }
      zeilenZaehler = 0;
    }
  }

  dir.close();
  vga_print_str("-----------------------------------------------\n\r");

  lua_pushboolean(L, true);
  return 1;
}

// 2. NEU: C++ Funktion für sd.cd("pfad") in Lua
FLASHMEM int lua_sd_cd(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushstring(L, "Fehler: Pfad (String) erwartet! Nutzen Sie: sd.cd(\"/neuer_pfad\")");
    lua_error(L);
    return 0;
  }

  String neuerPfad = lua_tostring(L, 1);

  // Sicherheits-Checks für saubere Formatierung
  if (!neuerPfad.startsWith("/")) {
    neuerPfad = "/" + neuerPfad;
  }
  if (!neuerPfad.endsWith("/")) {
    neuerPfad = neuerPfad + "/";
  }

  // Prüfen, ob der Ordner auf der SD-Karte überhaupt existiert
  if (SD.exists(neuerPfad.c_str())) {
    currentWorkDir = neuerPfad; // Pfad dynamisch umschalten!

    // Statusmeldung ausgeben
    vga_print_str("Arbeitsverzeichnis geaendert auf: ");
    vga_print_str(currentWorkDir.c_str());
    vga_print_str("\n\r");
    lua_pushboolean(L, true);
  } else {
    vga_print_str("Fehler: Verzeichnis existiert nicht!\n\r");
    lua_pushboolean(L, false);
  }

  return 1;
}


// 3. Datei löschen: sd.remove("datei.lua")
FLASHMEM int lua_sd_remove(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushstring(L, "Argument muss ein Dateiname (String) sein!");
    lua_error(L);
    return 0;
  }
  const char* filename = lua_tostring(L, 1);
  bool success = SD.remove(filename);

  lua_pushboolean(L, success);
  return 1;
}

// 3. Ordner erstellen: sd.mkdir("ordnername")
FLASHMEM int lua_sd_mkdir(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushstring(L, "Argument muss ein Ordnername (String) sein!");
    lua_error(L);
    return 0;
  }
  const char* dirname = lua_tostring(L, 1);
  bool success = SD.mkdir(dirname);

  lua_pushboolean(L, success);
  return 1;
}

// 4. Ordner löschen: sd.rmdir("ordnername")
FLASHMEM int lua_sd_rmdir(lua_State* L) {
  if (!lua_isstring(L, 1)) {
    lua_pushstring(L, "Argument muss ein Ordnername (String) sein!");
    lua_error(L);
    return 0;
  }
  const char* dirname = lua_tostring(L, 1);
  bool success = SD.rmdir(dirname);

  lua_pushboolean(L, success);
  return 1;
}



// 6. sd.copy("quelle.lua", "ziel.lua") - Kopiert eine Datei im aktuellen Arbeitsverzeichnis
FLASHMEM int lua_sd_copy(lua_State* L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    lua_pushstring(L, "Fehler: Zwei Dateinamen (Strings) erwartet! Nutzen Sie: sd.copy(\"von.lua\", \"nach.lua\")");
    lua_error(L);
    return 0;
  }

  String vonPfad = resolve_lua_path(lua_tostring(L, 1));    // Pfade, Arbeitsverzeichnis automatisch ergänzen
  String nachPfad = resolve_lua_path(lua_tostring(L, 2));

  if (!SD.exists(vonPfad.c_str())) {              // Quelldatei vorhanden?
    vga_print_str("Fehler: Quelldatei existiert nicht!\n\r");
    lua_pushboolean(L, false);
    return 1;
  }

  // 3. Kopier-Vorgang starten
  File sourceFile = SD.open(vonPfad.c_str(), FILE_READ);
  if (!sourceFile) {
    vga_print_str("Fehler: Konnte Quelldatei nicht oeffnen!\n\r");
    lua_pushboolean(L, false);
    return 1;
  }

  if (SD.exists(nachPfad.c_str())) {              // Falls die Zieldatei existiert,ueberschreiben
    SD.remove(nachPfad.c_str());
  }

  File destFile = SD.open(nachPfad.c_str(), FILE_WRITE);
  if (!destFile) {
    vga_print_str("Fehler: Konnte Zieldatei nicht erstellen!\n\r");
    sourceFile.close();
    lua_pushboolean(L, false);
    return 1;
  }

  const size_t bufferSize = 512;                  // Blockweise kopieren
  uint8_t buffer[bufferSize];

  while (sourceFile.available() > 0) {
    size_t bytesRead = sourceFile.read(buffer, bufferSize);
    destFile.write(buffer, bytesRead);
  }
  destFile.close();
  sourceFile.close();

  vga_print_str("Datei erfolgreich kopiert.\n\r");
  lua_pushboolean(L, true);
  return 1;
}


//********************************************** Grafikfunktionen *************************************
// ============================================================================
// VGA GRAPHICS INTERFACE (Modul: vga)
// ============================================================================

// 1. vga.color(vordergrund, hintergrund)
FLASHMEM int lua_vga_color(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    lua_pushstring(L, "Fehler: vga.color(vordergrund, hintergrund) erwartet 2 Zahlen!");
    lua_error(L);
    return 0;
  }
  fColor = (int)lua_tonumber(L, 1);
  bColor = (int)lua_tonumber(L, 2);

  return 0;
}

// 2. vga.cls()
FLASHMEM int lua_vga_cls(lua_State* L) {
  int farbe = bColor; // Fallback auf das aktuelle globale bColor
  if (lua_gettop(L) >= 1 && lua_isnumber(L, 1)) {
    farbe = (int)lua_tonumber(L, 1);
  }
  //Bildschirm und Textpuffer löschen
  vga.clear(farbe);
  cursorX = 0;
  cursorY = 0;
  memset(termBuffer, '\0', sizeof(termBuffer));
  return 0;
}

// 3. vga.text([spalte, zeile,] "Text" [, fcolor, bcolor])
// Nutzt logische Textkoordinaten (Spalten/Zeilen) und macht Positionen optional!
FLASHMEM int lua_vga_text(lua_State* L) {
  int argumente = lua_gettop(L);
  int spalte = cursorX; // Standardwert: Aktuelle Cursor-Spalte (oder x_pos)
  int zeile = cursorY;  // Standardwert: Aktuelle Cursor-Zeile (oder y_pos)
  const char* txt = NULL;

  int txtFColor = fColor;
  int txtBColor = bColor;

  // FALL 1: Es wurden mindestens 3 Argumente uebergeben (spalte, zeile, "text" ...)
  if (argumente >= 3 && lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isstring(L, 3)) {
    spalte = (int)lua_tonumber(L, 1);
    zeile = (int)lua_tonumber(L, 2);
    txt = lua_tostring(L, 3);

    // Optionale Farben auswerten (liegen bei 4 und 5)
    if (argumente >= 4 && lua_isnumber(L, 4)) txtFColor = (int)lua_tonumber(L, 4);
    if (argumente >= 5 && lua_isnumber(L, 5)) txtBColor = (int)lua_tonumber(L, 5);
  }
  // FALL 2: Es wurde nur der Text uebergeben (Nutzt aktuelle Cursor-Position)
  else if (argumente >= 1 && lua_isstring(L, 1)) {
    txt = lua_tostring(L, 1);

    // Optionale Farben auswerten (liegen hier bei 2 und 3)
    if (argumente >= 2 && lua_isnumber(L, 2)) txtFColor = (int)lua_tonumber(L, 2);
    if (argumente >= 3 && lua_isnumber(L, 3)) txtBColor = (int)lua_tonumber(L, 3);
  }
  else {
    lua_pushstring(L, "Fehler: Ungueltige Argumente fuer vga.text!");
    lua_error(L);
    return 0;
  }

  // --- DER TRICK: Umrechnung von Spalte/Zeile in Pixel (Faktor 8) ---
  int pixelX = spalte * 8;
  int pixelY = zeile * 8;

  // Text auf die VGA-Karte zeichnen
  vga.drawText(pixelX, pixelY, txt, txtFColor, txtBColor, false);

  // OPTIONAL: Den Cursor automatisch hinter den gedruckten Text weiterschieben
  cursorX = spalte + strlen(txt);
  if (cursorX >= TERM_COLS) {
    cursorX = 0;
    cursorY++; // Einfacher Zeilenumbruch, falls der Text den Rand sprengt
  }

  return 0;
}

// 4. Pixel zeichnen: vga.pset(x, y, farbe)
FLASHMEM int lua_vga_pset(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    lua_pushstring(L, "Fehler: vga.pset(x, y, farbe) erwartet 3 Zahlen!");
    lua_error(L); return 0;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int farbe = (int)lua_tonumber(L, 3);

  vga.drawPixel(x, y, farbe);
  return 0;
}

// 5. Rechteck zeichnen: vga.box(x, y, w, h, farbe)
FLASHMEM int lua_vga_box(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    lua_pushstring(L, "Fehler: vga.box(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_error(L); return 0;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  int rColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) rColor = (int)lua_tonumber(L, 5);

  // Ruft das gefüllte Rechteck Ihrer VGA_t4-Bibliothek auf
  vga.drawRect(x1, y1, x2, y2, rColor);
  return 0;
}

// 6. LEERES RECHTECK (vga.rect): 4-Linien-Methode
FLASHMEM int lua_vga_rect(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    lua_pushstring(L, "Fehler: vga.rect(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_error(L); return 0;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  // Standardwert aus globaler Variable laden
  int rColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) rColor = (int)lua_tonumber(L, 5);

  // Zeichnet den leeren Rahmen mit Ihren 4 Linien
  vga.drawline(x1, y1, x2, y1, rColor); // Oben quer
  vga.drawline(x1, y1, x1, y2, rColor); // Links runter
  vga.drawline(x2, y1, x2, y2, rColor); // Rechts runter
  vga.drawline(x1, y2, x2, y2, rColor); // Unten quer
  return 0;
}

// 7. Gefuellte Ellipse: vga.filledellipse(x, y, w, h [, fcolor, bcolor])
FLASHMEM int lua_vga_filledellipse(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    lua_pushstring(L, "Fehler: vga.filledellipse(x, y, w, h) erwartet mindestens 4 Zahlen!");
    lua_error(L); return 0;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int w = (int)lua_tonumber(L, 3);
  int h = (int)lua_tonumber(L, 4);

  // Standardwerte aus globalen Variablen laden
  int ellFColor = fColor;
  int ellBColor = bColor;

  // Optionale Farben überschreiben, falls vom Benutzer in Lua übergeben
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) ellFColor = (int)lua_tonumber(L, 5);
  if (lua_gettop(L) >= 6 && lua_isnumber(L, 6)) ellBColor = (int)lua_tonumber(L, 6);

  vga.drawfilledellipse(x, y, w, h, ellFColor, ellBColor);
  return 0;
}

// 8. Leere Ellipse: vga.ellipse(x, y, w, h [, fcolor])
FLASHMEM int lua_vga_ellipse(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    lua_pushstring(L, "Fehler: vga.ellipse(x, y, w, h) erwartet mindestens 4 Zahlen!");
    lua_error(L); return 0;
  }
  int x = (int)lua_tonumber(L, 1);
  int y = (int)lua_tonumber(L, 2);
  int w = (int)lua_tonumber(L, 3);
  int h = (int)lua_tonumber(L, 4);

  // Standardwert aus globaler Variable laden
  int ellFColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) ellFColor = (int)lua_tonumber(L, 5);

  vga.drawellipse(x, y, w, h, ellFColor);
  return 0;
}

// 9. EINE EINZELNE LINIE (vga.line(x1, y1, x2, y2 [, farbe])
FLASHMEM int lua_vga_line(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
    lua_pushstring(L, "Fehler: vga.line(x1, y1, x2, y2 [, farbe]) erwartet mindestens 4 Zahlen!");
    lua_error(L); return 0;
  }
  int x1 = (int)lua_tonumber(L, 1);
  int y1 = (int)lua_tonumber(L, 2);
  int x2 = (int)lua_tonumber(L, 3);
  int y2 = (int)lua_tonumber(L, 4);

  int lColor = fColor;
  if (lua_gettop(L) >= 5 && lua_isnumber(L, 5)) lColor = (int)lua_tonumber(L, 5);

  vga.drawline(x1, y1, x2, y2, lColor);
  return 0;
}

//10. vga.pos(spalte, zeile) zum Setzen der Cursor-Position auf der Konsole
FLASHMEM int lua_vga_pos(lua_State* L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    lua_pushstring(L, "Fehler: vga.pos(spalte, zeile) erwartet 2 Zahlen!");
    lua_error(L);
    return 0;
  }

  int spalte = (int)lua_tonumber(L, 1);
  int zeile = (int)lua_tonumber(L, 2);

  // Grenzen absichern, damit der Cursor nicht außerhalb des Bildschirms landet
  if (spalte < 0) spalte = 0;
  if (spalte >= TERM_COLS) spalte = TERM_COLS - 1;
  if (zeile < 0) zeile = 0;
  if (zeile >= TERM_ROWS) zeile = TERM_ROWS - 1;

  cursorX = spalte;
  cursorY = zeile;

  return 0;
}

// vga.print(wert) - Gibt Zahlen oder Strings ohne Zeilenumbruch aus
FLASHMEM int lua_vga_print(lua_State* L) {

  if (lua_gettop(L) < 1) {                          // Prüfen, ob überhaupt ein Argument übergeben wurde
    return 0;
  }

  if (lua_isnumber(L, 1)) {                         // Lua-Typ abfragen und passend konvertieren
    double num = lua_tonumber(L, 1);
    char buf[32];

    if (num == (int)num) {                          // Prüfen, ob es eine Ganzzahl oder Fließkommazahl ist für saubere Optik
      snprintf(buf, sizeof(buf), "%d", (int)num);
    } else {
      snprintf(buf, sizeof(buf), "%f", num);
    }
    vga_print_str(buf);
  }
  else if (lua_isstring(L, 1)) {
    const char* txt = lua_tostring(L, 1);
    vga_print_str(txt);
  }

  return 0;                                         // Kein Rückgabewert an Lua
}
//######################################################## SETUP #######################################################
void setup() {
  Serial.begin(9600);
  delay(200);
  myusb.begin();
  keyboard1.attachPress(OnPress);
  keyboard1.attachRelease(OnRelease); // Stoppt das Repeat-System

  // VGA Setup
  vga_error_t err = vga.begin(VGA_MODE_640x480);//352x240);
  vga.get_frame_buffer_size(&fb_width, &fb_height);

  memset(termBuffer, '\0', sizeof(termBuffer));
  vga.clear(bColor);

  // SD-Karte initialisieren
  if (!SD.begin(chipSelect)) {
    vga_print_str("SD-Karte: FEHLGESCHLAGEN\n");
  }

  // Lua Setup
  L = luaL_newstate();
  luaL_openlibs(L);

  lua_register(L, "print", lua_custom_print);
  lua_register(L, "delay", lua_delay);
  lua_register(L, "edit", lua_cmd_edit);
  lua_register(L, "hardware_control", lua_hardware_control);
  lua_register(L, "run", lua_dofile);
  lua_register(L, "exit", load_hex);
  lua_register(L, "inkey", lua_global_inkey);

  // Eine neue Tabelle für die SD-Bibliothek in Lua erstellen
  lua_newtable(L);
  // Die C++ Funktionen der Tabelle zuweisen
  lua_pushcfunction(L, lua_sd_ls);     lua_setfield(L, -2, "ls");
  lua_pushcfunction(L, lua_sd_remove); lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, lua_sd_mkdir);  lua_setfield(L, -2, "mkdir");
  lua_pushcfunction(L, lua_sd_rmdir);  lua_setfield(L, -2, "rmdir");
  lua_pushcfunction(L, lua_sd_cd);     lua_setfield(L, -2, "cd");
  lua_pushcfunction(L, lua_sd_copy);   lua_setfield(L, -2, "copy");

  // Die Tabelle global unter dem Namen "sd" registrieren
  lua_setglobal(L, "sd");
  srand(analogRead(0) + micros());  // Einmaliger Seed für die Zufallszahlen beim Systemstart

  // Neue Tabelle für die VGA-Grafik erstellen
  lua_newtable(L);
  lua_pushcfunction(L, lua_vga_color);         lua_setfield(L, -2, "color");
  lua_pushcfunction(L, lua_vga_pset);          lua_setfield(L, -2, "pset");
  lua_pushcfunction(L, lua_vga_line);          lua_setfield(L, -2, "line");
  lua_pushcfunction(L, lua_vga_rect);          lua_setfield(L, -2, "rect");
  lua_pushcfunction(L, lua_vga_box);           lua_setfield(L, -2, "box");
  lua_pushcfunction(L, lua_vga_ellipse);       lua_setfield(L, -2, "ellipse");
  lua_pushcfunction(L, lua_vga_filledellipse); lua_setfield(L, -2, "fillellipse");
  lua_pushcfunction(L, lua_vga_text);          lua_setfield(L, -2, "text");
  lua_pushcfunction(L, lua_vga_cls);           lua_setfield(L, -2, "cls");
  lua_pushcfunction(L, lua_vga_pos);           lua_setfield(L, -2, "pos");
  lua_pushcfunction(L, lua_vga_print);         lua_setfield(L, -2, "print");         // <-- NEU!

  lua_setglobal(L, "vga");        // Die Tabelle "vga" registrieren

  start_screen();
}

//**************************** Start-Bildschirm, entweder über init.lua-Skript oder Standard ***************************************
void start_screen() {
  delay(100); // Kurze Pause
  if (SD.exists("init.lua")) {
    File bootFile = SD.open("init.lua", FILE_READ);
    if (bootFile) {
      size_t fileSize = bootFile.size();

      char* bootBuffer = (char*)malloc(fileSize + 1);         // Dynamischen temporären Speicher im RAM1 für den Boot-Text anfordern
      if (bootBuffer != NULL) {
        bootFile.readBytes(bootBuffer, fileSize);
        bootBuffer[fileSize] = '\0';
        bootFile.close();

        if (luaL_dostring(L, bootBuffer) != LUA_OK) {         // Übergabe des geladenen Text-Strings an den Lua-Kern
          const char* error_msg = lua_tostring(L, -1);
          vga_print_str("Fehler in init.lua: ");
          vga_print_str(error_msg);
          vga_print_str("\n\r");
          lua_pop(L, 1);
        }
        free(bootBuffer);                                     // Speicher wieder freigeben
      } else {
        vga_print_str("Fehler: Zu wenig RAM fuer Boot-Puffer!\n\r");
        bootFile.close();
      }
    } else {
      vga_print_str("Fehler: Konnte init.lua nicht oeffnen!\n\r");
    }
  } else {

    //keine init.lua da, dann normaler Startbildschirm
    vga_print_str("--- Teensy 4.1 Standalone Lua Computer ---\n");
    vga_print_str("\n");
    vga_print_str("USB-Tastatur & VGA bereit.\n");
  }

  // Cursor für die anschließende REPL-Eingabe
  vga_print_str("> ");
}

//######################################################## LOOP ########################################################
void loop() {
  myusb.Task();
  handleRepeat();                                       // Prüfen, ob eine Taste wiederholt werden muss
  if (lastUsbChar != -1) {
    Serial.print(lastUsbChar);
    handleIncomingChar(lastUsbChar);
    lastUsbChar = -1;
  }

  while (Serial.available() > 0) {
    char c = Serial.read();
    handleIncomingChar(c);
  }

  if (millis() - lastCursorBlink >= blinkInterval) {
    lastCursorBlink = millis();
    cursorVisible = !cursorVisible; // Zustand umkehren

    int vgaX = cursorX * 8;
    int vgaY = cursorY * 8;

    if (cursorVisible) {
      vga.drawText(vgaX, vgaY, "_", fColor, bColor, false);
    } else {
      char currentChar = termBuffer[cursorY][cursorX];
      if (currentChar == '\0') {
        vga.drawText(vgaX, vgaY, " ", fColor, bColor, false);    // Leerzeichen, falls leer
      } else {
        char singleCharStr[] = { currentChar, '\0' };
        vga.drawText(vgaX, vgaY, singleCharStr, fColor, bColor, false);
      }
    }
  }
}
