#include "Stream_win32.h"

#include <Windows.h>
#include <sstream>

struct Stream::SData
{
    SData()
        : m_SerialFd(INVALID_HANDLE_VALUE)
    {
    }

    HANDLE m_SerialFd;
};

Stream::Stream(const char* devName)
    : m_IsAvailable(false),
    m_IsConnected(false),
    m_baudRate(115200),
    m_devName(devName),
    m_data(std::make_unique<SData>())
{
    open_comm();
    m_recvdByte = '\0';
}

Stream::Stream()
    : m_IsAvailable(false),
    m_IsConnected(false),
	m_baudRate(115200),
    m_data(std::make_unique<SData>())
{
    m_recvdByte = '\0';
}

Stream::~Stream()
{
    close_comm();
}

bool Stream::available()
{
    DWORD iRxLen = 0;
    m_IsAvailable = ReadFile(m_data->m_SerialFd, (void*)&m_recvdByte, sizeof(m_recvdByte), &iRxLen, NULL);
    return m_IsAvailable && (iRxLen > 0);
}

size_t Stream::write(uint8_t byte)
{
    m_IsAvailable = false;
    DWORD iTxLen = 0;
    WriteFile(m_data->m_SerialFd, (void*)&byte, sizeof(byte), &iTxLen, NULL);
    return iTxLen;
}

size_t Stream::write(uint8_t* byte, size_t num_of_bytes)
{
    m_IsAvailable = false;
    DWORD iTxLen = 0;
    WriteFile(m_data->m_SerialFd, byte, num_of_bytes, &iTxLen, NULL);
    return iTxLen;
}

void Stream::begin(uint16_t baud_rate)
{
    m_baudRate = baud_rate;
    close_comm();
    open_comm();
}

void Stream::close_comm()
{
    CloseHandle(m_data->m_SerialFd);
    m_IsAvailable = false;
    m_IsConnected = false;
    m_data->m_SerialFd = INVALID_HANDLE_VALUE;
}

std::string GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

bool Stream::open_comm()
{
	const bool flowctrl = false; // TODO: Which one?

	std::stringstream mode;
	mode << "baud=" << m_baudRate;
	mode << " data=" << 8; // TODO: How many data-bits? 5, 6, 7 or 8? Linux default: 8
	mode << " parity=" << 'n'; // TODO: Parity? n, e, o? Linux default: n
	mode << " stop=" << 1; // TODO: Stop bits? 1 or 2? Linux default: 1
	if (!flowctrl)
		mode << " xon=off to=off odsr=off dtr=on rts=on";
	else
		mode << " xon=off to=off odsr=off dtr=on rts=off";

	m_data->m_SerialFd = CreateFileA(m_devName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0, // no share
		NULL, // no security
		OPEN_EXISTING,
		0, // no threads
		NULL); // no templates

	if (m_data->m_SerialFd == INVALID_HANDLE_VALUE)
	{
		printf("1\n");
		return false;
	}

	DCB port_settings;
	memset(&port_settings, 0, sizeof(port_settings));  // clear the new struct
	port_settings.DCBlength = sizeof(port_settings);

	const auto mode_str = mode.str();
	printf("%s\n", mode_str.c_str());
	if (!BuildCommDCBA(mode_str.c_str(), &port_settings))
	{
		printf("2\n");
		auto err = GetLastErrorAsString();
		printf("%s\n", err.c_str());
		CloseHandle(m_data->m_SerialFd);
		return false;
	}

	if (flowctrl)
	{
		port_settings.fOutxCtsFlow = TRUE;
		port_settings.fRtsControl = RTS_CONTROL_HANDSHAKE;
	}

	if (!SetCommState(m_data->m_SerialFd, &port_settings))
	{
		printf("3\n");
		CloseHandle(m_data->m_SerialFd);
		return false;
	}

	COMMTIMEOUTS Cptimeouts{};

	Cptimeouts.ReadIntervalTimeout = MAXDWORD;
	Cptimeouts.ReadTotalTimeoutMultiplier = 0;
	Cptimeouts.ReadTotalTimeoutConstant = 0;
	Cptimeouts.WriteTotalTimeoutMultiplier = 0;
	Cptimeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(m_data->m_SerialFd, &Cptimeouts))
	{
		printf("4\n");
		CloseHandle(m_data->m_SerialFd);
		return false;
	}

	m_IsConnected = true;
	return m_IsConnected;
}
