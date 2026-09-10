# Tobers Multidisplay XXL
Tobers Multidisplay XXL for ESP32 and MAX7219 8x8 LED matrix modules is the expert version of Tobers Multidisplay.<br>
In addition to the well known features of Tobers Multidisplay it can also show graphic animations. Besides that there are lots of optimizations and nearly everything is configurable during runtime; you can even use different news and weather APIs.<br><br>
**Note:** You should be familiar with "Tobers Multidisplay" and only try out this new version if you know how the program works and how to set up everything. As this software is "experts only" there is no support. This is not a fork but a completely new version, so updating via OTA does not work. You have to upload boath code and files.<br><br>
<img src="showcase/Multidisplay_final.jpg" width="500">
<br><br>
**About**<br>
Always keeping Tobers Multidisplay up to date I considered adding new functions and optimizations to it. But as I don't want to drop support for ESP8266 and as I don't want to make it much more complicate for the may users that like my display very much, I've decided to set up a completely new project.<br>
(Still in pre AI era) I began with integrating the graphic animations of the [MAX72xx Library by majicDesigns](https://github.com/MajicDesigns/MD_MAX72XX). After that I got more and more ideas and now there is a program with lots of new features and even more optimizations under the hood. Finally, during the last month, I used some AI help for fine tuning and adding some features like multiple file upload. Going through the code you will still see the base from original Multidisplay on which a mighty programm has been built.

**New features:**
- graphic animations
- service Messages (Example: "News Service" is shown before news message)
- differents APIs, all configurable during runtime
- API-Logs
- optional combined date and time mode for large displays
- optional time with seconds during message loop
- shuffle mode for text effects and graphic animations
- multiple file uplaod und download (SPIFFS), support of gzip files

**Under the hood:**
* Lots of optimizations (Strings, Wifi, Time, dynamic html) and code simplifications.
  
<br>

**Requirements**
* *Hardware:*<br>
Classic ESP32 (4MB Flash)<br>
Max7219 8x8 LED matrix modules (Meanwhile I use displays with up to 20 modules)<br>

* *Arduino IDE and the following libraries:*<br>
[MAX72xx Library by majicDesigns](https://github.com/MajicDesigns/MD_MAX72XX)<br>
[Parola Library by majicDesigns](https://github.com/MajicDesigns/MD_Parola)<br>
[Arduino Json library by Benoit Blanchon](https://github.com/bblanchon/ArduinoJson)<br>
[My fork of WifiManager library (development branch) by tzapu/tablatronix](https://github.com/ElToberino/WiFiManager_for_Multidisplay)<br>

* *Installed boards:*<br>
[ESP32 core for Arduino](https://github.com/espressif/arduino-esp32)<br>
Note: I highly recommend V 2.0.17.<br>

* *Recommended tools:*<br>
[ESP32 Data Upload Tool](https://github.com/me-no-dev/arduino-esp32fs-plugin)<br>

* *Required accounts:*<br>
WEATHER: personal api key from [openweathermap.org](https://openweathermap.org/)<br>
NEWS: personal api key from [newsapi.org/](https://newsapi.org/) and from [thenewsapi.com/](https://thenewsapi.com/) <br>
SPOTIFY: premium account AND [developer registration of your device](https://developer.spotify.com/dashboard/)
<br><br>

**Setup**<br>
All settings required have to be done on top of the code in the /// USER SETTINGS /// section. All other configuration is done during runtime via config.html.

**Spotify**<br>
Please note that Spotify has recently limited the lifetime of the refreshtoken; it expires after 180 days. That means you have to do the authentication process again after this time.<br>
If you have problems connecting to Spotify, please check if the TLS certificates (saved in *cert_spot.txt* and *cert_spot_api.txt*) have changed. You can find this out with your browser: Go to *accounts.spotify.com* and *api.spotify.com*, click the key symbol in the address bar and compare the certificates. If they are different, change the files and upload them (a restart is required after that).
<br><br>
**FAQ**<br>
Why don't you provide a tutorial for setting up the display?<br>
*This is the expert version of my Multidisplay. Users should know how Tobers Multidisplay works and understand what's happening under the hood before using this expert version. Please not that there is no support for this expert version.*<br>
<br>
Where are the English html files?<br>
*To keep it simple I do not provide two versions of the html files. As an expert you could easily adopt the htnl files; but this is not necessary as they are self-explaining. Of course the English language version for the messages on display is still implemented.*<br>
<br>
Why can I choose on "Musikinfo" between Spotify, Castweb and Info extern?<br>
*The support of Castweb is experimental and not recommended. It bases on the [cast-web-api by vervallsweg](https://github.com/vervallsweg/cast-web-api) which is no longer maintained and very difficult to set up. It's better to just ignore this function. Info extern is a simple API of my program: You can send a simple json string containing "Artist - Song" to "DEVICE_IP/musicInfo" and the display will show this string as music information. Take a look at the code for further information.*<br>
<br>
Which API do you prefer?<br>
*As always - it depends.. Take a look at the API features and decide what fits better for your perposes. Also keep in mind the daily call limits of the APIs.<br>
Concerning weather you should find out, which API delivers the most precise current weather and forecast for your location. In my use case, open-meteo is more precise, but that depends on your location.*<br>
<br>
**Credits**<br>
This project wouldn't have been possible without the work of many others:
* Special thanks to Marco Colli (MajicDesigns) for his libraries, the excellent documentation and the support via arduino forum
* Special thanks to Benoit Blanchon for his great Arduino Json Library and his friendly support
* Local language concept and some parts of weather functions inspired by ericBcreator and his [really nice display project](https://www.hackster.io/ericBcreator/1024-led-matrix-wifi-message-board-with-menu-web-interface-1b2666)
* SPIFFS administration taken and adopted from the great Arduino ESP website https://fipsok.de/ by Jens Fleischer
* HTML background pattern graphic by Henry Daubrez, taken from http://thepatternlibrary.com/ <br>

Thanks to the many, many other programmers and enthusiasts in the web whose work and helpfulness enabled me to realize such a project.<br>

