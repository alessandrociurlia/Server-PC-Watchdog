// Auduino Watchdog
// Alessandro Ciurlia

#include <EEPROM.h> 

/*Definisco i parametri */
#define RESET_PIN 12 //pin usato per il reset
#define LED_PIN 13 //pin usato per il led (a scopo informativo) 
#define BUFFER_LENGTH 16 //lunghezza del vettore dove vengono appoggiati i dati in input
#define TRIGGERED_SERVER_BOOTED_COUNTER_EEPROM_ADDRESS 0 //indirizzo EEPROM del contatore di riavii innescati
#define SERVER_BOOTED_COUNTER_EEPROM_ADDRESS 4 //indirizzo EEPROM del contatore di riavvii effettivi
#define LONGEST_DELAY_EEPROM_ADDRESS 8 //indirizzo EEPROM del ritardo più lungo

/* Definisco le variabili globali */
char _inputBuffer[BUFFER_LENGTH]; //vettore dove vengono appoggiati i dati in input
byte _inputIndex = 0; //indice del vettore dove vengono appoggiati i dati in input
boolean _activated = false; //variabile che indica lo stato del whatchdog (attivo/disattivo)
boolean _allowReboot = false; //variabile che indica se è permesso il riavvio
unsigned long _timeout = 0; //tempo limite dopo il quale avviene il riavvio in caso di blocco
unsigned long _lastActivation = 0; //ultima volta che il watchdog è stato attivato
unsigned long _longestDelay = 0; //ritardo più lungo registrato

/* Funzione per leggere dalla EEPROM */
unsigned long readNumberFromEEPROM(int address) { 
  /* Leggo i byte dall'EEPROM*/
  byte b1 = EEPROM.read(address); 
  byte b2 = EEPROM.read(address+1); 
  byte b3 = EEPROM.read(address+2); 
  byte b4 = EEPROM.read(address+3); 
  /* Salvo i 4 byte letti in un unica variabile unsigned long (32bit=4byte) */
  unsigned long result = ((unsigned long)b1)<<24 | ((unsigned long)b2)<<16 |((unsigned long)b3)<<8 | (unsigned long)b4;  
  return result; 
}

/* Funzione per scrivere sulla EEPROM */
void writeNumberToEEPROM(unsigned long number, int address) { 
  /* Divido il numero a 32 bit che voglio scrivere in 4 byte */
  byte b1 = (byte)(number>>24); 
  byte b2 = (byte)(number>>16); 
  byte b3 = (byte)(number>>8); 
  byte b4 = (byte)number; 
  /* Scrivo i byte ottenuti sull EEPROM*/
  EEPROM.write(address, b1); 
  EEPROM.write(address+1, b2); 
  EEPROM.write(address+2, b3); 
  EEPROM.write(address+3, b4); 
}

/* Funzione che permette di aggiornare i dati dopo che il dispositivo si è riavviato */
void updateData() {
  delay(1000); 
  /* Aggiorno il ritardo più lungo */
  _longestDelay = readNumberFromEEPROM(LONGEST_DELAY_EEPROM_ADDRESS); 
  /* Aggiorno il numero di volte che il dispositivo si è riavviato */
  unsigned long n = readNumberFromEEPROM(SERVER_BOOTED_COUNTER_EEPROM_ADDRESS); 
  n++; 
  writeNumberToEEPROM(n, SERVER_BOOTED_COUNTER_EEPROM_ADDRESS); 
}

/* Funzione di inizializzazione */
void setup() {    
  /* Setto i pin */           
  pinMode(RESET_PIN, OUTPUT); 
  pinMode(LED_PIN, OUTPUT); 
  /* Setto la seriale */
  Serial.begin(115200); 
  /* Aggiorno i dati */
  updateData(); 
}

/* Funzione per attivare il whatchdog */
void activateWatchdog(int timeout) {
  /* Salvo il tempo limite dopo il quale riavviare in caso di blocco */
  if (timeout >= 10) { 
    unsigned long curTime = millis(); 
    _timeout = ((unsigned long)timeout) * 1000; 
    /* Se è già attivo salvo il nuovo ritardo più lungo */
    if (_activated) { 
      unsigned long duration = curTime - _lastActivation; 
      if (duration > _longestDelay) { 
        _longestDelay = duration; 
        writeNumberToEEPROM(_longestDelay, LONGEST_DELAY_EEPROM_ADDRESS);       
      }
    }
    /* Attivo il watchdog */
    _lastActivation = curTime; 
    _activated = true; 
  }
}

/* Funzione per disattivare il watchdog */
void deactivateWatchdog() {
  /* Disattivo il watchdog */
  _activated = false; 
}

/* Funzione per salvare sul vettore di appoggio definito globalmente il comando preso in input dal server */
char* getInput() {
  /* Salvo il comando nel vettore d'appoggio */
  int readChar = Serial.read(); 
  while(readChar > 0) { 
    char c = (char)readChar; 
    if (_inputIndex < BUFFER_LENGTH-2) { 
      if (c != '\r' && c!= '\n') 
        _inputBuffer[_inputIndex++] = c; 
    }
    /* Se il carattere letto è un'andata a capo fermo la lettura e verifico che il vettore non sia vuoto e solo in questo caso lo restituisco */
    if (c == '\n' || c == '\r') { 
      _inputBuffer[_inputIndex] = 0; 
      if (_inputIndex > 0) { 
        _inputIndex = 0; 
        return _inputBuffer; 
      }
    }
    readChar = Serial.read(); 
  }
  return 0; 
}

