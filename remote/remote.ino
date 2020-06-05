#include <RCSwitch.h>
#include <EEPROM.h>
#include <IRremote.h>

const int RECV_PIN = 11;
const int REPEAT_DELAY_RECV = 110;
const int REPEAT_DELAY_SEND = 42;
const int NONE              = 0;
const int RC_MAX_PULSE_GAP  = 300;
const int RC_PULSE_CYCLE    = 7;
const unsigned int RC_PROTOCOL = 1;
const unsigned int POWERED_ON_STORAGE = 0;
const unsigned long RC_POWER_ON       = 0x00000551;
const unsigned long RC_POWER_OFF      = 0x00000554;
const unsigned long YAMAHA_POWER      = 0x7E81542B;
const unsigned long YAMAHA_PHONO      = 0x5EA12857;
const unsigned long YAMAHA_DOCK       = 0xFE80522D;
const unsigned long YAMAHA_CD         = 0x5EA1A8D7;
const unsigned long YAMAHA_LINE1      = 0x5EA183FC;
const unsigned long YAMAHA_LINE2      = 0x5EA11867;
const unsigned long YAMAHA_LINE3      = 0x5EA198E7;
const unsigned long YAMAHA_TUNER      = 0x5EA16817;
const unsigned long YAMAHA_VOLUMEUP   = 0x5EA15827;
const unsigned long YAMAHA_VOLUMEDOWN = 0x5EA1D8A7;
const unsigned long YAMAHA_MUTE       = 0x5EA13847;
const unsigned int VOLUME_MAX = 153;
const unsigned int VOLUME_MIN = 0;

struct SoundSrc {
  unsigned long key;
  unsigned int volume;
};

const SoundSrc SOURCES[] = {
  {YAMAHA_PHONO, 35},
  {YAMAHA_DOCK,  0},
  {YAMAHA_CD,    40},
  {YAMAHA_LINE1, 50},
  {YAMAHA_LINE2, 50},
  {YAMAHA_LINE3, 40},
  {YAMAHA_TUNER, 40}
};
const unsigned int SOURCE_LENGTH = 7;
const unsigned int MUTE_VOLUME = 0;
const unsigned int STARTING_SOURCE = 4;
const unsigned int SECONDARY_SOURCE = 3;
unsigned int currVolume = SOURCES[STARTING_SOURCE].volume;
boolean poweredOn;
boolean keySent;
unsigned long lastRcRecvMillis;
unsigned long rcPulseCount;
unsigned long lastIrRecvMillis;
unsigned long lastIrRecvKey;
IRrecv irrecv(RECV_PIN);
IRsend irsend;
RCSwitch rcswitch = RCSwitch();
decode_results results;

/**
 * Handle an RC key push. Long pushes of the RC_POWER_ON button are recorded and handled by handleRCPulse
 */
void handleRC(unsigned long key) {
  if (key == RC_POWER_ON)  { 
    if(millis() - lastRcRecvMillis > RC_MAX_PULSE_GAP) {
      rcPulseCount = 0;
    }
    handleRCPulse(rcPulseCount++/RC_PULSE_CYCLE);
    lastRcRecvMillis = millis();
  }
  else if (key == RC_POWER_OFF) { setPower(false); }
}

/**
 * Cycle through the sources depending on how long the RC_POWER_ON button has been held down
 */
void handleRCPulse(unsigned int count) {
  if(count == 0) {
   setPower(true);
  } else {
   setSource(SECONDARY_SOURCE);
  } 
}

/**
 * Process a received IR signal and notify handleKey. If a REPEAT ir signal was received, this is translated
 * in to the last valid key press that was detected (as long as it was not received too long ago).
 */
void handleIR(decode_results *results) {
  if (results->decode_type == NEC) {
    unsigned long value = results->value;
    if (value == REPEAT && lastIrRecvKey != NONE && millis() - lastIrRecvMillis < REPEAT_DELAY_RECV) {
      handleKey(lastIrRecvKey, true);
    } else if (value != REPEAT) {
      handleKey(lastIrRecvKey = value, false);
    } else {
      lastIrRecvKey = NONE;
    }
    lastIrRecvMillis = millis();
  }
}

