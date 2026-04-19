///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//             Basic - Interpreter für TEENSY 4.1                                                                                                 //
//               for VGA monitor output - März 2026                                                                                               //
//                                                                                                                                                //
//                                                                                                                                                //
//      von:Reinhard Zielinski <zille09@gmail.com>                                                                                                //
//                                                                                                                                                //
//      Connections:                                                                                                                              //
//                                                                                                                                                //
//                                                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BasicVersion "1.5"
#define BuiltTime "15.04.2026"

//Logbuch
//Version 1.5 15.04.2026                   -Befehl RECT nach anfänglichen Schwierigkeiten eingebunden -> RECT x,y,w,h,fill
//                                         -Befehl LINE eingebunden -> LINE x,y,xx,yy
//                                         -Befehl CIRC eingebunden -> CIRC x,y,w,h,fill
//                                         -die Grafikbefehle führen noch zu Abstürzen wenn bsp.weise eine neue Basic-datei geladen werden soll - Screenpuffer ?
//                                         -DIR - Ausgabe etwas angepasst
//                                         -203976 Zeilen/sek. (Fastest)
//
//Version 1.4 12.04.2026                   -USB-Tastatur eingebunden, die Texteingaben erfolgen ab sofort nicht mehr über seriell sondern Tastatur
//                                         -Code soweit auf Tastatur umgebaut
//                                         -VGA-Funktionalität eingebunden, jetzt kanns richtig losgehen - kleiner Fehler in der Pinbeschaltung für VGA behoben
//                                         -ein Rot-Pin war statt auf Pin2 auf Pin1 dadurch gab es Farbverfälschungen
//                                         -Anpassung an die Tastenabfrage OnPress - die Tasten <> | [ ] { } und ^ waren nicht erreichbar
//                                         -Fehler in cmd_dim behoben -> AX=10 Dim S(AX) führte dazu das AX dimensioniert wurde und nicht S - die Variablen-Indexe wurden vorher nicht gesichert
//                                         -Syntax-Hervorhebung eingebaut, RUN-Befehl erweitert -> RUN"Filename" oder String
//
//Version 1.3 10.04.2026                   -ENDIF als TOKEN und in der IF-THEN-ELSE Ausführung entfernt, das hat den Basic-Interpreter extrem ausgebremst
//                                         -durch die Rückkehr zum normalen IF-THEN-ELSE ist die Geschwindigkeit mehr als doppelt so hoch
//                                         -DEFN und FN mit bis zu 4 Parametern eingebaut als Variablen sind aber nur einbuchstabige Variablen erlaubt!
//                                         -VGA-Funktionalität eingebunden, jetzt kanns richtig losgehen - kleiner Fehler in der Pinbeschaltung für VGA behoben
//                                         -242274 Zeilen/sek. (Optimize: Fastest)
//
//Version 1.2 08.04.2026                   -USB-Keyboard-Treiber eingebaut, muss aber noch am Teensy angesteckt werden (fehlendes Kabel)
//                                         -interne RTC aktiviert -> Befehle STIME h,m,s,d,m,y zum stellen GTIME(1...6) zum auslesen
//                                         -TIME$ Rückgabe Uhrzeit als String (hh:mm:ss), Date$ Rückgabe Datum als String (dd.mm.yyyy)
//                                         -90792 Zeilen/sek.
//
//Version 1.1 06.04.2026                   -solides Grundgerüst mit diversen Befehlen Arrays (Zahlen/Strings), SD-Card Zugriff
//                                         -erste Programme vom ESP32+Basic funktionieren, zusätzlich integrierte Befehle IF-THEN-ELSE-ENDIF
//                                         -sowie WHILE-WEND, diverse Stringfunktionen Left$,Right$,Mid$,STR$,CHR$,SPC
//                                         -TIMER-Befehl zur Ermittlung von Benchmark-Werten, als nächstes werden MODULO und die NOT (!) Funktion eingebaut
//                                         -ausserdem soll die Verarbeitung von BIN und HEX hinzugefügt werden -> erledigt
//                                         -91072 Zeilen/sek.
//
//
//Version 1.0 02.04.2026                   -erstes Grundgerüst erstellt mit den Befehlen RUN,LIST,FOR..NEXT..STEP,NEW,PRINT,PRZ,VARS
//                                         -Ausgabe noch über serielle Konsole, da der TEENSY noch nackig auf dem Tisch liegt
//                                         -Grundrechenarten integriert, Variablen mit 2 Buchstaben + 10 Zahlen (insgesamt 962 A...Z9)
//                                         -SIN,COS,ABS,SQR und INT hinzugefügt
//                                         -nächste Funktionalität wird SD-Card-Zugriff sein, um Programme auf der SD-Karte zu speichern und zu laden
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#define BUF_SIZE 256
char inputBuffer[BUF_SIZE];
static char *txtpos, *list_line;//, *tmptxtpos, *dataline;
bool inQuotes = false;
extern "C" uint8_t external_psram_size;

#include <VGA_t4.h>
VGA_T4 vga;
int vga_cursor_x = 0;
int vga_cursor_y = 0;
vga_pixel vga_fg = VGA_RGB(255, 255, 255); // Weiß
vga_pixel vga_bg = VGA_RGB(0, 0, 170);     // C64-Blau
static int fb_width, fb_height;
int currentIndent = 0;

const int fontH = 8;
const int MAX_C = 640 / 8;                 //Anzahl Textspalten
const int MAX_R = 240 / 8;                 //Anzahl Textzeilen
char screenBuffer[MAX_R][MAX_C];      // Bildschirm-Buffer: 30 Zeilen à 40 Zeichen
uint8_t colorBuffer_F[MAX_R][MAX_C];  // Speichert die Vordergrundfarbe (8-Bit VGA)
uint8_t colorBuffer_H[MAX_R][MAX_C];  // Speichert die Hintergrundfarbe (8-Bit VGA)
bool cursor_on_off = true;
#define BLUE       11
#define LIGHT_BLUE 83
#define WHITE      255
#define YELLOW     252
#define MAGENTA    163
#define CYAN       127
#define GREEN      24
#define RED        196
#define GRAY       114

uint8_t F_COL[] {0, 136, 255};
uint8_t H_COL[] {0, 0, 255};


int x_pos = 0;                              // Aktuelle Position (global)
int y_pos = 0;

struct Params {                             //struct für wiederkehrende Parametereingaben
  int val[10];
};


bool tron_marker = false;                   // TRON/TROFF - Marker

#include <stdarg.h>                         //für printf
#include <TimeLib.h>                        //RTC-Funktionen

#include <SPI.h>
#include <SD.h>


const int chipSelect = BUILTIN_SDCARD;      // Für Teensy 4.1/4.0/3.6 Onboard-SD-Slot:

#include "USBHost_t36.h"                    //USB-Keyboard-treiber
USBHost myusb;
USBHub hub1(myusb);                         // Optional, falls ein Hub verwendet wird
USBHIDParser hid1(myusb);                   // Der "Dolmetscher" für moderne Tastaturen
KeyboardController keyboard1(myusb);

volatile int lastUsbChar = -1;              //Merker für die letzte gedrückte Taste

// ASCII Characters
#define CR  '\r'
#define NL  '\n'
#define LF  0x0a
#define TAB '\t'
#define BELL  '\b'
#define SPACE   ' '
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC 0x03
#define CTRLH 0x08
#define CTRLS 0x13


static char *current_line;
typedef short unsigned LINENUM;

#define MAX_GOSUB_STACK 25
struct GosubStack {
  int lineIndex;
  const char* txtPos;
};

GosubStack gosubStack[MAX_GOSUB_STACK];
int gosubStackPtr = 0;

bool break_marker = false;
bool jumped = false;
double lastNumberValue = 0;
int precisionValue = 6; // Standardmäßig 6 Stellen


// Index 0: Kein zweites Zeichen (nur "A")
// Index 1-26: Zweiter Buchstabe (A-Z)
// Index 27-36: Zweite Zahl (0-9)
double variables[26][37];
int vIdx1 = 0; // Erster Buchstabe
int vIdx2 = 0; // Zweites Zeichen (0 = keines)
const char* lastTokenPos; // Speichert den Anfang des aktuellen Tokens
char lastVarName;  // Speichert den Buchstaben der aktuellen Variable (A-Z)

// 26 Buchstaben x 37 Folgezeichen (A-Z, 0-9, none)
String stringVars[26][37];
bool isStringVar = false; // Globales Flag
char lastStringValue[80];
#define MAX_STRING_LEN 64

// Arrays 3 Dimensionen Numerisch und String
#define MAX_ARRAYS 40

struct Array3D {
  int vIdx1;    // Erster Buchstabe (0-25)
  int vIdx2;    // Zweiter Buchstabe/Zahl (0-36)
  bool isString;
  int dimX, dimY, dimZ;
  double* numData;
  String* strData;
};

Array3D allArrays[MAX_ARRAYS];
int arrayCount = 0;

#define MAX_WHILE_STACK 10

struct WhileStack {
  int lineIndex;
  const char* txtPos;
};

WhileStack whileStack[MAX_WHILE_STACK];
int whileStackPtr = 0;


struct ForLoop {
  int vIdx1;       // Index für 1. Buchstabe (0-25)
  int vIdx2;       // Index für 2. Zeichen (0-36)
  double targetValue;
  double stepValue;
  int lineIndex;
  const char* txtPos;
};

#define MAX_FOR_STACK 26
ForLoop forStack[MAX_FOR_STACK];     // Maximal 8 verschachtelte Schleifen
int forStackPtr = 0;

int currentLineIndex = 0; // Index im program[] Array
bool isRunning = false;   // Status-Flag
bool last_if_result = false;  //IF-THEN-ELSE-FLAG

int printMode = 0; // 0=Normal, 16=HEX, 2=BIN, 8=OCT  - Print-Modi



struct UserFunction {
  char name;            // Name der Funktion (A-Z)
  char params[4];       // Die Namen der 4 Variablen (z.B. W, X, Y, Z)
  int paramCount;       // Wie viele Parameter wurden tatsächlich definiert?
  const char* formula;  // Zeiger auf den Ausdruck nach dem '='
};

UserFunction function[26]; // Für A-Z

//---------------------------------- Fehlermeldungen des Interpreters -----------------------------------------------------------------------------

static const char syntaxmsg[]        PROGMEM = "Syntax Error! ";                //1
static const char mathmsg[]          PROGMEM = "Math Error!";                   //2
static const char gosubmsg[]         PROGMEM = "Gosub Error!";                  //3
static const char fornextmsg[]       PROGMEM = "For-Next Error!";               //4
static const char memorymsg[]        PROGMEM = " bytes free";
static const char missing_then[]     PROGMEM = "Missing THEN!";                 //5
static const char breakmsg[]         PROGMEM = "Break! in Line:";
static const char breaks[]           PROGMEM = "Break!";
static const char datamsg[]          PROGMEM = "Out of DATA!";                  //6
static const char invalidmsg[]       PROGMEM = "Invalid comparison!";           //7
static const char sderrormsg[]       PROGMEM = "SD card Error.";                //8
static const char sdfilemsg[]        PROGMEM = "SD file Error.";                //9
static const char dirextmsg[]        PROGMEM = "(dir)";
static const char slashmsg[]         PROGMEM = "/";
static const char spacemsg[]         PROGMEM = " ";
static const char notexistmsg[]      PROGMEM = "File not exist!";               //10
static const char portmsg[]          PROGMEM = "Wrong Port-Number!";            //11
static const char valmsg[]           PROGMEM = "Invalid Value!";                //12
static const char dirmsg[]           PROGMEM = "Dir not empty!";                //13
static const char illegalmsg[]       PROGMEM = "Illegal quantity!";             //14
static const char zeroerror[]        PROGMEM = "Div/0-Error!";                  //15
static const char outofmemory[]      PROGMEM = "Out of Memory!";                //16
static const char mountmsg[]         PROGMEM = "SD-Card mounted";               //17
static const char notmount[]         PROGMEM = "SD-Card can't mount";           //18
static const char dimmsg[]           PROGMEM = "Array-Dimension!";              //19
static const char commsg[]           PROGMEM = "No COM-Port defined!";          //20
static const char comsetmsg[]        PROGMEM = "Wrong COM-Port Definition!";    //21
static const char bmpfilemsg[]       PROGMEM = "No BMP-File!";                  //22
static const char no_prg_msg[]       PROGMEM = "No Program in Memory!";         //23
static const char no_command_msg[]   PROGMEM = "Keyword not found!";            //24
static const char not_openmsg[]      PROGMEM = "File not open!";                //25
static const char dirnotfound[]      PROGMEM = "DIR not found !";               //26
static const char extension_error[]  PROGMEM = "invalid File-Extension !";      //27
static const char stringtolong[]     PROGMEM = "String to long!";               //28
static const char wronglinenr[]      PROGMEM = "Wrong Line-Number!";            //29
static const char illegalvariable[]  PROGMEM = "Illegal Variable Name";         //30
static const char whilewendmsg[]     PROGMEM = "While-Wend-Error!";             //31

int dataLineIdx = 0;   // In welcher Zeile im program[] Array sind wir?
char* dataPtr = NULL;  // Wo genau in dieser Zeile steht der nächste Wert?

enum {
  TOKEN_ERROR,
  TOKEN_PRINT,            //erster Basic-Befehl
  TOKEN_GOTO,
  TOKEN_PRZ,
  TOKEN_LIST,
  TOKEN_RUN,
  TOKEN_VARIABLE,         // A-Z
  TOKEN_NUMBER,           //Zahlen
  TOKEN_STRING_LITERAL,   //Strings
  TOKEN_NEW,
  TOKEN_FOR,
  TOKEN_TO,
  TOKEN_NEXT,
  TOKEN_STEP,
  TOKEN_VARS,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_STOP,
  TOKEN_REM,
  TOKEN_INPUT,
  TOKEN_DATA,
  TOKEN_READ,
  TOKEN_RESTORE,
  TOKEN_FILES,
  TOKEN_DIR,
  TOKEN_LOAD,
  TOKEN_SAVE,
  TOKEN_DELETE,
  TOKEN_DIM,
  TOKEN_GOSUB,
  TOKEN_RETURN,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_WEND,
  TOKEN_RENAME,
  TOKEN_COPY,
  TOKEN_DWRITE,
  TOKEN_PAUSE,
  TOKEN_MEM,
  TOKEN_ON,
  TOKEN_STIME,
  TOKEN_DUMP,
  TOKEN_DEFN,
  TOKEN_CLS,
  TOKEN_COL,
  TOKEN_PSET,
  TOKEN_TAB,       //TOKEN nur für Listausgabe relevant
  TOKEN_AT,        //TOKEN nur für Listausgabe relevant
  TOKEN_TRON,
  TOKEN_TROFF,
  TOKEN_PIC,
  TOKEN_LINE,
  TOKEN_RECT,
  TOKEN_CIRC,
  TOKEN_PEN,
  TOKEN_CUR,
  TOKEN_EDIT,      //letzter Basic-Befehl
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_STYLE,
  TOKEN_RND,       //erster Funktions-Befehl
  TOKEN_SQR,
  TOKEN_SIN,
  TOKEN_COS,
  TOKEN_ABS,
  TOKEN_INT,
  TOKEN_PI,
  TOKEN_DEG,
  TOKEN_RAD,
  TOKEN_SGN,
  TOKEN_LEN,
  TOKEN_ASC,
  TOKEN_VAL,
  TOKEN_DREAD,
  TOKEN_FRE,
  TOKEN_TIMER,
  TOKEN_LEFT,
  TOKEN_RIGHT,
  TOKEN_MID,
  TOKEN_SPC,
  TOKEN_STR,
  TOKEN_CHR,
  TOKEN_BIN,
  TOKEN_HEX,
  TOKEN_OCT,
  TOKEN_GTIME,
  TOKEN_TIME,
  TOKEN_DATE,
  TOKEN_FN,
  TOKEN_INKEY,
  TOKEN_STRING,      //letzter Funktions-Befehl
  TOKEN_EXP,
  TOKEN_END
};

struct Keyword {
  const char* name;
  int token;
};

