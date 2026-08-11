/*
 * KeypadController.hpp — AD-1 CTL board, 5-key keypad via 74HC14
 *
 * Hardware: buttons switch to GND, 10k pull-up + RC (10k/100nF) into
 * SN74HC14 Schmitt inverter -> MCU. Fully debounced in hardware.
 * Consequence: PRESSED = GPIO_PIN_SET (inverter!), pins = input, NO pull.
 *
 * Location: TouchGFX/target/ (user area, survives regeneration)
 */

#ifndef TARGET_KEYPADCONTROLLER_HPP_
#define TARGET_KEYPADCONTROLLER_HPP_

#include <platform/driver/button/ButtonController.hpp>
#include "main.h"

/* Key codes delivered to TouchGFX. Use these same values in Designer
* ("Hardware button is clicked" interactions) and in handleKeyEvent(). */
namespace Keys
{
	enum : uint8_t
	{
		Up		= 1,
		Down	= 2,
		Left	= 3,
		Right	= 4,
		Ok		= 13
	};
}

class KeypadController : public touchgfx::ButtonController
{
public:
	virtual void init();
	virtual bool sample(uint8_t& key);

private:
	uint8_t		prevMask;	/* pressed-state bitmask of previous frame 	*/
	int8_t		heldIndex;	/* index of key held for auto-repeat, -1=no */
	uint16_t	heldTicks;	/* frames the key has been held				*/
};

#endif /* TARGET_KEYPADCONTROLLER_HPP_ */
