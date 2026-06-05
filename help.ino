// 1. Die Texte im Flash definieren
//*********************Befehle*********************************  
const char h_DEFAULT[] PROGMEM = "Syntax: HELP [Befehl/Funktion]";
const char h_PRINT[]            PROGMEM = "PRINT Value, String...\rEXAMPLE: PRINT 5+4/7:PRINT NAME$:PRINT'TEXT'";     
const char h_GOTO[]             PROGMEM = "GOTO Linenumber\rEXAMPLE: GOTO 230";
const char h_LIST[]             PROGMEM = "LIST <from Line, to Line>\rEXAMPLE :LIST 10,100";
const char h_RUN[]              PROGMEM = "Start the Programm\rRUN'/FILENAME' starts the filename";
const char h_VARIABLE[]         PROGMEM = " ";        // A-Z
const char h_NUMBER[]           PROGMEM = " ";           //Zahlen
const char h_STRING_LITERAL[]   PROGMEM = " ";   //Strings
const char h_NEW[]              PROGMEM = "Delete Program and clears the Variables";
const char h_FOR[]              PROGMEM = "FOR Var=Start to End-Condition\rEXAMPLE: FOR I=0 TO 25 STEP 2:PRINT I:NEXT I";
const char h_VARS[]             PROGMEM = "Shows the defined Variables/Strings";
const char h_IF[]               PROGMEM = "IF condition THEN result\rEXAMPLE: IF A>5 THEN PRINT A";
const char h_STOP[]             PROGMEM = "program is terminated";
const char h_REM[]              PROGMEM = "REM comment\rEXAMPLE: REM here starts the Program";
const char h_INPUT[]            PROGMEM = "NPUT Inputtext;var,var$...\rEXAMPLE: INPUT'NAME:';A$";
const char h_DATA[]             PROGMEM = "DATA Lines with Numbers or Text";
const char h_READ[]             PROGMEM = "READ Var,Strings - reads Datalines\rEXAMPLE: READ A,N$,B";                                                         //20
const char h_RESTORE[]          PROGMEM = "RESTORE resets the Data-Pointer\rRESTORE lnr - set the Data-Pointer on Line->lnr";
const char h_FILES[]            PROGMEM = "Shows the Directory of SD-Card\rDIR'BAS'\rShows only Files with the BAS-Extension";
const char h_DIR[]              PROGMEM = "Shows the Directory of SD-Card\rDIR'BAS'\rShows only Files with the BAS-Extension";
const char h_LOAD[]             PROGMEM = "LOAD Filename\rFilename in qoute or as String\rEXAMPLE: LOAD'/TEST.BAS'";
const char h_SAVE[]             PROGMEM = "SAVE Filename\rFilename in qoute or as String";
const char h_DELETE[]           PROGMEM = "DEL Filename\rDelete File on SD-Card";
const char h_DIM[]              PROGMEM = "DIM Var(dim,dim,dim)\rdimensioned arrays with max 3 Dimensions";
const char h_GOSUB[]            PROGMEM = "GOSUB Linenumber\rEXAMPLE :500 GOSUB 700\r         700 PRINT A\r         710 RETURN";
const char h_RETURN[]           PROGMEM = "GOSUB Linenumber\rEXAMPLE :500 GOSUB 700\r         700 PRINT A\r         710 RETURN";
const char h_ELSE[]             PROGMEM = "alternative condition on IF THEN\rmust be in next line";                                                           //30
const char h_WHILE[]            PROGMEM = "";
const char h_WEND[]             PROGMEM = "";
const char h_RENAME[]           PROGMEM = "RENAME Filename_old, Filename_new\rFilename in qoute or as String";
const char h_COPY[]             PROGMEM = "COPY Filename_source, Filename_target\rcopy's a File";
const char h_DWRITE[]           PROGMEM = "";
const char h_PAUSE[]            PROGMEM = "PAUSE ms\rPause in milliseconds";
const char h_MEM[]              PROGMEM = "shows the Memory-usage";
const char h_ON[]               PROGMEM = "ON Variable\rExpandet GOTO or GOSUB Command\rEXAMPLE: ON A GOTO 120,230,430";
const char h_STIME[]            PROGMEM = "STIME hh,mm,ss,dd,mm,jjjj\rSet the Realtime-Clock";
const char h_DUMP[]             PROGMEM = "DMP Memtype,Adress\rHexmonitor for Program 0 or PSRAM 1";                                                          //40
const char h_DEFN[]             PROGMEM = "DEFN Var=(Function)\rCreate User-Function\rEXAMPLE: DEFN A(A,B)=(SIN(A)*COS(B))\r         c=FN A(12,5)";
const char h_CLS[]              PROGMEM = "Clear Screen";
const char h_COL[]              PROGMEM = "COL FG,BG\rChange the Foreground and Background Color\rEXAMPLE: COL 0,60";
const char h_PSET[]             PROGMEM = "PSET x,y,color\rSet a Pixel on x,y with color\rcolor is optional";
const char h_TAB[]              PROGMEM = "set a Tabulator\rEXAMPLE: PRINT TAB(8);'Hallo'\r->set the cursor 8 steps in x position";
const char h_AT[]               PROGMEM = "only allowed with PRINT command\rEXAMPLE: PRINT AT(x,y);A$";
const char h_TRON[]             PROGMEM = "set the TRON-Mode for debugging\rEXAMPLE: TRON ms -> TRON-Mode on with delay in ms";
const char h_TROFF[]            PROGMEM = "switch the TRON-Mode off";
const char h_PIC[]              PROGMEM = "PIC x,y,Filename\rshows a bitmap on Position x,y\rPicture-Formats are BMP or JPG";
const char h_LINE[]             PROGMEM = "LINE x,y,xx,yy,color\rDraws a line from x,y to xx,yy";                                                             //50
const char h_RECT[]             PROGMEM = "RECT x,y,w,h,fill\rDraws a Rect from x,y with w-wide and h-heigh in pencolor\rf=0 ->Frame,f=1 filled Rectangle";
const char h_CIRC[]             PROGMEM = "CIRC x,y,w,h,fill\rDraws a Circle or Ellipse from x,y,w,h, in pencolor\rf=0 ->outline f=1 filled with backcolor";
const char h_PEN[]              PROGMEM = "PEN color\rset the Pencolor 0...255";
const char h_CUR[]              PROGMEM = "CUR on/off\rswitch the Cursor on(CUR 1) or off (Cur 0)";
const char h_EDIT[]             PROGMEM = "EDIT linenumber\rstart the Editor for choosed line";
const char h_AND[]              PROGMEM = "Logical AND Condition\rEXAMPLE: IF A>5 AND B<6 THEN C=0";
const char h_OR[]               PROGMEM = "Logical OR Condition\rEXAMPLE: IF A>5 OR B<6 THEN C=0";
const char h_WINDOW[]           PROGMEM = "WINDOW(nr,x,y,xx,yy<,fcol,bcol,titlecol,Title>)\rcreates a window nr=1..5\rWINDOW without Parameters switch to Mainwindow\rWINDOW(nr) switch to Window nr\rfcol,bcol,titlecol set the colors\rTitle is the window title";
const char h_RENUM[]            PROGMEM = "RENUM the program-linenumbers\rRENUM start,step - renumbers with start and stepwise";
const char h_USING[]            PROGMEM = "";            //60
const char h_SID[]              PROGMEM = "";       
const char h_POS[]              PROGMEM = "";
const char h_PAINT[]            PROGMEM = "";
const char h_GET[]              PROGMEM = "";
const char h_PUT[]              PROGMEM = "";
const char h_UPDATE[]           PROGMEM = "";
const char h_HIDE[]             PROGMEM = "";
const char h_GFXCLS[]           PROGMEM = "";
const char h_SPLOAD[]           PROGMEM = "";
const char h_TDRAW[]            PROGMEM = "";             //70
const char h_SPRITE[]           PROGMEM = "";
const char h_TSHEET[]           PROGMEM = "";
const char h_SCROLL[]           PROGMEM = "";
const char h_MAPLOAD[]          PROGMEM = "";
const char h_SETSCROLL[]        PROGMEM = "";
const char h_PTILE[]            PROGMEM = "";
const char h_ANIMATE[]          PROGMEM = "";
const char h_SETCHAR[]          PROGMEM = "SETCHAR ascii_code, b1, b2, b3, b4, b5, b6, b7, b8\rchange the char of ascii_code with new char-code's";
const char h_MENU[]             PROGMEM = "Startmenu to choose another Firmware-Hex-File";      
const char h_MOUNT[]            PROGMEM = "MOUNT the SD-Card for use";             //80
const char h_UNMOUNT[]          PROGMEM = "UNMOUNT the SD-Card to savely remove";   
const char h_THEME[]            PROGMEM = "THEME shows the available Colorthemes\rTHEME n - set Colortheme n\rTHEME S - save the Colortheme as User";
const char h_TEXT[]             PROGMEM = "Draw Text on x,y Position\rx and y are pixel-coordinates\rEXAMPLE: TEXT x,y,tcolor,bcolor,Text,wide\rDraw Text on x,y in tcolor on bcolor\rwide=0 standard, wide=1 Text size doubled";
const char h_HELP[]             PROGMEM = "shows available Commands and Functions\rHelp search-name -> shows detailed Informations";      //letzter Basic-Befehl

