///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//             Basic - Interpreter für TEENSY 4.1                                                                                                 //
//               for VGA monitor output - März 2026                                                                                               //
//                                                                                                                                                //
//                                                                                                                                                //
//      von:Reinhard Zielinski <zille09@gmail.com>                                                                                                //
//                                                                                                                                                //
//      Connections: SD-Card -> Builtin                                                                                                           //
//                   VGA-Beschaltung: R: 3(2k), 4(1k), 33(470) | G:11(2k), 13(1k), 2(470) | B:10(820), 12(390) | HSync:15(82) | VSync:8 (82)      //
//                   PCM5102 : BCK: 21, DIN 7, LCK 20                                                                                             //
//                                                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BasicVersion "1.9"
#define BuiltTime "01.05.2026"

//Logbuch
//Version 1.9 01.05.2026                   -Umstellung der Token-Suche auf binäre Suche
//                                         -dadurch wurde die Geschwindigkeit mehr als verdoppelt :-)
//                                         -Umbau der cmd_print auf Token jetzt auch funktionsfähig.
//                                         -lange Variablennamen mögich, es werden aber nur die ersten beiden Zeichen genutzt
//                                         -cmd_save mit Überschreib-Abfrage ergänzt
//                                         -362514 Zeilen/sek. (Fastest)
//
//Version 1.8 26.04.2026                   -USING für formatierte Ausgabe von Zahlen integriert PRINT USING"##.##",23.456 -> 23.46
//                                         -funktioniert auch in Kombination mit anderen Print-Anweisungen
//                                         -Farben des Startbildschirms geändert (Hintergrund Atari800 - blau)
//                                         -DIR/FILES - Ausgabe angepasst, durch feste x_pos Zuweisungen wurde die Darstellung im WINDOW zerstört
//                                         -jetzt sollte die WINDOW-Funktion endlich korrekt funktionieren
//                                         -162570 Zeilen/sek. (Fastest)
//
//Version 1.7 24.04.2026                   -Fehler in der Syntax-Highlightning Funktion behoben -> Strings wurden nicht korrekt farblich beendet
//                                         -Fensterverwaltung funktioniert jetzt zufrieden stellend 6 Fenster sind möglich, das Hauptfenster (0) kann nicht umdefiniert werden
//                                         -Reset-Funktion der Fenster-Parameter in clear_all hinzugefügt - bei NEW, LOAD oder RUN werden die Parameter auf grundwerte gesetzt
//                                         -RENUM Befehl zur Neunummerierung der Basic-Zeilen hinzugefügt (RENUM Startwert,Schrittweite)
//                                         -MAP, MIN, MAX, LOG, LN, TAN, CONS(train) hinzugefügt
//                                         -nach vielen Fehlversuchen funktioniert die Window-Funktion jetzt (hoffentlich) richtig
//                                         -Fehler in AND und OR Behandlung behoben
//
//Version 1.6 20.04.2026                   -Basic-Programmspeicher und Bildschirm-Array in den PSRAM verschoben -> dadurch werden an die 150kb im Teensy frei
//                                         -Umstellung der Bildschirmausgabe auf 640x480 Pixel
//                                         -Korrektur in storeline() -> führende Leerzeichen (nach der Zeilennummer) werden jetzt beim Speichern entfernt
//                                         -Funktion GPX(x,y) -> gibt den Farbwert des Pixels an Position x,y zurück, hinzugefügt
//                                         -optische Änderungen an der MEM-Ausgabe
//                                         -Cursor-Handling bei DIR-Ausgabe geändert ->  es wurden öfter Cursorblöcke am Ende einer Zeile ausgegeben
//                                         -PIC-Befehl um die Möglichkeit des Ladens von jpg/jpeg - Bilddateien erweitert
//                                         -Identifizierung erfolgt durch die Dateiendung
//                                         -Befehl NEW löscht jetzt den gesamten PSRAM-Programmspeicher korrekt (mit 0 aufgefüllt)
//                                         -Window-Befehl begonnen - sieht schon ganz gut aus
//
//Version 1.5 15.04.2026                   -Befehl RECT nach anfänglichen Schwierigkeiten eingebunden -> RECT x,y,w,h,fill
//                                         -Befehl LINE eingebunden -> LINE x,y,xx,yy
//                                         -Befehl CIRC eingebunden -> CIRC x,y,w,h,fill
//                                         -die Grafikbefehle führen noch zu Abstürzen wenn bsp.weise eine neue Basic-datei geladen werden soll - Screenpuffer ?
//                                         -DIR - Ausgabe etwas farblich angepasst
//                                         -Fehler in den print-Funktionen behoben, die Anzeige in cmd_files für die Ausgabe der noch freien und gesamten Bytes auf der SD-Karte war falsch
//                                         -alle Print-Funktionen nach Function_print.ino ausgelagert
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
#include <malloc.h>
extern "C" char* sbrk(int incr);        //für Memory-Ausgabe

#include <VGA_t4.h>                     //VGA Anzeige-Treiber
VGA_T4 vga;
#include <JPEGDEC.h>                    //JPEG-Decoder
JPEGDEC jpeg;

#define BUF_SIZE 80
char inputBuffer[BUF_SIZE];
static char *txtpos;
bool inQuotes = false;
extern "C" uint8_t external_psram_size;

static int fb_width, fb_height;
int currentIndent = 0;

const int fontH = 8;
const int MAX_C = 640 / 8;                 //Anzahl Textspalten
const int MAX_R = 480 / 8;                 //Anzahl Textzeilen
EXTMEM char screenBuffer[MAX_R][MAX_C];      // Bildschirm-Buffer: 30 Zeilen à 40 Zeichen
EXTMEM uint8_t colorBuffer_F[MAX_R][MAX_C];  // Speichert die Vordergrundfarbe (8-Bit VGA)
EXTMEM uint8_t colorBuffer_H[MAX_R][MAX_C];  // Speichert die Hintergrundfarbe (8-Bit VGA)
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
File myFile;                                // Globale Variable für das Dateiobjekt

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

//static char *current_line;
//typedef short unsigned LINENUM;

//**************************************************Audio-Einstellungen**************************************************************
unsigned long soundEndTime[3] = {0, 0, 0};
//********************************************************************************************************************************

#define MAX_GOSUB_STACK 25
struct GosubStack {
  int lineIndex;
  const char* txtPos;
};

GosubStack gosubStack[MAX_GOSUB_STACK];
int gosubStackPtr = 0;

bool break_marker = false;
bool jumped  = false;
bool isError = false;
double lastNumberValue = 0;
int precisionValue = 6; // Standardmäßig 6 Stellen

double variables[26][37];
int vIdx1 = 0; // Erster Buchstabe
int vIdx2 = 0; // Zweites Zeichen (0 = keines)
const char* lastTokenPos; // Speichert den Anfang des aktuellen Tokens
char lastVarName;  // Speichert den Buchstaben der aktuellen Variable (A-Z)

// 26 Buchstaben x 37 Folgezeichen (A-Z, 0-9, none)
String stringVars[26][37];
bool isStringVar = false; // Globales Flag
char lastStringValue[BUF_SIZE];
#define MAX_STRING_LEN BUF_SIZE

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

struct BasicWindow {
  int x, y, w, h;
  uint16_t tcolor;
  uint16_t fcolor;
  uint16_t bcolor;
  char* title;
  int curX, curY; // Eigener Cursor-Speicher für jedes Fenster
  bool active;    // Ist das Fenster gerade sichtbar?
};

#define MAX_WINDOWS 7
BasicWindow windows[MAX_WINDOWS];
int currentWinIdx = 0;        // Welches Fenster wird gerade mit PRINT beschrieben?

struct RenumMap {             //RENUM - Funktion
  int oldNum;
  int newNum;
};
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
  TOKEN_ERROR,            //0
  TOKEN_PRINT,            //erster Basic-Befehl
  TOKEN_GOTO,
  TOKEN_LIST,
  TOKEN_RUN,
  TOKEN_VARIABLE,         // A-Z
  TOKEN_NUMBER,           //Zahlen
  TOKEN_STRING_LITERAL,   //Strings
  TOKEN_NEW,
  TOKEN_FOR,
  TOKEN_TO,               //10
  TOKEN_NEXT,
  TOKEN_STEP,
  TOKEN_VARS,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_STOP,
  TOKEN_REM,
  TOKEN_INPUT,
  TOKEN_DATA,
  TOKEN_READ,             //20
  TOKEN_RESTORE,
  TOKEN_FILES,
  TOKEN_DIR,
  TOKEN_LOAD,
  TOKEN_SAVE,
  TOKEN_DELETE,
  TOKEN_DIM,
  TOKEN_GOSUB,
  TOKEN_RETURN,
  TOKEN_ELSE,             //30
  TOKEN_WHILE,
  TOKEN_WEND,
  TOKEN_RENAME,
  TOKEN_COPY,
  TOKEN_DWRITE,
  TOKEN_PAUSE,
  TOKEN_MEM,
  TOKEN_ON,
  TOKEN_STIME,
  TOKEN_DUMP,             //40
  TOKEN_DEFN,
  TOKEN_CLS,
  TOKEN_COL,
  TOKEN_PSET,
  TOKEN_TAB,
  TOKEN_AT,
  TOKEN_TRON,
  TOKEN_TROFF,
  TOKEN_PIC,
  TOKEN_LINE,             //50
  TOKEN_RECT,
  TOKEN_CIRC,
  TOKEN_PEN,
  TOKEN_CUR,
  TOKEN_EDIT,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_WINDOW,
  TOKEN_RENUM,
  TOKEN_USING,            //60
  TOKEN_SID,       //letzter Basic-Befehl
  TOKEN_RND,       //erster Funktions-Befehl
  TOKEN_SQR,
  TOKEN_SIN,
  TOKEN_COS,
  TOKEN_ABS,
  TOKEN_INT,
  TOKEN_PI,
  TOKEN_DEG,
  TOKEN_RAD,              //70
  TOKEN_SGN,
  TOKEN_LEN,
  TOKEN_ASC,
  TOKEN_VAL,
  TOKEN_DREAD,
  TOKEN_FRE,
  TOKEN_TIMER,
  TOKEN_LEFT,
  TOKEN_RIGHT,
  TOKEN_MID,              //80
  TOKEN_SPC,
  TOKEN_STR,
  TOKEN_CHR,
  TOKEN_BIN,
  TOKEN_HEX,
  TOKEN_OCT,
  TOKEN_GTIME,
  TOKEN_TIME,
  TOKEN_DATE,
  TOKEN_FN,               //90
  TOKEN_INKEY,
  TOKEN_STRING,
  TOKEN_EXP,
  TOKEN_GPX,
  TOKEN_MAP,
  TOKEN_CONSTRAIN,
  TOKEN_MAX,
  TOKEN_MIN,
  TOKEN_LOG,
  TOKEN_LN,               //100
  TOKEN_TAN,              //letzter Funktions-Befehl
  TOKEN_END
};

struct Keyword {
  const char* name;
  int token;
};

