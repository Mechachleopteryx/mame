// license:GPL-2.0+
// copyright-holders:Raphael Nabet, Robbbert
/*
    Experimental tm990/189 ("University Module") driver.

    The tm990/189 is a simple board built around a tms9980 at 2.0 MHz.
    The board features:
    * a calculator-like alphanumeric keyboard, a 10-digit 8-segment display,
      a sound buzzer and 4 status LEDs
    * a 4kb ROM socket (0x3000-0x3fff), and a 2kb ROM socket (0x0800-0x0fff)
    * 1kb of RAM expandable to 2kb (0x0000-0x03ff and 0x0400-0x07ff)
    * a tms9901 controlling a custom parallel I/O port (available for
      expansion)
    * an optional on-board serial interface (either TTY or RS232): TI ROMs
      support a terminal connected to this port
    * an optional tape interface
    * an optional bus extension port for adding additional custom devices (TI
      sold a video controller expansion built around a tms9918, which was
      supported by University Basic)

    One tms9901 is set up so that it handles tms9980 interrupts.  The other
    tms9901, the tms9902, and extension cards can trigger interrupts on the
    interrupt-handling tms9901.

    TI sold two ROM sets for this machine: a monitor and assembler ("UNIBUG",
    packaged as one 4kb EPROM) and a Basic interpreter ("University BASIC",
    packaged as a 4kb and a 2kb EPROM).  Users could burn and install custom
    ROM sets, too.

    This board was sold to universities to learn either assembly language or
    BASIC programming.

    A few hobbyists may have bought one of these, too.  This board can actually
    be used as a development kit for the tms9980, but it was not supported as
    such (there was no EPROM programmer or mass storage system for the
    tm990/189, though you could definitively design your own and attach them to
    the extension port).

    - Raphael Nabet 2003

    Bug - The input buffer of character segments isn't fully cleared. If you
          press Shift, then Z enough times, garbage appears. This is because
          the boot process should have set 18C-1CB to FF, but only sets up to 1B3.

    Need a dump of the UNIBUG rom.

    Demo programs for the 990189v: You can get impressive colour displays (with
    sprites) from the 4 included demos. Press the Enter key after each instruction,
    and wait for the READY prompt before proceeding to the next.

    NEW
    LOADx (where x = 0,1,2,3)
    RUN

    University BASIC fully supports the tms9918 videocard option. For example, enter
    COLOR x (where x = 1 to 15), to get a coloured background.

    - Robbbert 2011

******************************************************************************************/

#include "emu.h"

#include "cpu/tms9900/tms9980a.h"
#include "imagedev/cassette.h"
#include "machine/timer.h"
#include "machine/tms9901.h"
#include "machine/tms9902.h"
#include "sound/spkrdev.h"
#include "video/tms9928a.h"

#include "speaker.h"

#include "tm990189.lh"
#include "tm990189v.lh"


#define TMS9901_0_TAG "tms9901_usr"
#define TMS9901_1_TAG "tms9901_sys"

class tm990189_state : public driver_device
{
	friend class tm990_189_rs232_image_device;

public:
	tm990189_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_tms9980a(*this, "maincpu")
		, m_speaker(*this, "speaker")
		, m_cass(*this, "cassette")
		, m_tms9918(*this, "tms9918")
		, m_tms9901_usr(*this, TMS9901_0_TAG)
		, m_tms9901_sys(*this, TMS9901_1_TAG)
		, m_tms9902(*this, "tms9902")
		, m_digits(*this, "digit%u", 0U)
		, m_leds(*this, "led%u", 0U)
	{ }

	void tm990_189_v(machine_config &config);
	void tm990_189(machine_config &config);

	DECLARE_INPUT_CHANGED_MEMBER( load_interrupt );

private:
	uint8_t video_vdp_r(offs_t offset);
	void video_vdp_w(offs_t offset, uint8_t data);
	uint8_t video_joy_r();
	void video_joy_w(uint8_t data);

	void external_operation(offs_t offset, uint8_t data);

	DECLARE_WRITE_LINE_MEMBER(usr9901_led0_w);
	DECLARE_WRITE_LINE_MEMBER(usr9901_led1_w);
	DECLARE_WRITE_LINE_MEMBER(usr9901_led2_w);
	DECLARE_WRITE_LINE_MEMBER(usr9901_led3_w);
	DECLARE_WRITE_LINE_MEMBER(usr9901_interrupt_callback);

	DECLARE_WRITE_LINE_MEMBER(sys9901_interrupt_callback);
	uint8_t sys9901_r(offs_t offset);
	DECLARE_WRITE_LINE_MEMBER(sys9901_digitsel0_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_digitsel1_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_digitsel2_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_digitsel3_w);

	DECLARE_WRITE_LINE_MEMBER(sys9901_segment0_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment1_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment2_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment3_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment4_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment5_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment6_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_segment7_w);

	DECLARE_WRITE_LINE_MEMBER(sys9901_dsplytrgr_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_shiftlight_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_spkrdrive_w);
	DECLARE_WRITE_LINE_MEMBER(sys9901_tapewdata_w);

	void xmit_callback(uint8_t data);
	DECLARE_MACHINE_START(tm990_189);
	DECLARE_MACHINE_RESET(tm990_189);
	DECLARE_MACHINE_START(tm990_189_v);
	DECLARE_MACHINE_RESET(tm990_189_v);

	emu_timer *m_rs232_input_timer;

