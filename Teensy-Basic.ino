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

#define BasicVersion "1.0"
#define BuiltTime "02.04.2026"

//Logbuch
//Version 1.0 02.04.2026                   -erstes Grundgerüst erstellt mit den Befehlen RUN,LIST,FOR..NEXT..STEP,NEW,PRINT,PRZ,VARS
//                                         -Ausgabe noch über serielle Konsole, da der TEENSY noch nackig auf dem Tisch liegt
//                                         -Grundrechenarten integriert, Variablen mit 2 Buchstaben + 10 Zahlen (insgesamt 962 A...Z9)
//                                         -SIN,COS,ABS,SQR und INT hinzugefügt
//                                         -nächste Funktionalität wird SD-Card-Zugriff sein, um Programme auf der SD-Karte zu speichern und zu laden




#include <malloc.h>
extern "C" char* sbrk(int incr);

#include <Arduino.h>
#include <ctype.h>
#define BUF_SIZE 256
char inputBuffer[BUF_SIZE];
static char *txtpos, *list_line;
bool inQuotes = false;

#include <SPI.h>
#include <SD.h>

// Für Teensy 4.1/4.0/3.6 Onboard-SD-Slot:
const int chipSelect = BUILTIN_SDCARD;

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

#define RAMEND 0xFFFF
#define kRamSize  RAMEND

#define STR_LEN 40
#define STR_SIZE 26*27*STR_LEN               //Stringspeicher = Stringlänge 26*40 Zeichen (A..Z * 40 Zeichen)

//static char program[kRamSize];            //Basic-Programmspeicher
//static char Stringtable[STR_SIZE];        //Stringvariablen mit 1 Buchstaben -> 26*40 = 1040 Bytes

//static char *program_start;
//static char *program_end;
static char *current_line;
typedef short unsigned LINENUM;
//static char *variables_begin;
//static char *stack;
//static char *data_line;

#define MAX_GOSUB_STACK 25
struct GosubStack {
  int lineIndex;
  const char* textPos;
};

GosubStack gosubStack[MAX_GOSUB_STACK];
int gosubStackPtr = 0;


//static char table_index;
//static char keyword_index;
//static char key_command;
//static LINENUM linenum;
bool jumped = false;
// Global definieren, damit GOTO und andere darauf zugreifen können
double lastNumberValue = 0;

int precisionValue = 6; // Standardmäßig 2 Stellen


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
  const char* textPos;
};

WhileStack whileStack[MAX_WHILE_STACK];
int whileStackPtr = 0;


struct ForLoop {
  int vIdx1;       // Index für 1. Buchstabe (0-25)
  int vIdx2;       // Index für 2. Zeichen (0-36)
  double targetValue;
  double stepValue;
  int lineIndex;
  const char* textPos;
};

#define MAX_FOR_STACK 26
ForLoop forStack[MAX_FOR_STACK];     // Maximal 8 verschachtelte Schleifen
int forStackPtr = 0;

int currentLineIndex = 0; // Index im program[] Array
bool isRunning = false;   // Status-Flag





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


int dataLineIdx = 0;   // In welcher Zeile im program[] Array sind wir?
char* dataPtr = NULL;  // Wo genau in dieser Zeile steht der nächste Wert?

enum {
  TOKEN_ERROR,
  TOKEN_PRINT,            //erster Basic-Befehl
  TOKEN_GOTO,
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
  TOKEN_PRZ,
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
  TOKEN_ENDIF,
  TOKEN_WHILE,
  TOKEN_WEND,
  TOKEN_RENAME,
  TOKEN_COPY,
  TOKEN_DWRITE,
  TOKEN_PAUSE,
  TOKEN_MEM,       //letzter Basic-Befehl
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
  TOKEN_FREEMEM,      //letzter Funktions-Befehl
  TOKEN_END
};

struct Keyword {
  const char* name;
  int token;
};

Keyword commands[] = {
  {"PRINT", TOKEN_PRINT},
  {"GOTO", TOKEN_GOTO},
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
  {"ENDIF", TOKEN_ENDIF},
  {"WHILE", TOKEN_WHILE},
  {"WEND",  TOKEN_WEND},
  {"RENAME", TOKEN_RENAME},
  {"COPY", TOKEN_COPY},
  {"DWRITE", TOKEN_DWRITE},
  {"PAUSE", TOKEN_PAUSE},
  {"MEM", TOKEN_MEM},
  {NULL, 0}
};

Keyword functions[] = {
  {"RND", TOKEN_RND},
  {"SQR", TOKEN_SQR},
  {"PRZ",   TOKEN_PRZ},
  {"SIN", TOKEN_SIN},
  {"COS", TOKEN_COS},
  {"ABS", TOKEN_ABS},
  {"INT", TOKEN_INT},
  {"PI", TOKEN_PI},
  {"DEG", TOKEN_DEG},
  {"RAD", TOKEN_RAD},
  {"SGN", TOKEN_SGN},
  {"LEN", TOKEN_LEN},
  {"ASC", TOKEN_ASC},
  {"VAL", TOKEN_VAL},
  {"DREAD", TOKEN_DREAD},
  {"FREEMEM", TOKEN_FREEMEM},
  {NULL, 0}
};


#define MAX_LINES 1000
#define LINE_LEN 80

struct BasicLine {
  int number;                                                                           // Die Zeilennummer (z.B. 10)
  char text[LINE_LEN];                                                                  // Der eigentliche Befehlstext
};

BasicLine program[MAX_LINES];
int lineCount = 0;                                                                      // Wie viele Zeilen aktuell im Speicher sind


static void outchar(char c) {
  Serial.write(c);
}

static void line_terminator()
{
  outchar(CR);
  outchar(NL);
}

extern "C" uint32_t _estack;     // Ende des Stacks
extern "C" uint32_t _sbreak;    // Aktueller Heap-Start
extern "C" uint32_t _ebss;      // Ende der statischen Variablen

uint32_t get_free_ram() {
  uint32_t stack_ptr;
  // Wir nehmen die aktuelle Position des Stack-Pointers
  __asm__ volatile ("mov %0, sp" : "=r" (stack_ptr));

  // Der freie Speicher liegt zwischen dem aktuellen Heap-Ende und dem Stack
  char *heap_end = (char *)sbrk(0);
  return (uint32_t)stack_ptr - (uint32_t)heap_end;
}

