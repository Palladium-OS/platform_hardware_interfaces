/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <JsonConfigLoader.h>

#include <AccessForVehicleProperty.h>
#include <ChangeModeForVehicleProperty.h>
#include <PropertyUtils.h>

#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES
#include <TestPropertyUtils.h>
#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES

#include <android-base/strings.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace jsonconfigloader_impl {

using ::aidl::android::hardware::automotive::vehicle::AccessForVehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::ChangeModeForVehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::EvConnectorType;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceState;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceType;
using ::aidl::android::hardware::automotive::vehicle::FuelType;
using ::aidl::android::hardware::automotive::vehicle::GsrComplianceRequirementType;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaMirror;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehicleGear;
using ::aidl::android::hardware::automotive::vehicle::VehicleHvacFanDirection;
using ::aidl::android::hardware::automotive::vehicle::VehicleIgnitionState;
using ::aidl::android::hardware::automotive::vehicle::VehicleOilLevel;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehicleSeatOccupancyState;
using ::aidl::android::hardware::automotive::vehicle::VehicleTurnSignal;
using ::aidl::android::hardware::automotive::vehicle::VehicleUnit;
using ::aidl::android::hardware::automotive::vehicle::VehicleVendorPermission;

using ::android::base::Error;
using ::android::base::Result;

// Defines a map from constant names to constant values, the values defined here corresponds to
// the "Constants::XXXX" used in JSON config file.
const std::unordered_map<std::string, int> CONSTANTS_BY_NAME = {
        {"DOOR_1_RIGHT", DOOR_1_RIGHT},
        {"DOOR_1_LEFT", DOOR_1_LEFT},
        {"DOOR_2_RIGHT", DOOR_2_RIGHT},
        {"DOOR_2_LEFT", DOOR_2_LEFT},
        {"DOOR_REAR", DOOR_REAR},
        {"HVAC_ALL", HVAC_ALL},
        {"HVAC_LEFT", HVAC_LEFT},
        {"HVAC_RIGHT", HVAC_RIGHT},
        {"VENDOR_EXTENSION_INT_PROPERTY", VENDOR_EXTENSION_INT_PROPERTY},
        {"VENDOR_EXTENSION_BOOLEAN_PROPERTY", VENDOR_EXTENSION_BOOLEAN_PROPERTY},
        {"VENDOR_EXTENSION_STRING_PROPERTY", VENDOR_EXTENSION_STRING_PROPERTY},
        {"VENDOR_EXTENSION_FLOAT_PROPERTY", VENDOR_EXTENSION_FLOAT_PROPERTY},
        {"WINDOW_1_LEFT", WINDOW_1_LEFT},
        {"WINDOW_1_RIGHT", WINDOW_1_RIGHT},
        {"WINDOW_2_LEFT", WINDOW_2_LEFT},
        {"WINDOW_2_RIGHT", WINDOW_2_RIGHT},
        {"WINDOW_ROOF_TOP_1", WINDOW_ROOF_TOP_1},
        {"WINDOW_1_RIGHT_2_LEFT_2_RIGHT", WINDOW_1_RIGHT | WINDOW_2_LEFT | WINDOW_2_RIGHT},
        {"SEAT_1_LEFT", SEAT_1_LEFT},
        {"SEAT_1_RIGHT", SEAT_1_RIGHT},
        {"SEAT_2_LEFT", SEAT_2_LEFT},
        {"SEAT_2_RIGHT", SEAT_2_RIGHT},
        {"SEAT_2_CENTER", SEAT_2_CENTER},
        {"WHEEL_REAR_RIGHT", WHEEL_REAR_RIGHT},
        {"WHEEL_REAR_LEFT", WHEEL_REAR_LEFT},
        {"WHEEL_FRONT_RIGHT", WHEEL_FRONT_RIGHT},
        {"WHEEL_FRONT_LEFT", WHEEL_FRONT_LEFT},
        {"CHARGE_PORT_FRONT_LEFT", CHARGE_PORT_FRONT_LEFT},
        {"CHARGE_PORT_REAR_LEFT", CHARGE_PORT_REAR_LEFT},
        {"FAN_DIRECTION_FLOOR", FAN_DIRECTION_FLOOR},
        {"FAN_DIRECTION_FACE", FAN_DIRECTION_FACE},
        {"FAN_DIRECTION_DEFROST", FAN_DIRECTION_DEFROST},
        {"FAN_DIRECTION_FACE_FLOOR", FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR},
        {"FAN_DIRECTION_FACE_DEFROST", FAN_DIRECTION_FACE | FAN_DIRECTION_DEFROST},
        {"FAN_DIRECTION_FLOOR_DEFROST", FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST},
        {"FAN_DIRECTION_FLOOR_DEFROST_FACE",
         FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST | FAN_DIRECTION_FACE},
        {"FUEL_DOOR_REAR_LEFT", FUEL_DOOR_REAR_LEFT},
        {"LIGHT_STATE_ON", LIGHT_STATE_ON},
        {"LIGHT_SWITCH_AUTO", LIGHT_SWITCH_AUTO},
        {"MIRROR_DRIVER_LEFT_RIGHT",
         toInt(VehicleAreaMirror::DRIVER_LEFT) | toInt(VehicleAreaMirror::DRIVER_RIGHT)},
#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES
        // Following are test properties:
        {"ECHO_REVERSE_BYTES", ECHO_REVERSE_BYTES},
        {"kMixedTypePropertyForTest", kMixedTypePropertyForTest},
        {"VENDOR_CLUSTER_NAVIGATION_STATE", VENDOR_CLUSTER_NAVIGATION_STATE},
        {"VENDOR_CLUSTER_REQUEST_DISPLAY", VENDOR_CLUSTER_REQUEST_DISPLAY},
        {"VENDOR_CLUSTER_SWITCH_UI", VENDOR_CLUSTER_SWITCH_UI},
        {"VENDOR_CLUSTER_DISPLAY_STATE", VENDOR_CLUSTER_DISPLAY_STATE},
        {"VENDOR_CLUSTER_REPORT_STATE", VENDOR_CLUSTER_REPORT_STATE},
        {"PLACEHOLDER_PROPERTY_INT", PLACEHOLDER_PROPERTY_INT},
        {"PLACEHOLDER_PROPERTY_FLOAT", PLACEHOLDER_PROPERTY_FLOAT},
        {"PLACEHOLDER_PROPERTY_BOOLEAN", PLACEHOLDER_PROPERTY_BOOLEAN},
        {"PLACEHOLDER_PROPERTY_STRING", PLACEHOLDER_PROPERTY_STRING}
#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES
};

