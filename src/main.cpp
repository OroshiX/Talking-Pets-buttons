/*
  Talking Pet Buttons - Arduino GIGA R1 WiFi

  Hardware:
  - Each button is a passive normally-open switch.
  - Wire one side of each switch to one Arduino input pin.
  - Wire the other side of each switch to GND.
  - The sketch uses INPUT_PULLUP, so a pressed button reads LOW.
  - WAV files live on a FAT32 USB stick connected to the GIGA USB-A port.
  - Audio goes from the GIGA 3.5 mm jack to an amplified AUX speaker.

  Audio files:
  - Use 16-bit PCM mono WAV files.
  - Keep file names short, uppercase, ASCII, and without accents.
*/

#include <Arduino.h>
#include <Arduino_AdvancedAnalog.h>
#include <DigitalOut.h>
#include <Arduino_USBHostMbed5.h>
#include <FATFileSystem.h>

#include <cerrno>
#include <cstring>

struct ButtonConfig {
  uint8_t pin;
  const char *word;
  const char *filePath;
};

const ButtonConfig kButtons[] = {
  {2, "manger", "/usb/MANGER.WAV"},
  {3, "eau", "/usb/EAU.WAV"},
  {4, "jouer", "/usb/JOUER.WAV"},
  {5, "dehors", "/usb/DEHORS.WAV"},
  {6, "dedans", "/usb/DEDANS.WAV"},
  {7, "calin", "/usb/CALIN.WAV"},
  {8, "brosser", "/usb/BROSSER.WAV"},
  {9, "dormir", "/usb/DORMIR.WAV"},
  {10, "oui", "/usb/OUI.WAV"},
  {11, "non", "/usb/NON.WAV"},
  {12, "aide", "/usb/AIDE.WAV"},
  {13, "encore", "/usb/ENCORE.WAV"},
  {14, "fini", "/usb/FINI.WAV"},
  {15, "litiere", "/usb/LITIERE.WAV"},
  {16, "friandise", "/usb/FRIAND.WAV"},
  {17, "ouvrir", "/usb/OUVRIR.WAV"},
  {18, "voir", "/usb/VOIR.WAV"},
  {19, "venir", "/usb/VENIR.WAV"},
  {20, "mal", "/usb/MAL.WAV"},
  {21, "content", "/usb/CONTENT.WAV"},
  {22, "peur", "/usb/PEUR.WAV"},
  {23, "toi", "/usb/TOI.WAV"},
  {24, "moi", "/usb/MOI.WAV"},
  {25, "maintenant", "/usb/MAINT.WAV"},
};

constexpr uint8_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);
constexpr unsigned long kDebounceMs = 35;
constexpr unsigned long kCooldownMs = 800;
constexpr uint8_t kQueueSize = 8;
constexpr size_t kSamplesPerBuffer = 256;
constexpr uint16_t kDacMidpoint = 2048;

struct ButtonState {
  bool lastRawPressed;
  bool stablePressed;
  unsigned long lastRawChangeMs;
  unsigned long lastTriggerMs;
};

ButtonState buttonStates[kButtonCount];

uint8_t audioQueue[kQueueSize];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;

AdvancedDAC dacRight(A12);
AdvancedDAC dacLeft(A13);
USBHostMSD msd;
mbed::FATFileSystem usb("usb");

FILE *audioFile = nullptr;
bool usbMounted = false;
bool dacStarted = false;
bool audioPlaying = false;
uint8_t currentButton = 255;
uint32_t bytesRemaining = 0;
uint16_t currentSampleSize = 0;

struct WavInfo {
  uint16_t audioFormat;
  uint16_t channels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint32_t dataSize;
};

bool readExact(FILE *file, void *destination, size_t byteCount) {
  return fread(destination, 1, byteCount, file) == byteCount;
}

bool readU16(FILE *file, uint16_t &value) {
  uint8_t bytes[2];
  if (!readExact(file, bytes, sizeof(bytes))) {
    return false;
  }

  value = static_cast<uint16_t>(bytes[0]) |
          (static_cast<uint16_t>(bytes[1]) << 8);
  return true;
}

bool readU32(FILE *file, uint32_t &value) {
  uint8_t bytes[4];
  if (!readExact(file, bytes, sizeof(bytes))) {
    return false;
  }

  value = static_cast<uint32_t>(bytes[0]) |
          (static_cast<uint32_t>(bytes[1]) << 8) |
          (static_cast<uint32_t>(bytes[2]) << 16) |
          (static_cast<uint32_t>(bytes[3]) << 24);
  return true;
}

