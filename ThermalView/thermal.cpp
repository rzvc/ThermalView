/* The MIT License (MIT)
 *
 * Copyright (c) 2015 Razvan C. Cojocariu (code@dumb.ro)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "thermal.h"
#include <string>

#define USB_TIMEOUT 1000


using namespace std;


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// Seek Thermal interface
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SeekThermal::SeekThermal()
{
	m_handle = 0;
	m_ep_claimed = false;
	m_thread_running = false;
	m_thread_should_stop = false;
}

SeekThermal::~SeekThermal()
{
	close();
}



//////////////////////////////////////////////////////////////////////////
/// Class to throw - we only use it locally
//////////////////////////////////////////////////////////////////////////
struct usb_failure
{
};


//////////////////////////////////////////////////////////////////////////////
/// CTRL_OUT - Execute a control out
//////////////////////////////////////////////////////////////////////////////
#define CTRL_OUT(req, ...) \
{\
	uint8_t data[] = {__VA_ARGS__};\
\
	if (libusb_control_transfer(m_handle, 0x41 | LIBUSB_ENDPOINT_OUT, req, 0, 0, reinterpret_cast<unsigned char *>(data), sizeof(data), USB_TIMEOUT) != sizeof(data))\
		throw usb_failure();\
}


//////////////////////////////////////////////////////////////////////////////
/// ctrl_in - Execute a control in
//////////////////////////////////////////////////////////////////////////////
std::vector<uint8_t> SeekThermal::ctrlIn(uint8_t req, uint16_t nr_bytes)
{
	std::vector<uint8_t> data;
	data.resize(nr_bytes);

	if (libusb_control_transfer(m_handle, 0xC1 | LIBUSB_ENDPOINT_IN, req, 0, 0, reinterpret_cast<unsigned char *>(&data[0]), nr_bytes, USB_TIMEOUT) != nr_bytes)
		throw usb_failure();

	return data;
}


//////////////////////////////////////////////////////////////////////////////
/// connect - Connect to the USB device
//////////////////////////////////////////////////////////////////////////////
bool SeekThermal::connect()
{
	onConnecting();
	
	unique_lock<recursive_mutex> lock(m_mx);
	
	// If it's already connected, try disconnect
	if (m_handle)
		close();

	// Let's find the device and connect to it
	libusb_device ** list;

	size_t len = libusb_get_device_list(0, &list);

	if (len > 0)
	{
		libusb_device_handle * handle;

		for (size_t i = 0; i < len; ++i)
		{
			libusb_device_descriptor descr;

			if (libusb_get_device_descriptor(list[i], &descr) == 0)		// 0 ok, LIBUSB_ERROR - not ok
			{
				if (libusb_open(list[i], &handle) == 0)
				{
					if (descr.idVendor == 0x289D && descr.idProduct == 0x0010)
					{
						m_handle = handle;

						break;
					}
					
					libusb_close(handle);
				}
			}
		}

		libusb_free_device_list(list, 1);
	}


	if (m_handle)
	{
		if (initialize())
		{
			onConnected();
			return true;
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////////
/// initialize - Initialize the USB device
//////////////////////////////////////////////////////////////////////////////
bool SeekThermal::initialize()
{
	if (m_handle)
	{
		try
		{
			// Claim endpoint
			if (!libusb_claim_interface(m_handle, 0) == 0)
				throw usb_failure();

			m_ep_claimed = true;


			// Now let's go through the initialization routine
			CTRL_OUT(0x56, 1);
			CTRL_OUT(0x56, 0, 0);

			vector<uint8_t> data1 = ctrlIn(0x4e, 4);
			vector<uint8_t> data2 = ctrlIn(0x36, 12);

			CTRL_OUT(0x56, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00);
			vector<uint8_t> data3 = ctrlIn(0x58, 0x0c);

			CTRL_OUT(0x56, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00);
			vector<uint8_t> data4 = ctrlIn(0x58, 0x02);

			CTRL_OUT(0x56, 0x01, 0x00, 0x01, 0x06, 0x00, 0x00);
			vector<uint8_t> data5 = ctrlIn(0x58, 0x02);

			CTRL_OUT(0x56, 0x20, 0x00, 0x30, 0x00, 0x00, 0x00);
			vector<uint8_t> data6 = ctrlIn(0x58, 0x40);

			CTRL_OUT(0x56, 0x20, 0x00, 0x50, 0x00, 0x00, 0x00);
			vector<uint8_t> data7 = ctrlIn(0x58, 0x40);

			CTRL_OUT(0x56, 0x0C, 0x00, 0x70, 0x00, 0x00, 0x00);
			vector<uint8_t> data8 = ctrlIn(0x58, 0x18);

			CTRL_OUT(0x3e, 0x08, 0x00);
			vector<uint8_t> data9 = ctrlIn(0x3d, 0x02);

			CTRL_OUT(0x3c, 0x01, 0x00);
			vector<uint8_t> data10 = ctrlIn(0x3d, 2);

			return true;
		}
		catch (usb_failure &)
		{
			close();
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////////
/// close - Close the USB device
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::close()
{
	unique_lock<recursive_mutex> lock(m_mx);
	
	stopThread();

	if (m_handle)
	{
		try
		{
			CTRL_OUT(0x3c, 0, 0);
			CTRL_OUT(0x3c, 0, 0);
			CTRL_OUT(0x3c, 0, 0);
		}
		catch (usb_failure &)
		{
		}

		if (m_ep_claimed)
		{
			libusb_release_interface(m_handle, 0);
			m_ep_claimed = false;
		}

		libusb_close(m_handle);

		m_handle = 0;
	}
	
	onDisconnected();
}

//////////////////////////////////////////////////////////////////////////////
/// is_open - Indicates if the USB device is open
//////////////////////////////////////////////////////////////////////////////
bool SeekThermal::isOpen()
{
	lock_guard<recursive_mutex> lck(m_mx);
	
	return m_handle != 0;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// get_frame - Fetch a frame from the camera
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
std::vector<uint16_t> SeekThermal::getFrame()
{
	unique_lock<recursive_mutex> lock(m_mx);
	
	std::vector<uint8_t> data;
	std::vector<uint16_t> frame;

	data.resize(0x7ec0 * 2); // 64896

	try
	{
		CTRL_OUT(0x53, 0xc0, 0x7e, 0, 0);

		// Do the bulk in transfer
		int total = 0;
		int transferred;

		while (true)
		{
			int res = libusb_bulk_transfer(m_handle, 0x81, &data[0] + total, data.size() - total, &transferred, USB_TIMEOUT);

			if (res == 0)
			{
				total += transferred;

				if (total >= static_cast<int>(data.size()))
					break;
			}
			else
				throw usb_failure();
		}

		if (total != static_cast<int>(data.size()))
			throw usb_failure();


		// Let's interpret the data
		frame.resize(206 * 156);

		for (size_t y = 0; y < 156; ++y)
		{
			for (size_t x = 0; x < 206; ++x)
			{
				uint16_t frame_pixel = y * 206 + x;
				uint16_t data_pixel = y * 208 + x;
				uint16_t val = (data[data_pixel * 2 + 1] << 8) | data[data_pixel * 2];

				frame[frame_pixel] = val;
			}
		}
	}
	catch (usb_failure &)
	{
		close();
	}

	return frame;
}


//////////////////////////////////////////////////////////////////////////////
/// get_stream - Starts the thread and issues a continous stream of frames
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::getStream()
{
	startThread(true);
}


//////////////////////////////////////////////////////////////////////////////
/// get_one - Starts the thread and stops it after the first frame
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::getOne()
{
	startThread(false);
}


//////////////////////////////////////////////////////////////////////////////
/// start_thread - Actual thread starter
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::startThread(bool stream)
{
	// You can't call start_thread from the thread itself
	assert(!m_thread_running || boost::this_thread::get_id() != m_thread.get_id());
	
	stopThread();
	
	m_thread_running = true;
	m_thread_should_stop = false;
	m_thread_single = !stream;
	m_thread = boost::thread(&SeekThermal::workerThread, this);
}


//////////////////////////////////////////////////////////////////////////////
/// stop_thread - Stops 
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::stopThread()
{
	// Just stop the thread
	if (m_thread_running)
	{
		// When stop_thread() was called from within the thread
		if (boost::this_thread::get_id() == m_thread.get_id())
		{
			m_thread_should_stop = true;
		}
		else
		{
			m_thread_running = false;
			m_thread.join();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
/// stop_streaming - Stops the thread
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::stopStreaming()
{
	stopThread();
}


//////////////////////////////////////////////////////////////////////////////
/// is_streaming - Indicates if the thread is running and is in streaming mode
//////////////////////////////////////////////////////////////////////////////
bool SeekThermal::isStreaming()
{
	return m_thread_running && !m_thread_single;
}


//////////////////////////////////////////////////////////////////////////////
/// worker_thread
//////////////////////////////////////////////////////////////////////////////
void SeekThermal::workerThread()
{
	if (!m_thread_single)
		onStreamingStart();
	
	while (m_thread_running && !m_thread_should_stop)
	{
		onNewFrame(getFrame());

		if (m_thread_single)
			break;
	}
	
	if (!m_thread_single)
		onStreamingStop();

	m_thread_running = false;
}
