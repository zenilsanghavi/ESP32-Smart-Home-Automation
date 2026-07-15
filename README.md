🏠 Voice-Controlled Smart Home Automation (ESP32)

A complete IoT home automation system built using an ESP32 (Type-C). This project enables dual-control of 4 household appliances (like lights and fans) through a localized Web UI dashboard and globally via voice commands using Amazon Alexa and Google Home.

> ⚠️ IMPORTANT NETWORK REQUIREMENT: 4G HOTSPOT
> This project is configured to operate on a standard 4G Mobile Network or Personal Hotspot. 
> It will not work on institutional, university, or corporate Enterprise Wi-Fi networks (WPA2-Enterprise) because they typically use captive web-login portals that microcontrollers cannot bypass, and their firewalls block the inbound/outbound ports required for the Sinric Pro IoT cloud.

✨ Project Highlights
Dual-Control Architecture: Seamlessly switch between the local Web UI and Cloud-based Voice Assistants.
Voice Integration: Fully integrated with Amazon Alexa and Google Assistant using the Sinric Pro platform.
Responsive Local Dashboard: Futuristic HTML/CSS frontend hosted directly on the ESP32's local web server.
Real-Time Sync: State changes made via voice commands instantly update on the Web UI.

🛠️ Hardware Components
ESP32 Development Board (Type-C)
4-Channel Relay Module (5V, Active-Low)
5V Power Adapter
LEDs & 5Ω Resistors
Breadboard & Jumper Cables

💻 Software & Tech Stack
Language: C++
IDE: Arduino IDE
IoT Platform: Sinric Pro
Voice Assistants: Amazon Alexa, Google Home
Libraries: `WiFi.h`, `WebServer.h`, `SinricPro.h`, `SinricProSwitch.h`

🚀 Setup & Installation

1. Sinric Pro Configuration
 Create an account at [Sinric Pro](https://sinric.pro/).
 Add 4 new "Smart Switch" devices.
 Note down your `APP_KEY`, `APP_SECRET`, and the 4 `DEVICE_ID`s.

2. Flashing the Code
 Clone this repository.
 Open `esp32_combined_relay.ino` in the Arduino IDE.
 Update the network credentials and Sinric Pro keys:
  cpp
  #define WIFI_SSID    "YOUR_4G_HOTSPOT_NAME"      
  #define WIFI_PASS    "YOUR_HOTSPOT_PASSWORD"  

  #define APP_KEY      "YOUR_APP_KEY"
  #define APP_SECRET   "YOUR_APP_SECRET"


📱 How to Use
Turn on your dedicated 4G Mobile Hotspot.

Power the ESP32 via the 5V adapter and wait for it to connect.

Web Control: Find the ESP32's local IP address (via the Arduino Serial Monitor) and enter it into any browser connected to the same hotspot.

Voice Control: Discover the devices in your Alexa or Google Home app and use commands like: "Alexa, turn on the bedroom light."

Diagram
That's the rough layout of your setup, bro. Quick summary of what's going on:

- ESP32 sends GPIO control signals to the relay's IN1–IN4 pins, and separate GPIO pins drive your 4 status LEDs (each through its own resistor) on the breadboard.
- The 5V adapter powers the relay module's VCC + GND independently — this is the important bit, since relays (especially when switching real loads) can pull more current than the ESP32's onboard 5V/3.3V regulator can safely supply.
- Important: the adapter's GND must still be tied to the ESP32's GND (common ground) even though it's a separate power source — otherwise the relay won't switch reliably when triggered from the ESP32.