/* Funzione per processare il comando preso in input dal server */
void reactToInput(char* input) {
  /* Se il comando è diverso da 0 lo salvo su una stringa */
  if (input != 0) { 
    String str(input); 
    /* Se la stringa inizia con "data" invio al server i dati del watchdog */
    if (str.startsWith("data")) { 
      unsigned long time = millis(); 
      Serial.print ("Stato: ");
      if (_activated) { 
        if (!_allowReboot) {
          Serial.print("Falso "); 
        }
        Serial.print("Attivo");
        Serial.print(", Tempo limite di blocco: "); 
        Serial.print(_timeout); 
        Serial.print(" ms"); 
      } else { 
        Serial.print("Disattivo"); 
      }
      Serial.print(", Riavii innescati: "); 
      Serial.print(readNumberFromEEPROM(TRIGGERED_SERVER_BOOTED_COUNTER_EEPROM_ADDRESS)); 
      Serial.print(", Riavvi effettivi: "); 
      Serial.print(readNumberFromEEPROM(SERVER_BOOTED_COUNTER_EEPROM_ADDRESS)); 
      Serial.print(", Ritardo più lungo: "); 
      Serial.print(_longestDelay); 
      Serial.print(" ms"); 
      Serial.print(", Clock: "); 
      Serial.print(time); 
      Serial.println(" ms");
      Serial.println("");

    /* Se la stringa inizia con "act" ed è specificato il tempo limite di blocco attivo il watchdog con il permesso di riavvio */
    } else if (str.length() >= 5 && str.startsWith("act ")) {  
      String strNum = str.substring(4); 
      int num = strNum.toInt(); 
      _allowReboot = true; 
      activateWatchdog(num); 

    /* Se la stringa inizia con "fakeact" ed è specificato il tempo limite di blocco attivo il watchdog senza il permesso di riavvio */
    } else if (str.length() >= 9 && str.startsWith("fakeact ")) { 
      String strNum = str.substring(8); 
      int num = strNum.toInt(); 
      _allowReboot = false; 
      activateWatchdog(num); 

    /* Se la stringa inizia con "deact" disattivo il watchdog */
    } else if (str.startsWith("deact")) { 
      _allowReboot = false; 
      activateWatchdog(10); 
      deactivateWatchdog(); 

     /* Se la stringa inizia con "clear" pulisco la memoria EEPROM */
     } else if (str.startsWith("clear")) { 
      writeNumberToEEPROM(0, TRIGGERED_SERVER_BOOTED_COUNTER_EEPROM_ADDRESS); 
      writeNumberToEEPROM(0, SERVER_BOOTED_COUNTER_EEPROM_ADDRESS); 
      _longestDelay = 0; 
      writeNumberToEEPROM(_longestDelay, LONGEST_DELAY_EEPROM_ADDRESS); 
    }
  }
}

/* Funzione operativa del watchdog */
void watchdog() {
  /* Se il watchdog è attivo calcolo la durata tra una risposta e l'altra del server */
  if (_activated) { 
    unsigned long time = millis(); 
    unsigned long duration = time - _lastActivation; 
    /* Se non è permesso il riavvio faccio lampeggiare il led*/
    if (!_allowReboot) { 
      bool ledflash = time%100 < 50; 
      if (ledflash) 
        digitalWrite(LED_PIN, LOW); 
      else 
        digitalWrite(LED_PIN, HIGH);
    /* altimenti lo lascio fisso acceso */
    } else { 
      digitalWrite(LED_PIN, HIGH); 
    }
    /* Se la durata tra una risposta e l'altra è maggiore del tempo limite di blocco allora disattivo il watchdog */
    if (duration > _timeout) { 
      deactivateWatchdog(); 
      /* Aggiorno il numero di riavvii innescati */
      unsigned long numReboots = readNumberFromEEPROM(TRIGGERED_SERVER_BOOTED_COUNTER_EEPROM_ADDRESS);
      numReboots++; 
      writeNumberToEEPROM(numReboots, TRIGGERED_SERVER_BOOTED_COUNTER_EEPROM_ADDRESS); 
      /* Riavvio il server se è permesso */
      if (_allowReboot) { 
        digitalWrite(RESET_PIN, HIGH); 
        delay(1000); 
        digitalWrite(RESET_PIN, LOW);
      }
    }
  /* Se i watchdog non è attivo lascio il led spento */
  } else { 
    digitalWrite(LED_PIN, LOW); 
  }
}

/* Funzione iterativa */
void loop() {
  /* Ottengo l'input */
  char* input = getInput(); 
  /* Lo processo */
  reactToInput(input); 
  /* Lancio la funzione operativa del watchdog */
  watchdog(); 
}
