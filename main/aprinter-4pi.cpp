/*
 * Copyright (c) 2013 Ambroz Bizjak
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>

#include <aprinter/platform/at91sam3u/at91sam3u_support.h>

static void emergency (void);

#define AMBROLIB_EMERGENCY_ACTION { cli(); emergency(); }
#define AMBROLIB_ABORT_ACTION { while (1); }

#include <aprinter/meta/MakeTypeList.h>
#include <aprinter/base/Object.h>
#include <aprinter/base/DebugObject.h>
#include <aprinter/system/BusyEventLoop.h>
#include <aprinter/system/At91Sam3uClock.h>
#include <aprinter/system/At91SamPins.h>
#include <aprinter/system/InterruptLock.h>
#include <aprinter/system/At91SamAdc.h>
#include <aprinter/system/At91SamWatchdog.h>
#include <aprinter/system/AsfUsbSerial.h>
#include <aprinter/system/At91SamSpi.h>
#include <aprinter/driver/AxisDriver.h>
#include <aprinter/printer/PrinterMain.h>
#include <aprinter/printer/AxisHomer.h>
#include <aprinter/printer/TemperatureObserver.h>
#include <aprinter/printer/pwm/SoftPwm.h>
#include <aprinter/printer/thermistor/GenericThermistor.h>
#include <aprinter/printer/temp_control/PidControl.h>
#include <aprinter/printer/temp_control/BinaryControl.h>
#include <aprinter/printer/config_manager/ConstantConfigManager.h>
#include <aprinter/printer/config_manager/RuntimeConfigManager.h>
#include <aprinter/printer/microstep/A4982MicroStep.h>
#include <aprinter/printer/current/Ad5206Current.h>

using namespace APrinter;

APRINTER_CONFIG_START

using AdcFreq = AMBRO_WRAP_DOUBLE(1000000.0);
using AdcAvgInterval = AMBRO_WRAP_DOUBLE(0.0025);
static uint16_t const AdcSmoothing = 0.95 * 65536.0;

using LedBlinkInterval = AMBRO_WRAP_DOUBLE(0.5);
using SpeedLimitMultiply = AMBRO_WRAP_DOUBLE(1.0 / 60.0);
using TheAxisDriverPrecisionParams = AxisDriverDuePrecisionParams;
using EventChannelTimerClearance = AMBRO_WRAP_DOUBLE(0.002);

APRINTER_CONFIG_OPTION_DOUBLE(MaxStepsPerCycle, 0.0017, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ForceTimeout, 0.1, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(InactiveTime, 8.0 * 60.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_BOOL(XInvertDir, true, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XStepsPerUnit, 80.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XMin, -53.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XMax, 210.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XMaxSpeed, 300.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XMaxAccel, 1500.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XDistanceFactor, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XCorneringDistance, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(XHomeDir, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(XHomeEndInvert, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeFastMaxDist, 280.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeRetractDist, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeSlowMaxDist, 5.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeFastSpeed, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeRetractSpeed, 50.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XHomeSlowSpeed, 5.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(XCurrent, 128.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_BOOL(YInvertDir, true, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YStepsPerUnit, 80.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YMin, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YMax, 155.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YMaxSpeed, 300.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YMaxAccel, 650.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YDistanceFactor, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YCorneringDistance, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(YHomeDir, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(YHomeEndInvert, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeFastMaxDist, 200.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeRetractDist, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeSlowMaxDist, 5.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeFastSpeed, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeRetractSpeed, 50.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YHomeSlowSpeed, 5.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(YCurrent, 128.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_BOOL(ZInvertDir, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZStepsPerUnit, 4000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZMin, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZMax, 100.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZMaxSpeed, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZMaxAccel, 30.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZDistanceFactor, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZCorneringDistance, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(ZHomeDir, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_BOOL(ZHomeEndInvert, false, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeFastMaxDist, 101.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeRetractDist, 0.8, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeSlowMaxDist, 1.2, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeFastSpeed, 2.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeRetractSpeed, 2.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZHomeSlowSpeed, 0.6, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ZCurrent, 128.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_BOOL(EInvertDir, true, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EStepsPerUnit, 928.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EMin, -40000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EMax, 40000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EMaxSpeed, 45.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EMaxAccel, 250.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(EDistanceFactor, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ECorneringDistance, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ECurrent, 128.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_BOOL(UInvertDir, true, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UStepsPerUnit, 660.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UMin, -40000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UMax, 40000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UMaxSpeed, 45.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UMaxAccel, 250.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UDistanceFactor, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UCorneringDistance, 40.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UCurrent, 128.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterThermistorResistorR, 4700.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterThermistorR0, 100000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterThermistorBeta, 3960.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterThermistorMinTemp, 10.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterThermistorMaxTemp, 300.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterMinSafeTemp, 20.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterMaxSafeTemp, 280.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterControlInterval, 0.2, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidP, 0.047, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidI, 0.0006, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidD, 0.17, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidIStateMin, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidIStateMax, 0.4, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterPidDHistory, 0.7, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterObserverInterval, 0.5, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterObserverTolerance, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ExtruderHeaterObserverMinTime, 3.0, ConfigNoProperties)
using ExtruderHeaterPulseInterval = AMBRO_WRAP_DOUBLE(0.2);

APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterThermistorResistorR, 4700.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterThermistorR0, 100000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterThermistorBeta, 3960.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterThermistorMinTemp, 10.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterThermistorMaxTemp, 300.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterMinSafeTemp, 20.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterMaxSafeTemp, 280.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterControlInterval, 0.2, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidP, 0.047, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidI, 0.0006, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidD, 0.17, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidIStateMin, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidIStateMax, 0.4, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterPidDHistory, 0.7, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterObserverInterval, 0.5, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterObserverTolerance, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(UxtruderHeaterObserverMinTime, 3.0, ConfigNoProperties)
using UxtruderHeaterPulseInterval = AMBRO_WRAP_DOUBLE(0.2);

APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterThermistorResistorR, 4700.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterThermistorR0, 10000.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterThermistorBeta, 3480.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterThermistorMinTemp, 10.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterThermistorMaxTemp, 150.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterMinSafeTemp, 20.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterMaxSafeTemp, 120.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterControlInterval, 0.3, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidP, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidI, 0.012, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidD, 2.5, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidIStateMin, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidIStateMax, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterPidDHistory, 0.8, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterObserverInterval, 0.5, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterObserverTolerance, 1.5, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(BedHeaterObserverMinTime, 3.0, ConfigNoProperties)
using BedHeaterPulseInterval = AMBRO_WRAP_DOUBLE(0.3);

APRINTER_CONFIG_OPTION_DOUBLE(ProbeOffsetX, -18.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeOffsetY, -31.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeStartHeight, 17.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeLowHeight, 5.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeRetractDist, 1.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeMoveSpeed, 120.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeFastSpeed, 2.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeRetractSpeed, 3.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeSlowSpeed, 0.6, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP1X, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP1Y, 31.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP2X, 0.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP2Y, 155.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP3X, 205.0, ConfigNoProperties)
APRINTER_CONFIG_OPTION_DOUBLE(ProbeP3Y, 83.0, ConfigNoProperties)

APRINTER_CONFIG_OPTION_DOUBLE(CurrentConversionFactor, 100.0 / 743.0, ConfigNoProperties)

APRINTER_CONFIG_END

using PrinterParams = PrinterMainParams<
    /*
     * Common parameters.
     */
    PrinterMainSerialParams<
        UINT32_C(250000), // BaudRate,
        8, // RecvBufferSizeExp
        9, // SendBufferSizeExp
        GcodeParserParams<16>, // ReceiveBufferSizeExp
        AsfUsbSerialService
    >,
    At91SamPin<At91SamPioC, 22>, // LedPin
    LedBlinkInterval, // LedBlinkInterval
    InactiveTime, // InactiveTime
    SpeedLimitMultiply, // SpeedLimitMultiply
    MaxStepsPerCycle, // MaxStepsPerCycle
    32, // StepperSegmentBufferSize
    32, // EventChannelBufferSize
    28, // LookaheadBufferSize
    10, // LookaheadCommitCount
    ForceTimeout, // ForceTimeout
    double, // FpType
    At91Sam3uClockInterruptTimerService<At91Sam3uClockTC0, At91Sam3uClockCompA, EventChannelTimerClearance>, // EventChannelTimer
    At91SamWatchdogService<260>,
    PrinterMainNoSdCardParams,
    PrinterMainProbeParams<
        MakeTypeList<WrapInt<'X'>, WrapInt<'Y'>>, // PlatformAxesList
        'Z', // ProbeAxis
        At91SamPin<At91SamPioC, 18>, // ProbePin,
        At91SamPinInputModePullUp, // ProbePinInputMode
        false, // ProbeInvert,
        MakeTypeList<ProbeOffsetX, ProbeOffsetY>, // ProbePlatformOffset
        ProbeStartHeight,
        ProbeLowHeight,
        ProbeRetractDist,
        ProbeMoveSpeed,
        ProbeFastSpeed,
        ProbeRetractSpeed,
        ProbeSlowSpeed,
        MakeTypeList< // ProbePoints
            MakeTypeList<ProbeP1X, ProbeP1Y>,
            MakeTypeList<ProbeP2X, ProbeP2Y>,
            MakeTypeList<ProbeP3X, ProbeP3Y>
        >
    >,
    PrinterMainCurrentParams<
        MakeTypeList< // CurrentAxesList
            PrinterMainCurrentAxis<'X', XCurrent, Ad5206CurrentChannelParams<3, CurrentConversionFactor>>,
            PrinterMainCurrentAxis<'Y', YCurrent, Ad5206CurrentChannelParams<1, CurrentConversionFactor>>,
            PrinterMainCurrentAxis<'Z', ZCurrent, Ad5206CurrentChannelParams<0, CurrentConversionFactor>>,
            PrinterMainCurrentAxis<'E', ECurrent, Ad5206CurrentChannelParams<2, CurrentConversionFactor>>,
            PrinterMainCurrentAxis<'U', UCurrent, Ad5206CurrentChannelParams<5, CurrentConversionFactor>>
        >,
        Ad5206CurrentService< // CurrentService
            At91SamPin<At91SamPioA, 16>, // SsPin
            At91SamSpiService<At91Sam3uSpiDevice> // SpiService
        >
    >,
    RuntimeConfigManagerService<
        RuntimeConfigManagerNoStoreService
    >,
    ConfigList,
    
    /*
     * Axes.
     */
    MakeTypeList<
        PrinterMainAxisParams<
            'X', // Name
            At91SamPin<At91SamPioB, 8>, // DirPin
            At91SamPin<At91SamPioC, 28>, // StepPin
            At91SamPin<At91SamPioC, 31>, // EnablePin
            XInvertDir,
            XStepsPerUnit, // StepsPerUnit
            XMin, // Min
            XMax, // Max
            XMaxSpeed, // MaxSpeed
            XMaxAccel, // MaxAccel
            XDistanceFactor, // DistanceFactor
            XCorneringDistance, // CorneringDistance
            PrinterMainHomingParams<
                XHomeDir,
                AxisHomerService< // HomerService
                    At91SamPin<At91SamPioB, 16>, // HomeEndPin
                    At91SamPinInputModePullUp, // HomeEndPinInputMode
                    XHomeEndInvert,
                    XHomeFastMaxDist, // HomeFastMaxDist
                    XHomeRetractDist, // HomeRetractDist
                    XHomeSlowMaxDist, // HomeSlowMaxDist
                    XHomeFastSpeed, // HomeFastSpeed
                    XHomeRetractSpeed, // HomeRetractSpeed
                    XHomeSlowSpeed // HomeSlowSpeed
                >
            >,
            true, // EnableCartesianSpeedLimit
            32, // StepBits
            AxisDriverService<
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC1, At91Sam3uClockCompA>, // StepperTimer,
                TheAxisDriverPrecisionParams // PrecisionParams
            >,
            PrinterMainMicroStepParams<
                A4982MicroStep, // MicroStepTemplate
                A4982MicroStepParams< // MicroStepParams
                    At91SamPin<At91SamPioC, 30>, // Ms1Pin
                    At91SamPin<At91SamPioC, 29> // Ms2Pin
                >,
                16 // MicroSteps
            >
        >,
        PrinterMainAxisParams<
            'Y', // Name
            At91SamPin<At91SamPioA, 31>, // DirPin
            At91SamPin<At91SamPioC, 23>, // StepPin
            At91SamPin<At91SamPioA, 22>, // EnablePin
            YInvertDir,
            YStepsPerUnit, // StepsPerUnit
            YMin, // Min
            YMax, // Max
            YMaxSpeed, // MaxSpeed
            YMaxAccel, // MaxAccel
            YDistanceFactor, // DistanceFactor
            YCorneringDistance, // CorneringDistance
            PrinterMainHomingParams<
                YHomeDir,
                AxisHomerService< // HomerService
                    At91SamPin<At91SamPioA, 17>, // HomeEndPin
                    At91SamPinInputModePullUp, // HomeEndPinInputMode
                    YHomeEndInvert,
                    YHomeFastMaxDist, // HomeFastMaxDist
                    YHomeRetractDist, // HomeRetractDist
                    YHomeSlowMaxDist, // HomeSlowMaxDist
                    YHomeFastSpeed, // HomeFastSpeed
                    YHomeRetractSpeed, // HomeRetractSpeed
                    YHomeSlowSpeed // HomeSlowSpeed
                >
            >,
            true, // EnableCartesianSpeedLimit
            32, // StepBits
            AxisDriverService<
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC2, At91Sam3uClockCompA>, // StepperTimer
                TheAxisDriverPrecisionParams // PrecisionParams
            >,
            PrinterMainMicroStepParams<
                A4982MicroStep, // MicroStepTemplate
                A4982MicroStepParams< // MicroStepParams
                    At91SamPin<At91SamPioC, 11>, // Ms1Pin
                    At91SamPin<At91SamPioC, 10> // Ms2Pin
                >,
                16 // MicroSteps
            >
        >,
        PrinterMainAxisParams<
            'Z', // Name
            At91SamPin<At91SamPioC, 27>, // DirPin
            At91SamPin<At91SamPioA, 27>, // StepPin
            At91SamPin<At91SamPioB, 7>, // EnablePin
            ZInvertDir,
            ZStepsPerUnit, // StepsPerUnit
            ZMin, // Min
            ZMax, // Max
            ZMaxSpeed, // MaxSpeed
            ZMaxAccel, // MaxAccel
            ZDistanceFactor, // DistanceFactor
            ZCorneringDistance, // CorneringDistance
            PrinterMainHomingParams<
                ZHomeDir,
                AxisHomerService< // HomerService
                    At91SamPin<At91SamPioC, 12>, // HomeEndPin
                    At91SamPinInputModePullUp, // HomeEndPinInputMode
                    ZHomeEndInvert,
                    ZHomeFastMaxDist, // HomeFastMaxDist
                    ZHomeRetractDist, // HomeRetractDist
                    ZHomeSlowMaxDist, // HomeSlowMaxDist
                    ZHomeFastSpeed, // HomeFastSpeed
                    ZHomeRetractSpeed, // HomeRetractSpeed
                    ZHomeSlowSpeed // HomeSlowSpeed
                >
            >,
            true, // EnableCartesianSpeedLimit
            32, // StepBits
            AxisDriverService<
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC0, At91Sam3uClockCompB>, // StepperTimer
                TheAxisDriverPrecisionParams // PrecisionParams
            >,
            PrinterMainMicroStepParams<
                A4982MicroStep, // MicroStepTemplate
                A4982MicroStepParams< // MicroStepParams
                    At91SamPin<At91SamPioB, 6>, // Ms1Pin
                    At91SamPin<At91SamPioB, 5> // Ms2Pin
                >,
                16 // MicroSteps
            >
        >,
        PrinterMainAxisParams<
            'E', // Name
            At91SamPin<At91SamPioB, 2>, // DirPin
            At91SamPin<At91SamPioC, 26>, // StepPin
            At91SamPin<At91SamPioB, 13>, // EnablePin
            EInvertDir,
            EStepsPerUnit, // StepsPerUnit
            EMin, // Min
            EMax, // Max
            EMaxSpeed, // MaxSpeed
            EMaxAccel, // MaxAccel
            EDistanceFactor, // DistanceFactor
            ECorneringDistance, // CorneringDistance
            PrinterMainNoHomingParams,
            false, // EnableCartesianSpeedLimit
            32, // StepBits
            AxisDriverService<
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC1, At91Sam3uClockCompB>, // StepperTimer
                TheAxisDriverPrecisionParams // PrecisionParams
            >,
            PrinterMainMicroStepParams<
                A4982MicroStep, // MicroStepTemplate
                A4982MicroStepParams< // MicroStepParams
                    At91SamPin<At91SamPioB, 12>, // Ms1Pin
                    At91SamPin<At91SamPioB, 11> // Ms2Pin
                >,
                16 // MicroSteps
            >
        >/*,
        PrinterMainAxisParams<
            'U', // Name
            At91SamPin<At91SamPioC, 25>, // DirPin
            At91SamPin<At91SamPioB, 1>, // StepPin
            At91SamPin<At91SamPioC, 13>, // EnablePin
            UInvertDir,
            UStepsPerUnit, // StepsPerUnit
            UMin, // Min
            UMax, // Max
            UMaxSpeed, // MaxSpeed
            UMaxAccel, // MaxAccel
            UDistanceFactor, // DistanceFactor
            UCorneringDistance, // CorneringDistance
            PrinterMainNoHomingParams,
            false, // EnableCartesianSpeedLimit
            32, // StepBits
            AxisDriverService<
                NONE, // StepperTimer
                TheAxisDriverPrecisionParams // PrecisionParams
            >,
            PrinterMainMicroStepParams<
                A4982MicroStep, // MicroStepTemplate
                A4982MicroStepParams< // MicroStepParams
                    At91SamPin<At91SamPioB, 0>, // Ms1Pin
                    At91SamPin<At91SamPioC, 24> // Ms2Pin
                >,
                16 // MicroSteps
            >
        >*/
    >,
    
    /*
     * Transform and virtual axes.
     */
    PrinterMainNoTransformParams,
    
    /*
     * Heaters.
     */
    MakeTypeList<
        PrinterMainHeaterParams<
            'T', // Name
            104, // SetMCommand
            109, // WaitMCommand
            At91SamPin<At91SamPioB, 4>, // AdcPin
            GenericThermistorService< // Thermistor
                ExtruderHeaterThermistorResistorR,
                ExtruderHeaterThermistorR0,
                ExtruderHeaterThermistorBeta,
                ExtruderHeaterThermistorMinTemp,
                ExtruderHeaterThermistorMaxTemp
            >,
            ExtruderHeaterMinSafeTemp, // MinSafeTemp
            ExtruderHeaterMaxSafeTemp, // MaxSafeTemp
            ExtruderHeaterControlInterval, // ControlInterval
            PidControlService<
                ExtruderHeaterPidP, // PidP
                ExtruderHeaterPidI, // PidI
                ExtruderHeaterPidD, // PidD
                ExtruderHeaterPidIStateMin, // PidIStateMin
                ExtruderHeaterPidIStateMax, // PidIStateMax
                ExtruderHeaterPidDHistory // PidDHistory
            >,
            TemperatureObserverService<
                ExtruderHeaterObserverInterval, // ObserverInterval
                ExtruderHeaterObserverTolerance, // ObserverTolerance
                ExtruderHeaterObserverMinTime // ObserverMinTime
            >,
            SoftPwmService<
                At91SamPin<At91SamPioA, 21>, // OutputPin
                false, // OutputInvert
                ExtruderHeaterPulseInterval, // PulseInterval
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC2, At91Sam3uClockCompB> // TimerTemplate
            >
        >,
        PrinterMainHeaterParams<
            'B', // Name
            140, // SetMCommand
            190, // WaitMCommand
            At91SamPin<At91SamPioC, 16>, // AdcPin
            GenericThermistorService< // Thermistor
                BedHeaterThermistorResistorR,
                BedHeaterThermistorR0,
                BedHeaterThermistorBeta,
                BedHeaterThermistorMinTemp,
                BedHeaterThermistorMaxTemp
            >,
            BedHeaterMinSafeTemp, // MinSafeTemp
            BedHeaterMaxSafeTemp, // MaxSafeTemp
            BedHeaterControlInterval, // ControlInterval
            PidControlService<
                BedHeaterPidP, // PidP
                BedHeaterPidI, // PidI
                BedHeaterPidD, // PidD
                BedHeaterPidIStateMin, // PidIStateMin
                BedHeaterPidIStateMax, // PidIStateMax
                BedHeaterPidDHistory // PidDHistory
            >,
            TemperatureObserverService<
                BedHeaterObserverInterval, // ObserverInterval
                BedHeaterObserverTolerance, // ObserverTolerance
                BedHeaterObserverMinTime // ObserverMinTime
            >,
            SoftPwmService<
                At91SamPin<At91SamPioA, 20>, // OutputPin
                false, // OutputInvert
                BedHeaterPulseInterval, // PulseInterval
                At91Sam3uClockInterruptTimerService<At91Sam3uClockTC0, At91Sam3uClockCompC> // TimerTemplate
            >
        >/*,
        PrinterMainHeaterParams<
            'U', // Name
            404, // SetMCommand
            409, // WaitMCommand
            At91SamPin<At91SamPioA, 30>, // AdcPin
            GenericThermistorService< // Thermistor
                UxtruderHeaterThermistorResistorR,
                UxtruderHeaterThermistorR0,
                UxtruderHeaterThermistorBeta,
                UxtruderHeaterThermistorMinTemp,
                UxtruderHeaterThermistorMaxTemp
            >,
            UxtruderHeaterMinSafeTemp, // MinSafeTemp
            UxtruderHeaterMaxSafeTemp, // MaxSafeTemp
            UxtruderHeaterControlInterval, // ControlInterval
            PidControlService<
                UxtruderHeaterPidP, // PidP
                UxtruderHeaterPidI, // PidI
                UxtruderHeaterPidD, // PidD
                UxtruderHeaterPidIStateMin, // PidIStateMin
                UxtruderHeaterPidIStateMax, // PidIStateMax
                UxtruderHeaterPidDHistory // PidDHistory
            >,
            TemperatureObserverService<
                UxtruderHeaterObserverInterval, // ObserverInterval
                UxtruderHeaterObserverTolerance, // ObserverTolerance
                UxtruderHeaterObserverMinTime // ObserverMinTime
            >,
            SoftPwmService<
                At91SamPin<At91SamPioA, 23>, // OutputPin
                false, // OutputInvert
                UxtruderHeaterPulseInterval, // PulseInterval
                NONE // TimerTemplate
            >
        >*/
    >,
    
    /*
     * Fans.
     */
    MakeTypeList<>