Keyword commands[] = {
  {"PRINT", TOKEN_PRINT},
  {"GOTO", TOKEN_GOTO},
  {"PRZ", TOKEN_PRZ},
  {"LIST", TOKEN_LIST},
  {"RUN",   TOKEN_RUN},
  {"NEW",  TOKEN_NEW},
  {"FOR",  TOKEN_FOR},
  {"TO",    TOKEN_TO},
  {"NEXT", TOKEN_NEXT},
  {"STEP", TOKEN_STEP},
  {"VARS", TOKEN_VARS},
  {"IF",   TOKEN_IF},
  {"THEN",  TOKEN_THEN},
  {"END", TOKEN_STOP},
  {"REM", TOKEN_REM},
  {"INPUT", TOKEN_INPUT},
  {"DATA", TOKEN_DATA},
  {"READ", TOKEN_READ},
  {"RESTORE", TOKEN_RESTORE},
  {"FILES", TOKEN_FILES},
  {"DIR", TOKEN_DIR},
  {"LOAD", TOKEN_LOAD},
  {"SAVE", TOKEN_SAVE},
  {"DELETE", TOKEN_DELETE},
  {"DIM", TOKEN_DIM},
  {"GOSUB", TOKEN_GOSUB},
  {"RETURN", TOKEN_RETURN},
  {"ELSE", TOKEN_ELSE},
  {"WHILE", TOKEN_WHILE},
  {"WEND",  TOKEN_WEND},
  {"RENAME", TOKEN_RENAME},
  {"COPY", TOKEN_COPY},
  {"DWRITE", TOKEN_DWRITE},
  {"PAUSE", TOKEN_PAUSE},
  {"MEM", TOKEN_MEM},
  {"ON", TOKEN_ON},
  {"STIME", TOKEN_STIME},
  {"DUMP", TOKEN_DUMP},
  {"DEFN", TOKEN_DEFN},
  {"CLS", TOKEN_CLS},
  {"COL", TOKEN_COL},
  {"PSET", TOKEN_PSET},
  {"TAB", TOKEN_TAB},
  {"AT", TOKEN_AT},
  {"TRON", TOKEN_TRON},
  {"TROFF", TOKEN_TROFF},
  {"PIC", TOKEN_PIC},
  {"LINE", TOKEN_LINE},
  {"RECT", TOKEN_RECT},
  {"CIRC", TOKEN_CIRC},
  {"PEN", TOKEN_PEN},
  {"CUR", TOKEN_CUR},
  {"EDIT", TOKEN_EDIT},
  {"AND", TOKEN_AND},
  {"OR", TOKEN_OR},
  {"STYLE", TOKEN_STYLE},
  {NULL, 0}
};

Keyword functions[] = {
  {"RND", TOKEN_RND},
  {"SQR", TOKEN_SQR},
  {"SIN", TOKEN_SIN},
  {"COS", TOKEN_COS},
  {"ABS", TOKEN_ABS},
  {"INT", TOKEN_INT},
  {"PI",  TOKEN_PI},
  {"DEG", TOKEN_DEG},
  {"RAD", TOKEN_RAD},
  {"SGN", TOKEN_SGN},
  {"LEN", TOKEN_LEN},
  {"ASC", TOKEN_ASC},
  {"VAL", TOKEN_VAL},
  {"DREAD", TOKEN_DREAD},
  {"FRE", TOKEN_FRE},
  {"TIMER", TOKEN_TIMER},
  {"LEFT$", TOKEN_LEFT},
  {"RIGHT$", TOKEN_RIGHT},
  {"MID$", TOKEN_MID},
  {"SPC", TOKEN_SPC},
  {"STR$", TOKEN_STR},
  {"CHR$", TOKEN_CHR},
  {"BIN", TOKEN_BIN},
  {"HEX", TOKEN_HEX},
  {"OCT", TOKEN_OCT},
  {"GTIME", TOKEN_GTIME},
  {"TIME$", TOKEN_TIME},
  {"DATE$", TOKEN_DATE},
  {"FN", TOKEN_FN},
  {"INKEY", TOKEN_INKEY},
  {"STRING$", TOKEN_STRING},
  {"EXP", TOKEN_EXP},
  {NULL, 0}
};


#define MAX_LINES 1560
#define LINE_LEN 80

struct BasicLine {
  int number;                                                                           // Die Zeilennummer (z.B. 10)
  char text[LINE_LEN];                                                                  // Der eigentliche Befehlstext
};

BasicLine program[MAX_LINES];
int lineCount = 0;                                                                      // Wie viele Zeilen aktuell im Speicher sind

#define BIN 2
#define OCT 8
#define DEC 10
#define HEX 16

// vga8ToRGB(meinVgaWert, r, g, b);
void fbcolor(uint8_t vgaVal, bool vh) {
  // Rot: Die obersten 3 Bits isolieren
  uint8_t  r = (vgaVal & 0xE0); //| ((vgaVal & 0xE0) >> 3) | ((vgaVal & 0xE0) >> 6);
  uint8_t  g = ((vgaVal & 0x1C) << 3) ;//| (vgaVal & 0x1C) | ((vgaVal & 0x1C) >> 3);
  uint8_t  b = ((vgaVal & 0x03) << 6) ;//| ((vgaVal & 0x03) << 4) | ((vgaVal & 0x03) << 2) | (vgaVal & 0x03);
  if (vh) {
    H_COL[0] = r;
    H_COL[1] = g;
    H_COL[2] = b;
    return;
  }
  else
  {
    F_COL[0] = r;
    F_COL[1] = g;
    F_COL[2] = b;
  }
  //r = (vgaVal & 0xE0);          // 0b11100000
  // Grün: Die mittleren 3 Bits isolieren und 3 Stellen nach links schieben
  //g = (vgaVal & 0x1C) << 3;     // 0b00011100 -> 0b11100000
  // Blau: Die untersten 2 Bits isolieren und 6 Stellen nach links schieben
  //b = (vgaVal & 0x03) << 6;     // 0b00000011 -> 0b11000000
}

uint8_t rgbToVGA(uint8_t r, uint8_t g, uint8_t b) {
  // Rot von 0-255 auf 0-7 bringen (3 Bit)
  // Grün von 0-255 auf 0-7 bringen (3 Bit)
  // Blau von 0-255 auf 0-3 bringen (2 Bit)
  return ( (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6) );
}

void cmd_color() {
  uint8_t v, h;
  spaces();
  v = expression();
  if (v > 255) v = 255;
  fbcolor(v, 0);
  spaces();
  if (*txtpos == ',') {
    txtpos++;
    h = expression();
    if (h > 255) h = 255;
    fbcolor(h, 1);
  }
  yield();
  return;

}

void setpos(int x, int y)
{
  x_pos = x;
  y_pos = y;
}


// Für Texte (Strings)
void print(const char *str) {
  while (*str) outchar(*str++);
}

// Für einzelne Zeichen
void print(char c) {
  outchar(c);
}

// Für ganze Zahlen (int)
void print(int n) {
  char buf[12];
  itoa(n, buf, 10);
  print(buf);
}

// Für Fließkommazahlen (float/double) - wichtig für BASIC
void print(double n) {
  char buf[20];
  // n, Vorkommastellen, Nachkommastellen, Puffer
  dtostrf(n, 4, 2, buf);
  print(buf);
}
void print(long n) {
  print(n, 10);
}
void println(long n) {
  print(n, 10);
  outchar(13);
}
void println(const char *str) {
  print(str);
  outchar(13);
}
void println(int n)          {
  print(n);
  outchar(13);
}
void println(double n)       {
  print(n);
  outchar(13);
}
void println()               {
  outchar(13);
}
void print(String s)         {
  print(s.c_str());
}
void println(String s)       {
  println(s.c_str());
}

void print(long n, int base) {
  char buf[33]; // Genug Platz für 32 Bits (BIN) + Null-Terminator
  ltoa(n, buf, base);

  // Bei Hexadezimal sieht es in Großbuchstaben (C64-Style) besser aus
  if (base == 16) {
    for (int i = 0; buf[i]; i++) buf[i] = toupper(buf[i]);
  }

  print(buf); // Ruft deine print(const char*) auf
}

// Und die passende println-Version
void println(long n, int base) {
  print(n, base);
  outchar(13);
}

void print(uint64_t n) {
  char buf[21]; // Genug Platz für die größte 64-Bit Zahl (20 Stellen + \0)

  // Da der Teensy ltoa/ultoa oft nur bis 32 Bit unterstützt,
  // nutzen wir sprintf für echte 64-Bit Unterstützung:
  sprintf(buf, "%llu", n);
  print(buf);
}

void println(uint64_t n) {
  print(n);
  outchar(13);
}
void printReady() {
  outchar(13); // Sicherstellen, dass wir in einer neuen Zeile ganz links starten
  println("READY.");
}

void vprintf(const char *format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  print(buf); // Nutzt deine bestehende VGA-print Funktion
}

void cmd_cls() {
  // 1. Die aktuellen Farben für das Leeren nehmen
  uint8_t bg = getVGA(H_COL);
  uint8_t fg = getVGA(F_COL);

  // 2. Alle Buffer im RAM leeren
  for (int r = 0; r < MAX_R; r++) {
    for (int c = 0; c < MAX_C; c++) {
      screenBuffer[r][c] = ' ';   // Alles auf Leerzeichen
      colorBuffer_F[r][c] = fg;   // Aktuelle Vordergrundfarbe speichern
      colorBuffer_H[r][c] = bg;   // Aktuelle Hintergrundfarbe speichern
    }
  }
  x_pos = 0;
  y_pos = 0;
  vga.clear(bg);
}

uint8_t getVGA(uint8_t col[]) {
  return VGA_RGB(col[0], col[1], col[2]);
}


void drawCursor(bool state) {
  int px = x_pos * 8;
  int py = y_pos * 8;

  if (state) {
    // Cursor an: Invertiertes Leerzeichen (Blau auf Hellblau)
    vga.drawText(px, py, " ", getVGA(H_COL), getVGA(F_COL), false);
  } else {
    // Cursor aus: Zeichen aus dem Buffer wiederherstellen
    char c = screenBuffer[y_pos][x_pos];
    static char buf[2] = {0, 0};
    buf[0] = (c <= 32) ? ' ' : c; // Leerzeichen, falls Buffer leer
    buf[1] = '\0';
    vga.drawText(px, py, buf, getVGA(F_COL), getVGA(H_COL), false);
  }
}




static void outchar(char c) {
  drawCursor(false);
  if (x_pos < 0) x_pos = 0;
  if (y_pos < 0) y_pos = 0;

  if (c == 13) {
    x_pos = 0;
    y_pos++;
    checkScroll();
    return;
  }
  if (c == 10) return;

  if (c == '\b') {            //Pfeiltaste zurück
    if (x_pos > 0) x_pos--;   // KEIN Leerzeichen schreiben!
    return;
  }
  if (c == 8 || c == 127) {
    if (x_pos > 0) {
      x_pos--;
      screenBuffer[y_pos][x_pos] = ' ';
      // beim Backspace die aktuelle Hintergrundfarbe im Buffer "leeren"
      colorBuffer_F[y_pos][x_pos] = getVGA(F_COL);
      colorBuffer_H[y_pos][x_pos] = getVGA(H_COL);
      vga.drawText(x_pos * 8, y_pos * 8, " ", getVGA(F_COL), getVGA(H_COL), false);

    }
    return;
  }

  if (x_pos >= MAX_C) {
    x_pos = 0;
    y_pos++;
    checkScroll();
  }

  if (y_pos < MAX_R) {
    // 1. Zeichen UND Farben im Buffer speichern
    screenBuffer[y_pos][x_pos] = c;
    colorBuffer_F[y_pos][x_pos] = getVGA(F_COL); // Aktuelle Vordergrundfarbe merken
    colorBuffer_H[y_pos][x_pos] = getVGA(H_COL); // Aktuelle Hintergrundfarbe merken

    // 2. Auf den Bildschirm zeichnen
    static char b[2] = {0, 0};
    b[0] = c;
    b[1] = '\0';

    vga.drawText(x_pos * 8, y_pos * 8, b, getVGA(F_COL), getVGA(H_COL), false);

    x_pos++;
    drawCursor(cursor_on_off);
  }
}

void redrawScreen() {
  for (int r = 0; r < MAX_R; r++) {
    for (int c = 0; c < MAX_C; c++) {
      char ch = screenBuffer[r][c];
      // Wir zeichnen ALLES, auch Leerzeichen, damit die Hintergrundfarbe stimmt
      static char b[2] = {0, 0};
      b[0] = (ch < 32) ? ' ' : ch;

      // HIER liegt oft der Fehler: Du MUSST die gespeicherten Farben nehmen!
      uint8_t fg = colorBuffer_F[r][c];
      uint8_t bg = colorBuffer_H[r][c];

      vga.drawText(c * 8, r * 8, b, fg, bg, false);
    }
  }
}

void checkScroll() {
  int fontH = 8;
  if (y_pos >= MAX_R) {
    // 1. Buffer schieben (ASCII und beide Farben)
    for (int r = 1; r < MAX_R; r++) {
      memcpy(screenBuffer[r - 1], screenBuffer[r], MAX_C);
      memcpy(colorBuffer_F[r - 1], colorBuffer_F[r], MAX_C);
      memcpy(colorBuffer_H[r - 1], colorBuffer_H[r], MAX_C);
    }

    // 2. Unterste Zeile leeren (Buffer)
    y_pos = MAX_R - 1;
    uint8_t current_fg = getVGA(F_COL);
    uint8_t current_bg = getVGA(H_COL);
    for (int c = 0; c < MAX_C; c++) {
      screenBuffer[y_pos][c] = ' ';
      colorBuffer_F[y_pos][c] = current_fg;
      colorBuffer_H[y_pos][c] = current_bg;
    }

    // 3. Den Monitor aktualisieren
    redrawScreen();
  }
}




static void line_terminator()
{
  outchar(13);
  //outchar(NL);
}

#include <malloc.h>

extern "C" char* sbrk(int incr);

double get_free_ram() {
  // 1. Hole Statistiken vom Speicher-Manager
  struct mallinfo mi = mallinfo();

  // 2. Berechne den freien Platz im bereits angeforderten Heap-Block
  // mi.fordblks sind die "Löcher" im Heap (freigegebener Speicher)
  uint32_t free_in_heap = mi.fordblks;

  // 3. Berechne den noch nicht angeforderten Speicher zwischen Heap und Stack
  // Auf dem Teensy 4.1 liegt der Heap im 512KB großen RAM2 (OCRAM)
  uint32_t total_ram2 = 512 * 1024;
  uint32_t used_ram2 = mi.uordblks; // Aktuell belegter Heap

  // Gesamter freier Platz im System-RAM
  double total_free = (double)(total_ram2 - used_ram2);

  return total_free;
}


void parseVarName() {

  spaces();

  if (*txtpos >= 'A' && *txtpos <= 'Z') {                                               // 1. Zeichen (Muss A-Z sein)
    vIdx1 = *txtpos++ - 'A';
  } else {
    syntaxerror(illegalvariable);
    isRunning = false; return;
  }
  if (*txtpos >= 'A' && *txtpos <= 'Z') {                                               // 2. Zeichen (Optional: A-Z oder 0-9)
    vIdx2 = (*txtpos++ - 'A') + 1;                                                      // Index 1 bis 26
  } else if (*txtpos >= '0' && *txtpos <= '9') {
    vIdx2 = (*txtpos++ - '0') + 27;                                                     // Index 27 bis 36
  } else {
    vIdx2 = 0; // Kein zweites Zeichen
  }
}

int getVIdx2(char c) {
  if (c >= 'A' && c <= 'Z') return (c - 'A' + 1); // A=1, B=2...
  if (isdigit(c)) return (c - '0' + 27);          // 0=27, 1=28...
  return 0;                                       // Kein Folgezeichen
}

// Hilfsfunktion: Sucht ein Wort in einer Liste und verschiebt txtpos nur bei Erfolg
int scanKeyword(Keyword* list) {
  spaces();
  for (int i = 0; list[i].name != NULL; i++) {
    int len = strlen(list[i].name);
    if (strncmp(txtpos, list[i].name, len) == 0) {
      // Wichtig: Sicherstellen, dass kein Buchstabe folgt (z.B. "PRINTX" vs "PRINT")
      if (!isalnum(txtpos[len]) && txtpos[len] != '$') {
        txtpos += len;
        return list[i].token;
      }
    }
  }
  return -1; // Nichts gefunden
}



