#include "framework.h"
#include "ioutils.h"

#define LED_RIGHT _D3
#define LED_LEFT _D4
#define LED_MAIN _D2

#define LED_LEFT_IND _C3
#define LED_RIGHT_IND _B6

#define BRAKE_SWITCH _B7

#define TURN_RESET _C0
#define TURN_DT _C1
#define TURN_CLK _C2

enum class TurnSignal
{
    Off,
    Left,
    Right,
    Emergency
};

#define EMERGENCY_HAZARD 0
#define EMERGENCY_PULSE 1
#define EMERGENCY_SOS 2
#define EMERGENCY__LAST EMERGENCY_SOS

#define T0_CLK CLK_1024

#include <lib/predividers.h>