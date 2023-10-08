/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include <memory> // For std::shared_ptr
#include <mosquitto.h>
#include <string>
#include <vector>

class DeviceBase;

/**
 * @brief Class for connecting to an MQTT server and registering devices to
 * listen for their MQTT topics
 *
 * @note This class is not thread-safe, so it should only be used from one
 * thread
 */

class MQTTConnector : public std::enable_shared_from_this<MQTTConnector> {
public:
  /**
   * @brief Construct a new MQTTConnector object
   *
   * @param server The MQTT server to connect to
   * @param port The port to use when connecting to the MQTT server
   * @param username The username to use when connecting to the MQTT server
   * @param password The password to use when connecting to the MQTT server
   */
  MQTTConnector(const std::string &server, const int port,
                const std::string &username, const std::string &password);

  /**
   * @brief Connect to the MQTT server
   */
  bool connect();

  /**
   * @brief Disconnect from the MQTT server
   */
  void disconnect();

  /**
   * @brief Check if connected to the MQTT server
   *
   * @return true if connected to the MQTT server, false otherwise
   */
  bool isConnected() const;

  /**
   * @brief Register a device to listen for its MQTT topics
   *
   * @param device The device to register
   */
  void registerDevice(std::shared_ptr<DeviceBase> device);

  /**
   * @brief Process incoming MQTT messages. Needs to be called regularly with a
   * timeout
   *
   * @note This method should be called in the main loop
   *
   * @param timeout The timeout in milliseconds
   */
  void processMessages(int timeout);

  /**
   * @brief Send a message to the MQTT server
   *
   * @param topic The topic to publish to
   * @param payload The payload to publish
   */
  void publishMessage(const std::string &topic, const std::string &payload);

private:
  /**
   * @brief Callback for incoming MQTT messages, implementing the on_message
   *
   * @param mosq The mosquitto instance
   * @param obj The user data
   * @param message The message
   */
  static void messageCallback(mosquitto *mosq, void *obj,
                              const mosquitto_message *message);

  /**
   * @brief Callback for successful connection to the MQTT server, implementing
   * on_connect
   *
   * @param mosq The mosquitto instance
   * @param obj The user data
   * @param rc The connection result
   */
  static void connectCallback(mosquitto *mosq, void *obj, int rc);

  /**
   * @brief Callback for disconnection from the MQTT server, implementing
   * on_disconnect
   *
   * @param mosq The mosquitto instance
   * @param obj The user data
   * @param rc The disconnection result
   */
  static void disconnectCallback(mosquitto *mosq, void *obj, int rc);

  /**
   * @brief Callback for successful subscription to an MQTT topic, implementing
   * on_subscribe
   *
   * @param mosq The mosquitto instance
   * @param obj The user data
   * @param mid The message ID
   * @param qos_count The number of granted subscriptions
   * @param granted_qos The granted QoS levels
   */
  static void subscribeCallback(mosquitto *mosq, void *obj, int mid,
                                int qos_count, const int *granted_qos);

  /**
   * @brief Callback for unsuccessful subscription to an MQTT topic,
   * implementing on_unsubscribe
   *
   * @param mosq The mosquitto instance
   * @param obj The user data
   * @param mid The message ID
   */
  static void unsubscribeCallback(mosquitto *mosq, void *obj, int mid);

  std::string m_server;
  int m_port;
  std::string m_username;
  std::string m_password;
  bool m_is_connected = false;
  std::vector<std::shared_ptr<DeviceBase>>
      m_registered_devices; // List of registered devices using smart pointers
  mosquitto *m_mosquitto;
};