// 1. Für den Zeilenanfang (Interpreter-Hauptschleife)
int getCommandToken() {

  spaces();
  int t = scanKeyword(commands);

  if (t != -1) return t;

  // In getCommandToken oder der Variablen-Logik:
  if (isalpha(*txtpos)) {
    lastVarName = *txtpos;
    isStringVar = false;
    vIdx1 = *txtpos - 'A';
    vIdx2 = 0;
    txtpos++; // Überspringt den ersten Buchstaben (z.B. 'A')

    // Prüfen auf zweiten Buchstaben/Zahl (z.B. 'A1')
    if (isalnum(*txtpos)) {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++; // Überspringt das zweite Zeichen
    }

    // WICHTIG: Das $ für Strings überspringen!
    if (*txtpos == '$') {
      isStringVar = true;
      txtpos++;
    }
    return TOKEN_VARIABLE;
  }

  return TOKEN_ERROR;
}

int getFunctionToken() {
  //isStringVar = false;
  spaces();
  lastTokenPos = txtpos;
  if (*txtpos == '\0') return TOKEN_END;

  // 1. ZUERST: Funktionen aus der Liste prüfen (LEN, SQR etc.)
  int t = scanKeyword(functions);
  if (t != -1) return t; // Wenn LEN gefunden wurde, sofort zurückgeben!

  // 2. DANN: Zahlen prüfen
  if (isdigit(*txtpos)) {
    char *endPtr;
    lastNumberValue = strtod(txtpos, &endPtr);
    txtpos = endPtr;
    return TOKEN_NUMBER;
  }

  // 3. DANN: String-Literale "..."
  if (*txtpos == '"') {
    txtpos++; // Überspringe das öffnende "
    int len = 0;

    // Kopiere den Inhalt in einen Puffer (z.B. lastStringValue), bis das schließende " kommt
    while (*txtpos != '"' && *txtpos != '\0' && len < MAX_STRING_LEN - 1) {
      lastStringValue[len++] = *txtpos++;
    }
    lastStringValue[len] = '\0'; // Null-Terminator für den C-String

    return TOKEN_STRING_LITERAL;
  }

  // 4. ALS LETZTES: Variablen (A-Z, A$, AB)
  // Da wir oben schon nach LEN gesucht haben, kann hier
  // ein "L" am Anfang nur noch eine Variable sein.
  if (isalpha(*txtpos)) {
    isStringVar = false;
    vIdx1 = *txtpos - 'A';
    vIdx2 = 0;
    txtpos++;

    if (isalpha(*txtpos) || isdigit(*txtpos)) {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }
    if (*txtpos == '$') {
      isStringVar = true;
      txtpos++;
    }
    return TOKEN_VARIABLE;
  }

  return TOKEN_ERROR;
}

void getln(int showReady) {
  if (showReady) {
    if (x_pos != 0) outchar(13);
    printmsg("READY.", true);
  }

  int cursor = 0;
  int length = 0;
  memset(inputBuffer, 0, BUF_SIZE);

  if (editLine(inputBuffer, BUF_SIZE, cursor, length)) {

    txtpos = inputBuffer;                                           // Wenn Enter gedrückt wurde: Text an txtpos übergeben

    bool q = false;
    for (int i = 0; i < length; i++) {
      if (inputBuffer[i] == '"') q = !q;
      if (!q && inputBuffer[i] >= 'a' && inputBuffer[i] <= 'z') {   // Alles außerhalb von Anführungszeichen groß machen
        inputBuffer[i] -= 32;
      }
    }
  } else {
    // Bei ESC: Zeile leeren
    inputBuffer[0] = '\0';
    txtpos = inputBuffer;
    println("Break!");
  }
}

static int inchar() {
  uint32_t lastBlink = 0;
  bool cursor_state = true;

  while (1) {
    myusb.Task();
    if (cursor_on_off) {
      if (millis() - lastBlink > 300) {
        cursor_state = !cursor_state;
        drawCursor(cursor_state);
        lastBlink = millis();
      }
    }
    if (lastUsbChar != -1) {
      int c = lastUsbChar;
      lastUsbChar = -1;
      drawCursor(false); // Cursor nur löschen
      // Hier KEIN outchar(c)!
      return c;
    }
    yield();
  }
}


static void syntaxerror(const char *msg)
{
  printmsg(msg, 1);
  if (current_line != NULL)
  {
    char tmp = *txtpos;           //Position merken
    if (*txtpos != NL) *txtpos = '^';
    list_line = current_line;
    printmsg(txtpos, 0);   //printline();
    *txtpos = tmp;                //gemerkte Position zurückschreiben
  }
  //Beep(0, 0);                     //Error-BEEP
  current_line = 0;
  line_terminator();
}

void printmsg(const char *msg, int nl) {

  while (*msg) {
    outchar(*msg++);
  }

  if (nl == 1) {
    line_terminator();
  }
}


void storeLine(int num, char* code) {
  int i, j;
  for (i = 0; i < lineCount; i++) {                                                                // Suche: Existiert die Zeilennummer bereits?
    if (program[i].number == num) {
      if (strlen(code) == 0) {
        for (j = i; j < lineCount - 1; j++) {                                                      // LÖSCHEN: Zeile ist leer, also alles dahinter eins nach vorne rücken
          program[j] = program[j + 1];
        }
        lineCount--;
        printmsg("DELETED", 1);
      } else {
        strncpy(program[i].text, code, LINE_LEN - 1);                                              // ÜBERSCHREIBEN
        program[i].text[LINE_LEN - 1] = '\0';
      }
      return;
    }
  }

  if (strlen(code) == 0) return;                                                                   // Wenn Code leer ist und Nummer nicht existiert -> Abbruch

  int insertPos = 0;                                                                               //Einfügen & Sortieren: Finde die richtige Position
  while (insertPos < lineCount && program[insertPos].number < num) {
    insertPos++;
  }

  if (lineCount < MAX_LINES) {                                                                     // Platz schaffen: Alles ab insertPos nach hinten schieben
    for (j = lineCount; j > insertPos; j--) {
      program[j] = program[j - 1];
    }
    program[insertPos].number = num;                                                               // Neue Zeile einsetzen
    strncpy(program[insertPos].text, code, LINE_LEN - 1);
    program[insertPos].text[LINE_LEN - 1] = '\0';
    lineCount++;
  } else {
    syntaxerror(outofmemory);
  }
}

int waitkey() {
  outchar(13);
  println("< Continue = Space/Enter, Break = ESC >");
  while (1) {
    if (lastUsbChar != -1) {
      int c = lastUsbChar;
      lastUsbChar = -1;
      if (c == 27) return 1;
      if (c == 32 || c == 13) break;
    }
  }
  return 0;
}


//################################################################# Syntax-Hervorhebung - muss noch angepasst werden #######################################################################################

// Hilfsfunktion zum farbigen Drucken einer Zeile

void printColoredLine(char* lineText) {
  char* old_txtpos = txtpos;
  txtpos = lineText;

  if (strstr(lineText, "NEXT") || strstr(lineText, "WEND")) {           // Einrückung zurücknehmen
    currentIndent--;
    if (currentIndent < 0) currentIndent = 0;
  }

  for (int i = 0; i < currentIndent; i++) print("  ");                  // Einrückung vor der Zeile drucken, wenn noch FOR oder WHILE Schleifen da sind

  while (*txtpos) {
    if (*txtpos == ' ') {
      print(*txtpos++);
      continue;
    }
    char* startOfToken = txtpos;

    int cmdToken = getCommandToken();                                   // Überprüfung auf Befehle
    if (cmdToken != TOKEN_ERROR && cmdToken != TOKEN_VARIABLE) {
      if (cmdToken == TOKEN_FOR || cmdToken == TOKEN_WHILE) {           // FOR und WHILE - Schleifen einrücken
        currentIndent++;
      }

      if (cmdToken == TOKEN_REM) {                                      // Grau für REM-Zeile ignorieren
        fbcolor(GRAY, 0);
        while (*startOfToken) print(*startOfToken++);
        fbcolor(WHITE, 0);
        break; // Beendet die Verarbeitung dieser Zeile
      }

      fbcolor(YELLOW, 0);                                   // Gelb für Befehle
      while (startOfToken < txtpos) print(*startOfToken++);
      fbcolor(WHITE, 0);
      continue;
    }

    txtpos = startOfToken;

    // 2. DANN: Alles andere über getFunctionToken
    int token = getFunctionToken();
    if (token != TOKEN_ERROR && token != TOKEN_END) {
      switch (token) {
        case TOKEN_STRING_LITERAL:                        // Rot für Strings in Anführungszeichen
          fbcolor(RED, 0);
          break;
        case TOKEN_VARIABLE:
          if (isStringVar) fbcolor(MAGENTA, 0);           // Magenta für String-Variablen
          else fbcolor(GREEN, 0);                         // Grün für numerische Variablen
          break;
        case TOKEN_NUMBER:
          fbcolor(WHITE, 0);                              // Zahlen weiss
          break;
        default:
          fbcolor(CYAN, 0);                               // Cyan für Funktionen
          break;
      }
      while (startOfToken < txtpos) print(*startOfToken++);
      fbcolor(WHITE, 0);
      continue;
    }


    txtpos = startOfToken;                                // 3. Fallback für Operatoren
    print(*txtpos++);
  }
  println("");
  txtpos = old_txtpos;
}

void cmd_list() {

  currentIndent = 0;
  int tmp_color = rgbToVGA(H_COL[0], H_COL[1], H_COL[2]); //aktuelle Hintergrundfarbe sichern
  fbcolor(0, 1);                                          //Hintergrund für die Listausgabe in Schwarz
  //cmd_cls();                                            //vorher Bildschirm löschen ?, reine Geschmackssache

  int zeilen = 0;
  if (lineCount == 0) {
    printmsg("EMPTY", 1);
    return;
  }

  int startline = (int)expression();
  int endline = 65535;
  if (*txtpos == ',') {
    txtpos++;
    endline = (int)expression();
  }

  for (int i = 0; i < lineCount; i++) {
    if ((program[i].number >= startline) && (program[i].number <= endline)) {
      fbcolor(WHITE, 0);
      print(program[i].number);

      printColoredLine(program[i].text);                  //List-Zeile farbig ausgeben

      //fbcolor(WHITE, 0);
      zeilen++;
      if (zeilen == 20) {
        if (waitkey()) return;
        zeilen = 0;
      }
    }
  }
  fbcolor(tmp_color, 1);
}
//################################################################# Ende Syntax-Hervorhebung - muss noch angepasst werden #######################################################################################


bool jumpTo(int label) {
  for (int i = 0; i < lineCount; i++) {
    if (program[i].number == label) {
      currentLineIndex = i; // Wir setzen den INDEX im Array
      return true;
    }
  }
  return false;
}

inline char spaces() {                                                                                     // Leerzeichen überspringen
  while (*txtpos == ' ' || *txtpos == '\t') txtpos++;
  return *txtpos;
}
/*
  bool expect(char expected) {
  spaces();
  // 2. Prüfen, ob das Zeichen übereinstimmt
  if (*txtpos == expected) {
    txtpos++;                                                                                       // Zeichen konsumieren und Pointer weiterbewegen
    return true;
  }
  isRunning = false;                                                                                // Programm stoppen
  return false;
  }
*/
double func_gtime(int w) {
  Teensy3Clock.get();
  switch (w) {
    case 1: return hour();
    case 2: return minute();
    case 3: return second();
    case 4: return day();
    case 5: return month();
    case 6: return year();
    case 7: return weekday();
    default:
      return now();
      break;
  }
}


double func_fn() {
  spaces();
  if (!isalpha(*txtpos)) return 0.0;

  char fName = toupper(*txtpos++);
  int idx = fName - 'A';

  double argValues[4] = {0.0, 0.0, 0.0, 0.0};
  int argCount = 0;

  // 1. Parameter-Werte berechnen
  if (*txtpos == '(') {
    txtpos++;
    while (*txtpos != ')' && *txtpos != '\0' && argCount < 4) {
      argValues[argCount++] = expression();
      spaces();
      if (*txtpos == ',') txtpos++;
      spaces();
    }
    if (*txtpos == ')') txtpos++;
  }

  // WICHTIG: Prüfen ob Singular "function" oder Plural "functions"
  if (function[idx].formula == NULL) return 0.0;

  // 2. BACKUP & KONTEXT
  const char* oldTxtpos = txtpos;
  double oldParamValues[4];

  for (int i = 0; i < function[idx].paramCount; i++) {
    int varIdx = function[idx].params[i] - 'A';
    oldParamValues[i] = variables[varIdx][0];
    variables[varIdx][0] = argValues[i];
  }

  // 3. FORMEL AUSWERTEN
  txtpos = function[idx].formula;
  double result = expression();

  // 4. WIEDERHERSTELLUNG
  // KORREKTUR: "function" statt "functions" (passend zu oben)
  for (int i = 0; i < function[idx].paramCount; i++) {
    int varIdx = function[idx].params[i] - 'A';
    variables[varIdx][0] = oldParamValues[i];
  }
  txtpos = oldTxtpos;

  return result;
}


static int inkey() {
  myusb.Task();

  if (lastUsbChar != -1) {
    int c = lastUsbChar;                          //Taste holen
    lastUsbChar = -1;                             //Tastaturpuffer löschen
    delay(10);                                     //wichtig, sonst flimmert das Bild bei Inkey
    return c;
  }
  delay(10);
  return 0;                                       // nichts gedrückt, dann zurück
}