Keyword commands[] = {
  {"AND", TOKEN_AND},
  {"AT", TOKEN_AT},
  {"CIRC", TOKEN_CIRC},
  {"CLS", TOKEN_CLS},
  {"COL", TOKEN_COL},
  {"COPY", TOKEN_COPY},
  {"CUR", TOKEN_CUR},
  {"DATA", TOKEN_DATA},
  {"DEFN", TOKEN_DEFN},
  {"DEL", TOKEN_DELETE},
  {"DIM", TOKEN_DIM},
  {"DIR", TOKEN_DIR},
  {"DUMP", TOKEN_DUMP},
  {"DWRITE", TOKEN_DWRITE},
  {"EDIT", TOKEN_EDIT},
  {"ELSE", TOKEN_ELSE},
  {"END", TOKEN_STOP},
  {"FILES", TOKEN_FILES},
  {"FOR", TOKEN_FOR},
  {"GOSUB", TOKEN_GOSUB},
  {"GOTO", TOKEN_GOTO},
  {"IF", TOKEN_IF},
  {"INPUT", TOKEN_INPUT},
  {"LINE", TOKEN_LINE},
  {"LIST", TOKEN_LIST},
  {"LOAD", TOKEN_LOAD},
  {"MEM", TOKEN_MEM},
  {"NEW", TOKEN_NEW},
  {"NEXT", TOKEN_NEXT},
  {"ON", TOKEN_ON},
  {"OR", TOKEN_OR},
  {"PAUSE", TOKEN_PAUSE},
  {"PEN", TOKEN_PEN},
  {"PIC", TOKEN_PIC},
  {"PRINT", TOKEN_PRINT},
  {"PSET", TOKEN_PSET},
  {"READ", TOKEN_READ},
  {"RECT", TOKEN_RECT},
  {"REM", TOKEN_REM},
  {"RENAME", TOKEN_RENAME},
  {"RENUM", TOKEN_RENUM},
  {"RESTORE", TOKEN_RESTORE},
  {"RETURN", TOKEN_RETURN},
  {"RUN", TOKEN_RUN},
  {"SAVE", TOKEN_SAVE},
  {"SID", TOKEN_SID},
  {"STEP", TOKEN_STEP},
  {"STIME", TOKEN_STIME},
  {"TAB", TOKEN_TAB},
  {"THEN", TOKEN_THEN},
  {"TO", TOKEN_TO},
  {"TROFF", TOKEN_TROFF},
  {"TRON", TOKEN_TRON},
  {"USING", TOKEN_USING},
  {"VARS", TOKEN_VARS},
  {"WEND", TOKEN_WEND},
  {"WHILE", TOKEN_WHILE},
  {"WINDOW", TOKEN_WINDOW},
  {NULL, 0}
};

Keyword functions[] = {
  {"ABS", TOKEN_ABS},
  {"ASC", TOKEN_ASC},
  {"BIN", TOKEN_BIN},
  {"CHR$", TOKEN_CHR},
  {"CONS", TOKEN_CONSTRAIN},
  {"COS", TOKEN_COS},
  {"DATE$", TOKEN_DATE},
  {"DEG", TOKEN_DEG},
  {"DREAD", TOKEN_DREAD},
  {"EXP", TOKEN_EXP},
  {"FN", TOKEN_FN},
  {"FRE", TOKEN_FRE},
  {"GPX", TOKEN_GPX},
  {"GTIME", TOKEN_GTIME},
  {"HEX", TOKEN_HEX},
  {"INKEY", TOKEN_INKEY},
  {"INT", TOKEN_INT},
  {"LEFT$", TOKEN_LEFT},
  {"LEN", TOKEN_LEN},
  {"LN", TOKEN_LN},
  {"LOG", TOKEN_LOG},
  {"MAP", TOKEN_MAP},
  {"MAX", TOKEN_MAX},
  {"MID$", TOKEN_MID},
  {"MIN", TOKEN_MIN},
  {"OCT", TOKEN_OCT},
  {"PI", TOKEN_PI},
  {"RAD", TOKEN_RAD},
  {"RIGHT$", TOKEN_RIGHT},
  {"RND", TOKEN_RND},
  {"SGN", TOKEN_SGN},
  {"SIN", TOKEN_SIN},
  {"SPC", TOKEN_SPC},
  {"SQR", TOKEN_SQR},
  {"STR$", TOKEN_STR},
  {"STRING$", TOKEN_STRING},
  {"TAN", TOKEN_TAN},
  {"TIME$", TOKEN_TIME},
  {"TIMER", TOKEN_TIMER},
  {"VAL", TOKEN_VAL},
  {NULL, 0}
};

// Berechnet die Anzahl der Einträge (ohne den NULL-Eintrag am Ende)
const int numCommands = (sizeof(commands) / sizeof(Keyword)) - 1;
const int numFunctions = (sizeof(functions) / sizeof(Keyword)) - 1;

#define MAX_LINES 1650
#define LINE_LEN 80

struct BasicLine {
  int number;                                                                           // Die Zeilennummer (z.B. 10)
  char text[LINE_LEN];                                                                  // Der eigentliche Befehlstext
};

EXTMEM BasicLine program[MAX_LINES];
int lineCount = 0;                                                                      // Wie viele Zeilen aktuell im Speicher sind

#define BIN 2
#define OCT 8
#define DEC 10
#define HEX 16

struct RGB {
  uint8_t r, g, b;
};


void fbcolor(uint8_t vgaVal, bool vh) {
  // Globale Farb-Arrays für den System-Standard aktualisieren
  if (vh) {
    windows[currentWinIdx].bcolor = vgaVal;
  } else {
    windows[currentWinIdx].fcolor = vgaVal;
  }
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

void cmd_cls() {
  // Aktuelle Fenster-Eigenschaften holen
  int x_min = windows[currentWinIdx].x;
  int y_min = windows[currentWinIdx].y;
  int x_max = windows[currentWinIdx].w;
  int y_max = windows[currentWinIdx].h;

  uint8_t bg = windows[currentWinIdx].bcolor;
  uint8_t fg = windows[currentWinIdx].fcolor;

  if (currentWinIdx == 0) {
    // Spezialfall: Hauptbildschirm komplett löschen
    for (int r = 0; r < MAX_R; r++) {
      for (int c = 0; c < MAX_C; c++) {
        screenBuffer[r][c] = ' ';
        colorBuffer_F[r][c] = fg;
        colorBuffer_H[r][c] = bg;
      }
    }
    x_pos = 0;
    y_pos = 0;
    vga.clear(bg);
  }
  else {
    // Nur den Bereich des aktuellen Fensters löschen (ohne Rahmen/Titel)
    // Wir starten bei y_min + 1, um die Titelzeile stehen zu lassen
    for (int r = y_min + 1; r < y_max; r++) {
      for (int c = x_min; c < x_max; c++) {
        screenBuffer[r][c] = ' ';
        colorBuffer_F[r][c] = windows[currentWinIdx].fcolor;
        colorBuffer_H[r][c] = windows[currentWinIdx].bcolor;

        // Pixel auf dem Display löschen (8x8 Zeichenblock)
        //vga.drawText(c * 8, r * 8, " ", windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor, false);
        vga.drawRect(c * 8, r * 8, 8, 8, windows[currentWinIdx].bcolor);
      }
    }
    // Cursor an den Anfang des Fensters setzen (unter die Titelzeile)
    x_pos = x_min + 1;
    y_pos = y_min + 1;
  }

  drawCursor(cursor_on_off);
}

void drawCursor(bool state) {
  int x_min, x_max, y_min, y_max;

  // Grenzwerte wie in outchar berechnen
  if (currentWinIdx == 0) {
    x_min = 0; y_min = 0; x_max = MAX_C - 1; y_max = MAX_R - 1;
  } else {
    x_min = windows[currentWinIdx].x + 1;
    y_min = windows[currentWinIdx].y + 1;
    x_max = windows[currentWinIdx].w - 2; // Letzte Spalte vor Rahmen
    y_max = windows[currentWinIdx].h - 2; // Letzte Zeile vor Rahmen
  }

  // Sicherheitscheck: Nur zeichnen, wenn wir INNERHALB des Schreibbereichs sind
  if (x_pos < x_min || x_pos > x_max || y_pos < y_min || y_pos > y_max) {
    return;
  }

  int px = x_pos * 8;
  int py = y_pos * 8;

  if (state) {
    // Cursor AN: Invertierte Farben des Zeichens im Puffer
    char c = screenBuffer[y_pos][x_pos];
    if (c < 32) c = ' ';
    uint8_t fg = colorBuffer_F[y_pos][x_pos];
    uint8_t bg = colorBuffer_H[y_pos][x_pos];

    char buf[2] = {c, 0};
    vga.drawText(px, py, buf, bg, fg, false); // Invertiert: bg/fg vertauscht
  } else {
    // Cursor AUS: Originalzustand aus Buffer wiederherstellen
    char c = screenBuffer[y_pos][x_pos];
    if (c < 32) c = ' ';
    uint8_t fg = colorBuffer_F[y_pos][x_pos];
    uint8_t bg = colorBuffer_H[y_pos][x_pos];

    char buf[2] = {c, 0};
    vga.drawText(px, py, buf, fg, bg, false);
  }
  yield();
}

static void outchar(char c) {
  drawCursor(false);

  // 1. GRENZEN DEFINIEREN
  int x_min, x_max;
  int y_min, y_max;

  if (currentWinIdx == 0) {
    x_min = 0; y_min = 0; x_max = MAX_C; y_max = MAX_R - 1;
  } else {
    x_min = windows[currentWinIdx].x + 1;
    y_min = windows[currentWinIdx].y + 1;
    x_max = windows[currentWinIdx].w - 1;
    y_max = windows[currentWinIdx].h - 2; // Letzte Zeile ÜBER dem Rahmen
  }


  if (c == 13) { // ENTER
    x_pos = x_min;
    y_pos++;
  } else if (c == 10) {

    return;
  } else {
    if (y_pos > y_max) {
      y_pos = y_max;
    }

    if (x_pos > x_max) {
      x_pos = x_min;
      y_pos++;
    }

    if (y_pos > y_max) {
      Scroll();
      y_pos = y_max;
    }

    screenBuffer[y_pos][x_pos] = c;
    colorBuffer_F[y_pos][x_pos] = windows[currentWinIdx].fcolor;
    colorBuffer_H[y_pos][x_pos] = windows[currentWinIdx].bcolor;

    static char b[2] = {0, 0};
    b[0] = c;
    vga.drawText(x_pos * 8, y_pos * 8, b, windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor, false);

    x_pos++;
  }

  if (x_pos >= x_max) {
    x_pos = x_min;
    y_pos++;
  }
  if (y_pos > y_max) {
    Scroll();
    y_pos = y_max;
  }

  drawCursor(cursor_on_off);

}

void Scroll() {
  int x_min, x_max, y_min, y_max;
  if (currentWinIdx == 0) {
    x_min = 0; y_min = 0; x_max = MAX_C - 1; y_max = MAX_R - 1;
  } else {
    x_min = windows[currentWinIdx].x + 1;
    x_max = windows[currentWinIdx].w - 2;
    y_min = windows[currentWinIdx].y + 1;
    y_max = windows[currentWinIdx].h - 2;
  }

  for (int r = y_min + 1; r <= y_max; r++) {
    int count = (x_max - x_min) + 1;
    memcpy(&screenBuffer[r - 1][x_min], &screenBuffer[r][x_min], count);
    memcpy(&colorBuffer_F[r - 1][x_min], &colorBuffer_F[r][x_min], count);
    memcpy(&colorBuffer_H[r - 1][x_min], &colorBuffer_H[r][x_min], count);
  }

  for (int c = x_min; c <= x_max; c++) {
    screenBuffer[y_max][c] = ' ';
    colorBuffer_F[y_max][c] = windows[currentWinIdx].fcolor;
    colorBuffer_H[y_max][c] = windows[currentWinIdx].bcolor;
  }

  for (int r = y_min; r <= y_max; r++) {
    for (int c = x_min; c <= x_max; c++) {
      char b[2] = {(char)screenBuffer[r][c], 0};
      vga.drawText(c * 8, r * 8, b, colorBuffer_F[r][c], colorBuffer_H[r][c], false);
    }
  }

}

static void line_terminator()
{
  outchar(13);
}


double get_free_ram() {
  struct mallinfo mi = mallinfo();
  //uint32_t free_in_heap = mi.fordblks;
  uint32_t total_ram2 = 512 * 1024;  //HEAP des Teensy im Ram2
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
    vIdx2 = 0;
  }
}

