
//    *********************************************
//
//    TOBERS MULTIDISPLAY XXL
//    FOR ESP32
//
//    V 0.8.6 - 16.09.2026
//
//    *********************************************
//
//    For further information see: https://github.com/ElToberino/Tobers_Multidisplay_XXL
//
//    Copyright (c) 2020-2026 Tobias Schulz
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//    ***************************************************************
//
//    successfully compiled with ARDUINO IDE 1.8.19
//
//    required board installation:
//
//    - ESP32 core for Arduino -> https://github.com/espressif/arduino-esp32           successfully compiled with V 2.0.17 (V 3++ also working)
//        -->  V 2.0.17 recommended
//
//    required libraries:
//    - MAX72xx Library by majicDesigns -> https://github.com/MajicDesigns/MD_MAX72XX             successfully compiled with V 3.5.1
//    - Parola Library by majicDesigns -> https://github.com/MajicDesigns/MD_Parola               successfully compiled with V 3.7.5
//    - Arduino Json library by Benoit Blanchon -> https://github.com/bblanchon/ArduinoJson       successfully compiled with V 7.4.3
//    - my fork of WifiManager library (development branch) by tzapu/tablatronix -> https://github.com/ElToberino/WiFiManager_for_Multidisplay
//
//    reqired accounts/api keys:
//    - WEATHER: personal api key from https://openweathermap.org/
//    - NEWS: personal api keys from https://newsapi.org/ and https://thenewsapi.com/
//    - SPOTIFY: premium account AND developer registration of your device -> https://developer.spotify.com/dashboard/
//
//    required files:
//    - all files delivered with this ino.file are required! -> the files in folder data must be uploaded on SPIFFS
//
//    The included certificate file (cert_spot.txt) is a G2 root certificate from Starfield Technologies, Inc.
//    The included certificate file (cert_spot_api.txt) is a G2 global root certificate from DigiCert Inc.
//    The included certificate file (cert_news.txt) is a GTS Root R4 certificate from Google Trust Services LLC
//    They may expire and need to be updated.
//    They can be found by clicking the lock symbol in address field of your browser calling "accounts.spotify.com" and "api.spotify.com".
//
//    ****************************************************************
//
//    Credits:
//    - Special thanks to Marco Colli (MajicDesigns) for his libraries, the excellent documentation and the support via arduino forum.
//    - Special thanks to Benoit Blanchon for his great Arduino Json Library and his friendly support.
//    - Local language concept and some parts of weather functions inspired by ericBcreator -> https://www.hackster.io/ericBcreator/1024-led-matrix-wifi-message-board-with-menu-web-interface-1b2666
//    - SPIFFS administration taken and adopted from the great Arduino ESP website https://fipsok.de/ (Jens Fleischer)
//    - HTML background pattern graphic by Henry Daubrez, taken from http://thepatternlibrary.com/
//    - Thanks to the many, many other programmers and enthusiasts whose work and helpfulness enabled me to realize such a project
//
//    ***************************************************************
//
//    PLEASE NOTICE THE COMMENTS and set the #defines and USER SETTINGS according to your needs.
//
//    ***************************************************************



//#define DEBUG                                // show debug messages in serial monitor
//#define LOGFILE                              // logfile DEBUG  -> If you want to use this, remember the file size enlarges with every API call !!!

//#define DANI                                 // JUST IGNORE THAT, as this has is private and of no use for you. (special values for Danis Display (7 modules))

#define  OTA                                    // enables Over the Air Update (OTA) via web browser
#define SPOTIFY                                 // enables call of Spotify API
#define CASTWEB                                 // enables call of Google Cast API
//#define NEWS_SECURE                           // uses encryption certificate for thenewsapi.com


// Core ESP32
#include <WiFi.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>

// Networking ESP32
#include <WiFiClient.h>
#include <WiFiClientSecure.h>                  // required for ESP32 Core > 3
#include <WebServer.h>
#include <HTTPClient.h>
#include <DNSServer.h>

// OTA ESP32
#include <Update.h>

// Other Libraries
#include <ArduinoJson.h>
#include <StreamUtils.h>                        // required for chunked input handling
#include <WiFiManager.h>                        // WiFiManager_for_Multidisplay
#include <MD_Parola.h>                          // Parola Library by majicDesigns
#include <MD_MAX72xx.h>                         // MAX72xx Library by majicDesigns

#ifdef SPOTIFY
  #include <base64.h>
#endif

// Local files
#include "fontfiles/Font_Data_Numeric.h"
#include "fontfiles/Font_Data_Tob_UTF8.h"

#ifndef ARRAY_SIZE
  #define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))
#endif




//// ------ USER SETTINGS SECTION ------ ////

// --- PAROLA DISPLAY --- //
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW       // define LED Matrix Hardware  (options: PAROLA_HW, GENERIC_HW, ICSTATION_HW, FC16_HW) see: https://forum.arduino.cc/index.php?topic=560635.0 
#define MAX_DEVICES 8                           // Number of 8x8 modules

#define CLK_PIN   18   // CLK                     
#define DATA_PIN  23   // MOSI                        
#define CS_PIN     5   // CS


// --- LDR --- //
#define Analog_Pin 34                             // ESP32 ADC1 Pins GPIO 32, 33, 34, 35, 36, 39

// --- LANGUAGE --- //
#define  LOCAL_LANG                               // enable local language (German), comment out for English

// --- WEATHER --- //
bool imperial = false;                            // set to true if you want to receive weather data with imperal unit for temperature "Fahrenheit". Default is metric "Celsius".

// --- TIME --- //
const char* timezone =  "CET-1CEST,M3.5.0/02,M10.5.0/03";       // = CET/CEST  --> for adjusting your local time zone see: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


// --- API KEYS --- //
/// openweathermap.org ///
  const char* WeatherApiKey = "xxx";        									// personal api key necessary for retrieving the weather from openweathermap
																				// open-meteo doesn't require an api key
/// newsapi.org /// 
  const char* NewsApiKey_0 = "xxx";               								// personal api key for newsapi.org

/// thenewsapi.com ///                                                         
  const char* NewsApiKey_1 = "xxx";												// personal api key for thenewsapi.com

/// spotify.com ///
  const char* clientID = "xxx";                                      			// register your device on https://developer.spotify.com/dashboard/
  const char* clientSecret = "xxx";


//// ------ END OF USER SETTINGS SECTION ------ ////





/// DEFINITIONS ///

///// PAROLA DISPLAY ////

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_MAX72XX *mx;                                    // graphic instance


//// LDR ////

bool auto_intens = false;                          // automatic intensity adjustment
uint8_t intens_min = 0;                            // min brightness (0-15)
uint8_t intens_max = 15;                           // max brightness (0-15)
uint16_t ldr_interval = 250;                       // interval for checking the light sensor (in milliseconds)
uint8_t ls_triggerStep = 1;                        // trigger intensity setting (set to 2 if flickering occurs -> note that min and max brightness can not be reached with a value of 2)
unsigned long last_LDR_Millis = 0;                 // millis for LDR check interval          


//// WIFI ////

#ifdef DANI
  const char* AP_NAME = "Danis_Display";              // parameters of wifimanager configportal and/or operation as Access Point without WiFi connection
#else
  const char* AP_NAME = "Tobers_Multidisplay_XXL";    // parameters of wifimanager configportal and/or operation as Access Point without WiFi connection
#endif

const char* AP_PW = "";                               // set a password here if you don't want the AP to be open -> at least 8 characters !

IPAddress AP_IP(192,168,4,1);                         // NOTE: This manual IP-Setting doesn't work with ESP32 if SPOTIFY is defined! Default IP is 192.168.4.1
IPAddress AP_Netmask(255,255,255,0);
uint8_t static_Mode_enabled = 0;

bool wifiManagerWasCalled = false;                    // is set to true if WifiManager is executed on startup
bool AP_established = false;                          // is set to true if WifiManager was skipped anddevice operates as AP
unsigned long lastWifiReset = 0;                      // timer for periodical wifi reset 

#ifdef DANI
  const char replaceH1[] = "<script>document.addEventListener('DOMContentLoaded', () => {document.getElementsByTagName('h1')[0].innerHTML ='Danis Multidisplay';})</script>";
#else
  const char replaceH1[] = "<script>document.addEventListener('DOMContentLoaded', () => {document.getElementsByTagName('h1')[0].innerHTML ='Tobers Multidisplay XXL';})</script>";
#endif


///// TIME SERVER AND DATE ////

char ntpServer[41] = "pool.ntp.org";                                // "2.europe.pool.ntp.org"; //"pool.ntp.org"; //"fritz.box";             // server pool prefix "2." could be necessary with IPv6
struct tm tm;
#if ESP_ARDUINO_VERSION_MAJOR < 3
  extern "C" uint8_t sntp_getreachability(uint8_t);              // shows reachability of NTP Server (value != 0 means server could be reached) see explanation in http://savannah.nongnu.org/patch/?9581#comment0:
#else
  #include <esp_sntp.h>
#endif
           