	void tm990_189_cru_map(address_map &map);
	void tm990_189_memmap(address_map &map);
	void tm990_189_v_memmap(address_map &map);

	void draw_digit(void);
	void led_set(int number, bool state);
	void segment_set(int offset, bool state);
	void digitsel(int offset, bool state);

	TIMER_DEVICE_CALLBACK_MEMBER(display_callback);
	TIMER_CALLBACK_MEMBER(clear_load);
	void hold_load();

	required_device<tms9980a_device> m_tms9980a;
	required_device<speaker_sound_device> m_speaker;
	required_device<cassette_image_device> m_cass;
	optional_device<tms9918_device> m_tms9918;

	required_device<tms9901_device> m_tms9901_usr;
	required_device<tms9901_device> m_tms9901_sys;
	required_device<tms9902_device> m_tms9902;
	output_finder<10> m_digits;
	output_finder<7> m_leds;

	int m_load_state = 0;

	int m_digitsel = 0;
	int m_segment = 0;
	emu_timer *m_displayena_timer;
	uint8_t m_segment_state[10]{};
	uint8_t m_old_segment_state[10]{};
	uint8_t m_LED_state = 0U;
	emu_timer *m_joy1x_timer;
	emu_timer *m_joy1y_timer;
	emu_timer *m_joy2x_timer;
	emu_timer *m_joy2y_timer;
	device_image_interface *m_rs232_fp;
	//uint8_t m_rs232_rts;
	uint8_t m_bogus_read_save = 0U;
};


#define displayena_duration attotime::from_usec(4500)   /* Can anyone confirm this? 74LS123 connected to C=0.1uF and R=100kOhm */

MACHINE_RESET_MEMBER(tm990189_state,tm990_189)
{
	m_tms9980a->set_ready(ASSERT_LINE);
	m_tms9980a->set_hold(CLEAR_LINE);
	hold_load();
}

MACHINE_START_MEMBER(tm990189_state,tm990_189)
{
	m_digits.resolve();
	m_leds.resolve();
	m_displayena_timer = machine().scheduler().timer_alloc(timer_expired_delegate());

	m_digitsel = 0;
	m_LED_state = 0;
}

MACHINE_START_MEMBER(tm990189_state,tm990_189_v)
{
	m_digits.resolve();
	m_leds.resolve();
	m_displayena_timer = machine().scheduler().timer_alloc(timer_expired_delegate());

	m_joy1x_timer = machine().scheduler().timer_alloc(timer_expired_delegate());
	m_joy1y_timer = machine().scheduler().timer_alloc(timer_expired_delegate());
	m_joy2x_timer = machine().scheduler().timer_alloc(timer_expired_delegate());
	m_joy2y_timer = machine().scheduler().timer_alloc(timer_expired_delegate());

	m_digitsel = 0;
	m_LED_state = 0;
}

MACHINE_RESET_MEMBER(tm990189_state,tm990_189_v)
{
	m_tms9980a->set_ready(ASSERT_LINE);
	m_tms9980a->set_hold(CLEAR_LINE);
	hold_load();
}

/*
    hold and debounce load line (emulation is inaccurate)
*/

TIMER_CALLBACK_MEMBER(tm990189_state::clear_load)
{
	m_load_state = false;
	m_tms9980a->set_input_line(INT_9980A_LOAD, CLEAR_LINE);
}

void tm990189_state::hold_load()
{
	m_load_state = true;
	m_tms9980a->set_input_line(INT_9980A_LOAD, ASSERT_LINE);
	machine().scheduler().timer_set(attotime::from_msec(100), timer_expired_delegate(FUNC(tm990189_state::clear_load),this));
}

/*
    LOAD interrupt switch
*/
INPUT_CHANGED_MEMBER( tm990189_state::load_interrupt )
{
	// When depressed, fire LOAD (neg logic)
	if (newval==CLEAR_LINE) hold_load();
}


/*
    tm990_189 video emulation.

    Has an integrated 10 digit 8-segment display.
    Supports EIA and TTY terminals, and an optional 9918 controller.
*/

void tm990189_state::draw_digit()
{
	m_segment_state[m_digitsel] |= ~m_segment;
}


TIMER_DEVICE_CALLBACK_MEMBER(tm990189_state::display_callback)
{
	// since the segment data is cleared after being used, the old_segment is there
	// in case the segment data hasn't been refreshed yet.
	for (uint8_t i = 0; i < 10; i++)
	{
		m_old_segment_state[i] |= m_segment_state[i];
		m_digits[i] = m_old_segment_state[i];
		m_old_segment_state[i] = m_segment_state[i];
		m_segment_state[i] = 0;
	}

	for (uint8_t i = 0; i < 7; i++)
		m_leds[i] = !BIT(m_LED_state, i);
}

/*
    tms9901 code
*/

WRITE_LINE_MEMBER( tm990189_state::usr9901_interrupt_callback )
{
	// Triggered by internal timer (set by ROM to 1.6 ms cycle) on level 3
	// or by keyboard interrupt (level 6)
	if (!m_load_state)
	{
		m_tms9980a->set_input_line(m_tms9901_usr->get_int_level() & 7, ASSERT_LINE);
	}
}

void tm990189_state::led_set(int offset, bool state)
{
	if (state)
		m_LED_state |= (1 << offset);
	else
		m_LED_state &= ~(1 << offset);
}

WRITE_LINE_MEMBER( tm990189_state::usr9901_led0_w )
{
	led_set(0, state);
}

