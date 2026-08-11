/*
 * KeypadController.cpp — AD-1 CTL board
 *
 * sample() is polled by the TouchGFX HAL once per frame (~57 Hz here).
 * Edge detection produces one event per press; arrows auto-repeat while
 * held (initial delay then steady rate) for comfortable menu scrolling.
 *
 * Adjust the keyMap[] table to match your CubeMX pin labels and the
 * physical layout of KEY0..KEY4 on the front panel.
 */
#include "KeypadController.hpp"

struct KeyPin
{
    GPIO_TypeDef* port;
    uint16_t      pin;
    uint8_t       code;
    bool          autorepeat;
};

/* KEY0..KEY4 from the schematic -> functions. FIX THE ORDER to match
 * which net goes to which physical button on your panel.               */
static const KeyPin keyMap[5] =
{
    { UP_BTN_GPIO_Port, UP_BTN_Pin, Keys::Up,    true  },
    { DN_BTN_GPIO_Port, DN_BTN_Pin, Keys::Down,  true  },
    { PR_BTN_GPIO_Port, PR_BTN_Pin, Keys::Left,  true  },
    { FW_BTN_GPIO_Port, FW_BTN_Pin, Keys::Right, true  },
    { OK_BTN_GPIO_Port, OK_BTN_Pin, Keys::Ok,    false },
};

/* Auto-repeat timing in frames (~17.5 ms each at 57 Hz)                */
static const uint16_t REPEAT_DELAY = 28;  /* ~0.5 s before repeating    */
static const uint16_t REPEAT_RATE  = 6;   /* ~10 events/s while held    */

void KeypadController::init()
{
    prevMask  = 0;
    heldIndex = -1;
    heldTicks = 0;
}

bool KeypadController::sample(uint8_t& key)
{
    /* HC14 inverts: pressed = pin HIGH */
    uint8_t mask = 0;
    for (int i = 0; i < 5; i++)
    {
        if (HAL_GPIO_ReadPin(keyMap[i].port, keyMap[i].pin) == GPIO_PIN_SET)
        {
            mask |= (uint8_t)(1u << i);
        }
    }

    const uint8_t newPress = (uint8_t)(mask & ~prevMask);
    prevMask = mask;

    /* New press: report it, arm auto-repeat if applicable */
    if (newPress != 0)
    {
        for (int i = 0; i < 5; i++)
        {
            if (newPress & (1u << i))
            {
                heldIndex = keyMap[i].autorepeat ? (int8_t)i : (int8_t)-1;
                heldTicks = 0;
                key = keyMap[i].code;
                return true;
            }
        }
    }

    /* Auto-repeat while the armed key stays down (and only that key)   */
    if (heldIndex >= 0)
    {
        if (mask == (uint8_t)(1u << heldIndex))
        {
            heldTicks++;
            if (heldTicks >= REPEAT_DELAY &&
                ((heldTicks - REPEAT_DELAY) % REPEAT_RATE) == 0)
            {
                key = keyMap[heldIndex].code;
                return true;
            }
        }
        else
        {
            heldIndex = -1;   /* released or chorded: disarm            */
        }
    }

    return false;
}
