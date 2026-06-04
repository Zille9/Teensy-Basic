#ifndef ABSTRACT_H
#define ABSTRACT_H

#ifdef PROFILE
#define printf(a, b) Serial.println(b)
#endif

#if defined ARDUINO_SAM_DUE || ARDUINO_GIGA || defined ADAFRUIT_GRAND_CENTRAL_M4
#define HostOS 0x01
#endif
#if defined CORE_TEENSY
#define HostOS 0x04
#endif
#if defined ESP32
#define HostOS 0x05
#endif
#if defined _STM32_DEF_
#define HostOS 0x06
#endif
#if defined ARDUINO_ARCH_RP2040
#define HostOS 0x07
#endif


//----------------------- SD-Updater hinzugefügt ---------------------------------
#include "FXUtil.h"     // Für die originale update_firmware() Funktion
extern "C" {
#include "FlashTxx.h" // Für firmware_buffer_init() und firmware_buffer_free()
}
//----------------------- SD-Updater hinzugefügt ---------------------------------


void load_hex() {
  uint32_t buffer_addr, buffer_size;

  delay(500);
  if (firmware_buffer_init(&buffer_addr, &buffer_size) == 0) {
    Serial.print("Fehler: Flash-Puffer fehlgeschlagen!");
    _puts("Fehler: Flash-Puffer fehlgeschlagen!");
    return;
  }
  File32 hexFile = SD.open("/basic.hex", FILE_READ);
  if (!hexFile) {
    Serial.println("HEX-Datei nicht gefunden!");
    _puts("HEX-Datei nicht gefunden!");
    firmware_buffer_free(buffer_addr, buffer_size);
    return;
  }
  Serial.println("Lade Basic...");
  _puts("Lade Basic...");

  delay(1000);
  // 3. Das originale FlasherX übernimmt: Einlesen, RAM-Kopieren, Reboot
  update_firmware(&hexFile, &Serial, buffer_addr, buffer_size);
  // Fallback: Nur wenn die HEX-Datei fehlerhaft/unvollständig war, läuft der Code hier weiter
  hexFile.close();
  Serial.println("FEHLER: Hex-Struktur ungueltig. Reboot...");
  firmware_buffer_free(buffer_addr, buffer_size);
  delay(1000);

  REBOOT;
}
//################################################### Flash-Loader ######################################################################


/* Memory abstraction functions */
/*===============================================================================*/
uint16 _RamLoad(uint8 *filename, uint16 address, uint16 maxsize) {
  File32 f;
  uint16 bytesread = 0;

  if (f = SD.open((char *)filename, FILE_READ)) {
    while (f.available()) {
      _RamWrite(address++, f.read());
      bytesread++;
      if (maxsize && bytesread >= maxsize)
        break;
    }
    f.close();
  }
  return (bytesread);
}

/* Filesystem (disk) abstraction functions */
/*===============================================================================*/
File32 rootdir, userdir;
#define FOLDERCHAR '/'
#define FILEBASE "./"

typedef struct {
  uint8 dr;
  uint8 fn[8];
  uint8 tp[3];
  uint8 ex, s1, s2, rc;
  uint8 al[16];
  uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
  uint8 dr;
  uint8 fn[8];
  uint8 tp[3];
  uint8 ex, s1, s2, rc;
  uint8 al[16];
} CPM_DIRENTRY;

static DirFat_t fileDirEntry;

bool _sys_exists(uint8 *filename) {
  return (SD.exists((const char *)filename));
}

File32 _sys_fopen_w(uint8 *filename) {
  return (SD.open((char *)filename, O_CREAT | O_WRITE));
}

int _sys_fputc(uint8 ch, File32 &f) {
  return (f.write(ch));
}

void _sys_fflush(File32 &f) {
  f.flush();
}

void _sys_fclose(File32 &f) {
  f.close();
}

