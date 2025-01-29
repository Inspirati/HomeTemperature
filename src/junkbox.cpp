
/*
Number of sensors found: 5
Sensor 0: 28FFC03D041503D0
Sensor 1: 28FF58E602150243
Sensor 2: 28FFB468041503FB
Sensor 3: 28FF0ED602150202
Sensor 4: 28FF37E3021502BC
*/

/*
  char dest[17];
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    //formatSensorAddress(sensorAddresses[i], dest, sizeof(dest));
    format1WireAddress(sensorAddresses[i], dest, sizeof(dest));
    doc["sensor"] = "ds18b20";
    doc["index"] = i;
    doc["time"] = _get_unix_time();
    doc["mac"] = get_macAddress();
    doc["addr"] = dest;
    JsonArray data = doc["data"].to<JsonArray>();
    doc["data"][1] = results[i];
    doc["data"][0] = offsets[i];
    serializeJson(doc, json);
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_JSON, 1, true, json.c_str());
    if (verbose) {
      Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_JSON, packetIdPub1);
      //serializeJson(doc, Serial);
      serializeJsonPretty(doc, Serial);
    }
    Serial.println();
  }
*/

// sprintf(buffer, "%02X:%02X:%02X:%02X", buf[0], buf[1], buf[2], buf[3]);
/*
int i;
char* buf2 = stringbuf;
char* endofbuf = stringbuf + sizeof(stringbuf);
for (i = 0; i < x; i++) {
    // i use 5 here since we are going to add at most 3 chars, need a space for the end '\n' and need a null terminator
    if (buf2 + 5 < endofbuf) {
        if (i > 0) {
            buf2 += sprintf(buf2, ":");
        }
        buf2 += sprintf(buf2, "%02X", buf[i]);
    }
}
buf2 += sprintf(buf2, "\n");
 */
/*
// This is a minimal workaround for compiler version with missing std::byte type, aka pre C++17
namespace std {
enum class byte : unsigned char {};
}
template <size_t byteCount>
std::string BytesArrayToHexString( const std::array<byte, 8>& src )
{
  static const char table[] = "0123456789ABCDEF";
  std::array<char, 2 * byteCount + 1> dst;
  const byte* srcPtr = &src[0];
  char* dstPtr = &dst[0];

  for (auto count = byteCount; count > 0; --count)
  {
      unsigned char c = *srcPtr++;
      *dstPtr++ = table[c >> 4];
      *dstPtr++ = table[c & 0x0f];
  }
  *dstPtr = 0;
  return &dst[0];
}
 */
/*
//#include <sstream>
//std::string BytesArrayToHexString(const std::array<byte, 8>& src);
//std::vector<unsigned char> stl_string_to_binary(const std::string& source);
//std::string stl_binary_to_string(const std::vector<unsigned char>& source);
std::vector<unsigned char> stl_string_to_binary(const std::string& source)
{
    static int nibbles[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
    std::vector<unsigned char> retval;
    for (std::string::const_iterator it = source.begin(); it < source.end(); it += 2) {
        unsigned char v = 0;
        if (std::isxdigit(*it))
            v = nibbles[std::toupper(*it) - '0'] << 4;
        if (it + 1 < source.end() && std::isxdigit(*(it + 1)))
            v += nibbles[std::toupper(*(it + 1)) - '0'];
        retval.push_back(v);
    }
    return retval;
}
std::string stl_binary_to_string(const std::vector<unsigned char>& source)
{
    static char syms[] = "0123456789ABCDEF";
    std::stringstream str_stre;
    for (std::vector<unsigned char>::const_iterator it = source.begin(); it != source.end(); it++)
        str_stre << syms[((*it >> 4) & 0xf)] << syms[*it & 0xf];

    return str_stre.str();
}
 */
/*
//    for (int j = 0; j < 8; j++) {
//      sprintf(&dest[0] + j*2, "%02x", sensorAddresses[i][j]);
//    }
//    dest[16] = '\0';
//    String strDest;
//    binary_to_string(sensorAddresses[i], 16, strDest);
//    std::string addr = BytesArrayToHexString(reinterpret_cast<const std::array<std::byte, 8>&>(sensorAddresses[i]));
//std::string addr = binary_to_string(const std::vector<unsigned char>& source);
//  std::string addr = stl_binary_to_string(reinterpret_cast<const std::vector<unsigned char>&>(sensorAddresses[i]));
    // const std::array<std::byte, 8>& arrayRef = reinterpret_cast<const std::array<std::byte, 8>&>(data);
//std::string BytesArrayToHexString( const std::array<byte, byteCount>& src )
//void binary_to_string(const unsigned char* source, unsigned int length, String& destination)
 */