int getVIdx2(char c) {
  if (c >= 'A' && c <= 'Z') return (c - 'A' + 1); // A=1, B=2...
  if (isdigit(c)) return (c - '0' + 27);          // 0=27, 1=28...
  return 0;                                       // Kein Folgezeichen
}

int scanKeyword(Keyword* list, int tableSize) {
  spaces();
  const char* startPos = txtpos; // Die echte Startposition merken
  const char* tempPos = txtpos;  // tempPos zum "Vorschauen" nutzen

  char buffer[32];
  int i = 0;

  while (isalnum(*tempPos) || *tempPos == '$') {
    buffer[i++] = toupper(*tempPos++);
    if (i >= 31) break;
  }
  buffer[i] = '\0';

  if (i == 0) {
    txtpos = startPos;
    return -1;
  }

  // 2. Binäre Suche
  int left = 0;
  int right = tableSize - 1;
  while (left <= right) {
    int mid = left + (right - left) / 2;
    int res = strcmp(buffer, list[mid].name);

    if (res == 0) {
      txtpos = tempPos;
      return list[mid].token;
    }
    if (res > 0) left = mid + 1;
    else right = mid - 1;
  }
  txtpos = startPos;                                  //txtpos zurücksetzen
  return -1;
}
//############################################### finde Befehls-Token ####################################################################

int getCommandToken() {

  spaces();
  int t = scanKeyword(commands, numCommands);
  //print(txtpos);
  if (t != -1) return t;

  if (isalpha(*txtpos)) {
    lastVarName = *txtpos;
    isStringVar = false;
    vIdx1 = *txtpos - 'A';
    vIdx2 = 0;
    txtpos++;
    // zweiter Buchstabe/Zahl (z.B. 'A1')
    if (isalnum(*txtpos)) {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }
    while (isalnum(*txtpos)) {
      txtpos++;
    }
    //$ für Strings überspringen!
    if (*txtpos == '$') {
      isStringVar = true;
      txtpos++;
    }
    return TOKEN_VARIABLE;
  }

  return TOKEN_ERROR;
}
//###################################################### finde Funktions-Token ###############################################################

int getFunctionToken() {
  //isStringVar = false;
  spaces();
  lastTokenPos = txtpos;
  if (*txtpos == '\0') return TOKEN_END;

  // Funktionen prüfen (SQR SIN etc.)
  int t = scanKeyword(functions, numFunctions);
  if (t != -1) return t;

  // Zahlen prüfen
  if (isdigit(*txtpos)) {
    char *endPtr;
    lastNumberValue = strtod(txtpos, &endPtr);
    txtpos = endPtr;
    return TOKEN_NUMBER;
  }

  // Strings
  if (*txtpos == '"') {
    txtpos++; // Überspringe das öffnende "
    int len = 0;

    // Kopiere den Inhalt in einen Puffer (z.B. lastStringValue), bis das schließende " kommt
    while (*txtpos != '"' && *txtpos != '\0' && len < MAX_STRING_LEN - 1) {
      lastStringValue[len++] = *txtpos++;
    }
    lastStringValue[len] = '\0'; // Null-Terminator für den C-String
    if (*txtpos == '"') txtpos++;
    return TOKEN_STRING_LITERAL;
  }

  // Variablen (A-Z, A$, AB, A1)
  if (isalpha(*txtpos)) {
    isStringVar = false;
    vIdx1 = *txtpos - 'A';
    vIdx2 = 0;
    txtpos++;

    if (isalnum(*txtpos) || isdigit(*txtpos)) {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }
    while (isalnum(*txtpos)) {
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
//############################################################ getln ####################################################################

void getln(int showReady) {
  if (showReady) printReady();

  int cursor = 0;
  int length = 0;
  memset(inputBuffer, 0, BUF_SIZE);

  if (editLine(inputBuffer, BUF_SIZE, cursor, length)) {
    txtpos = inputBuffer;                                             // Wenn Enter gedrückt wurde: Text an txtpos übergeben

  } else {
    // Bei ESC: Zeile leeren
    inputBuffer[0] = '\0';
    txtpos = inputBuffer;
    println("Break!");

  }
}
//############################################################ Inchar ####################################################################

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
//############################################# Anzeige Syntaxerror mit Position #######################################################

static void syntaxerror(const char *msg)
{
  if (isError == true) return;

  char* lineStart = program[currentLineIndex].text;
  if (isRunning) {
    if (lineStart != NULL) {

      print(program[currentLineIndex].number);                                    // Ausgabe der Zeilennummer
      print(" ");
      println(lineStart);
      int offset = txtpos - lineStart;                                            // Versatz berechnen: Aktuelle Position minus Startposition im Text

      int numDigits = 0;
      int tempNum = program[currentLineIndex].number;
      while (tempNum > 0) {
        tempNum /= 10;
        numDigits++;
      }
      if (program[currentLineIndex].number == 0) numDigits = 1;

      for (int i = 0; i < (numDigits + 1 + offset); i++) {                        // Einrücken: Nummer-Stellen + 1 (für das Leerzeichen nach der Nummer)
        print(" ");
      }
      println("^");                                                               // Markierung unter dem falschen Zeichen
    }
  }
  printmsg(msg, 0);
  currentLineIndex = 0;
  line_terminator();
  isRunning = false;
  isError = true;
}
//############################################################ Meldung ausgeben ###########################################################

void printmsg(const char *msg, int nl) {

  while (*msg) {
    outchar(*msg++);
  }

  if (nl == 1) {
    line_terminator();
  }
}

//############################################################ Zeile speichern ###########################################################

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
//############################################################ wait_key ###################################################################

static uint16_t wait_key(bool modes) {
  if (modes) {
    line_terminator();
    printmsg("SPACE<Continue> / ESC <Exit>", 1);
  }
  while (1) {
    // 1. Terminal-Check
    if (lastUsbChar != -1) {
      int c = lastUsbChar;
      lastUsbChar = -1;
      return (uint16_t)c;
    }
    yield();
  }
}

//################################################################# Syntax-Hervorhebung ###################################################
void printColoredLine(char* lineText) {
  char* old_txtpos = txtpos;
  bool fornext = false;
  txtpos = lineText;

  if (strstr(lineText, "NEXT") || strstr(lineText, "WEND")) {
    if (strstr(lineText, "FOR")) {
      fornext = true;
    }
    else {
      currentIndent--;
      if (currentIndent < 0) currentIndent = 0;
    }
  }

  for (int i = 0; i < currentIndent; i++) print(" ");
  spaces();
  while (*txtpos) {
    if (*txtpos == ' ') {
      print(*txtpos++);
      continue;
    }

    char* startOfToken = txtpos;

    // 1. Befehle prüfen
    int cmdToken = getCommandToken();
    if (cmdToken != TOKEN_ERROR && cmdToken != TOKEN_VARIABLE) {
      if ((fornext == false) && (cmdToken == TOKEN_FOR || cmdToken == TOKEN_WHILE)) currentIndent++;

      if (cmdToken == TOKEN_REM) {
        fbcolor(GRAY, 0);
        while (*startOfToken) print(*startOfToken++);
        fbcolor(WHITE, 0);
        break;
      }

      fbcolor(YELLOW, 0);
      while (startOfToken < txtpos) print(*startOfToken++);
      fbcolor(WHITE, 0);
      continue;
    }

    txtpos = startOfToken;

    // 2. Funktionen / Strings / Variablen prüfen
    int token = getFunctionToken();
    if (token != TOKEN_ERROR && token != TOKEN_END) {
      if (token == TOKEN_STRING_LITERAL) {
        fbcolor(RED, 0);
        print(*startOfToken++);
        while (*startOfToken && *startOfToken != '"') {         // alles bis zum nächsten " oder Zeilenende drucken
          print(*startOfToken++);
        }
        if (*startOfToken == '"') print(*startOfToken++);       // Falls ein schließendes " da ist, auch dieses drucken
        txtpos = startOfToken;
        fbcolor(WHITE, 0);
        continue;
      }

      // Variablen, Zahlen, Funktionen
      switch (token) {
        case TOKEN_VARIABLE:
          if (isStringVar) fbcolor(MAGENTA, 0);
          else fbcolor(GREEN, 0);
          break;
        case TOKEN_NUMBER:
          fbcolor(WHITE, 0);
          break;
        default:
          fbcolor(CYAN, 0);
          break;
      }
      while (startOfToken < txtpos) print(*startOfToken++);
      fbcolor(WHITE, 0);
      continue;
    }

    txtpos = startOfToken;
    print(*txtpos++);
  }
  println();
  txtpos = old_txtpos;
}


//############################################################ LIST ######################################################################

void cmd_list() {

  currentIndent = 0;
  int tmp_color = windows[currentWinIdx].bcolor;          //aktuelle Hintergrundfarbe sichern
  fbcolor(0, 1);                                          //Hintergrund für die Listausgabe in Schwarz

  int zeilen = 0;
  if (lineCount == 0) {
    fbcolor(tmp_color, 1);                                //Farbe zurücksetzen
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
      print(' ');
      printColoredLine(program[i].text);                  //List-Zeile farbig ausgeben
      fbcolor(WHITE, 0);
      zeilen++;
      if (zeilen == MAX_R - 10) {
        if (wait_key(1) == 27) return;
        zeilen = 0;
      }
    }
  }
  fbcolor(tmp_color, 1);
}

//############################################################ GOTO/GOSUB ####################################################################

bool jumpTo(int label) {
  for (int i = 0; i < lineCount; i++) {
    if (program[i].number == label) {
      currentLineIndex = i; // Wir setzen den INDEX im Array
      return true;
    }
  }
  return false;
}
//############################################################ spaces() ######################################################################

inline char spaces() {                                                                                     // Leerzeichen überspringen
  while (*txtpos == ' ' || *txtpos == '\t') txtpos++;
  return *txtpos;
}
//############################################################ RTC ######################################################################
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

//############################################################ FN #########################################################################
double func_fn() {
  spaces();
  if (!isalpha(*txtpos)) return 0.0;

  char fName = toupper(*txtpos++);
  int idx = fName - 'A';

  double argValues[4] = {0.0, 0.0, 0.0, 0.0};
  int argCount = 0;

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

  // Prüfung ob Singular "function" oder Plural "functions"
  if (function[idx].formula == NULL) return 0.0;

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

  for (int i = 0; i < function[idx].paramCount; i++) {
    int varIdx = function[idx].params[i] - 'A';
    variables[varIdx][0] = oldParamValues[i];
  }
  txtpos = oldTxtpos;

  return result;
}

//############################################################ INKEY ############################################################################

static int inkey() {
  myusb.Task();

  if (lastUsbChar != -1) {
    int c = lastUsbChar;                          //Taste holen
    lastUsbChar = -1;                             //Tastaturpuffer löschen
    delay(10);                                    //wichtig, sonst flimmert das Bild bei Inkey
    return c;
  }
  delay(10);
  return 0;                                       // nichts gedrückt, dann zurück
}

//############################################################ MAP ##############################################################################

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  if (in_max == in_min) return out_min; // Sicherheitscheck
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//############################################################ Constrain #########################################################################

double constrainDouble(double x, double minVal, double maxVal) {
  if (x < minVal) return minVal;
  if (x > maxVal) return maxVal;
  return x;
}
//############################################################ expression() ######################################################################

//unterste ebene factor
double factor() {
  spaces();

  // Sofort-Check für einfache Variablen
  if (isalpha(*txtpos)) {
    const char* p = txtpos + 1;
    if (!isalnum(*p) && *p != '$' && *p != '(') {
      int v1 = *txtpos - 'A';
      txtpos = p;
      return variables[v1][0];
    }
  }

  // --- NEU: Logisches NOT mit '!' ---
  if (*txtpos == '!') {
    txtpos++;
    double val = factor();
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

      case TOKEN_MAP: {
          if (Test_char('(')) return 0;
          double val    = expression(); // Wert, der skaliert werden soll
          if (Test_char(',')) return 0;
          double inMin  = expression(); // Quell-Untergrenze
          if (Test_char(',')) return 0;
          double inMax  = expression(); // Quell-Obergrenze
          if (Test_char(',')) return 0;
          double outMin = expression(); // Ziel-Untergrenze
          if (Test_char(',')) return 0;
          double outMax = expression(); // Ziel-Obergrenze
          if (Test_char(')')) return 0;

          return mapDouble(val, inMin, inMax, outMin, outMax);
        }
      case TOKEN_CONSTRAIN: {
          if (Test_char('(')) return 0;
          double val = expression();     // Der zu prüfende Wert
          if (Test_char(',')) return 0;
          double minLimit = expression(); // Untergrenze
          if (Test_char(',')) return 0;
          double maxLimit = expression(); // Obergrenze
          if (Test_char(')')) return 0;
          return constrainDouble(val, minLimit, maxLimit);
        }
      case TOKEN_MAX: {
          if (Test_char('(')) return 0;
          double v1 = expression();
          if (Test_char(',')) return 0;
          double v2 = expression();
          if (Test_char(')')) return 0;
          return (v1 > v2) ? v1 : v2;
        }

      case TOKEN_MIN: {
          if (Test_char('(')) return 0;
          double v1 = expression();
          if (Test_char(',')) return 0;
          double v2 = expression();
          if (Test_char(')')) return 0;
          return (v1 < v2) ? v1 : v2;
        }

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
          spaces();
          if (*txtpos == '(') txtpos++;

          double result = 0;
          int t = getFunctionToken();

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
            txtpos++;
            String s = parseStringExpression();
            spaces();
            if (*txtpos == ')') txtpos++;
            if (s.length() > 0) {
              return (double)((uint8_t)s[0]);           // ASCII-Wert des ersten Zeichens
            }
          }
          return 0;
        }
      case TOKEN_FRE: {
          // FRE(x) 1-usedlines 2-freelines 3-totalRam 4-usedRam 5-freeBytes
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
      case TOKEN_GPX:
        spaces();
        if (*txtpos == '(') {
          txtpos++; // Öffnende Klammer (
          int x = expression();
          if (*txtpos == ',') txtpos++;
          int y = expression();
          if (*txtpos == ')') txtpos++;
          return vga.getPixel(x, y);
        }

      // Gruppe der mathematischen Funktionen
      case TOKEN_RND: case TOKEN_SQR: case TOKEN_SIN: case TOKEN_COS:
      case TOKEN_ABS: case TOKEN_INT: case TOKEN_DEG: case TOKEN_RAD:
      case TOKEN_SGN: case TOKEN_BIN : case TOKEN_HEX : case TOKEN_OCT :
      case TOKEN_GTIME : case TOKEN_EXP: case TOKEN_LOG: case TOKEN_LN:
      case TOKEN_TAN:
        spaces();
        if (*txtpos == '(') txtpos++;
        arg = expression();
        if (*txtpos == ')') txtpos++;

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
          case TOKEN_LOG: return log10(arg);
          case TOKEN_LN : return log(arg);
          case TOKEN_TAN: return tan(arg);

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
    while (isalnum(*txtpos)) {                                   //lange Variablennamen
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


double power() {                                    //untere Ebene Power
  double val = factor();
  while (true) {
    spaces();
    if (*txtpos == '^') {
      txtpos++;
      val = pow(val, power());
    } else {
      return val;
    }
  }
}



double term() {                                   // Mittlere Ebene: Multiplikation und Division
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
    } else if (*txtpos == '%') {                  // Modulo Operator
      txtpos++;
      double divisor = power();
      if (divisor != 0) {
        val = fmod(val, divisor);                 // fmod ist die Modulo-Funktion für double
      } else {
        syntaxerror(zeroerror);
      }
    } else {
      return val;
    }
  }
}



double expression() {                             //Oberste Ebene: Addition und Subtraktion
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
  spaces();
  double left = expression();

  if (*txtpos == '<' || *txtpos == '>' || *txtpos == '=') {

    char op1 = *txtpos++;
    char op2 = '\0';

    if (*txtpos == '=' || *txtpos == '>') {
      op2 = *txtpos++;
    }

    double right = expression();

    if (op1 == '=') return left == right;
    if (op1 == '<') {
      if (op2 == '>') return left != right;
      if (op2 == '=') return left <= right;
      return left < right;
    }
    if (op1 == '>') {
      if (op2 == '=') return left >= right;
      return left > right;
    }
  }
  return (left != 0);                           // KEIN Operator, Ergebnis ist der Wert von left
}

bool logical_expression() {
  bool result = logical_and();

  while (true) {
    spaces();
    char *beforeToken = txtpos;
    int token = getCommandToken();

    if (token == TOKEN_OR) {
      bool or_res = logical_and();              // Rechter Teil muss erst AND prüfen
      result = result || or_res;
    }
    else {
      txtpos = beforeToken;
      return result;
    }
  }
}


bool logical_and() {                            // Verarbeitet AND (höhere Priorität)
  bool result = relation();

  while (true) {
    spaces();
    char *beforeToken = txtpos;
    int token = getCommandToken();

    if (token == TOKEN_AND) {
      bool and_res = relation();
      result = result && and_res;
    }
    else {
      txtpos = beforeToken;
      return result;
    }
  }
}

//############################################################ VARS ######################################################################
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
      print(") ");

      int totalElements = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
      printmsg("  [", 0);
      print(totalElements);
      printmsg(" Elemente]", 1);
    }
  }
  printmsg("--------------------------------", 1);
}
//############################################################ PRINT ######################################################################