>;

// need to list all used ADC pins here
using AdcPins = MakeTypeList<
    At91SamAdcSmoothPin<At91SamPin<At91SamPioB, 4>, AdcSmoothing>,
    At91SamAdcSmoothPin<At91SamPin<At91SamPioC, 16>, AdcSmoothing>,
    At91SamAdcSmoothPin<At91SamPin<At91SamPioA, 30>, AdcSmoothing>
>;

using AdcParams = At91Sam3uAdcParams<
    AdcFreq,
    255, // AdcStartup
    15, // AdcShtim
    At91SamAdcAvgParams<AdcAvgInterval>
>;

static const int clock_timer_prescaler = 3;
using ClockTcsList = MakeTypeList<
    At91Sam3uClockTC0,
    At91Sam3uClockTC1,
    At91Sam3uClockTC2
>;

struct MyContext;
struct MyLoopExtraDelay;
struct Program;

using MyDebugObjectGroup = DebugObjectGroup<MyContext, Program>;
using MyClock = At91Sam3uClock<MyContext, Program, clock_timer_prescaler, ClockTcsList>;
using MyLoop = BusyEventLoop<MyContext, Program, MyLoopExtraDelay>;
using MyPins = At91SamPins<MyContext, Program>;
using MyAdc = At91SamAdc<MyContext, Program, AdcPins, AdcParams>;
using MyPrinter = PrinterMain<MyContext, Program, PrinterParams>;

