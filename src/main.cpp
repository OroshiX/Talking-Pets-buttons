/*
  Talking Pet Buttons - audio-only portable listener for Arduino GIGA R1 WiFi

  Hardware V1:
  - MAX4466-style analog microphone module powered from 3V3.
  - Microphone OUT connected to A0.
  - USB stick in the GIGA USB-A port for config, templates, logs and queue.
  - Android hotspot for Wi-Fi and ntfy notifications.

  This firmware does not connect to the pet buttons. It learns the sound of
  each independent recordable button and recognizes future presses by comparing
  compact audio fingerprints.
*/

#include <Arduino.h>
#include <Arduino_AdvancedAnalog.h>
#include <Arduino_USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>
#include <WiFi.h>

#include <cerrno>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <time.h>

namespace {

constexpr const char *kFirmwareName = "Talking Pet Buttons audio listener";
constexpr const char *kFirmwareVersion = "1.0.0";

constexpr pin_size_t kMicPin = A0;
constexpr uint8_t kCalibrationButtonPin = 22;
constexpr uint8_t kNextButtonPin = 23;
constexpr uint8_t kPreviousButtonPin = 24;
constexpr uint8_t kBinaryLedPins[] = {25, 26, 27, 28, 29, 30};
constexpr uint8_t kBinaryLedCount = sizeof(kBinaryLedPins) / sizeof(kBinaryLedPins[0]);
constexpr uint8_t kMaxPanelSelection = (1U << kBinaryLedCount) - 1U;
constexpr unsigned long kPanelDebounceMs = 30;
constexpr unsigned long kPanelLongPressMs = 1000;
constexpr unsigned long kStatusBlinkMs = 140;
constexpr unsigned long kStatusPulseMs = 80;
constexpr uint8_t kStatusLedOn = LOW;
constexpr uint8_t kStatusLedOff = HIGH;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kAdcBlockSamples = 128;
constexpr size_t kAdcQueueDepth = 32;
constexpr size_t kMaxAdcBuffersPerLoop = 4;
constexpr size_t kMaxCaptureMs = 1500;
constexpr size_t kMaxCaptureSamples = (kSampleRate * kMaxCaptureMs) / 1000;
constexpr size_t kPreRollSamples = 1024;
constexpr size_t kTriggerHoldSamples = 96;
constexpr size_t kMaxButtons = kMaxPanelSelection;
constexpr size_t kMaxPendingLine = 260;
constexpr uint8_t kEnvelopeBins = 16;
constexpr uint8_t kBandBins = 8;
constexpr uint8_t kFeatureCount = kEnvelopeBins + kBandBins;
constexpr float kPi = 3.14159265358979323846f;

constexpr const char *kButtonsPath = "/usb/config/buttons.csv";
constexpr const char *kSecretsPath = "/usb/config/secrets.ini";
constexpr const char *kSettingsPath = "/usb/config/settings.ini";
constexpr const char *kPendingPath = "/usb/queue/ntfy-pending.jsonl";
constexpr const char *kPendingTmpPath = "/usb/queue/ntfy-pending.tmp";

struct ButtonConfig {
  char slot[8];
  char word[28];
  char ntfyLabel[40];
  bool templateLoaded;
  uint16_t templateSamples;
  float templ[kFeatureCount];
  unsigned long lastRecognizedMs;
};

struct RuntimeSettings {
  char wifiSsid[64];
  char wifiPass[64];
  char ntfyHost[48];
  char ntfyTopic[72];
  char ntfyToken[128];
  char ntpHost[48];
  bool ntfyTls;
  int timezoneOffsetMinutes;
  float confidenceThreshold;
  float matchDistanceAtZero;
  float triggerMultiplier;
  uint16_t minTriggerAbs;
  uint16_t cooldownMs;
  uint16_t captureMs;
  uint8_t calibrationSamples;
  bool autoNtp;
  bool autoQueueFlush;
};

enum class CaptureState {
  Idle,
  Capturing,
};

struct MatchResult {
  int buttonIndex;
  float confidence;
  float distance;
};

struct PanelButtonState {
  uint8_t pin;
  bool stablePressed;
  bool lastReadingPressed;
  unsigned long lastReadingChangedMs;
  unsigned long pressedSinceMs;
  bool longHandled;
};

struct PanelButtonEvents {
  bool shortPressed;
  bool longPressed;
};

ButtonConfig buttons[kMaxButtons];
size_t buttonCount = 0;

RuntimeSettings settings = {
  "",          // wifiSsid
  "",          // wifiPass
  "ntfy.sh",   // ntfyHost
  "",          // ntfyTopic
  "",          // ntfyToken
  "pool.ntp.org",
  true,        // ntfyTls
  120,         // timezoneOffsetMinutes, Europe/Paris summer default
  0.75f,       // confidenceThreshold
  0.12f,       // matchDistanceAtZero
  4.0f,        // triggerMultiplier
  650,         // minTriggerAbs
  800,         // cooldownMs
  1500,        // captureMs
  12,          // calibrationSamples
  false,       // autoNtp
  false,       // autoQueueFlush
};

AdvancedADC adc(kMicPin);
USBHostMSD msd;
mbed::FATFileSystem usb("usb");
mbed::DigitalOut usbHostEnable(PB_8, 1);
WiFiUDP ntpUdp;

bool usbMounted = false;
bool adcStarted = false;
bool wifiConfigured = false;
bool ntpStarted = false;
bool timeSynced = false;
uint32_t epochAtSync = 0;
unsigned long millisAtSync = 0;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastQueueFlushMs = 0;
unsigned long lastStatusPrintMs = 0;

float dcOffset = 32768.0f;
float noiseFloorAbs = 150.0f;
size_t loudSampleCount = 0;
CaptureState captureState = CaptureState::Idle;
uint16_t preRoll[kPreRollSamples];
size_t preRollIndex = 0;
bool preRollFilled = false;
uint16_t captureBuffer[kMaxCaptureSamples];
size_t captureCount = 0;
uint16_t captureNoiseFloor = 0;
unsigned long captureStartedMs = 0;

bool calibrationActive = false;
int calibrationButtonIndex = -1;
uint8_t calibrationTarget = 0;
uint8_t calibrationCount = 0;
float calibrationAccum[kFeatureCount];

uint8_t selectedButtonNumber = 0;
PanelButtonState calibrationPanelButton = {kCalibrationButtonPin, false, false, 0, 0, false};
PanelButtonState nextPanelButton = {kNextButtonPin, false, false, 0, 0, false};
PanelButtonState previousPanelButton = {kPreviousButtonPin, false, false, 0, 0, false};

uint8_t statusBlinkTogglesRemaining = 0;
bool statusBlinkState = false;
bool statusBlinkFinalOn = false;
unsigned long statusBlinkNextToggleMs = 0;
unsigned long statusLedPulseUntilMs = 0;

char serialLine[96];
size_t serialLineLength = 0;

bool isUsbPath(const char *path) {
  return path != nullptr && strncmp(path, "/usb", 4) == 0;
}

bool isUsbReady() {
  return usbMounted && msd.connected();
}

void copyText(char *dest, size_t destSize, const char *src) {
  if (destSize == 0) {
    return;
  }
  if (src == nullptr) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

char *trimInPlace(char *text) {
  if (text == nullptr) {
    return text;
  }

  while (*text && isspace(static_cast<unsigned char>(*text))) {
    text++;
  }

  char *end = text + strlen(text);
  while (end > text && isspace(static_cast<unsigned char>(*(end - 1)))) {
    end--;
  }
  *end = '\0';
  return text;
}

bool parseBoolValue(const char *value, bool defaultValue) {
  if (value == nullptr || value[0] == '\0') {
    return defaultValue;
  }
  return strcmp(value, "1") == 0 ||
         strcasecmp(value, "true") == 0 ||
         strcasecmp(value, "yes") == 0 ||
         strcasecmp(value, "on") == 0;
}

void ensureDirectories() {
  mkdir("/usb/config", 0777);
  mkdir("/usb/templates", 0777);
  mkdir("/usb/logs", 0777);
  mkdir("/usb/queue", 0777);
  mkdir("/usb/captures", 0777);
}

bool mountUsbStick(uint32_t timeoutMs = 15000) {
  if (isUsbReady()) {
    return true;
  }
  if (usbMounted && !msd.connected()) {
    Serial.println("USB stick disconnected; unmounting filesystem.");
    usb.unmount();
    usbMounted = false;
  }

  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);
  usbHostEnable = 1;

  Serial.println("Waiting for USB stick on USB-A...");
  const unsigned long started = millis();
  while (!msd.connect()) {
    if (millis() - started > timeoutMs) {
      Serial.println("USB stick not found yet.");
      return false;
    }
    delay(100);
  }

  Serial.println("Mounting USB stick as /usb...");
  const int mountResult = usb.mount(&msd);
  if (mountResult != 0) {
    Serial.print("USB mount failed: ");
    Serial.println(mountResult);
    return false;
  }

  usbMounted = true;
  ensureDirectories();
  Serial.println("USB stick mounted.");
  return true;
}

bool fileExists(const char *path) {
  if (isUsbPath(path) && !isUsbReady()) {
    return false;
  }

  FILE *file = fopen(path, "r");
  if (file == nullptr) {
    return false;
  }
  fclose(file);
  return true;
}

void loadDefaultButtons() {
  const char *defaults[][3] = {
    {"A1", "manger", "Manger"},
    {"A2", "eau", "Eau"},
    {"A3", "jouer", "Jouer"},
    {"A4", "dehors", "Dehors"},
    {"B1", "dedans", "Dedans"},
    {"B2", "calin", "Calin"},
  };

  buttonCount = sizeof(defaults) / sizeof(defaults[0]);
  for (size_t i = 0; i < buttonCount; i++) {
    copyText(buttons[i].slot, sizeof(buttons[i].slot), defaults[i][0]);
    copyText(buttons[i].word, sizeof(buttons[i].word), defaults[i][1]);
    copyText(buttons[i].ntfyLabel, sizeof(buttons[i].ntfyLabel), defaults[i][2]);
    buttons[i].templateLoaded = false;
    buttons[i].templateSamples = 0;
    buttons[i].lastRecognizedMs = 0;
    memset(buttons[i].templ, 0, sizeof(buttons[i].templ));
  }
}

bool loadButtons() {
  loadDefaultButtons();

  if (!mountUsbStick(1000)) {
    Serial.println("Using compiled default buttons: USB not mounted.");
    return false;
  }

  FILE *file = fopen(kButtonsPath, "r");
  if (file == nullptr) {
    Serial.println("Using compiled default buttons: /usb/config/buttons.csv missing.");
    return false;
  }

  char line[160];
  bool firstLine = true;
  size_t loaded = 0;
  while (fgets(line, sizeof(line), file) != nullptr && loaded < kMaxButtons) {
    char *row = trimInPlace(line);
    if (row[0] == '\0' || row[0] == '#') {
      continue;
    }
    if (firstLine && strstr(row, "slot") != nullptr) {
      firstLine = false;
      continue;
    }
    firstLine = false;

    char *slot = trimInPlace(strtok(row, ","));
    char *word = trimInPlace(strtok(nullptr, ","));
    char *label = trimInPlace(strtok(nullptr, "\r\n"));
    if (slot == nullptr || word == nullptr || slot[0] == '\0' || word[0] == '\0') {
      continue;
    }
    if (label == nullptr || label[0] == '\0') {
      label = word;
    }

    ButtonConfig &button = buttons[loaded];
    copyText(button.slot, sizeof(button.slot), slot);
    copyText(button.word, sizeof(button.word), word);
    copyText(button.ntfyLabel, sizeof(button.ntfyLabel), label);
    button.templateLoaded = false;
    button.templateSamples = 0;
    button.lastRecognizedMs = 0;
    memset(button.templ, 0, sizeof(button.templ));
    loaded++;
  }

  fclose(file);

  if (loaded > 0) {
    buttonCount = loaded;
  }

  Serial.print("Loaded buttons: ");
  Serial.println(buttonCount);
  return loaded > 0;
}

void applyIniValue(const char *key, const char *value, const bool secretsFile) {
  if (strcmp(key, "wifi_ssid") == 0) {
    copyText(settings.wifiSsid, sizeof(settings.wifiSsid), value);
  } else if (strcmp(key, "wifi_pass") == 0) {
    copyText(settings.wifiPass, sizeof(settings.wifiPass), value);
  } else if (strcmp(key, "ntfy_host") == 0) {
    copyText(settings.ntfyHost, sizeof(settings.ntfyHost), value);
  } else if (strcmp(key, "ntfy_topic") == 0) {
    copyText(settings.ntfyTopic, sizeof(settings.ntfyTopic), value);
  } else if (strcmp(key, "ntfy_token") == 0) {
    copyText(settings.ntfyToken, sizeof(settings.ntfyToken), value);
  } else if (strcmp(key, "ntfy_tls") == 0) {
    settings.ntfyTls = parseBoolValue(value, true);
  } else if (strcmp(key, "ntp_host") == 0) {
    copyText(settings.ntpHost, sizeof(settings.ntpHost), value);
  } else if (strcmp(key, "timezone_offset_minutes") == 0) {
    settings.timezoneOffsetMinutes = atoi(value);
  } else if (strcmp(key, "confidence_threshold") == 0 && !secretsFile) {
    settings.confidenceThreshold = atof(value);
  } else if (strcmp(key, "match_distance_at_zero") == 0 && !secretsFile) {
    settings.matchDistanceAtZero = atof(value);
  } else if (strcmp(key, "trigger_multiplier") == 0 && !secretsFile) {
    settings.triggerMultiplier = atof(value);
  } else if (strcmp(key, "min_trigger_abs") == 0 && !secretsFile) {
    settings.minTriggerAbs = static_cast<uint16_t>(atoi(value));
  } else if (strcmp(key, "cooldown_ms") == 0 && !secretsFile) {
    settings.cooldownMs = static_cast<uint16_t>(atoi(value));
  } else if (strcmp(key, "capture_ms") == 0 && !secretsFile) {
    const uint16_t requested = static_cast<uint16_t>(atoi(value));
    settings.captureMs = requested > kMaxCaptureMs ? kMaxCaptureMs : requested;
  } else if (strcmp(key, "calibration_samples") == 0 && !secretsFile) {
    const int requested = atoi(value);
    if (requested >= 3 && requested <= 30) {
      settings.calibrationSamples = static_cast<uint8_t>(requested);
    }
  } else if (strcmp(key, "auto_ntp") == 0 && !secretsFile) {
    settings.autoNtp = parseBoolValue(value, false);
  } else if (strcmp(key, "auto_queue_flush") == 0 && !secretsFile) {
    settings.autoQueueFlush = parseBoolValue(value, false);
  }
}

bool loadIniFile(const char *path, bool secretsFile) {
  if (!mountUsbStick(1000)) {
    return false;
  }

  FILE *file = fopen(path, "r");
  if (file == nullptr) {
    Serial.print("Config missing: ");
    Serial.println(path);
    return false;
  }

  char line[180];
  while (fgets(line, sizeof(line), file) != nullptr) {
    char *row = trimInPlace(line);
    if (row[0] == '\0' || row[0] == '#' || row[0] == ';') {
      continue;
    }

    char *equals = strchr(row, '=');
    if (equals == nullptr) {
      continue;
    }

    *equals = '\0';
    char *key = trimInPlace(row);
    char *value = trimInPlace(equals + 1);
    applyIniValue(key, value, secretsFile);
  }

  fclose(file);
  return true;
}

void loadRuntimeConfig() {
  loadIniFile(kSecretsPath, true);
  loadIniFile(kSettingsPath, false);
  wifiConfigured = settings.wifiSsid[0] != '\0' && settings.ntfyTopic[0] != '\0';

  Serial.print("Wi-Fi config: ");
  Serial.println(wifiConfigured ? "ready" : "missing SSID or ntfy topic");
  Serial.print("Capture window ms: ");
  Serial.println(settings.captureMs);
  Serial.print("Confidence threshold: ");
  Serial.println(settings.confidenceThreshold, 2);
  Serial.print("Auto NTP: ");
  Serial.println(settings.autoNtp ? "on" : "off");
  Serial.print("Auto queue flush: ");
  Serial.println(settings.autoQueueFlush ? "on" : "off");
}

void templatePathForButton(const ButtonConfig &button, char *path, size_t pathSize) {
  snprintf(path, pathSize, "/usb/templates/%s.tpl", button.slot);
}

bool parseFeaturesLine(char *value, float *features) {
  for (uint8_t i = 0; i < kFeatureCount; i++) {
    char *token = (i == 0) ? strtok(value, ",") : strtok(nullptr, ",");
    if (token == nullptr) {
      return false;
    }
    features[i] = atof(trimInPlace(token));
  }
  return true;
}

bool loadTemplate(ButtonConfig &button) {
  if (!isUsbReady()) {
    return false;
  }

  char path[80];
  templatePathForButton(button, path, sizeof(path));
  FILE *file = fopen(path, "r");
  if (file == nullptr) {
    button.templateLoaded = false;
    return false;
  }

  bool foundFeatures = false;
  char line[220];
  while (fgets(line, sizeof(line), file) != nullptr) {
    char *row = trimInPlace(line);
    char *equals = strchr(row, '=');
    if (equals == nullptr) {
      continue;
    }
    *equals = '\0';
    char *key = trimInPlace(row);
    char *value = trimInPlace(equals + 1);
    if (strcmp(key, "samples") == 0) {
      button.templateSamples = static_cast<uint16_t>(atoi(value));
    } else if (strcmp(key, "features") == 0) {
      foundFeatures = parseFeaturesLine(value, button.templ);
    }
  }

  fclose(file);
  button.templateLoaded = foundFeatures;
  return foundFeatures;
}

void loadTemplates() {
  uint8_t loaded = 0;
  for (size_t i = 0; i < buttonCount; i++) {
    if (loadTemplate(buttons[i])) {
      loaded++;
    }
  }

  Serial.print("Loaded templates: ");
  Serial.print(loaded);
  Serial.print("/");
  Serial.println(buttonCount);
}

bool saveTemplate(ButtonConfig &button, const float *features, uint16_t sampleCount) {
  if (!mountUsbStick(3000)) {
    return false;
  }

  char path[80];
  templatePathForButton(button, path, sizeof(path));
  FILE *file = fopen(path, "w");
  if (file == nullptr) {
    Serial.print("Template save failed: ");
    Serial.println(path);
    return false;
  }

  fprintf(file, "TPB_TEMPLATE_V1\n");
  fprintf(file, "slot=%s\n", button.slot);
  fprintf(file, "word=%s\n", button.word);
  fprintf(file, "samples=%u\n", sampleCount);
  fprintf(file, "features=");
  for (uint8_t i = 0; i < kFeatureCount; i++) {
    if (i > 0) {
      fputc(',', file);
    }
    fprintf(file, "%.7f", features[i]);
    button.templ[i] = features[i];
  }
  fputc('\n', file);
  fclose(file);

  button.templateLoaded = true;
  button.templateSamples = sampleCount;
  Serial.print("Saved template: ");
  Serial.println(path);
  return true;
}

void printButtonList() {
  Serial.println("Buttons:");
  for (size_t i = 0; i < buttonCount; i++) {
    Serial.print("  ");
    Serial.print(buttons[i].slot);
    Serial.print(" -> ");
    Serial.print(buttons[i].word);
    Serial.print(" / ");
    Serial.print(buttons[i].ntfyLabel);
    Serial.print(" / template=");
    Serial.println(buttons[i].templateLoaded ? "yes" : "no");
  }
}

int findButtonBySlot(const char *slot) {
  for (size_t i = 0; i < buttonCount; i++) {
    if (strcasecmp(buttons[i].slot, slot) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

size_t selectableButtonCount() {
  return buttonCount < kMaxPanelSelection ? buttonCount : kMaxPanelSelection;
}

int selectedButtonIndex() {
  if (selectedButtonNumber == 0 || selectedButtonNumber > selectableButtonCount()) {
    return -1;
  }
  return static_cast<int>(selectedButtonNumber - 1);
}

void setStatusLed(bool on) {
  digitalWrite(LED_BUILTIN, on ? kStatusLedOn : kStatusLedOff);
}

void stopStatusLedEffects() {
  statusBlinkTogglesRemaining = 0;
  statusLedPulseUntilMs = 0;
  setStatusLed(calibrationActive);
}

void startStatusBlink(uint8_t flashes, bool finalOn) {
  statusBlinkTogglesRemaining = flashes * 2;
  statusBlinkState = false;
  statusBlinkFinalOn = finalOn;
  statusBlinkNextToggleMs = millis();
  statusLedPulseUntilMs = 0;
  setStatusLed(false);
}

void pulseStatusLed() {
  if (calibrationActive && statusBlinkTogglesRemaining == 0) {
    statusLedPulseUntilMs = millis() + kStatusPulseMs;
  }
}

void updateStatusLed() {
  const unsigned long now = millis();
  if (statusBlinkTogglesRemaining > 0) {
    if (now >= statusBlinkNextToggleMs) {
      statusBlinkState = !statusBlinkState;
      setStatusLed(statusBlinkState);
      statusBlinkTogglesRemaining--;
      statusBlinkNextToggleMs = now + kStatusBlinkMs;
      if (statusBlinkTogglesRemaining == 0) {
        setStatusLed(statusBlinkFinalOn);
      }
    }
    return;
  }

  if (statusLedPulseUntilMs > 0) {
    if (now < statusLedPulseUntilMs) {
      setStatusLed(false);
      return;
    }
    statusLedPulseUntilMs = 0;
  }

  setStatusLed(calibrationActive);
}

void writeBinaryLeds(uint8_t value) {
  for (uint8_t i = 0; i < kBinaryLedCount; i++) {
    digitalWrite(kBinaryLedPins[i], (value & (1U << i)) ? HIGH : LOW);
  }
}

void updateBinaryLeds() {
  const uint8_t value = selectedButtonNumber <= kMaxPanelSelection ? selectedButtonNumber : 0;
  writeBinaryLeds(value);
}

void printPanelSelection() {
  const int index = selectedButtonIndex();
  Serial.print("Panel selection: ");
  if (index < 0) {
    Serial.println("none (0)");
    return;
  }

  Serial.print(selectedButtonNumber);
  Serial.print(" -> ");
  Serial.print(buttons[index].slot);
  Serial.print(" / ");
  Serial.println(buttons[index].word);
}

void setPanelSelection(uint8_t number, bool announce = false) {
  const size_t maxSelection = selectableButtonCount();
  if (number > maxSelection) {
    number = static_cast<uint8_t>(maxSelection);
  }
  selectedButtonNumber = number;
  updateBinaryLeds();
  if (announce) {
    printPanelSelection();
  }
}

void syncPanelSelectionToButtonIndex(int index) {
  if (index >= 0 && static_cast<size_t>(index) < selectableButtonCount()) {
    setPanelSelection(static_cast<uint8_t>(index + 1));
  } else {
    setPanelSelection(0);
  }
}

void clampPanelSelection() {
  if (selectedButtonNumber > selectableButtonCount()) {
    setPanelSelection(0);
  } else {
    updateBinaryLeds();
  }
}

void stepPanelSelection(int delta) {
  if (calibrationActive) {
    return;
  }

  const int maxSelection = static_cast<int>(selectableButtonCount());
  int next = static_cast<int>(selectedButtonNumber) + delta;
  if (next < 0) {
    next = 0;
  } else if (next > maxSelection) {
    next = maxSelection;
  }

  if (next != selectedButtonNumber) {
    setPanelSelection(static_cast<uint8_t>(next), true);
  }
}

void selectAfterCompletedCalibration(int completedIndex) {
  const size_t maxSelection = selectableButtonCount();
  const uint8_t next = (completedIndex >= 0 && static_cast<size_t>(completedIndex + 1) < maxSelection)
                         ? static_cast<uint8_t>(completedIndex + 2)
                         : 0;
  setPanelSelection(next, true);
}

void cancelCalibration(const char *message) {
  calibrationActive = false;
  calibrationButtonIndex = -1;
  calibrationTarget = 0;
  calibrationCount = 0;
  stopStatusLedEffects();
  if (message != nullptr) {
    Serial.println(message);
  }
}

uint16_t captureTargetSamples() {
  const uint32_t requested = (static_cast<uint32_t>(settings.captureMs) * kSampleRate) / 1000;
  if (requested == 0 || requested > kMaxCaptureSamples) {
    return kMaxCaptureSamples;
  }
  return static_cast<uint16_t>(requested);
}

float triggerThreshold() {
  const float adaptive = noiseFloorAbs * settings.triggerMultiplier;
  return adaptive > settings.minTriggerAbs ? adaptive : settings.minTriggerAbs;
}

void rememberPreRoll(uint16_t sample) {
  preRoll[preRollIndex] = sample;
  preRollIndex = (preRollIndex + 1) % kPreRollSamples;
  if (preRollIndex == 0) {
    preRollFilled = true;
  }
}

void startCapture() {
  captureState = CaptureState::Capturing;
  captureCount = 0;
  captureNoiseFloor = static_cast<uint16_t>(noiseFloorAbs);
  captureStartedMs = millis();

  const size_t available = preRollFilled ? kPreRollSamples : preRollIndex;
  const size_t first = preRollFilled ? preRollIndex : 0;
  for (size_t i = 0; i < available && captureCount < kMaxCaptureSamples; i++) {
    const size_t index = (first + i) % kPreRollSamples;
    captureBuffer[captureCount++] = preRoll[index];
  }

  Serial.print("Audio event started, noise=");
  Serial.print(captureNoiseFloor);
  Serial.print(", threshold=");
  Serial.println(triggerThreshold(), 0);
}

void resetCaptureDetector() {
  captureState = CaptureState::Idle;
  captureCount = 0;
  loudSampleCount = 0;
}

void normalizeFeatures(float *features, uint8_t start, uint8_t count) {
  float sum = 0.0f;
  for (uint8_t i = 0; i < count; i++) {
    sum += features[start + i];
  }
  if (sum <= 0.000001f) {
    const float uniform = 1.0f / count;
    for (uint8_t i = 0; i < count; i++) {
      features[start + i] = uniform;
    }
    return;
  }
  for (uint8_t i = 0; i < count; i++) {
    features[start + i] /= sum;
  }
}

bool computeFingerprint(const uint16_t *samples, size_t sampleCount, float *features) {
  if (sampleCount < 1000) {
    return false;
  }

  double mean = 0.0;
  for (size_t i = 0; i < sampleCount; i++) {
    mean += samples[i];
  }
  mean /= sampleCount;

  memset(features, 0, sizeof(float) * kFeatureCount);

  for (uint8_t bin = 0; bin < kEnvelopeBins; bin++) {
    const size_t start = (sampleCount * bin) / kEnvelopeBins;
    const size_t end = (sampleCount * (bin + 1)) / kEnvelopeBins;
    double sumSquares = 0.0;
    for (size_t i = start; i < end; i++) {
      const double x = static_cast<double>(samples[i]) - mean;
      sumSquares += x * x;
    }
    const double denom = (end > start) ? (end - start) : 1;
    features[bin] = static_cast<float>(sqrt(sumSquares / denom));
  }
  normalizeFeatures(features, 0, kEnvelopeBins);

  const uint16_t bandFrequencies[kBandBins] = {300, 500, 750, 1100, 1650, 2400, 3600, 5200};
  for (uint8_t band = 0; band < kBandBins; band++) {
    const float normalizedFrequency = static_cast<float>(bandFrequencies[band]) / kSampleRate;
    const float omega = 2.0f * kPi * normalizedFrequency;
    const float coeff = 2.0f * cosf(omega);
    float q0 = 0.0f;
    float q1 = 0.0f;
    float q2 = 0.0f;

    for (size_t i = 0; i < sampleCount; i++) {
      const float x = (static_cast<float>(samples[i]) - static_cast<float>(mean)) / 32768.0f;
      q0 = coeff * q1 - q2 + x;
      q2 = q1;
      q1 = q0;
    }

    const float power = q1 * q1 + q2 * q2 - coeff * q1 * q2;
    features[kEnvelopeBins + band] = sqrtf(power > 0.0f ? power : 0.0f);
  }
  normalizeFeatures(features, kEnvelopeBins, kBandBins);

  return true;
}

float featureDistance(const float *a, const float *b) {
  float sumAbs = 0.0f;
  for (uint8_t i = 0; i < kFeatureCount; i++) {
    sumAbs += fabsf(a[i] - b[i]);
  }
  return sumAbs / kFeatureCount;
}

float confidenceFromDistance(float distance) {
  if (settings.matchDistanceAtZero <= 0.0001f) {
    return 0.0f;
  }
  const float confidence = 1.0f - (distance / settings.matchDistanceAtZero);
  if (confidence < 0.0f) {
    return 0.0f;
  }
  if (confidence > 1.0f) {
    return 1.0f;
  }
  return confidence;
}

MatchResult recognizeFeatures(const float *features) {
  MatchResult best = {-1, 0.0f, 999.0f};
  for (size_t i = 0; i < buttonCount; i++) {
    if (!buttons[i].templateLoaded) {
      continue;
    }
    const float distance = featureDistance(features, buttons[i].templ);
    const float confidence = confidenceFromDistance(distance);
    if (confidence > best.confidence) {
      best.buttonIndex = static_cast<int>(i);
      best.confidence = confidence;
      best.distance = distance;
    }
  }
  return best;
}

void formatTimestamp(char *dest, size_t destSize) {
  if (!timeSynced) {
    snprintf(dest, destSize, "boot+%lus", millis() / 1000UL);
    return;
  }

  const uint32_t elapsed = (millis() - millisAtSync) / 1000UL;
  const time_t localEpoch = static_cast<time_t>(epochAtSync + elapsed +
                                                settings.timezoneOffsetMinutes * 60);
  struct tm *tm = gmtime(&localEpoch);
  if (tm == nullptr) {
    snprintf(dest, destSize, "boot+%lus", millis() / 1000UL);
    return;
  }

  snprintf(dest, destSize, "%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec,
           settings.timezoneOffsetMinutes / 60,
           abs(settings.timezoneOffsetMinutes % 60));
}

void logPath(char *dest, size_t destSize) {
  if (!timeSynced) {
    snprintf(dest, destSize, "/usb/logs/unsynced.csv");
    return;
  }

  const uint32_t elapsed = (millis() - millisAtSync) / 1000UL;
  const time_t localEpoch = static_cast<time_t>(epochAtSync + elapsed +
                                                settings.timezoneOffsetMinutes * 60);
  struct tm *tm = gmtime(&localEpoch);
  if (tm == nullptr) {
    snprintf(dest, destSize, "/usb/logs/unsynced.csv");
    return;
  }

  snprintf(dest, destSize, "/usb/logs/%04d-%02d-%02d.csv",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday);
}

void appendLog(const char *slot,
               const char *word,
               float confidence,
               uint16_t durationMs,
               uint16_t noiseFloor,
               bool notified) {
  if (!mountUsbStick(1000)) {
    Serial.println("Log skipped: USB unavailable.");
    return;
  }

  char path[80];
  logPath(path, sizeof(path));
  const bool exists = fileExists(path);
  FILE *file = fopen(path, "a");
  if (file == nullptr) {
    Serial.print("Log open failed: ");
    Serial.println(path);
    return;
  }

  if (!exists) {
    fprintf(file, "timestamp,slot,word,confidence,duration_ms,noise_floor,notified\n");
  }

  char timestamp[40];
  formatTimestamp(timestamp, sizeof(timestamp));
  fprintf(file, "%s,%s,%s,%.2f,%u,%u,%s\n",
          timestamp,
          slot,
          word,
          confidence,
          durationMs,
          noiseFloor,
          notified ? "true" : "false");
  fclose(file);
}

void writeJsonString(FILE *file, const char *value) {
  fputc('"', file);
  for (const char *p = value; *p; p++) {
    if (*p == '"' || *p == '\\') {
      fputc('\\', file);
    }
    if (*p == '\n' || *p == '\r') {
      fputc(' ', file);
    } else {
      fputc(*p, file);
    }
  }
  fputc('"', file);
}

bool appendPendingNotification(const char *title, const char *message) {
  if (!mountUsbStick(1000)) {
    Serial.println("Pending ntfy skipped: USB unavailable.");
    return false;
  }

  Serial.println("Queueing pending ntfy event.");
  FILE *file = fopen(kPendingPath, "a");
  if (file == nullptr) {
    Serial.println("Cannot append pending ntfy event.");
    return false;
  }

  fputs("{\"title\":", file);
  writeJsonString(file, title);
  fputs(",\"message\":", file);
  writeJsonString(file, message);
  fputs("}\n", file);
  fclose(file);
  Serial.println("Pending ntfy event queued.");
  return true;
}

bool extractJsonField(const char *line, const char *field, char *dest, size_t destSize) {
  char needle[32];
  snprintf(needle, sizeof(needle), "\"%s\":", field);
  const char *found = strstr(line, needle);
  if (found == nullptr) {
    return false;
  }

  const char *p = found + strlen(needle);
  while (*p && isspace(static_cast<unsigned char>(*p))) {
    p++;
  }
  if (*p != '"') {
    return false;
  }
  p++;

  size_t out = 0;
  while (*p && *p != '"' && out + 1 < destSize) {
    if (*p == '\\' && *(p + 1) != '\0') {
      p++;
    }
    dest[out++] = *p++;
  }
  dest[out] = '\0';
  return out > 0;
}

bool ensureWifiConnected(uint32_t minRetryMs = 15000) {
  if (!wifiConfigured) {
    return false;
  }
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  if (millis() - lastWifiAttemptMs < minRetryMs) {
    return false;
  }
  lastWifiAttemptMs = millis();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Wi-Fi module unavailable.");
    return false;
  }

  Serial.print("Connecting Wi-Fi SSID: ");
  Serial.println(settings.wifiSsid);
  Serial.println("Wi-Fi: begin");
  Serial.flush();
  const int status = WiFi.begin(settings.wifiSsid, settings.wifiPass);
  Serial.print("Wi-Fi: begin returned=");
  Serial.println(status);
  delay(2500);
  const int currentStatus = WiFi.status();
  Serial.print("Wi-Fi: status after wait=");
  Serial.println(currentStatus);

  if (status == WL_CONNECTED || currentStatus == WL_CONNECTED) {
    Serial.println("Wi-Fi: reading local IP");
    Serial.flush();
    Serial.print("Wi-Fi connected, IP=");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("Wi-Fi connection failed.");
  return false;
}

template <typename TClient>
bool postNtfyWithClient(TClient &client, uint16_t port, const char *title, const char *message) {
  client.setSocketTimeout(5000);
  Serial.print("ntfy connect: ");
  Serial.print(settings.ntfyTls ? "https://" : "http://");
  Serial.print(settings.ntfyHost);
  Serial.print(":");
  Serial.println(port);
  Serial.flush();

  if (!client.connect(settings.ntfyHost, port)) {
    Serial.println("ntfy connect failed.");
    client.stop();
    return false;
  }

  Serial.println("ntfy connected, sending request.");
  const size_t bodyLength = strlen(message);
  client.print("POST /");
  client.print(settings.ntfyTopic);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(settings.ntfyHost);
  client.println("User-Agent: talking-pet-buttons-giga/1.0");
  client.print("Title: ");
  client.println(title);
  client.println("Content-Type: text/plain; charset=utf-8");
  if (settings.ntfyToken[0] != '\0') {
    client.print("Authorization: Bearer ");
    client.println(settings.ntfyToken);
  }
  client.print("Content-Length: ");
  client.println(bodyLength);
  client.println("Connection: close");
  client.println();
  client.print(message);

  char statusLine[48] = {0};
  size_t idx = 0;
  const unsigned long deadline = millis() + 5000;
  while (millis() < deadline && client.connected()) {
    while (client.available()) {
      const char c = static_cast<char>(client.read());
      if (c == '\n') {
        statusLine[idx] = '\0';
        client.stop();
        Serial.print("ntfy status: ");
        Serial.println(statusLine);
        return strstr(statusLine, " 2") != nullptr;
      }
      if (c != '\r' && idx + 1 < sizeof(statusLine)) {
        statusLine[idx++] = c;
      }
    }
  }

  client.stop();
  Serial.println("ntfy response timeout.");
  return false;
}

bool sendNtfy(const char *title, const char *message) {
  if (!ensureWifiConnected()) {
    return false;
  }

  bool sent = false;
  if (settings.ntfyTls) {
    WiFiSSLClient client;
    sent = postNtfyWithClient(client, 443, title, message);
  } else {
    WiFiClient client;
    sent = postNtfyWithClient(client, 80, title, message);
  }

  Serial.print("ntfy send: ");
  Serial.println(sent ? "ok" : "failed");
  return sent;
}

void flushPendingNotifications() {
  if (!mountUsbStick(1000) || !fileExists(kPendingPath)) {
    return;
  }
  if (!ensureWifiConnected()) {
    return;
  }

  FILE *in = fopen(kPendingPath, "r");
  if (in == nullptr) {
    return;
  }
  FILE *out = fopen(kPendingTmpPath, "w");
  if (out == nullptr) {
    fclose(in);
    return;
  }

  uint16_t sentCount = 0;
  uint16_t keptCount = 0;
  char line[kMaxPendingLine];
  while (fgets(line, sizeof(line), in) != nullptr) {
    char title[64];
    char message[160];
    if (extractJsonField(line, "title", title, sizeof(title)) &&
        extractJsonField(line, "message", message, sizeof(message)) &&
        sendNtfy(title, message)) {
      sentCount++;
    } else {
      fputs(line, out);
      keptCount++;
    }
  }

  fclose(in);
  fclose(out);
  remove(kPendingPath);
  if (keptCount > 0) {
    rename(kPendingTmpPath, kPendingPath);
  } else {
    remove(kPendingTmpPath);
  }

  if (sentCount > 0) {
    Serial.print("Flushed pending ntfy events: ");
    Serial.println(sentCount);
  }
}

bool syncTimeWithNtp() {
  if (timeSynced || !ensureWifiConnected()) {
    return timeSynced;
  }

  if (!ntpStarted) {
    Serial.println("NTP: starting UDP on local port 2390");
    Serial.flush();
    ntpUdp.begin(2390);
    ntpStarted = true;
  }

  uint8_t packet[48] = {0};
  packet[0] = 0b11100011;
  packet[1] = 0;
  packet[2] = 6;
  packet[3] = 0xEC;
  packet[12] = 49;
  packet[13] = 0x4E;
  packet[14] = 49;
  packet[15] = 52;

  Serial.print("NTP: sending request to ");
  Serial.println(settings.ntpHost);
  Serial.flush();
  const int packetStarted = ntpUdp.beginPacket(settings.ntpHost, 123);
  Serial.print("NTP: beginPacket=");
  Serial.println(packetStarted);
  if (packetStarted == 0) {
    Serial.println("NTP sync failed: beginPacket returned 0.");
    return false;
  }
  ntpUdp.write(packet, sizeof(packet));
  const int packetSent = ntpUdp.endPacket();
  Serial.print("NTP: endPacket=");
  Serial.println(packetSent);
  if (packetSent == 0) {
    Serial.println("NTP sync failed: endPacket returned 0.");
    return false;
  }

  const unsigned long deadline = millis() + 1600;
  while (millis() < deadline) {
    if (ntpUdp.parsePacket()) {
      ntpUdp.read(packet, sizeof(packet));
      const uint32_t highWord = word(packet[40], packet[41]);
      const uint32_t lowWord = word(packet[42], packet[43]);
      const uint32_t secsSince1900 = (highWord << 16) | lowWord;
      epochAtSync = secsSince1900 - 2208988800UL;
      millisAtSync = millis();
      timeSynced = true;
      Serial.print("NTP synced, epoch=");
      Serial.println(epochAtSync);
      return true;
    }
    delay(10);
  }

  Serial.println("NTP sync failed.");
  return false;
}

void handleRecognizedEvent(const MatchResult &match,
                           uint16_t durationMs,
                           uint16_t eventNoiseFloor) {
  if (match.buttonIndex < 0) {
    appendLog("unknown", "", match.confidence, durationMs, eventNoiseFloor, false);
    Serial.print("Unknown event, best confidence=");
    Serial.println(match.confidence, 2);
    return;
  }

  ButtonConfig &button = buttons[match.buttonIndex];
  const unsigned long now = millis();
  if (now - button.lastRecognizedMs < settings.cooldownMs) {
    Serial.print("Cooldown ignored: ");
    Serial.println(button.slot);
    return;
  }
  button.lastRecognizedMs = now;

  char title[64];
  char message[160];
  snprintf(title, sizeof(title), "Bouton %s", button.ntfyLabel);
  snprintf(message, sizeof(message), "Bouton: %s (%d%%)",
           button.ntfyLabel,
           static_cast<int>(match.confidence * 100.0f + 0.5f));

  const bool delivered = sendNtfy(title, message);
  if (!delivered) {
    appendPendingNotification(title, message);
  }

  appendLog(button.slot,
            button.word,
            match.confidence,
            durationMs,
            eventNoiseFloor,
            delivered);

  Serial.print("Recognized ");
  Serial.print(button.slot);
  Serial.print(" / ");
  Serial.print(button.word);
  Serial.print(" confidence=");
  Serial.print(match.confidence, 2);
  Serial.print(" distance=");
  Serial.println(match.distance, 4);
}

void handleCapturedAudio() {
  float features[kFeatureCount];
  const uint16_t durationMs = static_cast<uint16_t>((captureCount * 1000UL) / kSampleRate);

  if (!computeFingerprint(captureBuffer, captureCount, features)) {
    Serial.println("Capture too short for fingerprint.");
    resetCaptureDetector();
    return;
  }

  if (calibrationActive && calibrationButtonIndex >= 0) {
    for (uint8_t i = 0; i < kFeatureCount; i++) {
      calibrationAccum[i] += features[i];
    }
    calibrationCount++;

    Serial.print("Calibration ");
    Serial.print(buttons[calibrationButtonIndex].slot);
    Serial.print(": ");
    Serial.print(calibrationCount);
    Serial.print("/");
    Serial.println(calibrationTarget);
    pulseStatusLed();

    if (calibrationCount >= calibrationTarget) {
      const int completedIndex = calibrationButtonIndex;
      float average[kFeatureCount];
      for (uint8_t i = 0; i < kFeatureCount; i++) {
        average[i] = calibrationAccum[i] / calibrationCount;
      }
      saveTemplate(buttons[calibrationButtonIndex], average, calibrationCount);
      calibrationActive = false;
      calibrationButtonIndex = -1;
      calibrationTarget = 0;
      calibrationCount = 0;
      stopStatusLedEffects();
      Serial.println("Calibration complete.");
      selectAfterCompletedCalibration(completedIndex);
    }

    resetCaptureDetector();
    return;
  }

  const MatchResult match = recognizeFeatures(features);
  if (match.confidence >= settings.confidenceThreshold) {
    handleRecognizedEvent(match, durationMs, captureNoiseFloor);
  } else {
    appendLog("unknown", "", match.confidence, durationMs, captureNoiseFloor, false);
    Serial.print("Rejected event, best confidence=");
    Serial.print(match.confidence, 2);
    Serial.print(" distance=");
    Serial.println(match.distance, 4);
  }

  resetCaptureDetector();
}

void processSample(uint16_t sample) {
  rememberPreRoll(sample);

  const float centered = static_cast<float>(sample) - dcOffset;
  const float absCentered = fabsf(centered);

  if (captureState == CaptureState::Idle) {
    dcOffset += (static_cast<float>(sample) - dcOffset) * 0.0008f;
    noiseFloorAbs = (noiseFloorAbs * 0.999f) + (absCentered * 0.001f);

    if (absCentered > triggerThreshold()) {
      loudSampleCount++;
      if (loudSampleCount >= kTriggerHoldSamples) {
        startCapture();
      }
    } else if (loudSampleCount > 0) {
      loudSampleCount--;
    }
    return;
  }

  if (captureCount < kMaxCaptureSamples) {
    captureBuffer[captureCount++] = sample;
  }

  if (captureCount >= captureTargetSamples()) {
    handleCapturedAudio();
  }
}

void pollAudio() {
  size_t processedBuffers = 0;
  while (adc.available() && processedBuffers < kMaxAdcBuffersPerLoop) {
    SampleBuffer buffer = adc.read();
    for (size_t i = 0; i < buffer.size(); i++) {
      processSample(buffer[i]);
    }
    buffer.release();
    processedBuffers++;
  }
}

bool startAdc() {
  if (adcStarted) {
    return true;
  }

  if (!adc.begin(AN_RESOLUTION_16, kSampleRate, kAdcBlockSamples, kAdcQueueDepth)) {
    Serial.println("Failed to start ADC on A0.");
    return false;
  }

  adcStarted = true;
  Serial.println("ADC started on A0 at 16 kHz.");
  return true;
}

void startCalibration(const char *slot, uint8_t target) {
  const int index = findButtonBySlot(slot);
  if (index < 0) {
    Serial.print("Unknown slot: ");
    Serial.println(slot);
    printButtonList();
    return;
  }

  calibrationActive = true;
  calibrationButtonIndex = index;
  calibrationTarget = target == 0 ? settings.calibrationSamples : target;
  calibrationCount = 0;
  memset(calibrationAccum, 0, sizeof(calibrationAccum));
  syncPanelSelectionToButtonIndex(index);
  startStatusBlink(3, true);

  Serial.print("Calibration armed for ");
  Serial.print(buttons[index].slot);
  Serial.print(" / ");
  Serial.print(buttons[index].word);
  Serial.print(". Press this button ");
  Serial.print(calibrationTarget);
  Serial.println(" times.");
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help              show this help");
  Serial.println("  list              show buttons and template state");
  Serial.println("  status            show noise, Wi-Fi, USB and panel state");
  Serial.println("  usb               mount or remount the USB stick now");
  Serial.println("  reload            reload USB config and templates");
  Serial.println("  select N          set panel selection, 0 turns binary LEDs off");
  Serial.println("  ledtest N         show raw binary value 0..63 on panel LEDs");
  Serial.println("  wifi              connect to configured Wi-Fi now");
  Serial.println("  ntp               sync time with NTP now");
  Serial.println("  cal A1 [12]       learn the next N presses for slot A1");
  Serial.println("  cancel            cancel active calibration");
  Serial.println("  testntfy          send a test notification");
  Serial.println("Panel:");
  Serial.println("  D22 long press    start/cancel calibration for selected button");
  Serial.println("  D23 short press   select next button");
  Serial.println("  D24 short press   select previous button");
}

void printStatus() {
  const int selectedIndex = selectedButtonIndex();
  Serial.print("USB=");
  Serial.print(isUsbReady() ? "mounted" : "missing");
  Serial.print(" WiFi=");
  Serial.print(WiFi.status() == WL_CONNECTED ? "connected" : "offline");
  Serial.print(" time=");
  Serial.print(timeSynced ? "synced" : "unsynced");
  Serial.print(" noise=");
  Serial.print(noiseFloorAbs, 0);
  Serial.print(" threshold=");
  Serial.print(triggerThreshold(), 0);
  Serial.print(" selected=");
  Serial.print(selectedIndex >= 0 ? buttons[selectedIndex].slot : "none");
  Serial.print(" selected_no=");
  Serial.print(selectedButtonNumber);
  Serial.print(" calibration=");
  Serial.print(calibrationActive ? "active" : "off");
  Serial.print(" progress=");
  Serial.print(calibrationActive ? calibrationCount : 0);
  Serial.print("/");
  Serial.print(calibrationActive ? calibrationTarget : 0);
  Serial.print(" auto_ntp=");
  Serial.print(settings.autoNtp ? "on" : "off");
  Serial.print(" auto_queue=");
  Serial.println(settings.autoQueueFlush ? "on" : "off");
}

void handleSelectCommand(char *argument) {
  if (calibrationActive) {
    Serial.println("Select ignored: calibration active.");
    return;
  }

  char *value = trimInPlace(argument);
  if (value[0] == '\0') {
    Serial.println("Usage: select 0..N");
    return;
  }

  char *end = nullptr;
  const long requested = strtol(value, &end, 10);
  end = trimInPlace(end);
  if (end == value || end[0] != '\0') {
    Serial.println("Usage: select 0..N");
    return;
  }

  const size_t maxSelection = selectableButtonCount();
  if (requested < 0 || requested > static_cast<long>(maxSelection)) {
    Serial.print("Select out of range. Use 0..");
    Serial.println(maxSelection);
    return;
  }

  setPanelSelection(static_cast<uint8_t>(requested), true);
}

void handleLedTestCommand(char *argument) {
  if (calibrationActive) {
    Serial.println("LED test ignored: calibration active.");
    return;
  }

  char *value = trimInPlace(argument);
  if (value[0] == '\0') {
    Serial.print("Usage: ledtest 0..");
    Serial.println(kMaxPanelSelection);
    return;
  }

  char *end = nullptr;
  const long requested = strtol(value, &end, 10);
  end = trimInPlace(end);
  if (end == value || end[0] != '\0') {
    Serial.print("Usage: ledtest 0..");
    Serial.println(kMaxPanelSelection);
    return;
  }

  if (requested < 0 || requested > kMaxPanelSelection) {
    Serial.print("LED test out of range. Use 0..");
    Serial.println(kMaxPanelSelection);
    return;
  }

  writeBinaryLeds(static_cast<uint8_t>(requested));
  Serial.print("LED test value: ");
  Serial.println(requested);
  Serial.println("Use select 0..N to restore normal panel selection.");
}

void handleWifiCommand() {
  const bool connected = ensureWifiConnected(0);
  Serial.print("Wi-Fi manual connect: ");
  Serial.println(connected ? "connected" : "failed");
}

void handleNtpCommand() {
  if (timeSynced) {
    Serial.println("NTP already synced.");
    return;
  }

  const bool synced = syncTimeWithNtp();
  Serial.print("NTP manual sync: ");
  Serial.println(synced ? "synced" : "failed");
}

void handleUsbCommand() {
  const bool mounted = mountUsbStick(5000);
  Serial.print("USB manual mount: ");
  Serial.println(mounted ? "mounted" : "missing");
}

void executeSerialCommand(char *line) {
  char *command = trimInPlace(line);
  if (command[0] == '\0') {
    return;
  }

  if (strcasecmp(command, "help") == 0) {
    printHelp();
  } else if (strcasecmp(command, "list") == 0) {
    printButtonList();
  } else if (strcasecmp(command, "status") == 0) {
    printStatus();
  } else if (strcasecmp(command, "usb") == 0) {
    handleUsbCommand();
  } else if (strcasecmp(command, "reload") == 0) {
    mountUsbStick(5000);
    loadButtons();
    loadRuntimeConfig();
    loadTemplates();
    clampPanelSelection();
    printButtonList();
  } else if (strncasecmp(command, "select ", 7) == 0) {
    handleSelectCommand(command + 7);
  } else if (strncasecmp(command, "ledtest ", 8) == 0) {
    handleLedTestCommand(command + 8);
  } else if (strcasecmp(command, "wifi") == 0) {
    handleWifiCommand();
  } else if (strcasecmp(command, "ntp") == 0) {
    handleNtpCommand();
  } else if (strncasecmp(command, "cal ", 4) == 0) {
    char *slot = trimInPlace(command + 4);
    char *space = strchr(slot, ' ');
    uint8_t target = settings.calibrationSamples;
    if (space != nullptr) {
      *space = '\0';
      target = static_cast<uint8_t>(atoi(trimInPlace(space + 1)));
      if (target == 0) {
        target = settings.calibrationSamples;
      }
    }
    startCalibration(slot, target);
  } else if (strcasecmp(command, "cancel") == 0) {
    cancelCalibration("Calibration cancelled.");
  } else if (strcasecmp(command, "testntfy") == 0) {
    if (!sendNtfy("Test boutons", "Test notification Arduino GIGA")) {
      appendPendingNotification("Test boutons", "Test notification Arduino GIGA");
    }
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
    printHelp();
  }
}

void pollSerial() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (serialLineLength > 0) {
        serialLine[serialLineLength] = '\0';
        executeSerialCommand(serialLine);
        serialLineLength = 0;
      }
      continue;
    }

    if (serialLineLength + 1 < sizeof(serialLine)) {
      serialLine[serialLineLength++] = c;
    }
  }
}

PanelButtonEvents pollPanelButton(PanelButtonState &button) {
  PanelButtonEvents events = {false, false};
  const unsigned long now = millis();
  const bool readingPressed = digitalRead(button.pin) == LOW;

  if (readingPressed != button.lastReadingPressed) {
    button.lastReadingPressed = readingPressed;
    button.lastReadingChangedMs = now;
  }

  if (now - button.lastReadingChangedMs >= kPanelDebounceMs &&
      readingPressed != button.stablePressed) {
    button.stablePressed = readingPressed;
    if (button.stablePressed) {
      button.pressedSinceMs = now;
      button.longHandled = false;
    } else {
      if (!button.longHandled && button.pressedSinceMs > 0) {
        events.shortPressed = true;
      }
      button.pressedSinceMs = 0;
      button.longHandled = false;
    }
  }

  if (button.stablePressed &&
      !button.longHandled &&
      button.pressedSinceMs > 0 &&
      now - button.pressedSinceMs >= kPanelLongPressMs) {
    events.longPressed = true;
    button.longHandled = true;
  }

  return events;
}

void startPanelCalibration() {
  const int index = selectedButtonIndex();
  if (index < 0) {
    Serial.println("Panel calibration ignored: selection is 0.");
    startStatusBlink(3, false);
    return;
  }

  startCalibration(buttons[index].slot, settings.calibrationSamples);
}

void pollControlPanel() {
  const PanelButtonEvents calEvents = pollPanelButton(calibrationPanelButton);
  const PanelButtonEvents nextEvents = pollPanelButton(nextPanelButton);
  const PanelButtonEvents previousEvents = pollPanelButton(previousPanelButton);

  if (calEvents.longPressed) {
    if (calibrationActive) {
      cancelCalibration("Calibration cancelled by panel.");
    } else {
      startPanelCalibration();
    }
  }

  if (!calibrationActive) {
    if (nextEvents.shortPressed) {
      stepPanelSelection(1);
    }
    if (previousEvents.shortPressed) {
      stepPanelSelection(-1);
    }
  }
}

void periodicNetworkWork() {
  const unsigned long now = millis();
  if (settings.autoNtp && !timeSynced && now > 5000) {
    syncTimeWithNtp();
  }
  if (settings.autoQueueFlush && now - lastQueueFlushMs > 30000UL) {
    lastQueueFlushMs = now;
    flushPendingNotifications();
  }
}

void periodicStatusPrint() {
  const unsigned long now = millis();
  if (now - lastStatusPrintMs > 30000UL) {
    lastStatusPrintMs = now;
    printStatus();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  pinMode(LED_BUILTIN, OUTPUT);
  setStatusLed(false);
  pinMode(kCalibrationButtonPin, INPUT_PULLUP);
  pinMode(kNextButtonPin, INPUT_PULLUP);
  pinMode(kPreviousButtonPin, INPUT_PULLUP);
  for (uint8_t i = 0; i < kBinaryLedCount; i++) {
    pinMode(kBinaryLedPins[i], OUTPUT);
    digitalWrite(kBinaryLedPins[i], LOW);
  }

  Serial.println();
  Serial.print(kFirmwareName);
  Serial.print(" ");
  Serial.println(kFirmwareVersion);

  mountUsbStick();
  loadButtons();
  loadRuntimeConfig();
  loadTemplates();
  printButtonList();
  printHelp();
  clampPanelSelection();

  startAdc();
}

void loop() {
  pollAudio();
  pollSerial();
  pollControlPanel();
  periodicNetworkWork();
  periodicStatusPrint();
  updateStatusLed();
}