void cmd_print() {
  bool newline = true;
  int tmp_win = currentWinIdx;
  String usingFormat = "";
  bool useUsing = false;

  while (*txtpos != '\0' && *txtpos != ':' && *txtpos != '\r' && *txtpos != '\n') {
    spaces();
    printMode = 0;

    // 1. Keywords scannen (AT, TAB, USING sind in deiner commands-Tabelle)
    int t = scanKeyword(commands, numCommands);

    if (t == TOKEN_AT) {
      if (currentWinIdx > 0) {
        windows[currentWinIdx].curX = x_pos;
        windows[currentWinIdx].curY = y_pos;
        currentWinIdx = 0;
      }
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
        if (*txtpos == ';') txtpos++;
      }
      continue;
    }

    if (t == TOKEN_TAB) {
      spaces();
      if (*txtpos == '(') {
        txtpos++;
        int newX = (int)expression();
        if (*txtpos == ')') txtpos++;
        if (newX >= 0 && newX < MAX_C) x_pos = newX;
        newline = false;
        if (*txtpos == ';') txtpos++;
      }
      continue;
    }

    if (t == TOKEN_USING) {
      spaces();
      usingFormat = parseStringExpression();
      useUsing = true;
      spaces();
      if (*txtpos == ';' || *txtpos == ',') txtpos++;
      continue;
    }

    // --- 2. STRING ODER NUMERISCH ---
    // Wir schauen, ob als Nächstes ein String kommt (Literal, Variable oder Funktion)
    const char* checkpoint = txtpos;
    int ft = getFunctionToken();

    // Prüfen, ob das Token ein String-Typ ist
    bool isString = (ft == TOKEN_STRING_LITERAL) ||
                    (ft == TOKEN_VARIABLE && isStringVar) || ft == TOKEN_STRING ||
                    (ft == TOKEN_LEFT || ft == TOKEN_MID || ft == TOKEN_RIGHT || ft == TOKEN_SPC ||
                     ft == TOKEN_STR || ft == TOKEN_CHR || ft == TOKEN_TIME || ft == TOKEN_DATE);

    txtpos = checkpoint; // Zurücksetzen, damit parseStringExpression oder expression() von vorne lesen

    if (isString) {
      print(parseStringExpression());
      newline = true;
    } else {                          // --- 2. NUMERISCHE AUSDRÜCKE (inkl. HEX/BIN/OCT) ---
      double val = expression();

      if (printMode == 16) {
        print("H");
        print((double)val, HEX);
      }
      else if (printMode == 2) {
        print("B");
        print((double)val, BIN);
      }
      else if (printMode == 8) {
        print("O");
        print((double)val, OCT);
      }
      else {
        if (useUsing) {
          int dotPos = usingFormat.indexOf('.');
          int totalWidth = (dotPos == -1) ? usingFormat.length() : dotPos;
          int precision = (dotPos == -1) ? 0 : usingFormat.length() - dotPos - 1;
          char buf[32];
          String fmt = "%" + String(totalWidth + (precision > 0 ? precision + 1 : 0)) + "." + String(precision) + "f";
          snprintf(buf, sizeof(buf), fmt.c_str(), val);
          print(buf);
        } else {
          if (val == (long)val && val < 2147483647 && val > -2147483648) {
            print((long)val);
          } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.10g", val);
            print(buf);
          }
        }
      }
      newline = true;
    }

    // --- 3. TRENNER ---
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

  useUsing = false;
  if (newline) println();
  currentWinIdx = tmp_win;
}

//############################################################ CLEAR ######################################################################