//unterste ebene factor
double factor() {
  spaces();

  // TURBO: Sofort-Check für einfache Variablen
  if (isalpha(*txtpos)) {
    const char* p = txtpos + 1;
    if (!isalnum(*p) && *p != '$' && *p != '(') {
      int v1 = *txtpos - 'A';
      txtpos = p;
      return variables[v1][0]; // Direkter Rücksprung!
    }
  }

  // --- NEU: Logisches NOT mit '!' ---
  if (*txtpos == '!') {
    txtpos++; // Überspringe das '!'
    double val = factor(); // Rekursiver Aufruf, um den Wert dahinter zu holen
    return (val == 0) ? 1.0 : 0.0;
  }
  // --- NEU: Präfix-Erkennung für Hex, Bin und Oct ---
  if (*txtpos == '&') {
    txtpos++; // Überspringe das '&'
    char type = *txtpos;
    txtpos++; // Überspringe den Typ-Kennner (H, B oder O)

    char *endPtr;
    long val = 0;

    if (type == 'H') {
      val = strtol(txtpos, &endPtr, 16); // Hexadezimal
    } else if (type == 'B') {
      val = strtol(txtpos, &endPtr, 2);  // Binär
    } else if (type == 'O') {
      val = strtol(txtpos, &endPtr, 8);  // Oktal
    } else {
      syntaxerror(valmsg);
      return 0;
    }

    txtpos = endPtr; // Setze den Zeiger hinter die erkannte Zahl
    return (double)val;
  }

  // 1. Einfache Klammer (ohne Funktion davor)
  if (*txtpos == '(') {
    txtpos++;
    double res = expression();
    if (*txtpos == ')') txtpos++;
    return res;
  }

  // 1.5 Direkte Prüfung auf Zahlen (inkl. Scientific Notation wie 1E+38)
  if (isdigit(*txtpos) || *txtpos == '.') {
    char* endPtr;
    double val = strtod(txtpos, &endPtr);
    txtpos = endPtr;
    return val;
  }

  const char* checkpoint = txtpos;                  //txtpos sichern falls kein Token gefunden wird

  // 2. Token holen
  int t = getFunctionToken();
  double arg;
  if (t != TOKEN_ERROR && t != TOKEN_VARIABLE) {
    //print(t);
    switch (t) {
      case TOKEN_NUMBER:
        return lastNumberValue;

      case TOKEN_PI:
        if (*txtpos == '(') txtpos++;
        if (*txtpos == ')') txtpos++;
        return 3.141592653589793;

      case TOKEN_TIMER:
        return (double)millis();

      case TOKEN_VARIABLE:
        if (isStringVar) {
          return 0;
        }
        return (double)variables[vIdx1][vIdx2];

      case TOKEN_FN:
        return func_fn();

      case TOKEN_INKEY:
        return (double)inkey();

      case TOKEN_VAL: {
          spaces();
          if (*txtpos == '(') {
            txtpos++;

            String s = parseStringExpression();           // Liest den String

            if (*txtpos == ')') {
              txtpos++;
              return strtod(s.c_str(), NULL); //return atof(s.c_str());
            } else {
              syntaxerror(valmsg);
              return 0;
            }
          }
          return 0;
        }

      case TOKEN_LEN: {
          // 1. Öffnende Klammer prüfen
          spaces();
          if (*txtpos == '(') txtpos++;

          // 2. Den Inhalt holen (Variable oder String)
          double result = 0;
          int t = getFunctionToken(); // Holt A$ oder "TEXT"

          if (t == TOKEN_VARIABLE && isStringVar) {
            result = (double)stringVars[vIdx1][vIdx2].length();
          } else if (t == TOKEN_STRING_LITERAL) {
            result = (double)strlen(lastStringValue);
          }
          spaces();
          if (*txtpos == ')') txtpos++;

          return result;
        }


      case TOKEN_ASC: {
          spaces();
          if (*txtpos == '(') {
            txtpos++; // Öffnende Klammer (

            // WICHTIG: Hier lassen wir parseStringExpression() die Arbeit machen!
            // Sie wertet MID$(A$, I, 1) komplett aus und gibt uns den String zurück.
            String s = parseStringExpression();

            spaces();
            if (*txtpos == ')') txtpos++; // Schließende Klammer )

            if (s.length() > 0) {
              return (double)((uint8_t)s[0]); // ASCII-Wert des ersten Zeichens
            }
          }
          return 0; // Fallback, falls String leer
        }
      case TOKEN_FRE: {
          // Prüfen, ob Klammern folgen: FRE(x) 1-usedlines 2-freelines 3-totalRam 4-usedRam 5-freeBytes
          spaces();
          if (*txtpos == '(') {
            txtpos++; // Öffnende Klammer (
            int i = expression();
            double res = get_mem(i);
            spaces();
            if (*txtpos == ')') txtpos++; // Schließende Klammer )
            if (i > 0 && i < 6) return res;
          }
          // Gibt Speicherwerte des Basic-Interpreters zurück
          return 0;
        }
      // Gruppe der mathematischen Funktionen
      case TOKEN_RND: case TOKEN_SQR: case TOKEN_SIN: case TOKEN_COS:
      case TOKEN_ABS: case TOKEN_INT: case TOKEN_DEG: case TOKEN_RAD:
      case TOKEN_SGN: case TOKEN_BIN : case TOKEN_HEX : case TOKEN_OCT :
      case TOKEN_GTIME : case TOKEN_EXP:
        // Gemeinsame Logik für die Klammern der Funktionen
        spaces();
        if (*txtpos == '(') txtpos++;
        arg = expression();
        if (*txtpos == ')') txtpos++;

        // Berechnung basierend auf dem Token
        switch (t) {
          case TOKEN_RND: return (double)(random((int)arg));
          case TOKEN_SQR: return sqrt(arg);
          case TOKEN_SIN: return sin(arg);
          case TOKEN_COS: return cos(arg);
          case TOKEN_ABS: return fabs(arg);
          case TOKEN_INT: return floor(arg);
          case TOKEN_RAD: return arg * (3.14159265359 / 180.0);
          case TOKEN_DEG: return arg * (180.0 / 3.14159265359);
          case TOKEN_SGN: return (arg > 0) ? 1.0 : (arg < 0 ? -1.0 : 0.0);
          case TOKEN_DREAD : pinMode(int(arg), INPUT_PULLUP); return (double)digitalRead(int(arg));
          case TOKEN_HEX: printMode = 16; return arg;
          case TOKEN_BIN: printMode = 2;  return arg;
          case TOKEN_OCT: printMode = 8;  return arg;
          case TOKEN_GTIME: return func_gtime(arg);
          case TOKEN_EXP: return exp(arg);

        }
        break;

      default:
        return 0;
    }
  }

  // 3. ERST JETZT: Wenn es kein Funktions-Token war, prüfen ob es eine Variable/Array ist
  txtpos = checkpoint; // Zurück zum Anfang des Wortes
  if (isalpha(*txtpos)) {
    lastVarName = toupper(*txtpos);
    vIdx1 = lastVarName - 'A';
    vIdx2 = 0;
    isStringVar = false;
    txtpos++;

    if (isalnum(*txtpos) && *txtpos != '$') {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }
    if (*txtpos == '$') {
      isStringVar = true;
      txtpos++;
    }
    return get_array_value_factor();
  }

  return 0;
}

//untere Ebene Power
double power() {
  double val = factor();
  while (true) {
    spaces();
    if (*txtpos == '^') {
      txtpos++;
      val = pow(val, power()); // Rechts-assoziativ für 2^3^2
    } else {
      return val;
    }
  }
}


// 3. Mittlere Ebene: Multiplikation und Division
double term() {
  double val = power();
  while (true) {
    spaces();
    if (*txtpos == '*') {
      txtpos++;
      val *= power();
    } else if (*txtpos == '/') {
      txtpos++;
      double divisor = power();
      if (divisor != 0) val /= divisor;
      else syntaxerror(zeroerror);
    } else if (*txtpos == '%') { // NEU: Modulo Operator
      txtpos++;
      double divisor = power();
      if (divisor != 0) {
        val = fmod(val, divisor); // fmod ist die Modulo-Funktion für double
      } else {
        syntaxerror(zeroerror);
      }
    } else {
      return val;
    }
  }
}


// 4. Oberste Ebene: Addition und Subtraktion
double expression() {
  double val = term();
  while (true) {
    spaces();
    if (*txtpos == '+') {
      txtpos++;
      val += term();
    } else if (*txtpos == '-') {
      txtpos++;
      val -= term();
    } else {
      return val;
    }
  }
}

bool relation() {
  double left = expression();
  spaces();

  // Wir lesen den Operator als String, um Kombis wie <= oder <> zu finden
  char op1 = *txtpos;
  char op2 = '\0';

  if (op1 == '<' || op1 == '>' || op1 == '=') {
    txtpos++;
    // Schauen, ob ein zweites Operator-Zeichen folgt
    if (*txtpos == '=' || *txtpos == '>') {
      op2 = *txtpos;
      txtpos++;
    }
  }

  double right = expression();

  // Logik-Check für alle Varianten
  if (op1 == '=') return (long)left == (long)right;
  //if (op1 == '=') return left == right;
  if (op1 == '<') {
    if (op2 == '>') return left != right; // <> Ungleich
    if (op2 == '=') return left <= right; // <= Kleiner gleich
    return left < right;                  // <  Kleiner
  }
  if (op1 == '>') {
    if (op2 == '=') return left >= right; // >= Größer gleich
    return left > right;                  // >  Größer
  }

  return false;
}

void ungetToken(int token) {

  if (token == TOKEN_THEN) txtpos -= 4;
  // Eleganter: Du speicherst dir in getCommandToken immer die "last_txtpos"
}


bool logical_expression() {
  char *tmp;
  bool result = relation(); // Ersten Vergleich auswerten (z.B. A < B)

  tmp = txtpos;

  while (true) {
    tmp = txtpos;
    int token = getCommandToken();

    if (token == TOKEN_AND) {
      // AND-Verknüpfung: Das nächste Ergebnis muss auch wahr sein
      result = result && relation();
    }
    else if (token == TOKEN_OR) {
      // OR-Verknüpfung: Eines von beiden muss wahr sein
      result = result || relation();
    }
    else {
      // Wenn es kein AND/OR ist, gehört das Token zum nächsten Befehl (z.B. THEN)
      // Wir müssen den txtpos zurücksetzen, damit IF das THEN findet.
      if (token == TOKEN_THEN) txtpos = tmp; //ungetToken(token);
      return result;
    }
  }
}

String getVarName(int i, int j) {
  String name = "";
  name += (char)('A' + i); // Erster Buchstabe
  if (j > 0) {
    if (j <= 26) name += (char)('A' + j - 1); // Zweiter Buchstabe A-Z
    else name += (char)('0' + j - 27);        // Zweite Stelle 0-9
  }
  return name;
}

void doVars() {
  // 1. Numerische Variablen (wie bisher)
  printmsg("--- NUMERISCHE VARIABLEN ---", 1);
  bool foundNum = false;
  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      if (variables[i][j] != 0.0) {
        print(getVarName(i, j));
        printmsg(" = ", 0);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.10g", variables[i][j]);
        println(buf);
        foundNum = true;
      }
    }
  }
  if (!foundNum) printmsg("(Keine)", 0);

  // 2. String Variablen (wie bisher)
  printmsg("\n--- STRING VARIABLEN ---", 1);
  bool foundStr = false;
  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      if (stringVars[i][j] != "") {
        print(getVarName(i, j));
        printmsg("$ = \"", 0);
        println(stringVars[i][j]);
        printmsg("\"", 1);
        foundStr = true;
      }
    }
  }
  if (!foundStr) printmsg("(Keine)", 1);

  // 3. ANGEPASST: 3D-ARRAYS mit Zwei-Buchstaben-Namen
  printmsg("\n--- DIMENSIONIERTE ARRAYS ---", 1);
  if (arrayCount == 0) {
    printmsg("(Keine)", 1);
  } else {
    for (int i = 0; i < arrayCount; i++) {
      print("DIM ");

      // Hier nutzen wir getVarName mit den gespeicherten Indizes
      print(getVarName(allArrays[i].vIdx1, allArrays[i].vIdx2));

      if (allArrays[i].isString) print("$");

      print("(");
      print(allArrays[i].dimX - 1);
      if ((allArrays[i].dimY - 1) > 0) {
        print(",");
        print(allArrays[i].dimY - 1); print(",");
        if ((allArrays[i].dimZ - 1) > 0)
          print(allArrays[i].dimZ - 1);
      }
      print(")");

      int totalElements = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
      printmsg("  [", 0);
      print(totalElements);
      printmsg(" Elemente]", 1);
    }
  }
  printmsg("--------------------------------", 1);
}

void cmd_print() {
  bool newline = true;

  while (*txtpos != '\0' && *txtpos != ':' && *txtpos != '\r' && *txtpos != '\n') {
    spaces();
    printMode = 0; // Vor jedem Element zurücksetzen!

    // --- AT(x,y) CHECK ---
    if (strncmp(txtpos, "AT", 2) == 0) {
      txtpos += 2;
      spaces();
      if (*txtpos == '(') {
        txtpos++;
        int newX = (int)expression();
        if (*txtpos == ',') txtpos++;
        int newY = (int)expression();
        if (*txtpos == ')') txtpos++;

        if (newX >= 0 && newX < MAX_C) x_pos = newX;
        if (newY >= 0 && newY < MAX_R) y_pos = newY;

        newline = false;
        spaces();
        if (*txtpos == ';') txtpos++;
      }
      continue; // Nächstes Element in PRINT bearbeiten
    }
    //--- TAB(x) CHECK ---
    if (strncmp(txtpos, "TAB", 3) == 0) {
      txtpos += 3;
      spaces();
      if (*txtpos == '(') {
        txtpos++;
        int newX = expression();
        if (*txtpos == ')') txtpos++;
        if (newX >= 0 && newX < MAX_C) x_pos = newX;

        newline = false;
        spaces();
        if (*txtpos == ';') txtpos++;
      }
      continue; // Nächstes Element in PRINT bearbeiten
    }

    // --- 1. STRING-CHECK ---
    bool isString = (*txtpos == '"');
    if (!isString && isalpha(*txtpos)) {
      if (strncmp(txtpos, "LEFT$", 5) == 0 || strncmp(txtpos, "MID$", 4) == 0 ||
          strncmp(txtpos, "RIGHT$", 6) == 0 || strncmp(txtpos, "STR$", 4) == 0 ||
          strncmp(txtpos, "SPC", 3) == 0 || strncmp(txtpos, "TIME$", 5) == 0 ||
          strncmp(txtpos, "DATES$", 6) == 0 ) {
        isString = true;
      } else {
        const char* p = txtpos;
        while (isalnum(*p)) p++;
        if (*p == '$') isString = true;
      }
    }

    if (isString) {
      print(parseStringExpression());
      newline = true;

    }
    // --- 2. NUMERISCHE AUSDRÜCKE (inkl. HEX/BIN/OCT) ---
    else {
      double val = expression();

      // Prüfen, ob eine Spezialfunktion das printMode-Flag gesetzt hat
      if (printMode == 16) {
        print("H");
        print((double)val, HEX);
      }
      else if (printMode == 2)  {
        print("B");
        print((double)val, BIN);
      }
      else if (printMode == 8)  {
        print("O");
        print((double)val, OCT);
      }
      else {
        // Normaler numerischer Print
        if (val == (long)val && val < 2147483647 && val > -2147483648) {
          print((long)val);
        }
        // 2. Alles andere (Groß, Klein oder Fließkomma)
        else {
          char buf[32];
          // %g ist die sicherste Wahl für double auf dem Teensy
          // Es wechselt automatisch zu 1.23E+38, wenn die Zahl zu groß wird
          snprintf(buf, sizeof(buf), "%.10g", val);
          print(buf);
        }
      }
      newline = true;
    }

    // --- 3. TRENNER-LOGIK ---
    spaces();
    if (*txtpos == ';') {
      txtpos++;
      newline = false;
    }
    else if (*txtpos == ',') {
      txtpos++;
      print("\t");
      newline = false;
    }
    else break;
  }

  if (newline) println();
}



void clearAll() {

  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      variables[i][j] = 0.0f;
      stringVars[i][j] = "";
    }
  }
  for (int i = 0; i < arrayCount; i++) {
    if (allArrays[i].isString) {
      delete[] allArrays[i].strData;
    } else {
      free(allArrays[i].numData);
    }
  }
  gosubStackPtr = 0;                                                                   // GOSUB Stack leeren
  arrayCount = 0;
  forStackPtr = 0;                                                                     // FOR-NEXT Verschachtelung löschen
  currentLineIndex = 0;                                                                // Zeiger auf die erste Programmzeile
  dataLineIdx = 0;
  dataPtr = NULL;
  isRunning = true;                                                                    // Ausführungs-Flag setzen
  jumped = false;                                                                       // WICHTIG: Signalisiert der Hauptschleife einen Neustart

}

void cmd_new() {
  clearAll();
  lineCount = 0;
  isRunning = false;
}


void cmd_run() {
  spaces();

  if (*txtpos != '\0') {
    String fileNameStr = parseStringExpression();
    if (fileNameStr.length() > 0) {

      if (!SD.exists(fileNameStr.c_str())) {
        print("FILE NOT FOUND: ");
        println(fileNameStr.c_str());
        return;
      }

      cmd_new();                                                                       // Speicher löschen vor dem Laden

      File file = SD.open(fileNameStr.c_str());
      if (file) {
        char lineBuffer[128];
        while (file.available()) {
          int len = 0;
          while (file.available() && len < 127) {
            char c = file.read();
            if (c == '\n') break;
            if (c != '\r') lineBuffer[len++] = c;
          }
          lineBuffer[len] = '\0';

          char* p = lineBuffer;
          while (*p && isspace(*p)) p++;
          if (isdigit(*p)) {
            int lineNumber = atoi(p);
            while (isdigit(*p)) p++;
            storeLine(lineNumber, p);
          }
        }
        file.close();
      }
    }
  }
  clearAll();

  if (lineCount == 0) {                                                                //Prüfen, ob überhaupt ein Programm im Speicher ist
    printmsg("NO PROGRAM", 1);
    return;
  }
  txtpos = program[0].text;                                                            // <--- Direkt die erste Zeile laden!

}



void skip_line() {
  while (*txtpos != '\0') txtpos++;                                                    //Rest der Zeile ignorieren
}

void cmd_input() {
  spaces();

  if (*txtpos == '"') {
    txtpos++;
    while (*txtpos != '"' && *txtpos != '\0') outchar(*txtpos++);
    if (*txtpos == '"') txtpos++;
    spaces();
    if (*txtpos == ',' || *txtpos == ';') txtpos++;
  } else {
    printmsg("? ", 0);
  }

  char *save_txtpos = txtpos;                                                         //Den BASIC-Pointer (txtpos) kurz sichern

  getln(0);
  char *inputPtr = inputBuffer;
  txtpos = save_txtpos;                                                               //Zurück zum alten BASIC-Programm-Pointer für die Variablen-Suche

  while (true) {
    int t = getCommandToken();
    if (t != TOKEN_VARIABLE) return;

    int tV1 = vIdx1;
    int tV2 = vIdx2;

    char *nextPtr;
    double inVal = strtod(inputPtr, &nextPtr);
    variables[tV1][tV2] = inVal;

    inputPtr = nextPtr;
    while (*inputPtr != '\0' && !isdigit(*inputPtr) && *inputPtr != '-' && *inputPtr != '.') {
      inputPtr++;
    }

    spaces();
    if (*txtpos == ',') {
      txtpos++;
    } else {
      line_terminator();
      return;
    }
  }
}