#ifdef LOCAL_LANG
  const char* const PROGMEM days[] { "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" } ;
  const char* const PROGMEM months[] { "Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" } ;
  const char* const PROGMEM monthsXL[] { "Jänner", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" } ;
#else
  const char* const PROGMEM days[] { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" } ;
  const char* const PROGMEM months[] { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" } ;
  const char* const PROGMEM monthsXL[] { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" } ;
#endif

uint8_t enableTime = 1;                       // internal variable set via webinterface
uint8_t enableDate = 0;                       // internal variable set via webinterface
uint8_t enableTimeXXL = 1;                    // internal variable set via webinterface
uint8_t timeDuration = 7;                     // duration of time display in seconds, variable set via webinterface                                                               

unsigned long previoustimecall = 0;                                                      
uint8_t timeRefreshInterval = 60;             // interval of time server call in minutes set via webinterface                                                    

uint8_t lastDayRun = 0;                       // day of last makeDate() run

//#define SHORTDATE                        // comment this out if you prefer a shorter date display: "1 Mar 2020" instead of "Sunday 1 March 2020" 


//// WEATHER API ////                                                   // Writes results in table "maintable.wetcur" 

#define NUM_OF_CITIES 2                                                 // global define for number of cities                                            

const uint8_t numberofcities = NUM_OF_CITIES;                           
unsigned long previousweathercall = 0;                                  // internal variable needed for refresh timer
char dayAfterTomorrow[12];                                              // shows the day after tomorrow in weather forecast
uint8_t weatherRefreshInterval = 20;                                    // interval of weather update in minutes set via webinterface
bool weathEverLoaded = false;                                           // is set to true if getWeatherData() is executed for the first time
uint8_t enableWeath = 1;                                                // internal variable set via webinterface
uint8_t weathApiChoice = 0;                                             // defines which Weather Api is used -> 0 = openweathermap.org, 1 = open-meteo.com, 99 = alternating
bool weathApiChangeFlag = true;                                         // internal flag
                                    
/// openweathemap.org ///                                               // Arrays for Weather Api 0
char cityName_A[numberofcities][51];                                    // city name
char Lat_A[numberofcities][21];                                         // latitude
char Lon_A[numberofcities][21];                                         // longitude

/// open-meteo.com ///                                                  // Arrays for Weather Api 1
char cityName_B[numberofcities][51];                                    // city name
char Lat_B[numberofcities][21];                                         // latitude
char Lon_B[numberofcities][21];                                         // longitude

#ifdef LOCAL_LANG 
  const char* const PROGMEM windDirection[] {
    "Nord", "Nord-Nordost", "Nordost", "Ost-Nordost", "Ost", "Ost-Südost", "Südost", "Süd-Südost",
    "Süd", "Süd-Südwest", "Südwest", "West-Südwest", "West", "West-Nordwest", "Nordwest", "Nord-Nordwest"
  };
  const String weatherLanguage = "DE";                                  // define local language for openweather
#else
  const char* const PROGMEM windDirection[] {
    "north", "north-northeast", "northeast", "east-northeast", "east", "east-southeast", "southeast", "south-southeast",
    "south", "south-southwest", "southwest", "west-southwest", "west", "west-northwest", "northwest", "north-northwest"
  };
  const String weatherLanguage = "EN";
#endif


////   NEWS API   ////                                              // Writes results in table "maintable.newscur" and "maintable.newslink" 
                                                                    
#define NUM_OF_NEWSSOURCES 4                                        // global define for number of newssources
#define NUM_OF_ARTICLES 3                                           // global define for number of articles per newssource

uint8_t newsApiChoice = 0;                                          // defines which News Api is used -> 0 = newaspi.org, 1 = thenewsapi.com, 99 = alternating
bool apiChangeFlag = true;                                          // internal flag

#ifdef NEWS_SECURE
  char cert_the_newsapi[1800];                                      // cerificate "GTS Root R4" für thenewsapi.com
#endif

const uint8_t newssources = NUM_OF_NEWSSOURCES;                     // number of news sources                             
const uint8_t numofarticles = NUM_OF_ARTICLES;                      // number of articles per news source
   
char ownNewsSources[newssources][51];                               // array for News Api 0                      
char ownNewsSources_A[newssources][51];                             // array for News Api 1       

bool newsEverLoaded = false;                                        // is set to true if getWeatherData() is executed for the first time
uint8_t enableNews1 = 1;                                            // internal variable set via webinterface
uint8_t enableNews2 = 1;                                            // internal variable set via webinterface
uint8_t enableNewsXL = 1;                                           // value 1 enables larger news arrays including news description; is set via webinterace config site
  
unsigned long previousnewscall = 0;                                 // internal variable needed for refresh timer
uint8_t newsRefreshInterval = 60;                                   // interval of news update in minutes set via webinterface


#if (NUM_OF_NEWSSOURCES*NUM_OF_ARTICLES) >= (NUM_OF_CITIES*2)       // tablesize dependent on global defines
const uint8_t tablesize = numofarticles*newssources;
#else                                                               
const uint8_t tablesize = numberofcities*2;
#endif


////  SPOTIFY  ////

uint8_t enableSpotify = 1;                                            // internal variable set via webinterface, enables calling Spotify for information about currently playing song (only ifdef Spotify)

#ifdef SPOTIFY
const char* redirectUri = "http://127.0.0.1:80/callback";             // redirect uri
char cert_spotify_account[1800];                                      // certificate necessary for https connection, loaded from file "certificate.txt" -> if certificate is expired update "certificate.txt" and upload it to SPIFFS
                                                                                                //  --> used certificate for accounts.spotify.com : " Starfield Root Certificate Authority - G2"
char cert_spotify_api[1800];                                          // certificate necessary for https connection, loaded from file "certificate.txt" -> if certificate is expired update "certificate.txt" and upload it to SPIFFS
                                                                                                //  --> used certificate for accounts.spotify.com : " Starfield Root Certificate Authority - G2"
String refreshtoken;                                                  // necessary for getting access token
String auth;                                                          // base64 encoded "client_id:client_secret"
String accesstoken;                                                   // necessary for calling information about currently playing song, valid until expire time has runs out
uint16_t expiretime = 1;                                              // expire time is set with every call for a new access token
unsigned long previousrefresh = 0;                                    // required for refresh timer
int remainingRefreshTokenDays = -1;                                   // days until refreshtoken expires
#endif


//// CAST WEB API ////

#ifdef CASTWEB
char castWebIP[16];                                                   // IP of network device running cast-web-api -> see: https://github.com/vervallsweg/cast-web-api
char castDeviceID[40];                                                // ID of active cast device
char musicSource[30];                                                 // Name of source playing on active cast device; radio station e.g.

char musicInfoBuf[500];
bool newMusicData = false;
#endif


////  LOG ////                                                       // Log of last API calls available via web page "api.html"

const uint8_t numOfLogs = 4;               

char logWeathStamp[numOfLogs][21];                                  // weather log
char logWeathSources[numberofcities*2*numOfLogs][51];
char logWeathHtml[numberofcities*2*numOfLogs][4];
char logWeathParse[numberofcities*2*numOfLogs][3];
uint8_t weathLogCounter = 0;
uint8_t logWeathType[numOfLogs];

char logNewsStamp[numOfLogs][21];                                   // news log
char logNewsSources[newssources*numOfLogs][51];
char logNewsHtml[newssources*numOfLogs][4];
char logNewsParse[newssources*numOfLogs][3];
uint8_t newsLogCounter = 0;
uint8_t logNewsType[numOfLogs];

const uint8_t numOfMusicLogs = 10;                                 // music log
char logMusic[numOfMusicLogs][70];
uint8_t musicLogCounter = 0;

const uint8_t numOfTimeLogs = 5;                                   // time server log
char timeServerLog[numOfTimeLogs][70];
uint8_t timeLogCounter = 0;


//// GRAPHIC ANIMATION ////                                       // internal values

const uint8_t numOfGraphics = 6;                                  
unsigned long prevTimeAnim;
uint8_t graphPlaytime = 5;
uint8_t graphPlaynum = 1;
bool graphfinished = false;
bool bInit = false;
uint8_t enableGraph[numOfGraphics];
uint8_t enableGraphTEMP[numOfGraphics];


////  WEBSERVER  ////

WebServer server(80);


////  AUTHENTICATION  ////                                          // set and changed via config.html       

uint8_t enableAuth = 1;                                             // enables html basic authentication (1 = enabled)                                           
char www_username[21] = "multi";                                    // credendtials for html sites -> must be entered in browser to reach certain html sites
char www_password[21] = "display";    

char fallback_username[21] = "patscher";                            // secret fallback credendtials for html sites
char fallback_password[21] = "kofel";                               // (if regular authentication information has been messed up for any reason)

////  OTA  ////                                                     // messages sent to browser if OTA successful or failed

#ifdef OTA
  #ifdef LOCAL_LANG
    const char OTAsuccessMsg[] PROGMEM = "<div style=\"border:0.1em solid #e1c3a9; margin:0; padding:0.5em;font-size:0.8em; font-family:sans-serif; text-align:center; color:white; background-color:green;\">Update erfolgreich</div>";
    const char OTAfailMsg[] PROGMEM = "<div style=\"border:0.1em solid #e1c3a9; margin:0; padding:0.5em;font-size:0.8em; font-family:sans-serif; text-align:center; color:white; background-color:red;\">Update fehlgeschlagen</div>";
  #else
    const char OTAsuccessMsg[] PROGMEM = "<div style=\"border:0.1em solid #e1c3a9; margin:0; padding:0.5em;font-size:0.8em; font-family:sans-serif; text-align:center; color:white; background-color:green;\">Update successful</div>";
    const char OTAfailMsg[] PROGMEM = "<div style=\"border:0.1em solid #e1c3a9; margin:0; padding:0.5em;font-size:0.8em; font-family:sans-serif; text-align:center; color:white; background-color:red;\">Update failed</div>";
  #endif
#endif


//// GLOBALS ////

// Message arrays

#define MSG_SIZE 500                   // global size for all custom messages

char infowifi[100];
char timeshow[35];                                            // array for time & date shown on display
char timeSaver[10];                                           // array for time
char dateSaver[30];                                           // array for date
char dateshow[30];                                            // array for date shown on display
char customServiceMsg[30];                                    // array for custom intro shown internally
char showcustomServiceMsg[30];                                // array for custom intro shown on display
char showweatherService[30];                                  // array for weather intro shown on display
char showweather[500];                                        // array for weather shown on display
char shownewsService[30];                                     // array for news1 intro shown on display
char shownewsService2[30];                                    // array for news2 intro shown on display
char shownews[500];                                           // array for news shown on display
char shownews2[500];                                          // array for news shown on display
  
char showspotifyService[30];                                  // array for Spotify intro shown on display
char spotify[500] = "Spotify is deactivated";                 // array for Spotify shown internally and sent to html page
char showspotify[500] = "no info yet";                        // array for Spotify shown on display
char spotifycover[200] = "/noSpot.jpg";                       // array for Spotify cover urls shown internally and sent to html page

char ownName[30];                                             // array for own name combined with own message
char ownmessage[MSG_SIZE];                                    // array for own message shown internally and sent to html page
char showownmessage[MSG_SIZE];                                // array for own message shown on display
char showownmsgService[30];                                   // array for own message intro shown on display

const uint8_t numOfMessages = 5;       

char messages[numOfMessages][MSG_SIZE] = {"Message A","Message B","Message C", "Message D", "Message E"};           // customizable messages array shown internally and sent to html page
char showmessages[numOfMessages][MSG_SIZE] = {"Message A","Message B","Message C", "Message D", "Message E"};       // customizable messages array shown on display
uint8_t enablemessage[numOfMessages] = { 1, 1, 1, 1, 1};                                                            // internal variables set via webinterface enable custom messages

uint8_t enableOwn = 1;                    // internal variable set via webinterface enables public user defined message
bool changeflag = true;                   // states if ownmessage has been deactivated during runtime

uint8_t presetMSG = 0;                    // currently chosen preset (saved in files) for messages (value 0 on startup, value 100 on starup in AP mode)


// Misc

char devicename[25] = "Tobers Multidisplay XXL";    // name of display shown on html pages configurable via web interface

uint16_t delayTime = 1000;                          // multiplier for delay before animation in ms
uint16_t pauseTime = 500;                           // multiplier for pause of animation in ms (time last frame stays on display)
      
uint8_t intens = 1;                                 // variable for display intensity
uint8_t globScrollSpeed = 4;                        // variable for global scroll speed (Infowifi, Date, News, Weather, Spotify, Ownmessage);
bool timeOnly = false;                              // if true display shows only time
bool graphOnly = false;                             // if true display shows only graphic animation
bool singleMsgOnly = false;                         // if true display constantly shows only one single message
bool msgUpdated = false;                            // internal flag       
unsigned long lastAnimation = 0;                    // replaces delay in loop

uint8_t enableServiceMsg = 1;                       // enables message intros globally

bool weatherUpdate = false;                         // flag if update is due
bool newsUpdate = false;                            // flag if update is due

// Own characters

uint8_t degreeC[] = {9, 3, 3, 0, 62, 65, 65, 65, 65, 0 };   // Degree Celsius character // online font-editor available at: https://pjrp.github.io/MDParolaFontEditor  
uint8_t degreeF[] = { 9, 3, 3, 0, 127, 9, 9, 9, 1, 0 };     // Degree Fahrenheit character
uint8_t degreeC_ascii = 0x8F;                               // defines position 143 in UTF-8 table
uint8_t degreeF_ascii = 0x90;                               // defines position 144 in UTF-8 table


// Table for news and weather messages

struct maintable                        // table for weather and news
{
 char wetcur[500];                      // current weather strings
 char newscur[500];                     // news strings
 char newslink[200];                    // news url strings
};

maintable table[tablesize];             // number of elements specified by (NUM_OF_NEWSSOURCES*NUM_OF_ARTICLES) or (NUM_OF_CITIES*2)  - depending on which number is larger




//--------------------------------------------------------------------------------------

/// MAIN ARRAY CALLED IN LOOP AND SHOWN BY DISPLAY ///
 
struct sCatalog                                                            
{
  uint8_t  effectin;           // text effect in
  char*    psz;                // text string
  uint8_t  effectout;          // text effect out
  uint8_t  just;               // text alignment
  uint16_t speed;              // speed multiplier of library default
  uint16_t pause;              // pause multiplier of library default
  uint16_t delay;              // delay multiplier before next message
  };

sCatalog  catalog[] =                                      //changes of number and order of elements can require changes in array location[] !
{
  { 3, infowifi, 3 , 1, globScrollSpeed, 4, 1},   //0                       
  { 1, timeshow, 1 , 1, 2, 6, 1},                 //1         // DON'T CHANGE any values here in catalog[] !!!
  { 3, dateshow, 3 , 1, 2, 0, 1},                 //2         // values of custom messages are loaded on startup from messages.txt saved on SPIFFS
  { 1, showmessages[0], 1, 1, 1, 1, 1},           //3         // values of other messages can be adopted in function enableOrDisable()
  { 1, shownewsService, 1, 1, 1, 1, 1},           //4
  { 3, shownews, 3 , 1, 2, 0, 1},                 //5
  { 1, showcustomServiceMsg, 1, 1, 1, 1, 1},      //6      
  { 1, showmessages[1], 1, 1, 1, 1, 1},           //7
  { 1, showspotifyService, 1, 1, 1, 1, 1},        //8         
  { 3, showspotify, 3 , 1, 2, 0, 0},              //9
  { 1, showmessages[2], 1, 1, 1, 1, 1},           //10
  { 28, showmessages[3], 28, 1, 1, 1, 1},         //11
  { 31, showmessages[4], 31, 1, 1, 1, 1},         //12
  { 1, shownewsService2, 1, 1, 1, 1, 1},          //13
  { 3, shownews2, 3 , 1, 2, 0, 1},                //14
  { 1, showweatherService, 1, 1, 1, 1, 1},        //15
  { 3, showweather, 3 , 1, 2, 0, 1},              //16
  { 1, showownmsgService, 1, 1, 1, 1, 1},         //17
  { 3, showownmessage, 3, 1, 2, 0, 1}             //18
   };


uint8_t location[numOfMessages] = {3,7,10,11,12};               // Location of showmessages in Main Array 
uint8_t locationTime = 1;                                       // Location of timeshow in Main Array
uint8_t locationDate = 2;                                       // Location of dateshow in Main Array
uint8_t locationWeath = 16;                                     // Location of showweather in Main Array
uint8_t locationNews[2] = {5,14};                               // Location of shownews in Main Array
uint8_t locationSpot = 9;                                       // Location of showspotify in Main Array
uint8_t locationOwn = 18;                                       // Location of showownmessage in Main Array

uint8_t locationGraphic[numOfGraphics] = {4,8,10,12,13,17};     // Location where graphic animations are inserted BEFORE parola animation runs

uint8_t locationUpdate = 2;                                     // Slot for weather and news update before animation runs (must be between to parola animation slots)
 


struct sCatalogTEMP              // internal -> holds custom messages when custom message[i] is deactivated                                                          
{
  uint8_t    effectinTEMP;       // text effect in
  char *          pszTEMP;       // text string
  uint8_t   effectoutTEMP;       // text effect out
  uint8_t        justTEMP;       // text alignment
  uint16_t      speedTEMP;       // speed multiplier of library default
  uint16_t      pauseTEMP;       // pause multiplier of library default
  uint16_t      delayTEMP;       // delay multiplier before next message
  };

sCatalogTEMP  catalogTEMP[] =           // internal -> holds custom messages when custom message[i] is deactivated   
{
  { 1, messages[0], 1, 1, 1, 1, 1},
  { 1, messages[1], 1, 1, 1, 1, 1},
  { 1, messages[2], 1, 1, 1, 1, 1},
  { 28, messages[3], 28, 1, 1, 1, 1},
  { 31, messages[4], 31, 1, 1, 1, 1},
};



textEffect_t effectlist[] = {                                 // List of in and out effects of Parola library
   PA_PRINT,                   // 0
   PA_SCROLL_UP,               // 1
   PA_SCROLL_DOWN,             // 2
   PA_SCROLL_LEFT,             // 3
   PA_SCROLL_RIGHT,            // 4
   PA_SLICE,                   // 5
   PA_MESH,                    // 6
   PA_FADE,                    // 7
   PA_DISSOLVE,                // 8
   PA_BLINDS,                  // 9
   PA_WIPE,                    // 10
   PA_WIPE_CURSOR,             // 11
   PA_SCAN_HORIZ,              // 12
   PA_SCAN_HORIZX,             // 13
   PA_SCAN_VERT,               // 14
   PA_SCAN_VERTX,              // 15
   PA_OPENING,                 // 16
   PA_OPENING_CURSOR,          // 17
   PA_CLOSING,                 // 18
   PA_CLOSING_CURSOR,          // 19
   PA_SCROLL_UP_LEFT,          // 20
   PA_SCROLL_UP_RIGHT,         // 21
   PA_SCROLL_DOWN_LEFT,        // 22
   PA_SCROLL_DOWN_RIGHT,       // 23
   PA_GROW_UP,                 // 24
   PA_GROW_DOWN,               // 25
   PA_NO_EFFECT,               // 26
   PA_RANDOM,                  // 27
   PA_SPRITE,                  // 28 WALKER
   PA_SPRITE,                  // 29 INVADER 
   PA_SPRITE,                  // 30 CHEVRON
   PA_SPRITE,                  // 31 HEART
   PA_SPRITE,                  // 32 ARROW 1
   PA_SPRITE,                  // 33 STEAMBOAT
   PA_SPRITE,                  // 34 FBALL
   PA_SPRITE,                  // 35 ROCKET
   PA_SPRITE,                  // 36 ROLL2
   PA_SPRITE,                  // 37 PMAN2
   PA_SPRITE,                  // 38 LINES
   PA_SPRITE,                  // 39 ROLL1
   PA_SPRITE,                  // 40 SAILBOAT
   PA_SPRITE,                  // 41 ARROW2
   PA_SPRITE,                  // 42 WAVE1
   PA_SPRITE                   // 43 PMAN1
};


textPosition_t justlist[] = {
PA_LEFT,            // 0
PA_CENTER,          // 1
PA_RIGHT            // 2
};


//------------------------------------------------------------------------------------------------


//// Sprite Definitions ////

const uint8_t F_PMAN1 = 6;
const uint8_t W_PMAN1 = 8;
const uint8_t PROGMEM pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
};

const uint8_t F_WAVE = 14;
const uint8_t W_WAVE = 14;
const uint8_t PROGMEM wave[F_WAVE * W_WAVE] =  // triangular wave / worm
{
  0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10,
  0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20,
  0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40,
  0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
  0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
  0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10,
  0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08,
  0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04,
  0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02,
  0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02,
  0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04,
  0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08,
};

const uint8_t F_ROLL1 = 4;
const uint8_t W_ROLL1 = 8;
const uint8_t PROGMEM roll1[F_ROLL1 * W_ROLL1] =  // rolling square
{
  0xff, 0x8f, 0x8f, 0x8f, 0x81, 0x81, 0x81, 0xff,
  0xff, 0xf1, 0xf1, 0xf1, 0x81, 0x81, 0x81, 0xff,
  0xff, 0x81, 0x81, 0x81, 0xf1, 0xf1, 0xf1, 0xff,
  0xff, 0x81, 0x81, 0x81, 0x8f, 0x8f, 0x8f, 0xff,
};

const uint8_t F_ROLL2 = 4;
const uint8_t W_ROLL2 = 8;
const uint8_t PROGMEM roll2[F_ROLL2 * W_ROLL2] =  // rolling octagon
{
  0x3c, 0x4e, 0x8f, 0x8f, 0x81, 0x81, 0x42, 0x3c,
  0x3c, 0x72, 0xf1, 0xf1, 0x81, 0x81, 0x42, 0x3c,
  0x3c, 0x42, 0x81, 0x81, 0xf1, 0xf1, 0x72, 0x3c,
  0x3c, 0x42, 0x81, 0x81, 0x8f, 0x8f, 0x4e, 0x3c,
};

const uint8_t F_LINES = 3;
const uint8_t W_LINES = 8;
const uint8_t PROGMEM lines[F_LINES * W_LINES] =  // spaced lines
{
  0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00,
  0xff, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00,
  0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff,
};

const uint8_t F_ARROW1 = 3;
const uint8_t W_ARROW1 = 10;
const uint8_t PROGMEM arrow1[F_ARROW1 * W_ARROW1] =  // arrow fading to center
{
  0x18, 0x3c, 0x7e, 0xff, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x00,
  0x18, 0x3c, 0x7e, 0xff, 0x00, 0x7e, 0x00, 0x00, 0x18, 0x00,
  0x18, 0x3c, 0x7e, 0xff, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x18,
};

const uint8_t F_ARROW2 = 3;
const uint8_t W_ARROW2 = 9;
const uint8_t PROGMEM arrow2[F_ARROW2 * W_ARROW2] =  // arrow fading to outside
{
  0x18, 0x3c, 0x7e, 0xe7, 0x00, 0x00, 0xc3, 0x00, 0x00,
  0x18, 0x3c, 0x7e, 0xe7, 0xe7, 0x00, 0x00, 0x81, 0x00,
  0x18, 0x3c, 0x7e, 0xe7, 0x00, 0xc3, 0x00, 0x00, 0x81,
};

const uint8_t F_SAILBOAT = 1;
const uint8_t W_SAILBOAT = 11;
const uint8_t PROGMEM sailboat[F_SAILBOAT * W_SAILBOAT] =  // sail boat
{
  0x10, 0x30, 0x58, 0x94, 0x92, 0x9f, 0x92, 0x94, 0x98, 0x50, 0x30,
};

const uint8_t F_STEAMBOAT = 2;
const uint8_t W_STEAMBOAT = 11;
const uint8_t PROGMEM steamboat[F_STEAMBOAT * W_STEAMBOAT] =  // steam boat
{
  0x10, 0x30, 0x50, 0x9c, 0x9e, 0x90, 0x91, 0x9c, 0x9d, 0x90, 0x71,
  0x10, 0x30, 0x50, 0x9c, 0x9c, 0x91, 0x90, 0x9d, 0x9e, 0x91, 0x70,
};

const uint8_t F_HEART = 5;
const uint8_t W_HEART = 9;
const uint8_t PROGMEM heart[F_HEART * W_HEART] =  // beating heart
{
  0x0e, 0x11, 0x21, 0x42, 0x84, 0x42, 0x21, 0x11, 0x0e,
  0x0e, 0x1f, 0x33, 0x66, 0xcc, 0x66, 0x33, 0x1f, 0x0e,
  0x0e, 0x1f, 0x3f, 0x7e, 0xfc, 0x7e, 0x3f, 0x1f, 0x0e,
  0x0e, 0x1f, 0x33, 0x66, 0xcc, 0x66, 0x33, 0x1f, 0x0e,
  0x0e, 0x11, 0x21, 0x42, 0x84, 0x42, 0x21, 0x11, 0x0e,
};

const uint8_t F_INVADER = 2;
const uint8_t W_INVADER = 10;
const uint8_t PROGMEM invader[F_INVADER * W_INVADER] =  // space invader
{
  0x0e, 0x98, 0x7d, 0x36, 0x3c, 0x3c, 0x36, 0x7d, 0x98, 0x0e,
  0x70, 0x18, 0x7d, 0xb6, 0x3c, 0x3c, 0xb6, 0x7d, 0x18, 0x70,
};

const uint8_t F_ROCKET = 2;
const uint8_t W_ROCKET = 11;
const uint8_t PROGMEM rocket[F_ROCKET * W_ROCKET] =  // rocket
{
  0x18, 0x24, 0x42, 0x81, 0x99, 0x18, 0x99, 0x18, 0xa5, 0x5a, 0x81,
  0x18, 0x24, 0x42, 0x81, 0x18, 0x99, 0x18, 0x99, 0x24, 0x42, 0x99,
};

const uint8_t F_FBALL = 2;
const uint8_t W_FBALL = 11;
const uint8_t PROGMEM fireball[F_FBALL * W_FBALL] =  // fireball
{
  0x7e, 0xab, 0x54, 0x28, 0x52, 0x24, 0x40, 0x18, 0x04, 0x10, 0x08,
  0x7e, 0xd5, 0x2a, 0x14, 0x24, 0x0a, 0x30, 0x04, 0x28, 0x08, 0x10,
};

const uint8_t F_CHEVRON = 1;
const uint8_t W_CHEVRON = 9;
const uint8_t PROGMEM chevron[F_CHEVRON * W_CHEVRON] =  // chevron
{
  0x18, 0x3c, 0x66, 0xc3, 0x99, 0x3c, 0x66, 0xc3, 0x81,
};

const uint8_t F_WALKER = 5;
const uint8_t W_WALKER = 7;
const uint8_t PROGMEM walker[F_WALKER * W_WALKER] =  // walking man
{
    0x00, 0x48, 0x77, 0x1f, 0x1c, 0x94, 0x68,
    0x00, 0x90, 0xee, 0x3e, 0x38, 0x28, 0xd0,
    0x00, 0x00, 0xae, 0xfe, 0x38, 0x28, 0x40,
    0x00, 0x00, 0x2e, 0xbe, 0xf8, 0x00, 0x00, 
    0x00, 0x10, 0x6e, 0x3e, 0xb8, 0xe8, 0x00,
};



struct bildlein                       // struct array for sprite effects
{
  const uint8_t *data;
  uint8_t width;
  uint8_t frames;
};

bildlein sprite[] =
{
  { walker, W_WALKER, F_WALKER },            //0
  { invader, W_INVADER, F_INVADER },         //1
  { chevron, W_CHEVRON, F_CHEVRON },         //3
  { heart, W_HEART, F_HEART },               //4
  { arrow1, W_ARROW1, F_ARROW1 },            //5
  { steamboat, W_STEAMBOAT, F_STEAMBOAT },   //6
  { fireball, W_FBALL, F_FBALL },            //7
  { rocket, W_ROCKET, F_ROCKET },            //8
  { roll2, W_ROLL2, F_ROLL2 },               //9
  { pacman2, W_PMAN2, F_PMAN2 },             //10
  { lines, W_LINES, F_LINES },               //11
  { roll1, W_ROLL1, F_ROLL1 },               //12
  { sailboat, W_SAILBOAT, F_SAILBOAT },      //13
  { arrow2, W_ARROW2, F_ARROW2 },            //14
  { wave, W_WAVE, F_WAVE },                  //15
  { pacman1, W_PMAN1, F_PMAN1 }              //16
};


//------------------------------------------------------------------------------------------------


 
////  Message Definitions  ////

#ifdef LOCAL_LANG

  const char msgWelcome[] =                                  "Willkommen";
  const char msgNews[] PROGMEM=                              "News von";
  const char msgCurrentWeather[] PROGMEM=                    "aktuell";
  const char msgForecast[] PROGMEM=                          "Vorhersage";
  const char msgToday[] PROGMEM =                            "heute";
  const char msg24[] PROGMEM=                                "morgen";
  const char msg48[] PROGMEM=                                "übermorgen";
  const char msghour[]PROGMEM=                               "Uhr";
  const char msgFromTo[] PROGMEM=                            "bis";
  const char msgTemp[] PROGMEM=                              "Temperatur";
  const char msgHumidity[] PROGMEM=                          "Luftfeuchte";
  const char msgClouds[] PROGMEM=                            "Bewölkung";
  const char msgRain[] PROGMEM=                              "Niederschlag";
  const char msgPureRain[] PROGMEM =                         "Regen";
  const char msgSnow[] PROGMEM=                              "Schneemenge";
  const char msgWind[] PROGMEM=                              "Wind";
  const char msgPop[] PROGMEM=                               "Niederschlagswahrscheinlichkeit";
  const char msgNetwork[] PROGMEM=                           "Verbunden mit Netzwerk:";
  const char msgAs[] PROGMEM=                                "als";
  const char msgAP[] PROGMEM=                                "Betrieb als Access Point:";
  const char msgWith[] PROGMEM=                              "mit IP";
  const char msgNoNews[] PROGMEM=                            "News deaktiviert: Noch keine Nachrichten bezogen!";
  const char msgSendOwn[] PROGMEM=                           "Deine Nachricht auf dem Display an";
  const char msgOrdinalNumber[] PROGMEM=                     ".";
  const char msgSpotifyProb[] PROGMEM=                       "keine aktuelle Information verfügbar";
  const char msgSpotifyOK[] PROGMEM=                         "Es läuft gerade auf Spotify";
  const char msgSpotifyDeact[] PROGMEM=                      "Musikinfo ist deaktiviert";
  const char msgBy[] PROGMEM=                                "von";
  const char msgCastWebOK[] PROGMEM=                         "Es läuft gerade:";
  const char weatherService[] PROGMEM=                       "Wetter-Service";
 #if MAX_DEVICES > 9
  const char newsService[] PROGMEM =                         "Nachrichten-Service";
 #else
  const char newsService[] PROGMEM =                         "News-Service";
 #endif
  const char spotifyService[] PROGMEM=                       "Musik-Service";
  const char ownmsgService[] PROGMEM=                        "Interaktiv";
  
#else
  
  const char msgWelcome[] =                                  "Welcome";
  const char msgNews[] PROGMEM=                              "News from";
  const char msgCurrentWeather[] PROGMEM=                    "Current weather in";
  const char msgForecast[] PROGMEM=                          "Weather forecast for";
  const char msgToday[] PROGMEM =                            "today";
  const char msg24[] PROGMEM=                                "tomorrow";
  const char msg48[] PROGMEM=                                "48h";
  const char msghour[] PROGMEM=                              "h";
  const char msgFromTo[] PROGMEM=                            "-";
  const char msgTemp[] PROGMEM=                              "temperature";
  const char msgHumidity[] PROGMEM=                          "humidity";
  const char msgClouds[] PROGMEM=                            "clouds";
  const char msgRain[] PROGMEM=                              "rainfall";
  const char msgPureRain[] PROGMEM =                         "rainfall";
  const char msgSnow[] PROGMEM=                              "snowfall";
  const char msgWind[] PROGMEM=                              "wind";
  const char msgPop[] PROGMEM=                               "probability of precipitation ";
  const char msgNetwork[] PROGMEM=                           "Connected to network:";
  const char msgAs[] PROGMEM=                                "as";
  const char msgAP[]PROGMEM=                                 "Operation as Access Point:";
  const char msgWith[] PROGMEM=                              "with IP";
  const char msgNoNews[] PROGMEM=                            "News deactivated: No news loaded yet";
  const char msgSendOwn[] PROGMEM=                           "Send your own message to";
  const char msgOrdinalNumber[] PROGMEM=                     "";
  const char msgSpotifyProb[] PROGMEM=                       "no current information available";
  const char msgSpotifyOK[] PROGMEM=                         "Currently playing on Spotify";
  const char msgSpotifyDeact[] PROGMEM=                      "Music Info is deactivated";
  const char msgBy[] PROGMEM=                                "by";
  const char msgCastWebOK[] PROGMEM=                         "Currently playing:";
  const char weatherService[] PROGMEM=                       "Weather Service";
  const char newsService[] PROGMEM=                          "News Service";
  const char spotifyService[] PROGMEM=                       "Music Service";
  const char ownmsgService[] PROGMEM=                        "Interactive";
  
#endif



////  FUNCTIONS  ////


////  WiFi ////

void wificonnect() {                                                       
  
  uint8_t wifi_retry = 0;
  uint8_t staticIP = 0;
  
  IPAddress ip, gateway, subnet, dns;

  WiFi.mode(WIFI_STA);                                                      // required for core >= 3.3 : requires setting mode and disconnecting BEFORE config
  WiFi.disconnect();                                                        //   --> also safe for core v2

  File f = SPIFFS.open("/IP_mode.txt", "r");
  
  if (f) {                                                                  // if !f reading skipped. staticIP stays 0
    String line;
    line.reserve(16);

    line = f.readStringUntil('\n');
    line.trim();
    staticIP = line.toInt();
    
    line = f.readStringUntil('\n'); line.trim();                                 // uses fromString() method now for robustness 
    if (!ip.fromString(line)) { ip = IPAddress(line.toInt()); }                  //  --> fallback for compatibility: if fromString fails (old format), use line.toInt() bit-cast.
    
    line = f.readStringUntil('\n'); line.trim();
    if (!gateway.fromString(line)) { gateway = IPAddress(line.toInt()); }
    
    line = f.readStringUntil('\n'); line.trim();
    if (!subnet.fromString(line)) { subnet = IPAddress(line.toInt()); }
    
    line = f.readStringUntil('\n'); line.trim();
    if (!dns.fromString(line)) { dns = IPAddress(line.toInt()); }
    
    f.close();
  }

 #ifdef DEBUG
  Serial.println("IP Configuration loaded from SPIFFS:");
  Serial.print("Static IP enabled: ");
  Serial.println(staticIP);
  if (staticIP == 1) {
    Serial.printf("IP: %s, Gateway: %s, Subnet: %s, DNS: %s\n", 
                  ip.toString().c_str(), gateway.toString().c_str(), 
                  subnet.toString().c_str(), dns.toString().c_str());
  }
 #endif

  if (staticIP == 1) WiFi.config(ip, gateway, subnet, dns);             // sets static IP parameters
  WiFi.softAPdisconnect(true);                                          // closes AP mode on startup to avoid ESP working as AP during normal operation

  while (WiFi.status() != WL_CONNECTED && wifi_retry < 3) {
 #ifdef DEBUG
    Serial.print("Attempt via WiFi.begin() -> Nr: ");
    Serial.println(wifi_retry + 1);
 #endif

    WiFi.begin();
                                                                  // INNER WAIT LOOP: Wait up to 6 seconds for this attempt to work
    for (int wait = 0; wait < 12; wait++) {                       // 12 * 500ms = 6 seconds
      if (WiFi.status() == WL_CONNECTED) break;                   // exit wait early if connected
      delay(500);
     #ifdef DEBUG
      Serial.print(".");
     #endif
    }
   #ifdef DEBUG
    Serial.println(""); 
   #endif
   wifi_retry++;
  }

  if (wifi_retry >= 3) {
   #ifdef DEBUG
    Serial.println("no connection, starting AP");
    Serial.println("Notice on Display: Portal");
    Serial.println("... starting AP");
   #endif
    P.print("no Wifi --> AP");
    WiFiManager wifiManager;
 
   #ifndef SPOTIFY                                                         // this doesn't work with SPOTIFY defined on ESP32 !
    wifiManager.setAPStaticIPConfig(AP_IP, AP_IP, AP_Netmask);             // if #define SPOTIFY default ESP IP 192.168.4.1 is set
   #endif

   #ifdef DANI
    wifiManager.setCustomHeadElement(replaceH1);
   #endif
    wifiManager.setConfigPortalTimeout(300);
    wifiManager.startConfigPortal(AP_NAME, AP_PW);

    if (wifiManager.getTimeoutState() == true) {                          // if config portal has timed out, ESP restarts(necessary after power blackout, when WiFi network needs some time to start up again)
     #ifdef DEBUG                                                         // (necessary after power blackout, when Home WiFi network needs some time to start up)
      Serial.println("Config Portal has timed out. Restarting...");
     #endif
      delay(500);
      ESP.restart();  
    }

    wifiManagerWasCalled = true;

    if (wifiManager.getStaticMode() == true) {                            // gets information from WiFiManager if connection has been made with static IP
      static_Mode_enabled = 1;                                            // and writes it to file on SPIFFS for future start ups. 
    }

    File fs = SPIFFS.open("/IP_mode.txt", "w");                           // SAVING: uses println and toString() to avoid integer overflow bugs analog to reading
    fs.println(static_Mode_enabled);
    fs.println(WiFi.localIP().toString());
    fs.println(WiFi.gatewayIP().toString());
    fs.println(WiFi.subnetMask().toString());
    fs.println(WiFi.dnsIP().toString());
    fs.close();
    
   #ifdef DEBUG
    Serial.println("IP Configuration saved on SPIFFS:");
    Serial.print("Static IP enabled: ");
    Serial.println(static_Mode_enabled);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
   #endif
  }

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
   #ifdef DEBUG
    Serial.print("Connected to network \"");
    Serial.print(WiFi.SSID());
    Serial.print("\" with IP ");
    Serial.println(WiFi.localIP());
   #endif
    P.displayReset();
  }
}                                                               // end of wificonnect()

                                      
void wifiReconnect() {
  static unsigned long lastWifiRetry = 0;
  static uint8_t wifi_retry_count = 0;
  static bool reconnecting = false;
                                                                   
  if (WiFi.status() == WL_CONNECTED) {                       // if connected, check if came back from a disconnect
    if (reconnecting) {
      reconnecting = false;
      wifi_retry_count = 0;
      #ifdef DEBUG
        Serial.println("WiFi Reconnected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
      #endif
    }
    return;                                                  // ok, so return
  }
  
  unsigned long currentMillis = millis();                    //  otherwise, if disconnected...
  if (!reconnecting) {
    reconnecting = true;
    wifi_retry_count = 0;
    WiFi.setAutoReconnect(true);                            // not necessary as it is default but does not harm
    #ifdef DEBUG
      Serial.println("WiFi connection lost... starting reconnection logic");
    #endif
  }
  
  if (currentMillis - lastWifiRetry >= 10000) {             // increased interval to 10 seconds to give Core >= 3.3. enough time to finish
    lastWifiRetry = currentMillis;
    wifi_retry_count++; 
   #ifdef DEBUG
    Serial.print("Retrying... Attempt ");
    Serial.println(wifi_retry_count);
   #endif
   #ifdef LOGFILE
    File f = SPIFFS.open("/logfile.txt", "a");
    if (f) {
      f.printf("%s WIFI RECONNECT ATTEMPT %i\n", timeSaver, wifi_retry_count);
      f.close();
    }
   #endif 
    WiFi.disconnect();
    //WiFi.reconnect();
    WiFi.begin();                                         // no args → uses stored credentials from WiFiManager (more robust approach than WiFi.reconnect()
   
    if (wifi_retry_count >= 10) {                         // if reconnection attempts have failed
      #ifdef DEBUG
        Serial.println("Reconnection failed 10 times. Restarting ESP...");
      #endif
      P.displayReset();
      P.print("Lost WiFi...");
      delay(1000);
      P.print("Restarting...");
      delay(1000);
      ESP.restart();
    }
  }
}


void checkIfAP(){                                                                      // opens Access Point if captive portal is skipped via "Exit"
  
  if (WiFi.status() != WL_CONNECTED && wifiManagerWasCalled == true) {                // --- extra waiting time für slow routers / wifi (ESP32 continues connecting in background)
    P.print("connecting...");                             
    for (int i = 0; i < 20; i++) {                                                    // max 10 seconds (20 x 500ms)
      if (WiFi.status() == WL_CONNECTED) break;
      delay(500);
    }
  }
  
  if (WiFi.status() != WL_CONNECTED && wifiManagerWasCalled == true) {
          WiFi.mode(WIFI_AP);
          delay(500);
         // WiFi.softAPConfig(AP_IP, AP_IP, AP_Netmask);                              // not necessary as wifimanger has done this
          WiFi.softAP(AP_NAME, AP_PW);
        #ifdef DEBUG
          Serial.print("AP started with IP ");
          Serial.println(WiFi.softAPIP());
        #endif
          AP_established = true;
    }

  if (WiFi.status() == WL_CONNECTED && wifiManagerWasCalled == true) {                  // restart after successful config
    P.print("WiFi ok");
    delay(2000);
   #ifdef DEBUG
    Serial.println("Wifi ok, restarting device");
   #endif
    P.print("restarting");
    delay(500);
    ESP.restart();
    }
}



//// UTF8 CONVERSION ////                                    // Note: Convert only one time, otherwise characters will disappear!

uint8_t utf8Ascii(uint8_t ascii, bool reset = false) {                           // See https://forum.arduino.cc/index.php?topic=171056.msg4457906#msg4457906
                                                                                 // and http://playground.arduino.cc/Main/Utf8ascii
  static uint8_t cPrev;
  static uint8_t cPrePrev;
  uint8_t c = '\0';

  if (reset) {
    cPrev = '\0';
    cPrePrev = '\0';
    if (ascii == '\0') return '\0';                          // safety return if called with empty string start
  }
  
  if (ascii < 0x7f || ascii == degreeC_ascii || ascii == degreeF_ascii) {
    cPrev = '\0';                                           // last character
    cPrePrev = '\0';                                        // penultimate character
    c = ascii;
  } else {
    switch (cPrev) {
      case 0xC2: c = ascii;  break;
      case 0xC3: c = ascii | 0xC0;  break;
      }
     switch (cPrePrev) {
      
      case 0xE2: if (cPrev==0x82 && ascii==0xAC) c = 128;  // EURO SYMBOL          // 128 -> number in defined font table
                 if (cPrev==0x80 && ascii==0xA6) c = 133;  // horizontal ellipsis
                 if (cPrev==0x80 && ascii==0x93) c = 150;  // en dash
                 if (cPrev==0x80 && ascii==0x9E) c = 132;  // DOUBLE LOW-9 QUOTATION MARK
                 if (cPrev==0x80 && ascii==0x9C) c = 147;  // LEFT DOUBLE QUOTATION MARK
                 if (cPrev==0x80 && ascii==0x9D) c = 148;  // RIGHT DOUBLE QUOTATION MARK
                 if (cPrev==0x80 && ascii==0x98) c = 145;  // LEFT SINGLE QUOTATION MARK
                 if (cPrev==0x80 && ascii==0x99) c = 146;  // RIGHT SINGLE QUOTATION MARK
                 
                 break; 
      }

    cPrePrev = cPrev;                                      // save penultimate character
    cPrev = ascii;                                         // save last character
  }
  return(c);
}


void utf8AsciiConvert(char* src, char*des)                 // converts array form source array "src" to destination array "des"
{       int k=0;
        char c;
        for (int i=0; src[i]; i++){
             c = utf8Ascii(src[i], i == 0);
             if (c!='\0') //if (c!=0)
             des[k++]=c;
        }
        des[k]='\0';  //des[k]=0;
}



//// TIME AND DATE FUNCTIONS ////

void getTimeFromServer(){
  uint8_t  time_retry=0;                                         // counter for connection attempts to time server
  struct tm initial;                                             // temp struct for checking if year==1970 (no received time information means year is 1970)
  initial.tm_year=70;
  char stamp[23];
  
  configTzTime(timezone, ntpServer);                             // adjust your local time zone with variable timezone
                                                                 // if connection not sucessful multiple retries with increasing intervals -> https://www.nongnu.org/lwip/2_1_x/group__sntp__opts.html
  while(initial.tm_year == 70 && time_retry < 20){                 

  #if ESP_ARDUINO_VERSION_MAJOR < 3                                       
    if (sntp_getreachability(0) != 0){                                         // if sntp_getreachability(0) == 0 -> ntp server call failed
  #else
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED){
  #endif  
    time_t now = time(NULL);
    localtime_r(&now, &initial);
   }

  #ifdef DEBUG
   Serial.print("Time Server connection attempt: ");
   Serial.println(time_retry + 1);
   Serial.print("current year: ");
   Serial.println(1900 + initial.tm_year);
  #endif

  delay(500);
  time_retry++;
  }

  if (time_retry >= 20){                                               // max waiting time 20 x 500ms = 10s
   #ifdef DEBUG
    Serial.println("Connection to time server failed");
   #endif
   #ifdef LOGFILE  
    File f = SPIFFS.open("/logfile.txt", "a");              /// logfile DEBUG -> If you want to use this, remember the file size enlarges with every call !!!
    f.printf("%s\n", "Connection to timeserver FAILED");    /// logfile DEBUG
    f.close();                                              /// logfile DEBUG
   #endif
    strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm);                                                       
    snprintf(timeServerLog[timeLogCounter], sizeof(timeServerLog[timeLogCounter]), "%s %s &#10060;", stamp, ntpServer);   
  } else {
    time_t now = time(NULL);
    localtime_r(&now, &tm);
    strftime (timeSaver, sizeof(timeSaver), "%H:%M", &tm);                          // required for combined time and date mode
    strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm);                                                      
    snprintf(timeServerLog[timeLogCounter], sizeof(timeServerLog[timeLogCounter]), "%s %s  &#9989;", stamp, ntpServer);   
    #ifdef DEBUG
      Serial.print("Successfully requested current time from server: ");
      Serial.println(timeSaver); 
    #endif
    #ifdef LOGFILE
      File f = SPIFFS.open("/logfile.txt", "a");                              /// logfile DEBUG -> If you want to use this, remember the file size enlarges with every call !!!
      f.printf("%s %s\n", "Timer Server Call successful:", timeSaver);        /// logfile DEBUG
      f.close();                                                              /// logfile DEBUG
    #endif
  }
timeLogCounter++;
if (timeLogCounter==numOfTimeLogs) timeLogCounter=0; 
}


void makeDate() {
  char buf1[20]; 
  char buf2[20];
  char buf3[20];
  char buf4[20];
  uint8_t weekday;

  time_t now = time(NULL);                                                                      // actually not necessary as function is always called with updated tm
  localtime_r(&now, &tm);

  #if MAX_DEVICES < 20                                                                          // for smaller displays
   strftime (buf2, sizeof(buf2), "%d", &tm);                                                    // required for combined time and date mode
   strftime (buf3, sizeof(buf3), "%m", &tm);                                                    // Example: 01.03.20
   strftime (buf4, sizeof(buf4), "%y", &tm);
   snprintf (dateSaver, sizeof(dateSaver), "%s.%s.%s", buf2, buf3, buf4);     
  #else
   strftime (buf2, sizeof(buf2), "%e", &tm);
   snprintf (buf3, sizeof(buf3), "%s", monthsXL[tm.tm_mon]);
   strftime (buf4, sizeof(buf4), "%Y", &tm);
   snprintf (dateSaver, sizeof(dateSaver), "%s%s %s %s", buf2, msgOrdinalNumber, buf3, buf4);   // Example: 1 March 2020 / 1. März 2020
   utf8AsciiConvert(dateSaver, dateSaver);
  #endif
  
   if (enableDate == 1){
    #ifdef SHORTDATE
      strftime (buf1, sizeof(buf1), "%e", &tm);                                                           // see: http://www.cplusplus.com/reference/ctime/strftime/
      snprintf (buf2, sizeof(buf2), "%s", months[tm.tm_mon]);                                             // see: http://www.willemer.de/informatik/cpp/timelib.htm
      strftime (buf3, sizeof(buf3), "%Y", &tm);
      snprintf (dateshow, sizeof(dateshow), "%s%s %s %s", buf1,msgOrdinalNumber, buf2, buf3);             // Example: 1 Mar 2020 / 1. Mrz 2020
    #else
      snprintf (buf1, sizeof(buf1), "%s", days[tm.tm_wday]);
      strftime (buf2, sizeof(buf2), "%e", &tm);
      snprintf (buf3, sizeof(buf3), "%s", monthsXL[tm.tm_mon]);
      strftime (buf4, sizeof(buf4), "%Y", &tm);
      snprintf (dateshow, sizeof(dateshow), "%s %s%s %s %s", buf1, buf2, msgOrdinalNumber, buf3, buf4);   // Example: Sunday 1 March 2020 / Sonntag 1. März 2020
      utf8AsciiConvert(dateshow, dateshow);                                                               // conversion necessary for "Jänner" and "März"
    #endif
  }

  weekday = tm.tm_wday;
  switch (weekday) {
    case 0: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[2]);                               // if today is Sunday, day after tomorrow is Tuesday
      break;
    case 1: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[3]);
      break;
    case 2: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[4]);
      break;
    case 3: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[5]);
      break;
    case 4: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[6]);
      break;
    case 5: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[0]);
      break;
    case 6: 
      snprintf(dayAfterTomorrow, sizeof(dayAfterTomorrow), "%s" ,days[1]);
      break;
  }
    