int _sys_select(uint8 *disk) {
  uint8 result = FALSE;
  File32 f;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (f = SD.open((char *)disk, O_READ)) {
    if (f.isDirectory())
      result = TRUE;
    f.close();
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

long _sys_filesize(uint8 *filename) {
  long l = -1;
  File32 f;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (f = SD.open((char *)filename, O_RDONLY)) {
    l = f.size();
    f.close();
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (l);
}

int _sys_openfile(uint8 *filename) {
  File32 f;
  int result = 0;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_READ);
  if (f) {
    f.dirEntry(&fileDirEntry);
    f.close();
    result = 1;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

int _sys_makefile(uint8 *filename) {
  File32 f;
  int result = 0;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_CREAT | O_WRITE);
  if (f) {
    f.close();
    result = 1;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

int _sys_deletefile(uint8 *filename) {
  digitalWrite(LED, HIGH ^ LEDinv);
  return (SD.remove((char *)filename));
  digitalWrite(LED, LOW ^ LEDinv);
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
  File32 f;
  int result = 0;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_WRITE | O_APPEND);
  if (f) {
    if (f.rename((char *)newname)) {
      f.close();
      result = 1;
    }
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
#ifdef CONSOLELOG
  puts((char *)buffer);
#else
  File f;
  uint8 s = 0;
  while (*(buffer + s)) // Computes buffer size
    ++s;
  if (f = SD.open(LogName, O_CREAT | O_APPEND | O_WRITE)) {
    f.write(buffer, s);
    f.flush();
    f.close();
  }
#endif
}
#endif

bool _sys_extendfile(char *fn, unsigned long fpos) {
  uint8 result = true;
  File32 f;
  unsigned long i;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (f = SD.open(fn, O_WRITE | O_APPEND)) {
    if (fpos > f.size()) {
      for (i = 0; i < f.size() - fpos; ++i) {
        if (f.write((uint8)0) != 1) {
          result = false;
          break;
        }
      }
    }
    f.close();
  } else {
    result = false;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

uint8 _sys_readseq(uint8 *filename, long fpos) {
  uint8 result = 0xff;
  File32 f;
  uint8 bytesread;
  uint8 dmabuf[BlkSZ];
  uint8 i;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_READ);
  if (f) {
    if (f.seek(fpos)) {
      for (i = 0; i < BlkSZ; ++i)
        dmabuf[i] = 0x1a;
      bytesread = f.read(&dmabuf[0], BlkSZ);
      if (bytesread) {
        for (i = 0; i < BlkSZ; ++i)
          _RamWrite(dmaAddr + i, dmabuf[i]);
      }
      result = bytesread ? 0x00 : 0x01;
    } else {
      result = 0x01;
    }
    f.close();
  } else {
    result = 0x10;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

uint8 _sys_writeseq(uint8 *filename, long fpos) {
  uint8 result = 0xff;
  File32 f;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (_sys_extendfile((char *)filename, fpos))
    f = SD.open((char *)filename, O_RDWR);
  if (f) {
    if (f.seek(fpos)) {
      if (f.write(_RamSysAddr(dmaAddr), BlkSZ))
        result = 0x00;
    } else {
      result = 0x01;
    }
    f.close();
  } else {
    result = 0x10;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

uint8 _sys_readrand(uint8 *filename, long fpos) {
  uint8 result = 0xff;
  File32 f;
  uint8 bytesread;
  uint8 dmabuf[BlkSZ];
  uint8 i;
  long extSize;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_READ);
  if (f) {
    if (f.seek(fpos)) {
      for (i = 0; i < BlkSZ; ++i)
        dmabuf[i] = 0x1a;
      bytesread = f.read(&dmabuf[0], BlkSZ);
      if (bytesread) {
        for (i = 0; i < BlkSZ; ++i)
          _RamWrite(dmaAddr + i, dmabuf[i]);
      }
      result = bytesread ? 0x00 : 0x01;
    } else {
      if (fpos >= 65536L * BlkSZ) {
        result = 0x06; // seek past 8MB (largest file size in CP/M)
      } else {
        extSize = f.size();
        // round file size up to next full logical extent
        extSize = ExtSZ * ((extSize / ExtSZ) + ((extSize % ExtSZ) ? 1 : 0));
        if (fpos < extSize)
          result = 0x01; // reading unwritten data
        else
          result = 0x04; // seek to unwritten extent
      }
    }
    f.close();
  } else {
    result = 0x10;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

uint8 _sys_writerand(uint8 *filename, long fpos) {
  uint8 result = 0xff;
  File32 f;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (_sys_extendfile((char *)filename, fpos)) {
    f = SD.open((char *)filename, O_RDWR);
  }
  if (f) {
    if (f.seek(fpos)) {
      if (f.write(_RamSysAddr(dmaAddr), BlkSZ))
        result = 0x00;
    } else {
      result = 0x06;
    }
    f.close();
  } else {
    result = 0x10;
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

static uint8 findNextDirName[13];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

uint8 _findnext(uint8 isdir) {
  File32 f;
  uint8 result = 0xff;
  bool isfile;
  uint32 bytes;

  digitalWrite(LED, HIGH ^ LEDinv);
  if (allExtents && fileRecords) {
    _mockupDirEntry(0);
    result = 0;
  } else {
    while (f = userdir.openNextFile()) {
      f.getName((char *)&findNextDirName[0], 13);
      isfile = !f.isDirectory();
      bytes = f.size();
      f.dirEntry(&fileDirEntry);
      f.close();
      if (!isfile)
        continue;
      _HostnameToFCBname(findNextDirName, fcbname);
      if (match(fcbname, pattern)) {
        if (isdir) {
          // account for host files that aren't multiples of the block size
          // by rounding their bytes up to the next multiple of blocks
          if (bytes & (BlkSZ - 1)) {
            bytes = (bytes & ~(BlkSZ - 1)) + BlkSZ;
          }
          fileRecords = bytes / BlkSZ;
          fileExtents = fileRecords / BlkEX + ((fileRecords & (BlkEX - 1)) ? 1 : 0);
          fileExtentsUsed = 0;
          firstFreeAllocBlock = firstBlockAfterDir;
          _mockupDirEntry(0);
        } else {
          fileRecords = 0;
          fileExtents = 0;
          fileExtentsUsed = 0;
          firstFreeAllocBlock = firstBlockAfterDir;
        }
        _RamWrite(tmpFCB, filename[0] - '@');
        _HostnameToFCB(tmpFCB, findNextDirName);
        result = 0x00;
        break;
      }
    }
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

uint8 _findfirst(uint8 isdir) {
  uint8 path[4] = {'?', FOLDERCHAR, '?', 0};
  path[0] = filename[0];
  path[2] = filename[2];
  if (userdir)
    userdir.close();
  userdir = SD.open((char *)path); // Set directory search to start from the first position
  _HostnameToFCBname(filename, pattern);
  fileRecords = 0;
  fileExtents = 0;
  fileExtentsUsed = 0;
  return (_findnext(isdir));
}

uint8 _findnextallusers(uint8 isdir) {
  uint8 result = 0xFF;
  char dirname[13];
  bool done = false;

  while (!done) {
    while (!userdir) {
      userdir = rootdir.openNextFile();
      if (!userdir) {
        done = true;
        break;
      }
      userdir.getName(dirname, sizeof dirname);
      if (userdir.isDirectory() && strlen(dirname) == 1 && isxdigit(dirname[0])) {
        currFindUser = dirname[0] <= '9' ? dirname[0] - '0' : toupper(dirname[0]) - 'A' + 10;
        break;
      }
      userdir.close();
    }
    if (userdir) {
      result = _findnext(isdir);
      if (result) {
        userdir.close();
      } else {
        done = true;
      }
    } else {
      result = 0xFF;
      done = true;
    }
  }
  return result;
}

uint8 _findfirstallusers(uint8 isdir) {
  uint8 path[2] = {'?', 0};

  path[0] = filename[0];
  if (rootdir)
    rootdir.close();
  if (userdir)
    userdir.close();
  rootdir = SD.open((char *)path); // Set directory search to start from the first position
  strcpy((char *)pattern, "???????????");
  if (!rootdir)
    return 0xFF;
  fileRecords = 0;
  fileExtents = 0;
  fileExtentsUsed = 0;
  return (_findnextallusers(isdir));
}

uint8 _Truncate(char *filename, uint8 rc) {
  File32 f;
  int result = 0;

  digitalWrite(LED, HIGH ^ LEDinv);
  f = SD.open((char *)filename, O_WRITE | O_APPEND);
  if (f) {
    if (f.truncate(rc * BlkSZ)) {
      f.close();
      result = 1;
    }
  }
  digitalWrite(LED, LOW ^ LEDinv);
  return (result);
}

void _MakeUserDir() {
  uint8 dFolder = cDrive + 'A';
  uint8 uFolder = toupper(tohex(userCode));

  uint8 path[4] = {dFolder, FOLDERCHAR, uFolder, 0};

  digitalWrite(LED, HIGH ^ LEDinv);
  SD.mkdir((char *)path);
  digitalWrite(LED, LOW ^ LEDinv);
}

uint8 _sys_makedisk(uint8 drive) {
  uint8 result = 0;
  if (drive < 1 || drive > 16) {
    result = 0xff;
  } else {
    uint8 dFolder = drive + '@';
    uint8 disk[2] = {dFolder, 0};
    digitalWrite(LED, HIGH ^ LEDinv);
    if (!SD.mkdir((char *)disk)) {
      result = 0xfe;
    } else {
      uint8 path[4] = {dFolder, FOLDERCHAR, '0', 0};
      SD.mkdir((char *)path);
    }
    digitalWrite(LED, LOW ^ LEDinv);
  }

  return (result);
}

/* Hardware abstraction functions */
/*===============================================================================*/
void _HardwareInit(void) {
}

void _HardwareOut(const uint32 Port, const uint32 Value) {
}

uint32 _HardwareIn(const uint32 Port) {
  return 0;
}

/* Console abstraction functions */
/*===============================================================================*/



//########################################################## Tastatur-Handling #################################################################################
void process_keyboard_logic(int unicode, uint8_t mod, uint8_t keycode) {
  // ===================================================
  // 1. MODIFIKATOREN AUSLESEN (Bitmasken aus 'mod')
  // ===================================================
  bool shiftPressed = (mod & 0x02) || (mod & 0x20); // Linkes oder rechtes Shift
  bool altGrPressed = (mod & 0x40);                // Rechter Alt-Key (AltGr)

  // ===================================================
  // 2. FLASHERX UPDATE-TASTE (F12 = Keycode 69)
  // ===================================================
  if (keycode == 69) {
    lastUsbChar = 250; // Einzigartiger Code für den FlasherX-Trigger
    return;
  }

  // ===================================================
  // 3. DEUTSCHES SONDERZEICHEN-MAPPING (Keycodes u. AltGr)
  // ===================================================
  
  // Taste links neben Shift-Rechts (ID 100): < > und |
  if (keycode == 100) {
    if (altGrPressed) {
      lastUsbChar = 124; // '|'
    } else if (shiftPressed) {
      lastUsbChar = 62;  // '>'
    } else {
      lastUsbChar = 60;  // '<'
    }
    return;
  }

  // AltGr-Kombinationen für Klammern
  if (altGrPressed) {
    switch (keycode) {
      case 36: lastUsbChar = 123; return; // AltGr + 7 -> '{' (Keycode 36)
      case 37: lastUsbChar = 91;  return; // AltGr + 8 -> '[' (Keycode 37)
      case 38: lastUsbChar = 93;  return; // AltGr + 9 -> ']' (Keycode 38)
      case 39: lastUsbChar = 125; return; // AltGr + 0 -> '}' (Keycode 39)
    }
  }

  // ===================================================
  // 4. PFEILTASTEN ÜBER IHREN UNICODE ABFANGEN
  // ===================================================
  if (unicode >= 215 && unicode <= 218) {
    switch (unicode) {
      case 218: lastUsbChar = 5;  return; // Pfeil HOCH   -> STRG+E
      case 217: lastUsbChar = 24; return; // Pfeil RUNTER -> STRG+X
      case 216: lastUsbChar = 19; return; // Pfeil LINKS  -> STRG+S
      case 215: lastUsbChar = 4;  return; // Pfeil RECHTS -> STRG+D
    }
  }

  // ===================================================
  // 5. RETTUNG FÜR DIE ENTER-TASTE (Keycode 40)
  // ===================================================
  if (keycode == 40 || unicode == 13 || unicode == 10) {
    lastUsbChar = 13; // Echtes CP/M & WordStar Enter
    return;
  }

  // ===================================================
  // 6. STANDARD-ZEICHENÜBERGABE (Text, STRG+C etc.)
  // ===================================================
  if (unicode > 0) {
    lastUsbChar = unicode;
    return;
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
  
  if (keycode == lastRepeatKeycode) {
    lastRepeatKeycode = 0; // Stop Repeat
  }
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


/*
  int _kbhit(void) {
    return (Serial.available());
  }
*/
int _kbhit(void) {
  handleRepeat();
  // Erst prüfen, ob noch Zeichen in der Pfeiltasten-Warteschlange sind
  if (keyQueueIdx < keyQueueLen) return 1;
  if (lastUsbChar != 0) return 1;
  return (Serial.available());
}


uint8 _getch(void) {
  while (!_kbhit()) {
    delayMicroseconds(10);
  }

  // 1. Zeichen aus der Pfeiltasten-Warteschlange bedienen
  if (keyQueueIdx < keyQueueLen) {
    uint8_t ch = keyQueue[keyQueueIdx++];
    if (keyQueueIdx >= keyQueueLen) {
      keyQueueIdx = 0;  // Puffer zurücksetzen
      keyQueueLen = 0;
    }
    return ch;
  }
  // 2. Normales USB-Zeichen bedienen
  if (lastUsbChar != 0) {
    uint8_t ch = lastUsbChar;
    if (ch == 250) {
      lastUsbChar = 0;
      load_hex();
    }

    lastUsbChar = 0;
    return ch;
  }
  // 3. Serial-Monitor bedienen
  return (Serial.read());
}

void scrollVGA() {
  // 1. Textzeilen im RAM-Puffer eine Position nach oben schieben
  for (int y = 0; y < MAX_R - 1; y++) {
    memcpy(screenBuffer[y], screenBuffer[y + 1], MAX_C);
  }
  // Die unterste, neue Zeile komplett mit Leerzeichen leeren
  memset(screenBuffer[MAX_R - 1], ' ', MAX_C);

  // 2. Den VGA-Bildschirm komplett leeren
  vga.clear(bColor);

  // 3. FIX: JEDES Zeichen (auch Leerzeichen!) lückenlos neu zeichnen.
  // Das überschreibt alle Cursor-Fragmente und Geisterzeichen restlos mit der korrekten Farbe!
  char buf[2] = {0, 0};
  for (int y = 0; y < MAX_R; y++) {
    for (int x = 0; x < MAX_C; x++) {
      buf[0] = screenBuffer[y][x];
      vga.drawText(x * 8, y * 8, buf, fColor, bColor, false);
    }
  }
}

void toggleCursor(bool show) {
  // Sicherheitsnetz: Sicherstellen, dass wir uns innerhalb der Bildschirmgrenzen bewegen
  if (x_pos >= MAX_C || y_pos >= MAX_R) return;

  char curChar = show ? '_' : screenBuffer[y_pos][x_pos];
  char buf[2] = {curChar, 0};

  // FIX: Wenn der Cursor ausgeblendet wird (show == false), stellen wir exakt das
  // Zeichen wieder her, das laut screenBuffer dort stehen muss.
  vga.drawText(x_pos * 8, y_pos * 8, buf, fColor, bColor, false);
}



void _clrscr(void) {
  // Seriellen Monitor via ANSI-Escape-Code leeren
  Serial.print("\e[H\e[J");

  // VGA_t4 Hardware-Bildschirm komplett schwärzen
  vga.clear(bColor);

  // Text-Matrix im RAM komplett mit Leerzeichen füllen
  memset(screenBuffer, ' ', sizeof(screenBuffer));

  // Cursor-Koordinaten auf Start setzen
  x_pos = 0;
  y_pos = 0;

  // Cursor initial anzeigen
  toggleCursor(true);
}

void _putch(uint8_t ch) {
  // Serielle Debug-Ausgabe an den PC mitsenden
  Serial.write(ch);

  // Cursor für die Dauer des Zeichnens unsichtbar machen
  toggleCursor(false);

  // ===================================================
  // STATE MACHINE FOR PARALLEL KAYPRO & ANSI EMULATION
  // ===================================================

  // --- ZUSTAND: NORMALTEXT ---
  if (kayproState == STATE_NORMAL) {
    switch (ch) {
      case 27: // ESCAPE (Leitet jede Sequenz ein)
        kayproState = STATE_ESC;
        toggleCursor(true);
        return;

      case 7:  // Signalton
        break;
      case 8:  // Cursor Links
        if (x_pos > 0) x_pos--;
        break;

      case 9:  // Tabulator-Unterstützung (Feste 8er-Stopps für WordStar/Pascal)
        x_pos = ((x_pos / 8) + 1) * 8;
        if (x_pos >= MAX_C) {
          x_pos = 0;
          y_pos++;
        }
        break;

      case 11: // Cursor Hoch
        if (y_pos > 0) y_pos--;
        break;
      case 12: // Cursor Rechts
        if (x_pos < MAX_C - 1) x_pos++;
        break;
      case 10: // Cursor Runter (Line Feed)
        y_pos++;
        break;

      case 13: // Carriage Return
        x_pos = 0;
        break;

      case 30: // Home Cursor
        x_pos = 0; y_pos = 0;
        break;

      case 26: // Bildschirm löschen und Home
        _clrscr();
        x_pos = 0; y_pos = 0;
        toggleCursor(true);
        break;

      case 24: { // Zeile ab Cursor leeren
          char spaceStr[] = {' ', 0};
          for (int x = x_pos; x < MAX_C; x++) {
            screenBuffer[y_pos][x] = ' ';
            vga.drawText(x * 8, y_pos * 8, " ", fColor, bColor, false);
          }
          break;
        }
      case 23: { // Bildschirm ab Cursor leeren
          char spaceStr[] = {' ', 0};
          for (int x = x_pos; x < MAX_C; x++) {
            screenBuffer[y_pos][x] = ' ';
            vga.drawText(x * 8, y_pos * 8, spaceStr, fColor, bColor, false);
          }
          for (int y = y_pos + 1; y < MAX_R; y++) {
            memset(screenBuffer[y], ' ', MAX_C);
            for (int x = 0; x < MAX_C; x++) {
              vga.drawText(x * 8, y * 8, spaceStr, fColor, bColor, false);
            }
          }
          break;
        }



      default:
        // Normales sichtbares ASCII-Zeichen drucken
        if (ch >= 32 && ch <= 126) {
          if (x_pos >= MAX_C) {
            x_pos = 0;
            y_pos++;
          }
          if (y_pos >= MAX_R) {
            scrollVGA();
            y_pos = MAX_R - 1;
          }

          screenBuffer[y_pos][x_pos] = ch;
          char buf[] = {(char)ch, 0};
          vga.drawText(x_pos * 8, y_pos * 8, buf, fColor, bColor, false);
          x_pos++;
        }
        break;
    }
  }

  // --- ZUSTAND: ESCAPE EMPFANGEN (WEICHENSTELLUNG) ---
  else if (kayproState == STATE_ESC) {
    if (ch == '[') {
      kayproState = STATE_ANSI_CSI;
      ansiParamIdx = 0;
      memset(ansiParams, 0, sizeof(ansiParams));
    }
    else if (ch == '_') {
      kayproState = STATE_KAYPRO_CMD;
      ansiParamIdx = 0;
      memset(ansiParams, 0, sizeof(ansiParams));
    }
    else if (ch == '=') {
      kayproState = STATE_ROW;
    }
    else if (ch == 'B' || ch == 'C') {
      lastAttrCmd = ch;
      kayproState = STATE_ATTR;
    }
    else {
      switch (ch) {
        case 'A':
          fColor = 0xFC; bColor = 0x00; // Reset auf Bernstein
          break;

        case 'E': { // === KAYPRO ZEILE EINFÜGEN (Insert Line) ===
            // BESTIMMUNG DER SCROLL-STARTZEILE:
            // Wenn der Cursor im WordStar-Textbereich steht, schieben wir den gesamten
            // Text ab der aktuellen Zeile nach unten. Das Menü (Zeile 0-6) bleibt geschützt!
            int startY = y_pos;
            if (startY < 7) startY = 7; // Sicherheitsnetz für das Menü

            // 1. Speicher-Shifting im RAM rückwärts (von unten nach oben)
            for (int y = MAX_R - 1; y > startY; y--) {
              memcpy(screenBuffer[y], screenBuffer[y - 1], MAX_C);
            }
            // Die eingefügte Zeile leeren
            memset(screenBuffer[startY], ' ', MAX_C);

            // 2. Komplettes Redraw vom Startpunkt bis zum echten Bildschirmende
            for (int y = startY; y < MAX_R; y++) {
              for (int x = 0; x < MAX_C; x++) {
                char buf[] = {screenBuffer[y][x], 0};
                vga.drawText(x * 8, y * 8, buf, fColor, bColor, false);
              }
            }
            kayproState = STATE_NORMAL; // State-Machine entriegeln
            break;
          }

        case 'R': { // === KAYPRO ZEILE LÖSCHEN (Delete Line) ===
            // BESTIMMUNG DER SCROLL-STARTZEILE:
            int startY = y_pos;
            if (startY < 7) startY = 7;

            // 1. Speicher-Shifting im RAM vorwärts (von oben nach unten)
            for (int y = startY; y < MAX_R - 1; y++) {
              memcpy(screenBuffer[y], screenBuffer[y + 1], MAX_C);
            }
            // Die unterste Zeile leeren
            memset(screenBuffer[MAX_R - 1], ' ', MAX_C);

            // 2. Komplettes Redraw vom Startpunkt bis zum echten Bildschirmende
            for (int y = startY; y < MAX_R; y++) {
              for (int x = 0; x < MAX_C; x++) {
                char buf[] = {screenBuffer[y][x], 0};
                vga.drawText(x * 8, y * 8, buf, fColor, bColor, false);
              }
            }
            kayproState = STATE_NORMAL; // State-Machine entriegeln
            break;
          }




        case '7': // VT100: Cursor-Position merken
          saved_x = x_pos;
          saved_y = y_pos;
          break;
        case '8': // VT100: Cursor-Position wiederherstellen
          x_pos = constrain(saved_x, 0, MAX_C - 1);
          y_pos = constrain(saved_y, 0, MAX_R - 1);
          break;
      }
      kayproState = STATE_NORMAL;
    }
    toggleCursor(true);
    return;
  }

  // --- ZUSTAND: KAYPRO SPEZIALCMD PARSER (ESC _) ---
  else if (kayproState == STATE_KAYPRO_CMD) {
    if (ch == 'F') return;
    else if (ch >= '0' && ch <= '9') {
      ansiParams[ansiParamIdx] = (ansiParams[ansiParamIdx] * 10) + (ch - '0');
    }
    else if (ch == ';') {
      if (ansiParamIdx < MAX_ANSI_PARAMS - 1) ansiParamIdx++;
    }
    else if (ch == '$') {
      kayproState = STATE_NORMAL;
      int rawCol = ansiParams[0];
      int rawRow = ansiParams[1];
      int finalCol = x_pos;
      if (rawCol > 0) {
        finalCol = (rawCol * 48) / 49;
      }
      int finalRow = y_pos;
      if (rawRow >= 2) {
        finalRow = rawRow - 2;
      } else {
        finalRow = rawRow;
      }
      x_pos = constrain(finalCol, 0, MAX_C - 1);
      y_pos = constrain(finalRow, 0, MAX_R - 1);
    }
    else {
      kayproState = STATE_NORMAL;
    }
    toggleCursor(true);
    return;
  }

  // --- ZUSTAND: ANSI / VT100 CSI PARSER ---
  else if (kayproState == STATE_ANSI_CSI) {
    if (ch >= '0' && ch <= '9') {
      ansiParams[ansiParamIdx] = (ansiParams[ansiParamIdx] * 10) + (ch - '0');
    }
    else if (ch == ';') {
      if (ansiParamIdx < MAX_ANSI_PARAMS - 1) ansiParamIdx++;
    }
    else {
      kayproState = STATE_NORMAL;

      switch (ch) {
        case 'H':
        case 'f': {
            int r = (ansiParamIdx >= 1) ? ansiParams[0] - 1 : 0;
            int c = (ansiParamIdx >= 1) ? ansiParams[1] - 1 : 0;
            if (ansiParamIdx == 0 && ansiParams[0] == 0) {
              r = 0;
              c = 0;
            }
            x_pos = constrain(c, 0, MAX_C - 1);
            y_pos = constrain(r, 0, MAX_R - 1);
            break;
          }

        case 'M': {
            int startY = y_pos;
            if (startY < 7) startY = 7; // Schutz für das obere Menü

            // Speicher im RAM vorwärts shiften (löscht aktuelle Zeile, zieht untere hoch)
            for (int y = startY; y < MAX_R - 1; y++) {
              memcpy(screenBuffer[y], screenBuffer[y + 1], MAX_C);
            }
            memset(screenBuffer[MAX_R - 1], ' ', MAX_C);

            // VGA komplett ab Shifting-Kante spiegeln
            for (int y = startY; y < MAX_R; y++) {
              for (int x = 0; x < MAX_C; x++) {
                char buf[] = {screenBuffer[y][x], 0};
                vga.drawText(x * 8, y * 8, buf, fColor, bColor, false);
              }
            }
            break;
          }

        // ===================================================
        // !!! NEU: ANSI ZEILE EINFÜGEN / NACH UNTEN SCROLLEN (CSI L oder CSI [ n L) !!!
        // Schafft Platz für eine neue Zeile, drückt Text nach unten
        // ===================================================
        case 'L': {
            int startY = y_pos;
            if (startY < 7) startY = 7; // Schutz für das obere Menü

            // Speicher im RAM rückwärts shiften (schiebt Zeilen nach unten)
            for (int y = MAX_R - 1; y > startY; y--) {
              memcpy(screenBuffer[y], screenBuffer[y - 1], MAX_C);
            }
            memset(screenBuffer[startY], ' ', MAX_C);

            // VGA komplett ab Shifting-Kante spiegeln
            for (int y = startY; y < MAX_R; y++) {
              for (int x = 0; x < MAX_C; x++) {
                char buf[] = {screenBuffer[y][x], 0};
                vga.drawText(x * 8, y * 8, buf, fColor, bColor, false);
              }
            }
            break;
          }


        case 'A': { // VT100: Cursor n Zeilen nach HOCH
            int n = (ansiParams[0] == 0) ? 1 : ansiParams[0];
            y_pos = max(0, y_pos - n);
            break;
          }
        case 'B': { // VT100: Cursor n Zeilen nach RUNTER
            int n = (ansiParams[0] == 0) ? 1 : ansiParams[0];
            y_pos = min(MAX_R - 1, y_pos + n);
            break;
          }
        case 'C': { // VT100: Cursor n Spalten nach RECHTS
            int n = (ansiParams[0] == 0) ? 1 : ansiParams[0];
            x_pos = min(MAX_C - 1, x_pos + n);
            break;
          }
        case 'D': { // VT100: Cursor n Spalten nach LINKS
            int n = (ansiParams[0] == 0) ? 1 : ansiParams[0];
            x_pos = max(0, x_pos - n);
            break;
          }

        case 'J':
          if (ansiParams[0] == 2) {
            _clrscr();
          }
          else if (ansiParams[0] == 0) { // Ab Cursor bis Bildschirmende löschen
            char spaceStr[] = {' ', 0};
            for (int x = x_pos; x < MAX_C; x++) {
              screenBuffer[y_pos][x] = ' ';
              vga.drawText(x * 8, y_pos * 8, spaceStr, fColor, bColor, false);
            }
            for (int y = y_pos + 1; y < MAX_R; y++) {
              memset(screenBuffer[y], ' ', MAX_C);
              for (int x = 0; x < MAX_C; x++) vga.drawText(x * 8, y * 8, spaceStr, fColor, bColor, false);
            }
          }
          break;

        case 'K':
          if (ansiParams[0] == 0) { // Ab Cursor bis Zeilenende löschen
            char spaceStr[] = {' ', 0};
            for (int x = x_pos; x < MAX_C; x++) {
              screenBuffer[y_pos][x] = ' ';
              vga.drawText(x * 8, y_pos * 8, spaceStr, fColor, bColor, false);
            }
          }
          else if (ansiParams[0] == 2) { // Ganze Zeile löschen
            char spaceStr[] = {' ', 0};
            memset(screenBuffer[y_pos], ' ', MAX_C);
            for (int x = 0; x < MAX_C; x++) vga.drawText(x * 8, y_pos * 8, spaceStr, fColor, bColor, false);
          }
          break;

        case 'm':
          for (int i = 0; i <= ansiParamIdx; i++) {
            int p = ansiParams[i];
            if (p == 0) {
              fColor = 0xFC;  // Bernstein-Reset
              bColor = 0x00;
            }
            else if (p == 1) {
              fColor = 0xFF;  // Weiß-Highlight
              bColor = 0x00;
            }
            // ===================================================
            // 1. STANDARD VORDERGRUND-FARBEN (30 bis 37) - Gedimmt
            // ===================================================
            else if (p >= 30 && p <= 37) {
              switch (p) {
                case 30: fColor = 0x00; break; // Schwarz
                case 31: fColor = 0x80; break; // Dunkelrot
                case 32: fColor = 0x10; break; // Dunkelgrün
                case 33: fColor = 0x90; break; // Dunkelgelb / Senf
                case 34: fColor = 0x02; break; // Dunkelblau
                case 35: fColor = 0x82; break; // Dunkelmagenta
                case 36: fColor = 0x12; break; // Dunkelcyan
                case 37: fColor = 0xB6; break; // Normalweiß / Vintage-Grau
              }
            }
            // ===================================================
            // 2. STANDARD HINTERGRUND-FARBEN (40 bis 47) - Gedimmt
            // ===================================================
            else if (p >= 40 && p <= 47) {
              switch (p) {
                case 40: bColor = 0x00; break; // Schwarz
                case 41: bColor = 0x80; break; // Dunkelrot
                case 42: bColor = 0x10; break; // Dunkelgrün
                case 43: bColor = 0x90; break; // Dunkelgelb / Senf
                case 44: bColor = 0x02; break; // Dunkelblau
                case 45: bColor = 0x82; break; // Dunkelmagenta
                case 46: bColor = 0x12; break; // Dunkelcyan
                case 47: bColor = 0xB6; break; // Normalweiß / Vintage-Grau
              }
            }
            // ===================================================
            // 3. HIGH-INTENSITY VORDERGRUND-FARBEN (90 bis 97) - Hell
            // ===================================================
            else if (p >= 90 && p <= 97) {
              switch (p) {
                case 90: fColor = 0x55; break; // Helles Grau
                case 91: fColor = 0xE0; break; // Leuchtendes Rot
                case 92: fColor = 0x1C; break; // Leuchtendes Grün
                case 93: fColor = 0xFC; break; // Leuchtendes Gelb / Bernstein
                case 94: fColor = 0x03; break; // Leuchtendes Blau
                case 95: fColor = 0xE3; break; // Leuchtendes Magenta / Pink
                case 96: fColor = 0x1F; break; // Leuchtendes Cyan
                case 97: fColor = 0xFF; break; // Reines Weiß
              }
            }
            // ===================================================
            // 4. HIGH-INTENSITY HINTERGRUND-FARBEN (100 bis 107) - Hell
            // ===================================================
            else if (p >= 100 && p <= 107) {
              switch (p) {
                case 100: bColor = 0x55; break; // Helles Grau
                case 101: bColor = 0xE0; break; // Leuchtendes Rot
                case 102: bColor = 0x1C; break; // Leuchtendes Grün
                case 103: bColor = 0xFC; break; // Leuchtendes Gelb / Bernstein
                case 104: bColor = 0x03; break; // Leuchtendes Blau
                case 105: bColor = 0xE3; break; // Leuchtendes Magenta / Pink
                case 106: bColor = 0x1F; break; // Leuchtendes Cyan
                case 107: bColor = 0xFF; break; // Reines Weiß
              }
            }
          } break;
      }
    } toggleCursor(true);
    return;
  }
  // --- ZUSTAND: KAYPRO ATTRIBUT-WERTE ---
  else if (kayproState == STATE_ATTR) {
    if (lastAttrCmd == 'B') {
      if (ch == 0 || ch == '0') {
        fColor = 0x00; bColor = 0xFF;
      }
      else if (ch == 1 || ch == '1') {
        fColor = 0xB6; bColor = 0x00;
      }
    }
    else if (lastAttrCmd == 'C') {
      if (ch == 0 || ch == '0') {
        fColor = 0xFC; bColor = 0x00;
      }
      else if (ch == 1 || ch == '1') {
        fColor = 0xFC; bColor = 0x00;
      }
    }
    kayproState = STATE_NORMAL;
    toggleCursor(true);
    return;
  }// --- ZUSTAND: KAYPRO POSITION REIHE ---
  else if (kayproState == STATE_ROW) {
    targetRow = ch - 32;
    kayproState = STATE_COL;
    toggleCursor(true); return;
  }// --- ZUSTAND: KAYPRO POSITION SPALTE ---
  else if (kayproState == STATE_COL) {
    int targetCol = ch - 32;
    y_pos = constrain(targetRow, 0, MAX_R - 1);
    x_pos = constrain(targetCol, 0, MAX_C - 1);
    kayproState = STATE_NORMAL;
    toggleCursor(true);
    return;
  }// Umlaufkontrolle nach Bewegungen
  if (x_pos >= MAX_C) {
    x_pos = 0;
    y_pos++;
  }
  if (y_pos >= MAX_R) {
    scrollVGA();
    y_pos = MAX_R - 1;
  }
  toggleCursor(true);
}

uint8_t _getche(void) {
  uint8 ch = _getch();
  Serial.write(ch);
  _putch(ch);
  return (ch);
}

#endif