bool skipBytes(FILE *file, const uint32_t byteCount) {
  return fseek(file, byteCount, SEEK_CUR) == 0;
}

bool readWavHeader(FILE *file, WavInfo &info) {
  char riffId[4];
  char waveId[4];
  uint32_t riffSize = 0;

  if (!readExact(file, riffId, sizeof(riffId)) ||
      !readU32(file, riffSize) ||
      !readExact(file, waveId, sizeof(waveId))) {
    return false;
  }

  if (memcmp(riffId, "RIFF", 4) != 0 || memcmp(waveId, "WAVE", 4) != 0) {
    return false;
  }

  bool foundFormat = false;

  while (!feof(file)) {
    char chunkId[4];
    uint32_t chunkSize = 0;

    if (!readExact(file, chunkId, sizeof(chunkId)) || !readU32(file, chunkSize)) {
      return false;
    }

    if (memcmp(chunkId, "fmt ", 4) == 0) {
      uint32_t byteRate = 0;
      uint16_t blockAlign = 0;

      if (chunkSize < 16 ||
          !readU16(file, info.audioFormat) ||
          !readU16(file, info.channels) ||
          !readU32(file, info.sampleRate) ||
          !readU32(file, byteRate) ||
          !readU16(file, blockAlign) ||
          !readU16(file, info.bitsPerSample)) {
        return false;
      }

      if (chunkSize > 16 && !skipBytes(file, chunkSize - 16)) {
        return false;
      }

      foundFormat = true;
    } else if (memcmp(chunkId, "data", 4) == 0) {
      if (!foundFormat) {
        return false;
      }

      info.dataSize = chunkSize;
      return true;
    } else {
      if (!skipBytes(file, chunkSize)) {
        return false;
      }
    }

    if ((chunkSize & 1) && !skipBytes(file, 1)) {
      return false;
    }
  }

  return false;
}

void printButtonMap() {
  Serial.println("Button map:");
  for (const auto kButton : kButtons) {
    Serial.print("  D");
    Serial.print(kButton.pin);
    Serial.print(" -> ");
    Serial.print(kButton.word);
    Serial.print(" -> ");
    Serial.println(kButton.filePath);
  }
}

bool enqueueAudio(uint8_t buttonIndex) {
  if (queueCount >= kQueueSize) {
    Serial.print("Audio queue full, ignored: ");
    Serial.println(kButtons[buttonIndex].word);
    return false;
  }

  audioQueue[queueTail] = buttonIndex;
  queueTail = (queueTail + 1) % kQueueSize;
  queueCount++;
  return true;
}

bool dequeueAudio(uint8_t &buttonIndex) {
  if (queueCount == 0) {
    return false;
  }

  buttonIndex = audioQueue[queueHead];
  queueHead = (queueHead + 1) % kQueueSize;
  queueCount--;
  return true;
}

void configureButtons() {
  const unsigned long now = millis();

  for (uint8_t i = 0; i < kButtonCount; i++) {
    pinMode(kButtons[i].pin, INPUT_PULLUP);
    buttonStates[i].lastRawPressed = false;
    buttonStates[i].stablePressed = false;
    buttonStates[i].lastRawChangeMs = now;
    buttonStates[i].lastTriggerMs = 0;
  }
}

void scanButtons() {
  const unsigned long now = millis();

  for (uint8_t i = 0; i < kButtonCount; i++) {
    const bool rawPressed = digitalRead(kButtons[i].pin) == LOW;
    ButtonState &state = buttonStates[i];

    if (rawPressed != state.lastRawPressed) {
      state.lastRawPressed = rawPressed;
      state.lastRawChangeMs = now;
    }

    if ((now - state.lastRawChangeMs) < kDebounceMs) {
      continue;
    }

    if (rawPressed == state.stablePressed) {
      continue;
    }

    state.stablePressed = rawPressed;

    if (!state.stablePressed) {
      continue;
    }

    if ((now - state.lastTriggerMs) < kCooldownMs) {
      continue;
    }

    state.lastTriggerMs = now;
    Serial.print("Pressed: ");
    Serial.println(kButtons[i].word);
    enqueueAudio(i);
  }
}

bool mountUsbStick() {
  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);

  Serial.println("Waiting for USB stick on the GIGA USB-A port...");
  while (!msd.connect()) {
    scanButtons();
    delay(100);
  }

  Serial.println("Mounting USB stick...");
  const int mountResult = usb.mount(&msd);
  if (mountResult != 0) {
    Serial.print("USB mount failed: ");
    Serial.println(mountResult);
    return false;
  }

  Serial.println("USB stick mounted as /usb");
  return true;
}