#ifdef DEBUG
  Serial.print("Date: ");
  Serial.println(dateSaver);
#endif

  lastDayRun = tm.tm_mday;                                                           // writes current value
}


void displayTime() {                                                                // standard case: if messages other than time are activated
  static time_t lastminute = 0;
  time_t now = time(NULL);
  localtime_r(&now, &tm);
  if (tm.tm_min != lastminute) {
    lastminute = tm.tm_min;
    strftime (timeSaver, sizeof(timeSaver), "%H:%M", &tm);                          // required for combined time and date mode     
    #ifdef DEBUG 
      Serial.print("current time: ");
      Serial.println(timeSaver);
      Serial.println("Free heap: ");                                                // monitoring heap every minute
      Serial.println(ESP.getFreeHeap());
    #endif
  }   
  if (tm.tm_mday != lastDayRun) {                                                   // run makeDate() when day has changed
    makeDate();
  }
}


void displayOnlyTime(bool preventRefresh = false) {                                 // special case: if only time message is activated -> hh:mm:ss
  P.setTextAlignment(PA_CENTER);
  static time_t lastsecond = 0;
  time_t now = time(NULL);
  localtime_r(&now, &tm);
  if (tm.tm_sec != lastsecond) {
    lastsecond = tm.tm_sec;
    if (enableTime==1){
      strftime (timeshow, sizeof(timeshow), "%H:%M:%S", &tm);
      P.displayClear();              // neu 0.8
      P.setFont(numeric7Seg);
      P.print(timeshow);
      P.displayReset();
    } else {
      P.displayClear();                                                             
      P.displayReset();
    }
  }  
  if (millis() - previoustimecall > (timeRefreshInterval* 60000) && WiFi.status() == WL_CONNECTED && !preventRefresh){
    getTimeFromServer();                                         
    previoustimecall=millis();
  }
  if (tm.tm_mday != lastDayRun) {                                                     // run makeDate() when day has changed
    makeDate();
  }
}


//// LDR FUNCTION ////

void checkLDR() {                                                   // checks LDR sensor and sets display brightness
  uint16_t ldr = analogRead(Analog_Pin);
  uint8_t tmpValue = map(ldr, 0, 4095, intens_min, intens_max);

  if (ls_triggerStep > 1) {                                         // ignores small changes, jumps directly to target brightness when difference is big enough
    if (abs((int)tmpValue - (int)intens) >= ls_triggerStep) {
      intens = tmpValue;
      P.setIntensity(intens);
    }
  } else {                                                          // always moves toward target, but only 1 step per interval (-> smooth change)
    if (tmpValue != intens) {
      if (tmpValue > intens) intens++;
      else intens--;
      P.setIntensity(intens);
    }
  }
}


////  WEATHER FUNCTIONS  ////

const char* getWindDirection(int degrees) {                 // translates wind direction from degrees to cardinal points
  int sector = ((degrees + 11) / 22.5 - 1);
  if (sector < 0) sector = 0;
  if (sector > 15) sector = 15;
  return windDirection[sector];
}


void getWeatherData_0() {                                   //gets weather data from openweathermap.org and saves it into table.wetcur                      
   uint8_t helper = numberofcities*2*weathLogCounter;       
   const char* units;
   uint8_t tempUnit;
   if (imperial == false){
      units = "metric";
      tempUnit = degreeC_ascii;
   } else {
      units = "imperial";
      tempUnit = degreeF_ascii;
   }
   JsonDocument doc;
   JsonDocument filter;
   WiFiClient client;
   HTTPClient http;
   
   strftime (logWeathStamp[weathLogCounter], sizeof(logWeathStamp[weathLogCounter]), "%d.%m.%y<br>%H:%M:%S", &tm);
   logWeathType[weathLogCounter] = 0;
   weathLogCounter++;
   if (weathLogCounter==numOfLogs) weathLogCounter=0;
  
   #ifdef DEBUG
    Serial.println("Calling Weather API openweathermap.org");
   #endif

   String url;
   url.reserve(200);                                                    // Pre-allocate for URL
    
  for (uint8_t w=0; w<numberofcities*2; w++){
      #ifdef LOGFILE
      uint32_t startMs = millis();                                      // timestamp before the first network contact
    #endif
      doc.clear();
      filter.clear();
      bool currentWeath = (w < numberofcities);                        // loop for version A of weather display:
      const char* Lat = Lat_A[w % numberofcities];                     // current City 1, current City 2, forecast City 2, forecast City 2
      const char* Lon = Lon_A[w % numberofcities];
      const char* cityname = cityName_A[w % numberofcities];            
    
      if (currentWeath){
        url = "http://api.openweathermap.org/data/2.5/weather?lat=";
        url += Lat;
        url += "&lon=";
        url += Lon;
        url += "&units=";
        url += units;
        url += "&lang=";
        url += weatherLanguage;
        url += "&APPID=";
        url += WeatherApiKey;
      } else {
        url = "http://api.openweathermap.org/data/2.5/forecast?lat=";
        url += Lat;
        url += "&lon=";
        url += Lon;
        url += "&units=";
        url += units;
        url += "&cnt=16&lang=";
        url += weatherLanguage;
        url += "&APPID=";
        url += WeatherApiKey;
      }
  
      if(http.begin(client,url)){
        http.setReuse(true);
        int httpCode = http.sendRequest("GET");

        snprintf(logWeathHtml[w+helper], sizeof(logWeathHtml[w+helper]), "%i", httpCode);
        snprintf(logWeathParse[w+helper], sizeof(logWeathParse[w+helper]), "99");                               // gets overwritten if ok
        #ifdef DEBUG  
          Serial.println(httpCode);
        #endif    
        if(httpCode == 200) {
                           
           if (currentWeath) {
            filter["weather"][0]["description"] = true;
            JsonObject filter_main = filter["main"].to<JsonObject>();
            filter_main["temp"] = true;
            filter_main["temp_min"] = true;
            filter_main["temp_max"] = true;
            filter_main["humidity"] = true;
            JsonObject filter_wind = filter["wind"].to<JsonObject>();
            filter_wind["speed"] = true;
            filter_wind["deg"] = true;
            filter["rain"]["3h"] = true;
            filter["snow"]["3h"] = true;
            filter["clouds"]["all"] = true;
           } else {
            JsonObject filter_list_0 = filter["list"].add<JsonObject>();
            filter_list_0["dt_txt"] = true;
            JsonObject filter_list_0_main = filter_list_0["main"].to<JsonObject>();
            filter_list_0_main["temp"] = true;
            filter_list_0_main["temp_min"] = true;
            filter_list_0_main["temp_max"] = true;
            filter_list_0_main["humidity"] = true;
            filter_list_0["weather"][0]["description"] = true;
            filter_list_0["clouds"]["all"] = true;
            JsonObject filter_list_0_wind = filter_list_0["wind"].to<JsonObject>();
            filter_list_0_wind["speed"] = true;
            filter_list_0_wind["deg"] = true;
            filter_list_0["rain"]["3h"] = true;
            filter_list_0["snow"]["3h"] = true;
            filter_list_0["pop"] = true;         
          }
            
          DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
          
          if (error) {
            #ifdef DEBUG 
              Serial.println("deserializeJson() failed: ");
              Serial.println(error.c_str());
            #endif
            #ifdef LOGFILE
              File f = SPIFFS.open("/logfile.txt", "a");
              uint32_t duration = millis() - startMs;
              f.printf("%s %s %s: %s (Status: %s, Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz, Call Duration: %u)\n", 
                      timeSaver, 
                      "ERROR WEATH_0 PARSE",
                      cityname,
                      error.c_str(), 
                      client.connected() ? "Connected" : "Disconnected", 
                      ESP.getFreeHeap(),
                      ESP.getMaxAllocHeap(), 
                      WiFi.RSSI(), 
                      getCpuFrequencyMhz(), 
                      duration);
              f.close();
            #endif
            http.end();
            continue;
          }

            
          if (currentWeath) {

            JsonObject weather_0 = doc["weather"][0];
            const char* current_description = weather_0["description"]| "---";

            JsonObject main = doc["main"];
            float current_temp = main["temp"];
            uint8_t current_humidity = main["humidity"];
            float main_temp_min = main["temp_min"];
            float main_temp_max = main["temp_max"];
            float wind_speed = doc["wind"]["speed"];
            wind_speed = wind_speed * 3.6;
            int wind_deg = doc["wind"]["deg"];
            float rain_3h = doc["rain"]["3h"];
            float snow_3h = 0;
            snow_3h = doc["snow"]["3h"];           
            uint8_t current_clouds = doc["clouds"]["all"];
            
            char msgNiederschlag_3h[20];     
            if (snow_3h == 0){
              snprintf(msgNiederschlag_3h, sizeof(msgNiederschlag_3h),"%s %.1f mm", msgRain, rain_3h);
            } else {
              snprintf(msgNiederschlag_3h, sizeof(msgNiederschlag_3h),"%s %.1f cm", msgSnow, snow_3h);
            }

            snprintf(table[w].wetcur, sizeof(table[w].wetcur), "%s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s  %s %.1f km/h %s",
                     msgCurrentWeather, cityname, current_description, msgTemp, current_temp, tempUnit, msgHumidity, 
                     current_humidity, msgClouds, current_clouds, msgNiederschlag_3h, msgWind, wind_speed, getWindDirection(wind_deg));

            utf8AsciiConvert(table[w].wetcur, table[w].wetcur);

            snprintf(logWeathParse[w+helper], sizeof(logWeathParse[w+helper]), "1");                                     
            snprintf(logWeathSources[w+helper], sizeof(logWeathSources[w+helper]), "%s  <i>%s</i>", cityname, msgCurrentWeather);  
                      
          } else {
       
            JsonArray list = doc["list"];

            JsonObject list_7 = list[7];
            const char* time7 = list_7["dt_txt"]| "0000-00-00 --:--:--";

            JsonObject list_7_main = list_7["main"];
            float temp24 = list_7_main["temp"];
            float tempmin24 = list_7_main["temp_min"];
            float tempmax24 = list_7_main["temp_max"];
            uint8_t humidity24 = list_7_main["humidity"];

            JsonObject list_7_weather_7 = list_7["weather"][0];
            const char* description24 = list_7_weather_7["description"]| "---";
            uint8_t clouds24 = list_7["clouds"]["all"];
            float windspeed24 = list_7["wind"]["speed"];
            windspeed24 = windspeed24 * 3.6;
            int winddeg24 = list_7["wind"]["deg"];
            float rain24 = list_7["rain"]["3h"];
            float snow24 = 0; 
            snow24 = list_7["snow"]["3h"];
            float pop24 = list_7["pop"];
            pop24 = pop24 * 100;

            JsonObject list_15 = list[15];
            const char* time15 = list_15["dt_txt"]| "0000-00-00 --:--:--";

            JsonObject list_15_main = list_15["main"];
            float temp48 = list_15_main["temp"];
            float tempmin48 = list_15_main["temp_min"];
            float tempmax48 = list_15_main["temp_max"];
            uint8_t humidity48 = list_15_main["humidity"];

            JsonObject list_15_weather_15 = list_15["weather"][0];
            const char* description48 = list_15_weather_15["description"]| "---";
            uint8_t clouds48 = list_15["clouds"]["all"];
            float windspeed48 = list_15["wind"]["speed"];
            windspeed48 = windspeed48 * 3.6;
            int winddeg48 = list_15["wind"]["deg"];
            float rain48 = list_15["rain"]["3h"];
            float snow48 = 0; 
            snow48 = list_15["snow"]["3h"];
            float pop48 = list_15["pop"];
            pop48 = pop48 * 100;

            char msgNiederschlag_24h[20];     
            if (snow24 == 0){
              snprintf(msgNiederschlag_24h, sizeof(msgNiederschlag_24h),"%s %.1f mm", msgRain, rain24);
            } else {
              snprintf(msgNiederschlag_24h, sizeof(msgNiederschlag_24h),"%s %.1f cm", msgSnow, snow24);
            }

            char msgNiederschlag_48h[20];     
            if (snow48 == 0){
              snprintf(msgNiederschlag_48h, sizeof(msgNiederschlag_48h),"%s %.1f mm", msgRain, rain48);
            } else {
              snprintf(msgNiederschlag_48h, sizeof(msgNiederschlag_48h),"%s %.1f cm", msgSnow, snow48);
            }            
      
            char time24[6] = {""};
            char time48[6] = {""};
      
            snprintf(time24, sizeof(time24),"%c%c:%c%c", time7[11], time7[12], time7[14], time7[15]);
            snprintf(time48, sizeof(time48),"%c%c:%c%c", time15[11], time15[12], time15[14], time15[15]);
      
            char forecast24[200] = {""};
            char forecast48[200] = {""};

            if (pop24==0){                                                                                                              
              snprintf(forecast24, sizeof(forecast24), "%s %s, %s %s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s: %.0f%%  %s %.1f km/h %s",
                       msgForecast, cityname, msg24, time24, msghour, description24, msgTemp, temp24, tempUnit, msgHumidity, 
                       humidity24, msgClouds, clouds24, msgPop, pop24, msgWind, windspeed24, getWindDirection(winddeg24));
            } else {
              snprintf(forecast24, sizeof(forecast24), "%s %s, %s %s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s: %.0f%%  %s  %s %.1f km/h %s",
                       msgForecast, cityname, msg24, time24, msghour, description24, msgTemp, temp24, tempUnit, msgHumidity, 
                       humidity24, msgClouds, clouds24, msgPop, pop24, msgNiederschlag_24h, msgWind, windspeed24, getWindDirection(winddeg24));
            }
            utf8AsciiConvert(forecast24, forecast24);
            
            if (pop48==0){
              snprintf(forecast48, sizeof(forecast48), "%s %s, %s %s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s: %.0f%%  %s %.1f km/h %s",
                       msgForecast, cityname, dayAfterTomorrow, time48, msghour, description48, msgTemp, temp48, tempUnit, msgHumidity, 
                       humidity48, msgClouds, clouds48, msgPop, pop48, msgWind, windspeed48, getWindDirection(winddeg48));
            } else {
              snprintf(forecast48, sizeof(forecast48), "%s %s, %s %s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s: %.0f%%  %s  %s %.1f km/h %s",
                       msgForecast, cityname, dayAfterTomorrow, time48, msghour, description48, msgTemp, temp48, tempUnit, msgHumidity, 
                       humidity48, msgClouds, clouds48, msgPop, pop48, msgNiederschlag_48h, msgWind, windspeed48, getWindDirection(winddeg48));
            }
            
            utf8AsciiConvert(forecast48, forecast48);
            
            snprintf(table[w].wetcur, sizeof(table[w].wetcur), "%s    %s", forecast24, forecast48);

            snprintf(logWeathSources[w+helper], sizeof(logWeathSources[w+helper]), "%s  <i>%s</i>", cityname, msgForecast); 
            snprintf(logWeathParse[w+helper], sizeof(logWeathParse[w+helper]), "1");                                                                           
          }
          weathEverLoaded = true;
          #ifdef LOGFILE
           File f = SPIFFS.open("/logfile.txt", "a");                /// logfile DEBUG
           f.printf("%s\n%s\n", timeSaver, "WEATH_0 OK");            /// logfile DEBUG
           f.close();                                                /// logfile DEBUG
          #endif
        } else {
          #ifdef LOGFILE
            File f = SPIFFS.open("/logfile.txt", "a");              /// logfile DEBUG
            f.printf("%s ERROR WEATH_0 HTML %s: %i (%s) (Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz)\n", 
                      timeSaver,
                      cityname, 
                      httpCode, 
                      http.errorToString(httpCode).c_str(), 
                      ESP.getFreeHeap(), 
                      ESP.getMaxAllocHeap(), 
                      WiFi.RSSI(), 
                      getCpuFrequencyMhz());
            f.close();                                              /// logfile DEBUG
          #endif
          #ifdef DEBUG 
            Serial.printf("http error: %i\n", httpCode);
          #endif
        }
      #ifdef DEBUG
        Serial.println(table[w].wetcur);                                  
      #endif
    } else {
      #ifdef DEBUG 
        Serial.println("Unable to connect to weather API");
      #endif
    }
    http.end();
  }
  client.stop();
}


#ifdef LOCAL_LANG                                                 // WMO weather code handling for open-meteo.com
const char wmo_0[] PROGMEM = "Klar";
const char wmo_1[] PROGMEM = "Überwiegend klar";
const char wmo_2[] PROGMEM = "Teilweise bewölkt";
const char wmo_3[] PROGMEM = "Bedeckt";
const char wmo_45[] PROGMEM = "Nebel";
const char wmo_48[] PROGMEM = "Raureif-Nebel";
const char wmo_51[] PROGMEM = "Leichter Nieselregen";
const char wmo_53[] PROGMEM = "Nieselregen";
const char wmo_55[] PROGMEM = "Starker Nieselregen";
const char wmo_56[] PROGMEM = "Leichter Sprühregen gefrierend";
const char wmo_57[] PROGMEM = "Starker Sprühregen gefrierend";
const char wmo_61[] PROGMEM = "Leichter Regen";
const char wmo_63[] PROGMEM = "Regen";
const char wmo_65[] PROGMEM = "Starker Regen";
const char wmo_66[] PROGMEM = "Leichter gefrierender Regen";
const char wmo_67[] PROGMEM = "Starker gefrierender Regen";
const char wmo_71[] PROGMEM = "Leichter Schneefall";
const char wmo_73[] PROGMEM = "Schneefall";
const char wmo_75[] PROGMEM = "Starker Schneefall";
const char wmo_77[] PROGMEM = "Schneegriesel";
const char wmo_80[] PROGMEM = "Leichte Regenschauer";
const char wmo_81[] PROGMEM = "Regenschauer";
const char wmo_82[] PROGMEM = "Heftige Regenschauer";
const char wmo_85[] PROGMEM = "Leichte Schneeschauer";
const char wmo_86[] PROGMEM = "Starke Schneeschauer";
const char wmo_95[] PROGMEM = "Gewitter";
const char wmo_96[] PROGMEM = "Gewitter mit leichtem Hagel";
const char wmo_99[] PROGMEM = "Gewitter mit starkem Hagel";
const char wmo_unknown[] PROGMEM = "Unbekannt";
#else
const char wmo_0[] PROGMEM = "Clear sky";
const char wmo_1[] PROGMEM = "Mainly clear";
const char wmo_2[] PROGMEM = "Partly cloudy";
const char wmo_3[] PROGMEM = "Overcast";
const char wmo_45[] PROGMEM = "Fog";
const char wmo_48[] PROGMEM = "Depositing rime fog";
const char wmo_51[] PROGMEM = "Light drizzle";
const char wmo_53[] PROGMEM = "Moderate drizzle";
const char wmo_55[] PROGMEM = "Dense drizzle";
const char wmo_56[] PROGMEM = "Light freezing drizzle";
const char wmo_57[] PROGMEM = "Dense freezing drizzle";
const char wmo_61[] PROGMEM = "Slight rain";
const char wmo_63[] PROGMEM = "Moderate rain";
const char wmo_65[] PROGMEM = "Heavy rain";
const char wmo_66[] PROGMEM = "Light freezing rain";
const char wmo_67[] PROGMEM = "Heavy freezing rain";
const char wmo_71[] PROGMEM = "Slight snowfall";
const char wmo_73[] PROGMEM = "Moderate snowfall";
const char wmo_75[] PROGMEM = "Heavy snowfall";
const char wmo_77[] PROGMEM = "Snow grains";
const char wmo_80[] PROGMEM = "Slight rain showers";
const char wmo_81[] PROGMEM = "Moderate rain showers";
const char wmo_82[] PROGMEM = "Violent rain showers";
const char wmo_85[] PROGMEM = "Slight snow showers";
const char wmo_86[] PROGMEM = "Heavy snow showers";
const char wmo_95[] PROGMEM = "Thunderstorm";
const char wmo_96[] PROGMEM = "Thunderstorm with slight hail";
const char wmo_99[] PROGMEM = "Thunderstorm with heavy hail";
const char wmo_unknown[] PROGMEM = "Unknown";
#endif

const char* get_WMO(int c) {
  switch (c) {
    case 0:  return wmo_0;
    case 1:  return wmo_1;
    case 2:  return wmo_2;
    case 3:  return wmo_3;
    case 45: return wmo_45;
    case 48: return wmo_48;
    case 51: return wmo_51;
    case 53: return wmo_53;
    case 55: return wmo_55;
    case 56: return wmo_56;
    case 57: return wmo_57;
    case 61: return wmo_61;
    case 63: return wmo_63;
    case 65: return wmo_65;
    case 66: return wmo_66;
    case 67: return wmo_67;
    case 71: return wmo_71;
    case 73: return wmo_73;
    case 75: return wmo_75;
    case 77: return wmo_77;
    case 80: return wmo_80;
    case 81: return wmo_81;
    case 82: return wmo_82;
    case 85: return wmo_85;
    case 86: return wmo_86;
    case 95: return wmo_95;
    case 96: return wmo_96;
    case 99: return wmo_99;
    default: return wmo_unknown;
  }
}

const char* getDayLabel(int dayIndex) {                        // day name labels for forecast
  switch(dayIndex) {
    case 0: return msgToday;
    case 1: return msg24;              
    case 2: return dayAfterTomorrow;
    default: return "";
  }
}


void getWeatherData_1() {                                      //gets weather data from open-meteo.com and saves it into table.wetcur      
  uint8_t tempUnit;
  if (imperial == false){
     tempUnit = degreeC_ascii;
  } else {
     tempUnit = degreeF_ascii;
   }
   
  const uint8_t forecastDays = 3;
  
  JsonDocument doc;
  JsonDocument filter;
  
  JsonObject filter_current = filter["current"].to<JsonObject>();
  filter_current["temperature_2m"] = true;
  filter_current["wind_speed_10m"] = true;
  filter_current["wind_direction_10m"] = true;
  filter_current["relative_humidity_2m"] = true;
  filter_current["precipitation"] = true;
  filter_current["snowfall"] = true;
  filter_current["weather_code"] = true;
  filter_current["cloud_cover"] = true;

  JsonObject filter_daily = filter["daily"].to<JsonObject>();
  filter_daily["weather_code"] = true;
  filter_daily["temperature_2m_max"] = true;
  filter_daily["temperature_2m_min"] = true;
  filter_daily["wind_speed_10m_max"] = true;
  filter_daily["wind_direction_10m_dominant"] = true;
  filter_daily["precipitation_probability_max"] = true;
  filter_daily["precipitation_sum"] = true;
  filter_daily["rain_sum"] = true;
  filter_daily["snowfall_sum"] = true;
  filter_daily["cloud_cover_mean"] = true;
  filter_daily["relative_humidity_2m_mean"] = true;

  WiFiClient client;
  HTTPClient http;
  
  strftime(logWeathStamp[weathLogCounter], sizeof(logWeathStamp[weathLogCounter]), "%d.%m.%y<br>%H:%M:%S", &tm);
  logWeathType[weathLogCounter] = 1;  // 1 = open-meteo
  uint8_t logBase = numberofcities * 2 * weathLogCounter;
  weathLogCounter++;
  if (weathLogCounter == numOfLogs) weathLogCounter = 0;

  #ifdef DEBUG
    Serial.println("Calling Weather API open-meteo.com");
  #endif

  for (uint8_t w = 0; w < numberofcities; w++) {             // version B of weather display: current City 1, forecast City 1, current City 2, forecast City 2
    #ifdef LOGFILE
      uint32_t startMs = millis();                           // timestamp before the first network contact
    #endif
    doc.clear();
    uint8_t helper = logBase + (w * 2);
    uint8_t tableSlot1 = w * 2;
    uint8_t tableSlot2 = w * 2 + 1;
    
    snprintf(logWeathSources[helper], sizeof(logWeathSources[helper]), "%s  <i>%s</i>", cityName_B[w], msgCurrentWeather);
    snprintf(logWeathSources[helper + 1], sizeof(logWeathSources[helper + 1]), "%s  <i>%s</i>", cityName_B[w], msgForecast);
    
    String url;
    url.reserve(400);
    url = "http://api.open-meteo.com/v1/forecast?latitude=";
    url += Lat_B[w];
    url += "&longitude=";
    url += Lon_B[w];
    url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,wind_speed_10m_max,wind_direction_10m_dominant,precipitation_sum,rain_sum,snowfall_sum,cloud_cover_mean,relative_humidity_2m_mean,precipitation_probability_max";
    url += "&current=temperature_2m,wind_speed_10m,wind_direction_10m,precipitation,snowfall,weather_code,cloud_cover,relative_humidity_2m";
    url += "&timezone=Europe%2FBerlin&forecast_days=";
    url += forecastDays;
    if (imperial) url += "&temperature_unit=fahrenheit";

    if (http.begin(client, url)) {
      http.setReuse(true);
      http.setTimeout(10000);                                     // default library value 5000, timeout means html code -11
      int httpCode = http.sendRequest("GET");
      
      snprintf(logWeathHtml[helper], sizeof(logWeathHtml[helper]), "%i", httpCode);
      snprintf(logWeathParse[helper], sizeof(logWeathParse[helper]), "99");
      snprintf(logWeathHtml[helper + 1], sizeof(logWeathHtml[helper + 1]), "%i", httpCode);
      snprintf(logWeathParse[helper + 1], sizeof(logWeathParse[helper + 1]), "99");

      #ifdef DEBUG
        Serial.println(httpCode);
      #endif

      if (httpCode == 200) {
      
        ChunkDecodingStream decodedStream(http.getStream());
        DeserializationError error = deserializeJson(doc, decodedStream, DeserializationOption::Filter(filter));

        if (!error) {
          JsonObject current = doc["current"];                              // current weather
          float current_temp = current["temperature_2m"];
          float wind_speed = current["wind_speed_10m"];
          int wind_deg = current["wind_direction_10m"];
          int current_humidity = current["relative_humidity_2m"];
          float current_precipitation = current["precipitation"];
          float current_snow = current["snowfall"];
          int weather_code = current["weather_code"];
          int current_clouds = current["cloud_cover"];

          char currweath[200];
          char msgNiederschlag_curr[20];     
            if (current_snow == 0){
              snprintf(msgNiederschlag_curr, sizeof(msgNiederschlag_curr),"%s %.1f mm", msgRain, current_precipitation);
            } else {
              snprintf(msgNiederschlag_curr, sizeof(msgNiederschlag_curr),"%s %.1f cm", msgSnow, current_snow);
            }

          snprintf(currweath, sizeof(currweath), "%s %s: %s, %s %.1f %c  %s %d%%  %s %d%%  %s  %s %.1f km/h %s",
                   msgCurrentWeather, cityName_B[w], get_WMO(weather_code), msgTemp, current_temp, tempUnit, 
                   msgHumidity, current_humidity, msgClouds, current_clouds, msgNiederschlag_curr, msgWind, 
                   wind_speed, getWindDirection(wind_deg));
          
          snprintf(logWeathParse[helper], sizeof(logWeathParse[helper]), "1");

          JsonObject daily = doc["daily"];                                // forecast
          JsonArray wmo_arr = daily["weather_code"];
          JsonArray tmax_arr = daily["temperature_2m_max"];
          JsonArray tmin_arr = daily["temperature_2m_min"];
          JsonArray wspd_arr = daily["wind_speed_10m_max"];
          JsonArray wdir_arr = daily["wind_direction_10m_dominant"];
          JsonArray precipitation_arr = daily["precipitation_sum"];
          JsonArray pure_rain_arr = daily["rain_sum"];
          JsonArray snow_arr = daily["snowfall_sum"];
          JsonArray cloud_arr = daily["cloud_cover_mean"];
          JsonArray hum_arr = daily["relative_humidity_2m_mean"];
          JsonArray pop_arr = daily["precipitation_probability_max"];

          char forecasts[forecastDays][250];
          char msgNiederschlag[40];

          for (int i = 0; i < forecastDays; i++) {
            int wmo = wmo_arr[i];
            float tempMin = tmin_arr[i];
            float tempMax = tmax_arr[i];
            float windSpeed = wspd_arr[i];
            int windDeg = wdir_arr[i];
            float precipitation = precipitation_arr[i];
            float pure_rain = pure_rain_arr[i];
            float snow = snow_arr[i];
            int clouds = cloud_arr[i];
            int humidity = hum_arr[i];
            int pop = pop_arr[i];

            if (snow == 0) {                                                                                                          // rain only
              snprintf(msgNiederschlag, sizeof(msgNiederschlag), "%s %.1f mm", msgRain, precipitation);
            } else if (pure_rain != 0) {                                                                                              // snow and rain                                                     
              snprintf(msgNiederschlag, sizeof(msgNiederschlag), "%s %.1f mm  %s %.1f cm", msgPureRain, pure_rain, msgSnow, snow);
            } else {                                                                                                                  // snow only
              snprintf(msgNiederschlag, sizeof(msgNiederschlag), "%s %.1f cm", msgSnow, snow);
            }

            if (pop == 0) {
              snprintf(forecasts[i], sizeof(forecasts[i]), 
                     "%s %s %s: %s, %s %.1f %c %s %.1f %c  %s %d%%  %s %d%%  %s %d%%  %s %.1f km/h %s",
                     msgForecast, cityName_B[w], getDayLabel(i), get_WMO(wmo), 
                     msgTemp, tempMin, tempUnit, msgFromTo, tempMax, tempUnit,
                     msgHumidity, humidity, msgClouds, clouds, msgPop, pop,
                     msgWind, windSpeed, getWindDirection(windDeg));
            } else {
              snprintf(forecasts[i], sizeof(forecasts[i]), 
                     "%s %s %s: %s, %s %.1f %c %s %.1f %c  %s %d%%  %s %d%%  %s %d%%  %s  %s %.1f km/h %s",
                     msgForecast, cityName_B[w], getDayLabel(i), get_WMO(wmo), 
                     msgTemp, tempMin, tempUnit, msgFromTo, tempMax, tempUnit,
                     msgHumidity, humidity, msgClouds, clouds, msgPop, pop,
                     msgNiederschlag, msgWind, windSpeed, getWindDirection(windDeg));
            }
          }

          snprintf(table[tableSlot1].wetcur, sizeof(table[tableSlot1].wetcur), "%s    %s", currweath, forecasts[0]);
          snprintf(table[tableSlot2].wetcur, sizeof(table[tableSlot2].wetcur), "%s    %s", forecasts[1], forecasts[2]);
          utf8AsciiConvert(table[tableSlot1].wetcur, table[tableSlot1].wetcur);
          utf8AsciiConvert(table[tableSlot2].wetcur, table[tableSlot2].wetcur);

          snprintf(logWeathParse[helper + 1], sizeof(logWeathParse[helper + 1]), "1");
          weathEverLoaded = true;

          #ifdef DEBUG
            Serial.println(table[tableSlot1].wetcur);
            Serial.println(table[tableSlot2].wetcur);
          #endif
          #ifdef LOGFILE
           File f = SPIFFS.open("/logfile.txt", "a");                /// logfile DEBUG
           f.printf("%s\n%s\n", timeSaver, "WEATH_1 OK");            /// logfile DEBUG
           f.close();                                                /// logfile DEBUG
          #endif
        } else {                                                      // JSON parse error
             #ifdef DEBUG
              Serial.print("deserializeJson() failed: ");
              Serial.println(error.c_str());
            #endif
            #ifdef LOGFILE
              File f = SPIFFS.open("/logfile.txt", "a");
              uint32_t duration = millis() - startMs;
              f.printf("%s %s %s: %s (Status: %s, Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz, Call Duration: %u)\n", 
                      timeSaver, 
                      "ERROR WEATH_1 PARSE", 
                      cityName_B[w], 
                      error.c_str(), 
                      client.connected() ? "Connected" : "Disconnected", 
                      ESP.getFreeHeap(), 
                      ESP.getMaxAllocHeap(), 
                      WiFi.RSSI(), 
                      getCpuFrequencyMhz(), 
                      duration);
              f.close();
            #endif
            http.end();
            delay(100);
        }
      } else {                                                        // HTTP error
            #ifdef DEBUG
              Serial.printf("http error: %i\n", httpCode);
            #endif
            #ifdef LOGFILE
              File f = SPIFFS.open("/logfile.txt", "a");                                                    /// logfile DEBUG
              f.printf("%s ERROR WEATH_1 HTML %s: %i (%s) (Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz)\n", 
                      timeSaver, 
                      cityName_B[w], 
                      httpCode, 
                      http.errorToString(httpCode).c_str(), 
                      ESP.getFreeHeap(), 
                      ESP.getMaxAllocHeap(), 
                      WiFi.RSSI(), 
                      getCpuFrequencyMhz());
              f.close();                                                                                    /// logfile DEBUG
            #endif
      }
    } else {                                                          // failed connection
        #ifdef DEBUG
          Serial.println("Unable to connect to weather API");
        #endif
    }
    http.end();
    delay(100);
  }
  client.stop();
}


