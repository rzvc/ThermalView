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

#include <libusb.h>
#include <cstdint>
#include <vector>
#include <boost/thread.hpp>
#include <mutex>
#include <boost/signals2.hpp>
#include <boost/thread/synchronized_value.hpp>

class SeekThermal
{
private:
	std::recursive_mutex	m_mx;	// Protects the USB stuff

	libusb_device_handle * m_handle;
	bool m_ep_claimed;

	// Thread stuff
	boost::thread m_thread;
	boost::synchronized_value<bool> m_thread_running;
	boost::synchronized_value<bool> m_thread_single;		// Single frame
	boost::synchronized_value<bool> m_thread_should_stop;	// Indicates that close() was called from within the thread
	
public:
    SeekThermal();
	~SeekThermal();

	bool connect();
	void close();

	bool isOpen();
	bool isStreaming();

	std::vector<uint16_t> getFrame();

	void getStream();
	void getOne();
	
	void stopStreaming();


	// Events
	boost::signals2::signal<void()> onConnecting;
	boost::signals2::signal<void()> onConnected;
	boost::signals2::signal<void()> onDisconnected;
	boost::signals2::signal<void()> onStreamingStart;
	boost::signals2::signal<void()> onStreamingStop;
	boost::signals2::signal<void(const std::vector<uint16_t> & data)> onNewFrame;

private:
    bool initialize();
	
	std::vector<uint8_t> ctrlIn(uint8_t req, uint16_t nr_bytes);

	void startThread(bool stream);
	void stopThread();
	void workerThread();
};