/*
  void cmd_input() {
  char c;
  char *tmptxt;

  spaces();
  if (*txtpos == '"') {
    txtpos++;
    while (*txtpos != '"' && *txtpos != '\0') print(*txtpos++);
    if (*txtpos == '"') txtpos++;
    spaces();
    if (*txtpos == ',' || *txtpos == ';') txtpos++;
  } else {
    printmsg("? ", 0);
  }
  // --- NEU: ALTE RESTE ENTFERNEN ---
  delay(10);                                                                                        // Kurz warten, falls noch Zeichen eintrudeln

  while (Serial.available() > 0) Serial.read();

  while (Serial.available() == 0) {                                                                 // 2. WARTEN, bis der User wirklich anfängt zu tippen

    yield(); // Watchdog füttern
  }

  String inputLine = Serial.readStringUntil('\n');
  println();
  inputLine.trim();

  char *inputPtr = (char*)inputLine.c_str();                                                        // Pointer auf den Anfang unserer gelesenen Zeile setzen

  while (true) {

    int t = getCommandToken();                                                                             // Variable im BASIC-Quelltext suchen
    if (t != TOKEN_VARIABLE) {                                                                      // Wenn nach dem Komma nichts mehr kommt oder Fehler
      return;
    }

    int tV1 = vIdx1;
    int tV2 = vIdx2;

    char *nextPtr;
    double inVal = strtod(inputPtr, &nextPtr);                                                      // Zahl aus dem eingelesenen String extrahieren
    variables[tV1][tV2] = inVal;

    inputPtr = nextPtr;                                                                             // Im String weiterhüpfen

    while (*inputPtr != '\0' && !isdigit(*inputPtr) && *inputPtr != '-' && *inputPtr != '.') {      // Im eingelesenen String zum nächsten Trenner springen (Komma/Leerzeichen)
      inputPtr++;
    }
    spaces();
    if (*txtpos == ',') {                                                                          // 5. Prüfen, ob im BASIC-CODE ein Komma für die nächste Variable steht
      txtpos++;
    } else {
      line_terminator();
      return;
    }
  }
  }
*/

void cmd_read() {
  while (isRunning) {
    // 1. Welche Variable/Array soll befüllt werden?
    int t = getCommandToken();
    if (t != TOKEN_VARIABLE) {
      syntaxerror(valmsg);
      isRunning = false;
      return;
    }

    // Indizes sichern (z.B. für O und X)
    int targetV1 = vIdx1;
    int targetV2 = vIdx2;
    bool isStr = isStringVar;

    // 2. Datenquelle prüfen (DATA-Pointer)
    if (dataPtr == NULL || *dataPtr == '\0' || *dataPtr == ':') {
      findNextData();
    }
    if (dataLineIdx >= lineCount) {
      syntaxerror(datamsg);
      isRunning = false;
      return;
    }

    // 3. Wert aus der DATA-Zeile lesen
    // Wir nutzen expression() auf dem dataPtr, damit auch DATA 10+5 geht!
    const char* savedTxtpos = txtpos; // Haupt-Pointer sichern
    txtpos = dataPtr;
    spaces();
    if (*txtpos == ',') txtpos++; // Komma in DATA-Zeile überspringen

    double val = 0;
    String sVal = "";

    if (isStr) {
      sVal = parseStringExpression();
    } else {
      val = expression();
    }

    dataPtr = txtpos; // DATA-Pointer aktualisieren
    txtpos = savedTxtpos; // Haupt-Pointer zurücksetzen

    // 4. In Ziel schreiben (Variable oder ARRAY)
    spaces();
    if (*txtpos == '(') {
      // Es ist ein Array! Wir nutzen deine Logik zum Index-Berechnen
      // Wir müssen aber den Wert 'val'/'sVal' dort hineinbekommen.

      // Hilfs-Logik: Wir "faken" eine Zuweisung für set_array_value
      // oder wir rufen die Index-Logik direkt auf:
      txtpos++; // '(' überspringen
      int x = (int)expression(); if (*txtpos == ',') txtpos++;
      int y = (int)expression(); if (*txtpos == ',') txtpos++;
      int z = (int)expression(); if (*txtpos == ')') txtpos++;

      int i = findArray(targetV1, targetV2, isStr);
      if (i != -1) {
        int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);
        if (isStr) allArrays[i].strData[idx] = sVal;
        else allArrays[i].numData[idx] = val;
      } else {
        syntaxerror(' '); printmsg(dimmsg, 0);
        isRunning = false; return;
      }
    } else {
      // Normale Variable
      if (isStr) stringVars[targetV1][targetV2] = sVal;
      else variables[targetV1][targetV2] = val;
    }

    // 5. Nächste Variable im READ-Befehl? (READ A, B, OX(1))
    spaces();
    if (*txtpos == ',') {
      txtpos++;
    } else {
      break;
    }
  }
}



void findNextData() {
  while (dataLineIdx < lineCount) {
    // Falls dataPtr NULL ist, fangen wir am Anfang der aktuellen Zeile an
    if (dataPtr == NULL) {
      dataPtr = program[dataLineIdx].text;
    }

    // Wir suchen das Wort "DATA" in der aktuellen Zeile
    char* found = strstr(dataPtr, "DATA");

    if (found) {
      dataPtr = found + 4; // Direkt hinter "DATA" springen
      // WICHTIG: Alle Leerzeichen nach DATA überspringen, bis zur ersten Zahl
      while (*dataPtr == ' ' || *dataPtr == ',') dataPtr++;
      if (*dataPtr != '\0') return; // Erfolg: Wir haben Daten gefunden!
    }

    // Wenn in dieser Zeile kein DATA mehr ist, ab zur nächsten Zeile
    dataLineIdx++;
    dataPtr = NULL;
  }
}

void cmd_restore() {
  spaces();

  // Prüfen, ob eine Zeilennummer folgt (Ziffer oder Ausdruck)
  if (isdigit(*txtpos)) {
    int targetLineNum = (int)expression();
    bool found = false;

    // Suche nach der Zeile mit dieser Nummer im Programm-Array
    for (int i = 0; i < lineCount; i++) {
      if (program[i].number == targetLineNum) {
        dataLineIdx = i;    // Setze den Daten-Zeilenindex auf diesen Index
        dataPtr = NULL;     // Pointer zurücksetzen, damit findNextData() dort sucht
        found = true;
        break;
      }
    }

    if (!found) {
      print("RESTORE LINE NOT FOUND: ");
      println(targetLineNum);
      isRunning = false;
    }
  } else {
    // Klassisches RESTORE ohne Nummer: Alles auf den Programmanfang setzen
    dataLineIdx = 0;
    dataPtr = NULL;
  }
}

String parseStringExpression() {
  String res = "";
  spaces();

  // --- Teil 1: Basis-Element (Literal, Funktion oder Variable) ---
  if (*txtpos == '"') { // 1. String Literal "TEXT"
    txtpos++;
    const char* start = txtpos;
    while (*txtpos != '"' && *txtpos != '\0') txtpos++;
    int len = txtpos - start;
    res = String(start).substring(0, len); // Schneller als zeichenweise +=
    if (*txtpos == '"') txtpos++;
  }
  else {
    // Nutze scanKeyword, um Funktionen wie LEFT$, MID$, etc. zu finden
    int t = scanKeyword(functions);

    switch (t) {
      case TOKEN_SPC: {
          if (*txtpos == '(') {
            txtpos++;
            int n = (int)expression();
            if (*txtpos == ')') txtpos++;
            res.reserve(n);
            for (int i = 0; i < n; i++) res += ' ';
          }
          break;
        }

      case TOKEN_LEFT:
      case TOKEN_RIGHT:
      case TOKEN_MID: {
          if (*txtpos == '(') {
            txtpos++;
            String s = parseStringExpression();
            int p1 = 0, p2 = -1; // p1 = start/n, p2 = len
            if (*txtpos == ',') {
              txtpos++;
              p1 = (int)expression();
            }
            if (t == TOKEN_MID && spaces() == ',') {
              txtpos++;
              p2 = (int)expression();
            }
            if (*txtpos == ')') txtpos++;

            if (t == TOKEN_LEFT) res = s.substring(0, p1);
            else if (t == TOKEN_RIGHT) {
              if (p1 > s.length()) p1 = s.length();
              res = s.substring(s.length() - p1);
            } else { // MID$
              p1--; // 1-basiert zu 0-basiert
              if (p1 < 0) p1 = 0;
              if (p2 == -1) res = s.substring(p1);
              else res = s.substring(p1, p1 + p2);
            }
          }
          break;
        }

      case TOKEN_STR:
      case TOKEN_CHR: {
          if (*txtpos == '(') {
            txtpos++;
            double val = expression();
            if (*txtpos == ')') txtpos++;
            if (t == TOKEN_STR) {
              res = (val == (long)val) ? String((long)val) : String(val, precisionValue);
            } else {
              res = String((char)(int)val);
            }
          }
          break;
        }
      case TOKEN_STRING: {
          if (*txtpos == '(') {
            txtpos++;
            double val = expression();
            if (*txtpos == ',') {
              txtpos++;
              String p2 = parseStringExpression();
              if (*txtpos == ')') txtpos++;
              for (int i = 0; i < val; i++) {
                res += p2;
              }
            }
          }
          break;
        }

      case TOKEN_TIME: {                                                                       //Uhrzeit-String
          char buffer[9];
          snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour(), minute(), second());
          res = String(buffer);
        }
        break;

      case TOKEN_DATE: {                                                                      //Datums-String
          char buffer[11];
          snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", day(), month(), year());
          res = String(buffer);
        }
        break;

      default: // 3. String-Variable A$ oder Array S$(i)
        if (isalpha(*txtpos)) {
          vIdx1 = toupper(*txtpos) - 'A';
          vIdx2 = 0;
          txtpos++;
          if (isalnum(*txtpos) && *txtpos != '$') {
            vIdx2 = getVIdx2(*txtpos);
            txtpos++;
          }
          if (*txtpos == '$') {
            txtpos++;
            if (*txtpos == '(') res = get_string_array_value();
            else res = stringVars[vIdx1][vIdx2];
          }
        }
        break;
    }
  }

  // --- Teil 2: String-Konkatenation (Addition mit +) ---
  spaces();
  if (*txtpos == '+') {
    txtpos++;
    String nextPart = parseStringExpression();
    res.reserve(res.length() + nextPart.length()); // Optimiert den RAM-Zugriff
    res += nextPart;
  }

  return res;

}


//************************************************************ GOSUB / RETURN **********************************************************
void cmd_on() {
  int index = (int)expression();
  int jumpToken = getCommandToken(); // GOTO oder GOSUB

  if (jumpToken != TOKEN_GOTO && jumpToken != TOKEN_GOSUB) {
    println("Syntax Error: ON expects GOTO or GOSUB");
    isRunning = false;
    return;
  }

  int currentIdx = 1;
  bool jumpedToTarget = false;

  while (true) {
    spaces();
    int targetLine = (int)expression(); // Liest die nächste Nummer in der Liste

    if (currentIdx == index) {
      if (jumpToken == TOKEN_GOSUB) {
        if (gosubStackPtr < MAX_GOSUB_STACK) {
          // Wir suchen das Ende dieser ON-Anweisung für den RETURN-Punkt
          const char* p = txtpos;
          while (*p != '\0' && *p != ':' && *p != '\r' && *p != '\n') p++;

          gosubStack[gosubStackPtr].lineIndex = currentLineIndex;
          gosubStack[gosubStackPtr].txtPos = p;
          gosubStackPtr++;
        } else {
          println("GOSUB STACK OVERFLOW");
          isRunning = false;
          return;
        }
      }

      if (jumpTo(targetLine)) {
        // jumpTo setzt bereits currentLineIndex, wir setzen nur noch txtpos
        txtpos = program[currentLineIndex].text;
        jumped = true;
        jumpedToTarget = true;
      }
      break;
    }

    spaces();
    if (*txtpos == ',') {
      txtpos++;
      currentIdx++;
    } else {
      // Index nicht gefunden in der Liste -> Wir müssen txtpos ans Ende
      // der Liste setzen, damit der Interpreter danach normal weiter macht.
      break;
    }
  }

  // Falls wir NICHT gesprungen sind, überspringe den Rest der Zahlenliste
  if (!jumpedToTarget) {
    while (*txtpos != '\0' && *txtpos != ':' && *txtpos != '\r' && *txtpos != '\n') {
      txtpos++;
    }
  }
}

void cmd_gosub() {
  if (getFunctionToken() == TOKEN_NUMBER) {
    int targetNum = (int)lastNumberValue;

    if (gosubStackPtr < MAX_GOSUB_STACK) {
      // Rücksprungpunkt speichern:
      // Wir speichern die aktuelle Zeile und die Position NACH der GOTO-Nummer
      gosubStack[gosubStackPtr].lineIndex = currentLineIndex;
      gosubStack[gosubStackPtr].txtPos = txtpos;
      gosubStackPtr++;

      // Den Sprung ausführen (nutzt deine existierende jumpTo Logik)
      if (jumpTo(targetNum)) {
        txtpos = program[currentLineIndex].text;
        jumped = true; // Wichtig für den Haupt-Loop!
      } else {
        syntaxerror("GOSUB LINE NOT FOUND");
      }
    } else {
      syntaxerror("GOSUB STACK OVERFLOW");
    }
  } else {
    syntaxerror("GOSUB EXPECTS LINE NUMBER");
  }
}

void cmd_return() {
  if (gosubStackPtr > 0) {
    gosubStackPtr--;

    // 1. Position vom Stack holen
    currentLineIndex = gosubStack[gosubStackPtr].lineIndex;
    txtpos = gosubStack[gosubStackPtr].txtPos;

    // 2. WICHTIG: Prüfen, was an der Rücksprungstelle steht
    spaces();

    if (*txtpos == ':') {
      // Es folgt ein weiterer Befehl in derselben Zeile (z.B. GOSUB 100: PRINT "A")
      txtpos++; // Überspringe den Doppelpunkt
      jumped = true;
    } else if (*txtpos == '\0') {
      // Die Zeile mit dem GOSUB ist zu Ende.
      // Wir setzen jumped auf FALSE, damit der Haupt-Loop die NÄCHSTE Zeile lädt.
      jumped = false;
    } else {
      // Sicherheitsfall für alles andere
      jumped = true;
    }
  } else {
    syntaxerror("RETURN WITHOUT GOSUB");
  }
}


//************************************************************ SD-Card-Funktionen **********************************************************
void cmd_files() {
  int zeilen = 0;
  spaces();
  drawCursor(false);

  String filter = "";
  if (*txtpos == '"' || isalpha(*txtpos)) {
    filter = parseStringExpression();
    filter.toUpperCase();
  }

  File root = SD.open("/");
  if (!root) {
    println("Fehler: SD-Karte nicht bereit!");
    return;
  }

  // Zähler für die Statistik
  int fileCount = 0;
  uint64_t totalFilesSize = 0;

  print("Files on SD-Crad");
  if (filter != "") {
    print(" (Filter: *"); print(filter); print("*)");
  }
  println(":");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String fileName = String(entry.name());
    String fileNameUpper = fileName;
    fileNameUpper.toUpperCase();
    // int f = check_extension(fileNameUpper);
    if (filter == "" || fileNameUpper.indexOf(filter) != -1) {
      fileCount++;
      totalFilesSize += entry.size();
      x_pos = 1;
      fbcolor(YELLOW, 0);
      if (fileNameUpper.indexOf(".BAS") != -1) fbcolor(CYAN, 0);
      else if (fileNameUpper.indexOf(".BIN") != -1) fbcolor(GREEN, 0);
      else if (fileNameUpper.indexOf(".BMP") != -1) fbcolor(RED, 0);
      print(fileName);
      fbcolor(WHITE, 0);
      if (entry.isDirectory()) {
        fbcolor(YELLOW, 0);
        print("/");
        x_pos = 21;
        println("< dir >");
        zeilen++;
        entry.close();
        continue;
      } else {
        //if (fileName.length() < 8) print("\t");
        //print("\t");
        x_pos = 20;                                     //horizontale Ausgabeposition festlegen
        print(entry.size());
        //println(" Bytes");

      }
      // 2. Datum und Zeit am Ende der Zeile

      x_pos = 29;
      // 2. Datum und Zeit am Ende der Zeile
      DateTimeFields tm;
      if (entry.getModifyTime(tm)) { // Übergabe der Struktur statt Pointer
        int year = tm.year + 1900;
        int month = tm.mon + 1;
        int day = tm.mday;
        int hour = tm.hour;
        int minute = tm.min;
        if (day < 10) print("0"); print(day); print(".");
        if (month < 10) print("0"); print(month); print(".");
        println(year);
      }

      zeilen++;
      if (zeilen == 20) {
        if (waitkey()) {
          entry.close();
          break;
        }
        zeilen = 0;
      }
    }
    entry.close();

  }
  root.close();

  // --- Kapazitätsberechnung ---
  // SD.totalSize() und SD.usedSize() geben Bytes als uint64_t zurück
  uint64_t totalCardBytes = SD.totalSize();
  uint64_t usedCardBytes = SD.usedSize();
  uint64_t freeCardBytes = totalCardBytes - usedCardBytes;

  println("----------------------------------------");
  print(fileCount); println(" Files found.");

  print("Used memory:   ");
  printSmartSize(usedCardBytes);

  print("Free memory:");
  printSmartSize(freeCardBytes);

  print("Total capacity: ");
  printSmartSize(totalCardBytes);

  drawCursor(cursor_on_off);
}

