/*
 *  Copyright (c) 2015, Parse, LLC. All rights reserved.
 *
 *  You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
 *  copy, modify, and distribute this software in source code or binary form for use
 *  in connection with the web services and APIs provided by Parse.
 *
 *  As with any software that integrates with the Parse platform, your use of
 *  this software is subject to the Parse Terms of Service
 *  [https://www.parse.com/about/terms]. This copyright notice shall be
 *  included in all copies or substantial portions of the software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ParseClient.h"
#include <sys/time.h>

const bool DEBUG = true;
const char* PARSE_API = "api.parse.com";
const char* USER_AGENT = "arduino-zero.v.1.0.1";
const char* CLIENT_VERSION = "1.0";
const char* PARSE_PUSH = "push.parse.com";
const unsigned short SSL_PORT = 443;

/*
 * !!! IMPORTANT !!!
 * This function does not conform to RFC 4122, even though it returns
 * a string formatted as an UUID. Do not use this to generate proper UUID!
 */
static String createNewInstallationId(void) {
  static bool randInitialized = false;
  char buff[40];

  if (!randInitialized) {
     struct timeval tm;
     gettimeofday(&tm, NULL);
     // Terrible choice for initialization, prone to collision
     srand((unsigned)tm.tv_sec + (unsigned)tm.tv_usec);
     randInitialized = true;
  }

  snprintf(buff, sizeof(buff),
    "%1x%1x%1x%1x%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x",
    rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16
  );
  return String(buff);
}

static void sendAndEchoToSerial(WiFiClient& client, const char *line) {
  client.println(line);
  if (Serial && DEBUG)
    Serial.println(line);
}


ParseClient::ParseClient() {
}

void ParseClient::begin(const char *applicationId, const char *clientKey) {
  if (Serial && DEBUG) {
    Serial.print("begin(");
    Serial.print(applicationId ? applicationId : "NULL");
    Serial.print(", ");
    Serial.print(clientKey ? clientKey : "NULL");
    Serial.println(")");
  }

  if(applicationId) {
    this->applicationId = String(applicationId);
  }
  if (clientKey) {
    this->clientKey = String(clientKey);
  }
}

void ParseClient::setInstallationId(const char *installationId) {
  if (installationId) {
      Serial.println(clientKey);
      Serial.println(this->installationId);
      this->installationId = String(installationId);
      Serial.println(clientKey);
      Serial.println(this->installationId);
  } else {
    this->installationId = "";
  }
}

const char* ParseClient::getInstallationId() {
  if (!installationId.length()) {
    installationId = createNewInstallationId();
    char buff[40];

    if (Serial && DEBUG) {
      Serial.print("creating new installationId:");
      Serial.println(installationId.c_str());
    }

    char content[120];
    snprintf(content, sizeof(content), "{\"installationId\": \"%s\", \"deviceType\": \"embedded\", \"parseVersion\": \"1.0.0\"}", installationId.c_str());

    ParseResponse response = sendRequest("POST", "/1/installations", content, "");
    if (Serial && DEBUG) {
      Serial.print("response:");
      Serial.println(response.getJSONBody());
    }
  }
  return installationId.c_str();
}

void ParseClient::setSessionToken(const char *sessionToken) {
  if ((sessionToken != NULL) && (strlen(sessionToken) > 0 )) {
    this->sessionToken = sessionToken;
    if (Serial && DEBUG) {
      Serial.print("setting the session for installation:");
      Serial.println(installationId.c_str());
    }
    getInstallationId();
    ParseResponse response = sendRequest("GET", "/1/sessions/me", "", "");
    String installation = response.getString("installationId");
    if (!installation.length() && installationId.length()) {
      sendRequest("PUT", "/1/sessions/me", "{}\r\n", "");
    }
  } else {
   this->sessionToken = "";
  }
}

void ParseClient::clearSessionToken() {
  setSessionToken(NULL);
}

const char* ParseClient::getSessionToken() {
  if (!sessionToken.length())
    return NULL;
  return sessionToken.c_str();
}

