//------------------------------------------------- FILE_RD und FILE_WR --------------------------------------------------------------
int File_Operations(void) {
  char c;
  char *a;                                // Array-Deklaration korrigiert
  int i;
  long ps;
  char *s;
  char *tempstring;
  if (Test_char('_')) return 1;                       // Unterstrich für folgenden Befehlsbuchstaben
  c = *txtpos;                                        // Befehlsbuchstabe lesen
  txtpos++;

  switch (c) {

    case 'O':                                         // FILE_OP -> Open File
      if (*txtpos != 'P') return 1;
      txtpos++;

      fileNameStr = parseStringExpression();

      if (fp) {                                                                   // Bereits geöffnete Datei sicherheitshalber schließen
        fp.close();
        File_pos = 0;
      }
      if (*txtpos == ',') txtpos++;
      spaces();

      if (*txtpos == 'R') {
        txtpos++; // Das 'R' überspringen
        fp = SD.open(fileNameStr.c_str(), FILE_READ); // Datei lesen
      }
      else if (*txtpos == 'W') {
        txtpos++; // Das 'W' überspringen
        fp = SD.open(fileNameStr.c_str(), FILE_WRITE); // Datei schreiben
      }
      else {
        syntaxerror(syntaxmsg);
        return 1; // Jetzt wird das 'return 1' korrekt nur im Fehlerfall ausgeführt
      }


      //spaces();

      if (!fp) {
        syntaxerror(sdfilemsg);
        return 1;
      }
      File_pos = fp.position();
      File_size = fp.size();
      break;

    case 'C':                                         // FILE_CL -> Close File
      if (*txtpos == 'L') {
        txtpos++;
        spaces();
      }

      if (fp) {
        fp.close();
        File_pos = 0;
      }
      break;

    case 'R':                                         // FILE_RD -> read from File
      if (*txtpos == 'D') {
        txtpos++;
        spaces();
      }

      i = 0;
      char Buffer[BUF_SIZE];
      memset(Buffer, 0, BUF_SIZE);
      
      while (fp.available() && i < 255) {
        char fileChar = fp.read();
        if (fileChar == '\r') continue;               // Windows-Zeilenumbruch ignorieren
        if (fileChar == '\n') break;                  // Zeilenende erreicht
        Buffer[i++] = fileChar;
      }
      
      Buffer[i] = '\0';
      s = strtok(Buffer, ";");

      i = 1;
      while (s != NULL) {                             // Ersten Wert in die im BASIC-Code angegebene Variable schreiben
        if (File_line(s)) {                           //hier steht der aktuell gelesene Wert - bei Fehler, Datei schließen und raus
          fp.close();
          return 1;
        }
        s = strtok(NULL, ";");                        //zum nächsten ; springen
      }
      File_pos = fp.position();                        //Position innerhalb der Datei merken
      break;

    case 'W':                                         // FILE_WR -> write in File
      if (*txtpos == 'R') {
        txtpos++;
        //spaces();
      }

      while (1) {
        const char* checkpoint = txtpos;
        int ft = getFunctionToken();
        // Prüfen, ob das Token ein String-Typ ist
        bool isString = (ft == TOKEN_STRING_LITERAL) ||
                        (ft == TOKEN_VARIABLE && isStringVar) || ft == TOKEN_STRING ||
                        (ft == TOKEN_LEFT || ft == TOKEN_MID || ft == TOKEN_RIGHT || ft == TOKEN_SPC ||
                         ft == TOKEN_STR || ft == TOKEN_CHR || ft == TOKEN_TIME || ft == TOKEN_DATE);

        txtpos = checkpoint; // Zurücksetzen, damit parseStringExpression oder get_value() von vorne lesen

        if (isString) {
          String sval = parseStringExpression();
          fp.print(sval);

        } else {                                      // --- 2. NUMERISCHE AUSDRÜCKE (inkl. HEX/BIN/OCT) ---
          double val = get_value();
          fp.print(val);                              // Wert in Datei schreiben
        }
        spaces();
        if (*txtpos == ',') {
          txtpos++;
          fp.print(";");                              // Trennzeichen auf SD-Karte schreiben
        } else {
          break;                                      // Keine weiteren Argumente
        }
      }
      fp.print('\n');                                 // Zeilenumbruch am Ende der Zeile (0x0A)
      fp.flush();                                     // Sofort auf SD-Karte speichern
      File_pos = fp.position();
      File_size = fp.size();
      break;

    case 'P':                                         // FILE_PS -> set Pos in File
      if (Test_char('S')) return 1;
      if (Test_char('(')) return 1;
      ps = long(get_value());
      if (Test_char(')')) return 1;

      if (fp && ps <= (long)fp.size()) {
        fp.seek(ps);                                  // Position auf der SD-Karte setzen
      }
      break;

    default:
      return 1; // Unbekannter Befehl
  }

  return 0;
}


int File_line(char *vals) {
  if (vals == NULL) return 1;
  //spaces();
  if (*txtpos < 'A' || *txtpos > 'Z')                                     //erster Variablenbuchstabe
  {
    syntaxerror(syntaxmsg);
    return 1;
  }
  int t = getCommandToken();
  if (t != TOKEN_VARIABLE) {
    syntaxerror(valmsg);
    isRunning = false;
    return 1;
  }

  // Indizes sichern (z.B. für O und X)
  int targetV1 = vIdx1;
  int targetV2 = vIdx2;
  bool isStr = isStringVar;
  double val = 0;
  String sVal = "";

  if (isStr) sVal = String(vals);
  else {
    val = strtod(vals, NULL);
  }

  if (*txtpos == '(') {
    txtpos++; // '(' überspringen
    int x = (int)get_value(); if (*txtpos == ',') txtpos++;
    int y = (int)get_value(); if (*txtpos == ',') txtpos++;
    int z = (int)get_value(); if (*txtpos == ')') txtpos++;

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
  spaces();
  if (*txtpos == ',') {
    txtpos++;
  }
  return 0;
}

//------------------------------------------------------- Befehl TYPE -------------------------------------------------------------------------------------
  void type_file(void) {
  char c, d;
  int b, ex = 0;
  fileNameStr = parseStringExpression();

  if (!SD.exists(fileNameStr.c_str())) {  //Datei nicht vorhanden
    syntaxerror(sdfilemsg);
    return;
  }
  fp = SD.open(fileNameStr.c_str(), FILE_READ);

  while (fp.available()) {
    c = fp.read();
    if (c == '\r') continue;
    if (c == '\n') {
      println();
      b++;
      continue;
    }
    if (b == 40)
    {
      if (wait_key(true) == 27) break;
      b = 0;
    }
    print(c);
  }
  fp.close();
  }