void clearAll() {

  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      variables[i][j] = 0.0f;
      stringVars[i][j] = "";
    }
  }
  for (int i = 0; i < arrayCount; i++) {
    if (allArrays[i].isString) {
      if (allArrays[i].strData != nullptr) {
        delete[] allArrays[i].strData;
        allArrays[i].strData = nullptr;
      }
    } else {
      if (allArrays[i].numData != nullptr) {
        delete[] allArrays[i].numData;
        allArrays[i].numData = nullptr;
      }
    }
    // Dimensionen zurücksetzen
    allArrays[i].dimX = 0;
    allArrays[i].dimY = 0;
    allArrays[i].dimZ = 0;
  }
  arrayCount = 0;                                                                      //Array-Zähler
  gosubStackPtr = 0;                                                                   // GOSUB Stack leeren
  forStackPtr = 0;                                                                     // FOR-NEXT Verschachtelung löschen
  currentLineIndex = 0;                                                                // Zeiger auf die erste Programmzeile
  dataLineIdx = 0;
  dataPtr = NULL;
  isRunning = true;                                                                    // Ausführungs-Flag setzen
  jumped = false;                                                                      // WICHTIG: Signalisiert der Hauptschleife einen Neustart
  isError = false;
  resetWindows();

}
//############################################################ NEW ######################################################################

void cmd_new() {
  clearAll();
  memset(program, 0, sizeof(program));
  lineCount = 0;
  isRunning = false;
}

//############################################################ RUN ######################################################################
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

//############################################################ INPUT ######################################################################
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
      return;
    }
  }
}

//############################################################ READ - RESTORE ###################################################################

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

    //Datenquelle prüfen (DATA-Pointer)
    if (dataPtr == NULL || *dataPtr == '\0' || *dataPtr == ':') {
      findNextData();
    }
    if (dataLineIdx >= lineCount) {
      syntaxerror(datamsg);
      isRunning = false;
      return;
    }

    // Wert aus der DATA-Zeile lesen
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

    spaces();
    if (*txtpos == '(') {
      txtpos++; // '(' überspringen
      int x = (int)expression(); if (*txtpos == ',') txtpos++;
      int y = (int)expression();
      if (*txtpos == ',') txtpos++;
      int z = (int)expression(); if (*txtpos == ')') txtpos++;

      int i = findArray(targetV1, targetV2, isStr);
      if (i != -1) {
        int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);
        if (isStr) allArrays[i].strData[idx] = sVal;
        else allArrays[i].numData[idx] = val;
      } else {
        syntaxerror(dimmsg);
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

//############################################################ String-Funktionen ##############################################################

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
    const char* checkpoint = txtpos; // Position sichern
    int t = scanKeyword(functions, numFunctions);

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
            // SICHERHEIT: Falls der erste Parameter kein String war
            if (s == "" && *txtpos != '"' && !isalpha(*txtpos)) {
              syntaxerror(valmsg);
              return "";
            }
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
          while (isalnum(*txtpos)) {                                   //lange Variablennamen
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


  spaces();
  if (*txtpos == '+') {                             // String-Addition
    txtpos++;
    String nextPart = parseStringExpression();
    res.reserve(res.length() + nextPart.length()); // Optimiert den RAM-Zugriff
    res += nextPart;
  }

  return res;

}

//############################################################ GOSUB ######################################################################
void cmd_on() {
  int index = (int)expression();
  int jumpToken = getCommandToken();
  if (jumpToken != TOKEN_GOTO && jumpToken != TOKEN_GOSUB) {
    syntaxerror(valmsg);
    isRunning = false;
    return;
  }

  int currentIdx = 1;
  bool jumpedToTarget = false;

  while (true) {
    spaces();
    int targetLine = (int)expression();

    if (currentIdx == index) {
      if (jumpToken == TOKEN_GOSUB) {
        if (gosubStackPtr < MAX_GOSUB_STACK) {
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

      gosubStack[gosubStackPtr].lineIndex = currentLineIndex;
      gosubStack[gosubStackPtr].txtPos = txtpos;
      gosubStackPtr++;

      if (jumpTo(targetNum)) {
        txtpos = program[currentLineIndex].text;
        jumped = true; // Wichtig für den Haupt-Loop!
      } else {
        syntaxerror(gosubmsg);
      }
    } else {
      syntaxerror(gosubmsg);
    }
  } else {
    syntaxerror(gosubmsg);
  }
}
//############################################################ RETURN ######################################################################

void cmd_return() {
  if (gosubStackPtr > 0) {
    gosubStackPtr--;

    currentLineIndex = gosubStack[gosubStackPtr].lineIndex;
    txtpos = gosubStack[gosubStackPtr].txtPos;
    spaces();

    if (*txtpos == ':') {
      txtpos++;
      jumped = true;
    } else if (*txtpos == '\0') {
      jumped = false;
    } else {
      jumped = true;
    }
  } else {
    syntaxerror(gosubmsg);
  }
}


//############################################################ DIR/FILES ######################################################################
void cmd_files() {
  int zeilen = 0;
  spaces();
  bool tmp_cur = cursor_on_off;
  cursor_on_off = false;

  String filter = "";
  if (*txtpos == '"' || isalpha(*txtpos)) {
    filter = parseStringExpression();
    filter.toUpperCase();
  }

  File root = SD.open("/");
  if (!root) {
    syntaxerror(sderrormsg);
    return;
  }
  int fileCount = 0;
  uint64_t totalFilesSize = 0;

  println("Files on SD-Card:");
  if (filter != "") {
    print(" (Filter: *"); print(filter); println("*)");
  }
  println("----------------------------------------");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String fileName = String(entry.name());
    String fileNameUpper = fileName;
    fileNameUpper.toUpperCase();

    if (filter == "" || fileNameUpper.indexOf(filter) != -1) {
      fileCount++;
      totalFilesSize += entry.size();

      // 1. DATEINAME (Spalte 1, Breite z.B. 15 Zeichen)
      print(" ");
      if (fileNameUpper.indexOf(".BAS") != -1) fbcolor(CYAN, 0);
      else if (fileNameUpper.indexOf(".BIN") != -1) fbcolor(GREEN, 0);
      else if (fileNameUpper.indexOf(".BMP") != -1 || fileNameUpper.indexOf(".JPG") != -1) fbcolor(RED, 0);
      else if (entry.isDirectory()) fbcolor(YELLOW, 0);

      print(fileName);
      if (entry.isDirectory()) print("/");

      // Auffüllen bis Spalte 16
      int pad = 20 - fileName.length() - (entry.isDirectory() ? 1 : 0);
      for (int i = 0; i < pad; i++) print(" ");

      fbcolor(WHITE, 0);

      // 2. GRÖSSE (Spalte 2, Breite 10 Zeichen)
      if (entry.isDirectory()) {
        fbcolor(YELLOW, 0);
        print("<DIR>     ");
        fbcolor(WHITE, 0);
      } else {
        String sSize = String((unsigned long)entry.size());
        print(sSize);
        for (int i = 0; i < (10 - sSize.length()); i++) print(" ");
      }

      // 3. DATUM (Spalte 3)
      DateTimeFields tm;
      if (entry.getModifyTime(tm)) {
        if (tm.mday < 10) print("0"); print(tm.mday); print(".");
        if (tm.mon + 1 < 10) print("0"); print(tm.mon + 1); print(".");
        print(tm.year + 1900);
      }
      println();

      zeilen++;

      int limit = (currentWinIdx == 0) ? MAX_R - 10 : windows[currentWinIdx].h - 4;       // Paging angepasst an Fensterhöhe
      if (zeilen >= limit) {
        print("-- Press Key --");
        if (wait_key(1) == 27) {
          entry.close();
          break;
        }
        zeilen = 0;
      }
    }
    entry.close();
  }
  root.close();

  println("----------------------------------------");
  print(fileCount); println(" Files found.");
  println();
  // Statistik
  print("Used : "); printSmartSize(SD.usedSize());
  print("Free : "); printSmartSize(SD.totalSize() - SD.usedSize());
  print("Total: "); printSmartSize(SD.totalSize());
  cursor_on_off = tmp_cur;
  drawCursor(cursor_on_off);
}

// Hilfsfunktion zur schöneren Größenanzeige (B, KB, MB, GB)
void printSmartSize(uint64_t bytes) {
  if (bytes < 1024) {
    print((long)bytes); println(" B");
  } else if (bytes < 1048576) {
    print((long)(bytes / 1024)); println(" KB");
  } else if (bytes < 1073741824.0) {
    print((long)(bytes / 1048576.0)); println(" MB");
  } else {
    print((double) bytes / 1073741824.0, 2); println(" GB");
  }
}

//############################################################ RENAME ######################################################################

void cmd_rename() {
  spaces();

  // 1. Alten Dateinamen lesen
  String oldName = parseStringExpression();

  spaces();
  if (*txtpos != ',') {
    syntaxerror(syntaxmsg);
    return;
  }
  txtpos++; // Überspringe das Komma

  // 2. Neuen Dateinamen lesen
  String newName = parseStringExpression();

  if (oldName.length() == 0 || newName.length() == 0) {
    syntaxerror(syntaxmsg);
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
//############################################################ LOAD ######################################################################

void cmd_load() {
  spaces();
  String fileNameStr = parseStringExpression();
  if (fileNameStr.length() == 0) {
    syntaxerror(syntaxmsg);
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
      trimLeadingSpaces(p);
      storeLine(lineNumber, p);
    }
  }

  file.close();
  // Wichtig: Nach dem Laden sicherstellen, dass isRunning false ist
  isRunning = false;
}

//############################################################ SAVE ######################################################################
void cmd_save() {
  spaces();
  String fileNameStr = parseStringExpression();

  if (fileNameStr.length() == 0) {
    println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // --- Überschreiben Abfrage mit waitkey ---
  if (SD.exists(fileName)) {
    print("File exists. Overwrite? (y/n): ");

    char choice = tolower(wait_key(0));
    println(String(choice));

    if (choice != 'y') {
      println("Save aborted.");
      return;
    }

    SD.remove(fileName);                                      // Wenn 'y', löschen wir die alte Datei vor dem Neuschreiben
  }

  // Ab hier wie gehabt: Datei öffnen und speichern
  File file = SD.open(fileName, FILE_WRITE);
  if (!file) {
    print("ERROR: Could not create ");
    println(fileName);
    return;
  }

  print("Saving to ");
  println(fileName);

  for (int j = 0; j < lineCount; j++) {
    file.print(program[j].number);
    file.print(" ");
    file.println(program[j].text);
  }
  file.close();
}

//############################################################ DEL  ########################################################################

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
//############################################################ COPY #######################################################################

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
//############################################################ Arrays ######################################################################

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

//############################################################ DIM - Befehl ######################################################################

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

    while (isalnum(*txtpos)) {                                   //lange Variablennamen
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
      isRunning = false;
      return;
    }

    // 3. Array im Speicher anlegen
    int sizeX = d1 + 1;
    int sizeY = (d2 == 0) ? 1 : d2 + 1;
    int sizeZ = (d3 == 0) ? 1 : d3 + 1;

    long long totalSize = (long long)sizeX * sizeY * sizeZ;
    const long long MAX_ELEMENTS = 10000;

    if (totalSize > MAX_ELEMENTS) {
      syntaxerror(outofmemory);
      isRunning = false; return;
    }
    if (arrayCount < MAX_ARRAYS) {
      allArrays[arrayCount].vIdx1 = id1;
      allArrays[arrayCount].vIdx2 = id2;
      allArrays[arrayCount].isString = sVar;
      allArrays[arrayCount].dimX = sizeX;
      allArrays[arrayCount].dimY = sizeY;
      allArrays[arrayCount].dimZ = sizeZ;

      if (sVar) {
        allArrays[arrayCount].strData = new (std::nothrow) String[totalSize];
        if (allArrays[arrayCount].strData == nullptr) {
          syntaxerror(outofmemory); isRunning = false; return;
        }
      } else {
        allArrays[arrayCount].numData = new (std::nothrow) double[totalSize];
        if (allArrays[arrayCount].numData == nullptr) {
          syntaxerror(outofmemory); isRunning = false; return;
        }
        for (int i = 0; i < totalSize; i++) allArrays[arrayCount].numData[i] = 0.0;
      }
      arrayCount++;
    }

    spaces();
    sVar = false;
    // --- DER SCHLÜSSEL: Wenn ein Komma folgt, loope weiter ---
  } while (*txtpos == ',' && (txtpos++));
}


void cmd_assignment() {

  int targetV1 = vIdx1;
  int targetV2 = vIdx2;
  bool sVar = isStringVar;

  //print(*txtpos);
  // Schneller Check: Ist es eine einfache Zuweisung?
  if (*txtpos == '=') {
    txtpos++;
    if (sVar) {
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
//############################################################ ELSE ############################################################################

void skip_to_else() {
  while (*txtpos != '\0' && *txtpos != ':') {
    const char* savedPos = txtpos;
    int t = scanKeyword(commands, 61);

    if (t == TOKEN_ELSE) return;                                // ELSE gefunden dann zurück
    txtpos = savedPos + 1;                                      // Falls es kein ELSE war, nur ein Zeichen weitergehen und weitersuchen
  }
  *txtpos = '\0';                                               // Kein ELSE in dieser Zeile gefunden -> Zeile beenden
}

//############################################################ While/Wend ######################################################################
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
    int t = scanKeyword(commands, numCommands);

    if (t == TOKEN_WHILE) {
      nesting++;                                // Innere Schleife gefunden
    }
    else if (t == TOKEN_WEND) {
      nesting--;                                // Ende einer Schleife gefunden
      if (nesting == 0) {                       // Das passende WEND zur ursprünglichen WHILE-Schleife gefunden

        spaces();
        return;
      }
    }

    if (pBefore == txtpos) {                    // Wenn kein Keyword erkannt wurde oder wir noch tiefer verschachtelt sind
      txtpos++;
    }
  }
  isRunning = false;                            // Fehlerfall: kein WEND gefunden
  println("Error: WHILE without WEND");
}


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
    int t = scanKeyword(commands, numCommands);

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

    if (pBefore == txtpos) txtpos++;
  }
}

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
//############################################################ DWRITE-Befehl ######################################################################
void cmd_dwrite() {
  spaces();
  int pin = (int)expression(); // Pin Nummer (z.B. 13)

  spaces();
  if (*txtpos == ',') {
    txtpos++;
    int state = (int)expression(); // Status (0 = LOW, 1 = HIGH)
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);

  } else {
    syntaxerror(syntaxmsg);
  }
}