struct MyContext {
    using DebugGroup = MyDebugObjectGroup;
    using Clock = MyClock;
    using EventLoop = MyLoop;
    using Pins = MyPins;
    using Adc = MyAdc;
    
    void check () const;
};

using MyLoopExtra = BusyEventLoopExtra<Program, MyLoop, typename MyPrinter::EventLoopFastEvents>;
struct MyLoopExtraDelay : public WrapType<MyLoopExtra> {};

struct Program : public ObjBase<void, void, MakeTypeList<
    MyDebugObjectGroup,
    MyClock,
    MyLoop,
    MyPins,
    MyAdc,
    MyPrinter,
    MyLoopExtra
>> {
    static Program * self (MyContext c);
};

Program p;

Program * Program::self (MyContext c) { return &p; }
void MyContext::check () const {}

AMBRO_AT91SAM3U_CLOCK_TC0_GLOBAL(MyClock, MyContext())
AMBRO_AT91SAM3U_CLOCK_TC1_GLOBAL(MyClock, MyContext())
AMBRO_AT91SAM3U_CLOCK_TC2_GLOBAL(MyClock, MyContext())

AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC0, At91Sam3uClockCompA, MyPrinter::GetEventChannelTimer, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC1, At91Sam3uClockCompA, MyPrinter::GetAxisTimer<0>, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC2, At91Sam3uClockCompA, MyPrinter::GetAxisTimer<1>, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC0, At91Sam3uClockCompB, MyPrinter::GetAxisTimer<2>, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC1, At91Sam3uClockCompB, MyPrinter::GetAxisTimer<3>, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC2, At91Sam3uClockCompB, MyPrinter::GetHeaterPwm<0>::TheTimer, MyContext())
AMBRO_AT91SAM3U_CLOCK_INTERRUPT_TIMER_GLOBAL(At91Sam3uClockTC0, At91Sam3uClockCompC, MyPrinter::GetHeaterPwm<1>::TheTimer, MyContext())

AMBRO_AT91SAM3U_ADC_GLOBAL(MyAdc, MyContext())
AMBRO_AT91SAM3U_SPI_GLOBAL(MyPrinter::GetCurrent<>::GetSpi, MyContext())

static void emergency (void)
{
    MyPrinter::emergency();
}

int main ()
{
    platform_init();
    udc_start();
    
    MyContext c;
    
    MyDebugObjectGroup::init(c);
    MyClock::init(c);
    MyLoop::init(c);
    MyPins::init(c);
    MyAdc::init(c);
    MyPrinter::init(c);
    
    MyLoop::run(c);
}
