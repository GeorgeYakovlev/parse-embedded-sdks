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
#include "ParseResponse.h"
#include "ParseUtils.h"
#include <WiFi101.h>

ParseResponse::ParseResponse(WiFiClient* client) {
  buf = NULL;
  tmpBuf = NULL;
  p = 0;
  resultCount = -1;
  bufSize = 0;
  isUserBuffer = false;
  this->client = client;
}

void ParseResponse::setBuffer(char* buffer, int size) {
  if (!buffer || size <= 0) {
    return;
  }

  buf = buffer;
  bufSize = size;
  isUserBuffer = true;
  memset(buf, 0, bufSize);
}

int ParseResponse::available() {
  return client->available();
}

void ParseResponse::read() {
  if (buf == NULL) {
    bufSize = BUFSIZE;
    buf = new char[bufSize];
    memset(buf, 0, bufSize);
  }

  if (p == bufSize - 1) {
    return;
  }

  memset(buf + p, 0, bufSize - p);

  while (p < bufSize-1 && available() > 0) {
    *(buf + p) = client->read();
    //client.read((uint8_t *)(buf + p), bufSize - p - 1);
    //p = strlen(buf);
  }

  while(available() > 0) {
    char tmp[64];
    client->read((uint8_t *)tmp, sizeof(tmp)); //discard exccessive data which buffer cannot contain
  }
}

int ParseResponse::getErrorCode() {
  return getInt("code");
}

const char* ParseResponse::getJSONBody() {
  read();
  return buf;
}

const char* ParseResponse::getString(const char* key) {
  read();
  if (!tmpBuf) {
    tmpBuf = new char[64];
  }
  memset(tmpBuf, 0, 64);
  ParseUtils::getStringFromJSON(buf, key, tmpBuf, 64);
  return tmpBuf;
}

int ParseResponse::getInt(const char* key) {
  read();
  return ParseUtils::getIntFromJSON(buf, key);
}

double ParseResponse::getDouble(const char* key) {
  read();
  return ParseUtils::getFloatFromJSON(buf, key);
}

bool ParseResponse::getBoolean(const char* key) {
  read();
  return ParseUtils::getBooleanFromJSON(buf, key);
}

void ParseResponse::readWithTimeout(int maxSec) {
  while((!available()) && (maxSec--)) { // wait till response
    delay(1000);
  }
  read();
}

bool ParseResponse::nextObject() {
  if(resultCount <= 0) {
    count();
  }

  if(resultCount <= 0) {
    return false;
  }

  client->print('n');
  // reset buffer and read next object
  p = 0;
  memset(buf, 0, bufSize);
  readWithTimeout(5);

  if (*buf) {
    return true;
  } else {
    return false;
  }
}

int ParseResponse::count() {
  client->print('c');

  // reset buffer and read count
  p = 0;
  memset(buf, 0, bufSize);
  readWithTimeout(30);
  read();

  String c = buf;
  int count = c.toInt();
  resultCount = count;
  return count;
}

void ParseResponse::freeBuffer() {
  if (!isUserBuffer) { // only free non-user buffer
    delete[] buf;
    buf = NULL;
  }
  if (tmpBuf) {
    delete[] tmpBuf;
    tmpBuf = NULL;
  }
}

void ParseResponse::close() {
  freeBuffer();
}
