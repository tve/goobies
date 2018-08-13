Simple JNZ5-based temperature probe with RFM69 (sx1231) radio
=============================================================

Components
----------

- JNZ4/JNZ5 STM32L082 with RFM69 (sx1231) radio and MCP9808 temperature sensor
- Small LiPo battery

Features
--------

- reports the temperature to a central GW via RF
- adjusts reporting interval based on temperature change threshold as well as ACK success
- adjusts TX power to minimize battery drain
- reports rf and battery stats
- shuts down when battery reaches low threshold to preserve LiPo life