// A class to parse constant values for type T.
template <class T>
class ConstantParser final : public ConstantParserInterface {
  public:
    ConstantParser() {
        for (const T& v : ndk::enum_range<T>()) {
            std::string name = aidl::android::hardware::automotive::vehicle::toString(v);
            // We use the same constant for both VehicleUnit::GALLON and VehicleUnit::US_GALLON,
            // which caused toString() not work properly for US_GALLON. So we explicitly add the
            // map here.
            if (name == "GALLON") {
                mValueByName["US_GALLON"] = toInt(v);
            }
            mValueByName[name] = toInt(v);
        }
    }

    ~ConstantParser() = default;

    Result<int> parseValue(const std::string& name) const override {
        auto it = mValueByName.find(name);
        if (it == mValueByName.end()) {
            return Error() << "Constant name: " << name << " is not defined";
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, int> mValueByName;
};

// A class to parse constant values defined in CONSTANTS_BY_NAME map.
class LocalVariableParser final : public ConstantParserInterface {
  public:
    ~LocalVariableParser() = default;

    Result<int> parseValue(const std::string& name) const override {
        auto constantsIt = CONSTANTS_BY_NAME.find(name);
        if (constantsIt == CONSTANTS_BY_NAME.end()) {
            return Error() << "Constant variable name: " << name << " is not defined";
        }
        return constantsIt->second;
    }
};

JsonValueParser::JsonValueParser() {
    mConstantParsersByType["VehiclePropertyAccess"] =
            std::make_unique<ConstantParser<VehiclePropertyAccess>>();
    mConstantParsersByType["VehiclePropertyChangeMode"] =
            std::make_unique<ConstantParser<VehiclePropertyChangeMode>>();
    mConstantParsersByType["VehicleGear"] = std::make_unique<ConstantParser<VehicleGear>>();
    mConstantParsersByType["VehicleAreaWindow"] =
            std::make_unique<ConstantParser<VehicleAreaWindow>>();
    mConstantParsersByType["VehicleAreaMirror"] =
            std::make_unique<ConstantParser<VehicleAreaMirror>>();
    mConstantParsersByType["VehicleOilLevel"] = std::make_unique<ConstantParser<VehicleOilLevel>>();
    mConstantParsersByType["VehicleUnit"] = std::make_unique<ConstantParser<VehicleUnit>>();
    mConstantParsersByType["VehicleSeatOccupancyState"] =
            std::make_unique<ConstantParser<VehicleSeatOccupancyState>>();
    mConstantParsersByType["VehicleHvacFanDirection"] =
            std::make_unique<ConstantParser<VehicleHvacFanDirection>>();
    mConstantParsersByType["VehicleApPowerStateReport"] =
            std::make_unique<ConstantParser<VehicleApPowerStateReport>>();
    mConstantParsersByType["VehicleTurnSignal"] =
            std::make_unique<ConstantParser<VehicleTurnSignal>>();
    mConstantParsersByType["VehicleVendorPermission"] =
            std::make_unique<ConstantParser<VehicleVendorPermission>>();
    mConstantParsersByType["EvsServiceType"] = std::make_unique<ConstantParser<EvsServiceType>>();
    mConstantParsersByType["EvsServiceState"] = std::make_unique<ConstantParser<EvsServiceState>>();
    mConstantParsersByType["EvConnectorType"] = std::make_unique<ConstantParser<EvConnectorType>>();
    mConstantParsersByType["VehicleProperty"] = std::make_unique<ConstantParser<VehicleProperty>>();
    mConstantParsersByType["GsrComplianceRequirementType"] =
            std::make_unique<ConstantParser<GsrComplianceRequirementType>>();
    mConstantParsersByType["VehicleIgnitionState"] =
            std::make_unique<ConstantParser<VehicleIgnitionState>>();
    mConstantParsersByType["FuelType"] = std::make_unique<ConstantParser<FuelType>>();
    mConstantParsersByType["Constants"] = std::make_unique<LocalVariableParser>();
}

template <>
Result<int32_t> JsonValueParser::convertValueToType<int32_t>(const std::string& fieldName,
                                                             const Json::Value& value) {
    if (!value.isInt()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect int";
    }
    return static_cast<int32_t>(value.asInt());
}

template <>
Result<float> JsonValueParser::convertValueToType<float>(const std::string& fieldName,
                                                         const Json::Value& value) {
    // isFloat value does not exist, so we use isDouble here.
    if (!value.isDouble()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect float";
    }
    return value.asFloat();
}

template <>
Result<int64_t> JsonValueParser::convertValueToType<int64_t>(const std::string& fieldName,
                                                             const Json::Value& value) {
    if (!value.isInt64()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect int64";
    }
    return static_cast<int64_t>(value.asInt64());
}

template <>
Result<std::string> JsonValueParser::convertValueToType<std::string>(const std::string& fieldName,
                                                                     const Json::Value& value) {
    if (!value.isString()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect string";
    }
    return value.asString();
}

Result<std::string> JsonValueParser::parseStringValue(const std::string& fieldName,
                                                      const Json::Value& value) const {
    return convertValueToType<std::string>(fieldName, value);
}

template <class T>
Result<T> JsonValueParser::parseValue(const std::string& fieldName,
                                      const Json::Value& value) const {
    if (!value.isString()) {
        return convertValueToType<T>(fieldName, value);
    }
    auto maybeTypeAndValue = maybeGetTypeAndValueName(value.asString());
    if (!maybeTypeAndValue.has_value()) {
        return Error() << "Invalid constant value: " << value << " for field: " << fieldName;
    }
    auto constantParseResult = parseConstantValue(maybeTypeAndValue.value());
    if (!constantParseResult.ok()) {
        return constantParseResult.error();
    }
    int constantValue = constantParseResult.value();
    return static_cast<T>(constantValue);
}

template <>
Result<std::string> JsonValueParser::parseValue<std::string>(const std::string& fieldName,
                                                             const Json::Value& value) const {
    return parseStringValue(fieldName, value);
}

template <class T>
Result<std::vector<T>> JsonValueParser::parseArray(const std::string& fieldName,
                                                   const Json::Value& value) const {
    if (!value.isArray()) {
        return Error() << "The value: " << value << " for field: " << fieldName
                       << " is not in correct type, expect array";
    }
    std::vector<T> parsedValues;
    for (unsigned int i = 0; i < value.size(); i++) {
        auto result = parseValue<T>(fieldName, value[i]);
        if (!result.ok()) {
            return result.error();
        }
        parsedValues.push_back(result.value());
    }
    return std::move(parsedValues);
}

std::optional<std::pair<std::string, std::string>> JsonValueParser::maybeGetTypeAndValueName(
        const std::string& jsonFieldValue) const {
    size_t pos = jsonFieldValue.find(DELIMITER);
    if (pos == std::string::npos) {
        return {};
    }
    std::string type = jsonFieldValue.substr(0, pos);
    std::string valueName = jsonFieldValue.substr(pos + DELIMITER.length(), std::string::npos);
    if (type != "Constants" && mConstantParsersByType.find(type) == mConstantParsersByType.end()) {
        return {};
    }
    return std::make_pair(type, valueName);
}

Result<int> JsonValueParser::parseConstantValue(
        const std::pair<std::string, std::string>& typeValueName) const {
    const std::string& type = typeValueName.first;
    const std::string& valueName = typeValueName.second;
    auto it = mConstantParsersByType.find(type);
    if (it == mConstantParsersByType.end()) {
        return Error() << "Unrecognized type: " << type;
    }
    auto result = it->second->parseValue(valueName);
    if (!result.ok()) {
        return Error() << type << "::" << valueName << " undefined";
    }
    return result;
}

template <class T>
bool JsonConfigParser::tryParseJsonValueToVariable(const Json::Value& parentJsonNode,
                                                   const std::string& fieldName,
                                                   bool fieldIsOptional, T* outPtr,
                                                   std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return false;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        if (!fieldIsOptional) {
            errors->push_back("Missing required field: " + fieldName +
                              " in node: " + parentJsonNode.toStyledString());
            return false;
        }
        return true;
    }
    auto result = mValueParser.parseValue<T>(fieldName, parentJsonNode[fieldName]);
    if (!result.ok()) {
        errors->push_back(result.error().message());
        return false;
    }
    *outPtr = std::move(result.value());
    return true;
}

template <class T>
bool JsonConfigParser::tryParseJsonArrayToVariable(const Json::Value& parentJsonNode,
                                                   const std::string& fieldName,
                                                   bool fieldIsOptional, std::vector<T>* outPtr,
                                                   std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return false;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        if (!fieldIsOptional) {
            errors->push_back("Missing required field: " + fieldName +
                              " in node: " + parentJsonNode.toStyledString());
            return false;
        }
        return true;
    }
    auto result = mValueParser.parseArray<T>(fieldName, parentJsonNode[fieldName]);
    if (!result.ok()) {
        errors->push_back(result.error().message());
        return false;
    }
    *outPtr = std::move(result.value());
    return true;
}

