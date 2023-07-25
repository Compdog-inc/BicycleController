#include "ioutils.h"

static StaticList<IOPort8> irq_list(IO_IRQ_MAX_COUNT);

static void global_irq_handler(uint8_t pcgroup)
{
    for (uint8_t i = 0; i < irq_list.Count(); i++)
    {
        irq_list.Get(i)->irq_handler(pcgroup);
    }
}

#if IO_IRQ_PCINT0
ISR(PCINT0_vect)
{
    global_irq_handler(0);
}
#endif

#if IO_IRQ_PCINT1
ISR(PCINT1_vect)
{
    global_irq_handler(1);
}
#endif

#if IO_IRQ_PCINT2
ISR(PCINT2_vect)
{
    global_irq_handler(2);
}
#endif

IOPort8::IOPort8(uint8_t port, uint8_t ddr, uint8_t irqIndex, volatile uint8_t *_pinaddr, volatile uint8_t *_portaddr, volatile uint8_t *_ddraddr, volatile uint8_t *_pcmskaddr, bool is_null)
{
    this->is_null = is_null;
    if (is_null)
        return;
    this->ddr = ddr;
    this->lastPin = (*_pinaddr);
    this->irqIndex = irqIndex;
    this->_pinaddr = _pinaddr;
    this->_portaddr = _portaddr;
    this->_ddraddr = _ddraddr;
    this->_pcmskaddr = _pcmskaddr;

    this->_histbuf = 0x00;

    // in (ddr:0) and pull-up (port:1)
    this->pullUpMap = (~(*_ddraddr)) & (*_portaddr);
}

void IOPort8::reset()
{
    if (is_null)
        return;
    // reset AVR io
    ddr = 0x00;
    lastPin = (*_pinaddr);
    (*_portaddr) = 0x00;
    (*_ddraddr) = 0x00;
    pullUpMap = 0x00;
    _histbuf = 0x00;

#ifndef IOPORT_NO_MCUCR
    MCUCR &= ~(1 << PUD); // enable pull ups
#endif
}

void IOPort8::reset(uint8_t i)
{
    if (is_null)
        return;
    // set ddr to 0 (in)
    ddr &= ~(1 << i);
    (*_ddraddr) &= ~(1 << i);
    // disable output
    port &= ~(1 << i);
    // disable pull-up (Hi-Z)
    (*_portaddr) &= ~(1 << i);
    pullUpMap &= ~(1 << i);
    _histbuf &= ~(1 << i);
}

void IOPort8::set_dir(uint8_t i, IODir dir)
{
    if (is_null)
        return;

    switch (dir)
    {
    case IODir::In:
        // set ddr to 0 (in)
        ddr &= ~(1 << i);
        (*_ddraddr) &= ~(1 << i);

        // restore pull-up setting
        if (pullUpMap & (1 << i))
            (*_portaddr) |= (1 << i);
        else
            (*_portaddr) &= ~(1 << i);
        break;
    case IODir::Out:
        // set ddr to 1 (out)
        ddr |= (1 << i);
        (*_ddraddr) |= (1 << i);

        // restore port setting
        if (port & (1 << i))
            (*_portaddr) |= (1 << i);
        else
            (*_portaddr) &= ~(1 << i);
        break;
    }
}

IODir IOPort8::get_dir(uint8_t i)
{
    if (is_null)
        return IODir::In;

    return (ddr & (1 << i)) ? IODir::Out : IODir::In;
}

void IOPort8::put(uint8_t i, bool value)
{
    if (is_null)
        return;

    // update internal buffer regardless of direction
    if (value)
        port |= (1 << i);
    else
        port &= ~(1 << i);

    if (get_dir(i) == IODir::Out)
    {
        // update port setting only if pin dir is out
        if (port & (1 << i))
            (*_portaddr) |= (1 << i);
        else
            (*_portaddr) &= ~(1 << i);
    }
}

bool IOPort8::get(uint8_t i)
{
    if (is_null)
        return false;

    if (get_dir(i) == IODir::In)
    {
        return ((*_pinaddr) & (1 << i)) != 0;
    }

    return false; // invalid pin direction
}

bool IOPort8::get_port(uint8_t i)
{
    if (is_null)
        return false;

    // direction doesn't really matter because of internal buffer
    return (port & (1 << i)) != 0;
}

void IOPort8::set_pull_up(uint8_t i, bool pullUp)
{
    if (is_null)
        return;

    // update internal buffer regardless of direction
    if (pullUp)
        pullUpMap |= (1 << i);
    else
        pullUpMap &= ~(1 << i);

    if (get_dir(i) == IODir::In)
    {
        // update port setting only if pin dir is in
        if (pullUpMap & (1 << i))
            (*_portaddr) |= (1 << i);
        else
            (*_portaddr) &= ~(1 << i);
    }
}

bool IOPort8::get_pull_up(uint8_t i)
{
    if (is_null)
        return false;

    // direction doesn't really matter because of internal buffer
    return (pullUpMap & (1 << i)) != 0;
}

bool IOPort8::get_falling(uint8_t i)
{
    if (is_null)
        return false;

    if (
        get_dir(i) == IODir::In &&
        (_histbuf & (1 << i)) // only return value if history enabled for this pin
    )
    {
        return !((*_pinaddr) & (1 << i)) && (lastPin & (1 << i));
    }
    return false;
}

