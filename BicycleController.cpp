using namespace std;

#include "BicycleController.h"

#define IN_RANGE(val, min, max) ((val) > (min) && (val) <= (max))

IOPort io = io_port_default;

volatile TurnSignal turn;
volatile uint64_t time = 0;
volatile uint8_t currentEmergency = EMERGENCY_HAZARD;
volatile uint8_t holdCounter = 0;
volatile uint64_t lastResetTime = 0;

void irq_callback(IOPort8 *port8, uint8_t localInd)
{
	// get IOPort32 index of IRQ
	uint8_t i = io.getPinIndex(port8, localInd);

	if (i == BRAKE_SWITCH)
	{
		// set led value to brake switch
		io.put(LED_MAIN, io.get(BRAKE_SWITCH));
	}
	else
	{
		if (io.get_falling(TURN_RESET) && ((time - lastResetTime) > 15 || time < lastResetTime)) // actually rising but relative to gound
		{
			lastResetTime = time;
			if (turn == TurnSignal::Emergency)
			{
				time = 0;

				if (++currentEmergency > EMERGENCY__LAST)
				{
					currentEmergency = 0;
				}
			}
			else
			{
				turn = TurnSignal::Off;
			}

			// start hold timer
			TCCR0B = T0_CLK;
		}
		else if (io.get_rising(TURN_RESET) && ((time - lastResetTime) > 15 || time < lastResetTime)) // actually falling but relative to gound
		{
			lastResetTime = time;
			TCCR0B = 0; // stop timer
			TCNT0 = 0;	// reset counter
			holdCounter = 0;
		}
		else if (i == TURN_CLK || i == TURN_DT)
		{
			static bool clk_val = false;
			static int8_t desync_counter = 0;

			bool clk = io.get(TURN_CLK);
			if (clk != clk_val && turn != TurnSignal::Emergency)
			{
				bool dt = io.get(TURN_DT);

				if (clk == dt)
					desync_counter++;
				else
					desync_counter--;

				if (desync_counter < -1)
				{
					desync_counter = 0; // resync
					time = 0;
					turn = TurnSignal::Left;
				}
				else if (desync_counter > 1)
				{
					desync_counter = 0; // resync
					time = 0;
					turn = TurnSignal::Right;
				}
			}

			clk_val = clk;
		}
	}
}

// reset button hold timer interrupt
ISR(TIMER0_OVF_vect)
{
	if (++holdCounter > 50) // extend hold duration
	{
		TCCR0B = 0; // stop timer
		TCNT0 = 0;	// reset counter
		holdCounter = 0;

		if (turn == TurnSignal::Emergency)
		{
			turn = TurnSignal::Off;
		}
		else
		{
			turn = TurnSignal::Emergency;
		}
	}
}

void initIO()
{
	io.reset();

	// set up leds
	io.set_dir(LED_LEFT, IODir::Out);
	io.put(LED_LEFT, true); // inverted
	io.set_dir(LED_RIGHT, IODir::Out);
	io.put(LED_RIGHT, true); // inverted
	io.set_dir(LED_MAIN, IODir::Out);
	io.set_dir(LED_LEFT_IND, IODir::Out);
	io.set_dir(LED_RIGHT_IND, IODir::Out);

	// set up rotary encoder
	io.set_dir(TURN_RESET, IODir::In);
	io.set_pull_up(TURN_RESET, true);
	io.set_dir(TURN_DT, IODir::In);
	io.set_pull_up(TURN_DT, true);
	io.set_dir(TURN_CLK, IODir::In);
	io.set_pull_up(TURN_CLK, true);

	io.set_pin_irq(TURN_RESET, true);
	io.set_pin_irq(TURN_DT, true);
	io.set_pin_irq(TURN_CLK, true);

	// set up brake switch
	io.set_dir(BRAKE_SWITCH, IODir::In);
	io.set_pull_up(BRAKE_SWITCH, true);
	io.set_pin_irq(BRAKE_SWITCH, true);

	// set irq callback per pin because of IOPort32 transforms (hard to convert entire bit mask)
	io.port_b.set_irq_callback(irq_callback, IRQCallbackMode::PerPin);
	io.port_c.set_irq_callback(irq_callback, IRQCallbackMode::PerPin);
	io.port_b.enable_irq_handler();
	io.port_c.enable_irq_handler();

	sei(); // enable interrupts

	// set led value to brake switch
	io.put(LED_MAIN, io.get(BRAKE_SWITCH));
}