template <class T>
void JsonConfigParser::parseAccessChangeMode(
        const Json::Value& parentJsonNode, const std::string& fieldName, int propId,
        const std::string& propStr, const std::unordered_map<VehicleProperty, T>& defaultMap,
        T* outPtr, std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return;
    }
    if (parentJsonNode.isMember(fieldName)) {
        auto result = mValueParser.parseValue<int32_t>(fieldName, parentJsonNode[fieldName]);
        if (!result.ok()) {
            errors->push_back(result.error().message());
            return;
        }
        *outPtr = static_cast<T>(result.value());
        return;
    }
    auto it = defaultMap.find(static_cast<VehicleProperty>(propId));
    if (it == defaultMap.end()) {
        errors->push_back("No " + fieldName + " specified for property: " + propStr);
        return;
    }
    *outPtr = it->second;
    return;
}

void JsonConfigParser::parsePropValues(const Json::Value& parentJsonNode,
                                       const std::string& fieldName, RawPropValues* outPtr,
                                       std::vector<std::string>* errors) {
    if (!parentJsonNode.isObject()) {
        errors->push_back("Node: " + parentJsonNode.toStyledString() + " is not an object");
        return;
    }
    if (!parentJsonNode.isMember(fieldName)) {
        return;
    }
    const Json::Value& jsonValue = parentJsonNode[fieldName];
    tryParseJsonArrayToVariable(jsonValue, "int32Values", /*optional=*/true, &(outPtr->int32Values),
                                errors);
    tryParseJsonArrayToVariable(jsonValue, "floatValues", /*optional=*/true, &(outPtr->floatValues),
                                errors);
    tryParseJsonArrayToVariable(jsonValue, "int64Values", /*optional=*/true, &(outPtr->int64Values),
                                errors);
    // We don't support "byteValues" yet.
    tryParseJsonValueToVariable(jsonValue, "stringValue", /*optional=*/true, &(outPtr->stringValue),
                                errors);
}

