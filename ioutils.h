#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "framework.h"
#include "staticlist.h"

/**
 * Disables IOPort32 pin definitions (_A0..._D7)
 */
// #define IOPORT_DISABLE_32

/**
 * Disables updating MCUCR in IOPort8::reset() to enable pull-ups
 * Define if using custom logic for pull-ups
 */
// #define IOPORT_NO_MCUCR

/**
 * Disables IOPort panic (same as #define IOPORT_PANIC ;)
 */
// #define IOPORT_NO_PANIC

/**
 * Sets IOPort panic to empty code (same as #define IOPORT_NO_PANIC).
 * Also supports custom panic handling code
 */
// #define IOPORT_PANIC ;

/**
 * Makes history-dependent functions return same value
 * for every irq callback (only used when in PerPin mode).
 * If not defined, those functions only return values for
 * the current IRQ pin (useful when callback is just a bunch of history checks)
 * When in Batch mode the history is shared by default.
 *
 * Example:
 * Default behavior:
 * void irq_callback(IOPort8 *port, uint8_t i)
 * {
 *    i == 0 && port->get_rising(0) // true
 *    i != 0 && port->get_rising(0) // false because current pin is not 0
 * }
 * Shared behavior:
 * void irq_callback(IOPort8 *port, uint8_t i)
 * {
 *    i == 0 && port->get_rising(0) // true
 *    i != 0 && port->get_rising(0) // true
 * }
 */
// #define IOPORT_SHARED_HISTORY

#ifndef IO_IRQ_PCINT0
#define IO_IRQ_PCINT0 true
#endif

#ifndef IO_IRQ_PCINT1
#define IO_IRQ_PCINT1 true
#endif

#ifndef IO_IRQ_PCINT2
#define IO_IRQ_PCINT2 true
#endif

#ifndef IO_IRQ_MAX_COUNT
#define IO_IRQ_MAX_COUNT 16
#endif

/// @brief IO pin direction
enum class IODir
{
    In,
    Out
};

enum class IRQCallbackMode
{
    PerPin,
    Batch
};