void initTimers()
{
	TCCR0A = 0x00;		   // normal mode
	TCCR0B = 0;			   // stop initially
	TIMSK0 = (1 << TOIE0); // enable overflow interrupt
}

int main()
{
	initStream();
	initUSART(9600);
	initIO();
	initTimers();

	printf("\n========================================================\n                   BicycleController                    \n========================================================\n");

	bool blinkState = false;

	// main light controller loop
	while (1)
	{
		switch (turn)
		{
		case TurnSignal::Off:
			io.put(LED_LEFT, !false);
			io.put(LED_RIGHT, !false);
			io.put(LED_LEFT_IND, false);
			io.put(LED_RIGHT_IND, false);
			blinkState = false;
			break;
		case TurnSignal::Left:
			// every 500 ms
			blinkState = time % 500 <= 500 / 2;
			io.put(LED_LEFT, !blinkState);
			io.put(LED_LEFT_IND, blinkState);
			io.put(LED_RIGHT, !false);
			io.put(LED_RIGHT_IND, false);
			break;
		case TurnSignal::Right:
			// every 500 ms
			blinkState = time % 500 <= 500 / 2;
			io.put(LED_RIGHT, !blinkState);
			io.put(LED_RIGHT_IND, blinkState);
			io.put(LED_LEFT, !false);
			io.put(LED_LEFT_IND, false);
			break;
		case TurnSignal::Emergency:
			switch (currentEmergency)
			{
			case EMERGENCY_HAZARD:
			{
				// every 500 ms
				uint64_t cycle = time % 500;
				blinkState = IN_RANGE(cycle, 0, 250);
				io.put(LED_LEFT, !blinkState);
				io.put(LED_LEFT_IND, blinkState);
				io.put(LED_RIGHT, !blinkState);
				io.put(LED_RIGHT_IND, blinkState);
			}
			break;
			case EMERGENCY_PULSE:
			{
				uint64_t cycle = time % 500;
				blinkState = IN_RANGE(cycle, 0, 100) || IN_RANGE(cycle, 150, 250);
				io.put(LED_LEFT, !blinkState);
				io.put(LED_LEFT_IND, blinkState);
				io.put(LED_RIGHT, !blinkState);
				io.put(LED_RIGHT_IND, blinkState);
			}
			break;
			case EMERGENCY_SOS:
			{
				uint64_t cycle = time % 1700;
				blinkState = IN_RANGE(cycle, 0, 100) || IN_RANGE(cycle, 200, 300) || IN_RANGE(cycle, 400, 500) || IN_RANGE(cycle, 700, 900) || IN_RANGE(cycle, 1000, 1200) || IN_RANGE(cycle, 1300, 1500);
				io.put(LED_LEFT, !blinkState);
				io.put(LED_LEFT_IND, blinkState);
				io.put(LED_RIGHT, !blinkState);
				io.put(LED_RIGHT_IND, blinkState);
			}
			break;
			}
			break;
		}
		_delay_ms(1);
		time++;

		// IO <=> IRQ sync
		if (io.get(TURN_RESET) && ((time - lastResetTime) > 15 || time < lastResetTime))
		{
			TCCR0B = 0; // stop timer
			TCNT0 = 0;	// reset counter
			holdCounter = 0;
		}

		// set led value to brake switch
		io.put(LED_MAIN, io.get(BRAKE_SWITCH));
	}

	return 0;
}