//############################################################ Pause-Befehl ######################################################################
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

//############################################################  MEM-Befehl ######################################################################
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
//########################################################## MEM ##################################################################################

void cmd_mem() {
  char buffer[30];
  printmsg("--- BASIC PROGRAM MEMORY ---", 1);
  itoa(get_mem(1), buffer, 10);
  printmsg("Lines used    : ", 0); printmsg(buffer, 1);
  ltoa(get_mem(2), buffer, 10);
  printmsg("Free          : ", 0); printmsg(buffer, 1);
  ltoa(get_mem(3), buffer, 10);
  printmsg("Basic-RAM     : ", 0); printmsg(buffer, 1);
  ltoa(get_mem(4), buffer, 10);
  printmsg("Basic-RAM     : ", 0); printmsg(buffer, 0);
  printmsg(" Bytes used.", 1);

  // Dynamischer Speicher (für deine 3D-Arrays)
  printmsg("Free Basic-RAM: ", 0);
  dtostrf(get_mem(5), 6, 0,  buffer);
  //dtostrf(external_psram_size *1024 *1024,12,0,buffer);
  printmsg(buffer, 0); // Die Funktion von oben
  printmsg(" Bytes", 1);
}
//########################################################## Test_char ###########################################################################

static int Test_char(char az)
{
  if (*txtpos != az)
  {
    return 1;
  }
  txtpos++;
  spaces();
  return 0;
}
//########################################################## STIME ###############################################################################

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

//########################################################## Dump #################################################################################


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

    if (zeilen == MAX_R - 10) { // 2. Check gegen 20
      if (wait_key(1) == 27) break; // Falls waitkey true (z.B. ESC), beenden
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
//########################################################## DEFN - Befehl ######################################################################################

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
    function[idx].formula = txtpos;                             // Hier speichern wir den Zeiger auf den Beginn der Formel
  }

  txtpos = (char*)"";
}

//########################################################## PSET - Befehl ########################################################################################


void cmd_pset() {
  spaces();

  int px = (int)expression(); // X-Koordinate parsen

  if (*txtpos == ',') txtpos++;
  else {
    syntaxerror(nullptr);
    return;
  }

  int py = (int)expression(); // Y-Koordinate parsen

  uint8_t col = windows[currentWinIdx].fcolor; // Vordergrundfarbe, falls keine Farbe angegeben wird
  if (*txtpos == ',') {
    txtpos++;
    col = (uint8_t)expression(); // Farbe parsen (0-255)
  }
  vga.drawPixel(px, py, col);
}

//########################################################## TRON - Funktion ########################################################################################
void cmd_tron(unsigned long wait, int index) {
  print("<");
  print(program[index].number);
  print(">");
  delay(wait);
}
//########################################################## PIC - Befehl ############################################################################################

int getFileTypeID(const char* filename) {
  String name = String(filename);
  name.toLowerCase();

  if (name.endsWith(".jpg") || name.endsWith(".jpeg")) return 1;
  if (name.endsWith(".bmp")) return 2;

  return 0; // Unbekannt
}

int JPEGDraw(JPEGDRAW * pDraw) {
  uint16_t *pSrc = pDraw->pPixels;
  int xStart = pDraw->x;
  int yStart = pDraw->y;

  for (int y = 0; y < pDraw->iHeight; y++) {
    int currentY = yStart + y;
    for (int x = 0; x < pDraw->iWidth; x++) {
      uint16_t p = *pSrc++;

      uint8_t color8 = ((p >> 13) << 5) | (((p >> 8) & 0x07) << 2) | ((p >> 3) & 0x03);     //565 -> 332 Konvertierung
      if ((xStart + x < fb_width) && (currentY < fb_height)) {
        vga.drawPixel(xStart + x, currentY, color8);
      }
    }
  }
  return 1;
}

void * myOpen(const char *filename, int32_t *size) {
  myFile = SD.open(filename);
  if (myFile) {
    *size = myFile.size();
    return &myFile;
  }
  syntaxerror(notexistmsg);
  return NULL;
}

void myClose(void *handle) {
  if (handle) myFile.close();
}

int32_t myRead(JPEGFILE * handle, uint8_t *buffer, int32_t length) {
  return myFile.read(buffer, length);
}

int32_t mySeek(JPEGFILE * handle, int32_t position) {
  return myFile.seek(position);
}

void drawJpeg(const char* filename, int x, int y) {

  if (jpeg.open( filename, myOpen, myClose, myRead, mySeek, JPEGDraw)) {
    jpeg.decode(x, y, 0);  //scale= 0,1,2,4,8
    jpeg.close();
  }


}