/**
 * Handle a received IR key press. If the hifi is currently powered off, all key presses will be ignored (other
 * than a push of the power key).
 */
void handleKey(unsigned long key, boolean isRepeat) {
//  Serial.print("Key Pressed: 0x");
//  Serial.print(key, HEX);
//  Serial.print(" Power: ");
//  Serial.print(poweredOn);
//  Serial.print(" Volume: ");
//  Serial.println(currVolume);
 
  if (key == YAMAHA_POWER && !isRepeat) { setPower(!poweredOn); }// Toggle power
  else if (poweredOn) {
    if (isRepeat) { // Use the first repeat signal for volume adjustment
      if      (key == YAMAHA_VOLUMEUP)   { volUp(); }
      else if (key == YAMAHA_VOLUMEDOWN) { volDown(); }
    } else if (key == YAMAHA_MUTE)       { setVolume(MUTE_VOLUME); }
    else {
      // Set the source based upon the key press then break out of loop
      for (int i = 0; i < SOURCE_LENGTH; i++) {
        if (key == SOURCES[i].key) { return setSource(i); }
      }
    }
  }
}

/**
 * Turn the hifi on or off based upon the passed in value. Store the current powered on value in the EEPROM for
 * retrieval after power loss. This function will return the hifi to the initial source and volume.
 */
void setPower(boolean on) {
  if (poweredOn) {
    setSource(STARTING_SOURCE);
    delay(REPEAT_DELAY_SEND);
  }
  if (poweredOn != on) {
    sendIR(YAMAHA_POWER);
    EEPROM.write(POWERED_ON_STORAGE, poweredOn = on);
  }
}

/**
 * Set the hifi source to the source represented within the SOURCES array. Adjust the volume
 * to the default level for this source.
 */
void setSource(unsigned int src) {
  sendIR(SOURCES[src].key);
  setVolume(SOURCES[src].volume);
}

/**
 * Adjust the volume to the target level
 */
void setVolume(unsigned int target) {
  int delta = target - currVolume;
  unsigned long button = (delta > 0) ? YAMAHA_VOLUMEUP: YAMAHA_VOLUMEDOWN;
  for (int i=0; i < abs(delta); i++) {
    sendIR(button);
    delay(REPEAT_DELAY_SEND);
  }
  currVolume = target;
}

void volUp() {
  if (currVolume < VOLUME_MAX) {
    currVolume++;
    sendIR(YAMAHA_VOLUMEUP);
  }
}

void volDown() {
  if (currVolume > VOLUME_MIN) {
    currVolume--;
    sendIR(YAMAHA_VOLUMEDOWN);
  }
}

/**
 * Perform an IR key send to the hifi
 */
void sendIR(unsigned long key) {
  irsend.sendNEC(key, 32);
  keySent = true;
}

/**
 * Check to see if an IR signal has been received
 */
void receiveIR() {
   if (irrecv.decode(&results)) { 
    irrecv.resume(); // Receive the next value
    handleIR(&results);    
  }
}

/**
 * Briefly turn on the RC receiver and check to see if a key has been pressed
 */
void receiveRC() {
  rcswitch.enableReceive(0);
  if (rcswitch.available()) {
    if (rcswitch.getReceivedProtocol() == RC_PROTOCOL) {
      handleRC(rcswitch.getReceivedValue());
    }
    rcswitch.resetAvailable();
  }
  rcswitch.disableReceive();
}

/**
 * Check to see if an IR signal has been sent. If so, reenable the IR receiver.
 */
void reset() {
  if (keySent) {
    keySent = false;
    irrecv.enableIRIn();
  }
}

void setup() {
  //Serial.begin(115200);
  poweredOn = EEPROM.read(POWERED_ON_STORAGE) == true;
  irrecv.enableIRIn();
}

void loop() {
  receiveIR();
  receiveRC();  
  reset();
}