void stopDac() {
  if (!dacStarted) {
    return;
  }

  dacRight.stop();
  dacLeft.stop();
  dacStarted = false;
}

bool startDac(uint32_t sampleRate) {
  stopDac();
  delay(20);

  const bool rightOk = dacRight.begin(AN_RESOLUTION_12, sampleRate, kSamplesPerBuffer, 16);
  const bool leftOk = dacLeft.begin(AN_RESOLUTION_12, sampleRate, kSamplesPerBuffer, 16);

  if (!rightOk || !leftOk) {
    Serial.println("Failed to start DAC output");
    stopDac();
    return false;
  }

  dacStarted = true;
  return true;
}

void closeCurrentFile() {
  if (audioFile != nullptr) {
    fclose(audioFile);
    audioFile = nullptr;
  }
}

void finishCurrentSound() {
  closeCurrentFile();
  audioPlaying = false;
  currentButton = 255;
  bytesRemaining = 0;
  currentSampleSize = 0;
  stopDac();
}

bool startSound(uint8_t buttonIndex) {
  if (!usbMounted) {
    Serial.println("Cannot play sound: USB stick is not mounted");
    return false;
  }

  closeCurrentFile();

  const ButtonConfig &button = kButtons[buttonIndex];
  Serial.print("Opening: ");
  Serial.println(button.filePath);

  audioFile = fopen(button.filePath, "rb");
  if (audioFile == nullptr) {
    Serial.print("File open failed: ");
    Serial.println(strerror(errno));
    return false;
  }

  WavInfo wav = {};
  if (!readWavHeader(audioFile, wav)) {
    Serial.println("Invalid or unsupported WAV header");
    closeCurrentFile();
    return false;
  }

  if (wav.audioFormat != 1 || wav.channels != 1 || wav.bitsPerSample != 16) {
    Serial.println("Unsupported WAV. Use 16-bit PCM mono files.");
    closeCurrentFile();
    return false;
  }

  if (!startDac(wav.sampleRate)) {
    closeCurrentFile();
    return false;
  }

  currentSampleSize = wav.bitsPerSample / 8;
  bytesRemaining = wav.dataSize;
  currentButton = buttonIndex;
  audioPlaying = true;

  Serial.print("Playing: ");
  Serial.print(button.word);
  Serial.print(" at ");
  Serial.print(wav.sampleRate);
  Serial.println(" Hz");

  return true;
}

void streamCurrentSound() {
  if (!audioPlaying || audioFile == nullptr) {
    return;
  }

  if (!dacRight.available() || !dacLeft.available()) {
    return;
  }

  uint16_t sampleData[kSamplesPerBuffer] = {0};
  const auto samplesLeft = static_cast<size_t>(bytesRemaining / currentSampleSize);
  const size_t samplesWanted = samplesLeft < kSamplesPerBuffer ? samplesLeft : kSamplesPerBuffer;
  const size_t samplesRead = fread(sampleData, currentSampleSize, samplesWanted, audioFile);

  SampleBuffer rightBuffer = dacRight.dequeue();
  SampleBuffer leftBuffer = dacLeft.dequeue();

  for (size_t i = 0; i < rightBuffer.size(); i++) {
    uint16_t dacValue = kDacMidpoint;

    if (i < samplesRead) {
      dacValue = ((static_cast<unsigned int>(sampleData[i]) + 32768) >> 4) & 0x0fff;
    }

    rightBuffer[i] = dacValue;
    leftBuffer[i] = dacValue;
  }

  dacRight.write(rightBuffer);
  dacLeft.write(leftBuffer);

  bytesRemaining -= samplesRead * currentSampleSize;

  if (samplesRead < samplesWanted || bytesRemaining == 0) {
    Serial.print("Finished: ");
    Serial.println(kButtons[currentButton].word);
    finishCurrentSound();
  }
}

void startNextQueuedSound() {
  if (audioPlaying) {
    return;
  }

  uint8_t nextButton = 0;
  while (dequeueAudio(nextButton)) {
    if (startSound(nextButton)) {
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("Talking Pet Buttons - Arduino GIGA R1 WiFi");

  configureButtons();
  printButtonMap();

  usbMounted = mountUsbStick();
  if (!usbMounted) {
    Serial.println("USB mount failed. Restart the board after fixing the USB stick.");
  }
}

void loop() {
  scanButtons();
  startNextQueuedSound();
  streamCurrentSound();
}
