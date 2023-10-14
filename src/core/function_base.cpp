/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

// Include the corresponding header file
#include "hass_mqtt_device/core/function_base.h"

// Include any other necessary headers
#include "hass_mqtt_device/logger/logger.hpp" // For logging
#include <vector>

FunctionBase::FunctionBase(const std::string &functionName)
    : m_functionName(functionName) {}

std::string FunctionBase::getName() const { return m_functionName; }

std::string FunctionBase::getId() const {
  auto parent = m_parentDevice.lock();
  if (!parent) {
    LOG_ERROR("Parent device is not available.");
    return "";
  }
  return parent->getFullId() + "_" + getName();
}

std::string FunctionBase::getBaseTopic() const {
  auto parent = m_parentDevice.lock();
  if (!parent) {
    LOG_ERROR("Parent device is not available.");
    return "";
  }
  return "home/" + parent->getFullId() + "/light/" + getName() + "/";
};