/// @brief 8 bit io port
typedef struct IOPort8
{
    /// @brief Pin change IRQ callback
    typedef void (*IOPinChangeCallback)(IOPort8 *, uint8_t);

    /// @brief default constructor
    IOPort8() {}

    /// @param port initial AVR port data
    /// @param ddr initial AVR ddr data
    /// @param _pinaddr AVR pin pointer
    /// @param _portaddr AVR port pointer
    /// @param _ddraddr AVR ddr pointer
    /// @param is_null set to true to disable all IO functions
    IOPort8(uint8_t port, uint8_t ddr, uint8_t irqIndex, volatile uint8_t *_pinaddr, volatile uint8_t *_portaddr, volatile uint8_t *_ddraddr, volatile uint8_t *_pcmskaddr, bool is_null);

    /// @brief Resets whole port
    void reset();
    /// @brief Resets single pin
    /// @param i Pin index
    void reset(uint8_t i);

    /// @brief Sets direction of pin
    /// @param i Pin index
    /// @param dir Pin direction
    void set_dir(uint8_t i, IODir dir);
    /// @brief Gets direction of pin
    /// @param i Pin index
    /// @return Pin direction
    IODir get_dir(uint8_t i);

    /// @brief Put value in port (only if dir is out)
    /// @param i Pin index
    /// @param value Output value
    void put(uint8_t i, bool value);
    /// @brief Gets value from pin (only if dir is in)
    /// @note To get current output value set with put() use get_port()
    /// @param i Pin index
    /// @return Input value
    bool get(uint8_t i);
    /// @brief Gets value set in port (only if dir is out)
    /// @param i Pin index
    /// @return Current output value set with put()
    bool get_port(uint8_t i);

    /// @brief Enables or disables input pull-up (only if dir is in)
    /// @param i Pin index
    /// @param pullUp true to pull up the pin, false to enter Hi-Z
    void set_pull_up(uint8_t i, bool pullUp);
    /// @brief Gets pull-up state for pin (only if dir is in)
    /// @param i Pin index
    /// @return true if pull-up is enabled, false if pin is in Hi-Z
    bool get_pull_up(uint8_t i);

    /// @brief Gets if value is falling from pin (only if dir is in)
    /// @param i Pin index
    /// @return True if value is falling
    /// @note Call from IRQ callback
    bool get_falling(uint8_t i);
    /// @brief Gets if value is rising from pin (only if dir is in)
    /// @param i Pin index
    /// @return True if value is rising
    /// @note Call from IRQ callback
    bool get_rising(uint8_t i);

    /// @brief Enables pin change interrupt (required for get_falling(), get_rising() and other history-dependent functions)
    void enable_irq_handler();
    /// @brief Disables pin change interrupt
    void disable_irq_handler();
    /// @brief Sets pin change interrupt callback
    /// @param callback void (port, pin OR changed_pins)
    /// @param callbackMode if PerPin callback gets called for each changed pin index, if Batch callback is called once for entire port with bit mask of changed pins
    void set_irq_callback(IOPinChangeCallback callback, IRQCallbackMode callbackMode);
    /// @brief Gets pin change interrupt callback mode
    /// @return Callback mode
    IRQCallbackMode get_irq_callback_mode();

    /// @brief Set pin interrupt state
    /// @param i Pin index
    /// @param enable true to enable interrupts for this pin, false to disable
    void
    set_pin_irq(uint8_t i, bool enable);
    /// @brief Gets pin interrupt state
    /// @param i Pin index
    /// @return true if interrupts are enabled for this pin, false if disabled
    bool get_pin_irq(uint8_t i);

    /// @brief !! DO NOT CALL DIRECTLY !!
    void irq_handler(uint8_t pcgroup);

    bool operator==(const IOPort8 &other)
    {
        if (_pinaddr == other._pinaddr &&
            _portaddr == other._portaddr &&
            _ddraddr == other._ddraddr &&
            _pcmskaddr == other._pcmskaddr &&
            is_null == other.is_null &&
            callback_mode == other.callback_mode &&
            port == other.port &&
            ddr == other.ddr &&
            lastPin == other.lastPin &&
            irqIndex == other.irqIndex &&
            pullUpMap == other.pullUpMap)
            return true;
        return false;
    }

private:
    /// @brief AVR port for port
    volatile uint8_t port;
    /// @brief AVR ddr for port
    volatile uint8_t ddr;
    /// @brief last AVR pin for port (for detecting changes)
    volatile uint8_t lastPin;
    /// @brief AVR index for PCICR used to enable and disable pin change interrupts
    volatile uint8_t irqIndex;
    /// @brief internal AVR port for input pins
    volatile uint8_t pullUpMap;

    /// @brief AVR pin pointer
    volatile uint8_t *_pinaddr;
    /// @brief AVR port pointer
    volatile uint8_t *_portaddr;
    /// @brief AVR ddr pointer
    volatile uint8_t *_ddraddr;
    /// @brief AVR pcmsk pointer
    volatile uint8_t *_pcmskaddr;

    volatile uint8_t _histbuf;

    /// @brief if true, disables all IO functions
    bool is_null;

    /// @brief User IRQ callback (caller_port, pin | changed pins bit mask)
    IOPinChangeCallback irq_callback;
    IRQCallbackMode callback_mode;
} IOPort8;

typedef struct IOPort32
{
    IOPort8 port_a;
    IOPort8 port_b;
    IOPort8 port_c;
    IOPort8 port_d;

    IOPort32(IOPort8 a, IOPort8 b, IOPort8 c, IOPort8 d);

    /// @brief Convert IOPort8 and its local index into IOPort32 index
    /// @param port8 Pointer to 8-bit port
    /// @param localIndex The local index (0-7)
    /// @return Global IOPort32 index
    uint8_t getPinIndex(IOPort8 *port8, uint8_t localIndex);
    /// @brief Convert IOPort32 index into IOPort8 and its local index
    /// @param i IOPort32 index
    /// @param port8 8-bit port reference (Out)
    /// @param localIndex Local index reference (Out)
    /// @return True if i is a valid IOPort32 index
    bool getLocalPort(uint8_t i, IOPort8 **port8, uint8_t *localIndex);

    /// @brief Resets whole port
    void reset();
    /// @brief Resets single pin
    /// @param i Pin index
    void reset(uint8_t i);

    /// @brief Sets direction of pin
    /// @param i Pin index
    /// @param dir Pin direction
    void set_dir(uint8_t i, IODir dir);
    /// @brief Gets direction of pin
    /// @param i Pin index
    /// @return Pin direction
    IODir get_dir(uint8_t i);

    /// @brief Put value in port (only if dir is out)
    /// @param i Pin index
    /// @param value Output value
    void put(uint8_t i, bool value);
    /// @brief Gets value from pin (only if dir is in)
    /// @note To get current output value set with put() use get_port()
    /// @param i Pin index
    /// @return Input value
    bool get(uint8_t i);
    /// @brief Gets value set in port (only if dir is out)
    /// @param i Pin index
    /// @return Current output value set with put()
    bool get_port(uint8_t i);

    /// @brief Enables or disables input pull-up (only if dir is in)
    /// @param i Pin index
    /// @param pullUp true to pull up the pin, false to enter Hi-Z
    void set_pull_up(uint8_t i, bool pullUp);
    /// @brief Gets pull-up state for pin (only if dir is in)
    /// @param i Pin index
    /// @return true if pull-up is enabled, false if pin is in Hi-Z
    bool get_pull_up(uint8_t i);

    /// @brief Gets if value is falling from pin (only if dir is in)
    /// @param i Pin index
    /// @return True if value is falling
    /// @note Call from IRQ callback
    bool get_falling(uint8_t i);
    /// @brief Gets if value is rising from pin (only if dir is in)
    /// @param i Pin index
    /// @return True if value is rising
    /// @note Call from IRQ callback
    bool get_rising(uint8_t i);

    /// @brief Set pin interrupt state
    /// @param i Pin index
    /// @param enable true to enable interrupts for this pin, false to disable
    void
    set_pin_irq(uint8_t i, bool enable);
    /// @brief Gets pin interrupt state
    /// @param i Pin index
    /// @return true if interrupts are enabled for this pin, false if disabled
    bool get_pin_irq(uint8_t i);
} IOPort32, IOPort;