WRITE_LINE_MEMBER( tm990189_state::usr9901_led1_w )
{
	led_set(1, state);
}

WRITE_LINE_MEMBER( tm990189_state::usr9901_led2_w )
{
	led_set(2, state);
}

WRITE_LINE_MEMBER( tm990189_state::usr9901_led3_w )
{
	led_set(3, state);
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_interrupt_callback )
{
	// TODO: Check this
	m_tms9901_usr->set_int_line(5, state);
}

uint8_t tm990189_state::sys9901_r(offs_t offset)
{
	// |-|Cass|K|K|K|K|K|C|
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8" };

	offset &= 0x0F;
	switch (offset)
	{
		case tms9901_device::INT1:
		case tms9901_device::INT2:
		case tms9901_device::INT3:
		case tms9901_device::INT4:
		case tms9901_device::INT5:
			if (m_digitsel < 9)
				return BIT(ioport(keynames[m_digitsel])->read(), offset-tms9901_device::INT1);
			else return 0;

		case tms9901_device::INT6:
			return (m_cass->input() > 0);
		default:
			return 0;
	}
}

/*
      uint8_t data = 0;
      if (offset == tms9901_device::CB_INT7)
      {
              static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8" };
              // keyboard read
              if (m_digitsel < 9)
                    data |= ioport(keynames[m_digitsel])->read() << 1;
              // tape input
              if (m_cass->input() > 0.0)
                      data |= 0x40;
      }
      return data;
*/

