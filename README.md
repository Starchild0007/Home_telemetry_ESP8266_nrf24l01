# Home_telemetry_ESP8266_nrf24l01
Basic home telemetry and remote light switcing, server using ESP8266 and nrf24l01.
Meteo station is connected with ESP8266 using nrf24l01 and sends temperature, humidity, and atmospheric pressure.
###Used sensors:
DHT11 is used for humidity redings, BMP180 is used for temperature and atmospheric pressure reading, information is send every 5 minutes.


Readings are stored every hour for 12 hours, all data is presented in JSON format on HTTP server.
####Example:
```sh
{
        "Home_Telemetry": {
        "Temperature": {
            "-outside": "23.9",
            "-index": 9,
            "-Last12Hours": ["22.6", "23.2", "23.3", "23.5", "23.7", "23.7", "23.7", "23.7", "23.7", "23.8", "23.5", "23.4"]
        },
        "Humidity": {
            "-outside": "33",
            "-index": 9,
            "-Last12Hours": ["36", "36", "35", "34", "33", "33", "33", "33", "32", "33", "40", "38"]
        },
        "Pressure": {
            "-outside": "983",
            "-index": 9,
            "-Last12Hours": ["982", "983", "983", "983", "983", "983", "983", "983", "983", "983", "982", "982"]
        }
    }
}
```

Pinout for connecting nRF module 
####nRF24L01 - ESP8266 12


  - 1 GRND
  - 2 VCC
  - 3 CE   - GPIO4
  - 4 CSN  - GPIO15
  - 5 SCK  - GPIO14
  - 6 MOSI - GPIO13
  - 7 MISO - GPIO12
  - 8 IRQ  - NC


Parameter "-outside" is updated every 5 minutes.
Also ESP8266 + solid state relay used for wireless light switching on a balcony. Array of MSP430 + nrf24l01 are used as a button.

####nRF24L01 - MSP430 Launchpad


  - 3 CE   - P2.0:
  - 4 CSN  - P2.1:
  - 5 SCK  - P1.5:
  - 6 MOSi - P1.7:
  - 7 MISO - P1.6:
  - 8 IRQ  - P2.2:
