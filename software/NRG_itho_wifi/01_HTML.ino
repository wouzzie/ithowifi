

#include <ESPAsyncWebServer.h>

const char html_mainpage[] PROGMEM = R"=====(
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="A layout example with a side menu that hides on mobile, just like the Pure website.">
    <title>Itho WiFi controller &ndash; System setup</title>
    <script src="/js/zepto.min.js"></script>
    <script src="/js/controls.js"></script>
    <link rel="stylesheet" href="pure-min.css">
    <style>.ithoset{padding: .3em .2em !important;}.dot-elastic{position:relative;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElastic 1s infinite linear}.dot-elastic::after,.dot-elastic::before{content:'';display:inline-block;position:absolute;top:0}.dot-elastic::before{left:-15px;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElasticBefore 1s infinite linear}.dot-elastic::after{left:15px;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElasticAfter 1s infinite linear}@keyframes dotElasticBefore{0%{transform:scale(1,1)}25%{transform:scale(1,1.5)}50%{transform:scale(1,.67)}75%{transform:scale(1,1)}100%{transform:scale(1,1)}}@keyframes dotElastic{0%{transform:scale(1,1)}25%{transform:scale(1,1)}50%{transform:scale(1,1.5)}75%{transform:scale(1,1)}100%{transform:scale(1,1)}}@keyframes dotElasticAfter{0%{transform:scale(1,1)}25%{transform:scale(1,1)}50%{transform:scale(1,.67)}75%{transform:scale(1,1.5)}100%{transform:scale(1,1)}}</style>
</head>
<body>
<div id="layout">
<div id="message_box"></div>
    <!-- Menu toggle -->
    <a href="#menu" id="menuLink" class="menu-link">
        <!-- Hamburger icon -->
        <span></span>
    </a>
    <div id="menu">
        <div class="pure-menu">
            <a class="pure-menu-heading" id="headingindex" href="index">Itho WiFi controller</a>
            <ul class="pure-menu-list">
                <li class="pure-menu-item"><a href="wifisetup" class="pure-menu-link">Wifi setup</a></li>
                <li class="pure-menu-item"><a href="system" class="pure-menu-link">System settings</a></li>
                <li class="pure-menu-item"><a href="itho" class="pure-menu-link">Itho settings</a></li>
                <li class="pure-menu-item"><a href="status" class="pure-menu-link">Itho status</a></li>                
                <li id="remotemenu" class="pure-menu-item hidden"><a href="remotes" class="pure-menu-link">RF remotes</a></li>
                <li class="pure-menu-item"><a href="mqtt" class="pure-menu-link">MQTT</a></li>
                <li class="pure-menu-item"><a href="api" class="pure-menu-link">API</a></li>
                <li class="pure-menu-item"><a href="help" class="pure-menu-link">Help</a></li>
                <li class="pure-menu-item"><a href="update" class="pure-menu-link">Update</a></li>
                <li class="pure-menu-item"><a href="reset" class="pure-menu-link">Reset</a></li>
                <li class="pure-menu-item"><a href="debug" class="pure-menu-link">Debug</a></li>
            </ul>
        <div id="memory_box"></div>
        </div>
    </div>
    <div id="main">
    </div>
</div>
<div id="loader">
<div style="width: 100px;position: absolute;top: calc(50% - 7px);left: calc(50% - 50px);text-align: center;">Connecting...</div>
<div id="spinner"></div>
</div>
</body>
<script src="/js/general.js"></script>
</html>
)=====";


void handleAPI(AsyncWebServerRequest *request) {
  bool parseOK = false;
  nextIthoTimer = 0;
  
  int params = request->params();
  
  if (systemConfig.syssec_api) {
    bool username = false;
    bool password = false;
    
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(strcmp(p->name().c_str(), "username") == 0) {
        if (strcmp(p->value().c_str(), systemConfig.sys_username) == 0 ) {
          username = true;
        }
      }
      else if(strcmp(p->name().c_str(), "password") == 0) {
        if (strcmp(p->value().c_str(), systemConfig.sys_password) == 0 ) {
          password = true;
        }
      }
    }
    if (!username || !password) {
      request->send(200, "text/html", "AUTHENTICATION FAILED");
      return;
    }  
  }
  
  for(int i=0;i<params;i++){
    char logBuff[LOG_BUF_SIZE] = "";
    AsyncWebParameter* p = request->getParam(i);
    
    if(strcmp(p->name().c_str(), "get") == 0) {
      AsyncWebParameter* p = request->getParam("get");
      if (strcmp(p->value().c_str(), "currentspeed") == 0 ) {
        char ithoval[5];
        sprintf(ithoval, "%d", ithoCurrentVal);
        request->send(200, "text/html", ithoval);
        return;
      }
      else if (strcmp(p->value().c_str(), "queue") == 0 ) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");        
        DynamicJsonDocument root(1000);
        JsonObject obj = root.to<JsonObject>(); // Fill the object
        ithoQueue.get(obj);
        obj["ithoSpeed"] = ithoQueue.ithoSpeed;
        obj["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
        obj["fallBackSpeed"] = ithoQueue.fallBackSpeed;
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }      
    }
    else if(strcmp(p->name().c_str(), "debug") == 0) {
      if (strcmp(p->value().c_str(), "format") == 0 ) {
        formatFileSystem = true;
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "reboot") == 0 ) {
        
        shouldReboot = true;
        jsonLogMessage(F("Reboot requested"), WEBINTERFACE);

        parseOK = true;
      }
#if defined (HW_VERSION_TWO)     
      if (strcmp(p->value().c_str(), "level0") == 0 ) {
        
        debugLevel = 0;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level1") == 0 ) {
        
        debugLevel = 1;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }        
      if (strcmp(p->value().c_str(), "level2") == 0 ) {
        
        debugLevel = 2;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level3") == 0 ) {
        
        debugLevel = 3;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }      
#endif  
    }      
    else if(strcmp(p->name().c_str(), "command") == 0) {
      if (strcmp(p->value().c_str(), "low") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_low;
        updateItho = true;
      }
      else if (strcmp(p->value().c_str(), "medium") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_medium;
        updateItho = true;
      }
      else if (strcmp(p->value().c_str(), "high") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        updateItho = true;      
      }
      else if (strcmp(p->value().c_str(), "timer1") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer1;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "timer2") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer2;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "timer3") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer3;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "clearqueue") == 0 ) {
        parseOK = true;
        clearQueue = true;
      }
    }
    else if(strcmp(p->name().c_str(), "speed") == 0) {
      uint16_t val = strtoul (p->value().c_str(), NULL, 10);
      if (val > 0 && val < 255) {
        parseOK = true;
        nextIthoVal = val;
        updateItho = true;        
      }    
      else if (strcmp(p->value().c_str(), "0") == 0 ) {
        parseOK = true;
        nextIthoVal = 0;
        updateItho = true;
      }
  
    }
    else if(strcmp(p->name().c_str(), "timer") == 0) {
      uint16_t timer = strtoul (p->value().c_str(), NULL, 10);
      if (timer > 0 && timer < 65535) {
        parseOK = true;
        nextIthoTimer = timer;
        updateItho = true;        
      }    
    }  

  }


  if(parseOK) {
    request->send(200, "text/html", "OK");
  }
  else {
    request->send(200, "text/html", "NOK");
  }
  
}

void handleDebug(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print(F("<div class=\"header\"><h1>Debug page</h1></div><br><br>"));
  response->print(F("<div>Config version: "));
  response->print(CONFIG_VERSION);
  response->print(F("<br><br><span>Itho I2C connection status: </span><span id=\'ithoinit\'>unknown</span></div>"));
  
  response->print(F("<br><span>File system: </span><span>"));
#if defined (HW_VERSION_ONE)
  SPIFFS.info(fs_info);
  response->print(fs_info.usedBytes);
#elif defined (HW_VERSION_TWO)
  response->print(SPIFFS.usedBytes());
#endif
  response->print(F(" bytes used / "));
#if defined (HW_VERSION_ONE)
  response->print(fs_info.totalBytes);
#elif defined (HW_VERSION_TWO)
  response->print(SPIFFS.totalBytes());
#endif   
  response->print(F(" bytes total</span><br><a href='#' class='pure-button' onclick=\"$('#main').empty();$('#main').append( html_edit );\">Edit filesystem</a>&nbsp;<button id=\"format\" class=\"pure-button\">Format filesystem</button>"));
#if defined (HW_VERSION_TWO)
  response->print(F("<br><br><span>CC1101 task memory: </span><span>"));
  response->print(TaskCC1101HWmark);
  response->print(F(" bytes free</span>"));
  response->print(F("<br><span>MQTT task memory: </span><span>"));
  response->print(TaskMQTTHWmark);
  response->print(F(" bytes free</span>"));    
  response->print(F("<br><span>Web task memory: </span><span>"));
  response->print(TaskWebHWmark);
  response->print(F(" bytes free</span>"));
  response->print(F("<br><span>Config and Log task memory: </span><span>"));
  response->print(TaskConfigAndLogHWmark);
  response->print(F(" bytes free</span>"));  
  response->print(F("<br><span>SysControl task memory: </span><span>"));
  response->print(TaskSysControlHWmark);
  response->print(F(" bytes free</span></div>"));
#endif 
  response->print(F("<br><br><div id='syslog_outer'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>System Log:</div>"));
  
  response->print(F("<div style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>"));
  char link[24] = "";
  char linkcur[24] = "";

  if ( SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(linkcur, "/logfile0.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
    strlcpy(linkcur, "/logfile1.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile0.log", sizeof(link));      
  }

  File file = SPIFFS.open(linkcur, FILE_READ);
  while (file.available()) {
    if(char(file.peek()) == '\n') response->print("<br>");
    response->print(char(file.read()));
  }
  file.close();

  response->print(F("</div><div style='padding-top:5px;'><a class='pure-button' href='/curlog'>Download current logfile</a>"));

  if ( SPIFFS.exists(link) ) {
    response->print(F("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>"));

  }

#if defined (HW_VERSION_TWO)  
  response->print(F("</div></div><br><br><div id='rflog_outer' class='hidden'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>RF Log:</div>"));
  response->print(F("<div id='rflog' style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>"));
  response->print(F("</div><div style='padding-top:5px;'><a href='#' class='pure-button' onclick=\"$('#rflog').empty()\">Clear</a></div></div></div>"));
#endif

  response->print(F("<form class=\"pure-form pure-form-aligned\"><fieldset><legend><br>Low level itho I2C commands:</legend><br><span>I2C virtual remote commands:</span><br><button id=\"button1\" class=\"pure-button pure-button-primary\">Low</button>&nbsp;<button id=\"button2\" class=\"pure-button pure-button-primary\">Medium</button>&nbsp;<button id=\"button3\" class=\"pure-button pure-button-primary\">High</button><br><br>"));
  response->print(F("<button id=\"buttonjoin\" class=\"pure-button pure-button-primary\">Join</button>&nbsp;<button id=\"buttonleave\" class=\"pure-button pure-button-primary\">Leave</button><br><br>"));
  response->print(F("<button id=\"buttontype\" class=\"pure-button pure-button-primary\">Query Devicetype</button><br><span>Result:&nbsp;</span><span id=\'ithotype\'></span><br><br>"));
  response->print(F("<button id=\"buttonstatusformat\" class=\"pure-button pure-button-primary\">Query Status Format</button><br><span>Result:&nbsp;</span><span id=\'ithostatusformat\'></span><br><br>"));
  response->print(F("<button id=\"buttonstatus\" class=\"pure-button pure-button-primary\">Query Status</button><br><span>Result:&nbsp;</span><span id=\'ithostatus\'></span><br><br>"));
  response->print(F("<button id=\"button2400\" class=\"pure-button pure-button-primary\">Query 2400</button><br><span>Result:&nbsp;</span><span id=\'itho2400\'></span><br><br>"));
  response->print(F("<button id=\"button2401\" class=\"pure-button pure-button-primary\">Query 2401</button><br><span>Result:&nbsp;</span><span id=\'itho2401\'></span><br><br>"));
  response->print(F("<button id=\"button2410\" class=\"pure-button pure-button-primary\">Query 2410</button>setting index: <input id=\"itho_setting_id\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"><br><span>Result:&nbsp;</span><span id=\'itho2410\'></span><br><span>Current:&nbsp;</span><span id=\'itho2410cur\'></span><br><span>Minimum value:&nbsp;</span><span id=\'itho2410min\'></span><br><span>Maximum value:&nbsp;</span><span id=\'itho2410max\'></span><br><br>"));
  response->print(F("<span style=\"color:red\">Warning!!<br> \"Set 2410\" changes the settings of your itho unit<br>Use with care and use only if you know what you are doing!</span><br>"));
  response->print(F("<button id=\"button2410set\" class=\"pure-button pure-button-primary\">Set 2410</button>setting index: <input id=\"itho_setting_id_set\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"> setting value: <input id=\"itho_setting_value_set\" type=\"number\" min=\"-2147483647\" max=\"2147483647\" size=\"10\" value=\"0\"><br><span>Sent command:&nbsp;</span><span id=\'itho2410set\'></span><br><span>Result:&nbsp;</span><span id=\'itho2410setres\'></span><br>"));
  response->print(F("<span style=\"color:red\">Warning!!</span><br><br>"));
  response->print(F("<button id=\"button31DA\" class=\"pure-button pure-button-primary\">Query 31DA</button><br><span>Result:&nbsp;</span><span id=\'itho31DA\'></span><br><br>"));
  response->print(F("<button id=\"button31D9\" class=\"pure-button pure-button-primary\">Query 31D9</button><br><span>Result:&nbsp;</span><span id=\'itho31D9\'></span></fieldset></form><br>"));

  response->print("<br><br>");
  
  request->send(response);
  
  
  DelayedReq.once(1, []() { sysStatReq = true; });

}

void handleCurLogDownload(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile0.current.log", sizeof(link));
  }
  else {
    strlcpy(link, "/logfile1.current.log", sizeof(link));
  }  
  request->send(SPIFFS, link, "", true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
     strlcpy(link, "/logfile0.log", sizeof(link)); 
  }  
  request->send(SPIFFS, link, "", true);
}