void tm990189_state::digitsel(int offset, bool state)
{
	if (state)
		m_digitsel |= 1 << offset;
	else
		m_digitsel &= ~ (1 << offset);
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_digitsel0_w )
{
	digitsel(0, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_digitsel1_w )
{
	digitsel(1, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_digitsel2_w )
{
	digitsel(2, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_digitsel3_w )
{
	digitsel(3, state);
}


void tm990189_state::segment_set(int offset, bool state)
{
	if (state)
		m_segment |= 1 << offset;
	else
	{
		m_segment &= ~ (1 << offset);
		if ((m_displayena_timer->remaining() > attotime::zero)  && (m_digitsel < 10))
			draw_digit();
	}
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_segment0_w )
{
	segment_set(0, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment1_w )
{
	segment_set(1, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment2_w )
{
	segment_set(2, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment3_w )
{
	segment_set(3, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment4_w )
{
	segment_set(4, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment5_w )
{
	segment_set(5, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment6_w )
{
	segment_set(6, state);
}
WRITE_LINE_MEMBER( tm990189_state::sys9901_segment7_w )
{
	segment_set(7, state);
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_dsplytrgr_w )
{
	if ((!state) && (m_digitsel < 10))
	{
		m_displayena_timer->reset(displayena_duration);
		draw_digit();
	}
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_shiftlight_w )
{
	if (state)
		m_LED_state |= 0x10;
	else
		m_LED_state &= ~0x10;
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_spkrdrive_w )
{
	m_speaker->level_w(state);
}

WRITE_LINE_MEMBER( tm990189_state::sys9901_tapewdata_w )
{
	m_cass->output(state ? +1.0 : -1.0);
}

class tm990_189_rs232_image_device : public device_t, public device_image_interface
{
public:
	// construction/destruction
	template <typename T> tm990_189_rs232_image_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock, T &&tms_tag)
		: tm990_189_rs232_image_device(mconfig, tag, owner, clock)
	{
		m_tms9902.set_tag(std::forward<T>(tms_tag));
	}

	tm990_189_rs232_image_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// image-level overrides
	virtual bool is_readable()  const noexcept override { return true; }
	virtual bool is_writeable() const noexcept override { return true; }
	virtual bool is_creatable() const noexcept override { return true; }
	virtual bool is_reset_on_load() const noexcept override { return false; }
	virtual bool support_command_line_image_creation() const noexcept override { return true; }
	virtual const char *file_extensions() const noexcept override { return ""; }
	virtual const char *image_type_name() const noexcept override { return "serial"; }
	virtual const char *image_brief_type_name() const noexcept override { return "serl"; }

	virtual image_init_result call_load() override;
	virtual void call_unload() override;

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param) override;

	required_device<tms9902_device> m_tms9902;
};

DEFINE_DEVICE_TYPE(TM990_189_RS232, tm990_189_rs232_image_device, "tm990_189_rs232_image", "TM990/189 RS232 port")

tm990_189_rs232_image_device::tm990_189_rs232_image_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, TM990_189_RS232, tag, owner, clock)
	, device_image_interface(mconfig, *this)
	, m_tms9902(*this, finder_base::DUMMY_TAG)
{
}

void tm990_189_rs232_image_device::device_start()
{
}

void tm990_189_rs232_image_device::device_timer(emu_timer &timer, device_timer_id id, int param)
{
	uint8_t buf;
	if (/*m_rs232_rts &&*/ /*(mame_ftell(m_rs232_fp) < mame_fsize(m_rs232_fp))*/1)
	{
		if (fread(&buf, 1) == 1)
			m_tms9902->rcv_data(buf);
	}
}

image_init_result tm990_189_rs232_image_device::call_load()
{
	tm990189_state *state = machine().driver_data<tm990189_state>();
	m_tms9902->rcv_dsr(ASSERT_LINE);
	state->m_rs232_input_timer = timer_alloc();
	state->m_rs232_input_timer->adjust(attotime::zero, 0, attotime::from_msec(10));
	return image_init_result::PASS;
}


void tm990_189_rs232_image_device::call_unload()
{
	tm990189_state *state = machine().driver_data<tm990189_state>();
	m_tms9902->rcv_dsr(CLEAR_LINE);

	state->m_rs232_input_timer->reset();    /* FIXME - timers should only be allocated once */
}


/* static TMS9902_RTS_CALLBACK( rts_callback )
{
    tm990189 *state = device->machine().driver_data<tm990189>();
    state->m_rs232_rts = RTS;
    tms9902->set_cts(RTS);
} */

void tm990189_state::xmit_callback(uint8_t data)
{
	uint8_t buf = data;
	if (m_rs232_fp) m_rs232_fp->fwrite(&buf, 1);
}

/*
    External instruction decoding
*/
void tm990189_state::external_operation(offs_t offset, uint8_t data)
{
	switch (offset)
	{
	case 2: // IDLE
		if (data)
			m_LED_state |= 0x40;
		else
			m_LED_state &= ~0x40;
		break;
	case 3: // RSET
		// Not used on the default board
		break;
	case 5: // CKON: set DECKCONTROL
		m_LED_state |= 0x20;
		m_cass->change_state(CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		break;
	case 6: // CKOF: clear DECKCONTROL
		m_LED_state &= ~0x20;
		m_cass->change_state(CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		break;
	case 7: // LREX: trigger LOAD
		hold_load();
		break;
	default: // undefined
		break;
	}
}

/*
    Video Board handling
*/

uint8_t tm990189_state::video_vdp_r(offs_t offset)
{
	int reply = 0;

	/* When the tms9980 reads @>2000 or @>2001, it actually does a word access:
	it reads @>2000 first, then @>2001.  According to schematics, both access
	are decoded to the VDP: read accesses are therefore bogus, all the more so
	since the two reads are too close (1us) for the VDP to be able to reload
	the read buffer: the read address pointer is probably incremented by 2, but
	only the first byte is valid.  There is a work around for this problem: all
	you need is reloading the address pointer before each read.  However,
	software always uses the second byte, which is very weird, particularly
	for the status port.  Presumably, since the read buffer has not been
	reloaded, the second byte read from the memory read port is equal to the
	first; however, this explanation is not very convincing for the status
	port.  Still, I do not have any better explanation, so I will stick with
	it. */

	if (offset & 2)
		reply = m_tms9918->register_read();
	else
		reply = m_tms9918->vram_read();

	if (!(offset & 1))
		m_bogus_read_save = reply;
	else
		reply = m_bogus_read_save;

	return reply;
}

void tm990189_state::video_vdp_w(offs_t offset, uint8_t data)
{
	if (offset & 1)
	{
		if (offset & 2)
			m_tms9918->register_write(data);
		else
			m_tms9918->vram_write(data);
	}
}

uint8_t tm990189_state::video_joy_r()
{
	uint8_t data = ioport("BUTTONS")->read();

	if (m_joy1x_timer->remaining() < attotime::zero)
		data |= 0x01;

	if (m_joy1y_timer->remaining() < attotime::zero)
		data |= 0x02;

	if (m_joy2x_timer->remaining() < attotime::zero)
		data |= 0x08;

	if (m_joy2y_timer->remaining() < attotime::zero)
		data |= 0x10;

	return data;
}

void tm990189_state::video_joy_w(uint8_t data)
{
	m_joy1x_timer->reset(attotime::from_usec(ioport("JOY1_X")->read()*28+28));
	m_joy1y_timer->reset(attotime::from_usec(ioport("JOY1_Y")->read()*28+28));
	m_joy2x_timer->reset(attotime::from_usec(ioport("JOY2_X")->read()*28+28));
	m_joy2y_timer->reset(attotime::from_usec(ioport("JOY2_Y")->read()*28+28));
}

/*
// user tms9901 setup
static const tms9901_interface usr9901reset_param =
{
    tms9901_device::INT1 | tms9901_device::INT2 | tms9901_device::INT3 | tms9901_device::INT4 | tms9901_device::INT5 | tms9901_device::INT6,    // only input pins whose state is always known

    // Read handler. Covers all input lines (see tms9901.h)
    DEVCB_NOOP,

    // write handlers
    {
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, usr9901_led0_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, usr9901_led1_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, usr9901_led2_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, usr9901_led3_w),
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP,
        DEVCB_NOOP
    },

    // interrupt handler
    DEVCB_DRIVER_MEMBER(tm990189_state, usr9901_interrupt_callback)
};
*/

/*
// system tms9901 setup
static const tms9901_interface sys9901reset_param =
{
    0,  // only input pins whose state is always known

    // Read handler. Covers all input lines (see tms9901.h)
    DEVCB_DRIVER_MEMBER(tm990189_state, sys9901_r),

    // write handlers
    {
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_digitsel0_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_digitsel1_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_digitsel2_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_digitsel3_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment0_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment1_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment2_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment3_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment4_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment5_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment6_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_segment7_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_dsplytrgr_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_shiftlight_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_spkrdrive_w),
        DEVCB_DRIVER_LINE_MEMBER(tm990189_state, sys9901_tapewdata_w)
    },

    // interrupt handler
    DEVCB_DRIVER_MEMBER(tm990189_state, sys9901_interrupt_callback)
};

*/

/*
    Memory map:

    0x0000-0x03ff: 1kb RAM
    0x0400-0x07ff: 1kb onboard expansion RAM
    0x0800-0x0bff or 0x0800-0x0fff: 1kb or 2kb onboard expansion ROM
    0x1000-0x2fff: reserved for offboard expansion
        Only known card: Color Video Board
            0x1000-0x17ff (R): Video board ROM 1
            0x1800-0x1fff (R): Video board ROM 2
            0x2000-0x27ff (R): tms9918 read ports (bogus)
                (address & 2) == 0: data port (bogus)
                (address & 2) == 1: status register (bogus)
            0x2800-0x2fff (R): joystick read port
                d2: joy 2 switch
                d3: joy 2 Y (length of pulse after retrigger is proportional to axis position)
                d4: joy 2 X (length of pulse after retrigger is proportional to axis position)
                d2: joy 1 switch
                d3: joy 1 Y (length of pulse after retrigger is proportional to axis position)
                d4: joy 1 X (length of pulse after retrigger is proportional to axis position)
            0x1801-0x1fff ((address & 1) == 1) (W): joystick write port (retrigger)
            0x2801-0x2fff ((address & 1) == 1) (W): tms9918 write ports
                (address & 2) == 0: data port
                (address & 2) == 1: control register
    0x3000-0x3fff: 4kb onboard ROM
*/

void tm990189_state::tm990_189_memmap(address_map &map)
{
	map(0x0000, 0x07ff).ram();                                 /* RAM */
	map(0x0800, 0x0fff).rom();                                 /* extra ROM - application programs with unibug, remaining 2kb of program for university basic */
	map(0x1000, 0x2fff).noprw();                                 /* reserved for expansion (RAM and/or tms9918 video controller) */
	map(0x3000, 0x3fff).rom();                                 /* main ROM - unibug or university basic */
}

void tm990189_state::tm990_189_v_memmap(address_map &map)
{
	map(0x0000, 0x07ff).ram();                                 /* RAM */
	map(0x0800, 0x0fff).rom();                                 /* extra ROM - application programs with unibug, remaining 2kb of program for university basic */

	map(0x1000, 0x17ff).rom().nopw();     /* video board ROM 1 */
	map(0x1800, 0x1fff).rom().w(FUNC(tm990189_state::video_joy_w));   /* video board ROM 2 and joystick write port*/
	map(0x2000, 0x27ff).r(FUNC(tm990189_state::video_vdp_r)).nopw();   /* video board tms9918 read ports (bogus) */
	map(0x2800, 0x2fff).rw(FUNC(tm990189_state::video_joy_r), FUNC(tm990189_state::video_vdp_w)); /* video board joystick read port and tms9918 write ports */

	map(0x3000, 0x3fff).rom();                                 /* main ROM - unibug or university basic */
}

/*
    CRU map

    The board features one tms9901 for keyboard and sound I/O, another tms9901
    to drive the parallel port and a few LEDs and handle tms9980 interrupts,
    and one optional tms9902 for serial I/O.

    * bits >000->1ff (R12=>000->3fe): first TMS9901: parallel port, a few LEDs,
        interrupts
        - CRU bits 1-5: UINT1* through UINT5* (jumper connectable to parallel
            port or COMINT from tms9902)
        - CRU bit 6: KBINT (interrupt request from second tms9901)
        - CRU bits 7-15: mirrors P15-P7
        - CRU bits 16-31: P0-P15 (parallel port)
            (bits 16, 17, 18 and 19 control LEDs numbered 1, 2, 3 and 4, too)
    * bits >200->3ff (R12=>400->7fe): second TMS9901: display, keyboard and
        sound
        - CRU bits 1-5: KB1*-KB5* (inputs from current keyboard row)
        - CRU bit 6: RDATA (tape in)
        - CRU bits 7-15: mirrors P15-P7
        - CRU bits 16-19: DIGITSELA-DIGITSELD (select current display digit and
            keyboard row)
        - CRU bits 20-27: SEGMENTA*-SEGMENTG* and SEGMENTP* (drives digit
            segments)
        - CRU bit 28: DSPLYTRGR*: used as an alive signal for a watchdog circuit
                      which turns off the display when the program hangs and
                      LED segments would continuously emit light

        - bit 29: SHIFTLIGHT (controls shift light)
        - bit 30: SPKRDRIVE (controls buzzer)
        - bit 31: WDATA (tape out)
    * bits >400->5ff (R12=>800->bfe): optional TMS9902
    * bits >600->7ff (R12=>c00->ffe): reserved for expansion
    * write 0 to bits >1000->17ff: IDLE: flash IDLE LED
    * write 0 to bits >1800->1fff: RSET: not connected, but it is decoded and
        hackers can connect any device they like to this pin
    * write 1 to bits >0800->0fff: CKON: light FWD LED and activates
        DECKCONTROL* signal (part of tape interface)
    * write 1 to bits >1000->17ff: CKOF: light off FWD LED and deactivates
        DECKCONTROL* signal (part of tape interface)
    * write 1 to bits >1800->1fff: LREX: trigger load interrupt

    Keyboard map: see input ports

    Display segment designation:
           a
          ___
         |   |
        f|   |b
         |___|
         | g |
        e|   |c
         |___|  .p
           d
*/

void tm990189_state::tm990_189_cru_map(address_map &map)
{
	map(0x0000, 0x03ff).rw(m_tms9901_usr, FUNC(tms9901_device::read), FUNC(tms9901_device::write));    /* user I/O tms9901 */
	map(0x0400, 0x07ff).rw(m_tms9901_sys, FUNC(tms9901_device::read), FUNC(tms9901_device::write));    /* system I/O tms9901 */
	map(0x0800, 0x0bff).rw(m_tms9902, FUNC(tms9902_device::cruread), FUNC(tms9902_device::cruwrite));   /* optional tms9902 */
}

void tm990189_state::tm990_189(machine_config &config)
{
	/* basic machine hardware */
	TMS9980A(config, m_tms9980a, 8_MHz_XTAL); // clock divided by 4 internally
	m_tms9980a->set_addrmap(AS_PROGRAM, &tm990189_state::tm990_189_memmap);
	m_tms9980a->set_addrmap(AS_IO, &tm990189_state::tm990_189_cru_map);
	m_tms9980a->extop_cb().set(FUNC(tm990189_state::external_operation));

	MCFG_MACHINE_START_OVERRIDE(tm990189_state, tm990_189 )
	MCFG_MACHINE_RESET_OVERRIDE(tm990189_state, tm990_189 )

	/* Video hardware */
	config.set_default_layout(layout_tm990189);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	SPEAKER_SOUND(config, "speaker").add_route(ALL_OUTPUTS, "mono", 0.50);

	/* Devices */
	CASSETTE(config, "cassette", 0).add_route(ALL_OUTPUTS, "mono", 0.25);

	TMS9901(config, m_tms9901_usr, 8_MHz_XTAL / 4);
	m_tms9901_usr->p_out_cb(0).set(FUNC(tm990189_state::usr9901_led0_w));
	m_tms9901_usr->p_out_cb(1).set(FUNC(tm990189_state::usr9901_led1_w));
	m_tms9901_usr->p_out_cb(2).set(FUNC(tm990189_state::usr9901_led2_w));
	m_tms9901_usr->p_out_cb(3).set(FUNC(tm990189_state::usr9901_led3_w));
	m_tms9901_usr->intreq_cb().set(FUNC(tm990189_state::usr9901_interrupt_callback));

	TMS9901(config, m_tms9901_sys, 8_MHz_XTAL / 4);
	m_tms9901_sys->read_cb().set(FUNC(tm990189_state::sys9901_r));
	m_tms9901_sys->p_out_cb(0).set(FUNC(tm990189_state::sys9901_digitsel0_w));
	m_tms9901_sys->p_out_cb(1).set(FUNC(tm990189_state::sys9901_digitsel1_w));
	m_tms9901_sys->p_out_cb(2).set(FUNC(tm990189_state::sys9901_digitsel2_w));
	m_tms9901_sys->p_out_cb(3).set(FUNC(tm990189_state::sys9901_digitsel3_w));
	m_tms9901_sys->p_out_cb(4).set(FUNC(tm990189_state::sys9901_segment0_w));
	m_tms9901_sys->p_out_cb(5).set(FUNC(tm990189_state::sys9901_segment1_w));
	m_tms9901_sys->p_out_cb(6).set(FUNC(tm990189_state::sys9901_segment2_w));
	m_tms9901_sys->p_out_cb(7).set(FUNC(tm990189_state::sys9901_segment3_w));
	m_tms9901_sys->p_out_cb(8).set(FUNC(tm990189_state::sys9901_segment4_w));
	m_tms9901_sys->p_out_cb(9).set(FUNC(tm990189_state::sys9901_segment5_w));
	m_tms9901_sys->p_out_cb(10).set(FUNC(tm990189_state::sys9901_segment6_w));
	m_tms9901_sys->p_out_cb(11).set(FUNC(tm990189_state::sys9901_segment7_w));
	m_tms9901_sys->p_out_cb(12).set(FUNC(tm990189_state::sys9901_dsplytrgr_w));
	m_tms9901_sys->p_out_cb(13).set(FUNC(tm990189_state::sys9901_shiftlight_w));
	m_tms9901_sys->p_out_cb(14).set(FUNC(tm990189_state::sys9901_spkrdrive_w));
	m_tms9901_sys->p_out_cb(15).set(FUNC(tm990189_state::sys9901_tapewdata_w));
	m_tms9901_sys->intreq_cb().set(FUNC(tm990189_state::sys9901_interrupt_callback));

	TMS9902(config, m_tms9902, 8_MHz_XTAL / 4);
	m_tms9902->xmit_cb().set(FUNC(tm990189_state::xmit_callback)); // called when a character is transmitted
	TM990_189_RS232(config, "rs232", 0, m_tms9902);

	timer_device &display_timer(TIMER(config, "display_timer"));
	display_timer.configure_periodic(FUNC(tm990189_state::display_callback), attotime::from_hz(30));
	// Need to delay the timer, or it will spoil the initial LOAD
	// TODO: Fix this, probably inside CPU
	display_timer.set_start_delay(attotime::from_msec(150));
}

void tm990189_state::tm990_189_v(machine_config &config)
{
	/* basic machine hardware */
	TMS9980A(config, m_tms9980a, 8_MHz_XTAL);
	m_tms9980a->set_addrmap(AS_PROGRAM, &tm990189_state::tm990_189_v_memmap);
	m_tms9980a->set_addrmap(AS_IO, &tm990189_state::tm990_189_cru_map);
	m_tms9980a->extop_cb().set(FUNC(tm990189_state::external_operation));

	MCFG_MACHINE_START_OVERRIDE(tm990189_state, tm990_189_v )
	MCFG_MACHINE_RESET_OVERRIDE(tm990189_state, tm990_189_v )

	/* video hardware */
	tms9918_device &vdp(TMS9918(config, "tms9918", XTAL(10'738'635)));
	vdp.set_screen("screen");
	vdp.set_vram_size(0x4000);
	SCREEN(config, "screen", SCREEN_TYPE_RASTER);

	config.set_default_layout(layout_tm990189v);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	SPEAKER_SOUND(config, "speaker").add_route(ALL_OUTPUTS, "mono", 0.50);   /* one two-level buzzer */

	/* Devices */
	CASSETTE(config, "cassette", 0).add_route(ALL_OUTPUTS, "mono", 0.25);

	TMS9901(config, m_tms9901_usr, 8_MHz_XTAL / 4);
	m_tms9901_usr->p_out_cb(0).set(FUNC(tm990189_state::usr9901_led0_w));
	m_tms9901_usr->p_out_cb(1).set(FUNC(tm990189_state::usr9901_led1_w));
	m_tms9901_usr->p_out_cb(2).set(FUNC(tm990189_state::usr9901_led2_w));
	m_tms9901_usr->p_out_cb(3).set(FUNC(tm990189_state::usr9901_led3_w));
	m_tms9901_usr->intreq_cb().set(FUNC(tm990189_state::usr9901_interrupt_callback));

	TMS9901(config, m_tms9901_sys, 8_MHz_XTAL / 4);
	m_tms9901_sys->read_cb().set(FUNC(tm990189_state::sys9901_r));
	m_tms9901_sys->p_out_cb(0).set(FUNC(tm990189_state::sys9901_digitsel0_w));
	m_tms9901_sys->p_out_cb(1).set(FUNC(tm990189_state::sys9901_digitsel1_w));
	m_tms9901_sys->p_out_cb(2).set(FUNC(tm990189_state::sys9901_digitsel2_w));
	m_tms9901_sys->p_out_cb(3).set(FUNC(tm990189_state::sys9901_digitsel3_w));
	m_tms9901_sys->p_out_cb(4).set(FUNC(tm990189_state::sys9901_segment0_w));
	m_tms9901_sys->p_out_cb(5).set(FUNC(tm990189_state::sys9901_segment1_w));
	m_tms9901_sys->p_out_cb(6).set(FUNC(tm990189_state::sys9901_segment2_w));
	m_tms9901_sys->p_out_cb(7).set(FUNC(tm990189_state::sys9901_segment3_w));
	m_tms9901_sys->p_out_cb(8).set(FUNC(tm990189_state::sys9901_segment4_w));
	m_tms9901_sys->p_out_cb(9).set(FUNC(tm990189_state::sys9901_segment5_w));
	m_tms9901_sys->p_out_cb(10).set(FUNC(tm990189_state::sys9901_segment6_w));
	m_tms9901_sys->p_out_cb(11).set(FUNC(tm990189_state::sys9901_segment7_w));
	m_tms9901_sys->p_out_cb(12).set(FUNC(tm990189_state::sys9901_dsplytrgr_w));
	m_tms9901_sys->p_out_cb(13).set(FUNC(tm990189_state::sys9901_shiftlight_w));
	m_tms9901_sys->p_out_cb(14).set(FUNC(tm990189_state::sys9901_spkrdrive_w));
	m_tms9901_sys->p_out_cb(15).set(FUNC(tm990189_state::sys9901_tapewdata_w));
	m_tms9901_sys->intreq_cb().set(FUNC(tm990189_state::sys9901_interrupt_callback));

	TMS9902(config, m_tms9902, 8_MHz_XTAL / 4);
	m_tms9902->xmit_cb().set(FUNC(tm990189_state::xmit_callback)); // called when a character is transmitted;
	TM990_189_RS232(config, "rs232", 0, m_tms9902);

	timer_device &display_timer(TIMER(config, "display_timer"));
	display_timer.configure_periodic(FUNC(tm990189_state::display_callback), attotime::from_hz(30));
	display_timer.set_start_delay(attotime::from_msec(150));
}


/*
  ROM loading
*/
ROM_START(990189)
	/*CPU memory space*/
	ROM_REGION(0x4000, "maincpu", 0 )

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb) SHA1(fa9751fd2e3e5d7ae03819fc9c7099e2ddd9fb53))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b) SHA1(96e88f4cb7a374033cdf3af0dc26ca5b1d55b9f9))
ROM_END

ROM_START(990189v)
	/*CPU memory space*/
	ROM_REGION(0x4000, "maincpu", 0 )

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb) SHA1(fa9751fd2e3e5d7ae03819fc9c7099e2ddd9fb53))

	/* extension ROM */
	ROM_LOAD_OPTIONAL("demo1000.u13", 0x1000, 0x0800, CRC(c0e16685) SHA1(d0d314134c42fa4682aafbace67f539f67f6ba65))

	/* extension ROM */
	ROM_LOAD_OPTIONAL("demo1800.u11", 0x1800, 0x0800, CRC(8737dc4b) SHA1(b87da7aa4d3f909e70f885c4b36999cc1abf5764))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b) SHA1(96e88f4cb7a374033cdf3af0dc26ca5b1d55b9f9))
	/*ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744) SHA1(47afe7f6b04b564d2f30f21461e0ed7ea97fba4c) )*/ /* older, partial dump of university BASIC */