// Hilfsfunktion zur schöneren Größenanzeige (B, KB, MB, GB)
void printSmartSize(uint64_t bytes) {
  if (bytes < 1024) {
    print((long)bytes); println(" B");
  } else if (bytes < 1048576) {
    print((long)(bytes / 1024)); println(" KB");
  } else if (bytes < 1073741824) {
    print((long)(bytes / 1048576)); println(" MB");
  } else {
    print((float)(bytes / 1073741824.0), 2); println(" GB");
  }
}

void cmd_rename() {
  spaces();

  // 1. Alten Dateinamen lesen
  String oldName = parseStringExpression();

  spaces();
  if (*txtpos != ',') {
    println("SYNTAX ERROR: Komma erwartet (RENAME alt, neu)");
    return;
  }
  txtpos++; // Überspringe das Komma

  // 2. Neuen Dateinamen lesen
  String newName = parseStringExpression();

  if (oldName.length() == 0 || newName.length() == 0) {
    println("SYNTAX ERROR: Dateinamen fehlen");
    return;
  }

  // 3. Umbenennen auf der SD-Karte
  if (SD.exists(oldName.c_str())) {
    if (SD.rename(oldName.c_str(), newName.c_str())) {
      print("Datei umbenannt in: ");
      println(newName);
    } else {
      println("FEHLER: Umbenennen fehlgeschlagen.");
    }
  } else {
    print("FEHLER: Datei nicht gefunden: ");
    println(oldName);
  }
}

void cmd_load() {
  spaces();
  String fileNameStr = parseStringExpression();
  if (fileNameStr.length() == 0) {
    println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // 2. Datei auf der SD-Karte des Teensy 4.1 suchen
  if (!SD.exists(fileName)) {
    print("FILE NOT FOUND: ");
    println(fileName);
    return;
  }

  File file = SD.open(fileName);
  if (!file) {
    println("ERROR: Could not open file");
    return;
  }

  cmd_new();

  print("Loading "); println(fileName);

  char lineBuffer[128];
  while (file.available()) {
    int len = 0;
    // Zeile aus Datei lesen
    while (file.available() && len < 127) {
      char c = file.read();
      if (c == '\n') break;
      if (c != '\r') lineBuffer[len++] = c;
    }
    lineBuffer[len] = '\0';

    const char* p = lineBuffer;
    while (*p && isspace(*p)) p++; // spaces() Ersatz für lokalen Buffer

    if (isdigit(*p)) {
      int lineNumber = atoi(p);
      while (isdigit(*p)) p++;
      // storeLine speichert den Rest der Zeile unter dieser Nummer
      storeLine(lineNumber, p);
    }
  }

  file.close();
  // Wichtig: Nach dem Laden sicherstellen, dass isRunning false ist
  isRunning = false;
}


void cmd_save() {
  spaces();
  String fileNameStr = parseStringExpression();

  if (fileNameStr.length() == 0) {
    println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // 2. Falls Datei schon existiert, löschen (Überschreiben)
  if (SD.exists(fileName)) {
    SD.remove(fileName);
  }

  // 3. Datei zum Schreiben öffnen
  File file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    print("ERROR: Could not create ");
    println(fileName);
    return;
  }

  print("Saving to ");
  println(fileName);

  // 4. Zeile für Zeile speichern
  for (int j = 0; j < lineCount; j++) {
    file.print(program[j].number);
    file.print(" ");
    file.println(program[j].text);
  }

  file.close();
}


void cmd_delete() {
  spaces();
  String fileNameStr = parseStringExpression();

  if (fileNameStr.length() == 0) {
    println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // 2. Prüfen, ob die Datei auf der SD-Karte existiert
  if (SD.exists(fileName)) {
    // 3. Datei löschen
    if (SD.remove(fileName)) {
      print("File ");
      print(fileName);
      println(" deleted.");
    } else {
      println("ERROR: Could not delete file.");
    }
  } else {
    print("FILE NOT FOUND: ");
    println(fileName);
  }
}

void cmd_copy() {
  spaces();
  String srcName = parseStringExpression(); // Quell-Datei

  spaces();
  if (*txtpos != ',') {
    println("SYNTAX ERROR: Komma erwartet (COPY src, dest)");
    return;
  }
  txtpos++; // Komma überspringen

  String destName = parseStringExpression(); // Ziel-Datei

  if (srcName == "" || destName == "") return;

  if (!SD.exists(srcName.c_str())) {
    print("FILE NOT FOUND: "); println(srcName);
    return;
  }

  // Falls Ziel existiert, löschen
  if (SD.exists(destName.c_str())) SD.remove(destName.c_str());

  File srcFile = SD.open(srcName.c_str(), FILE_READ);
  File destFile = SD.open(destName.c_str(), FILE_WRITE);

  if (srcFile && destFile) {
    print("Copying "); print(srcName); print(" to "); println(destName);

    // Puffer für schnelles Kopieren (Teensy hat genug RAM)
    uint8_t buffer[512];
    while (srcFile.available()) {
      int bytesRead = srcFile.read(buffer, sizeof(buffer));
      destFile.write(buffer, bytesRead);
    }

    destFile.close();
    srcFile.close();
  } else {
    println("ERROR: Could not open files for copying.");
    if (srcFile) srcFile.close();
    if (destFile) destFile.close();
  }
}
//************************************************************************************************************************
//************************************************ Arrays ****************************************************************

String get_string_array_value() {
  // Wir nutzen die globalen Indizes, die der Parser gerade gefunden hat
  int v1 = vIdx1;
  int v2 = vIdx2;

  if (*txtpos == '(') {
    txtpos++; // Überspringe '('
    int x = (int)expression();
    int y = 0, z = 0;

    if (*txtpos == ',') {
      txtpos++;
      y = (int)expression();
    }
    if (*txtpos == ',') {
      txtpos++;
      z = (int)expression();
    }

    if (*txtpos == ')') {
      txtpos++;
    } else {
      isRunning = false; return "";
    }

    // Suche mit BEIDEN Indizes nach dem String-Array
    int i = findArray(v1, v2, true);

    if (i == -1) {
      print("STRING ARRAY NOT DIMENSIONED: ");
      println(getVarName(v1, v2)); // Schönerer Error mit vollem Namen
      isRunning = false; return "";
    }

    // Index berechnen
    int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);

    // Sicherheitscheck
    int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
    if (idx >= totalSize || idx < 0) {
      println("STRING ARRAY INDEX OUT OF BOUNDS");
      isRunning = false; return "";
    }

    return allArrays[i].strData[idx];
  }

  // Wenn keine Klammer da ist: normale String-Variable zurückgeben
  return stringVars[v1][v2];
}

double get_array_value_factor() {
  // Wir nutzen die globalen Indizes vIdx1 und vIdx2, die der Parser
  // gerade in factor() oder getCommandToken() ermittelt hat.
  int v1 = vIdx1;
  int v2 = vIdx2;
  //println(isStringVar);
  bool sVar = isStringVar;

  if (*txtpos == '(') {
    txtpos++; // Überspringe '('
    int x = (int)expression();
    int y = 0, z = 0;

    if (*txtpos == ',') {
      txtpos++;
      y = (int)expression();
    }
    if (*txtpos == ',') {
      txtpos++;
      z = (int)expression();
    }

    if (*txtpos == ')') {
      txtpos++;
    } else {
      isRunning = false; return 0;
    }

    // Suche mit beiden Indizes (v1, v2) nach dem numerischen Array
    int i = findArray(v1, v2, sVar);

    if (i == -1) {
      print("ARRAY NOT DIMENSIONED: ");
      println(getVarName(v1, v2)); // Zeigt z.B. "OX" statt nur "O"
      isRunning = false; return 0;
    }

    // Flachen Index berechnen
    int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);

    // Sicherheitscheck
    int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
    if (idx >= totalSize || idx < 0) {
      println("ARRAY INDEX OUT OF BOUNDS");
      isRunning = false; return 0;
    }

    if (sVar) return 0;
    return allArrays[i].numData[idx];
  }

  // Wenn keine Klammer da ist, ist es eine normale Variable
  return variables[v1][v2];
}

int findArray(int v1, int v2, bool isString) {
  for (int i = 0; i < arrayCount; i++) {
    if (allArrays[i].vIdx1 == v1 && allArrays[i].vIdx2 == v2 && allArrays[i].isString == isString) {
      return i;
    }
  }
  return -1;
}

void cmd_dim() {
  bool sVar = false;

  do {
    spaces();
    if (!isalpha(*txtpos)) {
      isRunning = false;
      return;
    }

    // 1. Namen und IDs holen (z.B. O und X)
    vIdx1 = toupper(*txtpos) - 'A';
    vIdx2 = 0;
    txtpos++;

    if (isalnum(*txtpos) && *txtpos != '(' && *txtpos != '$') {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }

    int id1 = vIdx1;        //Variablen-Index sichern, da expression() sie verändert
    int id2 = vIdx2;

    if (*txtpos == '$') {
      sVar = true;
      txtpos++;
    }

    // 2. Dimensionen in Klammern lesen
    if (*txtpos != '(') {
      isRunning = false;
      return;
    }
    txtpos++;

    int d1 = (int)expression();
    int d2 = 0, d3 = 0;

    if (*txtpos == ',') {
      txtpos++;
      d2 = (int)expression();
      if (*txtpos == ',') {
        txtpos++;
        d3 = (int)expression();
      }
    }

    if (*txtpos == ')') {
      txtpos++;
    } else {
      isRunning = false; return;
    }

    // 3. Array im Speicher anlegen
    int sizeX = d1 + 1;
    int sizeY = d2 + 1;
    int sizeZ = d3 + 1;
    int totalSize = sizeX * sizeY * sizeZ;

    if (arrayCount < MAX_ARRAYS) {
      allArrays[arrayCount].vIdx1 = id1;
      allArrays[arrayCount].vIdx2 = id2;
      allArrays[arrayCount].isString = sVar;
      allArrays[arrayCount].dimX = sizeX;
      allArrays[arrayCount].dimY = sizeY;
      allArrays[arrayCount].dimZ = sizeZ;

      if (sVar) {
        allArrays[arrayCount].strData = new String[totalSize];
      } else {
        allArrays[arrayCount].numData = (double*)malloc(totalSize * sizeof(double));
        for (int i = 0; i < totalSize; i++) allArrays[arrayCount].numData[i] = 0.0;
      }
      arrayCount++;
    }

    spaces();
    // --- DER SCHLÜSSEL: Wenn ein Komma folgt, loope weiter ---
  } while (*txtpos == ',' && (txtpos++));
}

void cmd_assignment() {
  // Wir nutzen die vom Parser (getCommandToken) gesetzten Indizes
  int targetV1 = vIdx1;
  int targetV2 = vIdx2;
  bool sVar = isStringVar;

  // Schneller Check: Ist es eine einfache Zuweisung?
  if (*txtpos == '=') {
    txtpos++;
    if (isStringVar) {
      stringVars[targetV1][targetV2] = parseStringExpression();
    } else {
      variables[targetV1][targetV2] = expression();
    }
    return; // Sofort raus!
  }

  spaces();

  // --- FALL A: ARRAY-Zuweisung (z.B. OX(1) = 10 oder S$(1,1) = "A" + "B") ---
  if (*txtpos == '(') {
    txtpos++;

    int x = (int)expression();
    int y = 0, z = 0;
    if (*txtpos == ',') {
      txtpos++;
      y = (int)expression();
      if (*txtpos == ',') { // Falls 3D: Drittes Komma prüfen
        txtpos++;
        z = (int)expression();
      }
    }

    if (*txtpos == ')') {
      txtpos++;
    } else {
      syntaxerror(syntaxmsg); return;
    }

    spaces();
    if (*txtpos == '=') {
      txtpos++;

      // Suche das Array mit beiden Indizes (wichtig für OX, OY etc.)
      int i = findArray(targetV1, targetV2, sVar);

      if (i == -1) {
        syntaxerror(dimmsg);
        //println(getVarName(targetV1, targetV2)); // Zeigt jetzt "OX" statt nur "O"
        isRunning = false; return;
      }

      int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);
      int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;

      if (idx >= totalSize || idx < 0) {
        syntaxerror(dimmsg); return;
      }

      if (sVar) {
        allArrays[i].strData[idx] = parseStringExpression();
      } else {
        allArrays[i].numData[idx] = expression();
      }
    }
  }
  // --- FALL B: NORMALE Variable (z.B. A$ = "HI " + B$ oder OX = 5) ---
  else if (*txtpos == '=') {
    txtpos++;
    if (sVar) {
      stringVars[targetV1][targetV2] = parseStringExpression();
    } else {
      variables[targetV1][targetV2] = expression();
    }
  }
  else {
    syntaxerror(syntaxmsg);
  }
}

void skip_to_else() {
  while (*txtpos != '\0' && *txtpos != ':') {
    const char* savedPos = txtpos;
    int t = scanKeyword(commands);

    if (t == TOKEN_ELSE) return;                                // ELSE gefunden dann zurück
    txtpos = savedPos + 1;                                      // Falls es kein ELSE war, nur ein Zeichen weitergehen und weitersuchen
  }
  *txtpos = '\0';                                               // Kein ELSE in dieser Zeile gefunden -> Zeile beenden
}

void skip_to_wend() {
  int nesting = 1; // Wir starten IN einer While-Schleife

  while (currentLineIndex < lineCount) {
    spaces();

    // Zeilenende erreicht? Gehe zur nächsten Zeile
    if (*txtpos == '\0') {
      currentLineIndex++;
      if (currentLineIndex >= lineCount) break;
      txtpos = program[currentLineIndex].text;
      continue;
    }

    const char* pBefore = txtpos;
    int t = scanKeyword(commands);

    if (t == TOKEN_WHILE) {
      nesting++; // Innere Schleife gefunden
    }
    else if (t == TOKEN_WEND) {
      nesting--; // Ende einer Schleife gefunden
      if (nesting == 0) {
        // Das passende WEND zur ursprünglichen WHILE-Schleife gefunden
        spaces();
        return;
      }
    }

    // Wenn kein Keyword erkannt wurde oder wir noch tiefer verschachtelt sind
    if (pBefore == txtpos) {
      txtpos++;
    }
  }

  // Fehlerfall: WEND wurde nie gefunden
  isRunning = false;
  println("Error: WHILE without WEND");
}

