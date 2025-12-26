#include <RH_ASK.h>
#include <SPI.h>

// ------------------------------------------------------------------
// CONFIGURATION RELAIS
// ------------------------------------------------------------------
const int RELAIS_COUNT = 4;
const int relaisPins[RELAIS_COUNT] = {6, 7, 8, 9};

// Relais actifs HIGH
const bool RELAY_ACTIVE_HIGH = true;

// Mapping bouton → relais (1 = contrôle, 0 = ignore)
bool mapping[4][4] = {
  {1,1,0,0}, // B1 → R1
  {0,1,0,0}, // B2 → R2
  {0,0,1,0}, // B3 → R3
  {0,0,0,1}  // B4 → R4
};

// ------------------------------------------------------------------
// ÉTATS PAR DÉFAUT DES RELAIS
// ------------------------------------------------------------------
// true = ON par défaut, false = OFF par défaut
bool defaultRelayState[RELAIS_COUNT] = {
  false,  // R1
  false,  // R2
  false,  // R3
  true    // R4 (exemple : B4 → R4 par défaut ON)
};

// ------------------------------------------------------------------
// MODES PAR BOUTON
// ------------------------------------------------------------------
enum Mode {
  TOGGLE,
  MOMENTANE,
  TEMPORISE_EXCITATION,
  TEMPORISE_DESEXCITATION
};

Mode modeBouton[4] = {
  TOGGLE,                    // B1
  MOMENTANE,                 // B2
  TEMPORISE_EXCITATION,      // B3
  TEMPORISE_DESEXCITATION    // B4
};

// Durées des temporisations (ms)
unsigned long tempoBouton[4] = {
  0,     // B1
  0,     // B2
  2000,  // B3 = 2s
  5000   // B4 = 5s
};

// Gestion interne des temporisations
unsigned long tempoStart[4] = {0,0,0,0};
bool tempoActive[4] = {false,false,false,false};

// Mémoire d’état de la bobine (commande)
bool lastState[4] = {false,false,false,false};

// ------------------------------------------------------------------
// RADIOHEAD
// ------------------------------------------------------------------
RH_ASK driver;

char buffer[48];
uint8_t buflen = 48;

unsigned int lastSeqProcessed = 0;
bool hasLastSeq = false;

const char* ID_AUTORISE = "TX1";

// ------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  if (!driver.init()) {
    Serial.println("Erreur init RH_ASK");
  }

  // Init relais selon état par défaut
  for (int i = 0; i < RELAIS_COUNT; i++) {
    pinMode(relaisPins[i], OUTPUT);
    setRelay(i, defaultRelayState[i]);
  }

  Serial.println("Récepteur prêt !");
}

// ------------------------------------------------------------------
void setRelay(int index, bool on) {
  if (index < 0 || index >= RELAIS_COUNT) return;

  if (RELAY_ACTIVE_HIGH)
    digitalWrite(relaisPins[index], on ? HIGH : LOW);
  else
    digitalWrite(relaisPins[index], on ? LOW : HIGH);
}

void toggleRelay(int r) {
  bool current = digitalRead(relaisPins[r]);
  setRelay(r, !current);
}

// ------------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // ----------------------------------------------------------
  // GESTION DES TEMPORISATIONS
  // ----------------------------------------------------------
  for (int b = 0; b < 4; b++) {
    if (tempoActive[b]) {
      if (now - tempoStart[b] >= tempoBouton[b]) {

        for (int r = 0; r < RELAIS_COUNT; r++) {
          if (mapping[b][r]) {

            if (modeBouton[b] == TEMPORISE_EXCITATION) {
              setRelay(r, defaultRelayState[r]); // retour à l’état par défaut
            }

            else if (modeBouton[b] == TEMPORISE_DESEXCITATION) {
              setRelay(r, defaultRelayState[r]); // retour à l’état par défaut
            }
          }
        }

        tempoActive[b] = false;
      }
    }
  }

  // ----------------------------------------------------------
  // RÉCEPTION RADIO
  // ----------------------------------------------------------
  buflen = sizeof(buffer);
  if (!driver.recv((uint8_t*)buffer, &buflen)) return;

  buffer[buflen] = '\0';
  Serial.print("Message reçu: ");
  Serial.println(buffer);

  // Parse ID|B<n>|ON/OFF|seq
  char* id = strtok(buffer, "|");
  char* bouton = strtok(NULL, "|");
  char* action = strtok(NULL, "|");
  char* seqStr = strtok(NULL, "|");

  if (!id || !bouton || !action) return;
  if (strcmp(id, ID_AUTORISE) != 0) return;

  if (bouton[0] != 'B' && bouton[0] != 'b') return;

  int btnIndex = bouton[1] - '1';
  if (btnIndex < 0 || btnIndex >= 4) return;

  unsigned int seq = seqStr ? atoi(seqStr) : 0;
  if (seq != 0 && hasLastSeq && seq == lastSeqProcessed) return;
  lastSeqProcessed = seq;
  hasLastSeq = true;

  bool isOn = (strcasecmp(action, "ON") == 0);

  // ----------------------------------------------------------
  // DÉTECTION EXCITATION / DÉSÉXCITATION
  // ----------------------------------------------------------
  bool wasOn = lastState[btnIndex];

  bool excitation    = (!wasOn && isOn);   // OFF → ON
  bool desexcitation = (wasOn && !isOn);   // ON → OFF

  lastState[btnIndex] = isOn;

  // ----------------------------------------------------------
  // TRAITEMENT SELON MODE
  // ----------------------------------------------------------
  switch (modeBouton[btnIndex]) {

    // -------------------------
    // MODE TOGGLE
    // -------------------------
    case TOGGLE:
      if (excitation) {
        for (int r = 0; r < RELAIS_COUNT; r++) {
          if (mapping[btnIndex][r]) toggleRelay(r);
        }
      }
      break;

    // -------------------------
    // MODE MOMENTANE
    // -------------------------
    case MOMENTANE:
      for (int r = 0; r < RELAIS_COUNT; r++) {
        if (mapping[btnIndex][r]) setRelay(r, isOn);
      }
      break;

    // -------------------------
    // TEMPORISATION À L’EXCITATION
    // OFF → ON : ON pendant tempo puis retour état défaut
    // -------------------------
    case TEMPORISE_EXCITATION:
      if (excitation) {
        tempoStart[btnIndex] = now;
        tempoActive[btnIndex] = true;

        for (int r = 0; r < RELAIS_COUNT; r++) {
          if (mapping[btnIndex][r]) {
            setRelay(r, true); // ON pendant tempo
          }
        }
      }
      break;

    // -------------------------
    // TEMPORISATION À LA DÉSÉXCITATION
    // OFF → ON : OFF pendant tempo puis retour état défaut
    // -------------------------
    case TEMPORISE_DESEXCITATION:
      if (excitation) { // tu voulais OFF→ON
        tempoStart[btnIndex] = now;
        tempoActive[btnIndex] = true;

        for (int r = 0; r < RELAIS_COUNT; r++) {
          if (mapping[btnIndex][r]) {
            setRelay(r, false); // OFF pendant tempo
          }
        }
      }
      break;
  }
}