ROM_END

#define JOYSTICK_DELTA          10
#define JOYSTICK_SENSITIVITY    100

static INPUT_PORTS_START(tm990_189)

	PORT_START( "LOADINT")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Load interrupt") PORT_CODE(KEYCODE_PRTSCR) PORT_CHANGED_MEMBER(DEVICE_SELF, tm990189_state, load_interrupt, 1)

	/* 45-key calculator-like alphanumeric keyboard... */
	PORT_START("LINE0")    /* row 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Sp *") PORT_CODE(KEYCODE_SPACE)  PORT_CHAR(' ')  PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ret '") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)   PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("$ =") PORT_CODE(KEYCODE_STOP)    PORT_CHAR('$')  PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)   PORT_CHAR(',')  PORT_CHAR('<')

	PORT_START("LINE1")    /* row 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ (") PORT_CODE(KEYCODE_OPENBRACE)   PORT_CHAR('+')  PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- )") PORT_CODE(KEYCODE_CLOSEBRACE)  PORT_CHAR('-')  PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ /") PORT_CODE(KEYCODE_MINUS)   PORT_CHAR('@')  PORT_CHAR('/')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("> %") PORT_CODE(KEYCODE_EQUALS)  PORT_CHAR('>')  PORT_CHAR('%')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 ^") PORT_CODE(KEYCODE_0)   PORT_CHAR('0')  PORT_CHAR('^')

	PORT_START("LINE2")    /* row 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 .") PORT_CODE(KEYCODE_1)   PORT_CHAR('1')  PORT_CHAR('.')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 ;") PORT_CODE(KEYCODE_2)   PORT_CHAR('2')  PORT_CHAR(';')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 :") PORT_CODE(KEYCODE_3)   PORT_CHAR('3')  PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 ?") PORT_CODE(KEYCODE_4)   PORT_CHAR('4')  PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 !") PORT_CODE(KEYCODE_5)   PORT_CHAR('5')  PORT_CHAR('!')

	PORT_START("LINE3")    /* row 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 _") PORT_CODE(KEYCODE_6)   PORT_CHAR('6')  PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \"") PORT_CODE(KEYCODE_7)  PORT_CHAR('7')  PORT_CHAR('\"')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 #") PORT_CODE(KEYCODE_8)   PORT_CHAR('8')  PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (ESC)") PORT_CODE(KEYCODE_9)   PORT_CHAR('9')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A (SOH)") PORT_CODE(KEYCODE_A)   PORT_CHAR('A')

	PORT_START("LINE4")    /* row 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B (STH)") PORT_CODE(KEYCODE_B)   PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C (ETX)") PORT_CODE(KEYCODE_C)   PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D (EOT)") PORT_CODE(KEYCODE_D)   PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E (ENQ)") PORT_CODE(KEYCODE_E)   PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F (ACK)") PORT_CODE(KEYCODE_F)   PORT_CHAR('F')

	PORT_START("LINE5")    /* row 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G (BEL)") PORT_CODE(KEYCODE_G)   PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H (BS)") PORT_CODE(KEYCODE_H)    PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I (HT)") PORT_CODE(KEYCODE_I)    PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J (LF)") PORT_CODE(KEYCODE_J)    PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K (VT)") PORT_CODE(KEYCODE_K)    PORT_CHAR('K')

	PORT_START("LINE6")    /* row 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L (FF)") PORT_CODE(KEYCODE_L)    PORT_CHAR('L')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M (DEL)") PORT_CODE(KEYCODE_M)   PORT_CHAR('M')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N (SO)") PORT_CODE(KEYCODE_N)    PORT_CHAR('N')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O (SI)") PORT_CODE(KEYCODE_O)    PORT_CHAR('O')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P (DLE)") PORT_CODE(KEYCODE_P)   PORT_CHAR('P')

	PORT_START("LINE7")    /* row 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q (DC1)") PORT_CODE(KEYCODE_Q)   PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R (DC2)") PORT_CODE(KEYCODE_R)   PORT_CHAR('R')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S (DC3)") PORT_CODE(KEYCODE_S)   PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T (DC4)") PORT_CODE(KEYCODE_T)   PORT_CHAR('T')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U (NAK)") PORT_CODE(KEYCODE_U)   PORT_CHAR('U')

	PORT_START("LINE8")    /* row 8 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V <-D") PORT_CODE(KEYCODE_V)     PORT_CHAR('V')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W (ETB)") PORT_CODE(KEYCODE_W)   PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X (CAN)") PORT_CODE(KEYCODE_X)   PORT_CHAR('X')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y (EM)") PORT_CODE(KEYCODE_Y)    PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z ->D") PORT_CODE(KEYCODE_Z)     PORT_CHAR('Z')

	/* analog joysticks (video board only) */

	PORT_START("BUTTONS")   /* joystick 1 & 2 buttons */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)

	PORT_START("JOY1_X")    /* joystick 1, X axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1)

	PORT_START("JOY1_Y")    /* joystick 1, Y axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1) PORT_REVERSE

	PORT_START("JOY2_X")    /* joystick 2, X axis */
	PORT_BIT( 0x3ff, 0x180,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x180 ) PORT_PLAYER(2)

	PORT_START("JOY2_Y")    /* joystick 2, Y axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

//    YEAR  NAME     PARENT  COMPAT  MACHINE      INPUT      CLASS           INIT        COMPANY              FULLNAME                                                                FLAGS
COMP( 1978, 990189,  0,      0,      tm990_189,   tm990_189, tm990189_state, empty_init, "Texas Instruments", "TM 990/189 University Board microcomputer",                            0 )
COMP( 1980, 990189v, 990189, 0,      tm990_189_v, tm990_189, tm990189_state, empty_init, "Texas Instruments", "TM 990/189 University Board microcomputer with Video Board Interface", 0 )