//*********************Funktionen*********************************  

const char h_RND[]              PROGMEM = "RND(Value)\rreturns a random number from 0 to Value\rEXAMPLE: A=RND(25)";      
const char h_SQR[]              PROGMEM = "SQR(Value)\rthe square root of Value";
const char h_SIN[]              PROGMEM = "SIN(Value)\rreturns the Sinus in RAD";
const char h_COS[]              PROGMEM = "COS(Value)\rreturns the Cosinus in Rad";
const char h_ABS[]              PROGMEM = "ABS(Value)\rreturns the Absolut-Value";

// 2. Struktur für die Zuordnung von Token zu Text
struct CommandDetails {
  uint8_t token;
  const char* text;
};

const CommandDetails cdetail_table[] = {
  {TOKEN_ERROR,h_DEFAULT},
  {TOKEN_PRINT, h_PRINT},
  {TOKEN_GOTO, h_GOTO},
  {TOKEN_LIST, h_LIST},
  {TOKEN_RUN, h_RUN},
  {TOKEN_VARIABLE,0},
  {TOKEN_NUMBER,0},
  {TOKEN_STRING_LITERAL,0},
  {TOKEN_NEW, h_NEW},
  {TOKEN_FOR, h_FOR},
  {TOKEN_TO, h_FOR},
  {TOKEN_NEXT, h_FOR},
  {TOKEN_STEP, h_FOR},
  {TOKEN_VARS, h_VARS},
  {TOKEN_IF, h_IF},
  {TOKEN_THEN, h_IF},
  {TOKEN_STOP, h_STOP},
  {TOKEN_REM, h_REM},
  {TOKEN_INPUT,h_INPUT},
  {TOKEN_DATA,h_DATA},
  {TOKEN_READ,h_READ},             //20
  {TOKEN_RESTORE,h_RESTORE},
  {TOKEN_FILES,h_FILES},
  {TOKEN_DIR,h_DIR},
  {TOKEN_LOAD,h_LOAD},
  {TOKEN_SAVE,h_SAVE},
  {TOKEN_DELETE,h_DELETE},
  {TOKEN_DIM,h_DIM},
  {TOKEN_GOSUB,h_GOSUB},
  {TOKEN_RETURN,h_RETURN},
  {TOKEN_ELSE,h_ELSE},             //30
  {TOKEN_WHILE,h_WHILE},
  {TOKEN_WEND,h_WEND},
  {TOKEN_RENAME,h_RENAME},
  {TOKEN_COPY,h_COPY},
  {TOKEN_DWRITE,h_DWRITE},
  {TOKEN_PAUSE,h_PAUSE},
  {TOKEN_MEM,h_MEM},
  {TOKEN_ON,h_ON},
  {TOKEN_STIME,h_STIME},
  {TOKEN_DUMP,h_DUMP},             //40
  {TOKEN_DEFN,h_DEFN},
  {TOKEN_CLS,h_CLS},
  {TOKEN_COL,h_COL},
  {TOKEN_PSET,h_PSET},
  {TOKEN_TAB,h_TAB},
  {TOKEN_AT,h_AT},
  {TOKEN_TRON,h_TRON},
  {TOKEN_TROFF,h_TROFF},
  {TOKEN_PIC,h_PIC},
  {TOKEN_LINE,h_LINE},             //50
  {TOKEN_RECT,h_RECT},
  {TOKEN_CIRC,h_CIRC},
  {TOKEN_PEN,h_PEN},
  {TOKEN_CUR,h_CUR},
  {TOKEN_EDIT,h_EDIT},
  {TOKEN_AND,h_AND},
  {TOKEN_OR,h_OR},
  {TOKEN_WINDOW,h_WINDOW},
  {TOKEN_RENUM,h_RENUM},
  {TOKEN_USING,h_USING},            //60
  {TOKEN_SID,h_SID},       
  {TOKEN_POS,h_POS},
  {TOKEN_PAINT,h_PAINT},
  {TOKEN_GET,h_GET},
  {TOKEN_PUT,h_PUT},
  {TOKEN_UPDATE,h_UPDATE},
  {TOKEN_HIDE,h_HIDE},
  {TOKEN_GFXCLS,h_GFXCLS},
  {TOKEN_SPLOAD,h_SPLOAD},
  {TOKEN_TDRAW,h_TDRAW},             //70
  {TOKEN_SPRITE,h_SPRITE},
  {TOKEN_TSHEET,h_TSHEET},
  {TOKEN_SCROLL,h_SCROLL},
  {TOKEN_MAPLOAD,h_MAPLOAD},
  {TOKEN_SETSCROLL,h_SETSCROLL},
  {TOKEN_PTILE,h_PTILE},
  {TOKEN_ANIMATE,h_ANIMATE},
  {TOKEN_SETCHAR,h_SETCHAR},
  {TOKEN_MENU,h_MENU},      
  {TOKEN_MOUNT,h_MOUNT},             //80
  {TOKEN_UNMOUNT,h_UNMOUNT},   
  {TOKEN_THEME,h_THEME},
  {TOKEN_TEXT,h_TEXT},
  {TOKEN_HELP,h_HELP},      //letzter Basic-Befehl
//******************************************************  
  {TOKEN_RND, h_RND},       //erster Funktionsbefehl
  {TOKEN_SQR, h_SQR},
  {TOKEN_SIN, h_SIN},
  {TOKEN_COS, h_COS},
  {TOKEN_ABS, h_ABS},

  
  {0, NULL}
};