void getWeatherData(){                                    // calls weather data dependent from API choice
  if (weathApiChoice==0) getWeatherData_0();              // openweathermap.com        
  if (weathApiChoice==1) getWeatherData_1();              // open-meteo.com
  if (weathApiChoice==99){                                // both - alternating
    if (weathApiChangeFlag) getWeatherData_0();
    else getWeatherData_1();
    weathApiChangeFlag = !weathApiChangeFlag;
  }
}



////  NEWS FUNCTIONS  ////

void getNewsData_0() {                                                // NEWSAPI.ORG
  uint8_t helper = newssources * newsLogCounter;
  JsonDocument filter;                                             
  JsonObject filter_articles_0 = filter["articles"].add<JsonObject>();
  filter_articles_0["source"]["name"] = true;
  filter_articles_0["title"] = true;
  filter_articles_0["description"] = true;
  filter_articles_0["url"] = true;
  JsonDocument doc;
  
  WiFiClient client;
  HTTPClient http;
  
  #ifdef DEBUG 
    Serial.println("Calling News");
  #endif
  String url;
  url.reserve(150);
  
  strftime (logNewsStamp[newsLogCounter], sizeof(logNewsStamp[newsLogCounter]), "%d.%m.%y<br>%H:%M:%S", &tm);
  logNewsType[newsLogCounter] = 0;
  newsLogCounter++;
  if (newsLogCounter == numOfLogs) newsLogCounter = 0;
 
  for (uint8_t w = 0; w < newssources; w++) {
    #ifdef LOGFILE
      uint32_t startMs = millis();                // timestamp before the first network contact
    #endif
    doc.clear();                        
    uint8_t newsmultiplier = numofarticles * w;                        
    snprintf(logNewsSources[w + helper], sizeof(logNewsSources[w + helper]), "%s", ownNewsSources[w]);
       
    url = "http://newsapi.org/v2/";
    url += ownNewsSources[w];
    url += "&pageSize=";
    url += numofarticles;  // Request 4
    url += "&page=1&apiKey=";
    url += NewsApiKey_0;
    
    #ifdef DEBUG 
      Serial.println(url);
    #endif
    if (http.begin(client, url)) {
      http.setReuse(true);
      int httpCode = http.sendRequest("GET");
     
      snprintf(logNewsHtml[w + helper], sizeof(logNewsHtml[w + helper]), "%i", httpCode);
      snprintf(logNewsParse[w + helper], sizeof(logNewsParse[w + helper]), "99"); 
      
      #ifdef DEBUG  
        Serial.println(httpCode);
      #endif
      
      if (httpCode == 200) {
        
        DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
        
        if (error) {
          #ifdef DEBUG 
            Serial.printf("deserializeJson() failed: %s\n", error.c_str());
          #endif
          #ifdef LOGFILE
            File f = SPIFFS.open("/logfile.txt", "a");
            uint32_t duration = millis() - startMs;
            f.printf("%s ERROR NEWS_0 PARSE %s: %s (Status: %s, Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz, Call Duration: %u)\n", 
                    timeSaver, 
                    ownNewsSources[w], 
                    error.c_str(), 
                    client.connected() ? "Connected" : "Disconnected", 
                    ESP.getFreeHeap(), 
                    ESP.getMaxAllocHeap(), 
                    WiFi.RSSI(), 
                    getCpuFrequencyMhz(), 
                    duration); 
            f.close();
        #endif
        http.end();
        continue;
      }
     
        JsonArray articles = doc["articles"];
        
        for (uint8_t i = 0; i < numofarticles; i++) {              // dynamic loop: handles 3, 4, or 10 articles automatically
             JsonObject article = articles[i];
             bool news_ok = false;
             
             // Variables to hold data
             const char* name = "";
             const char* title = "";
             const char* description = "";
             const char* url = "";
             
             if (article) {
                 news_ok = true;
                 name = article["source"]["name"];
                 title = article["title"];
                 if (article["description"]) description = article["description"];     
                 else description = " - keine Detailinfo verfügbar - ";
                 
                 url = article["url"];
                 
                 if (i == 0) snprintf(logNewsParse[w + helper], sizeof(logNewsParse[w + helper]), "1");      // log parse success if at least one valid article
             }
            
             uint16_t tableIndex = newsmultiplier + i;                  // 0,1,2,3... derived from w*num + i  // writing to main table
             
             if (tableIndex < tablesize) {                              // check if within bounds (sanity check)
                 if (enableNewsXL == 1) {
                    if (news_ok) snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "%s %s: %s: %s", msgNews, name, title, description);
                    else snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "check news source: %s: no info", ownNewsSources[w]); 
                 } else {
                    if (news_ok) snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "%s %s: %s", msgNews, name, title);
                    else snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "check news source: %s", ownNewsSources[w]);
                 }

                  // === SANITIZATION BLOCK === // Cleans the buffer in-place. Replaces ", \n, \r with safe characters.                
                 char* p = table[tableIndex].newscur;
                 while (*p) {
                   if (*p == '"') *p = '\'';      // Double quote -> Single quote
                   if (*p == '\n') *p = ' ';      // Newline -> Space
                   if (*p == '\r') *p = ' ';      // Carriage Return -> Space
                   p++;
                 }
                 // ================================
                 
                 if (news_ok) snprintf(table[tableIndex].newslink, sizeof(table[tableIndex].newslink), "%s", url);
                 else table[tableIndex].newslink[0] = '\0';
             }
        }
        
        newsEverLoaded = true;
        #ifdef LOGFILE
           File f = SPIFFS.open("/logfile.txt", "a");                 /// logfile DEBUG
           f.printf("%s\n%s\n", timeSaver, "NEWS_0 OK");              /// logfile DEBUG
           f.close();                                                 /// logfile DEBUG
        #endif
      } else {                                     // HTTP error
        #ifdef DEBUG 
          Serial.printf("http error: %i\n", httpCode);
        #endif 
        #ifdef LOGFILE
          File f = SPIFFS.open("/logfile.txt", "a");
          f.printf("%s ERROR NEWS_0 HTML %s: %i (%s)\n(Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz)\n", 
                  timeSaver, 
                  ownNewsSources[w],
                  httpCode, 
                  http.errorToString(httpCode).c_str(), 
                  ESP.getFreeHeap(), 
                  ESP.getMaxAllocHeap(), 
                  WiFi.RSSI(), 
                  getCpuFrequencyMhz());
          f.close();
        #endif
      }
   } else {                                       // connection failed
      #ifdef DEBUG 
        Serial.println("Unable to connect to news API");
      #endif
   }
   http.end();
  }
  client.stop();
}


void getNewsData_1() {                                                // THENEWSAPI.COM
  WiFi.setSleep(false);                                               // == WiFi.setSleep(WIFI_PS_NONE);
  uint8_t helper = newssources * newsLogCounter;
  
  JsonDocument doc;
  JsonDocument filter;
  JsonObject filter_data_0 = filter["data"].add<JsonObject>();
  filter_data_0["title"] = true;
  filter_data_0["description"] = true;
  filter_data_0["url"] = true;
  filter_data_0["source"] = true;
  
  WiFiClientSecure client;
  #ifdef NEWS_SECURE
    client.setCACert(cert_the_newsapi);
  #else  
    client.setInsecure();
  #endif 
  
  HTTPClient http;
  
  #ifdef DEBUG 
    Serial.println("Calling News");
  #endif
  
  String url;
  url.reserve(150);
  strftime(logNewsStamp[newsLogCounter], sizeof(logNewsStamp[newsLogCounter]), "%d.%m.%y<br>%H:%M:%S", &tm);
  logNewsType[newsLogCounter] = 1;
  newsLogCounter++;
  if (newsLogCounter == numOfLogs) newsLogCounter = 0;
 
  for (uint8_t w = 0; w < newssources; w++) {
    #ifdef LOGFILE
      uint32_t startMs = millis();                // timestamp before the first network contact
    #endif
    doc.clear();
    uint8_t newsmultiplier = numofarticles * w;
    snprintf(logNewsSources[w + helper], sizeof(logNewsSources[w + helper]), "%s", ownNewsSources_A[w]);
       
    url = "https://api.thenewsapi.com/v1/news/all?api_token=";
    url += NewsApiKey_1;
    url += ownNewsSources_A[w];
    url += "&limit=";
    url += numofarticles;
    
    #ifdef DEBUG 
      Serial.println(url);
      Serial.print("Free heap: ");
      Serial.println(ESP.getFreeHeap());
    #endif
   
    if (http.begin(client, url)) {
      http.setReuse(true);
      //http.useHTTP10(true);                      // for testeing purposes only
      //http.setConnectTimeout(3000);              // default library value 20000, timeout means html code -1
  
      #if ESP_ARDUINO_VERSION_MAJOR >= 3
         http.setTimeout(25000);                   // Core 3 needs more time for mbedTLS
      #else
         http.setTimeout(20000);                   // default library value 5000, timeout means html code -11
      #endif
      http.setUserAgent("Mozilla/5.0 (ESP32; +https://github.com/espressif/esp-idf) AppleWebKit/537.36 (KHTML, like Gecko)");     // brwoser agent, can help in some special cases
      int httpCode = http.sendRequest("GET");
     
      snprintf(logNewsHtml[w + helper], sizeof(logNewsHtml[w + helper]), "%i", httpCode);     
      snprintf(logNewsParse[w + helper], sizeof(logNewsParse[w + helper]), "99");
      
      #ifdef DEBUG  
        Serial.println(httpCode);
      #endif
      
      if (httpCode == 200) {        
        ChunkDecodingStream decodedStream(http.getStream());
        Stream& response = decodedStream;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
        
        if (error) {
          #ifdef DEBUG 
            Serial.printf("deserializeJson() failed: %s\n", error.c_str());
          #endif
          #ifdef LOGFILE
            File f = SPIFFS.open("/logfile.txt", "a");
            uint32_t duration = millis() - startMs;
            f.printf("%s ERROR NEWS_1 PARSE %s: %s (Status: %s, Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz, Call Duration: %u)\n", 
                    timeSaver, 
                    ownNewsSources_A[w], 
                    error.c_str(), 
                    client.connected() ? "Connected" : "Disconnected", 
                    ESP.getFreeHeap(), 
                    ESP.getMaxAllocHeap(), 
                    WiFi.RSSI(), 
                    getCpuFrequencyMhz(), 
                    duration); 
            f.close();
          #endif
          http.end();
          delay(100);
          continue;
        }
        
        JsonArray articles = doc["data"];
       
        for (uint8_t i = 0; i < numofarticles; i++) {             // dynamic loop: handles 3, 4, or 10 articles automatically
             JsonObject article = articles[i];
             bool news_ok = false;
             
             const char* name = "";
             const char* title = "";
             const char* description = "";
             const char* url = "";
             
             if (article) {
                 news_ok = true;
                 name = article["source"];
                 title = article["title"];
                 if (article["description"]) description = article["description"];     
                 else description = " - keine Detailinfo verfügbar - ";
                 
                 url = article["url"];
                 
                 if (i == 0) snprintf(logNewsParse[w + helper], sizeof(logNewsParse[w + helper]), "1");
             }
             
             uint16_t tableIndex = newsmultiplier + i;             // 0,1,2,3... derived from w*num + i  // writing to main table
             
             if (tableIndex < tablesize) {                         // check if within bounds 
                 if (enableNewsXL == 1) {
                    if (news_ok) snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "%s %s: %s: %s", msgNews, name, title, description);
                    else snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "check news source: %s: no info", ownNewsSources_A[w]);
                 } else {
                    if (news_ok) snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "%s %s: %s", msgNews, name, title);
                    else snprintf(table[tableIndex].newscur, sizeof(table[tableIndex].newscur), "check news source: %s", ownNewsSources_A[w]);
                 }

                 
                  // === SANITIZATION BLOCK === // Cleans the buffer in-place. Replaces ", \n, \r with safe characters.
                 char* p = table[tableIndex].newscur;
                 while (*p) {
                   if (*p == '"') *p = '\'';      // Double quote -> Single quote
                   if (*p == '\n') *p = ' ';      // Newline -> Space
                   if (*p == '\r') *p = ' ';      // Carriage Return -> Space
                   p++;
                 }
                 // ================================
                 
                 if (news_ok) snprintf(table[tableIndex].newslink, sizeof(table[tableIndex].newslink), "%s", url);
                 else table[tableIndex].newslink[0] = '\0';
             }
        }

        #ifdef LOGFILE
           File f = SPIFFS.open("/logfile.txt", "a");                 /// logfile DEBUG
           f.printf("%s\n%s\n", timeSaver, "NEWS_1 OK");              /// logfile DEBUG
           f.close();                                                 /// logfile DEBUG
          #endif
        newsEverLoaded = true;
      } else {                                    // HTTP error
        #ifdef DEBUG 
          Serial.printf("http error: %i\n", httpCode);
        #endif 
        #ifdef LOGFILE
          File f = SPIFFS.open("/logfile.txt", "a");
          f.printf("%s ERROR NEWS_1 HTML %s: %i (%s)\n(Heap: %u, MaxAlloc: %u, RSSI: %d, CPU: %u MHz)\n", 
                  timeSaver, 
                  ownNewsSources_A[w], 
                  httpCode, 
                  http.errorToString(httpCode).c_str(), 
                  ESP.getFreeHeap(), 
                  ESP.getMaxAllocHeap(), 
                  WiFi.RSSI(), 
                  getCpuFrequencyMhz());
          f.close();
        #endif
      }
   } else {                                       // connection failed
      #ifdef DEBUG 
        Serial.println("Unable to connect to news API");
      #endif
   }
   http.end();
   delay(100);
  }
  client.stop();
  WiFi.setSleep(true);                          // == WiFi.setSleep(WIFI_PS_MIN_MODEM);
}


void getNewsData(){                                                  // calls data dependent from API choice
  if (newsApiChoice==0) getNewsData_0();                             // newsapi.org
  if (newsApiChoice==1) getNewsData_1();                             // thenewsapi.com             
  if (newsApiChoice==99){                                            // both - alternating
    if (apiChangeFlag) getNewsData_0();
    else getNewsData_1();
    apiChangeFlag = !apiChangeFlag;
  }
}

#ifdef NEWS_SECURE
void loadNewsAuth() {                                                // loads certificate from file
    File A = SPIFFS.open("/cert_news.txt", "r");
    if (!A) {
        #ifdef DEBUG
          Serial.println("cert_news.txt not found");
        #endif
        return;
    }
    
    A.setTimeout(300);                                                // timeout for read operations (ms)
    
    size_t fileSize = A.size();
    if (fileSize == 0 || fileSize >= sizeof(cert_the_newsapi)) {
        #ifdef DEBUG
          Serial.println("News certificate file invalid or too large!");
        #endif
        A.close();
        return;
    }
    
    size_t bytesRead = A.readBytes(cert_the_newsapi, fileSize);
    cert_the_newsapi[bytesRead] = '\0';                               // null-terminate
    A.close();
    
    #ifdef DEBUG
      Serial.println("TheNewsApi.com Server Certificate loaded successfully");
    #endif   
}
#endif


//// MUSIC API FUNCTIONS ////

#ifdef SPOTIFY
/// SPOTIFY FUNCTIONS ///

void spotiauth() {
  #ifdef DEBUG
    Serial.println("First Step: Calling Spotify for one time authentication token...");
  #endif 
  
  String firstContact;
  firstContact.reserve(300);  // Pre-allocate
  firstContact = "https://accounts.spotify.com/de/authorize/?client_id=";
  firstContact += clientID;
  firstContact += "&response_type=code&redirect_uri=";
  firstContact += redirectUri;
  firstContact += "&scope=user-read-currently-playing";
  
  server.sendHeader("Location", firstContact);
  server.send(302, "text/plain", "ok");    
}


void spoticallback() {                               // handles complete Spotify authentication process
  if(!server.hasArg("code")){
    server.send(500, "text/plain", "Server call failed!");
    return;
  }
  String oneTimeCode;
  oneTimeCode = server.arg("code");                  // one time token sent by spotify                  
  #ifdef DEBUG 
    Serial.printf("Second Step: Received one time authentication token: %s  --> Calling for persistent refresh token...\n", oneTimeCode.c_str());
  #endif
  bool authSuccess = false;
  WiFiClientSecure client;
  //client.setInsecure();                    // for testing purposes only
  client.setCACert(cert_spotify_account);    // must be disabled if setInsecure
  HTTPClient http;

  if (http.begin(client, "https://accounts.spotify.com/api/token")) {
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      http.setTimeout(15000);                  // Core 3 needs more time for mbedTLS
    #endif
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String completeAuth;
    completeAuth.reserve(300);  // Pre-allocate
    completeAuth = "grant_type=authorization_code&code=";
    completeAuth += oneTimeCode;
    completeAuth += "&redirect_uri=";
    completeAuth += redirectUri;
    completeAuth += "&client_id=";
    completeAuth += clientID;
    completeAuth += "&client_secret=";
    completeAuth += clientSecret;
    
    int httpCode = http.POST(completeAuth);
    #ifdef DEBUG
      Serial.println(httpCode);
    #endif
    if(httpCode == 200) {                     
      JsonDocument doc;                                                   // Json document received from Spotify
      DeserializationError error = deserializeJson(doc, http.getStream());
      if (error) {
        #ifdef DEBUG
          Serial.println("deserializeJson() failed: ");
          Serial.println(error.c_str());
        #endif 
      http.end();
      client.stop();
      return;
      }
      const char* initialAccesstoken = doc["access_token"];                          // access token valid until expire time elapses
      const char* initialRefreshtoken = doc["refresh_token"];                        // persistent refresh token for all future Spotify api calls, will be saved on SPIFFS 
      
      accesstoken = "Bearer ";
      accesstoken += initialAccesstoken;
      
      refreshtoken = initialRefreshtoken;
      expiretime = doc["expires_in"];
      
      String credentials;
      credentials.reserve(100);
      credentials = clientID;
      credentials += ":";
      credentials += clientSecret;
      auth = "Basic ";
      auth += base64::encode(credentials);
      
      authSuccess = true;
      } else {
      #ifdef DEBUG 
        Serial.printf("http error: %i\n", httpCode);
      #endif
      } 
    } else {
      #ifdef DEBUG 
        Serial.println("Unable to connect to Spotify");
      #endif
    }
  http.end();
  client.stop();
  
  if (authSuccess == true){
     #ifdef DEBUG
       Serial.println("Spotify Authentication Process completed:");
       Serial.print("Refreshtoken: ");
       Serial.println(refreshtoken);
       Serial.print("Accesstoken: ");
       Serial.println(accesstoken);
       Serial.print("expires in ");
       Serial.print(expiretime);
       Serial.println(" seconds.");
     #endif

     remainingRefreshTokenDays = 179;
     
     time_t now = time(NULL);
     time(&now);
     
     File f = SPIFFS.open("/spotifyAuth.txt", "w");                         // writes persistent refreshtoken and auth credentials to file -> will be read on startup
     f.printf("%s\n%s\n%ld\n", auth.c_str(), refreshtoken.c_str(),(long)now);
     f.close();
  
      #ifdef DEBUG
        Serial.println("Spotify credentials saved to file");
      #endif
      //parseSpotify();                                                     // you can uncomment this for debugging purposes
     
      char successUri[40];
      snprintf(successUri, sizeof(successUri), "http://%d.%d.%d.%d/spotAuthOK.html", 
               WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      server.sendHeader("Location", successUri);                      // redirets to html-page with success message
      server.send(303);
   } else {
      char FailUri[40];
      snprintf(FailUri, sizeof(FailUri), "http://%d.%d.%d.%d/spotAuthFail.html", 
               WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      server.sendHeader("Location", FailUri);                       // redirets to html-page with fail message
      server.send(303);
   }
}


void refreshSpotify(){                                                      // demands and receives a new access token (on startup or when expiretime has elapsed)
  uint8_t refresh_retry = 0;
  bool refresh_success = false;
  
  while(refresh_retry < 5 && refresh_success == false){  
    #ifdef DEBUG 
      Serial.println("Calling Spotify for accesstoken");
    #endif
    
    char stamp[23];                                                            
    strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm);           
    
    WiFiClientSecure client;
    //client.setInsecure();                    // for testing purposes only
    client.setCACert(cert_spotify_account);    // must be disabled if setInsecure
    HTTPClient http;

    if(http.begin(client, "https://accounts.spotify.com/api/token")){
      #if ESP_ARDUINO_VERSION_MAJOR >= 3
        http.setTimeout(15000);                  // Core 3 needs more time for mbedTLS
      #endif
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); 
      http.addHeader("Authorization", auth);
  
      String url;
      url.reserve(100);
      url = "grant_type=refresh_token&refresh_token=";
      url += refreshtoken;
      
      int httpCode =  http.POST(url);
      snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Spotify Refresh >> Html:%i, Parse:99 &#10060;", stamp, httpCode); 
      #ifdef DEBUG 
        Serial.println(httpCode);
      #endif

      if(httpCode == 200) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getStream());
        if (error) {
          #ifdef DEBUG
            Serial.println("deserializeJson() failed: ");
            Serial.println(error.c_str());
          #endif
          refresh_retry++;
          musicLogCounter++;                                        
          if (musicLogCounter==numOfMusicLogs) musicLogCounter=0;
          http.end();
          client.stop(); 
          return;
        }
        const char* newtoken = doc["access_token"];
        expiretime = doc["expires_in"];
        #ifdef DEBUG 
          Serial.print("New accesstoken: ");
          Serial.print(newtoken);
          Serial.print(" expires in ");
          Serial.print(expiretime);
          Serial.println(" seconds.");
        #endif 
        accesstoken = "Bearer " + String(newtoken);
        #ifdef DEBUG
          Serial.println(accesstoken);
        #endif
        refresh_success = true;
        //parseSpotify();                                   // you can uncomment this for debugging purposes
        snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Spotify Refresh >> Html:%i, Parse:ok  &#9989;", stamp, httpCode);  
      } else {
        #ifdef DEBUG 
          Serial.printf("http error: %i\n", httpCode);
        #endif
        refresh_retry++;
      }
    } else {
      #ifdef DEBUG 
        Serial.println("Unable to connect to Spotify");
      #endif
      refresh_retry++;
    }
    http.end();
    client.stop();
    musicLogCounter++;                                  
    if (musicLogCounter==numOfMusicLogs) musicLogCounter=0; 
    if (!refresh_success) yield();                 // Yield between retries
  }
  
  if(refresh_retry >= 5) {
    enableSpotify = 0;
    #ifdef DEBUG 
      Serial.println("Call for accesstoken failed! Spotify message disabled.");
    #endif
     snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
     snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
     enableOrDisable(locationSpot,showspotify, enableSpotify,1);
     expiretime = 1; 
  }
}



void parseSpotify(){                                  // calls Spotify api with accesstoken and receives Json document with data about currently playing song                                    
 WiFiClientSecure client;
 //client.setInsecure();                    // for testing purposes only
 client.setCACert(cert_spotify_api);        // must be disabled if setInsecure
 HTTPClient http;

  #ifdef DEBUG 
    Serial.println("Calling Spotify for currently playing");
  #endif

  char stamp[23];
  strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm);            
  
  if(http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing")){
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      http.setTimeout(15000);                  // Core 3 needs more time for mbedTLS
    #endif
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", accesstoken);
    int httpCode = http.sendRequest("GET");
    snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Spotify Music >> Html:%i, Parse:99 &#10060;", stamp, httpCode);  
    #ifdef DEBUG 
      Serial.println(httpCode);
    #endif 
    
    if(httpCode == 200) {
      
      JsonDocument filter;

      JsonObject filter_item = filter["item"].to<JsonObject>();
      filter_item["album"]["images"] = true;
      filter_item["name"] = true;
      filter_item["artists"][0]["name"] = true;
      
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      
      if (error) {
        hideMusicInfo();
        snprintf(spotify, sizeof(spotify), "%s (parserr)", msgSpotifyProb);        // information on spotify.html if parsing error occurs
        snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
        #ifdef DEBUG
          Serial.println("deserializeJson() failed: ");
          Serial.println(error.c_str());
        #endif
        musicLogCounter++;
        if (musicLogCounter==numOfMusicLogs) musicLogCounter=0; 
        http.end();
        client.stop();
        return;
      }
      JsonObject item = doc["item"];
      JsonObject item_album = item["album"];
      JsonArray item_album_images = item_album["images"];
      JsonObject item_album_images_0 = item_album_images[0];
      const char* coverurl = item_album_images_0["url"];
      const char* item_name = item["name"];
      JsonObject item_artists_0 = item["artists"][0];
      const char* artist = item_artists_0["name"];
  
      #ifdef DEBUG
        Serial.println("Spotify Response:");
        Serial.println(coverurl);
        Serial.println(artist);
        Serial.println(item_name);
      #endif 
  
      snprintf(spotify, sizeof(spotify), "%s<br><span class=\"von\">%s</span><br>%s", item_name, msgBy, artist);             // information shown on spotify.html
      snprintf(showspotify, sizeof(showspotify), "%s: \"%s\" %s %s", msgSpotifyOK, item_name, msgBy, artist);                // information shown on display
      utf8AsciiConvert(showspotify, showspotify);
      showMusicInfo();
      //strcpy(spotifycover,coverurl);
      snprintf(spotifycover, sizeof(spotifycover), "%s", coverurl);

      snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Spotify Music >> Html:%i, Parse:ok  &#9989;", stamp, httpCode);  
      #ifdef DEBUG 
        Serial.println(showspotify);
      #endif
    } else {
      hideMusicInfo();
      if (httpCode == 204){
        snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Spotify Music >> Html:%i, Parse:99  &#9898;", stamp, httpCode); 
        snprintf(spotify, sizeof(spotify), "%s (204)", msgSpotifyProb);
      } else {
        snprintf(spotify, sizeof(spotify), "%s (!=200)", msgSpotifyProb);         // information on spotify.html if no data available
      }
      snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
      #ifdef DEBUG 
        Serial.printf("http error: %i\n", httpCode);
      #endif 
    }
  } else {
    snprintf(spotify, sizeof(spotify), "%s (nocon)", msgSpotifyProb);          // information on spotify.html if no connection
    snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
    hideMusicInfo();
    #ifdef DEBUG 
      Serial.println("Unable to connect to Spotify");
    #endif
  }
