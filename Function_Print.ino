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
  print((const char*)buf); //print(buf);
}


// Für Fließkommazahlen (float/double) - wichtig für BASIC
void print(double n) {
  char buf[32];
  // n, Vorkommastellen, Nachkommastellen, Puffer
  dtostrf(n, 4, 2, buf);
  print((const char*)buf); //print(buf);
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

void print(double n, int decimals) {
  char buf[32];
  dtostrf(n, 4, decimals, buf); // Hier wird decimals korrekt genutzt
  print((const char*)buf);      // Ruft deine String-Funktion auf
}

void println(double n, int decimals) {
  char buf[32];
  dtostrf(n, 4, decimals, buf); // Hier wird decimals korrekt genutzt
  print((const char*)buf);      // Ruft deine String-Funktion auf
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
  print((const char*)buf); //print(buf); // Ruft deine print(const char*) auf
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
  print((const char*)buf); //print(buf);
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

  print((const char*)buf); 
}