std::optional<ConfigDeclaration> JsonConfigParser::parseEachProperty(
        const Json::Value& propJsonValue, std::vector<std::string>* errors) {
    size_t initialErrorCount = errors->size();
    ConfigDeclaration configDecl = {};
    int32_t propId;

    if (!tryParseJsonValueToVariable(propJsonValue, "property", /*optional=*/false, &propId,
                                     errors)) {
        return std::nullopt;
    }

    configDecl.config.prop = propId;
    std::string propStr = propJsonValue["property"].toStyledString();

    parseAccessChangeMode(propJsonValue, "access", propId, propStr, AccessForVehicleProperty,
                          &configDecl.config.access, errors);

    parseAccessChangeMode(propJsonValue, "changeMode", propId, propStr,
                          ChangeModeForVehicleProperty, &configDecl.config.changeMode, errors);

    tryParseJsonValueToVariable(propJsonValue, "configString", /*optional=*/true,
                                &configDecl.config.configString, errors);

    tryParseJsonArrayToVariable(propJsonValue, "configArray", /*optional=*/true,
                                &configDecl.config.configArray, errors);

    parsePropValues(propJsonValue, "defaultValue", /*optional=*/true, &configDecl.initialValue,
                    errors);

    tryParseJsonValueToVariable(propJsonValue, "minSampleRate", /*optional=*/true,
                                &configDecl.config.minSampleRate, errors);

    tryParseJsonValueToVariable(propJsonValue, "maxSampleRate", /*optional=*/true,
                                &configDecl.config.maxSampleRate, errors);

    // TODO(b/238685398): AreaConfigs

    if (errors->size() != initialErrorCount) {
        return std::nullopt;
    }
    return configDecl;
}

Result<std::vector<ConfigDeclaration>> JsonConfigParser::parseJsonConfig(std::istream& is) {
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::vector<ConfigDeclaration> configs;
    std::string errs;
    if (!Json::parseFromStream(builder, is, &root, &errs)) {
        return Error() << "Failed to parse property config file as JSON, error: " << errs;
    }
    Json::Value properties = root["properties"];
    std::vector<std::string> errors;
    for (unsigned int i = 0; i < properties.size(); i++) {
        if (auto maybeConfig = parseEachProperty(properties[i], &errors); maybeConfig.has_value()) {
            configs.push_back(std::move(maybeConfig.value()));
        }
    }
    if (!errors.empty()) {
        return Error() << android::base::Join(errors, '\n');
    }
    return configs;
}

}  // namespace jsonconfigloader_impl

JsonConfigLoader::JsonConfigLoader() {
    mParser = std::make_unique<jsonconfigloader_impl::JsonConfigParser>();
}

android::base::Result<std::vector<ConfigDeclaration>> JsonConfigLoader::loadPropConfig(
        std::istream& is) {
    return mParser->parseJsonConfig(is);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
