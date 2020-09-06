/*
 * Copyright (C) 2020 The Android Open Source Project
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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/powerstats/IPowerStats.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::powerstats::EnergyData;
using aidl::android::hardware::powerstats::IPowerStats;
using aidl::android::hardware::powerstats::PowerEntityInfo;
using aidl::android::hardware::powerstats::PowerEntityStateResidencyResult;
using aidl::android::hardware::powerstats::RailInfo;
using ndk::SpAIBinder;

class PowerStatsAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        powerstats = IPowerStats::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(nullptr, powerstats.get());
    }

    std::shared_ptr<IPowerStats> powerstats;
};

TEST_P(PowerStatsAidl, TestGetEnergyData) {
    std::vector<EnergyData> data;
    ASSERT_TRUE(powerstats->getEnergyData({}, &data).isOk());
}

// Each PowerEntity must have a valid name
TEST_P(PowerStatsAidl, ValidatePowerEntityNames) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        EXPECT_NE(info.powerEntityName, "");
    }
}

// Each power entity must have a unique name
TEST_P(PowerStatsAidl, ValidatePowerEntityUniqueNames) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    std::set<std::string> names;
    for (auto info : infos) {
        EXPECT_TRUE(names.insert(info.powerEntityName).second);
    }
}

// Each PowerEntity must have a unique ID
TEST_P(PowerStatsAidl, ValidatePowerEntityIds) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    std::set<int32_t> ids;
    for (auto info : infos) {
        EXPECT_TRUE(ids.insert(info.powerEntityId).second);
    }
}

// Each state must have a valid name
TEST_P(PowerStatsAidl, ValidateStateNames) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        for (auto state : info.states) {
            EXPECT_NE(state.powerEntityStateName, "");
        }
    }
}

// Each state must have a name that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueNames) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        std::set<std::string> stateNames;
        for (auto state : info.states) {
            EXPECT_TRUE(stateNames.insert(state.powerEntityStateName).second);
        }
    }
}

// Each state must have an ID that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueIds) {
    std::vector<PowerEntityInfo> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        std::set<uint32_t> stateIds;
        for (auto state : info.states) {
            EXPECT_TRUE(stateIds.insert(state.powerEntityStateId).second);
        }
    }
}

TEST_P(PowerStatsAidl, TestGetPowerEntityStateResidencyData) {
    std::vector<PowerEntityStateResidencyResult> data;
    ASSERT_TRUE(powerstats->getPowerEntityStateResidencyData({}, &data).isOk());
}

TEST_P(PowerStatsAidl, TestGetRailInfo) {
    std::vector<RailInfo> info;
    ASSERT_TRUE(powerstats->getRailInfo(&info).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerStatsAidl);
INSTANTIATE_TEST_SUITE_P(
        PowerStats, PowerStatsAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IPowerStats::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