ParseResponse ParseClient::sendRequest(const char* httpVerb, const char* httpPath, const char* requestBody, const char* urlParams) {
  return sendRequest(String(httpVerb), String(httpPath), String(requestBody), String(urlParams));
}

ParseResponse ParseClient::sendRequest(const String& httpVerb, const String& httpPath, const String& requestBody, const String& urlParams) {
  char buff[91] = {0};
  client.stop();
  if (Serial && DEBUG) {
    Serial.print("sendRequest(\"");
    Serial.print(httpVerb.c_str());
    Serial.print("\", \"");
    Serial.print(httpPath.c_str());
    Serial.print("\", \"");
    Serial.print(requestBody.c_str());
    Serial.print("\", \"");
    Serial.print(urlParams.c_str());
    Serial.println("\")");
  }
  if (client.connectSSL(PARSE_API, SSL_PORT)) {
    if (Serial && DEBUG) {
      Serial.println("connected to server");
      Serial.println(applicationId);
      Serial.println(clientKey);
      Serial.println(installationId);
    }
    // Make a HTTP request:
    snprintf(buff, sizeof(buff) - 1, "%s %s HTTP/1.1", httpVerb.c_str(), httpPath.c_str());
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "Host: %s",  PARSE_API);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "User-Agent: %s", USER_AGENT);
    // client.println("X-Parse-OS-Version: %s", PARSE_OS);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Client-Version: %s", CLIENT_VERSION);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Application-Id: %s", applicationId.c_str());
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Client-Key: %s", clientKey.c_str());
    sendAndEchoToSerial(client, buff);
    if (Serial && DEBUG)
      Serial.println("buff");
    if (installationId.length() > 0) {
      snprintf(buff, sizeof(buff) - 1, "X-Parse-Installation-Id: %s", installationId.c_str());
      sendAndEchoToSerial(client, buff);
    }
    if (sessionToken.length() > 0) {
      snprintf(buff, sizeof(buff) - 1, "X-Parse-Session-Token: %s", sessionToken.c_str());
      sendAndEchoToSerial(client, buff);
    }
    String payload;
    if (requestBody.length() > 0) {
      payload = requestBody;
      sendAndEchoToSerial(client, "Content-Type: application/json; charset=utf-8");
    } else if (urlParams.length() > 0) {
      payload = urlParams;
      sendAndEchoToSerial(client, "Content-Type: html/text");
    }
    if (payload.length() > 0) {
      snprintf(buff, sizeof(buff) - 1, "Content-Length: %d", payload.length());
      sendAndEchoToSerial(client, buff);
    }
    sendAndEchoToSerial(client, "Connection: close");
    sendAndEchoToSerial(client, "");
    if (payload.length() > 0) {
      sendAndEchoToSerial(client, payload.c_str());
    }
  } else {
    if (Serial && DEBUG)
      Serial.println("failed to connect to server");
  }
  ParseResponse response(&client);
  return response;
}

bool ParseClient::startPushService() {
  pushClient.stop();
  if (Serial && DEBUG)
    Serial.println("start push");
  if (pushClient.connectSSL(PARSE_PUSH, SSL_PORT)) {
    if (Serial && DEBUG)
      Serial.println("push started");
    char buff[80] = {0};
    snprintf(buff, sizeof(buff),
       "{\"installation_id\":\"%s\", \"oauth_key\":\"%s\", \"v\": \"e1.0.0\", \"last\": null}",
       installationId.c_str(),
       applicationId.c_str());
    sendAndEchoToSerial(pushClient, buff);
  } else {
    if (Serial && DEBUG)
      Serial.println("failed to connect to push server");
  }
}

ParsePush ParseClient::nextPush() {
  ParsePush push(&client);
  return push;
}

bool ParseClient::pushAvailable() {
  bool available = (pushClient.available() > 0);
  // send keepalive if connected.
  if (!available && pushClient.connected())
    pushClient.println("{}");
  return available;
}

void ParseClient::stopPushService() {
  pushClient.stop();
}

void ParseClient::end() {
  stopPushService();
}

ParseClient Parse;
