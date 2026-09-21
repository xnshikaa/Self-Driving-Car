// =====================================================================
//  TEST 1 — "Hello ESP32": blink the LED and print messages
// =====================================================================
//  Goal: prove that your laptop can upload code to the ESP32.
//  What you should see:
//    * the small blue LED on the board blinks once per second
//    * Tools > Serial Monitor (115200 baud) prints "Hello from ESP32!" lines
//  If upload fails: hold the BOOT button while "Connecting..." appears.
// =====================================================================
const int LED_PIN = 2;   // the blue LED on most ESP32 DevKit V1 boards
int counter = 0;
void setup() {
  Serial.begin(115200);          // open the USB "chat line" to your laptop
  pinMode(LED_PIN, OUTPUT);      // this pin will OUTPUT electricity to the LED
  delay(500);
  Serial.println();
  Serial.println("Hello from ESP32!");
  Serial.printf("Chip model: %s, %d CPU cores, %u MB flash\n", ESP.getChipModel(), ESP.getChipCores(),
                (unsigned)(ESP.getFlashChipSize() / (1024 * 1024)));
}
void loop() {
  digitalWrite(LED_PIN, HIGH);   // LED on
  delay(500);                    // wait half a second
  digitalWrite(LED_PIN, LOW);    // LED off
  delay(500);
  counter++;
  Serial.printf("Hello from ESP32! I have blinked %d times.\n", counter);
}
