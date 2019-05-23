#define INPUT_DATA_PIN 12
#define INPUT_MODE_PIN 10
#define CLOCK_PIN 9
#define OUTPUT_DATA_PIN 13
#define OUTPUT_LATCH_PIN 11

enum TubeStatus
{
    Empty = 0,
    Torpedo = 1,
    Mine = 2
};

// These structs' format must be kept in sync with the corresponding structs in the
// code that runs on the computer
struct Control {
  uint8_t tubeArmed[5];
  uint8_t tubeLoadTorpedo[5];
  uint8_t tubeLoadMine[5];
  uint32_t debugValue;
} __attribute__((packed));

struct Display {
  uint8_t tubeOccupancy[5];
} __attribute__((packed));

Control cont;
Display disp;

void setup() {
  Serial.begin(115200);
  pinMode(INPUT_DATA_PIN, INPUT);
  pinMode(INPUT_MODE_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(OUTPUT_DATA_PIN, OUTPUT);
  pinMode(OUTPUT_LATCH_PIN, OUTPUT);
}

void loop() {
  switchTest();
  /*
  receiveInput();

  // hack for testing
  disp.tubeOccupancy[1] = Torpedo;
  disp.tubeOccupancy[1] = Mine;
  disp.tubeOccupancy[4] = Torpedo;

  uint16_t switchValues;
  shiftRegisters(prepareShiftDisplay(Torpedo), &switchValues);
  delay(5);
  shiftRegisters(prepareShiftDisplay(Mine), &switchValues);
  delay(5);
  cont.debugValue = switchValues;
  
  sendOutput();
  */
}

void switchTest() {

  uint16_t switchValues;
  
  for (uint8_t i = 0; i < 5; i++) {
    disp.tubeOccupancy[i] = Torpedo;
    
    for (uint16_t d = 0; d < 200; d++) {
        shiftRegisters(prepareShiftDisplay(Torpedo), &switchValues);
        delay(5);
        shiftRegisters(prepareShiftDisplay(Mine), &switchValues);
        delay(5);
        cont.debugValue = switchValues;
        sendOutput();
    }

    disp.tubeOccupancy[i] = Mine;
    
    for (uint16_t d = 0; d < 200; d++) {
        shiftRegisters(prepareShiftDisplay(Torpedo), &switchValues);
        delay(5);
        shiftRegisters(prepareShiftDisplay(Mine), &switchValues);
        delay(5);
        cont.debugValue = switchValues;
        sendOutput();
    }

    disp.tubeOccupancy[i] = Empty;
  }
}

void shiftRegisters(uint16_t output, uint16_t *input) {
  *input = 0;
  digitalWrite(INPUT_MODE_PIN, LOW);
  digitalWrite(INPUT_MODE_PIN, HIGH);
  for (int i = 0; i < 16; i++) {
    digitalWrite(OUTPUT_DATA_PIN, (output & 0x0001) ? HIGH : LOW);
    if (digitalRead(INPUT_DATA_PIN) == HIGH) {
      *input |= 0x0001;
    }
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
    output >>= 1;
    *input <<= 1;
  }
  digitalWrite(OUTPUT_LATCH_PIN, HIGH);
  digitalWrite(OUTPUT_LATCH_PIN, LOW);
}

uint16_t prepareShiftDisplay(TubeStatus type) {
  uint16_t s = 0;

  /* High side */
  if (disp.tubeOccupancy[0] != type) s |= (1 << 15);
  if (disp.tubeOccupancy[1] != type) s |= (1 << 13);
  if (disp.tubeOccupancy[2] != type) s |= (1 << 14);
  if (disp.tubeOccupancy[3] != type) s |= (1 << 12);
  if (disp.tubeOccupancy[4] != type) s |= (1 << 8);

  /* Low side */
  if (type == Torpedo) s |= (1 << 11);
  if (type == Mine) s |= (1 << 10);

  return s;
}

/* I/O code for talking to the computer */

static const int inputBufSize = sizeof(Display) + 1;
uint8_t inputBuf[inputBufSize];
int inputPos = 0; // index into inputBuf measured in *nibbles*!
uint8_t inputChecksum = 0;

int parseHex(int character) {
  if (character >= '0' && character <= '9') {
    return character - '0';
  } else if (character >= 'a' && character <= 'f') {
    return 10 + character - 'a';
  } else if (character >= 'A' && character <= 'F') {
    return 10 + character - 'A';
  } else {
    return -1;
  }
}

void receiveInput() {
  while (true) {
    int c = Serial.read();
    if (c == -1) {
      return;
    } else if (c == '[') {
      inputPos = 0;
      inputChecksum = 0;
    } else if (c == ']') {
      if (inputPos == inputBufSize * 2 && inputChecksum == 0) {
        memcpy(&disp, inputBuf, sizeof(Display));
      }
      inputPos = inputBufSize * 2;
    } else {
      int h = parseHex(c);
      if (h == -1 || inputPos == inputBufSize * 2) {
        inputPos = inputBufSize * 2;
        continue;
      }
      int bytePos = inputPos >> 1;
      int nibblePos = inputPos & 0x01;
      if (nibblePos == 0) {
        inputBuf[bytePos] = h << 4;
      } else {
        inputBuf[bytePos] |= h;
        inputChecksum ^= inputBuf[bytePos];
      }
      inputPos += 1;
    }
  }
}

int generateHex(int value) {
  if (value < 10) {
    return '0' + value;
  } else {
    return 'A' + (value - 10);
  }
}

void sendOutput() {
  Serial.write('[');
  uint8_t checksum = 0;
  const uint8_t *outputBuf = (const uint8_t *)&cont;
  for (int i = 0; i < sizeof(Control); ++i) {
    Serial.write(generateHex((outputBuf[i] >> 4) & 0x0F));
    Serial.write(generateHex(outputBuf[i] & 0x0F));
    checksum ^= outputBuf[i];
  }
  Serial.write(generateHex((checksum >> 4) & 0x0F));
  Serial.write(generateHex(checksum & 0x0F));
  Serial.write(']');
}