#define NULL_U8 ((volatile uint8_t)0)
#define _SFR_PTR_NULL ((volatile uint8_t *)0x00)
#define _SFR_PTR(io_addr) ((volatile uint8_t *)((io_addr) + __SFR_OFFSET))
#define _SFR_MEM_PTR(mem_addr) ((volatile uint8_t *)mem_addr)
#define io_port_null ((IOPort8){NULL_U8, NULL_U8, NULL_U8, _SFR_PTR_NULL, _SFR_PTR_NULL, _SFR_PTR_NULL, _SFR_PTR_NULL, true})

#if defined(__AVR_ATmega328P__)
#define io_port_a io_port_null // port does not exist on this target
#define io_port_b ((IOPort8){_SFR_IO8(0x05), _SFR_IO8(0x04), PCIE0, _SFR_PTR(0x03), _SFR_PTR(0x05), _SFR_PTR(0x04), _SFR_MEM_PTR(0x6B), false})
#define io_port_c ((IOPort8){_SFR_IO8(0x08), _SFR_IO8(0x07), PCIE1, _SFR_PTR(0x06), _SFR_PTR(0x08), _SFR_PTR(0x07), _SFR_MEM_PTR(0x6C), false})
#define io_port_d ((IOPort8){_SFR_IO8(0x0B), _SFR_IO8(0x0A), PCIE2, _SFR_PTR(0x09), _SFR_PTR(0x0B), _SFR_PTR(0x0A), _SFR_MEM_PTR(0x6D), false})
#else
#error Unknown controller target
#endif

#define io_port_default ((IOPort32){io_port_a, io_port_b, io_port_c, io_port_d})

// IOPort32 pin index definitions
#ifndef IOPORT_DISABLE_32
#define _A0 0
#define _A1 1
#define _A2 2
#define _A3 3
#define _A4 4
#define _A5 5
#define _A6 6
#define _A7 7

#define _B0 8
#define _B1 9
#define _B2 10
#define _B3 11
#define _B4 12
#define _B5 13
#define _B6 14
#define _B7 15

#define _C0 16
#define _C1 17
#define _C2 18
#define _C3 19
#define _C4 20
#define _C5 21
#define _C6 22
#define _C7 23

#define _D0 24
#define _D1 25
#define _D2 26
#define _D3 27
#define _D4 28
#define _D5 29
#define _D6 30
#define _D7 31
#endif

#endif

#ifdef IOPORT_NO_PANIC
// empty panic
#ifndef IOPORT_PANIC
#define IOPORT_PANIC ;
#endif
#endif

#ifndef IOPORT_PANIC
// default panic: strobe all ports
#define IOPORT_PANIC                  \
    while (1)                         \
    {                                 \
        _delay_ms(40);                \
        DDRB = DDRC = DDRD = 0xFF;    \
        PORTB = PORTC = PORTD = 0xFF; \
        _delay_ms(40);                \
        PORTB = PORTC = PORTD = 0x00; \
    };
#endif