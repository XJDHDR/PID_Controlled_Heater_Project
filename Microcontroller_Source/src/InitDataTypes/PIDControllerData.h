// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_PIDCONTROLLERINITDATA_H
#define ENGINEERING_PROJECT_PIDCONTROLLERINITDATA_H

#include <cstdint>

enum PIDSettings
{
	TemperatureSetPoint,
	LoopTimeStep,
	ProportionalGain,
	IntegralGain,
	IntegralWindupLimitMax,
	IntegralWindupLimitMin,
	DerivativeGain,
	DerivativeTermMaxValue,
	DerivativeTermMinValue,
	OutputMaxValue
};

struct PIDControllerInitData
{
	float TemperatureSetPointDegCent;
	int32_t LoopTimeStepMs;
	float ProportionalGain;
	float IntegralGain;
	float IntegralWindupLimitMax;
	float IntegralWindupLimitMin;
	float DerivativeGain;
	float DerivativeTermMaxValue;
	float DerivativeTermMinValue;
	float OutputMaxValue;
};

struct PIDFloatDataPacket
{
	PIDSettings Setting;
	float Value;
};

struct PIDIntDataPacket
{
	PIDSettings Setting;
	int32_t Value;
};

#endif //ENGINEERING_PROJECT_PIDCONTROLLERINITDATA_H