int print_keyword_table(const char* titel, Keyword tabelle[]) {
  println(titel);
  println();
  
  int spaltenZaehler = 0;
  int zaehler = 0;
  
  for (int i = 0; tabelle[i].name != NULL; i++) {
    x_pos = spaltenZaehler * 12; // Spalten-Position setzen
    print(tabelle[i].name);   
    
    spaltenZaehler++;
    if (spaltenZaehler >= 5) {
      println();
      spaltenZaehler = 0;
    }
    zaehler++;
  } 
  
  if (spaltenZaehler != 0) {
    println();
  }
  
  println("---------------------------------------------------------");
  print(zaehler);
  println(" Entries found.");
  
  return zaehler; 
}

void cmd_help() {
  bool tmpcur = cursor_on_off;
  char* buf;
  int i=0;
  int token;
  cursor_on_off = false;
  spaces();
  if(*txtpos =='\0'){
  drawCursor(cursor_on_off);

  print_keyword_table("-------------------- BASIC COMMANDS ---------------------", commands);
  println("--- Press any key for FUNCTIONS ---");
  if (wait_key(1) == 27) {
    cursor_on_off = tmpcur;
    drawCursor(cursor_on_off);
    return; 
  }
  println();
  print_keyword_table("-------------------- BASIC FUNCTIONS --------------------", functions);
  println("Enter 'HELP Command Name' for details.");
  }
  else {
    char* startOfToken = txtpos;  //Position merken
    token = getCommandToken();
    
    if ((token == TOKEN_ERROR && token == TOKEN_END) || token == TOKEN_VARIABLE) {
       txtpos = startOfToken;
       token = getFunctionToken();
       if (token == TOKEN_ERROR && token == TOKEN_END){
        println("no Command/Function found !");
       }else{
        print(cdetail_table[token].text);
        //println(token);
        
       }
    }
    else {
      //println(token);
      print(cdetail_table[token].text);
      }
    
  }
  cursor_on_off = tmpcur;
  drawCursor(cursor_on_off);
}
