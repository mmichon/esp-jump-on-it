# Mini Jump/Social Bicycles Display

![Hardware](screenshot.jpg?raw=true)

This project implements a tiny display that shows the distance to (in minutes it'll take to walk), and charge level of, the nearest two Jump Bike / Social Bicycles bikes. It measures 1" x 1" and costs <$10USD.

## Hardware Needed
* ESP8266 microcontroller, like a [Wemos D1 Mini](https://www.aliexpress.com/store/product/WEMOS-D1-mini-Pro-16M-bytes-external-antenna-connector-ESP8266-WIFI-Internet-of-Things-development-board/1331105_32724692514.html)
* SSD1036 OLED display, like a [Wemos OLED Shield](https://www.wemos.cc/product/oled-shield.html). You can stack this shield on top of the Wemos D1.
* Wifi connection

## Configuration
1. Sign up for a Social Bicycles developer account at  https://app.socialbicycles.com/developer/. Grab your access key and put it into ```config.h```.
1. Set ```auth``` in ```config.h``` to be your base64-encoded Social Bicycles API ```username:password```.
1. Use Google Maps to find the coordinates of the location that you want to find the nearest bikes from and set ```lat``` and ```lon``` parameters in ```config.h```.
1. Turn the device on, and wait a few minutes for the ```Jump On It``` SSID to show up, then connect to it. A few seconds later, a captive Wifi connection screen should show up and you can configure the device's Wifi settings. It'll remember them from that point on.
1. Wait for it to reset and you should see updates every minute.