/*
  void skip_to(int target1) {
  // Wir suchen nur noch nach einem Ziel (normalerweise TOKEN_ELSE)
  while (currentLineIndex < lineCount) {
    spaces();

    // Wenn die Zeile zu Ende ist, haben wir das implizite Ende des IF erreicht
    if (*txtpos == '\0' || *txtpos == '\n' || *txtpos == '\r') {
      return;
    }

    int t = scanKeyword(commands);

    // WICHTIG: WHILE muss weiterhin korrekt übersprungen werden
    if (t == TOKEN_WHILE) {
      skip_to_wend(); // Du brauchst eine separate Funktion für WEND
    }
    else if (t == target1) {
      return; // ELSE gefunden!
    }

    // Falls kein Keyword erkannt wurde, ein Zeichen weitergehen
    if (*txtpos != '\0') txtpos++;
  }
  }
*/
void skip_to(int target1, int target2) {
  int nesting = 0;
  while (currentLineIndex < lineCount) {
    spaces();
    if (*txtpos == '\0') {
      currentLineIndex++;
      if (currentLineIndex >= lineCount) break;
      txtpos = program[currentLineIndex].text;
      continue;
    }

    const char* pBefore = txtpos;
    int t = scanKeyword(commands);

    if (t == TOKEN_WHILE) {
      nesting++;
    }
    else if (t == TOKEN_WEND) {
      if (nesting > 0) {
        nesting--;
      } else {
        if (t == target1 || t == target2) return;
      }
    }
    /*
      else if (t == TOKEN_ELSE && nesting == 0) {
      if (t == target1 || t == target2) return;
      }*/

    if (pBefore == txtpos) txtpos++;
  }
}

//**************************************** While/Wend ********************************************************************
void cmd_while() {
  spaces();
  const char* conditionStart = txtpos;                                                          // 1. Position der Bedingung für den Stack merken
  bool condition = relation();                                                                  // 2. Bedingung auswerten (txtpos wandert hinter die Bedingung)

  if (condition) {
    bool alreadyOnStack = false;                                                                // Prüfung bereits in dieser Schleife (Rücksprung von WEND)?
    if (whileStackPtr > 0 && whileStack[whileStackPtr - 1].txtPos == conditionStart) {
      alreadyOnStack = true;
    }

    if (!alreadyOnStack) {
      if (whileStackPtr < MAX_WHILE_STACK) {
        whileStack[whileStackPtr].lineIndex = currentLineIndex;
        whileStack[whileStackPtr].txtPos = conditionStart;
        whileStackPtr++;
      } else {
        isRunning = false;
        syntaxerror(whilewendmsg);
        return;
      }
    }

    while (*txtpos != '\0' && *txtpos != ':') txtpos++;                                         // REST DER ZEILE IGNORIEREN (verhindert Assignment-Fehler)

  } else {
    skip_to(TOKEN_WEND, TOKEN_WEND);                                                            // BEDINGUNG FALSCH: Den gesamten Block bis zum WEND überspringen

    spaces();                                                                                   // Wir stehen jetzt am WEND. Wir müssen zur NÄCHSTEN Zeile
    if (*txtpos == '\0') {
      currentLineIndex++;
      if (currentLineIndex < lineCount) {
        txtpos = program[currentLineIndex].text;
      } else {
        isRunning = false;
      }
    } else if (*txtpos == ':') {
      txtpos++;
    }

    if (whileStackPtr > 0 && whileStack[whileStackPtr - 1].txtPos == conditionStart) {         // Schleife auf dem Stack  entfernen
      whileStackPtr--;
    }
    jumped = true;                                                                              // Dem Haupt-Loop signalisieren: Zeiger ist gesetzt
  }
}


void cmd_wend() {
  if (whileStackPtr > 0) {
    currentLineIndex = whileStack[whileStackPtr - 1].lineIndex;                                 // Springe zurück zum WHILE-Befehl
    txtpos = program[currentLineIndex].text;                                                    // Wir springen zum ANFANG der Zeile (Oder exakt zum Wort WHILE, falls du textPos dort gespeichert hast)
    jumped = true;
  } else {
    isRunning = false; syntaxerror("ERR: WEND WITHOUT WHILE");
  }
}
//************************************** DWRITE **************************************************************************
void cmd_dwrite() {
  spaces();
  int pin = (int)expression(); // Pin Nummer (z.B. 13)

  spaces();
  if (*txtpos == ',') {
    txtpos++;
    int state = (int)expression(); // Status (0 = LOW, 1 = HIGH)

    // Teensy Hardware-Befehl
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);

  } else {
    syntaxerror(syntaxmsg);
  }
}

//*********************************************** Pause-Befehl ****************************************************
void cmd_pause() {
  spaces();
  int ms = (int)expression();                                                       // Die Millisekunden einlesen (kann auch eine Rechnung sein, z.B. PAUSE 100*5)

  if (ms > 0) {
    if (ms > 100) {
      unsigned long start = millis();
      while (millis() - start < (unsigned long)ms) {

        if (break_marker) {
          isRunning = false;
          println("BREAK!");
          return;
        }
        yield();
      }
    } else {
      delay(ms);
    }
  }
}
//************************************************************************* MEM-Befehl ****************************************************************************************
double get_mem(int m) {
  int usedLines = lineCount;
  int freeLines = MAX_LINES - lineCount;                                            // MAX_LINES ist deine definierte Obergrenze
  long totalCapacity = MAX_LINES * sizeof(BasicLine);                               // Jede Zeile verbraucht sizeof(BasicLine)(84bytes)
  long usedBytes = lineCount * sizeof(BasicLine);
  long freeBytes = totalCapacity - usedBytes;

  switch (m) {
    case 1: return usedLines;
    case 2: return freeLines;
    case 3: return totalCapacity;
    case 4: return usedBytes;
    case 5: return freeBytes;
    default:
      break;
      
  }
  return 0;
}

void cmd_mem() {
  char buffer[30];
  printmsg("--- BASIC PROGRAM MEMORY ---", 1);
  itoa(get_mem(1), buffer, 10);
  printmsg("Zeilen belegt: ", 0); printmsg(buffer, 0);
  ltoa(get_mem(2), buffer, 10);
  printmsg(" / Frei: ", 0); printmsg(buffer, 1);
  ltoa(get_mem(3), buffer, 10);
  printmsg("Basic-RAM: ", 0); printmsg(buffer, 1);
  ltoa(get_mem(4), buffer, 10);
  printmsg("Basic-RAM: ", 0); printmsg(buffer, 0);
  printmsg(" Bytes belegt.", 1);

  // Dynamischer Speicher (für deine 3D-Arrays)
  printmsg("Freier Basic-RAM: ", 0);
  dtostrf(get_mem(5), 6, 0,  buffer);
  //dtostrf(external_psram_size *1024 *1024,12,0,buffer);
  printmsg(buffer, 0); // Die Funktion von oben
  printmsg(" Bytes", 1);
}

static int Test_char(char az)
{ //println(txtpos);
  // check for char az
  if (*txtpos != az)
  {
    syntaxerror(syntaxmsg);
    return 1;
  }
  txtpos++;
  spaces();
  //println(txtpos);
  return 0;
}

void cmd_settime()
{ int tagzeit[7];
  spaces();
  tagzeit[0] = abs(int(expression()));         //nur ganze Zahlen
  for (int i = 1; i < 6; i++)
  {
    if (Test_char(',')) {
      syntaxerror(syntaxmsg);
      return;
    }
    tagzeit[i] = abs(int(expression()));         //nur ganze Zahlen
  }
  setTime(tagzeit[0], tagzeit[1], tagzeit[2], tagzeit[3], tagzeit[4], tagzeit[5]);                   //h, mi, s, d, m, y);
  Teensy3Clock.set(now());
  spaces();
}

//########################################################## Memory-Dump #################################################
/*void hexMonitor(uint8_t* startAddr) {
  int zeilen;

  if (startAddr == NULL) {
    println("Error: Null Pointer");
    return;
  }
  while (1) {
    uint32_t relativeAddr = (uint32_t)((uintptr_t)startAddr - 0x70000000);
    vprintf("%06X: ", relativeAddr);
    // 2. 16 Bytes in HEX
    for (int j = 0; j < 8; j++) {
      vprintf("%02X ", startAddr[j]);
      print("   ");
    }
    // 3. 16 Bytes als ASCII
    for (int j = 0; j < 8; j++) {

      char c = startAddr[j];
      print((c >= 32 && c <= 126) ? c : '.');

    }
    println("|");
    startAddr += 8;

    zeilen++;
    if (zeilen == 20) {
      if (waitkey()) break;
    }
    zeilen = 0;
  }
  }
*/
void hexMonitor(uint8_t* startAddr) {
  int zeilen = 0; // 1. Initialisieren!

  if (startAddr == NULL) {
    println("Error: Null Pointer");
    return;
  }

  while (1) {
    uint32_t relativeAddr = (uint32_t)((uintptr_t)startAddr - 0x70000000);
    vprintf("%06X: ", relativeAddr);

    for (int j = 0; j < 8; j++) {
      vprintf("%02X ", startAddr[j]);
    }
    print("   ");

    for (int j = 0; j < 8; j++) {
      char c = startAddr[j];
      print((c >= 32 && c <= 126) ? c : '.');
    }
    println("|");

    startAddr += 8;
    zeilen++;

    if (zeilen >= 20) { // 2. Check gegen 20
      if (waitkey()) break; // Falls waitkey true (z.B. ESC), beenden
      zeilen = 0; // 3. NUR HIER zurücksetzen!
    }
  }
}


void cmd_dump() {
  uint32_t offset;
  spaces();

  // Wenn keine Eingabe erfolgt, fange bei 0 an
  if (*txtpos == '\0' || *txtpos == '\r' || *txtpos == '\n') {
    offset = 0;
  } else {
    offset = expression();
  }

  // Validierung: Offset darf maximal 8MB (0x7FFFFF) sein
  if (offset <= 0x7FFFFF) {
    // Addiere die PSRAM Basis-Adresse 0x70000000 zum Offset
    uint8_t* psramAddr = (uint8_t*)(0x70000000 + offset);

    // Den Monitor mit der berechneten Adresse aufrufen
    hexMonitor(psramAddr);
  } else {
    syntaxerror(memorymsg);//println("Error: Offset too large! Max is 0x7FFFFF (8MB).");
  }
}

void cmd_defn() {
  spaces();
  if (!isalpha(*txtpos)) {
    // Fehler: Funktionsname muss A-Z sein
    isRunning = false;
    return;
  }

  char fName = toupper(*txtpos++);
  int idx = fName - 'A';
  function[idx].name = fName;
  function[idx].paramCount = 0;

  // Parameter einlesen: (W,X,Y,Z)
  if (*txtpos == '(') {
    txtpos++;
    while (*txtpos != ')' && *txtpos != '\0' && function[idx].paramCount < 4) {
      spaces();
      if (isalpha(*txtpos)) {
        function[idx].params[function[idx].paramCount++] = toupper(*txtpos++);
      }
      spaces();
      if (*txtpos == ',') txtpos++;
    }
    if (*txtpos == ')') txtpos++;
  }

  spaces();
  if (*txtpos == '=') {
    txtpos++;
    // Hier speichern wir den Zeiger auf den Beginn der Formel
    function[idx].formula = txtpos;
  }

  // WICHTIG: Die Formel in dieser Zeile nicht ausführen!
  // Wir setzen txtpos ans Ende der Zeile.
  txtpos = (char*)"";
}


void cmd_pset() {
  spaces();

  int px = (int)expression(); // X-Koordinate parsen

  if (*txtpos == ',') txtpos++;
  else {
    syntaxerror(nullptr);
    return;
  }

  int py = (int)expression(); // Y-Koordinate parsen

  uint8_t col = 255; // Standardfarbe Weiß, falls keine Farbe angegeben wird
  if (*txtpos == ',') {
    txtpos++;
    col = (uint8_t)expression(); // Farbe parsen (0-255)
  }

  // Sicherheitscheck: Nur zeichnen, wenn innerhalb der Bildschirmgrenzen
  //if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
  vga.drawPixel(px, py, col);
  //}

}


void cmd_tron(unsigned long wait, int index) {
  print("<");
  print(program[index].number);
  print(">");
  delay(wait);
}

void drawBMP(const char* filename, int x, int y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    println("FILE NOT FOUND");
    return;
  }

  // Sehr vereinfachter Header-Check (Offset 18 = Breite, 22 = Höhe)
  bmpFile.seek(18);
  int32_t width = bmpFile.read() | (bmpFile.read() << 8);
  bmpFile.seek(22);
  int32_t height = bmpFile.read() | (bmpFile.read() << 8);
  bmpFile.seek(10); // Offset zum Pixelspeicher
  uint32_t offset = bmpFile.read() | (bmpFile.read() << 8);

  bmpFile.seek(offset);

  // Pixeldaten lesen und zeichnen (Teensy ist schnell genug für Einzelpixel)
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();

      vga.drawPixel(x + j, y + i, VGA_RGB(r, g, b));
    }
  }
  bmpFile.close();
  vga.waitSync();
}


Params getParams(int n) {
  Params p;
  for (int i = 0; i < n; i++) {
    p.val[i] = (int)expression();
    if (i < n - 1 && *txtpos == ',') txtpos++;
  }
  return p; // Die Struktur wird als Ganzes zurückgegeben
}