http.end();
client.stop();
musicLogCounter++;
if (musicLogCounter==numOfMusicLogs) musicLogCounter=0; 
}


bool loadCertificate(const char* path, char* destBuffer, size_t maxBufferSize) {
    File f = SPIFFS.open(path, "r");
    if (!f) {
        #ifdef DEBUG
          Serial.printf("%s not found\n", path);
        #endif
        return false;
    }
    
    f.setTimeout(300);
    size_t fileSize = f.size();
    if (fileSize == 0 || fileSize >= maxBufferSize) {
        #ifdef DEBUG
          Serial.printf("Cert file %s invalid or too large!\n", path);
        #endif
        f.close();
        return false;
    }
    
    size_t bytesRead = f.readBytes(destBuffer, fileSize);
    destBuffer[bytesRead] = '\0';
    f.close();
    return true;
}

void loadSpotifyAuth() {
    File f = SPIFFS.open("/spotifyAuth.txt", "r");                  // loads auth and refresh token
    if (!f) {
        #ifdef DEBUG
          Serial.println("spotifyAuth.txt not found");
        #endif
        return;
    }
    
    String line;
    line.reserve(200);
    
    line = f.readStringUntil('\n');
    line.trim();
    auth = line;
    
    line = f.readStringUntil('\n');
    line.trim();
    refreshtoken = line;

    line = f.readStringUntil('\n');                                 // loads timestamp of last refresh token call (valid for 180 days)
    line.trim();
    long savedTimestamp = line.toInt();
    f.close();

    const long maxAgeSeconds = 15552000L;                           // 180 days in seconds
    time_t now = time(NULL);

    long remainingSeconds = maxAgeSeconds - (now - savedTimestamp);   // remaining seconds (can be negative if now > savedTimestamp + maxAgeSeconds)
    
    remainingRefreshTokenDays = remainingSeconds / (24 * 60 * 60);    // caluclates days
    
    #ifdef DEBUG
      Serial.println("Spotify Data loaded from file:");
      Serial.print("Auth: ");
      Serial.println(auth);
      Serial.print("Refreshtoken: ");
      Serial.println(refreshtoken);
      if (remainingRefreshTokenDays < 0){
        Serial.printf("Warning: Token expired since %ld days\n", abs(remainingRefreshTokenDays));
      } else { 
        Serial.printf("Token valid for %i days.\n", remainingRefreshTokenDays);
      }
    #endif 

    if (loadCertificate("/cert_spot.txt", cert_spotify_account, sizeof(cert_spotify_account))){
    #ifdef DEBUG 
     Serial.println("Server Certificate ACCOUNT loaded successfully");
    #endif 
    }
    if (loadCertificate("/cert_spot_api.txt", cert_spotify_api, sizeof(cert_spotify_api))){
    #ifdef DEBUG 
     Serial.println("Server Certificate API loaded successfully");
    #endif 
    }
}

#endif                                    // end #ifdef SPOTIFY


#ifdef CASTWEB
/// CAST WEB FUNCTIONS ///

void parseCastWeb(){                                  // calls CastWeb api and receives Json document with data about currently playing song                                    
  static uint8_t callAttempt = 0;
  if (strcmp(castWebIP, "") == 0 || strcmp(castDeviceID, "") == 0){
  #ifdef DEBUG 
    Serial.println("Cast Web API Parameters Missing -> Music Info Message disabled. ");
  #endif
    enableSpotify = 0;
    enableOrDisable(locationSpot,showspotify, enableSpotify,1);
  } else {
  WiFiClient client;
  HTTPClient http;
  #ifdef DEBUG 
    Serial.println("Calling Cast Web API");
  #endif

  char stamp[23];
  strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm);
  
  String url;
  url.reserve(80);
  url = "http://";
  url += castWebIP;
  url += "/device/";
  url += castDeviceID;
 
    if(http.begin(client,url)){
    int httpCode = http.sendRequest("GET");
    snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Castweb >> Html:%i, Parse:99 &#10060;", stamp, httpCode);
    #ifdef DEBUG 
      Serial.println(httpCode);
    #endif 
    
    if(httpCode == 200) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, http.getStream());
      if (error) {
        snprintf(spotify, sizeof(spotify), "%s (parserr)", msgSpotifyProb);               // information on spotify.html if parsing error occurs
        snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
        hideMusicInfo();
        #ifdef DEBUG
          Serial.println("deserializeJson() failed: ");
          Serial.println(error.c_str());
        #endif
        musicLogCounter++;
        if (musicLogCounter==numOfMusicLogs) musicLogCounter=0;
        client.stop();
        return;
      }
      const char* connection = doc["connection"];
      if (strcmp(connection, "connected") == 0){                                                                                              // if defined cast web device is connected
        JsonObject status = doc["status"];
        const char* status_application;
        const char* status_status;
        const char* status_title;
        const char* status_subtitle;
        const char* status_image;
        if (status["application"]) status_application = status["application"]; else status_application = "";    // "TuneIn Free"
        if (status["status"]) status_status = status["status"]; else status_status = "";                        // "PLAYING"
        if (status["title"]) status_title = status["title"]; else status_title= "";                             // "Mein Radio Tirol"
        if (status["subtitle"]) status_subtitle = status["subtitle"]; else status_subtitle= "";                 // "ORF Radio Tirol"
        if (status["image"]) status_image = status["image"]; else status_image= "";                             // "https://cdn-profiles.tunein.com/s10740/images/logog.jpg?t=153668"
        #ifdef DEBUG
          Serial.println("Cast Web Api Response:");
          Serial.println(status_application);
          Serial.println(status_status);
          Serial.println(status_title);
          Serial.println(status_subtitle);
          Serial.println(status_image);
        #endif 

        if(strcmp(status_title, "") == 0 || strcmp(status_status, "PAUSED") == 0){  
          snprintf(spotify, sizeof(spotify), "%s (!='  ')", msgSpotifyProb);                                                                   // information on spotify.html if no data available
          snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
          hideMusicInfo(); 
          snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Castweb >> Html:%i, DevStop  &#9898;", stamp, httpCode);
          musicSource[0] = '\0';                                                                                        
          #ifdef DEBUG 
            Serial.println("no information availabe - cast web device paused ot stopped");
          #endif 
        } else {
          if (strcmp(status_application, "Spotify") == 0){                                                                                       // if cast web device is playing spotify
            snprintf(spotify, sizeof(spotify), "%s<br><span class=\"von\">%s</span><br>%s", status_title, msgBy, status_subtitle);               // information shown on spotify.html
            snprintf(showspotify, sizeof(showspotify), "%s: \"%s\" %s %s", msgSpotifyOK, status_title, msgBy, status_subtitle);                  // information shown on display
            utf8AsciiConvert(showspotify, showspotify);
            showMusicInfo(); 
            strcpy(spotifycover,status_image);
            snprintf(musicSource, sizeof(musicSource), "%s", status_application);
          } else {
            snprintf(spotify, sizeof(spotify), "%s", status_title);                                                                              // information shown on spotify.html
            snprintf(showspotify, sizeof(showspotify), "%s '%s':   %s", msgCastWebOK, status_subtitle, status_title);                            // information shown on display
            utf8AsciiConvert(showspotify, showspotify);
            showMusicInfo();
            strcpy(spotifycover,status_image);
            snprintf(musicSource, sizeof(musicSource), "%s", status_subtitle);
          }
          snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Castweb >> Html:%i, Parse:ok  &#9989;", stamp, httpCode);
          #ifdef DEBUG 
           Serial.println(showspotify);
          #endif
        }  
      } else {                                                                                                                                   // if defined cast web device is not connected
          snprintf(spotify, sizeof(spotify), "%s (na)", msgSpotifyProb);
          snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
          musicSource[0] = '\0';
          hideMusicInfo();
          snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Castweb >> Html:%i, DevNoCon  &#9898;", stamp, httpCode);
          #ifdef DEBUG 
            Serial.println("cast web device is not available");
          #endif 
      }
    } else {
      snprintf(spotify, sizeof(spotify), "%s (!=200)", msgSpotifyProb);         // information on spotify.html if no data available
      snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
      musicSource[0] = '\0';
      hideMusicInfo();
      #ifdef DEBUG 
        Serial.printf("http error: %i\n", httpCode);
      #endif
      callAttempt ++;
      if (callAttempt >7){                                                      // if API can't be reached within 8 attempts, Spotify message is deactivated                                                  
        enableSpotify = 0;
        callAttempt = 0;
        #ifdef DEBUG 
          Serial.println("Cast Web Api not availabe -> Music Info Message disabled.");
        #endif
        snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
        snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
        enableOrDisable(locationSpot,showspotify, enableSpotify,1);
        musicSource[0] = '\0';
        #ifdef SPOTIFY
          expiretime = 1;
        #endif
      }
    }
  } else {
    snprintf(spotify, sizeof(spotify), "%s (nocon)", msgSpotifyProb);          // information on spotify.html if no connection
    snprintf(spotifycover, sizeof(spotifycover), "/Spotprob.jpg");
    hideMusicInfo();
    #ifdef DEBUG 
      Serial.println("Unable to connect to Cast Web Api");
    #endif
  }
 http.end();
 client.stop();
 musicLogCounter++;
 if (musicLogCounter==numOfMusicLogs) musicLogCounter=0; 
 }                                                                             // else from first "if (strcmp(castWebIP, "") == 0 || strcmp(castDeviceID, "") == 0)"
}


void getCastWebDevices(){                                         // sends API IP and active devcie to requesting page castweb.htnl                               
  #ifdef DEBUG 
    Serial.println("Sending Cast Web Configuration");
  #endif
  
  String temp;
  // Safe pre-allocation formula for String reserve:                          // String optimization to avoid heap fragmentation 
  // castWebIP(16) + castDeviceID(40) + devicename(25) + overhead(50)
  // = 16 + 40 + 25 + 50 = 131 → rounds to 256 ✅
  temp.reserve(256);  // pre-allocate based on expected size
  
  temp = "{\"CurrIP\":\"";
  temp += castWebIP;
  temp += "\",\"CurrDevice\":\"";
  temp += castDeviceID;
  temp += "\",\"Name\":\"";
  temp += devicename;
  temp += "\"}";

  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
  #endif
}


void handleCastWebDevices(){
if (server.hasArg("newIP")){
   snprintf(castWebIP, sizeof(castWebIP), "%s", server.arg("newIP").c_str());
   server.send(200, "text/plain", "");
  #ifdef DEBUG
      Serial.printf("IP for Cast Web Api Bridge received and saved to file: \"%s\" \n", castWebIP);
  #endif
  }
   if (server.hasArg("newDev")){
   snprintf(castDeviceID, sizeof(castDeviceID), "%s", server.arg("newDev").c_str());
   server.send(200, "text/plain", "");
  #ifdef DEBUG
      Serial.printf("Cast Web Device ID received and saved to file: \"%s\" \n", castDeviceID);
  #endif
  }
   File f = SPIFFS.open("/CastCfg.txt", "w"); 
   f.printf("%s\n%s\n", castWebIP, castDeviceID);
   f.close();
}



void loadCastWebDevices() {                                                           // loads configuration data from file on SPIFFS
  File f = SPIFFS.open("/CastCfg.txt", "r");
  if(!f) {
   #ifdef DEBUG
     Serial.println("file open failed");
   #endif
    enableSpotify = 0;
    enableOrDisable(locationSpot,showspotify, enableSpotify,1);
    return;
  }
  
  String line;
  line.reserve(64);  // Pre-allocate for config lines
  
  line = f.readStringUntil('\n');
  line.trim();
  snprintf(castWebIP, sizeof(castWebIP), "%s", line.c_str());
  
  line = f.readStringUntil('\n');
  line.trim();
  snprintf(castDeviceID, sizeof(castDeviceID), "%s", line.c_str());
  
  f.close();
  
  #ifdef DEBUG
    Serial.println("Loaded from file successfully:");
    Serial.printf("IP for Cast Web Api Bridge:\"%s\", Cast Web Device ID:\"%s\"\n", castWebIP, castDeviceID);
  #endif
}


void parseMusicInfo() {                                                            // API for sending simple music info data from external devive via curl (batch on Windows PC e.g.)
  if (server.hasArg("musicInfo")){
  String musicData = server.arg("musicInfo");                                              
  snprintf(musicInfoBuf, sizeof(musicInfoBuf), "%s %s", msgCastWebOK, musicData.c_str());    // information shown on display
  musicData = "<span style=\"color:#af601a;\">" + musicData;
  musicData.replace(" - ","</span><br>");
  if (enableSpotify==3){
    char stamp[23];
    strftime (stamp, sizeof(stamp), "%d.%m.%y <br> %H:%M:%S", &tm); 
    musicLogCounter++;
    if (musicLogCounter==numOfMusicLogs) musicLogCounter=0; 
    
    snprintf(spotify, sizeof(spotify), "%s", musicData.c_str());                            // information shown on spotify.html
    snprintf(spotifycover, sizeof(spotifycover), "/musicExtern.jpg");
    snprintf(logMusic[musicLogCounter], sizeof(logMusic[musicLogCounter]), "%s Music Extern >> Html:200, Parse:ok  &#9989;", stamp);
    #ifdef DEBUG
      Serial.print(spotify);
      Serial.println(" received and processed successfully");
    #endif
  }
  newMusicData = true;
  server.send(200, "text/plain", "");
 }
}


void newMusicLoad() {                                                                         // loads data from buffer into show table
  if (newMusicData==true) {
     utf8AsciiConvert(musicInfoBuf, showspotify);
     showMusicInfo();
     newMusicData = false;
  }
}

#endif

//// END OF MUSIC API FUNCTIONS ////


void enableOrDisable(uint8_t loc, char* msg, uint8_t ena, uint8_t specialcase) {             // main function for defining values of all messages EXCEPT...
  if (ena==1){                                                                               // ...the custom messages of array messages[] which are handled via admin html page
      if (specialcase==2){                  //special case timeshow effect and pause
        catalog[loc].effectin = 1;
        catalog[loc].effectout = 1;
        catalog[loc].just = 1;
        catalog[loc].speed = 4;
        catalog[loc].pause = timeDuration;
        catalog[loc].delay = 1;
      } 
      if (specialcase==3){                  //special case dateshow effect and pause
       #ifdef SHORTDATE
        catalog[loc].effectin = 2;
        catalog[loc].effectout = 2;
        catalog[loc].just = 1;
        catalog[loc].speed = 4;
        catalog[loc].pause = 6;
        catalog[loc].delay = 1;
       #else
        catalog[loc].effectin = 3;
        catalog[loc].effectout = 3;
        catalog[loc].just = 1;
        catalog[loc].speed = globScrollSpeed;
        catalog[loc].pause = 0;
        catalog[loc].delay = 1;
       #endif
      }
      if (specialcase==0){                // standard for all other messages
        catalog[loc].effectin = 3;
        catalog[loc].effectout = 3;
        catalog[loc].just = 1;
        catalog[loc].speed = globScrollSpeed;
        catalog[loc].pause = 0;
        catalog[loc].delay = 1;
      }
      if(specialcase==1){                 // special case showspotify delay
        catalog[loc].effectin = 3;
        catalog[loc].effectout = 3;
        catalog[loc].just = 1;
        catalog[loc].speed = globScrollSpeed;
        catalog[loc].pause = 0;
        catalog[loc].delay = 1;
      }
      if (specialcase==5){                // special case Service-Info
        if (enableServiceMsg==1){
          catalog[loc].effectin = 10;
          catalog[loc].effectout = 10;
          catalog[loc].just = 1;
          catalog[loc].speed = 2;
          catalog[loc].pause = 3;
          catalog[loc].delay = 1;
        } else {
        msg[0] = '\0';                                            // directly set first byte to null terminator  -> snprintf(msg, sizeof(msg), ""); not correct
        catalog[loc].effectin = 0;
        catalog[loc].effectout = 26;
        catalog[loc].just = 1;
        catalog[loc].speed = 0;
        catalog[loc].pause = 0;
        catalog[loc].delay = 0;
        }
      }
  } else if (ena==2 || ena==3){           // special case castweb or music extern -> required only for service message. showspotify is handled via showMusicInfo() and hideMusicInfo()
      if (specialcase==5){                // special case Service-Info
        if (enableServiceMsg==1){
          catalog[loc].effectin = 10;
          catalog[loc].effectout = 10;
          catalog[loc].just = 1;
          catalog[loc].speed = 2;
          catalog[loc].pause = 3;
          catalog[loc].delay = 1;
        } else {
        msg[0] = '\0'; 
        catalog[loc].effectin = 0;
        catalog[loc].effectout = 26;
        catalog[loc].just = 1;
        catalog[loc].speed = 0;
        catalog[loc].pause = 0;
        catalog[loc].delay = 0;
        }
      }
  } else {                                 // standard if messages deactivated -> ena = 0
      msg[0] = '\0'; 
      catalog[loc].effectin = 0;
      catalog[loc].effectout = 26;
      catalog[loc].just = 1;
      catalog[loc].speed = 0;
      catalog[loc].pause = 0;
      catalog[loc].delay = 0;
      }
}


void hideMusicInfo(){                                                                   // hides music service message and spotify message
  showspotifyService[0] = '\0';
  enableOrDisable(locationSpot-1,showspotifyService, 0, 5);
  enableOrDisable(locationSpot,showspotify, 0,1);
}

void showMusicInfo(){                                                                   // shows music service message and spotify message
  if (enableServiceMsg==1) snprintf(showspotifyService,sizeof(showspotifyService), "%s", spotifyService);
  enableOrDisable(locationSpot-1,showspotifyService, 1, 5);
  enableOrDisable(locationSpot,showspotify, 1,1);
}


void allOffCheck(){                                                                     // checks enable state of messages and activates modes

  bool allMessagesOff = true;
  for (uint8_t i = 0; i < numOfMessages; i++) {
    if (enablemessage[i] != 0) { allMessagesOff = false; break; }
  }
  
  bool allGraphicsOff = true;
  for (uint8_t i = 0; i < numOfGraphics; i++) {
    if (enableGraph[i] != 0) { allGraphicsOff = false; break; }
  }
  
  bool allGraphicsExceptFirstOff = true;
  for (uint8_t i = 1; i < numOfGraphics; i++) {
    if (enableGraph[i] != 0) { allGraphicsExceptFirstOff = false; break; }
  }
  
  bool allMessagesExceptFirstOff = true;
  for (uint8_t i = 1; i < numOfMessages; i++) {
    if (enablemessage[i] != 0) { allMessagesExceptFirstOff = false; break; }
  }
  
  if (enableDate==0 && allMessagesOff && enableWeath==0 && enableNews1==0                        // condition: only time (everything else off)
      && enableSpotify==0 && enableNews2==0 && enableOwn==0 && allGraphicsOff) {
    timeOnly = true;
    graphOnly = false;
    singleMsgOnly = false;
  }
  
  else if (enableGraph[0]!=0 && enableTime==0 && enableDate==0 && allMessagesOff                  // condition: only graphics[0] (everything else off including time)
           && enableWeath==0 && enableNews1==0 && enableSpotify==0 && enableNews2==0 
           && enableOwn==0 && allGraphicsExceptFirstOff) {
    timeOnly = false;
    graphOnly = true;
    singleMsgOnly = false;     
  }
  
  else if (enablemessage[0]==1 && enableTime==0 && enableDate==0 && allMessagesExceptFirstOff     // condition: only message[0] (everything else off including time)
           && enableWeath==0 && enableNews1==0 && enableSpotify==0 && enableNews2==0 
           && enableOwn==0 && allGraphicsOff) {
    timeOnly = false;
    graphOnly = false;
    singleMsgOnly = true;       
  } 
  else {                                                                                          // regular mode
    timeOnly = false;
    graphOnly = false;
    singleMsgOnly = false;
  }
}



//// SPIFFS AND WEBSERVER RELATED FUNCTIONS ////

File openWithGzip(const String& path) {                          // helper: opens file, preferring .gz version if available. streamFile handles gzip encoding automatically.
  if (SPIFFS.exists(path + ".gz")) {
    return SPIFFS.open(path + ".gz", "r");
  }
  return SPIFFS.open(path, "r");
}

bool fileExists(const String& path) {                           // helper: checks if file exists (plain or .gz)
  return SPIFFS.exists(path) || SPIFFS.exists(path + ".gz");
}

void html_authentify(String& site) {
  if (enableAuth == 1 && !server.authenticate(www_username, www_password) && !server.authenticate(fallback_username, fallback_password)) {
    return server.requestAuthentication();
  } 
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));
    server.sendHeader("Location", "/spiffs.html");
    server.send(303);
  } else {
    if (enableAuth == 1){
      server.sendHeader("Cache-Control", "private, no-cache");
    } else {
      server.sendHeader("Cache-Control", "max-age=2628000");
    }
    File f = openWithGzip(site); server.streamFile(f, contentType(site)); f.close();
  }
}


void html_authentify_ota() {
  if (enableAuth == 1 && !server.authenticate(www_username, www_password) && !server.authenticate(fallback_username, fallback_password)) {
    return server.requestAuthentication();
  }
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));
    server.sendHeader("Location", "/spiffs.html");
    server.send(303);
  } else {
   if (enableAuth == 1){
      server.sendHeader("Cache-Control", "private, no-cache");
    } else {
      server.sendHeader("Cache-Control", "max-age=2628000");
    }
    #ifdef OTA
      File f = openWithGzip("/ota.html"); server.streamFile(f, "text/html"); f.close();
    #else    
      File f = openWithGzip("/noota.html"); server.streamFile(f, "text/html"); f.close();
    #endif     
  }
}


void html_authentify_castweb() {
  if (enableAuth == 1 && !server.authenticate(www_username, www_password) && !server.authenticate(fallback_username, fallback_password)) {
    return server.requestAuthentication();
  }
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));
    server.sendHeader("Location", "/spiffs.html");
    server.send(303);
  } else {
    if (enableAuth == 1){
      server.sendHeader("Cache-Control", "private, no-cache");
    } else {
      server.sendHeader("Cache-Control", "max-age=2628000");
    }
    #ifdef CASTWEB
      File f = openWithGzip("/castweb.html"); server.streamFile(f, "text/html"); f.close();
    #else    
      File f = openWithGzip("/nocastweb.html"); server.streamFile(f, "text/html"); f.close();
    #endif     
  }
}


void send404() {
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));
    server.sendHeader("Location", "/spiffs.html");
    server.send(303);
  } else {
    File f = openWithGzip("/404.html");
    server.streamFile(f, "text/html");
    f.close();
  }
}


const char Helper[] PROGMEM = R"(<form method="POST" action="/upload" enctype="multipart/form-data">              //enables uploading a file before spiffs.html is present on SPIFFS
     <input type="file" name="upload"><input type="submit" value="Upload"></form>Lade die spiffs.html hoch.)";



const char* contentType(const String& filename) {
  String f = filename;
  if (f.endsWith(".gz")) f = f.substring(0, f.length() - 3);                      // strip .gz, check underlying type
  if (f.endsWith(".htm") || f.endsWith(".html")) return "text/html";
  if (f.endsWith(".css")) return "text/css";
  if (f.endsWith(".js")) return "application/javascript";
  if (f.endsWith(".json")) return "application/json";
  if (f.endsWith(".png")) return "image/png";
  if (f.endsWith(".gif")) return "image/gif";
  if (f.endsWith(".jpg")) return "image/jpeg";
  if (f.endsWith(".ico")) return "image/x-icon";
  if (f.endsWith(".xml")) return "text/xml";
  if (f.endsWith(".pdf")) return "application/x-pdf";
  if (f.endsWith(".zip")) return "application/x-zip";
  return "text/plain";
}


bool handleFile(const String& path) {
  String localPath = path;
  
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));
    server.sendHeader("Location", "/spiffs.html");
    server.send(303); 
    return true;
  }
  
  if (!fileExists("/spiffs.html")) {
    server.send(200, "text/html", Helper);
  }
  
  if (localPath.endsWith("/")) localPath += "index.html";
  
  if (localPath == "/admin.html" || localPath == "/config.html" || 
      localPath == "/spiffs.html" || localPath == "/api.html" || 
      localPath == "/daniel.html") {
    return fileExists(localPath) ? ({html_authentify(localPath); true;}) : false;
  } else {
    return fileExists(localPath) ? ({
      File f = openWithGzip(localPath); 
      server.sendHeader("Cache-Control", "max-age=2628000"); 
      server.streamFile(f, contentType(localPath)); 
      f.close(); 
      true;
    }) : false;
  }
}


const char* formatBytes(size_t bytes) {
  static char buf[16];
  if (bytes < 1024) snprintf(buf, sizeof(buf), "%d Byte", bytes);
  else if (bytes < 1048576) snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
  else snprintf(buf, sizeof(buf), "%.1f MB", bytes / 1048576.0);
  return buf;
}


void formatSpiffs() {                                                                             // formats SPIFFS Memory
  SPIFFS.format();
  server.sendHeader("Location","/spiffs.html");
  server.send(303); 
}



void handleList() {                                                                               // sends a list of all files on SPIFFS to requesting html page
  
  File root = SPIFFS.open("/");
  
  String temp;
  temp.reserve(1024);
  temp = "[";
  
  File file = root.openNextFile();
  while (file) {
    if (temp.length() > 1) temp += ",";
    
    #ifdef ESP_ARDUINO_VERSION                                         // for ESP32 core Version >=2.0.0
      temp += R"({"name":")";                                         
      temp += file.name();                                             
      temp += R"(","size":")";                                         
      temp += formatBytes(file.size());                               
      temp += R"("})";                                                
    #else
      temp += R"({"name":")";                                         
      temp += String(file.name()).substring(1);                        // (needs String for substring)
      temp += R"(","size":")";                                         
      temp += formatBytes(file.size());                                
      temp += R"("})";                                                
    #endif
    
    file = root.openNextFile();
  }
  
  temp += R"(,{"usedBytes":")";                                        
  temp += formatBytes(SPIFFS.usedBytes() * 1.05);                      
  temp += R"(","totalBytes":")";                                       
  temp += formatBytes(SPIFFS.totalBytes());                            
  temp += R"(","freeBytes":")";                                        
  temp += (SPIFFS.totalBytes() - (SPIFFS.usedBytes() * 1.05));         
  temp += R"("}])";                                                   
  
  server.send(200, "application/json", temp);
}


void handleUpload() {                                                                             // loads file from computer and saves it to SPIFFS
  static File fsUploadFile;                                                                       // keeps current upload
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (upload.filename.length() > 30) {
      upload.filename = upload.filename.substring(upload.filename.length() - 30, upload.filename.length());           // shortens file name to a length of 30 characters
    }
   #ifdef DEBUG 
    Serial.println("FileUpload Name: " + upload.filename);
   #endif 
    fsUploadFile = SPIFFS.open("/" + server.urlDecode(upload.filename), "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
   #ifdef DEBUG  
    Serial.println("FileUpload Data: " + (String)upload.currentSize);
   #endif 
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
   #ifdef DEBUG    
    Serial.println("FileUpload Size: " + (String)upload.totalSize);
   #endif 
    server.sendHeader("Location","/spiffs.html");
    server.send(303); 
  }
}


void handleDeleteMulti() {                                                                        // deletes multiple files from SPIFFS at once
  if (server.hasArg("files")) {
    String fileList = server.arg("files");
    int deleted = 0;
    while (fileList.length() > 0) {
      int commaIdx = fileList.indexOf(',');
      String fileName;
      if (commaIdx == -1) {
        fileName = fileList;
        fileList = "";
      } else {
        fileName = fileList.substring(0, commaIdx);
        fileList = fileList.substring(commaIdx + 1);
      }
      fileName.trim();
      if (fileName.length() > 0) {
        String path = fileName.startsWith("/") ? fileName : "/" + fileName;
        if (SPIFFS.exists(path)) {
          SPIFFS.remove(path);
          deleted++;
          #ifdef DEBUG
            Serial.printf("Deleted: %s\n", path.c_str());
          #endif
        }
      }
    }
    String resp = "{\"deleted\":";
    resp += deleted;
    resp += "}";
    server.send(200, "application/json", resp);
  } else {
    server.send(400, "text/plain", "Missing files parameter");
  }
}


//// FUNCTIONS CALLED BY WEBSERVER ////