bool IOPort8::get_rising(uint8_t i)
{
    if (is_null)
        return false;

    if (
        get_dir(i) == IODir::In &&
        (_histbuf & (1 << i)) // only return value if history enabled for this pin
    )
    {
        return ((*_pinaddr) & (1 << i)) && !(lastPin & (1 << i));
    }
    return false;
}

void IOPort8::enable_irq_handler()
{
    if (is_null)
        return;

    PCICR |= (1 << irqIndex);
    irq_list.Add(this);
}

void IOPort8::disable_irq_handler()
{
    if (is_null)
        return;

    PCICR &= ~(1 << irqIndex);
    irq_list.Remove(irq_list.Find(this));
}

void IOPort8::set_irq_callback(IOPinChangeCallback callback, IRQCallbackMode callbackMode)
{
    if (is_null)
        return;

    irq_callback = callback;
    callback_mode = callbackMode;
}

IRQCallbackMode IOPort8::get_irq_callback_mode()
{
    if (is_null)
        return IRQCallbackMode::PerPin;

    return callback_mode;
}

void IOPort8::set_pin_irq(uint8_t i, bool enable)
{
    if (is_null)
        return;

    if (enable)
        (*_pcmskaddr) |= (1 << i);
    else
        (*_pcmskaddr) &= ~(1 << i);
}

bool IOPort8::get_pin_irq(uint8_t i)
{
    if (is_null)
        return false;

    return ((*_pcmskaddr) & (1 << i)) != 0;
}

void IOPort8::irq_handler(uint8_t pcgroup)
{
    if (is_null)
        return;

    if (pcgroup == irqIndex) // only listen for target group
    {
        uint8_t _mem = (*_pinaddr);
        // get changed pins
        uint8_t change = _mem ^ lastPin;

        switch (callback_mode)
        {
        case IRQCallbackMode::PerPin:
#ifdef IOPORT_SHARED_HISTORY
            _histbuf = change;
#endif
            // call irq callback for each pin
            for (uint8_t i = 0; i < 8; i++)
            {
                if (change & (1 << i))
                {
#ifndef IOPORT_SHARED_HISTORY
                    // enable only current pin for history (when not in shared mode)
                    _histbuf = (1 << i);
#endif
                    irq_callback(this, i);
                }
            }
            break;
        case IRQCallbackMode::Batch:
            _histbuf = change;
            irq_callback(this, change);
            break;
        }

        lastPin = _mem;
    }
}

IOPort32::IOPort32(IOPort8 a, IOPort8 b, IOPort8 c, IOPort8 d) : port_a(a), port_b(b), port_c(c), port_d(d)
{
}

uint8_t IOPort32::getPinIndex(IOPort8 *port8, uint8_t localIndex)
{
#ifndef IOPORT_DISABLE_32
    // offset local index by first index in IOPort32 segment
    if (*port8 == port_a)
    {
        return localIndex + _A0;
    }
    else if (*port8 == port_b)
    {
        return localIndex + _B0;
    }
    else if (*port8 == port_c)
    {
        return localIndex + _C0;
    }
    else if (*port8 == port_d)
    {
        return localIndex + _D0;
    }
#else
    // IOPort32 pin definitions were excluded from this build
    return localIndex;
#endif
}

bool IOPort32::getLocalPort(uint8_t i, IOPort8 **port8, uint8_t *localIndex)
{
#ifndef IOPORT_DISABLE_32
    if (i >= _A0 && i <= _A7)
    {
        *port8 = &port_a;
        *localIndex = i - _A0;
        return true;
    }
    else if (i >= _B0 && i <= _B7)
    {
        *port8 = &port_b;
        *localIndex = i - _B0;
        return true;
    }
    else if (i >= _C0 && i <= _C7)
    {
        *port8 = &port_c;
        *localIndex = i - _C0;
        return true;
    }
    else if (i >= _D0 && i <= _D7)
    {
        *port8 = &port_d;
        *localIndex = i - _D0;
        return true;
    }
    return false;
#else
    // IOPort32 pin definitions were excluded from this build
    return false;
#endif
}

void IOPort32::reset()
{
    port_a.reset();
    port_b.reset();
    port_c.reset();
    port_d.reset();
}

void IOPort32::reset(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        port->reset(ind);
    else
        IOPORT_PANIC;
}

void IOPort32::set_dir(uint8_t i, IODir dir)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        port->set_dir(ind, dir);
    else
        IOPORT_PANIC;
}

IODir IOPort32::get_dir(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_dir(ind);
    return IODir::In;
}

void IOPort32::put(uint8_t i, bool value)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        port->put(ind, value);
    else
        IOPORT_PANIC;
}

bool IOPort32::get(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get(ind);
    return false;
}

bool IOPort32::get_port(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_port(ind);
    return false;
}

void IOPort32::set_pull_up(uint8_t i, bool pullUp)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        port->set_pull_up(ind, pullUp);
    else
        IOPORT_PANIC;
}

bool IOPort32::get_pull_up(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_pull_up(ind);
    return false;
}

bool IOPort32::get_falling(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_falling(ind);
    return false;
}

bool IOPort32::get_rising(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_rising(ind);
    return false;
}

void IOPort32::set_pin_irq(uint8_t i, bool enable)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        port->set_pin_irq(ind, enable);
    else
        IOPORT_PANIC;
}

bool IOPort32::get_pin_irq(uint8_t i)
{
    IOPort8 *port;
    uint8_t ind;
    if (getLocalPort(i, &port, &ind))
        return port->get_pin_irq(ind);
    return false;
}