void drawBMP(const char* filename, int x, int y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    syntaxerror(notexistmsg);
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

//################################ Unterfunktion zu Einsammeln von Parametern ###########################################################################################

Params getParams(int n) {
  Params p;
  for (int i = 0; i < n; i++) {
    p.val[i] = (int)expression();

    // Wenn wir noch nicht beim letzten Element sind...
    if (i < n - 1) {
      if (*txtpos == ',') {
        txtpos++;
      } else {
        // Nur hier ist es ein wirklicher Syntaxfehler!
        syntaxerror(syntaxmsg);
        break;
      }
    }
  }
  return p;
}
//########################################################## WINDOWS - Befehl ###########################################################################################

void resetWindows() {
  currentWinIdx = 0; // Zurück zum Hauptbildschirm
  for (int i = 1; i <= 6; i++) {
    // Fenster 1-6: "Löschen" (Breite/Höhe auf 0 setzen)
    windows[i].x = 0;
    windows[i].y = 0;
    windows[i].w = 0;
    windows[i].h = 0;
    windows[i].fcolor = WHITE;
    windows[i].bcolor = BLUE;
    windows[i].tcolor = 0;
    windows[i].title = "";
    windows[i].curX = 0;
    windows[i].curY = 0;
  }
}


void drawWindow_BordersOnly(int id) {

  // Nur die grafischen Linien und den Titelbalken zeichnen
  vga.drawline((windows[id].x * 8) - 1, windows[id].y * 8, windows[id].w * 8, windows[id].y * 8, windows[id].tcolor);
  vga.drawline((windows[id].x * 8) - 1, windows[id].y * 8, (windows[id].x * 8) - 1, windows[id].h * 8, windows[id].tcolor);
  vga.drawline((windows[id].x * 8) - 1, windows[id].h * 8, windows[id].w * 8, windows[id].h * 8, windows[id].tcolor);
  vga.drawline(windows[id].w * 8, windows[id].y * 8, windows[id].w * 8, windows[id].h * 8, windows[id].tcolor);
  vga.drawRect(windows[id].x * 8 , windows[id].y * 8, (windows[id].w * 8) - (windows[id].x * 8), 8 , windows[id].tcolor);

  if (strlen(windows[id].title) > 0) {
    vga.drawText((windows[id].x * 8) + 8 + 8 , windows[id].y * 8 , windows[id].title, windows[id].fcolor, windows[id].tcolor, false);
  }
  vga.drawRect(windows[id].x * 8 , windows[id].y * 8, 8, 8 , windows[id].tcolor);
}


// Hilfsfunktion, die Puffer UND VGA gleichzeitig bedient
void writeToBuffer(int x, int y, unsigned char c, uint8_t fg, uint8_t bg) {
  if (x >= 0 && x < MAX_C && y >= 0 && y < MAX_R) {
    screenBuffer[y][x] = c;
    colorBuffer_F[y][x] = fg;
    colorBuffer_H[y][x] = bg;

    // Sofort auf VGA ausgeben
    char buf[2] = {(char)c, 0};
    vga.drawText(x * 8, y * 8, buf, fg, bg, false);
  }
}

void drawWindowTitle(int winIdx) {
  if (windows[winIdx].title == NULL || strlen(windows[winIdx].title) == 0) return;

  int x1 = windows[winIdx].x;
  int y1 = windows[winIdx].y;
  int width = windows[winIdx].w - x1;

  String t = " " + String(windows[winIdx].title) + " ";
  int tLen = t.length();

  // Titel zentrieren (innerhalb der Fensterbreite)
  int startX = x1 + (width - tLen) / 2;
  if (startX <= x1) startX = x1 + 1;

  for (int i = 0; i < tLen; i++) {
    int targetX = startX + i;
    if (targetX < windows[winIdx].w - 1) {
      writeToBuffer(targetX, y1, t[i], windows[winIdx].tcolor, windows[winIdx].fcolor);
    }
  }
}

void cmd_win() {
  //bool switch_win = false;
  spaces();
  drawCursor(false);

  // 1. Hauptfenster-Reset (WINDOW ohne alles)
  if (*txtpos == ':' || *txtpos == '\0') {
    windows[currentWinIdx].curX = x_pos;
    windows[currentWinIdx].curY = y_pos;
    currentWinIdx = 0;
    x_pos = windows[0].curX;
    y_pos = windows[0].curY;
    return;
  }
  if (*txtpos == '(') txtpos++;
  // 2. Position im ALTEN Fenster sichern
  windows[currentWinIdx].curX = x_pos;
  windows[currentWinIdx].curY = y_pos;

  // 3. Fenster-Nummer holen
  int nr = expression();
  if ((nr > 6) || (nr < 1)) syntaxerror(valmsg) ;                     //das Hauptfenster darf nicht umdefiniert werden und es sind maximal 6 Fenster definierbar
  bool isRedefinition = false;                                        //wenn jetzt eine schließende Klammer kommt, dann nur den Rahmen des Fensters neuzeichnen (Inhalt des Fensters bleibt erhalten)
  currentWinIdx = nr;

  if (*txtpos == ',') {
    isRedefinition = true;                                            //Fenster wird auf jeden Fall neu gezeichnet (incl.Inhalt)
    txtpos++;
    Params lp = getParams(4);

    windows[currentWinIdx].x = constrain(lp.val[0], 0, MAX_C - 1);
    windows[currentWinIdx].y = constrain(lp.val[1], 0, MAX_R - 1);
    windows[currentWinIdx].w = constrain(lp.val[2], windows[currentWinIdx].x + 1, MAX_C);
    windows[currentWinIdx].h = constrain(lp.val[3], windows[currentWinIdx].y + 1, MAX_R);
    windows[currentWinIdx].curX = constrain(lp.val[0] + 1, 0, MAX_C - 1);
    windows[currentWinIdx].curY = constrain(lp.val[1] + 1, 0, MAX_R - 1);

    windows[currentWinIdx].fcolor = windows[0].fcolor;    // Standardfarben setzen, falls keine farben angegeben werden
    windows[currentWinIdx].bcolor = windows[0].bcolor;
    windows[currentWinIdx].tcolor = 0;
    windows[currentWinIdx].title  = "";

  }

  if (*txtpos == ',') {
    txtpos++;
    windows[currentWinIdx].tcolor = expression();
    if (*txtpos == ',') txtpos++;
    windows[currentWinIdx].fcolor = expression();
    if (*txtpos == ',') txtpos++;
    windows[currentWinIdx].bcolor = expression();
    if (*txtpos == ',') txtpos++;
    static String winTitles[30];
    winTitles[currentWinIdx] = parseStringExpression();
    windows[currentWinIdx].title = winTitles[currentWinIdx].c_str();

    windows[currentWinIdx].curX = windows[currentWinIdx].x;                                   // NUR BEI NEUDEFINITION: Cursor zurücksetzen
    windows[currentWinIdx].curY = windows[currentWinIdx].y;

  }

  if (*txtpos == ')') txtpos++;

  x_pos = windows[currentWinIdx].curX;                                                        // Globalen Cursor laden (entweder den alten oder den soeben zurückgesetzten)
  y_pos = windows[currentWinIdx].curY;

  // Fenster neu erstellt, dann gesamtes Fenster neu zeichnen
  if (isRedefinition) {
    for (int y = windows[currentWinIdx].y; y < windows[currentWinIdx].h; y++) {
      for (int x = windows[currentWinIdx].x; x < windows[currentWinIdx].w; x++) {
        writeToBuffer(x, y, ' ', windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor);
      }
    }
    vga.drawRect(windows[currentWinIdx].x * 8, (windows[currentWinIdx].y * 8 ) , (windows[currentWinIdx].w * 8) - (windows[currentWinIdx].x * 8), ((windows[currentWinIdx].h * 8) - (windows[currentWinIdx].y * 8 )), windows[currentWinIdx].bcolor);
  }
  drawWindow_BordersOnly(currentWinIdx);                                                      // Fenster gewechselt, dann nur Rahmen neuzeichnen
  drawCursor(cursor_on_off);
}

//########################################################## RENUM - Befehl ###############################################################################################

void updateJumpText(int lineIdx, char* pos, int newNum) {
  char buf[10];
  sprintf(buf, "%d", newNum);
  int oldLen = 0;

  while (isdigit(pos[oldLen])) oldLen++;
  int newLen = strlen(buf);
  int diff = newLen - oldLen;

  if (strlen(program[lineIdx].text) + diff >= LINE_LEN - 1) return;                     // Prüfen, ob die Zeile noch in den Puffer passt
  memmove(pos + newLen, pos + oldLen, strlen(pos + oldLen) + 1);                        // Restlichen String verschieben (inkl. Nullterminator)
  memcpy(pos, buf, newLen);                                                             // Neue Nummer einfügen
}

void cmd_renum() {
  spaces();
  int nextLine = 10;                                                                    // Standard-Startwert
  int step = 10;                                                                        // Standard-Schrittweite
  //print(txtpos);
  if (*txtpos != NL && *txtpos != ':' && *txtpos != '\0') {
    nextLine = (int)expression();                                                       // (Startwert)

    if (!Test_char(',')) {
      step = (int)expression();                                                         // (Schrittweite)
    }
  }

  if (step <= 0) step = 10;
  RenumMap* mapping = new RenumMap[lineCount];

  int currentNew = nextLine;
  for (int i = 0; i < lineCount; i++) {
    mapping[i].oldNum = program[i].number;
    mapping[i].newNum = currentNew;
    program[i].number = currentNew; // Nummer im EXTMEM direkt ändern
    currentNew += step;
  }

  for (int i = 0; i < lineCount; i++) {
    processLineJumps(i, mapping, lineCount);                                             // Sprungziele korrigieren (processLineJumps)
  }
  delete[] mapping; // Speicher freigeben
}

void processLineJumps(int lineIdx, RenumMap * mapping, int mapSize) {
  char* text = program[lineIdx].text;
  const char* kGOTO = "GOTO";
  const char* kGOSUB = "GOSUB";
  const char* kON = "ON";
  //int jumpType;

  char* p = text;
  while (*p != '\0') {
    if (strncmp(p, "REM", 3) == 0) break;                                               // bei REM Zeile komplett überspringen
    char* targetPos = NULL;
    //jumpType = 0;

    if (strncmp(p, kGOTO, 4) == 0) {
      targetPos = p + 4;  // 1: GOTO/GOSUB
      //jumpType = 1;
    }
    else if (strncmp(p, kGOSUB, 5) == 0) {
      targetPos = p + 5;
      //jumpType = 1;
    }
    else if (strncmp(p, kON, 2) == 0) {                                                 // 2: ON ,GOTO/GOSUB dahinter suchen
      char* s = p + 2;
      while (*s != '\0' && strncmp(s, kGOTO, 4) != 0 && strncmp(s, kGOSUB, 5) != 0) s++;
      if (*s != '\0') {
        targetPos = (strncmp(s, kGOTO, 4) == 0) ? s + 4 : s + 5;
        //jumpType = 2;
      }
    }

    if (targetPos) {
      char* s = targetPos;
      while (*s != '\0' && *s != ':' && *s != '\r' && *s != '\n') {
        if (isdigit(*s)) {
          int oldTarget = atoi(s);
          int foundNew = -1;

          for (int m = 0; m < mapSize; m++) {
            if (mapping[m].oldNum == oldTarget) {
              foundNew = mapping[m].newNum;
              break;
            }
          }

          if (foundNew != -1) {
            updateJumpText(lineIdx, s, foundNew);                                       // Zahl ersetzen

            char buf[16];
            sprintf(buf, "%d", foundNew);                                               // WICHTIG: s hinter die NEUE Zahl schieben
            s += strlen(buf);
          } else {

            while (isdigit(*s)) s++;                                                    // Nicht gefunden: überspringe die Ziffern
          }
        } else {
          s++;
        }


        char* check = s;
        while (*check == ' ') check++;                                                  // Wenn wir am Ende einer Liste angekommen sind (kein Komma folgt),aufhören, damit wir nicht in den nächsten Befehl laufen
        if (*check != ',' && !isdigit(*check)) break;
      }
      p = s;                                                                            // Hauptpointer hinter den verarbeiteten Sprung setzen
    } else {
      p++;
    }
  }
}


//#################################################################################### Hauptprogrammschleife ###########################################################################################
void Basic_interpreter() {
  int lineNumber;
  unsigned long tron_delay;


  while (1) {
    myusb.Task();

    if (!isRunning) {                                      // --- 1. Direktmodus ---
      isError = false;
      getln(1);
      spaces();
      if (*txtpos == '\0') continue;

      if (isdigit(*txtpos)) {                             //Zeilennummer, dann speichern
        lineNumber = atoi(txtpos);
        while (isdigit(*txtpos)) txtpos++;
        storeLine(lineNumber, txtpos);
        continue;
      }

    } else {
      if (currentLineIndex >= lineCount) {                //letzte Zeile erreicht
        isRunning = false;
        continue;
      }
      if (!jumped) {
        txtpos = program[currentLineIndex].text;
      }
    }



    while (!isError) { //*txtpos != 0)  {                     // --- 2. ZEILEN-SCHLEIFE ---

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
      //print(*txtpos);
      int token = getCommandToken();
      //print(token);
      if (token == TOKEN_END) break;

      switch (token) {
        case TOKEN_PRINT: cmd_print(); break;
        case TOKEN_LIST:  cmd_list();    break;
        case TOKEN_RUN:   cmd_run();   break;
        case TOKEN_NEW:   cmd_new();   break;
        case TOKEN_VARS:  doVars();    break;
        case TOKEN_STOP:  isRunning = false; break;
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
        case TOKEN_WINDOW: cmd_win(); break;
        case TOKEN_RENUM: cmd_renum(); break;
        case TOKEN_DATA: skip_line();  break;
        case TOKEN_READ: cmd_read(); break;
        case TOKEN_RESTORE: cmd_restore(); break;
        case TOKEN_WHILE: cmd_while(); break;
        case TOKEN_WEND:  cmd_wend();  break;
        case TOKEN_VARIABLE: cmd_assignment(); break;
        case TOKEN_INPUT: cmd_input(); break;

        case TOKEN_PEN: {
            int c = (int)expression();
            fbcolor(c, 0);
            break;
          }


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


        case TOKEN_GOTO: {
            int target = (int)expression();
            if (jumpTo(target)) {
              txtpos = program[currentLineIndex].text;
              jumped = true;
            } else {
              syntaxerror(wronglinenr);
              println(target);
              print("in Line:");
              println(program[currentLineIndex].number);
              isRunning = false;
              txtpos = '\0';
            }
            break;
          }

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
            getFunctionToken();
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


        case TOKEN_PIC: {
            spaces();
            Params lp = getParams(2);
            if (*txtpos == ',') txtpos++;
            String fileName = parseStringExpression();
            int t = getFileTypeID(fileName.c_str());                                    //gucken, ob jpg,jpeg oder bmp
            if (t == 1) drawJpeg(fileName.c_str(), lp.val[0], lp.val[1]);
            else if (t == 2) drawBMP(fileName.c_str(), lp.val[0], lp.val[1]);
            else syntaxerror(extension_error);
            break;
          }

        case TOKEN_LINE: {
            spaces();
            Params lp = getParams(4);
            int c;
            if (*txtpos == ',') {
              txtpos++;
              c = expression();
            }
            else {
              c = windows[currentWinIdx].fcolor;
            }

            vga.drawline(lp.val[0], lp.val[1], lp.val[2], lp.val[3], c);
          }
          break;

        case TOKEN_RECT: {
            spaces();
            Params lp = getParams(5);
            if (lp.val[4] > 0) vga.drawRect(lp.val[0], lp.val[1], lp.val[2], lp.val[3], windows[currentWinIdx].fcolor);// RECT(x,y,w,h,color,fill)
            else {
              vga.drawline(lp.val[0], lp.val[1], lp.val[2], lp.val[1], windows[currentWinIdx].fcolor); //quer
              vga.drawline(lp.val[0], lp.val[1], lp.val[0], lp.val[3], windows[currentWinIdx].fcolor);//links runter
              vga.drawline(lp.val[2], lp.val[1], lp.val[2], lp.val[3], windows[currentWinIdx].fcolor);//rechts runter
              vga.drawline(lp.val[0], lp.val[3], lp.val[2], lp.val[3], windows[currentWinIdx].fcolor);//unten quer
            }

          }
          break;

        case TOKEN_CIRC: {
            spaces();
            Params lp = getParams(5);
            if (lp.val[4] > 0) vga.drawfilledellipse(lp.val[0], lp.val[1], lp.val[2], lp.val[3] , windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor); //CIRC x,y,w,h,fill
            else
              vga.drawellipse(lp.val[0], lp.val[1], lp.val[2], lp.val[3], windows[currentWinIdx].fcolor);
          }
          break;
        case TOKEN_CUR: {
            spaces();
            int c = (int) expression();
            if (c > 0) cursor_on_off = true;
            else cursor_on_off = false;
            break;
          }
        case TOKEN_ERROR:
          if (strlen(txtpos) > 0) {
            syntaxerror(syntaxmsg);
            isRunning = false;
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
      //delay(0);
      yield();
      vga.waitSync();
      break;

    }  //while(*txtpos != '\0')

    yield();                                                                                 //Watchdog-erholung
  }// while(1)
} //Basic-Interpreter

void check_error() {
  if (currentLineIndex > 0)
  {
    print(program[currentLineIndex].number);
    print(" ");
    print(program[currentLineIndex].text);
    println("");
    char buf[10];
    int numLen = sprintf(buf, "%d", program[currentLineIndex].number);
    int errorOffset = (txtpos - program[currentLineIndex].text);
    for (int i = 0; i < (numLen + 1 + errorOffset); i++) {                // 3. Die entsprechende Anzahl Leerzeichen drucken
      print(" ");
    }
    println("^");                                                         //markiert die Fehlerstelle

  }
}
//########################################################## RTC - auslesen ####################################################################################
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void trimLeadingSpaces(char* text) {
  if (text == nullptr || text[0] == '\0') return;

  int start = 0;
  // Zähle die führenden Leerzeichen (oder andere Whitespaces)
  while (text[start] == ' ' || text[start] == '\t') {
    start++;
  }

  // Wenn Leerzeichen gefunden wurden, verschiebe den Rest nach vorne
  if (start > 0) {
    int i = 0;
    while (text[start] != '\0') {
      text[i++] = text[start++];
    }
    text[i] = '\0'; // Den String terminieren
  }
}

//########################################################## Zeileneditor ######################################################################################

bool editLine(char* buffer, int bufferSize, int& cursor, int& length) {
  char c;
  bool inEdit = true;
  bool q = false;
  bool gfxMode = false;

  // Fenstergrenzen für die Cursor-Logik bestimmen
  int x_min = (currentWinIdx == 0) ? 0 : windows[currentWinIdx].x + 1;
  int x_max = (currentWinIdx == 0) ? MAX_C : windows[currentWinIdx].w - 1;

  while (inEdit) {
    c = inchar();
    switch (c) {
      case 13: // ENTER
      case 10:
        buffer[length] = '\0';
        outchar(13);
        for (int i = 0; i < length; i++) {
          if (buffer[i] == '"') q = !q;
          if (!q && buffer[i] >= 'a' && buffer[i] <= 'z') {   // Alles außerhalb von Anführungszeichen groß machen
            buffer[i] -= 32;
          }
        }
        return true;
        break;

      case 27: // ESC
        return false;
      case 205:
        gfxMode = !gfxMode;
        break;

      case 212: // ENTF / DELETE
        if (cursor < length) {
          // 1. Puffer-Logik: Alles rechts vom Cursor eins nach links schieben
          for (int i = cursor; i < length - 1; i++) {
            buffer[i] = buffer[i + 1];
          }
          length--;
          buffer[length + 1] = '\0'; // Puffer sauber terminieren

          // 2. Aktuelle Position merken
          int tempX = x_pos;
          int tempY = y_pos;

          // 3. Den restlichen Text ab der aktuellen Position neu schreiben
          for (int i = cursor; i < length; i++) {
            outchar(buffer[i]);
          }
          vga.drawText(x_pos * 8, y_pos * 8, " ", windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor, false);
          vga.drawText((x_pos + 8) * 8, y_pos * 8, " ", windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor, false);
          // 5. Cursor wieder an die gemerkte Stelle zurücksetzen
          x_pos = tempX;
          y_pos = tempY;
          drawCursor(cursor_on_off);
        }
        break;
      case 216:  // LINKS
        if (cursor > 0) {
          drawCursor(false);
          cursor--;
          if (x_pos <= x_min) {      // Linker Fensterrand erreicht?
            x_pos = x_max - 1;       // An das Ende der Zeile darüber
            y_pos--;
          } else {
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

      case 8:   // Backspace (ASCII 8)
      case 127: // Backspace (oft bei USB-Tastaturen)

        if (cursor > 0) {
          drawCursor(false);

          // Alles rechts vom Cursor um eins nach links schieben
          memmove(&buffer[cursor - 1], &buffer[cursor], length - cursor + 1);

          length--;
          cursor--;

          // 2. Cursor-Position grafisch zurücksetzen
          if (x_pos <= x_min) {
            x_pos = x_max - 1;
            y_pos--;
          } else {
            x_pos--;
          }

          int tempX = x_pos;
          int tempY = y_pos;

          // Schreibe ab der neuen Cursor-Position den verschobenen Rest-String
          for (int i = cursor; i < length; i++) {
            outchar(buffer[i]);
          }


          outchar(' ');                                                                                                  // Das Zeichen unter dem Cursor löschen
          vga.drawText(x_pos * 8, y_pos * 8, "  ", windows[currentWinIdx].fcolor, windows[currentWinIdx].bcolor, false); // Das ehemals letzte Zeichen am Zeilenende im Fenster löschen

          // 5. Cursor wieder dorthin setzen, wo gelöscht wurde
          x_pos = tempX;
          y_pos = tempY;
          drawCursor(cursor_on_off);
        }
        break;

      default:
        if (c >= 32 && length < bufferSize - 1) {
          // Platz im Puffer schaffen
          for (int i = length; i > cursor; i--) buffer[i] = buffer[i - 1];
          buffer[cursor] = c;
          length++;

          int startX = x_pos;
          int startY = y_pos;

          // Den gesamten Rest ab Cursor drucken (outchar regelt Fenster-Umbruch)
          for (int i = cursor; i < length; i++) {
            if (gfxMode && buffer[i] >= 'a' && buffer[i] <= 'z') {
              buffer[i] += 31;//buffer[i] - 'a' + 1; // Mappt 'a' auf Font-Zeichen 1, 'b' auf 2, etc.
            }

            outchar(buffer[i]);
          }

          // Cursor-Position für das nächste Zeichen berechnen
          cursor++;
          startX++;
          if (startX >= x_max) {     // Umbruch am rechten Fensterrand
            startX = x_min;
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
  int targetLine = (int)expression();
  bool continueEditing = true;

  while (continueEditing) {
    int foundIndex = -1;

    for (int i = 0; i < lineCount; i++) {
      if (program[i].number == targetLine) {
        foundIndex = i;
        break;
      }
    }

    if (foundIndex == -1) {
      println("LINE NOT FOUND");
      break;
    }
    memset(inputBuffer, 0, BUF_SIZE);
    strncpy(inputBuffer, program[foundIndex].text, BUF_SIZE - 1);

    int length = strlen(inputBuffer);
    int cursor = length;
    print(targetLine); print(" ");
    print(inputBuffer);

    if (editLine(inputBuffer, BUF_SIZE, cursor, length)) {
      storeLine(targetLine, inputBuffer); // Speichern

      if (foundIndex + 1 < lineCount) {
        targetLine = program[foundIndex + 1].number;
        //outchar(13);
      } else {
        println(" (Last line reached)");
        continueEditing = false;
      }
    } else {
      continueEditing = false;
    }
  }

  *txtpos = '\0';
}
//########################################################## Tastatur-Handling #################################################################################
void OnPress(int unicode, uint8_t modifier, uint8_t keycode) {
  uint8_t mod = keyboard1.getModifiers();
  bool shift = (mod & 0x02) || (mod & 0x20);
  bool altGr = (mod & 0x40);
  //print(keycode);
  // 1.PRIORITÄT: AltGr (muss vor Unicode kommen!)
  if (altGr) {
    switch (keycode) {

      case 36: lastUsbChar = 0x7B; return; // { (Taste 7)
      case 37: lastUsbChar = 0x5B; return; // [ (Taste 8)
      case 38: lastUsbChar = 0x5D; return; // ] (Taste 9)
      case 39: lastUsbChar = 0x7D; return; // } (Taste 0)
      case 100: lastUsbChar = 0x7C; return; //| (Taste <)
        //case 114: lastUsbChar = '\''; return;
    }
  }

  if (unicode > 31) {
    lastUsbChar = unicode;
    return;
  }

  switch (keycode) {

    case 40: lastUsbChar = 13; return; // Enter
    case 42: lastUsbChar = 8;  return; // Backspace
    case 41:                           // ESC
      lastUsbChar = 27;
      break_marker = true;
      return;
    case 50: lastUsbChar = 35; return;
    case 53: lastUsbChar = '^'; return; // Zirkumflex
    case 100:
      if (shift) lastUsbChar = '>';
      else lastUsbChar = '<';
      return;


  }
}
//########################################################## SETUP ########################################################################################

void setup() {
  /*
    Serial.begin(9600);
    delay(1000);
    if (CrashReport) {
      Serial.print(CrashReport); // Zeigt an, ob es ein Speicherfehler (MPU) oder ähnliches war
    }
  */
  myusb.begin();
  //keyboard1.forceBootProtocol();
  keyboard1.attachPress(OnPress);
  setSyncProvider(getTeensy3Time);

  //Serial.begin(115200);
  //while (!Serial);

  vga_error_t err = vga.begin(VGA_MODE_640x480);//352x240);
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
  //------ Hauptfenster-Attribute setzen --------
  currentWinIdx = 0;
  windows[currentWinIdx].x = 0;
  windows[currentWinIdx].y = 0;
  windows[currentWinIdx].w = MAX_C;
  windows[currentWinIdx].h = MAX_R;
  windows[currentWinIdx].fcolor = WHITE;
  windows[currentWinIdx].bcolor = 14;
  windows[currentWinIdx].curX = 0;
  windows[currentWinIdx].curY = 0;
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