void composeNews() {
  String temp;
  // Dynamic formula with ~10% margin:
  // articles: (newssources × numofarticles) * (500 + 200 + 50) -> (newssources * numofarticles * 825)
  // source names: newssources × 110
  // overhead (Name, config vars): 250
  size_t estimatedSize = (newssources * numofarticles * 825) + (newssources * 110) + 250;
  
  temp = "{\"Name\":\"";
  temp += devicename;
  temp += "\",\"newssources\":";   // ✅ Fixed
  temp += newssources;
  temp += ",\"numofarticles\":";
  temp += numofarticles;
  temp += ",";
  
  String source[newssources];
  String postnews;
  postnews.reserve(MSG_SIZE);
  
  uint8_t totalArticles = newssources * numofarticles;
  
  for (uint8_t t = 0; t < totalArticles; t++) {
    String newsline = table[t].newscur;
                                                                                                  // Extract source name from first article of each source group
    uint8_t sourceIndex = t / numofarticles;                                                      // 0,0,0,1,1,1,2,2,2,3,3,3
    if (t % numofarticles == 0) {                                                                 // first article of each source
      source[sourceIndex] = newsline.substring(0, newsline.indexOf(':'));
    }
    
    postnews = newsline.substring(newsline.indexOf(':') + 1);
    postnews.replace("\"", "\\\"");
    
    temp += "\"News";
    temp += t;
    temp += "\":\"";
    temp += postnews;
    temp += "\",\"NewsURL";
    temp += t;
    temp += "\":\"";
    temp += table[t].newslink;
    temp += "\",";
    
    #ifdef DEBUG
      Serial.print("String: News");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (uint8_t s = 0; s < newssources; s++) {                 // add all source names
    temp += "\"S";
    temp += (s + 1);
    temp += "\":\"";
    temp += source[s];
    if (s < newssources - 1) {
      temp += "\",";
    } else {
      temp += "\"";
    }
 }
  
  temp += "}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("JSON size: ");
    Serial.println(temp.length());
  #endif 
}


void showSimple() {                                                                                 // sends ownmessage to requesting html page              
  String temp;
  // Dynamic formula:
  // ownmessage(MSG_SIZE) + ownName(30) + devicename(25) + enableOwn(3) + overhead(50)
  size_t estimatedSize = MSG_SIZE + 30 + 25 + 103;
  temp.reserve(estimatedSize);
  
  temp = "{\"No1\":\"";
  temp += ownmessage;
  temp += "\",\"No2\":\"";
  temp += enableOwn;
  temp += "\",\"No3\":\"";
  temp += ownName;
  temp += "\",\"Name\":\"";
  temp += devicename;
  temp += "\"}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.printf("Name: %s, Ownmessage: %s and Device Name: %s sent successfully\n", ownName, ownmessage, devicename);
  #endif
}
 

void showAdvanced() {
  String temp;
  // Dynamic formula:
  // Messages: numOfMessages × (MSG_SIZE + 110 overhead)
  // Graphics: numOfGraphics × 30
  // Fixed fields & overhead: 400
  size_t estimatedSize = (numOfMessages * (MSG_SIZE + 110)) + (numOfGraphics * 30) + 400;
  temp.reserve(estimatedSize);
  
  
  temp = "{";
  
  for (int t = 0; t < numOfMessages; t++) {
    if (t > 0) temp += ",";
    temp += "\"Enable";     temp += t;
    temp += "\":\"";        temp += enablemessage[t];
    temp += "\",\"In";      temp += t;
    temp += "\":\"";        temp += catalogTEMP[t].effectinTEMP;
    temp += "\",\"Text";    temp += t;
    temp += "\":\"";        temp += messages[t];
    temp += "\",\"Out";     temp += t;
    temp += "\":\"";        temp += catalogTEMP[t].effectoutTEMP;
    temp += "\",\"Align";   temp += t;
    temp += "\":\"";        temp += catalogTEMP[t].justTEMP;
    temp += "\",\"Speed";   temp += t;
    temp += "\":\"";        temp += catalogTEMP[t].speedTEMP;
    temp += "\",\"Pause";   temp += t;
    temp += "\":\"";        temp += catalogTEMP[t].pauseTEMP;
    temp += "\"";
  }
  
  temp += ",\"EnableTime\":\"";       temp += enableTime;
  temp += "\",\"EnableDate\":\"";     temp += enableDate;
  temp += "\",\"EnableWeath\":\"";    temp += enableWeath;
  temp += "\",\"EnableNews1\":\"";    temp += enableNews1;
  temp += "\",\"EnableSpot\":\"";     temp += enableSpotify;
  temp += "\",\"EnableNews2\":\"";    temp += enableNews2;
  temp += "\",\"EnableOwn\":\"";      temp += enableOwn;
  temp += "\",\"Name\":\"";           temp += devicename;
  temp += "\",\"EnableTimeXXL\":\"";  temp += enableTimeXXL;
  temp += "\",\"EnableServiceMSG\":\""; temp += enableServiceMsg;
  temp += "\",\"custServ\":\"";       temp += customServiceMsg;
  temp += "\",\"Preset\":\"";         temp += presetMSG;
  temp += "\"";
                                                                      
  
  for (int m = 0; m < numOfGraphics; m++) {
    temp += ",\"EnableGraph_";  temp += m;
    temp += "\":\"";            temp += enableGraph[m];
    temp += "\"";
  }
  
  temp += "}";
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.println("JSON size: ");
    Serial.println(temp.length());
  #endif
  server.send(200, "application/json", temp);
}


void handleSimple() {
  if (server.hasArg("myText")){
   snprintf(ownName, sizeof(ownName), "%s",server.arg("myName").c_str());
   snprintf(ownmessage, sizeof(ownmessage), "%s",server.arg("myText").c_str());
   #ifdef DEBUG
    Serial.printf("Name: %s and Ownmessage: %s received and processed successfully\n", ownName, ownmessage);
   #endif
   server.send(200, "text/plain", "");
   }
}


void handleAdvanced (){                                                  // processes all custom messages and enabling data received from html page
  if (server.hasArg("MyEnable0")){
    char argName[16];
    for (int t=0; t<numOfMessages; t++){

      snprintf(argName, sizeof(argName), "MyEnable%d", t);
      enablemessage[t] = server.arg(argName).toInt();

      #ifdef DEBUG
        Serial.print("Custom Message ");
        Serial.print(t+1);
        Serial.print(" enabled: ");
        Serial.print(enablemessage[t]);
        Serial.println("  -> 1 == yes, 0 == no");
      #endif

      snprintf(argName, sizeof(argName), "MyText%d", t);
      snprintf(messages[t], sizeof(messages[t]), "%s", server.arg(argName).c_str());

      snprintf(argName, sizeof(argName), "MyIn%d", t);
      catalogTEMP[t].effectinTEMP = server.arg(argName).toInt();

      snprintf(argName, sizeof(argName), "MyOut%d", t);
      catalogTEMP[t].effectoutTEMP = server.arg(argName).toInt();

      snprintf(argName, sizeof(argName), "MyAlign%d", t);
      catalogTEMP[t].justTEMP = server.arg(argName).toInt();

      snprintf(argName, sizeof(argName), "MySpeed%d", t);
      catalogTEMP[t].speedTEMP = server.arg(argName).toInt();

      snprintf(argName, sizeof(argName), "MyPause%d", t);
      catalogTEMP[t].pauseTEMP = server.arg(argName).toInt();

      catalogTEMP[t].delayTEMP = 1;

    #ifdef DEBUG
      Serial.print("String ");
      Serial.print(messages[t]);
      Serial.println(" received successfully");
    #endif    
  }
 
  enableTime = server.arg("MyEnableTime").toInt();
  enableOrDisable(locationTime, timeshow, enableTime, 2);
  if (enableTime==1){
      time_t now = time(NULL);
      localtime_r(&now, &tm);
      strftime (timeSaver, sizeof(timeSaver), "%H:%M", &tm); 
  }

 enableDate = server.arg("MyEnableDate").toInt();
 if (enableTimeXXL==1) enableDate = 0;
 enableOrDisable(locationDate, dateshow, enableDate, 3);
 if (enableDate==1) makeDate();
  
  enableWeath = server.arg("MyEnableWeath").toInt();
  enableOrDisable(locationWeath, showweather, enableWeath, 0);

  enableNews1 = server.arg("MyEnableNews1").toInt();
  enableOrDisable(locationNews[0], shownews, enableNews1, 0);
  
  enableSpotify = server.arg("MyEnableSpot").toInt();
   if (enableSpotify==1){
      #ifdef SPOTIFY
        enableOrDisable(locationSpot,showspotify, enableSpotify,1);
      #else
        enableSpotify=0;
        snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
        snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
        enableOrDisable(locationSpot,showspotify, enableSpotify,1);
      #endif
   } else if (enableSpotify==2 || enableSpotify==3){
      #ifdef CASTWEB
        enableOrDisable(locationSpot,showspotify, 1 ,0);
      #else
        enableSpotify=0;
        snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
        snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
        enableOrDisable(locationSpot,showspotify, enableSpotify,1);
      #endif
      #ifdef SPOTIFY
       expiretime = 1;
      #endif
   } else {
        snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
        snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
        enableOrDisable(locationSpot,showspotify, enableSpotify,1);
      #ifdef SPOTIFY
       expiretime = 1;
      #endif
   }
  
  enableNews2 = server.arg("MyEnableNews2").toInt();
  enableOrDisable(locationNews[1], shownews2, enableNews2, 0);

  enableOwn = server.arg("MyEnableOwn").toInt();
  enableOrDisable(locationOwn, showownmessage, enableOwn, 0);

  for (int m=0; m<numOfGraphics; m++){
      snprintf(argName, sizeof(argName), "MyEnableGraph_%d", m);
      enableGraph[m] = server.arg(argName).toInt();
  }

  snprintf(customServiceMsg, sizeof(customServiceMsg), "%s",server.arg("custServ").c_str());
  
 if (enableWeath==1 && weathEverLoaded == false) getWeatherData();
 
 bool newsActiveCheck = false;
 if (enableNews1 == 1 || enableNews2 == 1) newsActiveCheck = true;
 if (newsActiveCheck == true && newsEverLoaded == false) getNewsData();

 if (enableOwn==0) changeflag = true;
 if (enableOwn==1 && changeflag == true ) {
     if (AP_established == false){
        snprintf(ownmessage, sizeof(ownmessage), "--> %s %d.%d.%d.%d <--", 
                 msgSendOwn, WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
     } else {
        snprintf(ownmessage, sizeof(ownmessage), "--> %s %d.%d.%d.%d <--", 
                 msgSendOwn,  WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
     }
     ownName[0] = '\0'; 
 }
 if (enableOwn==1) changeflag = false; 

 allOffCheck();
 msgUpdated = true;
 server.send(200, "text/plain", "");
 }
}



void saveAllMessages (){                                                                          // saves all custom messages and enabling data to file on SPIFFS
  const char* path; 

  if (server.hasArg("PresetSave")){
    presetMSG = server.arg("PresetSave").toInt();
    switch(presetMSG) {
      case 0: path = "/messages.txt"; break;
      case 1: path = "/messages_1.txt"; break;
      case 2: path = "/messages_2.txt"; break;
      case 3: path = "/messages_3.txt"; break;
      case 4: path = "/messages_4.txt"; break;
      case 99: path = "/default.txt"; break;
      case 100: path = "/AP_only.txt"; break;
      case 199: path = "/AP_only_default.txt"; break;
      default: path = "/messages.txt"; break;
    }  
  File f = SPIFFS.open(path, "w"); 
  for (int t=0; t<numOfMessages; t++){                                                            
    f.printf("%i{%i{%s{%i{%i{%i{%i\n", 
              enablemessage[t], catalogTEMP[t].effectinTEMP, messages[t], catalogTEMP[t].effectoutTEMP, 
              catalogTEMP[t].justTEMP, catalogTEMP[t].speedTEMP, catalogTEMP[t].pauseTEMP);
    }
  f.printf("%i\n%i\n%i\n%i\n%i\n%i\n%i\n%s\n", 
            enableTime, enableDate, enableWeath, enableNews1, enableSpotify, enableNews2, enableOwn, customServiceMsg);
  for (uint8_t m=0; m<numOfGraphics; m++) {
    f.printf("%i\n", enableGraph[m]);      
  }
  f.close();
  server.send(200, "text/plain", "");
 #ifdef DEBUG
  Serial.println("Messages saved successfully!");
 #endif
  }
}


void loadAllMessages (uint8_t defmessage) {                         // loads and processes all messages and enabling data from file on SPIFFS
  const char* path;
  switch(defmessage) {
    case 0: path = "/messages.txt"; break;
    case 1: path = "/messages_1.txt"; break;
    case 2: path = "/messages_2.txt"; break;
    case 3: path = "/messages_3.txt"; break;
    case 4: path = "/messages_4.txt"; break;
    case 99: path = "/default.txt"; break;
    case 100: path = "/AP_only.txt"; break;
    case 199: path = "/AP_only_default.txt"; break;
    default: path = "/messages.txt"; break;
  }  
  
  File f = SPIFFS.open(path, "r");
  if (!f) return;
  
  String line;
  line.reserve(MSG_SIZE + 50);
  
  for (int t = 0; t < numOfMessages; t++) {
    line = f.readStringUntil('\n');
    line.trim();
    
    #ifdef DEBUG
      Serial.println("Loaded from file: ");
      Serial.println(line);
      Serial.println("processing String:");
    #endif
   
    int index0;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    index0 = line.indexOf("{");   
    #ifdef DEBUG
      Serial.println(line.substring(0, index0));
    #endif
    
    index1 = line.indexOf("{", index0 + 1);   
    #ifdef DEBUG
      Serial.println(line.substring(index0 + 1, index1));
    #endif
    
    index2 = line.indexOf("{", index1 + 1);
    #ifdef DEBUG
      Serial.println(line.substring(index1 + 1, index2));
    #endif
    
    index3 = line.indexOf("{", index2 + 1);
    #ifdef DEBUG
      Serial.println(line.substring(index2 + 1, index3));
    #endif
    
    index4 = line.indexOf("{", index3 + 1);
    #ifdef DEBUG
      Serial.println(line.substring(index3 + 1, index4));
    #endif
    
    index5 = line.indexOf("{", index4 + 1);
    #ifdef DEBUG
      Serial.println(line.substring(index4 + 1, index5));
    #endif
    
    index6 = line.indexOf("{", index5 + 1);
    #ifdef DEBUG
      Serial.println(line.substring(index5 + 1, index6));
    #endif
        
    enablemessage[t] = line.substring(0, index0).toInt();
    
    String msgText = line.substring(index1 + 1, index2);
    snprintf(messages[t], sizeof(messages[t]), "%s", msgText.c_str());
    catalogTEMP[t].effectinTEMP = line.substring(index0 + 1, index1).toInt();
    catalogTEMP[t].effectoutTEMP = line.substring(index2 + 1, index3).toInt();
    catalogTEMP[t].justTEMP = line.substring(index3 + 1, index4).toInt();
    catalogTEMP[t].speedTEMP = line.substring(index4 + 1, index5).toInt();
    catalogTEMP[t].pauseTEMP = line.substring(index5 + 1, index6).toInt();
    catalogTEMP[t].delayTEMP = 1;
  }
                                                                      // reuse 'line' for remaining reads instead of creating new Strings
  line = f.readStringUntil('\n');
  line.trim();
  enableTime = line.toInt();
  enableOrDisable(locationTime, timeshow, enableTime, 2);
  if (enableTime == 1) {
    time_t now = time(NULL);
    localtime_r(&now, &tm);
    strftime(timeSaver, sizeof(timeSaver), "%H:%M", &tm);
  }
  line = f.readStringUntil('\n');
  line.trim();
  enableDate = line.toInt();
  if (enableTimeXXL == 1) enableDate = 0;
  enableOrDisable(locationDate, dateshow, enableDate, 3);
  if (enableDate == 1) makeDate();
  line = f.readStringUntil('\n');
  line.trim();
  enableWeath = line.toInt();
  enableOrDisable(locationWeath, showweather, enableWeath, 0);
  line = f.readStringUntil('\n');
  line.trim();
  enableNews1 = line.toInt();
  enableOrDisable(locationNews[0], shownews, enableNews1, 0);
    
  line = f.readStringUntil('\n');
  line.trim();
  enableSpotify = line.toInt();
  if (enableSpotify == 1) {
    #ifdef SPOTIFY
      enableOrDisable(locationSpot, showspotify, enableSpotify, 1);
    #else
      enableSpotify = 0;
      snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
      snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
      enableOrDisable(locationSpot, showspotify, enableSpotify, 1);
    #endif
  } else if (enableSpotify == 2 || enableSpotify == 3) {
    #ifdef CASTWEB
      enableOrDisable(locationSpot, showspotify, 1, 0);
    #else
      enableSpotify = 0;
      snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
      snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
      enableOrDisable(locationSpot, showspotify, enableSpotify, 1);
    #endif
    #ifdef SPOTIFY
      expiretime = 1;
    #endif
  } else {
    snprintf(spotify, sizeof(spotify), msgSpotifyDeact);
    snprintf(spotifycover, sizeof(spotifycover), "/noSpot.jpg");
    enableOrDisable(locationSpot, showspotify, enableSpotify, 1);
    #ifdef SPOTIFY
      expiretime = 1;
    #endif
  }
  line = f.readStringUntil('\n');
  line.trim();
  enableNews2 = line.toInt();
  enableOrDisable(locationNews[1], shownews2, enableNews2, 0);
  line = f.readStringUntil('\n');
  line.trim();
  enableOwn = line.toInt();
  enableOrDisable(locationOwn, showownmessage, enableOwn, 0);
  line = f.readStringUntil('\n');
  line.trim();
  snprintf(customServiceMsg, sizeof(customServiceMsg), "%s", line.c_str());
  for (uint8_t m = 0; m < numOfGraphics; m++) {
    line = f.readStringUntil('\n');
    line.trim();
    enableGraph[m] = line.toInt();
  }
  
  if (enableWeath == 1 && weathEverLoaded == false) getWeatherData();
  
  bool newsActiveCheck = (enableNews1 == 1 || enableNews2 == 1);
  if (newsActiveCheck && newsEverLoaded == false) getNewsData();
  if (enableOwn == 0) changeflag = true;
  if (enableOwn == 1 && changeflag == true) {
    if (AP_established == false) {
      snprintf(ownmessage, sizeof(ownmessage), "--> %s %d.%d.%d.%d <--", 
               msgSendOwn, WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    } else {
      snprintf(ownmessage, sizeof(ownmessage), "--> %s %d.%d.%d.%d <--", 
               msgSendOwn, WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
    }
    ownName[0] = '\0';
  }
  if (enableOwn == 1) changeflag = false;
  allOffCheck();
  msgUpdated = true;
  f.close();
}


void showConfig() {                                                                      // sends all configuration data to requesting html page
  int minutes = millis()/60000;
  
  uint8_t temp_intens;
  if (auto_intens == true){
    temp_intens = 99;
  } else {
    temp_intens = intens;
  }
  
  String temp;
  // Safe formula:
  // SSID(32) + IP(15) + ntpServer(40) + devicename(25) + username(20) + password(20) + overhead(200)
  // = 32 + 15 + 40 + 25 + 20 + 20 + 200 + 60 = 412 → rounds to 512 ✅
  temp.reserve(512);  // Pre-allocate based on expected size
  
  temp = "{\"Intens\":\"";         
  temp += temp_intens;
  temp += "\",\"Minutes\":\"";     
  temp += minutes;
  temp += "\",\"SSID\":\"";
  if(AP_established == false){
    temp += WiFi.SSID();
  } else {
    temp += "AP ";
    temp += AP_NAME;
  }
  temp += "\",\"IP\":\"";
  if(AP_established == false){
    temp += WiFi.localIP().toString();
  } else {
    temp += WiFi.softAPIP().toString();
  }
  temp += "\",\"EnableXL\":\"";    
  temp += enableNewsXL;
  temp += "\",\"Speed\":\"";       
  temp += globScrollSpeed;
  temp += "\",\"NTP\":\"";         
  temp += ntpServer;
  temp += "\",\"EnableService\":\""; 
  temp += enableServiceMsg;
  temp += "\",\"Name\":\""; 
  temp += devicename;
  temp += "\",\"EnableTimeXXL\":\""; 
  temp += enableTimeXXL;
  temp += "\",\"TimeDuration\":\""; 
  temp += timeDuration;
  temp += "\",\"NewsApiType\":\""; 
  temp += newsApiChoice;
  temp += "\",\"WeathApiType\":\""; 
  temp += weathApiChoice;
  temp += "\",\"EnableAuth\":\"";  
  temp += enableAuth;
  temp += "\",\"USER\":\"";        
  temp += www_username;
  temp += "\",\"PW\":\"";          
  temp += www_password;
  temp += "\",\"N_Refresh\":\""; 
  temp += newsRefreshInterval;
  temp += "\",\"W_Refresh\":\""; 
  temp += weatherRefreshInterval;
  temp += "\",\"T_Refresh\":\""; 
  temp += timeRefreshInterval;
  temp += "\"}";
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.printf("Current Configuration: \nIntensity: %i, Global Scroll Speed (internal value): %i, Enable NewsXL: %i, \n"
                  "NTP Server: %s, EnableService: %i \nDevice Name: %s, EnableTimeXXL: %i, Time Duration: %i, News Api Type: %i\n "
                  "EnableAuth: %i, User: %s, Password: %s \nIntervals: News:%i, Weather:%i, Time:%i ", 
                  temp_intens, globScrollSpeed, enableNewsXL, ntpServer, enableServiceMsg, devicename, enableTimeXXL, 
                  timeDuration, newsApiChoice, enableAuth, www_username, www_password, newsRefreshInterval, weatherRefreshInterval, 
                  timeRefreshInterval);
  #endif
  
  server.send(200, "application/json", temp);
}


void handleConfig(){
   if (server.hasArg("newSpeed")){
   globScrollSpeed = server.arg("newSpeed").toInt();
   setGlobalScrollSpeed();
   #ifdef DEBUG
    Serial.printf("Global Scroll Speed: \"%i\" received and processed successfully\n", globScrollSpeed);
   #endif
   server.send(200, "text/plain", "");
   }
  if (server.hasArg("newIntens")){
   uint8_t temp_intens = server.arg("newIntens").toInt();
   if (temp_intens == 99){
      auto_intens = true;
   } else {
      auto_intens = false;
      intens = temp_intens;
      P.setIntensity(intens);
   }
   #ifdef DEBUG
    Serial.printf("Intens: \"%i\" received and processed successfully\n", temp_intens);
   #endif
   server.send(200, "text/plain", "");
   }
   if (server.hasArg("ApiTypeW")){
    uint8_t tempweathApiChoice = server.arg("ApiTypeW").toInt();
    if (tempweathApiChoice != weathApiChoice) {
      weathApiChoice = tempweathApiChoice;
      loadAPI();
    }
    #ifdef DEBUG
      Serial.printf("Weather Api Choice: \"%i\" received and processed successfully\n", weathApiChoice);
    #endif
    server.send(200, "text/plain", "");
    refreshWeather();
   }
   if (server.hasArg("MyEnableXL")){
    enableNewsXL = server.arg("MyEnableXL").toInt();
    uint8_t tempnewsApiChoice = server.arg("ApiType").toInt();
    if (tempnewsApiChoice != newsApiChoice) {
      newsApiChoice = tempnewsApiChoice;
      loadAPI();
    }
    #ifdef DEBUG
      Serial.printf("Enable News XL: \"%i\" received and processed successfully\n", enableNewsXL);
    #endif
    server.send(200, "text/plain", "");
    refreshNews();
   }
    if (server.hasArg("MyEnableService")){
    enableServiceMsg = server.arg("MyEnableService").toInt();
    #ifdef DEBUG
      Serial.printf("Enable Service Message: \"%i\" received and processed successfully\n", enableServiceMsg);
    #endif
    server.send(200, "text/plain", "");
   }
   if (server.hasArg("MyEnableTimeXXL")){
    enableTimeXXL = server.arg("MyEnableTimeXXL").toInt();
    if (enableTimeXXL==1) enableDate = 0; 
    enableOrDisable(locationDate, dateshow, enableDate, 3);
    timeDuration = server.arg("MyEnableTimeDur").toInt();
    enableOrDisable(locationTime, timeshow, enableTime, 2);
    #ifdef DEBUG
      Serial.printf("Enable Time XXL: \"%i\" and Time Duration: \"%i\" received and processed successfully\n", enableServiceMsg, timeDuration);
    #endif
    server.send(200, "text/plain", "");
   }
   if (server.hasArg("NTP")){
   snprintf(ntpServer, sizeof(ntpServer), "%s", server.arg("NTP").c_str());
   server.send(200, "text/plain", "");
  #ifdef DEBUG
      Serial.printf("New NTP Server: \"%s\" received and processed successfully\n", ntpServer);
  #endif
   getTimeFromServer();
  }
  if (server.hasArg("DeviceName")){
   snprintf(devicename, sizeof(devicename), "%s", server.arg("DeviceName").c_str());
   server.send(200, "text/plain", "");
  #ifdef DEBUG
      Serial.printf("New Device Name: \"%s\" received and processed successfully\n", devicename);
  #endif
  }
   if (server.hasArg("MyEnableAuth")){ 
    enableAuth = server.arg("MyEnableAuth").toInt();
    snprintf(www_username, sizeof(www_username), "%s", server.arg("USER").c_str());
    snprintf(www_password, sizeof(www_password), "%s", server.arg("PW").c_str()); 
    #ifdef DEBUG
       Serial.printf("New User Name:\"%s\", New Password:\"%s%\", Enable Auth:%i received and processed successfully\n", www_username, www_password, enableAuth); 
    #endif
    server.send(200, "text/plain", "");
   }
   if (server.hasArg("MyEnableInt_N")){ 
    newsRefreshInterval = server.arg("MyEnableInt_N").toInt();
    weatherRefreshInterval = server.arg("MyEnableInt_W").toInt();
    timeRefreshInterval = server.arg("MyEnableInt_T").toInt();
    #ifdef DEBUG
      Serial.printf("Refreh Interval: News:%i, Weather:%i, Time:%i processed successfully\n", newsRefreshInterval, weatherRefreshInterval, timeRefreshInterval);
    #endif
    server.send(200, "text/plain", "");
   }
   saveConfig ();
}


void saveConfig (){                                                                   // saves all configuration data to file on SPIFFS
  uint8_t temp_intens;                                                               
  if (auto_intens == true){
    temp_intens = 99;
  } else {
    temp_intens = intens;
  }
  File f = SPIFFS.open("/config.txt", "w");
  f.printf("%i\n%i\n%i\n%s\n%i\n%s\n%i\n%i\n%i\n%i\n%s\n%s\n%i\n%i\n%i\n%i\n", 
            temp_intens, enableNewsXL, globScrollSpeed, ntpServer, enableServiceMsg, devicename, enableTimeXXL, 
            timeDuration, newsApiChoice, enableAuth, www_username, www_password, newsRefreshInterval, weatherRefreshInterval, 
            timeRefreshInterval, weathApiChoice);
  f.close();
  server.send(200, "text/plain", "");
  #ifdef DEBUG
    Serial.printf("Config saved to file: \nIntensity: %i, Global Scroll Speed (internal value): %i, Enable NewsXL: %i, \n"
                  "NTP Server: %s, EnableService: %i \nDevice Name: %s, EnableTimeXXL: %i, Time Duration: %i, News Api Type: %i\n "
                  "EnableAuth: %i, User: %s, Password: %s \nIntervals: News:%i, Weather:%i, Time:%i, Weather Api Type:%i \n", 
                  temp_intens, globScrollSpeed, enableNewsXL, ntpServer, enableServiceMsg, devicename, enableTimeXXL, 
                  timeDuration, newsApiChoice, enableAuth, www_username, www_password, newsRefreshInterval, weatherRefreshInterval, 
                  timeRefreshInterval, weathApiChoice);
  #endif
}


void loadConfig () {                                                               // loads all configuration data from file on SPIFFS
    File f = SPIFFS.open("/config.txt", "r");
    if (!f) return;
    
    String line;
    line.reserve(64);
    
    line = f.readStringUntil('\n');
    line.trim();
    uint8_t temp_intens = line.toInt();          
    if (temp_intens == 99){
      P.setIntensity(1);
      auto_intens = true;
    } else {
      auto_intens = false;
      intens = temp_intens;
      P.setIntensity(intens);
    }
    
    line = f.readStringUntil('\n');
    line.trim();
    enableNewsXL = line.toInt();
    
    line = f.readStringUntil('\n');
    line.trim();
    globScrollSpeed = line.toInt();
    setGlobalScrollSpeed();
    
    line = f.readStringUntil('\n');
    line.trim();
    if (line != "") snprintf(ntpServer, sizeof(ntpServer), "%s", line.c_str());
    
    line = f.readStringUntil('\n');
    line.trim();
    enableServiceMsg = line.toInt();
    
    line = f.readStringUntil('\n');
    line.trim();
    if (line != "") snprintf(devicename, sizeof(devicename), "%s", line.c_str());
    
    line = f.readStringUntil('\n');
    line.trim();
    enableTimeXXL = line.toInt();
    if (enableTimeXXL == 1) enableDate = 0;
    enableOrDisable(locationDate, dateshow, enableDate, 3);
    
    line = f.readStringUntil('\n');
    line.trim();
    timeDuration = line.toInt();
    
    line = f.readStringUntil('\n');
    line.trim();
    newsApiChoice = line.toInt();
    
    line = f.readStringUntil('\n');
    line.trim();
    enableAuth = line.toInt();
    
    line = f.readStringUntil('\n');
    line.trim();
    snprintf(www_username, sizeof(www_username), "%s", line.c_str());
    
    line = f.readStringUntil('\n');
    line.trim();
    snprintf(www_password, sizeof(www_password), "%s", line.c_str());

    line = f.readStringUntil('\n');
    line.trim();
    newsRefreshInterval = line.toInt();
    if (newsRefreshInterval < 5) newsRefreshInterval = 5;

    line = f.readStringUntil('\n');
    line.trim();
    weatherRefreshInterval = line.toInt();
    if (weatherRefreshInterval < 5) weatherRefreshInterval = 5;

    line = f.readStringUntil('\n');
    line.trim();
    timeRefreshInterval = line.toInt();
    if (timeRefreshInterval < 5) timeRefreshInterval = 5;

    line = f.readStringUntil('\n');
    line.trim();
    weathApiChoice = line.toInt();
    
    #ifdef DEBUG
      Serial.printf("Configuration loaded from file: \nIntensity: %i, Global Scroll Speed (internal value): %i, "
                    "Enable NewsXL: %i, \nNTP Server: %s, EnableService: %i \nDevice Name: %s, EnableTimeXXL: %i, Time Duration: %i, "
                    "News Api Type: %i\n EnableAuth: %i, User: %s, Password: %s \nIntervals: News:%i, Weather:%i, Time:%i, Weather Api Type:%i\n ", 
                    temp_intens, globScrollSpeed, enableNewsXL, ntpServer, enableServiceMsg, devicename, enableTimeXXL, timeDuration, newsApiChoice, 
                    enableAuth, www_username, www_password, newsRefreshInterval, weatherRefreshInterval, timeRefreshInterval, weathApiChoice);
    #endif
    
    f.close();
}


void showDeviceInfo() {                                                                      // sends all configuration data to requesting html page
  String temp;
  // Safe formula:
  // RSSI(4) + Heap(7) + Alloc(7) + CPU(3) + overhead(50)
  // = 4 + 7 + 7 + 3 + 50 = 71 → rounds to 128 ✅
  temp.reserve(128);  // Pre-allocate based on expected size
  
  temp = "{\"RSSI\":\"";         
  temp += WiFi.RSSI();
  temp += "\",\"Heap\":\"";     
  temp +=  ESP.getFreeHeap();
  temp += "\",\"Alloc\":\"";
  temp += ESP.getMaxAllocHeap();
  temp += "\",\"CPU\":\""; 
  temp += getCpuFrequencyMhz();
  temp += "\"}";
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.printf("Device Info: \nRSSI: %d | Heap: %u | Alloc: %u | CPU: %u MHz\n", WiFi.RSSI(), ESP.getFreeHeap(), ESP.getMaxAllocHeap(), getCpuFrequencyMhz());
  #endif
  
  server.send(200, "application/json", temp);
}


void showDeviceName() {
  String temp;
  // Safe formula:
  // {"Name":"<devicename>"} = 10 + 25 + 2 = 37 → rounds to 64 ✅
  temp.reserve(64);
  
  temp = "{\"Name\":\"";
  temp += devicename;
  temp += "\"}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.printf("Device Name \"%s\" sent successfully\n", devicename);    
  #endif
}


void showCover() {                                 // sends Spotify information and cover url to requesting html page
  #ifdef CASTWEB
    if (enableSpotify == 2) parseCastWeb();
  #endif
  
  String temp;
  // Safe formula:
  // spotify(500) + spotifycover(200) + source(50) + devicename(25) + overhead(100)
  // = 500 + 200 + 50 + 25 + 100 = 875 → rounds to 1024 ✅
  temp.reserve(1024);
  
  temp = "{\"Info\":\"";
                                                  // quotes replaced inline
  for (size_t i = 0; spotify[i] != '\0'; i++) {
    if (spotify[i] == '"') {
      temp += '\'';
    } else {
      temp += spotify[i];
    }
  }
  
  temp += "\",\"Url\":\"";
  temp += spotifycover;
  
  temp += "\",\"Source\":\"";
  switch(enableSpotify){ 
    case 0: temp += FPSTR(msgSpotifyDeact); break;
    case 1: temp += FPSTR(msgSpotifyOK); break;
    #ifdef CASTWEB  
      case 2: temp += FPSTR(msgCastWebOK); temp += " "; temp += musicSource; break;
      case 3: temp += FPSTR(msgCastWebOK); break;
    #endif
  }
  
  temp += "\",\"Name\":\"";
  temp += devicename;
  temp += "\"}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.println(temp);
    Serial.println(" sent successfully");
  #endif 
}


String sketchName() {                                               // shortens file name for sendSketchName()
  char file[sizeof(__FILE__)] = __FILE__;
  char * pch;
  char * svr;
  pch = strtok (file,"\\");
  while (pch != NULL){ 
    svr = pch;
    pch = strtok (NULL, "\\");
  }
  return svr;
}  


void sendSketchName(){
  String temp;
  // Safe formula:
  // sketchName(~50) + devicename(25) + overhead(50)
  // = 50 + 25 + 50 = 125 → rounds to 256 ✅
  temp.reserve(256);
  
  temp = "{\"Name\":\"";
  temp += sketchName();
  temp += "\",\"DeviceName\":\"";
  temp += devicename;
  temp += "\"}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print("Current sketch name: ");
    Serial.print(sketchName());
    Serial.println(" sent successfully");
  #endif
}


void setGlobalScrollSpeed(){                                                          // sets speed for all scrolling messages (date, weather, news, ownmessage, spotify)
catalog[locationDate].speed = globScrollSpeed;
catalog[locationWeath].speed = globScrollSpeed;                                                              
catalog[locationNews[0]].speed = globScrollSpeed;                 
catalog[locationNews[1]].speed = globScrollSpeed;                            
catalog[locationSpot].speed = globScrollSpeed;                           
catalog[locationOwn].speed = globScrollSpeed;
}                      


void resetDevice(){               // restarts device
  P.setFont(tobFont);             // Parola standard font set, necessary if MAIN DISPLAY ROUTINE is at position locationTime
  P.print("restarting...");
  delay(1000);
  ESP.restart();
}


void refreshNews(){               // refreshes news data
  P.displayClear(); 
  P.setFont(tobFont);
  P.setTextAlignment(PA_CENTER);
  P.print("getting news");
  delay(500);
  getNewsData();
  P.displayClear();               // clears the display — turns off all LEDs (all pixels go dark).
  P.displayReset();               // resets the animation state machine back to the beginning.
  msgUpdated = true;              // required for singleMsgOnly mode
}


void refreshWeather(){            // refreshes weather data
  P.displayClear(); 
  P.setFont(tobFont);
  P.setTextAlignment(PA_CENTER);
  P.print("getting weath");
  delay(500);
  getWeatherData();
  P.displayClear(); 
  P.displayReset();
  msgUpdated = true;              // required for singleMsgOnly mode 
}


void clearCredentials() {         // erases WiFi credentials persistently saved in flash memory and restarts device -> WifiManager captive portal is shown on follwing start up
  #ifdef DEBUG
   Serial.println("erasing WiFi credentials");
  #endif
  static_Mode_enabled = 0;
  File f = SPIFFS.open("/IP_mode.txt", "w");      // resets IP config file on SPIFFS to default values
  f.printf("%i\n%i\n%i\n%i\n%i\n", 
           static_Mode_enabled, uint32_t(WiFi.localIP()), uint32_t(WiFi.gatewayIP()), uint32_t(WiFi.subnetMask()), uint32_t(WiFi.dnsIP()));
  f.close();
   WiFi.disconnect(true,true);
  #ifdef DEBUG
   Serial.println("restarting");
  #endif
  P.print("erasing WiFi...");
  delay(5000);
  ESP.restart();
}


void loadAll() {                                                                         // load SAVED custom messages and enabling data from file on SPIFFS
  if (server.hasArg("PresetLoad")){
    presetMSG = server.arg("PresetLoad").toInt();
    loadAllMessages(presetMSG); 
  }
  server.send(200, "text/plain", "");
}



/// API ADMINISTRATION AND LOGS //// 

void saveAPI(){                                                            
 File f = SPIFFS.open("/api_news_0.txt" , "w");                                                       
 for (uint8_t m=0; m<newssources; m++){  
    f.printf("%s\n",ownNewsSources[m]);                                                                
 }
  f.close();

 File f2 = SPIFFS.open("/api_news_1.txt" , "w");                                                       
 for (uint8_t m=0; m<newssources; m++){  
    f2.printf("%s\n",ownNewsSources_A[m]);                                                                
 }
  f2.close();
 
 File w = SPIFFS.open("/api_weath_0.txt", "w");
  for (uint8_t t=0; t<numberofcities; t++){
    w.printf("%s\n%s\n%s\n",cityName_A[t], Lat_A[t], Lon_A[t]);                                                                          
  }
  w.close();  
  
File w2 = SPIFFS.open("/api_weath_1.txt", "w");
  for (uint8_t t=0; t<numberofcities; t++){
    w2.printf("%s\n%s\n%s\n",cityName_B[t], Lat_B[t], Lon_B[t]);                                                                          
  }
  w2.close();
  
  server.send(200, "text/plain", "");
  #ifdef DEBUG
    Serial.println("API Info saved to file.");
  #endif
}


void loadAPI(){

  File f = SPIFFS.open("/api_news_0.txt", "r");
  if (!f) return;  // Safety check
  
  String line;
  line.reserve(64);
  
  for (uint8_t m = 0; m < newssources; m++) {
    line = f.readStringUntil('\n');
    line.trim();
    #ifdef DEBUG
      Serial.println("News API 1 loaded from file: ");
    #endif
    snprintf(ownNewsSources[m], sizeof(ownNewsSources[m]), "%s", line.c_str());
  }
  f.close();

  
  File f2 = SPIFFS.open("/api_news_1.txt", "r");
  if (!f2) return;  // Safety check
  
  for (uint8_t m = 0; m < newssources; m++) {
    line = f2.readStringUntil('\n');
    line.trim();
    #ifdef DEBUG
      Serial.println("News API 2 loaded from file: ");
    #endif
    snprintf(ownNewsSources_A[m], sizeof(ownNewsSources_A[m]), "%s", line.c_str());
  }
  f2.close();
  
 File w = SPIFFS.open("/api_weath_0.txt", "r");
  if (!w) return;
  
  for (uint8_t t = 0; t < numberofcities; t++) {
    line = w.readStringUntil('\n');
    line.trim();
    snprintf(cityName_A[t], sizeof(cityName_A[t]), "%s", line.c_str());
    line = w.readStringUntil('\n');
    line.trim();
    snprintf(Lat_A[t], sizeof(Lat_A[t]), "%s", line.c_str());
    line = w.readStringUntil('\n');
    line.trim();
    snprintf(Lon_A[t], sizeof(Lon_A[t]), "%s", line.c_str());
    #ifdef DEBUG
      Serial.println("Weather API 1 loaded from file: ");
    #endif
  }
  w.close();

  File w2 = SPIFFS.open("/api_weath_1.txt", "r");
  if (!w2) return;
  
  for (uint8_t t = 0; t < numberofcities; t++) {
    line = w2.readStringUntil('\n');
    line.trim();
    snprintf(cityName_B[t], sizeof(cityName_B[t]), "%s", line.c_str());
    line = w2.readStringUntil('\n');
    line.trim();
    snprintf(Lat_B[t], sizeof(Lat_B[t]), "%s", line.c_str());
    line = w2.readStringUntil('\n');
    line.trim();
    snprintf(Lon_B[t], sizeof(Lon_B[t]), "%s", line.c_str());
    #ifdef DEBUG
      Serial.println("Weather API 2 loaded from file: ");
    #endif
  }
  w2.close();
}


void showAPI(){
  String temp;
  // Dynamic formula:
  // fixed_fields (Name, NumOfLogs, etc.): ~150
  // cities: numberofcities × 30 -> numberofcities × 35 
  // newssources: newssources × 70 -> newssources × 80
  // overhead: 50 
  size_t estimatedSize = 150 + (numberofcities * 35) + (newssources * 80) + 50;
  temp.reserve(estimatedSize);
  
  temp = "{\"Name\":\"";
  temp += devicename;
  
  temp += "\",\"NumOfLogs\":\"";
  temp += numOfLogs;
  
  temp += "\",\"NumOfCities\":\"";
  temp += numberofcities;
  
  temp += "\",\"NewsSources\":\"";
  temp += newssources;
  
  temp += "\",\"NumOfMusicLogs\":\"";
  temp += numOfMusicLogs;

  temp += "\",\"NumOfTimeLogs\":\"";
  temp += numOfTimeLogs;

  temp += "\",\"NewsApiType\":\""; 
  temp += newsApiChoice;

  temp += "\",\"WeathApiType\":\""; 
  temp += weathApiChoice;
  temp += "\",";
  
  for (int t = 0; t < numberofcities; t++) {
    temp += "\"City_A";
    temp += t;
    temp += "\":\"";
    temp += cityName_A[t];
    temp += "\",";
    temp += "\"Lat_A";
    temp += t;
    temp += "\":\"";
    temp += Lat_A[t];
    temp += "\",";
    temp += "\"Lon_A";
    temp += t;
    temp += "\":\"";
    temp += Lon_A[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: City");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }

  for (int t = 0; t < numberofcities; t++) {
    temp += "\"City_B";
    temp += t;
    temp += "\":\"";
    temp += cityName_B[t];
    temp += "\",";
    temp += "\"Lat_B";
    temp += t;
    temp += "\":\"";
    temp += Lat_B[t];
    temp += "\",";
    temp += "\"Lon_B";
    temp += t;
    temp += "\":\"";
    temp += Lon_B[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: City");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (int m = 0; m < newssources; m++) {
    temp += "\"Source";
    temp += m;
    temp += "\":\"";
    temp += ownNewsSources[m];
    temp += "\"";
    temp += ",";
    #ifdef DEBUG
      Serial.print("String: Source");
      Serial.print(m);
      Serial.println(" composed successfully");
    #endif
  }

  for (int m = 0; m < newssources; m++) {                                 
    temp += "\"Source";
    temp += m+newssources;
    temp += "\":\"";
    temp += ownNewsSources_A[m];
    temp += "\"";
    if (m < newssources - 1) temp += ",";
    #ifdef DEBUG
      Serial.print("String: Source");
      Serial.print(m);
      Serial.println(" composed successfully");
    #endif
  }
  
  temp += "}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print(temp);
    Serial.println(" sent successfully");
  #endif
}



void handleAPI(){
  char argName[16];                                 // approach with fixed buffer - avoids String heap allocation (not required, just a variation
  if (server.hasArg("City_A0")){
    
    for (int t=0; t<numberofcities; t++){                                       
      snprintf(argName, sizeof(argName), "City_A%d", t);
      snprintf(cityName_A[t], sizeof(cityName_A[t]), "%s", server.arg(argName).c_str());
      snprintf(argName, sizeof(argName), "City_B%d", t);
      snprintf(cityName_B[t], sizeof(cityName_B[t]), "%s", server.arg(argName).c_str());
      snprintf(argName, sizeof(argName), "Lat_A%d", t);
      snprintf(Lat_A[t], sizeof(Lat_A[t]), "%s", server.arg(argName).c_str());
      snprintf(argName, sizeof(argName), "Lat_B%d", t);
      snprintf(Lat_B[t], sizeof(Lat_B[t]), "%s", server.arg(argName).c_str());
      snprintf(argName, sizeof(argName), "Lon_A%d", t);
      snprintf(Lon_A[t], sizeof(Lon_A[t]), "%s", server.arg(argName).c_str());
      snprintf(argName, sizeof(argName), "Lon_B%d", t);
      snprintf(Lon_B[t], sizeof(Lon_B[t]), "%s", server.arg(argName).c_str());
      #ifdef DEBUG
        Serial.printf("City_A%d: %s\n", t+1, cityName_A[t]);
        Serial.printf("Lat_A%d: %s\n", t+1, Lat_A[t]);
        Serial.printf("Lon_A%d: %s\n", t+1, Lon_A[t]);
        Serial.printf("City_B%d: %s\n", t+1, cityName_B[t]);
        Serial.printf("Lat_B%d: %s\n", t+1, Lat_B[t]);
        Serial.printf("Lon_B%d: %s\n", t+1, Lon_B[t]);
      #endif
    }
    refreshWeather();
    server.send(200, "text/plain", "");
  }
  
  if (server.hasArg("Source0")){                          // block 1: newsapi.org sources
    for (int t=0; t<newssources; t++){                                       
      snprintf(argName, sizeof(argName), "Source%d", t);
      snprintf(ownNewsSources[t], sizeof(ownNewsSources[t]), "%s", server.arg(argName).c_str());
      #ifdef DEBUG
        Serial.printf("Source%d (API 1): %s\n", t+1, ownNewsSources[t]);
      #endif
    }
    for (int t=0; t<newssources; t++){                    // block 2: thenewsapi.com sources                            
      snprintf(argName, sizeof(argName), "Source%d", t + newssources);
      snprintf(ownNewsSources_A[t], sizeof(ownNewsSources_A[t]), "%s", server.arg(argName).c_str());
      #ifdef DEBUG
        Serial.printf("Source%d (API 2): %s\n", t+1, ownNewsSources_A[t]);
      #endif
    }
    refreshNews();
    server.send(200, "text/plain", "");
  }
}


void showLogNews(){
  String temp;
  // Dynamic formula based on numOfLogs and newssources:
  // stamps: numOfLogs × (21 + 20) = numOfLogs × 41 -> 45
  // sources: (numOfLogs × newssources) × (51 + 20) = numOfLogs × newssources × 71
  // html: (numOfLogs × newssources) × (4 + 15) = numOfLogs × newssources × 19
  // parse: (numOfLogs × newssources) × (3 + 15) = numOfLogs × newssources × 18   -> 71+19+18=108 -> 115
  // overhead: 150
  size_t estimatedSize = (numOfLogs * 45) + (numOfLogs * newssources * 115) + 150;  // with extra safety
  temp.reserve(estimatedSize);
  
  temp = "{";
  
  for (int t = 0; t < numOfLogs; t++) {
    temp += "\"Stamp";
    temp += t;
    temp += "\":\"";
    temp += logNewsStamp[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Stamp");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }

  for (int t = 0; t < numOfLogs; t++) {
    temp += "\"Type";
    temp += t;
    temp += "\":\"";
    temp += logNewsType[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Type");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (int t = 0; t < numOfLogs * newssources; t++) {
    temp += "\"Sources";
    temp += t;
    temp += "\":\"";
    temp += logNewsSources[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Sources");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }

  
  for (int t = 0; t < numOfLogs * newssources; t++) {
    temp += "\"Html";
    temp += t;
    temp += "\":\"";
    temp += logNewsHtml[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Html");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  int totalParse = numOfLogs * newssources;
  for (int m = 0; m < totalParse; m++) {
    temp += "\"Parse";
    temp += m;
    temp += "\":\"";
    temp += logNewsParse[m];
    temp += "\"";
    if (m < totalParse - 1) temp += ",";
    #ifdef DEBUG
      Serial.print("String: Parse");
      Serial.print(m);
      Serial.println(" composed successfully");
    #endif
  }
  
  temp += "}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print(temp);
    Serial.println(" sent successfully");
  #endif
}



void showLogWeath(){
  String temp;
  // Dynamic formula:
  // stamps: numOfLogs × (21+20) -> numOfLogs × (21+20)*45
  // sources: (numOfLogs × numberofcities × 2) × (51 + 20) -> (numOfLogs × numberofcities × 2) × 75
  // html: (numOfLogs × numberofcities × 2) × (4 + 15) -> html: (numOfLogs × numberofcities × 2) × 20
  // parse: (numOfLogs × numberofcities × 2) × (3 + 15) -> (numOfLogs × numberofcities × 2) × 20
  // overhead: 150
  int totalItems = numOfLogs * numberofcities * 2;
  size_t estimatedSize = (numOfLogs * 45) + (totalItems * 115) + 150;
  temp.reserve(estimatedSize);
  
  temp = "{";
  
  for (int t = 0; t < numOfLogs; t++) {
    temp += "\"Stamp";
    temp += t;
    temp += "\":\"";
    temp += logWeathStamp[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Stamp");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }

  for (int t = 0; t < numOfLogs; t++) {
    temp += "\"Type";
    temp += t;
    temp += "\":\"";
    temp += logWeathType[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Type");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (int t = 0; t < totalItems; t++) {
    temp += "\"Sources";
    temp += t;
    temp += "\":\"";
    temp += logWeathSources[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Sources");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (int t = 0; t < totalItems; t++) {
    temp += "\"Html";
    temp += t;
    temp += "\":\"";
    temp += logWeathHtml[t];
    temp += "\",";
    #ifdef DEBUG
      Serial.print("String: Html");
      Serial.print(t);
      Serial.println(" composed successfully");
    #endif
  }
  
  for (int m = 0; m < totalItems; m++) {
    temp += "\"Parse";
    temp += m;
    temp += "\":\"";
    temp += logWeathParse[m];
    temp += "\"";
    if (m < totalItems - 1) temp += ",";
    #ifdef DEBUG
      Serial.print("String: Parse");
      Serial.print(m);
      Serial.println(" composed successfully");
    #endif
  }
  
  temp += "}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print(temp);
    Serial.println(" sent successfully");
  #endif
}


void showLogMusic() {                                                                  // sends log of time server requests
  String temp;
  // Dynamic formula:
  // logMusic: numOfMusicLogs × (70 + 5) -> numOfMusicLogs × 80 (with margin)
  // overhead: 50
  size_t estimatedSize = (numOfMusicLogs * 80) + 50;
  temp.reserve(estimatedSize);
  
  temp = "{\"LOGS\":[";
  
  for (uint8_t i = 0; i < numOfMusicLogs; i++) {
    temp += "\"";
    temp += logMusic[i];
    temp += "\"";
    if (i < numOfMusicLogs - 1) temp += ",";
  }
  
  //temp += "]}";
  temp += "],";
  temp += "\"Remaining\":\"";
  temp += remainingRefreshTokenDays;
  temp += "\"}";

  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print(temp);
    Serial.println(" sent successfully");
  #endif
}


void showLogTime() {                                                                  // sends log of time server requests
  String temp;
  // Dynamic formula:
  // logMusic: numOfTimecLogs × (70 + 5) -> numOfTimeLogs × 80 (with margin)
  // overhead: 50
  size_t estimatedSize = (numOfTimeLogs * 80) + 50;
  temp.reserve(estimatedSize);
  
  temp = "{\"LOGS\":[";
  
  for (uint8_t i = 0; i < numOfTimeLogs; i++) {
    temp += "\"";
    temp += timeServerLog[i];
    temp += "\"";
    if (i < numOfTimeLogs - 1) temp += ",";
  }
  
  temp += "]}";
  
  server.send(200, "application/json", temp);
  
  #ifdef DEBUG
    Serial.print("String: ");
    Serial.print("JSON size: ");
    Serial.println(temp.length());
    Serial.print(temp);
    Serial.println(" sent successfully");
  #endif
}

//// WEBSERVER LISTENER ////

void listener() {                                                  // handles all server requests from html pages
  server.on("/showSimple", showSimple);
  server.on("/showNews", composeNews);
  server.on("/showAdvanced", showAdvanced);
  server.on("/postFormSimple", handleSimple);
  server.on("/postFormAdv", handleAdvanced);
  server.on("/saveAll", saveAllMessages);
  server.on("/loadAll", loadAll);
  //server.on("/loadDefault", loadDefault);
  server.on("/showConfig", showConfig);
  server.on("/postConfig",handleConfig);
  server.on("/reset", resetDevice);
  server.on("/ex", clearCredentials);
  server.on("/postAPI", handleAPI);
  server.on("/showAPI", showAPI);
  server.on("/saveAPI", saveAPI);
  server.on("/loadAPI", loadAPI);
  server.on("/showLogNews", showLogNews);
  server.on("/showLogWeath", showLogWeath);
  server.on("/showLogMusic", showLogMusic);
  server.on("/showLogTime", showLogTime);
  server.on("/cover", showCover);
  server.on("/ota.html", html_authentify_ota);
  server.on("/castweb.html", html_authentify_castweb);
  server.on("/sketchName", sendSketchName);
  server.on("/showname", showDeviceName);
  server.on("/showDeviceInfo", showDeviceInfo);
  
  server.onNotFound([]() {
            if (!handleFile(server.urlDecode(server.uri())))
              send404();
             });
                   
  server.on("/json", handleList);
  server.on("/format", formatSpiffs);
  server.on("/upload", HTTP_POST, []() {}, handleUpload);
  server.on("/deleteMulti", HTTP_POST, handleDeleteMulti);


 
 #ifdef OTA                   /// OVER THE AIR UPDATE VIA WEBBROWSER (OTA)
                                                    // see: https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/
  server.on("/ota", HTTP_POST, [=]() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", (Update.hasError()) ? OTAfailMsg : OTAsuccessMsg);
    delay(1500);
    ESP.restart();
    }, []() {
    P.setFont(tobFont);
    P.setTextAlignment(PA_CENTER);     
    P.print("OTA running...");
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
     #ifdef DEBUG
      Serial.printf("Update: %s\n", upload.filename.c_str());
     #endif
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
      #ifdef DEBUG
        Update.printError(Serial);
      #endif
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
       #ifdef DEBUG
        Update.printError(Serial);
       #endif
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
       #ifdef DEBUG
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
       #endif
      } else {
       #ifdef DEBUG
        Update.printError(Serial);
       #endif
      }
    }
  });

 #endif                 //endif OTA


 #ifdef SPOTIFY
  server.on("/spotiauth", spotiauth);                               // starts Spotify authentication process
  server.on("/callback", spoticallback);
 #endif

 #ifdef CASTWEB
  server.on("/getDevices", getCastWebDevices);
  server.on("/postCastConf", handleCastWebDevices);
  server.on("/musicInfo", parseMusicInfo);
 #endif
 
}                      // end of function listener()

//// END OF WEBSERVER LISTENER ////


//// GRAPHIC ANIMATION FUNCTIONS ////

void playGraphic (uint8_t slot){                                    // graphic determination and adjustment                                       
 uint8_t type;
 for (uint8_t m=0; m<numOfGraphics; m++) {
  if (slot == locationGraphic[m]) type=enableGraphTEMP[m];
 }
 if (graphOnly) type = enableGraph[0];
 
 switch (type){
  case 1: graphPlaytime = 25;
          graphPlaynum = 5;
          graphicHeartbeat();                               
          break;

  case 2: graphPlaytime = 8;
          graphicSpectrum();
          break;   

  case 3: graphPlaytime = 8;
          graphicArrowRotate();
          break;

  case 4: graphPlaytime = 25;
          graphPlaynum = 5;
          graphicHearts();
          break;

  case 5: graphPlaytime = 8;
          graphicArrowScroll();
          break;

  case 6: graphPlaytime = 8;
          graphicEyes();
          break;

  case 7: graphPlaytime = 8;
          graphicSinewave();
          break;

  case 8: graphPlaytime = 25;
          graphPlaynum = 1;
          graphicPacman();
          break;

  case 9: graphPlaytime = 8;
          graphicMidline2();
          break;

  case 10: graphPlaytime = 25;
           graphPlaynum = 2;
           graphicScanner();
           break;

  case 11: graphPlaytime = 8;
           graphicRandom();
           break;

  case 12: graphPlaytime = 8;
           graphicScroller();
           break;

  case 13: graphPlaytime = 10;
           graphicSpectrum2();
           break;

  case 14: graphPlaytime = 25;
           graphPlaynum = 3;
           graphicWiper();
           break;
 }
}


void graphicHeartbeat(){
 static uint8_t  state;
 static uint8_t  r, c;
 static bool     bPoint;
 if (bInit){
  state = 0;
  r = 4;                                                                       
  c = mx->getColumnCount()-1;
  bPoint = true;
  bInit = false;
 }
 static uint8_t counter=0;
 if (counter <= graphPlaynum+2){
  if (millis()-prevTimeAnim > 5){
    mx->setPoint(r, c, bPoint);
    switch (state){
      case 0: // straight line from the right side
        if (c == mx->getColumnCount()/2 + COL_SIZE) state = 1;
        c--;
        break;

      case 1: // first stroke
        if (r != 0) { r--; c--; }
        else state = 2;
        break;

      case 2: // down stroke
        if (r != ROW_SIZE-1) { r++; c--; }
        else state = 3;
        break;

      case 3: // second up stroke
        if (r != 4) { r--; c--; }
        else state = 4;
        break;

      case 4: // straight line to the left
        if (c == 0){
          c = mx->getColumnCount()-1;
          bPoint = !bPoint;
          state = 0;
          counter++;
        }
        else c--;
        break;

      default:
        state = 0;
  }
  prevTimeAnim = millis(); }  // starting point for next time
 }
 if (counter == graphPlaynum+3){
  counter = 0;
  graphPlaynum = 0;
  }
}


void graphicSpectrum(){
 if (millis()- prevTimeAnim > 100){
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t i=0; i<MAX_DEVICES; i++){
    uint8_t r = random(ROW_SIZE);
    uint8_t cd = 0;
    for (uint8_t j=0; j<r; j++) cd |= 1<<j;
    for (uint8_t j=1; j<COL_SIZE-1; j++)  mx->setColumn(i, j, ~cd);
  }
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();
 }
}


void graphicArrowRotate(){
 static uint16_t idx;        // transformation index
 uint8_t arrow[COL_SIZE] =
  {0b00000000,
   0b00011000,
   0b00111100,
   0b01111110,
   0b00011000,
   0b00011000,
   0b00011000,
   0b00000000};

 MD_MAX72XX::transformType_t  t[] =
  {MD_MAX72XX::TRC, MD_MAX72XX::TRC,
   MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
   MD_MAX72XX::TRC, MD_MAX72XX::TRC,
   MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
   MD_MAX72XX::TRC,};

 if (bInit){
  bInit = false;
  idx = 0;
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);                          // use the arrow bitmap
  for (uint8_t j = 0; j<mx->getDeviceCount(); j++)
  mx->setBuffer(((j + 1)*COL_SIZE) - 1, COL_SIZE, arrow);
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
 }
 
 if (millis() - prevTimeAnim > 200){                                          //200
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  mx->transform(t[idx++]);
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);
  if (idx == (sizeof(t) / sizeof(t[0]))) bInit = true;                      // check if we are completed and set initialize for next time around
  prevTimeAnim = millis();    // starting point for next time
 }
}


void graphicHearts(){
 #define NUM_HEARTS  ((MAX_DEVICES/2) +1)
 const uint8_t heartFull[] = { 0x1c, 0x3e, 0x7e, 0xfc };
 const uint8_t heartEmpty[] = { 0x1c, 0x22, 0x42, 0x84 };
 const uint8_t offset = mx->getColumnCount()/(NUM_HEARTS+1);
 const uint8_t dataSize = (sizeof(heartFull)/sizeof(heartFull[0]));
 static bool   bEmpty;
 static uint8_t counter=0;
 uint8_t corrOffset = 0;                                                      // adjust total offset manually to center graphic
 if (MAX_DEVICES == 8) corrOffset = 2;
 
 if (bInit){
  bEmpty = true;
  bInit = false;
 }
 if (counter <= graphPlaynum+1){
  if (millis()-prevTimeAnim > 700){
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t h=1; h<=NUM_HEARTS; h++){
    for (uint8_t i=0; i<dataSize; i++){
      mx->setColumn((h*offset)-dataSize+i+corrOffset, bEmpty ? heartEmpty[i] : heartFull[i]);
      mx->setColumn((h*offset)+dataSize-i-1+corrOffset, bEmpty ? heartEmpty[i] : heartFull[i]);
    }
  }
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  bEmpty = !bEmpty;
  if (bEmpty) counter++;
  prevTimeAnim = millis();    // starting point for next time
 }
 }
 if (counter == graphPlaynum+1){
  counter = 0;
  graphPlaynum = 0;
 }
}


void graphicArrowScroll(){
 const uint8_t arrow[] = { 0x3c, 0x66, 0xc3, 0x99 };
 const uint8_t dataSize = (sizeof(arrow)/sizeof(arrow[0]));
 static uint8_t  idx = 0;
 if (bInit){
  idx = 0;
  bInit = false;
 }
 if (millis()-prevTimeAnim > 25){                                         // 75
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx->transform(MD_MAX72XX::TSL);
  mx->setColumn(0, arrow[idx++]);
  if (idx == dataSize) idx = 0;
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();
 }
}


void graphicPacman(){
  #define MAX_FRAMES  4   // number of animation frames
  #define PM_DATA_WIDTH  18
  const uint8_t pacman[MAX_FRAMES][PM_DATA_WIDTH] =   // ghost pursued by a pacman
  {
    { 0x3c, 0x7e, 0x7e, 0xff, 0xe7, 0xc3, 0x81, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xe7, 0xe7, 0x42, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xe7, 0x66, 0x24, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
  };

  static int16_t idx;        // display index (column)
  static uint8_t frame;      // current animation frame
  static uint8_t deltaFrame; // the animation frame offset for the next frame
  static uint8_t counter=0;
 if (counter <= graphPlaynum){
  if (bInit){
    bInit = false;
    idx = -1; //DATA_WIDTH;
    frame = 0;
    deltaFrame = 1;
    counter++;
  }

  if (millis() - prevTimeAnim > 35){                                   // 100
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx->clear();
    for (uint8_t i = 0; i < PM_DATA_WIDTH; i++) {
        int16_t col = idx - PM_DATA_WIDTH + i;
        if (col >= 0 && col < mx->getColumnCount()) mx->setColumn(col, 0);                    // clear old graphic
    }
    idx++;                                                                                                  // move reference column and draw new graphic
    for (uint8_t i = 0; i < PM_DATA_WIDTH; i++) {
        int16_t col = idx - PM_DATA_WIDTH + i;
        if (col >= 0 && col < mx->getColumnCount()) mx->setColumn(col, pacman[frame][i]);
    }
    frame += deltaFrame;                                                                                    // advance the animation frame
    if (frame == 0 || frame == MAX_FRAMES - 1) deltaFrame = -deltaFrame;
    if (idx == mx->getColumnCount() + PM_DATA_WIDTH) bInit = true;                                          // check if we are completed and set initialize for next time around
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    prevTimeAnim = millis();    // starting point for next time
  }
 }
 if (counter == graphPlaynum+1){
  counter = 0;
  graphPlaynum = 0;
 }
}


void graphicEyes(){
 #define NUM_EYES  2
 const uint8_t eyeOpen[] = { 0x18, 0x3c, 0x66, 0x66 };
 const uint8_t eyeClose[] = { 0x18, 0x3c, 0x3c, 0x3c };
 const uint8_t offset = mx->getColumnCount()/(NUM_EYES+1);
 const uint8_t dataSize = (sizeof(eyeOpen)/sizeof(eyeOpen[0]));
 bool bOpen;
 if (bInit) bInit = false;
 uint8_t corrOffset = 0;

 if (MAX_DEVICES == 17) corrOffset = 10;
 if (MAX_DEVICES == 20) corrOffset = 10;

 if (millis()-prevTimeAnim > 250){      //500
  bOpen = (random(1000) > 100);
  //bOpen = (random(1000) > 20);
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  if (corrOffset == 0){                                                                   // standard case without manual adjusting
    for (uint8_t e=1; e<=NUM_EYES; e++){
      for (uint8_t i=0; i<dataSize; i++){
      mx->setColumn((e*offset)-dataSize+i, bOpen ? eyeOpen[i] : eyeClose[i]);
      mx->setColumn((e*offset)+dataSize-i-1, bOpen ? eyeOpen[i] : eyeClose[i]);
      }
    }
  } else {                                                                                // two eyes closer to each other
    for (uint8_t i=0; i<dataSize; i++){
      mx->setColumn((1*offset+corrOffset)-dataSize+i, bOpen ? eyeOpen[i] : eyeClose[i]);
      mx->setColumn((1*offset+corrOffset)+dataSize-i-1, bOpen ? eyeOpen[i] : eyeClose[i]);
    }
    for (uint8_t i=0; i<dataSize; i++){
      mx->setColumn((2*offset-corrOffset)-dataSize+i, bOpen ? eyeOpen[i] : eyeClose[i]);
      mx->setColumn((2*offset-corrOffset)+dataSize-i-1, bOpen ? eyeOpen[i] : eyeClose[i]);
    }
  }
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();
 }
}


void graphicSinewave(){
 static uint8_t curWave = 0;
 static uint8_t idx;
 #define SW_DATA_WIDTH  11    // valid data count followed by up to 10 data points
 const uint8_t waves[][SW_DATA_WIDTH] =
  {
    {  9,   8,  6,   1,   6,  24,  96, 128,  96,  16,   0 },
    {  6,  12,  2,  12,  48,  64,  48,   0,   0,   0,   0 },
    { 10,  12,   2,   1,   2,  12,  48,  64, 128,  64, 48 },

  };
 const uint8_t WAVE_COUNT = sizeof(waves) / (SW_DATA_WIDTH * sizeof(uint8_t));

 if (bInit){
  bInit = false;
  idx = 1;
 }
 if (millis() - prevTimeAnim > 25){                                                     // 50
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  mx->transform(MD_MAX72XX::TSL);
  mx->setColumn(0, waves[curWave][idx++]);
  if (idx > waves[curWave][0]){
    curWave = random(WAVE_COUNT);
    idx = 1;
  }
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);
  prevTimeAnim = millis();    // starting point for next time
 }
}


void graphicMidline2(){
 static uint8_t  idx = 0;      // position
 static int8_t   idOffs = 1;   // increment direction
 if (bInit){
    idx = 0;
    idOffs = 1;
    bInit = false;
 }
 if (millis()-prevTimeAnim > 80){                                            // 150
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t j=0; j<MAX_DEVICES; j++){                                     // turn off the old lines
    mx->setRow(j, idx, 0x00);
    mx->setRow(j, ROW_SIZE-1-idx, 0x00);
  }
  idx += idOffs;
  if ((idx == 0) || (idx == ROW_SIZE-1))
  idOffs = -idOffs;
  for (uint8_t j=0; j<MAX_DEVICES; j++){                                      // turn on the new lines
    mx->setRow(j, idx, 0xff);
    mx->setRow(j, ROW_SIZE-1-idx, 0xff);
  }
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();    // starting point for next time
 }
}


void graphicScanner(){
 const uint8_t width = 3;      // scanning bar width
 static uint8_t  idx = 0;      // position
 static int8_t   idOffs = 1;   // increment direction
 static uint8_t counter=0;
 if (counter <= graphPlaynum){
  if (bInit){
    idx = 0;
    idOffs = 1;
    bInit = false;
  }
  if (millis()-prevTimeAnim > 10){                                               // 50
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for (uint8_t i=0; i<width; i++) mx->setColumn(idx+i, 0);                      // turn off the old lines
    idx += idOffs;
    if ((idx == 0) || (idx + width == mx->getColumnCount())) idOffs = -idOffs;
    if (idx == 0) counter++;
    for (uint8_t i=0; i<width; i++) mx->setColumn(idx+i, 0xff);                   // turn on the new lines
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    prevTimeAnim = millis();    // starting point for next time
  }
 }
 if (counter == graphPlaynum){
  counter = 0;
  graphPlaynum = 0;
 }
}


void graphicRandom(){
 if (bInit) bInit = false;
 if (millis()-prevTimeAnim > 150){
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t i=0; i<mx->getColumnCount(); i++) mx->setColumn(i, (uint8_t)random(255));
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();    // starting point for next time
 }
}


void graphicScroller(){
 const uint8_t   width = 3;     // width of the scroll bar
 const uint8_t   offset = mx->getColumnCount()/6;                           // /3
 static uint8_t  idx = 0;      // counter
 if (bInit){
  idx = 0;
  bInit = false;
 }
 if (millis()-prevTimeAnim > 20){                                   // 50
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx->transform(MD_MAX72XX::TSL);
  mx->setColumn(0, idx>=0 && idx<width ? 0xff : 0);
  if (++idx == offset) idx = 0;
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  prevTimeAnim = millis();    // starting point for next time
 }
}


void graphicSpectrum2(){
 if (bInit) bInit = false;
  if (millis() - prevTimeAnim > 100){
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for (uint8_t i = 0; i<mx->getColumnCount(); i++){
      uint8_t r = random(ROW_SIZE);
      uint8_t cd = 0;
    for (uint8_t j = 0; j<r; j++) cd |= 1 << j;
    mx->setColumn(i, ~cd);
    }
    mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    prevTimeAnim = millis();    // starting point for next time
  }
}


void graphicWiper(){
 static uint8_t  idx = 0;      // position
 static int8_t   idOffs = 1;   // increment direction
 static uint8_t counter=0;
 if (counter <= graphPlaynum){
  if (bInit){ 
    idx = 0;
    bInit = false;
  }
  if (millis()-prevTimeAnim > 8){                                           // 25 15
    mx->setColumn(idx, idOffs == 1 ? 0xff : 0);
    idx += idOffs;
    if ((idx == 0) || (idx == mx->getColumnCount())) idOffs = -idOffs;
    if (idx == 0) counter++;
    prevTimeAnim = millis();    // starting point for next time
  }
 }
 if (counter == graphPlaynum){
  counter = 0;
  graphPlaynum = 0;}
}

//// END OF GRAPHIC FUNCTIONS ////



//// SETUP ////

void setup(void){      
  P.begin();                                 // Parola begin
  P.setIntensity(0);                         // sets safe start value of 0, gets overwritten if config file is loaded successfully
  #ifdef DEBUG
    Serial.begin(57600);
  #endif

  mx = P.getGraphicObject();                 // graphic instance initialization
  
  SPIFFS.begin();
   
  loadConfig();                              // loads configuration saved in SPIFFS
  if (auto_intens == true) checkLDR();       // LDR
  loadAPI();                                 // load API information from files 
 
  P.setTextAlignment(PA_CENTER);
  P.print("starting up...");
  //P.displayReset();                        // neu 0.8
  
  wificonnect();                             // connects to WiFi or establishes access point
  //WiFi.setSleep(false);                    // Test

  checkIfAP();                               // sets up AP if WifiManager config portal has been skipped

  #ifdef NEWS_SECURE
    loadNewsAuth();
  #endif

  P.displayClear();
  P.print("getting data");

  if(AP_established == false){                      // normal operation with wifi connection
      getTimeFromServer();                          // calls time server for current time
      makeDate();                                   // forced call to enable writing into of dayAfterTomorrow
      loadAllMessages(0);                           // loads messages and messages config saved in SPIFFS
      presetMSG = 0;
  } else {                                          // operation as AP
      loadAllMessages(100);
      presetMSG = 100;                    
  }
  
  if (enableNews1==0 && enableNews2==0) snprintf(table[0].newscur, sizeof(table[0].newscur), "%s", msgNoNews);     // message shown in news.html when no news loaded

#ifdef SPOTIFY
  loadSpotifyAuth();
  if (enableSpotify == 1){
    #ifdef DEBUG
      Serial.println("Free heap: ");
      Serial.println(ESP.getFreeHeap());
    #endif
    refreshSpotify();
    parseSpotify();
  }
#endif

#ifdef CASTWEB
  loadCastWebDevices();    
  if (enableSpotify == 2) parseCastWeb();
#endif
     
  server.begin();                              // starts webserver
  delay(100);
  
  //P.displayReset();                         // neu 0.8
  P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);        

  P.addChar(degreeC_ascii, degreeC);                 // adds characters for celsius and fahrenheit
  P.addChar(degreeF_ascii, degreeF);

  if(AP_established == false){
    snprintf(infowifi, sizeof(infowifi), "%s %s %s %d.%d.%d.%d",
             msgNetwork, WiFi.SSID().c_str(), msgAs, WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  } else {
    snprintf(infowifi, sizeof(infowifi), "%s %s %s %d.%d.%d.%d",
             msgAP, AP_NAME, msgWith, WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
  }

  delay(500);                                 // keeps "getting data" on display
  P.displayClear();
  P.displayText(msgWelcome ,PA_CENTER,50,3000,PA_SCROLL_UP_LEFT,PA_SCROLL_DOWN_RIGHT);           //message shown on start up
  
  listener();                                   // server listens for request sent by html sites
  P.displayReset();  
}




//// LOOP ////                    

void loop(void){

  if(AP_established == false)  wifiReconnect();                                     // necessary only for ESP32 to reconnect if WiFi connection was lost
     
  server.handleClient();

  if (auto_intens == true){
    if (millis() - last_LDR_Millis > ldr_interval) {                                // read ldr sensor every 250 ms
      checkLDR();
      last_LDR_Millis = millis();
    }
  }


   /// MAIN DISPLAY ROUTINE VARIABLES ///
  static uint8_t i = 0;           //counter table sCatalog
  static uint8_t si = 0;          //sprite in  -> position in table bildlein
  static uint8_t so = 0;          //sprite out -> position in table bildlein
  static uint8_t a = 0;           //counter table maintable.wetcur
  static uint8_t b = 0;           //counter table maintable.newscur


  /// GRAPHIC ANIMATION VARIABLES ///
  static bool graphic = false;
  static uint32_t startTime;
  static bool startOnce = false;
  
  static bool timeWithSecs = false;           // variable if showing time withs seconds is active
  static unsigned long timeshowmillis;        

if (timeOnly) {                       // if all messages except time are deactivated
   displayOnlyTime();
   if (msgUpdated){                                                           
    i=1;                               // resets message loop to start position 1 (for return out of 'time only' mode into normal mode)
    graphic=false;                     // condition for proper start at pos 1
    msgUpdated = false;
  }

} else if (timeWithSecs) {            // leaves main routine for showing time with seconds
 
  if (millis() - timeshowmillis > (catalog[locationTime].delay * delayTime)) displayOnlyTime(true);
  if (millis() - timeshowmillis > ((catalog[locationTime].delay * delayTime) + (catalog[locationTime].pause * pauseTime))){ 
    timeWithSecs = false;
  }

} else if (singleMsgOnly) {            // leaves main routine for constantly showing a single message

  if (msgUpdated){
    utf8AsciiConvert(messages[0],showmessages[0]);
    P.displayClear();
    P.setFont(tobFont);
    P.setTextAlignment(PA_CENTER);                                                    
    P.print(showmessages[0]);
    i=1;                              // resets message loop to start position 1 (for return out of 'single static message only mode' into normal mode)
    graphic=false;                    // condition for start at pos 1
    msgUpdated=false; 
  }

} else {                          // if time AND other messages are activated --> MAIN ROUTINE and STANDARD CASE
   
   displayTime();  

  if (millis() - previousweathercall > (weatherRefreshInterval* 60000)){   // getting new weather data from server every x minutes
    if (enableWeath == 1) weatherUpdate = true;
  }

  if (millis() - previousnewscall > (newsRefreshInterval* 60000)){        // getting new news data from server every x minutes
    if (enableNews1 == 1 || enableNews2 == 1 ) newsUpdate = true;
  }

  if (millis() - previoustimecall > (timeRefreshInterval* 60000) && P.displayAnimate() && !graphic){      // get time from server every x minutes
    if (AP_established == false && WiFi.status() == WL_CONNECTED) getTimeFromServer();                                         
    previoustimecall = millis();
  }
  
    
#ifdef SPOTIFY
   if (enableSpotify == 1){
    if (millis() - previousrefresh > (1000 * expiretime) && !graphic && P.displayAnimate() && WiFi.status() == WL_CONNECTED){
      refreshSpotify();
      previousrefresh = millis();
    }
   }
#endif


//// MAIN DISPLAY ROUTINE ////

uint8_t j = i-1;                                                                  
if (i==1) j = ARRAY_SIZE(catalog)-1;                                               // i == 1 und nicht null, weil cataolg nach startup immer bei position 1 startet 
      
if (!graphic){                                                                    // normal mode
 if (millis() - lastAnimation > (catalog[j].delay * delayTime)){ 
  if (P.displayAnimate()){                                                        // animates and returns true when an animation is completed
    lastAnimation = millis();                                                     // got rid of delay()
    #ifdef DEBUG
    Serial.print("Catalog pos: ");
    Serial.println(i);
  #endif
    P.displayClear();                                                             // clears display before animation (necessary for return out of 'time only' mode)
    
     if (i==locationUpdate){                                                      // updating before parola valies are set and before animation runs
      if (weatherUpdate && WiFi.status() == WL_CONNECTED){
        refreshWeather();
        weatherUpdate = false;
        previousweathercall = millis();
      }
      if (newsUpdate && WiFi.status() == WL_CONNECTED) {
        refreshNews();
        newsUpdate = false;
        previousnewscall = millis();
      }
   }
      
    P.setFont(tobFont);                                                           // Parola standard font set
    
    uint16_t speedIn;                                                             // speed effect in 
    uint16_t speedOut;                                                            // speed effect out 
    
    switch (catalog[i].effectin) {                                                // adjusts speed values according to effect
    case 5:  speedIn = 1;  break;       //PA_SLICE           
    case 6:  speedIn = 40; break;       //PA_MESH
    case 7:  speedIn = 100; break;      //PA_FADE  
    case 8:  speedIn = 100; break;      //PA_DISSOLVE
    case 9:  speedIn = 25; break;       //PA_BLINDS
    case 14:                            //PA_SCAN_VERT
    case 15: speedIn = 25; break;       //PA_SCAN_VERTX
    case 24:                            //PA_GROW_UP
    case 25: speedIn = 25; break;       //PA_GROW_DOWN
    case 27: speedIn = 3;  break;       //PA_RANDOM
    default: speedIn = 5;               //all other
    }

    switch (catalog[i].effectout) {                                               // adjusts speed values according to effect
    case 5:  speedOut = 1;  break;      //PA_SLICE           
    case 6:  speedOut = 40; break;      //PA_MESH
    case 7:  speedOut = 100; break;     //PA_FADE  
    case 8:  speedOut = 100; break;     //PA_DISSOLVE
    case 9:  speedOut = 25; break;      //PA_BLINDS
    case 14:                            //PA_SCAN_VERT
    case 15: speedOut = 25; break;      //PA_SCAN_VERTX
    case 24:                            //PA_GROW_UP
    case 25: speedOut = 25; break;      //PA_GROW_DOWN
    case 27: speedOut = 3;  break;      //PA_RANDOM
    default: speedOut = 5;              //all other
    }

    speedIn = speedIn*catalog[i].speed;
    speedOut = speedOut*catalog[i].speed;
    
    uint8_t effIn;
    uint8_t effOut;

    if (catalog[i].effectin == 99) effIn = random(0,44); else effIn = catalog[i].effectin;
    if (catalog[i].effectout == 99) effOut = random(0,44); else effOut = catalog[i].effectout;
    
     
    if (effIn >= 28)                                                // picks sprites from table bildlein according to chosen effect
    si = effIn - 28;

    if (effOut >= 28)
    so = effOut - 28;

    P.setSpriteData(sprite[si].data, sprite[si].width, sprite[si].frames,         // entry sprite
                      sprite[so].data, sprite[so].width, sprite[so].frames);      // exit sprite
    
    P.setTextEffect(effectlist[effIn],effectlist[effOut]);
    P.setSpeedInOut(speedIn,speedOut); 
    P.setTextBuffer(catalog[i].psz);
    P.setTextAlignment(justlist[catalog[i].just]);
    P.setPause(catalog[i].pause * pauseTime);

  
   for (uint8_t m=0; m<numOfMessages; m++) {                                                // loads custom messages and configuration from array sCatalogTEMP into SHOW ARRAY sCatalog
        if (i==(location[m]-1)){  //-1
            if (enablemessage[m]==1){
                utf8AsciiConvert(messages[m],showmessages[m]);
                catalog[location[m]].effectin = catalogTEMP[m].effectinTEMP;
                catalog[location[m]].effectout = catalogTEMP[m].effectoutTEMP;
                catalog[location[m]].just = catalogTEMP[m].justTEMP;
                catalog[location[m]].speed = catalogTEMP[m].speedTEMP;
                catalog[location[m]].pause = catalogTEMP[m].pauseTEMP;
                catalog[location[m]].delay = catalogTEMP[m].delayTEMP;
            } else {
                showmessages[m][0] = '\0';
                catalog[location[m]].effectin = 0;
                catalog[location[m]].effectout = 26;
                catalog[location[m]].just = 1;
                catalog[location[m]].speed = 0;
                catalog[location[m]].pause = 0;
                catalog[location[m]].delay = 0;
            }
         }
       }

    if (i==location[1]-2){ //-2
      enableOrDisable(location[1]-1, showcustomServiceMsg, enablemessage[1], 5);
      if (enablemessage[1]==1 && enableServiceMsg==1)  utf8AsciiConvert(customServiceMsg, showcustomServiceMsg);
    }
    
    if (i==locationTime){                                                                                          
      if (enableTime==1){
        if (enableTimeXXL==1){
          #if MAX_DEVICES < 20
            snprintf (timeshow, sizeof(timeshow), "%s-%s", timeSaver, dateSaver);                                     // time and date combined (smaller version)   
          #else
            snprintf (timeshow, sizeof(timeshow), "%s %s - %s", timeSaver, msghour, dateSaver);                       // time and date combined     
          #endif
        }
        else if (enableTimeXXL==2){                                                                                   // time with seconds
          timeshowmillis = millis();
          timeWithSecs = true;                                                                                        // leave main routine for time with seconds
        } else {                                                                                                      // time normal mode
          strftime (timeshow, sizeof(timeshow), "%H:%M", &tm);
          P.setFont(numeric7Seg);                                                                                     // special font set only for numbers in time display
        }
      }
    }
    
    if (i==(locationWeath-2)) {
       enableOrDisable(locationWeath-1, showweatherService, enableWeath, 5);
       if (enableWeath==1 && enableServiceMsg==1) snprintf(showweatherService, sizeof(showweatherService), "%s", weatherService); 
    }
    
    if (i==(locationWeath-1)) {
        if (enableWeath == 1){
          snprintf(showweather, sizeof(showweather), "%s",  table[a].wetcur);                                         // loading weather info into "showweather"
          a++;
       }
    }
    
    if (i==(locationNews[0]-2)) {
       enableOrDisable(locationNews[0]-1, shownewsService, enableNews1, 5);                                          
      if (enableNews1==1 && enableServiceMsg==1) snprintf(shownewsService, sizeof(shownewsService), "%s", newsService); 
    }
    
    if (i==(locationNews[0]-1)) {                                                                                 // loading news into "shownews1"
        if (enableNews1 == 1){
          utf8AsciiConvert(table[b].newscur,shownews);
          b++;
      }
    }

    if (i==(locationNews[1]-2)) {
       enableOrDisable(locationNews[1]-1, shownewsService2, enableNews2, 5);                                         
      if (enableNews2==1 && enableServiceMsg==1) snprintf(shownewsService2, sizeof(shownewsService2), "%s", newsService); 
    }

    if (i==(locationNews[1]-1)) {                                                                                // loading news into "shownews2"
        if (enableNews2 == 1){
          utf8AsciiConvert(table[b].newscur,shownews2);
          b++;
      }
    }

    if (i==(locationOwn-2)) {
      enableOrDisable(locationOwn-1, showownmsgService, enableOwn, 5);                                                
      if (enableOwn==1 && enableServiceMsg==1) snprintf(showownmsgService, sizeof(showownmsgService), "%s", ownmsgService); 
    }
    
    if (i==(locationOwn)){                                                                                            // loading and converting ownmessage into showownmessage if enabled
        if (enableOwn == 1) {
          if (strcmp(ownName, "") == 0) snprintf(showownmessage, sizeof(showownmessage), "%s",ownmessage);
          else snprintf(showownmessage, sizeof(showownmessage), "Nachricht von %s: %s", ownName, ownmessage);
          utf8AsciiConvert(showownmessage, showownmessage);
        }
    }

    if (i==locationSpot-2){                                                                                      // calling spotify and loading information about currently playing song if enabled
        enableOrDisable(locationSpot-1,showspotifyService, enableSpotify, 5);
        if (enableSpotify == 1){
         #ifdef SPOTIFY
            parseSpotify();
         #endif
         }
        if (enableSpotify == 2){
         #ifdef CASTWEB 
           parseCastWeb();
         #endif
        }
        if (enableSpotify == 3){
         #ifdef CASTWEB 
           newMusicLoad();
         #endif
        }
    }

    #ifdef CASTWEB                                                          // double check if new music data arrived
    if (i==locationSpot && enableSpotify == 3) newMusicLoad();
    #endif

    for (uint8_t m=0; m<numOfGraphics; m++) {                                                     // checks if graphic slots are activated                                    
      if (i==locationGraphic[m]-1) enableGraphTEMP[m] = enableGraph[m];
      if (enableGraphTEMP[m]==15) enableGraphTEMP[m]=random(1,15);                                //  https://techtutorialsx.com/2017/12/22/esp32-arduino-random-number-generation/
      if (i==locationGraphic[m] && enableGraph[m] > 0) {                                          
         #ifdef DEBUG
          Serial.print("Leaving main routine for graphic at catalog pos: ");
          Serial.println(i);
         #endif
        mx->clear();
        startOnce = true;
        graphic = true;                             
      }
    }

    i++;                                                                           // show next message of main array
    P.displayReset();
    
    if (i == ARRAY_SIZE(catalog)) i= 1;                                            // loop main array without first message
     
    if (a == numberofcities*2) a= 0;                                               // loop weather data
 
    if (b == numofarticles*newssources) b = 0;                                     // loop news data
   }
  }
} else {                                                                          // if graphic mode active
  if (millis() - lastAnimation > (1 * delayTime)){
    if (startOnce){                                                               // grahipc animation startup
      startOnce = false; 
      bInit=true; 
      startTime = millis(); 
      graphPlaynum = 1;
    }
    playGraphic(i-1);
  if (graphOnly){                                                               // graphic only mode
    if (msgUpdated){                                                           
      mx->clear();
      bInit=true;                                                              
      msgUpdated = false;
     }
     graphPlaynum = 1;
     graphPlaytime = 5;
  } else {                                                                      // normal graphic routine
      if (millis() - startTime >= graphPlaytime*1000 || graphPlaynum == 0 ){
        #ifdef DEBUG                                                              
          Serial.print("Back from graphic to main routine at catalog pos: ");
          Serial.println(i-1);
        #endif
        P.displayClear();
        P.displayReset();
        lastAnimation = millis();
        bInit=true;
        graphic=false;
      }
   }
  }
 }  
}
}