void parseVarName() {

  spaces();

  if (*txtpos >= 'A' && *txtpos <= 'Z') {                                               // 1. Zeichen (Muss A-Z sein)
    vIdx1 = *txtpos++ - 'A';
  } else {
    Serial.println("SYNTAX ERROR: Illegal Variable Name");
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
  /*
    // DEBUG:
    if (t != -1) {
      Serial.print("Keyword gefunden, Token ID: ");
      Serial.println(t);
    }
  */

  if (t != -1) return t;

  // In getCommandToken oder der Variablen-Logik:
  if (isalpha(*txtpos)) {
    lastVarName = toupper(*txtpos);
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
    // (Deine Logik zum Kopieren in lastStringValue)
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

void getln(int m) {
  int index = 0;
  char c;
  inQuotes = false;
  if (m)
  {
    printmsg("READY.", 1);
  }

  while (true) {
    c = inchar();                                                                         //Input-Funktion

    switch (c) {
      // 1. Eingabe Ende (Line Feed / Carriage Return)
      case '\r':
      case '\n':
        inputBuffer[index] = '\0';
        txtpos = inputBuffer;
        Serial.println();
        return;

      // 2. Backspace / Delete
      case 8:
      case 127:
        if (index > 0) {
          index--;
          if (inputBuffer[index] == '"') inQuotes = !inQuotes;                            // Status flippen, falls ein Anführungszeichen gelöscht wurde
          Serial.print("\b \b");
        }
        break;


      case '"':
        inQuotes = !inQuotes;
        // Speichern ohne toupper
        if (index < BUF_SIZE - 1) {
          inputBuffer[index++] = c;
          Serial.print(c);
        }
        break;

      default:
        if (index < BUF_SIZE - 1) {
          if (!inQuotes) c = toupper(c); // Nur außerhalb von "" umwandeln
          inputBuffer[index++] = c;
          Serial.print(c);
        }
        break;
    }
    delayMicroseconds(100);
  }
}

static int inchar()
{
  //int v;
  char c;
    //char d;


  while (1)
  {
    if (Serial.available()) {
      c = Serial.read();          //Standard-Tasteneingabe

      switch (c) {

        case 0x23: //notdürftig auf # geändert //0x03:       // ESC        -> BREAK
          isRunning = false;
          break;

        default:

          break;
      }

      return c;
    }//if(Terminal.available)


  }//while

}

static void syntaxerror(const char *msg)
{
  printmsg(msg, 1);
  if (current_line != NULL)
  {
    char tmp = *txtpos;           //Position merken
    if (*txtpos != NL) *txtpos = '^';
    list_line = current_line;
    //printline();
    *txtpos = tmp;                //gemerkte Position zurückschreiben
  }
  //Beep(0, 0);                     //Error-BEEP
  current_line = 0;
  line_terminator();
}

void printmsg(const char *msg, int nl) {

  while (*msg) {
    Serial.write(*msg++);
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
        Serial.println("DELETED");
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
    Serial.println("MEMORY FULL");
  }
}

void doList() {
  if (lineCount == 0) {
    Serial.println("EMPTY");
    return;
  }
  Serial.println("--- PROGRAM ---");
  for (int i = 0; i < lineCount; i++) {
    // Zeilennummer ausgeben
    Serial.print(program[i].number);
    Serial.print(" ");
    Serial.println(program[i].text);                                                                // Den Text der Zeile ausgeben
    if (i % 10 == 0) delay(2);                                                                      // Kurze Pause für den Serial-Puffer bei sehr langen Programmen
  }
  Serial.println("READY.");
}

bool jumpTo(int label) {
  for (int i = 0; i < lineCount; i++) {
    if (program[i].number == label) {
      currentLineIndex = i; // Wir setzen den INDEX im Array
      return true;
    }
  }
  return false;
}


char spaces() {                                                                                     // Leerzeichen überspringen
  while (*txtpos == ' ' || *txtpos == '\t') txtpos++;
  return *txtpos;
}

bool expect(char expected) {
  // Leerzeichen überspringen
  spaces();
  // 2. Prüfen, ob das Zeichen übereinstimmt
  if (*txtpos == expected) {
    txtpos++;                                                                                       // Zeichen konsumieren und Pointer weiterbewegen
    return true;
  }
  isRunning = false;                                                                                // Programm stoppen
  return false;
}




double factor() {
  spaces();

  // 1. Einfache Klammer (ohne Funktion davor)
  if (*txtpos == '(') {
    txtpos++;
    double res = expression();
    if (*txtpos == ')') txtpos++;
    return res;
  }

  const char* checkpoint = txtpos;                  //txtpos sichern falls kein Token gefunden wird
  // 2. Token holen
  int t = getFunctionToken();
  double arg;
  if (t != TOKEN_ERROR && t != TOKEN_VARIABLE) {
    //Serial.print(t);
    switch (t) {
      case TOKEN_NUMBER:
        return lastNumberValue;

      case TOKEN_PI:
        return 3.141592653589793;

      case TOKEN_VARIABLE:
        if (isStringVar) {
          return 0;
        }
        return variables[vIdx1][vIdx2];

      case TOKEN_VAL: {
          if (spaces() == '(') {
            txtpos++;

            String s = parseStringExpression();           // Liest den String

            if (*txtpos == ')') {
              txtpos++;                                   // WICHTIG: Die ')' überspringen, damit sie weg ist!
              return atof(s.c_str());
            } else {
              syntaxerror("MISSING ) IN VAL");
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
      case TOKEN_FREEMEM: {
          // Prüfen, ob Klammern folgen: FREEMEM()
          spaces();
          // Gibt den freien Heap-Speicher des Teensy zurück
          return (double)get_free_ram();
        }
      
      // Gruppe der mathematischen Funktionen
      case TOKEN_RND: case TOKEN_SQR: case TOKEN_SIN: case TOKEN_COS:
      case TOKEN_ABS: case TOKEN_INT: case TOKEN_DEG: case TOKEN_RAD:
      case TOKEN_SGN:
        // Gemeinsame Logik für die Klammern der Funktionen
        spaces();
        if (*txtpos == '(') txtpos++;
        arg = expression();
        if (*txtpos == ')') txtpos++;
        spaces();
        // Berechnung basierend auf dem Token
        switch (t) {
          case TOKEN_RND: return (double)(random((int)arg) + 1);
          case TOKEN_SQR: return sqrt(arg);
          case TOKEN_SIN: return sin(arg);
          case TOKEN_COS: return cos(arg);
          case TOKEN_ABS: return fabs(arg);
          case TOKEN_INT: return floor(arg);
          case TOKEN_RAD: return arg * (3.14159265359 / 180.0);
          case TOKEN_DEG: return arg * (180.0 / 3.14159265359);
          case TOKEN_SGN: return (arg > 0) ? 1.0 : (arg < 0 ? -1.0 : 0.0);
          case TOKEN_DREAD : pinMode(int(arg), INPUT_PULLUP); return (double)digitalRead(int(arg));
          
    
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

// 2. Mittlere Ebene: Multiplikation und Division
double term() {
  double val = factor();
  while (true) {
    spaces();
    if (*txtpos == '*') {
      txtpos++;
      val *= factor();
    } else if (*txtpos == '/') {
      txtpos++;
      double divisor = factor();
      if (divisor != 0) val /= divisor;
      else Serial.println("DIV BY ZERO");
    } else {
      return val;
    }
  }
}


// 3. Oberste Ebene: Addition und Subtraktion
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
  Serial.println("--- NUMERISCHE VARIABLEN ---");
  bool foundNum = false;
  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      if (variables[i][j] != 0.0f) {
        Serial.print(getVarName(i, j));
        Serial.print(" = ");
        Serial.println(variables[i][j], precisionValue);
        foundNum = true;
      }
    }
  }
  if (!foundNum) Serial.println("(Keine)");

  // 2. String Variablen (wie bisher)
  Serial.println("\n--- STRING VARIABLEN ---");
  bool foundStr = false;
  for (int i = 0; i < 26; i++) {
    for (int j = 0; j < 37; j++) {
      if (stringVars[i][j] != "") {
        Serial.print(getVarName(i, j));
        Serial.print("$ = \"");
        Serial.print(stringVars[i][j]);
        Serial.println("\"");
        foundStr = true;
      }
    }
  }
  if (!foundStr) Serial.println("(Keine)");

  // 3. ANGEPASST: 3D-ARRAYS mit Zwei-Buchstaben-Namen
  Serial.println("\n--- DIMENSIONIERTE ARRAYS (3D) ---");
  if (arrayCount == 0) {
    Serial.println("(Keine)");
  } else {
    for (int i = 0; i < arrayCount; i++) {
      Serial.print("DIM ");

      // Hier nutzen wir getVarName mit den gespeicherten Indizes
      Serial.print(getVarName(allArrays[i].vIdx1, allArrays[i].vIdx2));

      if (allArrays[i].isString) Serial.print("$");

      Serial.print("(");
      Serial.print(allArrays[i].dimX - 1); Serial.print(",");
      Serial.print(allArrays[i].dimY - 1); Serial.print(",");
      Serial.print(allArrays[i].dimZ - 1);
      Serial.print(")");

      int totalElements = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
      Serial.print("  [");
      Serial.print(totalElements);
      Serial.println(" Elemente]");
    }
  }
  Serial.println("--------------------------------");
}

void cmd_print() {
  bool newline = true;

  while (*txtpos != '\0' && *txtpos != ':' && *txtpos != '\r' && *txtpos != '\n') {
    spaces();

    // --- 1. STRING-CHECK (Literale, Variablen oder ARRAYS) ---
    // Wir schauen nur kurz voraus, ob ein " oder ein Buchstabe mit $ folgt
    bool isString = (*txtpos == '"');
    // Prüfen auf String-Keywords oder Variablen mit $
    if (!isString && isalpha(*txtpos)) {
      // Prüfen auf LEFT$, MID$, RIGHT$, STR$ oder A$
      if (strncmp(txtpos, "LEFT$", 5) == 0 ||
          strncmp(txtpos, "MID$", 4) == 0 ||
          strncmp(txtpos, "RIGHT$", 6) == 0 ||
          strncmp(txtpos, "STR$", 4) == 0) {
        isString = true;
      } else {
        // Prüfen ob es eine Variable mit $ ist (z.B. A$)
        const char* p = txtpos;
        while (isalnum(*p)) p++;
        if (*p == '$') isString = true;
      }
    }
    /*
      if (!isString && isalpha(*txtpos)) {
      // Vorschau: Folgt ein $ nach dem Variablennamen?
      const char* p = txtpos + 1;
      if (isalnum(*p)) p++; // Zweites Zeichen (A1)
      if (*p == '$') isString = true;
      }
    */
    if (isString) {
      // Nutze deine mächtige Funktion für ALLES, was String ist!
      Serial.print(parseStringExpression());
      newline = true;
    }
    // --- 2. NUMERISCHE AUSDRÜCKE ---
    else {
      double val = expression();
      if (val == (long)val) Serial.print((long)val);
      else Serial.print(val, precisionValue);
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
      Serial.print("\t");
      newline = false;
    }
    else break;
  }

  if (newline) Serial.println();
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
  clearAll();
  if (lineCount == 0) {                                                                //Prüfen, ob überhaupt ein Programm im Speicher ist
    Serial.println("NO PROGRAM");
    return;
  }
  txtpos = program[0].text; // <--- Direkt die erste Zeile laden!
}

void skip_line() {
  while (*txtpos != '\0') txtpos++;                                                      //Rest der Zeile ignorieren
}

void cmd_input() {
  spaces();
  if (*txtpos == '"') {
    txtpos++;
    while (*txtpos != '"' && *txtpos != '\0') Serial.print(*txtpos++);
    if (*txtpos == '"') txtpos++;
    spaces();
    if (*txtpos == ',' || *txtpos == ';') txtpos++;
  } else {
    Serial.print("? ");
  }
  // --- NEU: ALTE RESTE ENTFERNEN ---
  delay(10);                                                                                        // Kurz warten, falls noch Zeichen eintrudeln
  while (Serial.available() > 0) Serial.read();

  while (Serial.available() == 0) {                                                                 // 2. WARTEN, bis der User wirklich anfängt zu tippen
    yield(); // Watchdog füttern
  }

  String inputLine = Serial.readStringUntil('\n');                                                  // 3. JETZT die ganze Zeile lesen
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

    if (spaces() == ',') {                                                                          // 5. Prüfen, ob im BASIC-CODE ein Komma für die nächste Variable steht
      txtpos++;
    } else {
      Serial.println();
      return;
    }
  }
}

void cmd_read() {
  while (isRunning) {
    // 1. Welche Variable/Array soll befüllt werden?
    int t = getCommandToken();
    if (t != TOKEN_VARIABLE) {
      Serial.println("SYNTAX ERROR: Variable expected in READ");
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
      Serial.println("OUT OF DATA ERROR");
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
        Serial.print("ARRAY NOT DIMENSIONED: "); Serial.println(getVarName(targetV1, targetV2));
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

/*
  void cmd_read() {
  while (true) {
    // 1. Welche Variable soll befüllt werden? (z.B. READ A)
    if (getCommandToken() != TOKEN_VARIABLE) {
      Serial.println("SYNTAX ERROR: Variable expected");
      isRunning = false;
      return;
    }

    // Indizes der Zielvariable von getToken() sichern
    int targetV1 = vIdx1;
    int targetV2 = vIdx2;

    // 2. Datenquelle suchen
    if (dataPtr == NULL || *dataPtr == '\0') {
      findNextData();
    }

    if (dataLineIdx >= lineCount) {
      Serial.println("OUT OF DATA ERROR");
      isRunning = false;
      return;
    }

    // 3. Zahl extrahieren
    char* next;
    variables[targetV1][targetV2] = strtod(dataPtr, &next);

    // Falls keine Zahl gelesen wurde (strtod hat sich nicht bewegt)
    if (dataPtr == next) {
      dataPtr++; // Ein Zeichen weiterschieben (z.B. Komma überspringen)
      findNextData(); // Neu suchen
      if (dataLineIdx >= lineCount) {
        isRunning = false;
        return;
      }
      variables[targetV1][targetV2] = strtod(dataPtr, &next);
    }
    dataPtr = next;

    // 4. Den DATA-Pointer zum nächsten Element schieben (Komma/Leerzeichen überspringen)
    while (*dataPtr == ' ' || *dataPtr == ',') dataPtr++;

    // 5. Trenner im READ-Befehl prüfen (z.B. READ A, B)
    if (spaces() == ',') {
      txtpos++; // Komma im Quellcode überspringen
      // Schleife läuft weiter für die nächste Variable B
    } else {
      return; // Ende des READ-Befehls
    }
  }
  }
*/

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
      Serial.print("RESTORE LINE NOT FOUND: ");
      Serial.println(targetLineNum);
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

  // --- Teil 1: Den ersten String-Teil lesen ---
  if (*txtpos == '"') {
    txtpos++;
    while (*txtpos != '"' && *txtpos != '\0') {
      res += *txtpos++;
    }
    if (*txtpos == '"') txtpos++;
    //Serial.print(*txtpos);
  }
  // --- LEFT$(s$, n) ---
  if (strncmp(txtpos, "LEFT$", 5) == 0) {
    txtpos += 5;
    if (spaces() == '(') {
      txtpos++;
      String s = parseStringExpression();
      int n = 0;
      if (spaces() == ',') {
        txtpos++;
        n = (int)expression();
      }
      if (spaces() == ')') txtpos++;
      res = s.substring(0, n);
    }
  }
  // --- RIGHT$(s$, n) ---
  else if (strncmp(txtpos, "RIGHT$", 6) == 0) {
    txtpos += 6;
    if (spaces() == '(') {
      txtpos++;
      String s = parseStringExpression();
      int n = 0;
      if (spaces() == ',') {
        txtpos++;
        n = (int)expression();
      }
      if (spaces() == ')') txtpos++;
      if (n > s.length()) n = s.length();
      res = s.substring(s.length() - n);
    }
  }
  // --- MID$(s$, start, len) ---
  else if (strncmp(txtpos, "MID$", 4) == 0) {
    txtpos += 4; // Überspringe "MID$"
    if (spaces() == '(') {
      txtpos++;
      String s = parseStringExpression();
      int start = 0, len = -1;
      if (spaces() == ',') {
        txtpos++;
        start = (int)expression();
      }
      if (spaces() == ',') {
        txtpos++;
        len = (int)expression();
      }
      if (spaces() == ')') txtpos++;

      start--; // BASIC 1-basiert zu C++ 0-basiert
      if (start < 0) start = 0;
      if (len == -1) res = s.substring(start);
      else res = s.substring(start, start + len);
    }
  }
  // B) STR$(zahl) Funktion
  else if (strncmp(txtpos, "STR$", 4) == 0) {
    txtpos += 4;
    if (spaces() == '(') {
      txtpos++;
      double val = expression();
      if (spaces() == ')') txtpos++;
      if (val == (long)val) res = String((long)val);
      else res = String(val, 2);
    }
  }
  //  CHR$(zahl)
  else if (strncmp(txtpos, "CHR$", 4) == 0) {
    txtpos += 4;
    spaces();
    if (*txtpos == '(') {
      txtpos++;
      int code = (int)expression(); // Den ASCII-Wert berechnen
      spaces();
      if (*txtpos == ')') txtpos++;

      // Den ASCII-Code in einen String mit einem Zeichen umwandeln
      res = String((char)code);
    }
  }
  //  SPC(zahl)
  else if (strncmp(txtpos, "SPC$", 3) == 0) {
    txtpos += 4;
    spaces();
    if (*txtpos == '(') {
      txtpos++;
      int code = (int)expression(); // Den ASCII-Wert berechnen
      spaces();
      if (*txtpos == ')') txtpos++;
      for(int i=1; i<int(code); i++) res+=' ';
      // Den ASCII-Code in einen String mit einem Zeichen umwandeln
      //res = String((char)code);
    }
  }
  
  // FALL 2: String-Variable oder String-Array (z.B. A$ oder S$(1,2,3))
  else if (isalpha(*txtpos)) {
    lastVarName = toupper(*txtpos);
    vIdx1 = lastVarName - 'A';
    vIdx2 = 0;
    txtpos++;

    if (isalnum(*txtpos) && *txtpos != '$') {
      //Serial.print("ich bin hier");
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }

    if (*txtpos == '$') {
      txtpos++; // Überspringe das '$'

      // Jetzt entscheiden: Ist es ein Array S$(...) oder eine einfache Variable S$?
      if (*txtpos == '(') {
        return get_string_array_value(); // Nutzt die Funktion von oben
      } else {
        res = stringVars[vIdx1][vIdx2]; // Einfache String-Variable
      }
    }
  }

  // --- Teil 2: REKURSION für das '+' (Addition) ---
  spaces();
  if (*txtpos == '+') {
    txtpos++; // Überspringe das '+'
    res += parseStringExpression(); // Füge den nächsten Teil hinzu
  }

  return res;
}

/*
  // FALL 2: String-Variable oder String-Array (z.B. A$ oder S$(1,2,3))
  if (isalpha(*txtpos)) {
  lastVarName = toupper(*txtpos);
  vIdx1 = lastVarName - 'A';
  vIdx2 = 0;
  txtpos++;

  if (isalnum(*txtpos) && *txtpos != '$') {
    vIdx2 = getVIdx2(*txtpos);
    txtpos++;
  }

  if (*txtpos == '$') {
    txtpos++; // Überspringe das '$'

    // Jetzt entscheiden: Ist es ein Array S$(...) oder eine einfache Variable S$?
    if (*txtpos == '(') {
      return get_string_array_value(); // Nutzt die Funktion von oben
    } else {
      return stringVars[vIdx1][vIdx2]; // Einfache String-Variable
    }
  }
  }

  return res;
  }*/
/*
  String parseStringExpression() {
  String res = "";
  spaces();
  if (*txtpos == '"') {
    txtpos++;
    while (*txtpos != '"' && *txtpos != '\0') {
      res += *txtpos++;
    }
    if (*txtpos == '"') txtpos++;
  }
  return res;
  }
*/
//************************************************************ GOSUB / RETURN **********************************************************

void cmd_gosub() {
  if (getFunctionToken() == TOKEN_NUMBER) {
    int targetNum = (int)lastNumberValue;

    if (gosubStackPtr < MAX_GOSUB_STACK) {
      // Rücksprungpunkt speichern:
      // Wir speichern die aktuelle Zeile und die Position NACH der GOTO-Nummer
      gosubStack[gosubStackPtr].lineIndex = currentLineIndex;
      gosubStack[gosubStackPtr].textPos = txtpos;
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
    txtpos = gosubStack[gosubStackPtr].textPos;

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
  spaces();

  String filter = "";
  if (*txtpos == '"' || isalpha(*txtpos)) {
    filter = parseStringExpression();
    filter.toUpperCase();
  }

  File root = SD.open("/");
  if (!root) {
    Serial.println("Fehler: SD-Karte nicht bereit!");
    return;
  }

  // Zähler für die Statistik
  int fileCount = 0;
  uint64_t totalFilesSize = 0;

  Serial.print("Dateien auf SD-Karte");
  if (filter != "") {
    Serial.print(" (Filter: *"); Serial.print(filter); Serial.print("*)");
  }
  Serial.println(":");

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String fileName = String(entry.name());
    String fileNameUpper = fileName;
    fileNameUpper.toUpperCase();

    if (filter == "" || fileNameUpper.indexOf(filter) != -1) {
      fileCount++;
      totalFilesSize += entry.size();

      Serial.print(fileName);
      if (entry.isDirectory()) {
        Serial.println("/");
      } else {
        if (fileName.length() < 8) Serial.print("\t");
        Serial.print("\t");
        Serial.print(entry.size());
        Serial.println(" Bytes");
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

  Serial.println("-----------------------------------");
  Serial.print(fileCount); Serial.println(" Datei(en) gefunden.");

  Serial.print("Belegter Platz:   ");
  printSmartSize(usedCardBytes);

  Serial.print("Freier Speicher:  ");
  printSmartSize(freeCardBytes);

  Serial.print("Gesamtkapazitaet: ");
  printSmartSize(totalCardBytes);

  Serial.println("READY.");
}

// Hilfsfunktion zur schöneren Größenanzeige (B, KB, MB, GB)
void printSmartSize(uint64_t bytes) {
  if (bytes < 1024) {
    Serial.print((long)bytes); Serial.println(" B");
  } else if (bytes < 1048576) {
    Serial.print((long)(bytes / 1024)); Serial.println(" KB");
  } else if (bytes < 1073741824) {
    Serial.print((long)(bytes / 1048576)); Serial.println(" MB");
  } else {
    Serial.print((float)(bytes / 1073741824.0), 2); Serial.println(" GB");
  }
}

void cmd_rename() {
  spaces();

  // 1. Alten Dateinamen lesen
  String oldName = parseStringExpression();

  spaces();
  if (*txtpos != ',') {
    Serial.println("SYNTAX ERROR: Komma erwartet (RENAME alt, neu)");
    return;
  }
  txtpos++; // Überspringe das Komma

  // 2. Neuen Dateinamen lesen
  String newName = parseStringExpression();

  if (oldName.length() == 0 || newName.length() == 0) {
    Serial.println("SYNTAX ERROR: Dateinamen fehlen");
    return;
  }

  // 3. Umbenennen auf der SD-Karte
  if (SD.exists(oldName.c_str())) {
    if (SD.rename(oldName.c_str(), newName.c_str())) {
      Serial.print("Datei umbenannt in: ");
      Serial.println(newName);
    } else {
      Serial.println("FEHLER: Umbenennen fehlgeschlagen.");
    }
  } else {
    Serial.print("FEHLER: Datei nicht gefunden: ");
    Serial.println(oldName);
  }

  Serial.println("READY.");
}

void cmd_load() {
  spaces();
  String fileNameStr = parseStringExpression();
  if (fileNameStr.length() == 0) {
    Serial.println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // 2. Datei auf der SD-Karte des Teensy 4.1 suchen
  if (!SD.exists(fileName)) {
    Serial.print("FILE NOT FOUND: ");
    Serial.println(fileName);
    return;
  }

  File file = SD.open(fileName);
  if (!file) {
    Serial.println("ERROR: Could not open file");
    return;
  }

  cmd_new();

  Serial.print("Loading "); Serial.println(fileName);

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
  Serial.println("READY.");
}


void cmd_save() {
  spaces();
  String fileNameStr = parseStringExpression();

  if (fileNameStr.length() == 0) {
    Serial.println("SYNTAX ERROR: File name expected");
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
    Serial.print("ERROR: Could not create ");
    Serial.println(fileName);
    return;
  }

  Serial.print("Saving to ");
  Serial.println(fileName);

  // 4. Zeile für Zeile speichern
  for (int j = 0; j < lineCount; j++) {
    file.print(program[j].number);
    file.print(" ");
    file.println(program[j].text);
  }

  file.close();
  Serial.println("READY.");
}


void cmd_delete() {
  spaces();
  String fileNameStr = parseStringExpression();

  if (fileNameStr.length() == 0) {
    Serial.println("SYNTAX ERROR: File name expected");
    return;
  }

  const char* fileName = fileNameStr.c_str();

  // 2. Prüfen, ob die Datei auf der SD-Karte existiert
  if (SD.exists(fileName)) {
    // 3. Datei löschen
    if (SD.remove(fileName)) {
      Serial.print("File ");
      Serial.print(fileName);
      Serial.println(" deleted.");
    } else {
      Serial.println("ERROR: Could not delete file.");
    }
  } else {
    Serial.print("FILE NOT FOUND: ");
    Serial.println(fileName);
  }

  Serial.println("READY.");
}

void cmd_copy() {
  spaces();
  String srcName = parseStringExpression(); // Quell-Datei

  spaces();
  if (*txtpos != ',') {
    Serial.println("SYNTAX ERROR: Komma erwartet (COPY src, dest)");
    return;
  }
  txtpos++; // Komma überspringen

  String destName = parseStringExpression(); // Ziel-Datei

  if (srcName == "" || destName == "") return;

  if (!SD.exists(srcName.c_str())) {
    Serial.print("FILE NOT FOUND: "); Serial.println(srcName);
    return;
  }

  // Falls Ziel existiert, löschen
  if (SD.exists(destName.c_str())) SD.remove(destName.c_str());

  File srcFile = SD.open(srcName.c_str(), FILE_READ);
  File destFile = SD.open(destName.c_str(), FILE_WRITE);

  if (srcFile && destFile) {
    Serial.print("Copying "); Serial.print(srcName); Serial.print(" to "); Serial.println(destName);

    // Puffer für schnelles Kopieren (Teensy hat genug RAM)
    uint8_t buffer[512];
    while (srcFile.available()) {
      int bytesRead = srcFile.read(buffer, sizeof(buffer));
      destFile.write(buffer, bytesRead);
    }

    destFile.close();
    srcFile.close();
    Serial.println("READY.");
  } else {
    Serial.println("ERROR: Could not open files for copying.");
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
      Serial.print("STRING ARRAY NOT DIMENSIONED: ");
      Serial.println(getVarName(v1, v2)); // Schönerer Error mit vollem Namen
      isRunning = false; return "";
    }

    // Index berechnen
    int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);

    // Sicherheitscheck
    int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
    if (idx >= totalSize || idx < 0) {
      Serial.println("STRING ARRAY INDEX OUT OF BOUNDS");
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
      Serial.print("ARRAY NOT DIMENSIONED: ");
      Serial.println(getVarName(v1, v2)); // Zeigt z.B. "OX" statt nur "O"
      isRunning = false; return 0;
    }

    // Flachen Index berechnen
    int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);

    // Sicherheitscheck
    int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
    if (idx >= totalSize || idx < 0) {
      Serial.println("ARRAY INDEX OUT OF BOUNDS");
      isRunning = false; return 0;
    }

    if (sVar) return 0;
    return allArrays[i].numData[idx];
  }

  // Wenn keine Klammer da ist, ist es eine normale Variable
  return variables[v1][v2];
}
/*
  void set_array_value(int v1, int v2, bool isString) {
  // Wir stehen am Anfang der Klammer '('
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
    isRunning = false; return;
  }

  spaces();
  if (*txtpos == '=') {
    txtpos++;

    // Suche das Array mit beiden Indizes (v1, v2)
    int i = findArray(v1, v2, isString);

    if (i == -1) {
      Serial.print("ARRAY NOT DIMENSIONED: ");
      Serial.println(getVarName(v1, v2));
      isRunning = false;
      return;
    }

    // Index berechnen
    int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);

    // Sicherheitscheck gegen Buffer Overflow
    int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
    if (idx >= totalSize || idx < 0) {
      Serial.println("ARRAY INDEX OUT OF BOUNDS");
      isRunning = false; return;
    }

    // Wert schreiben
    if (isString) {
      allArrays[i].strData[idx] = parseStringExpression();
    } else {
      allArrays[i].numData[idx] = expression();
    }
  }
  }
*/
int findArray(int v1, int v2, bool isString) {
  for (int i = 0; i < arrayCount; i++) {
    if (allArrays[i].vIdx1 == v1 && allArrays[i].vIdx2 == v2 && allArrays[i].isString == isString) {
      return i;
    }
  }
  return -1;
}
/*
  int findArray(char name, bool isString) {
  for (int i = 0; i < arrayCount; i++) {
    if (allArrays[i].name == name && allArrays[i].isString == isString) {
      return i;
    }
  }
  return -1; // Nicht gefunden
  }
*/
void cmd_dim() {
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

    if (isalnum(*txtpos) && *txtpos != '$') {
      vIdx2 = getVIdx2(*txtpos);
      txtpos++;
    }

    bool sVar = false;
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
      allArrays[arrayCount].vIdx1 = vIdx1;
      allArrays[arrayCount].vIdx2 = vIdx2;
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
/*
  void cmd_dim() {
  spaces();
  if (!isalpha(*txtpos)) {
    isRunning = false;
    return;
  }

  char name = toupper(*txtpos);
  txtpos++;

  bool sVar = (*txtpos == '$');
  if (sVar) txtpos++;

  if (*txtpos != '(') {
    isRunning = false;
    return;
  }
  txtpos++;

  // Dimensionen einlesen (Standardmäßig 0, falls nicht angegeben)
  int d1 = (int)expression();
  int d2 = 0;
  int d3 = 0;

  if (*txtpos == ',') {
    txtpos++;
    d2 = (int)expression();
  }
  if (*txtpos == ',') {
    txtpos++;
    d3 = (int)expression();
  }

  if (*txtpos == ')') txtpos++;
  else {
    isRunning = false;
    return;
  }

  // Prüfen, ob Array schon existiert (dann erst löschen)
  int existing = findArray(name, sVar);
  if (existing != -1) {
    // Hier könnte man den alten Speicher mit free/delete freigeben
  }

  // Speicher reservieren (Größe + 1, da BASIC-Arrays bei 0 beginnen)
  int sizeX = d1 + 1;
  int sizeY = d2 + 1;
  int sizeZ = d3 + 1;
  int totalSize = sizeX * sizeY * sizeZ;

  allArrays[arrayCount].name = name;
  allArrays[arrayCount].isString = sVar;
  allArrays[arrayCount].dimX = sizeX;
  allArrays[arrayCount].dimY = sizeY;
  allArrays[arrayCount].dimZ = sizeZ;

  if (sVar) {
    allArrays[arrayCount].numData = NULL;
    allArrays[arrayCount].strData = new String[totalSize];
  } else {
    allArrays[arrayCount].strData = NULL;
    allArrays[arrayCount].numData = (double*)malloc(totalSize * sizeof(double));
    for (int i = 0; i < totalSize; i++) allArrays[arrayCount].numData[i] = 0.0;
  }

  arrayCount++;
  spaces();
  }
*/
/*
  void cmd_assignment() {
  char varNameChar = lastVarName;
  int targetV1 = vIdx1;
  int targetV2 = vIdx2;
  bool sVar = isStringVar;

  spaces();

  // --- FALL A: ARRAY-Zuweisung (z.B. S$(1,2,3) = A$ + "!") ---
  if (*txtpos == '(') {
    txtpos++;

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
      syntaxerror("MISSING )"); return;
    }

    spaces();
    if (*txtpos == '=') {
      txtpos++;
      int i = findArray(targetV1, targetV2, sVar); //int i = findArray(varNameChar, sVar);
      if (i == -1) {
        Serial.print("ARRAY NOT DIMENSIONED: "); Serial.println(varNameChar);
        isRunning = false; return;
      }

      int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);
      int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;
      if (idx >= totalSize || idx < 0) {
        syntaxerror("ARRAY INDEX OUT OF BOUNDS"); return;
      }

      if (sVar) {
        // Hier wird jetzt die rekursive Addition unterstützt!
        allArrays[i].strData[idx] = parseStringExpression();
      } else {
        allArrays[i].numData[idx] = expression();
      }
    }
  }
  // --- FALL B: NORMALE Variable (z.B. A$ = "HI " + B$) ---
  else if (*txtpos == '=') {
    txtpos++;
    if (sVar) {
      // Nutzt die neue parseStringExpression mit "+" Support
      stringVars[targetV1][targetV2] = parseStringExpression();
    } else {
      variables[targetV1][targetV2] = expression();
    }
  }
  else {
    syntaxerror("SYNTAX ERROR IN ASSIGNMENT");
  }
  }

*/
void cmd_assignment() {
  // Wir nutzen die vom Parser (getCommandToken) gesetzten Indizes
  int targetV1 = vIdx1;
  int targetV2 = vIdx2;
  bool sVar = isStringVar;

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
      syntaxerror("MISSING )"); return;
    }

    spaces();
    if (*txtpos == '=') {
      txtpos++;

      // Suche das Array mit beiden Indizes (wichtig für OX, OY etc.)
      int i = findArray(targetV1, targetV2, sVar);

      if (i == -1) {
        Serial.print("ARRAY NOT DIMENSIONED: ");
        Serial.println(getVarName(targetV1, targetV2)); // Zeigt jetzt "OX" statt nur "O"
        isRunning = false; return;
      }

      int idx = x + (y * allArrays[i].dimX) + (z * allArrays[i].dimX * allArrays[i].dimY);
      int totalSize = allArrays[i].dimX * allArrays[i].dimY * allArrays[i].dimZ;

      if (idx >= totalSize || idx < 0) {
        syntaxerror("ARRAY INDEX OUT OF BOUNDS"); return;
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
    syntaxerror("SYNTAX ERROR IN ASSIGNMENT");
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

    if (t == TOKEN_IF || t == TOKEN_WHILE) {
      nesting++;
    }
    else if (t == TOKEN_ENDIF || t == TOKEN_WEND) {
      if (nesting > 0) {
        nesting--;
      } else {
        if (t == target1 || t == target2) return;
      }
    }
    else if (t == TOKEN_ELSE && nesting == 0) {
      if (t == target1 || t == target2) return;
    }

    if (pBefore == txtpos) txtpos++;
  }
}
//**************************************** While/Wend ********************************************************************
void cmd_while() {
  spaces();
  // 1. Position der Bedingung für den Stack merken
  const char* conditionStart = txtpos;

  // 2. Bedingung auswerten (txtpos wandert hinter die Bedingung)
  bool condition = relation();

  if (condition) {
    // PRÜFEN: Sind wir bereits in dieser Schleife (Rücksprung von WEND)?
    bool alreadyOnStack = false;
    if (whileStackPtr > 0 && whileStack[whileStackPtr - 1].textPos == conditionStart) {
      alreadyOnStack = true;
    }

    if (!alreadyOnStack) {
      if (whileStackPtr < MAX_WHILE_STACK) {
        whileStack[whileStackPtr].lineIndex = currentLineIndex;
        whileStack[whileStackPtr].textPos = conditionStart;
        whileStackPtr++;
      } else {
        isRunning = false;
        Serial.println("ERR: WHILE STACK OVERFLOW");
        return;
      }
    }

    // REST DER ZEILE IGNORIEREN (verhindert Assignment-Fehler)
    while (*txtpos != '\0' && *txtpos != ':') txtpos++;

  } else {
    // BEDINGUNG FALSCH: Den gesamten Block bis zum WEND überspringen
    skip_to(TOKEN_WEND, TOKEN_WEND);

    // Wir stehen jetzt am WEND. Wir müssen zur NÄCHSTEN Zeile (z.B. Zeile 60)
    spaces();
    if (*txtpos == '\0') {
      currentLineIndex++;
      if (currentLineIndex < lineCount) {
        txtpos = program[currentLineIndex].text;
      } else {
        isRunning = false; // Ende des Programms
      }
    } else if (*txtpos == ':') {
      txtpos++; // Falls nach dem WEND noch ein Befehl in der Zeile steht
    }

    // Falls die Schleife auf dem Stack war: Jetzt entfernen
    if (whileStackPtr > 0 && whileStack[whileStackPtr - 1].textPos == conditionStart) {
      whileStackPtr--;
    }

    jumped = true; // Dem Haupt-Loop signalisieren: Zeiger ist gesetzt
  }
}
void cmd_wend() {
  if (whileStackPtr > 0) {
    // Springe zurück zum WHILE-Befehl
    currentLineIndex = whileStack[whileStackPtr - 1].lineIndex;
    txtpos = program[currentLineIndex].text; // Wir springen zum ANFANG der Zeile
    // (Oder exakt zum Wort WHILE, falls du textPos dort gespeichert hast)
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

    // Optional: Kurze Info im Serial Monitor
    // Serial.print("Pin "); Serial.print(pin); Serial.println(state ? " HIGH" : " LOW");
  } else {
    syntaxerror("KOMMA ERWARTET: DWRITE pin, state");
  }
}

//*********************************************** Pause-Befehl ****************************************************
void cmd_pause() {
  spaces();

  // Die Millisekunden einlesen (kann auch eine Rechnung sein, z.B. PAUSE 100*5)
  int ms = (int)expression();

  if (ms > 0) {
    // Kurze Sicherheitsprüfung für den Break-Check während der Pause
    // Bei sehr langen Pausen (z.B. PAUSE 10000) wäre der Teensy sonst "taub"
    if (ms > 100) {
      unsigned long start = millis();
      while (millis() - start < (unsigned long)ms) {
        // Prüfen, ob der User STRG+C (ASCII 3) gesendet hat, um abzubrechen
        if (Serial.available() && Serial.read() == 3) {
          isRunning = false;
          Serial.println("\nBREAK.");
          return;
        }
        yield(); // Dem Teensy-System Zeit für Hintergrundaufgaben geben
      }
    } else {
      delay(ms); // Für sehr kurze Pausen reicht das Standard-Delay
    }
  }
}
//************************************************************************* MEM-Befehl ****************************************************************************************
void cmd_mem() {
  int usedLines = lineCount;
  int freeLines = MAX_LINES - lineCount; // MAX_LINES ist deine definierte Obergrenze

  // Berechnung des verbrauchten Speichers im Programm-Array
  long usedBytes = 0;
  for (int i = 0; i < lineCount; i++) {
    usedBytes += strlen(program[i].text);
  }

  Serial.println("--- BASIC PROGRAM MEMORY ---");
  Serial.print("Zeilen belegt: "); Serial.print(usedLines);
  Serial.print(" / Frei: "); Serial.println(freeLines);

  Serial.print("Speicher Text: "); Serial.print(usedBytes);
  Serial.println(" Bytes belegt.");

  // Dynamischer Speicher (für deine 3D-Arrays)
  Serial.print("Freier RAM (Heap): ");
  Serial.print(get_free_ram()); // Die Funktion von oben
  Serial.println(" Bytes");

  Serial.println("READY.");
}

void Basic_interpreter() {
  int lineNumber;

  while (1) {

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
        Serial.println("READY.");
        continue;
      }
      if (!jumped) {
        txtpos = program[currentLineIndex].text;
      }

    }

    //Serial.print("Lade Zeile: ");
    //Serial.println(program[currentLineIndex].number);

    while (*txtpos != 0) {                     // --- 2. ZEILEN-SCHLEIFE ---

      jumped = false;

      if (Serial.available() && Serial.peek() == 3) {
        Serial.read();
        isRunning = false;
        Serial.println("\nBREAK.");
        break;
      }
      if (*txtpos == ':') {
        txtpos++;
        continue;
      }

      int token = getCommandToken();

      if (token == TOKEN_END) break;

      switch (token) {
        case TOKEN_PRINT: cmd_print(); break;
        case TOKEN_LIST:  doList();    break;
        case TOKEN_RUN:   cmd_run();   break;
        case TOKEN_NEW:   cmd_new();   break;
        case TOKEN_VARS:  doVars();    break;
        case TOKEN_STOP:  isRunning = false; Serial.println("END."); break;
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
        case TOKEN_ELSE: {
            skip_to(TOKEN_ENDIF, TOKEN_ENDIF);
            spaces();
            if (*txtpos == '\0') {
              currentLineIndex++;
              if (currentLineIndex < lineCount) {
                txtpos = program[currentLineIndex].text;
                jumped = true;
              } else {
                isRunning = false;
              }
            }
          }
          break;
        case TOKEN_ENDIF: break;                                                      //dient nur als Sprungmarke für skip_to
        case TOKEN_WHILE: cmd_while(); break;
        case TOKEN_WEND:  cmd_wend();  break;
        case TOKEN_GOTO: {
            if (getFunctionToken() == TOKEN_NUMBER) {                                 // GOTO erwartet eine Nummer, getFunctionToken() kann diese lesen
              int targetNum = (int)lastNumberValue;
              if (jumpTo(targetNum)) {
                txtpos = program[currentLineIndex].text;
                jumped = true;
              } else {
                printmsg(wronglinenr, 0);
                Serial.println(targetNum);
                Serial.print("in Line:");
                Serial.println(program[currentLineIndex].number);
                isRunning = false;
                *txtpos = '\0';
              }
            }
            break;
          }


        case TOKEN_VARIABLE: cmd_assignment(); break;

        case TOKEN_IF: {
            bool condition = relation();
            if (getCommandToken() != TOKEN_THEN) {
              isRunning = false;
              break;
            }

            if (!condition) {
              // Bedingung FALSCH: Springe zum ELSE oder ENDIF
              skip_to(TOKEN_ELSE, TOKEN_ENDIF);
            } else {
              // Bedingung WAHR:
              spaces();
              if (*txtpos == '\0') {
                // Zeile ist nach THEN zu Ende, wir müssen zur nächsten Zeile springen
                currentLineIndex++;
                if (currentLineIndex < lineCount) {
                  txtpos = program[currentLineIndex].text;
                  jumped = true; // Verhindert, dass der äußere Loop zurücksetzt
                } else {
                  isRunning = false;
                }
              }
              // Falls noch was in der Zeile steht (z.B. THEN PRINT...), macht er einfach weiter
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
              forStack[forStackPtr].textPos = txtpos;

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
                txtpos = forStack[sIdx].textPos;                                        // ZURÜCKSPRINGEN:
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

        case TOKEN_ERROR:
          if (strlen(txtpos) > 0) {
            syntaxerror(syntaxmsg);//Serial.print("SYNTAX ERROR in Line: ");
            Serial.print(program[currentLineIndex].number);
            Serial.println(program[currentLineIndex].text);
            isRunning = false;
            *txtpos = '\0';                                                               //Wichtig sonst gibt's ne Endlosschleife bei Error
            break;
          }
          break;

        default:

          break;
      }


      spaces();

      // 1. Zuerst schauen, ob ein Doppelpunkt kommt (für mehrere Befehle)
      if (*txtpos == ':') {
        txtpos++;
        continue; // Nächster Befehl in derselben Zeile
      }

      // 2. Wenn ein Sprung (GOTO/NEXT/RUN) passierte: Zeile verlassen,
      // aber NICHT hochzählen!

      //if (jumped) break;

      // 3. Wenn die Zeile noch nicht zu Ende ist (z.B. nach THEN)
      if (*txtpos != '\0') continue;

      // 4. Nur wenn wir am echten Ende (\0) sind UND nicht gesprungen wurde:
      if (isRunning && !jumped) {
        // Wenn wir am Ende der Zeile stehen (\0)
        if (*txtpos == '\0') {
          currentLineIndex++;
          if (currentLineIndex < lineCount) {
            txtpos = program[currentLineIndex].text;
            // Wichtig: Wir bleiben in der while(1), damit die neue Zeile
            // sofort bearbeitet wird
            continue;
          } else {
            isRunning = false;
          }
        }
      }

      /*{ //&& currentLineIndex == oldLine) {
        currentLineIndex++;
        break;
        }*/

      break;
    }  //while(*txtpos != '\0')

  }// while(1)
} //Basic-Interpreter



void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("**** TEENSY BASIC 1.0 ****");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Initialisierung fehlgeschlagen!");
  }
}


void loop() {
  Basic_interpreter();
}