//#################################################################################### Hauptprogrammschleife ###########################################################################################
void Basic_interpreter() {
  int lineNumber;
  unsigned long tron_delay;

  while (1) {
    myusb.Task();

    if (!isRunning) {                             // --- 1. Direktmodus ---
      getln(1);
      spaces();
      if (*txtpos == '\0') continue;

      if (isdigit(*txtpos)) {
        lineNumber = atoi(txtpos);
        while (isdigit(*txtpos)) txtpos++;
        storeLine(lineNumber, txtpos);
        continue;
      }

    } else {
      if (currentLineIndex >= lineCount) {
        isRunning = false;
        continue;
      }
      if (!jumped) {
        txtpos = program[currentLineIndex].text;

      }

    }


    while (*txtpos != 0) {                     // --- 2. ZEILEN-SCHLEIFE ---

      jumped = false;
      myusb.Task();

      if (break_marker) {
        line_terminator();
        print("Break in Line ");
        println(program[currentLineIndex].number);           //Zeilennummer ausgeben
        currentLineIndex = 0;
        break_marker = false;
        isRunning = false;
      }

      if (*txtpos == ':') {
        txtpos++;
        continue;
      }
      spaces();

      int token = getCommandToken();

      if (token == TOKEN_END) break;

      switch (token) {
        case TOKEN_PRINT: cmd_print(); break;
        case TOKEN_LIST:  cmd_list();    break;
        case TOKEN_RUN:   cmd_run();   break;
        case TOKEN_NEW:   cmd_new();   break;
        case TOKEN_VARS:  doVars();    break;
        case TOKEN_STOP:  isRunning = false; println("END."); break;
        case TOKEN_REM:   skip_line(); break;
        case TOKEN_FILES:
        case TOKEN_DIR:  cmd_files(); break;
        case TOKEN_LOAD: cmd_load(); break;
        case TOKEN_SAVE: cmd_save(); break;
        case TOKEN_DELETE: cmd_delete(); break;
        case TOKEN_DIM: cmd_dim();    break;
        case TOKEN_GOSUB: cmd_gosub(); break;
        case TOKEN_RETURN: cmd_return(); break;
        case TOKEN_RENAME: cmd_rename(); break;
        case TOKEN_COPY: cmd_copy(); break;
        case TOKEN_DWRITE: cmd_dwrite(); break;
        case TOKEN_PAUSE:  cmd_pause();  break;
        case TOKEN_MEM: cmd_mem(); break;
        case TOKEN_ON:  cmd_on();  break;
        case TOKEN_STIME: cmd_settime(); break;
        case TOKEN_DUMP: cmd_dump(); break;
        case TOKEN_DEFN: cmd_defn(); break;
        case TOKEN_CLS:  cmd_cls(); break;
        case TOKEN_COL: cmd_color(); break;
        case TOKEN_PSET:  cmd_pset(); break;
        case TOKEN_EDIT: cmd_edit(); break;
        case TOKEN_PEN: {
            int c = (int)expression();
            fbcolor(c, 0);
            break;
          }
        
        case TOKEN_AND:                                        //Logisch UND

          break;

        case TOKEN_OR:                                         //Logisch OR

          break;

        case TOKEN_TRON: {
            if (*txtpos == '\0') tron_delay = 0;
            else tron_delay = (unsigned long)expression();
            tron_marker = true;
          }
          break;

        case TOKEN_TROFF: {
            tron_marker = false;
            tron_delay = 0;
          }
          break;
        case TOKEN_ELSE: {
            if (last_if_result) {
              if (tron_marker && isRunning) {                                                     // TRON-Funktion nur im RUN-Modus
                cmd_tron(tron_delay, currentLineIndex);
              }
              currentLineIndex++; // IF war wahr, also ELSE-Zeile komplett überspringen
              txtpos = program[currentLineIndex].text;
              jumped = true;
            }
          }
          break;

        case TOKEN_WHILE: cmd_while(); break;
        case TOKEN_WEND:  cmd_wend();  break;
        case TOKEN_GOTO: {
            int target = (int)expression();
            if (jumpTo(target)) {
              txtpos = program[currentLineIndex].text;
              jumped = true;
            } else {
              printmsg(wronglinenr, 0);
              println(target);
              print("in Line:");
              println(program[currentLineIndex].number);
              isRunning = false;
              txtpos = '\0';
            }

            //}
            break;
          }


        case TOKEN_VARIABLE: cmd_assignment(); break;

        case TOKEN_IF: {
            last_if_result = logical_expression(); //relation(); // Ergebnis für das ELSE speichern
            if (getCommandToken() != TOKEN_THEN) {
              isRunning = false;
              break;
            }
            if (tron_marker && isRunning) cmd_tron(tron_delay, currentLineIndex);                                                     // TRON-Funktion nur im RUN-Modus

            if (!last_if_result) {
              currentLineIndex++; // Springe zur nächsten Zeile (wo das ELSE steht)
              if (tron_marker && isRunning) cmd_tron(tron_delay, currentLineIndex);                                                     // TRON-Funktion nur im RUN-Modus
              txtpos = program[currentLineIndex].text;
              jumped = true;
            }
          }
          break;


        case TOKEN_FOR: {
            int t = getFunctionToken();                                               // ZUERST die Variable nach dem FOR einlesen (z.B. das 'I')

            if (t != TOKEN_VARIABLE) {
              isRunning = false;
              *txtpos = '\0';
              break;
            }
            int v1 = vIdx1;
            int v2 = vIdx2;

            spaces();
            while (isalnum(*txtpos) || *txtpos == '$') txtpos++;                      // 2. WICHTIG: Den Variablennamen (z.B. "I") überspringen!
            spaces();

            if (*txtpos == '=') {                                                     // 3. Jetzt das '=' suchen
              txtpos++;
            } else {
              isRunning = false;

              break;
            }

            variables[v1][v2] = expression();                                         // Startwert einlesen (z.B. die 1)

            if (getCommandToken() != TOKEN_TO) {                                      // TO suchen (getCommandToken schiebt den Pointer über 'TO' drüber)
              isRunning = false;
              break;
            }

            double target = expression();                                               //Zielwert einlesen (z.B. die 20)

            double step = 1.0;                                                          // Standard-Schrittweite
            const char* savedPos = txtpos;
            if (getCommandToken() == TOKEN_STEP) {
              step = expression();
            } else {
              txtpos = savedPos;                                                        // txtpos zurücksetzen, falls kein STEP da war
            }

            spaces();
            if (*txtpos == ':') txtpos++;

            if (forStackPtr < 26) {                                                     // Schleife auf den Stack legen
              forStack[forStackPtr].vIdx1 = v1;
              forStack[forStackPtr].vIdx2 = v2;
              forStack[forStackPtr].targetValue = target;
              forStack[forStackPtr].stepValue = step;
              forStack[forStackPtr].lineIndex = currentLineIndex;                       // Zeile UND die Position NACH dem FOR-Befehl merken
              forStack[forStackPtr].txtPos = txtpos;

              forStackPtr++;

            }
          }
          break;

        case TOKEN_NEXT: {
            int t = getFunctionToken();
            if (forStackPtr > 0) {
              int sIdx = forStackPtr - 1;
              int v1 = forStack[sIdx].vIdx1;
              int v2 = forStack[sIdx].vIdx2;

              variables[v1][v2] += forStack[sIdx].stepValue;                            // 1. Variable erhöhen

              bool keepRunning = (forStack[sIdx].stepValue > 0) ?                       // 2. Prüfen, ob Ziel erreicht (funktioniert für positive & negative Steps)
                                 (variables[v1][v2] <= forStack[sIdx].targetValue) :
                                 (variables[v1][v2] >= forStack[sIdx].targetValue);

              if (keepRunning) {
                txtpos = forStack[sIdx].txtPos;                                        // ZURÜCKSPRINGEN:
                currentLineIndex = forStack[sIdx].lineIndex;
                if (*txtpos == '\0') {
                  jumped = false;
                }
                else {
                  jumped = true;                                                        // Wichtig für die Zeilensteuerung!
                }
              } else {
                forStackPtr--;                                                          // Schleife beendet: Stack-Eintrag entfernen
              }
            } else {
              isRunning = false;
            }
          }
          break;
        case TOKEN_INPUT:
          cmd_input();
          break;

        case TOKEN_DATA:
          skip_line();
          break;

        case TOKEN_READ:
          cmd_read();
          break;

        case TOKEN_RESTORE:
          cmd_restore();
          break;

        case TOKEN_PIC: {
            spaces();
            Params lp = getParams(2);
            if (*txtpos == ',') txtpos++;
            String fileName = parseStringExpression();
            drawBMP(fileName.c_str(), lp.val[0], lp.val[1]);
            break;
          }

        case TOKEN_LINE: {
            spaces();
            Params lp = getParams(5);
            vga.drawline(lp.val[0], lp.val[1], lp.val[2], lp.val[3], lp.val[4]);
          }
          break;

        case TOKEN_RECT: {
            spaces();
            Params lp = getParams(5);
            if (lp.val[4] > 0) vga.drawRect(lp.val[0], lp.val[1], lp.val[2], lp.val[3], rgbToVGA(F_COL[0], F_COL[1], F_COL[2])); // RECT(x,y,w,h,color,fill)
            else {
              vga.drawline(lp.val[0], lp.val[1], lp.val[2], lp.val[1], rgbToVGA(F_COL[0], F_COL[1], F_COL[2])); //quer
              vga.drawline(lp.val[0], lp.val[1], lp.val[0], lp.val[3], rgbToVGA(F_COL[0], F_COL[1], F_COL[2])); //links runter
              vga.drawline(lp.val[2], lp.val[1], lp.val[2], lp.val[3], rgbToVGA(F_COL[0], F_COL[1], F_COL[2])); //rechts runter
              vga.drawline(lp.val[0], lp.val[3], lp.val[2], lp.val[3], rgbToVGA(F_COL[0], F_COL[1], F_COL[2])); //unten quer
            }

          }
          break;

        case TOKEN_CIRC: {
            spaces();
            Params lp = getParams(5);
            if (lp.val[4] > 0) vga.drawfilledellipse(lp.val[0], lp.val[1], lp.val[2], lp.val[3] , rgbToVGA(F_COL[0], F_COL[1], F_COL[2]) , rgbToVGA(H_COL[0], H_COL[1], H_COL[2]));
            else
              vga.drawellipse(lp.val[0], lp.val[1], lp.val[2], lp.val[3], rgbToVGA(F_COL[0], F_COL[1], F_COL[2]));
          }
          break;
        case TOKEN_CUR: {
            spaces();
            int c = (int) expression();
            if (c > 0) cursor_on_off = true;
            else cursor_on_off = false;

          }
        case TOKEN_ERROR:
          if (strlen(txtpos) > 0) {
            syntaxerror(syntaxmsg);
            print(program[currentLineIndex].number);
            println(program[currentLineIndex].text);
            isRunning = false;
            *txtpos = '\0';                                                               //Wichtig sonst gibt's ne Endlosschleife bei Error
            break;
          }
          break;

        default:

          break;
      }

      spaces();

      if (*txtpos == ':') {
        txtpos++;
        continue;                                                                         // Nächster Befehl in derselben Zeile
      }

      if (*txtpos != '\0') continue;                                                      // 3. Wenn die Zeile noch nicht zu Ende ist (z.B. nach THEN)

      if (isRunning ) {                                                                   //Nur wenn wir am echten Ende (\0) sind UND nicht gesprungen wurde:
        if (*txtpos == '\0') {                                                            // Wenn wir am Ende der Zeile stehen (\0)
          if (tron_marker) cmd_tron(tron_delay, currentLineIndex);                        // TRON-Funktion nur im RUN-Modus
          currentLineIndex++;
          if (currentLineIndex < lineCount) {
            txtpos = program[currentLineIndex].text;
            continue;
          } else {
            isRunning = false;
          }
        }
      }
      yield();
      break;

    }  //while(*txtpos != '\0')

    yield();                                                                                 //Watchdog-erholung
  }// while(1)
} //Basic-Interpreter


//------------------------------------------------------------------- RTC-auslesen --------------------------------------------------------------------------------------------------------------------
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}


bool editLine(char* buffer, int bufferSize, int& cursor, int& length) {
  char c;
  bool inEdit = true;

  while (inEdit) {
    c = inchar();

    switch (c) {
      case 13: // ENTER
      case 10:
        buffer[length] = '\0';
        outchar(13);
        return true;

      case 27: // ESC
        return false;


      case 216:  // LINKS
        if (cursor > 0) {
          drawCursor(false);
          cursor--;
          // (x_pos/y_pos Logik wie bisher)
          if (x_pos == 0) {
            x_pos = MAX_C - 1;
            y_pos--;
          }
          else {
            x_pos--;
          }
          drawCursor(cursor_on_off);
        }
        break;


      case 215: // RECHTS
        if (cursor < length) {
          outchar(buffer[cursor]);
          cursor++;
        }
        break;

      case 8: // BACKSPACE
      case 127:
        if (cursor > 0) {
          drawCursor(false);                                                                                //Cursor ausschalten
          for (int i = cursor - 1; i < length; i++) buffer[i] = buffer[i + 1];                              // Puffer-Logik
          length--;
          cursor--;

          if (x_pos == 0) {                                                                                 // Cursor zurücksetzen (Zeilensprung-Gefahr)
            x_pos = MAX_C - 1;
            y_pos--;
          } else {
            x_pos--;
          }

          int tempX = x_pos;                                                                                // Position merken
          int tempY = y_pos;
          for (int i = cursor; i < length; i++) outchar(buffer[i]);
          vga.drawText(x_pos * 8, y_pos * 8, " ", getVGA(F_COL), getVGA(H_COL), false);                     // Das ehemals letzte Zeichen am Bildschirm löschen
          vga.drawText((x_pos + 1) * 8, y_pos * 8, " ", getVGA(F_COL), getVGA(H_COL), false);

          x_pos = tempX;                                                                                    // Cursor an die gemerkte Stelle setzen
          y_pos = tempY;
          drawCursor(cursor_on_off);                                                                        //Cursor wieder einschalten
        }
        break;


      case 212:                                                                                             // ENTF / DELETE (Löscht das Zeichen RECHTS vom Cursor)
        if (cursor < length) {

          for (int i = cursor; i < length - 1; i++) {                                                      // 1. Puffer-Logik: Alles rechts vom Cursor eins nach links schieben
            buffer[i] = buffer[i + 1];
          }
          length--;
          buffer[length] = '\0';

          int tempX = x_pos;
          int tempY = y_pos;

          for (int i = cursor; i < length; i++) {                                                           // 3. Den restlichen Text ab der aktuellen Position neu schreiben
            outchar(buffer[i]);
          }

          vga.drawText(x_pos * 8, y_pos * 8, " ", getVGA(F_COL), getVGA(H_COL), false);                     // Das überflüssige Zeichen am Ende der Kette löschen

          x_pos = tempX;                                                                                    // Cursor wieder an die gemerkte Stelle zurücksetzen
          y_pos = tempY;
          drawCursor(cursor_on_off);
        }
        break;
      default:

        if (c >= 32 && length < bufferSize - 1) {

          for (int i = length; i > cursor; i--) buffer[i] = buffer[i - 1];                                  // Platz im Puffer schaffen (Einfügemodus)
          buffer[cursor] = c;
          length++;
          int startX = x_pos;                                                                               // Aktuelle Position merken
          int startY = y_pos;                                                                               // (Wichtig, damit wir nach dem Zeilenumbruch-Drucken zurückfinden)
          for (int i = cursor; i < length; i++) {                                                           // Das neue Zeichen und den gesamten Rest der Zeile drucken
            outchar(buffer[i]);                                                                             // outchar() sorgt hier automatisch für den Zeilenumbruch nach unten
          }
          cursor++;                                                                                         // Den Cursor intern um eins erhöhen
          startX++;                                                                                         // Den Cursor an die NEUE Position setzen
          if (startX >= MAX_C) {
            startX = 0;
            startY++;
          }

          x_pos = startX;
          y_pos = startY;
          drawCursor(cursor_on_off);
        }
        break;
    }
  }
  return false;
}


void cmd_edit() {


  int targetLine = (int)expression(); // Zeilennummer einlesen
  int foundIndex = -1;


  // 1. Zeile im Speicher suchen
  for (int i = 0; i < lineCount; i++) {
    if (program[i].number == targetLine) {
      foundIndex = i;
      break;
    }
  }

  if (foundIndex == -1) {
    println("LINE NOT FOUND");
    return;
  }

  // 2. Zeile in den inputBuffer laden
  memset(inputBuffer, 0, BUF_SIZE);
  strncpy(inputBuffer, program[foundIndex].text, BUF_SIZE - 1);

  int length = strlen(inputBuffer);
  int cursor = length; // Cursor ans Ende setzen

  // 3. Editor auf dieser Zeile starten
  // Wir geben die Zeilennummer aus, damit der User weiß, wo er ist
  print(targetLine); print(" ");
  print(inputBuffer);

  // Jetzt den Line-Editor nutzen (editLine gibt true bei Enter zurück)
  if (editLine(inputBuffer, BUF_SIZE, cursor, length)) {
    // Die bearbeitete Zeile wieder speichern
    storeLine(targetLine, inputBuffer);
  } else {
    println(" Aborted.");
  }
  *txtpos = '\0';
}


void OnPress(int unicode, uint8_t modifier, uint8_t keycode) {
  // ZUERST: Unicode prüfen.
  if (unicode > 31) {
    lastUsbChar = unicode;
    return; // Funktion beenden, 'M' ist sicher gespeichert.
  }

  // NUR WENN unicode 0 ist (Sondertasten), prüfen wir den Keycode
  if (unicode == 0) {
    switch (keycode) {
      case 41: lastUsbChar = 27;  break_marker = true; return; // ESC
    }
  }

  // ENTER & BACKSPACE separat abfangen (da unicode hier oft 0, 10 oder 13 ist)
  if (keycode == 40) {
    lastUsbChar = 13;
    return;
  }
  if (keycode == 42) {
    lastUsbChar = 8;
    return;
  }
  if (keycode == 41) {
    lastUsbChar = 27;
    break_marker = true;
    return;
  }
}

void setup() {
  /*
    Serial.begin(9600);
    delay(1000);
    if (CrashReport) {
    Serial.print(CrashReport); // Zeigt an, ob es ein Speicherfehler (MPU) oder ähnliches war
    }
  */
  myusb.begin();
  keyboard1.forceBootProtocol();
  keyboard1.attachPress(OnPress);
  setSyncProvider(getTeensy3Time);

  //Serial.begin(115200);
  //while (!Serial);

  vga_error_t err = vga.begin(VGA_MODE_640x240);//320x240);//352x240);//352x240);
  if (err != 0)
  {
    println("fatal error");
    while (1);
  }
  vga.get_frame_buffer_size(&fb_width, &fb_height);
  // In der Initialisierung (z.B. bei vga.init)
  for (int r = 0; r < MAX_R; r++) {
    for (int c = 0; c < MAX_C; c++) {
      screenBuffer[r][c] = ' ';
      colorBuffer_F[r][c] = 0xFF; // Standard: Weiß
      colorBuffer_H[r][c] = 0x00; // Standard: Schwarz
    }
  }
  cmd_cls();


  printmsg(" *** TEENSY BASIC ", 0);
  print(BasicVersion);
  printmsg(" by Zille-Soft ***", 1);
  printmsg("       **** Built ", 0);
  print(BuiltTime);
  printmsg(" ****", 1);
  printmsg("    *** ", 0);
  print((int)get_mem(5));
  printmsg(" Basic Bytes free ***", 1);
  println();
  if (!SD.begin(chipSelect)) {
    syntaxerror(sderrormsg);
  }
  else println(mountmsg);
  if (external_psram_size > 0) {
    print("PSRAM found:");
    print(external_psram_size);
    println("MB");
  } else {
    println("no PSRAM found!");
  }
}


void loop() {
  Basic_interpreter();
}
