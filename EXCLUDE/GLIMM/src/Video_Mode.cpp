#include "Video_Mode.hpp"
#include <vector>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace
{

	typedef std::vector<Video_Mode> Video_Mode_List;
	Video_Mode_List Supported_Modes;

	struct Compare_Modes
	{
		bool operator()(const Video_Mode &Mode_1, const Video_Mode &Mode_2) const
		{
			if (Mode_1.Bits_Per_Pixel > Mode_2.Bits_Per_Pixel)
				return true;
			else if (Mode_1.Bits_Per_Pixel < Mode_2.Bits_Per_Pixel)
				return false;
			else if (Mode_1.Width > Mode_2.Width)
				return true;
			else if (Mode_1.Width < Mode_2.Width)
				return false;
			else
				return Mode_1.Height > Mode_2.Height;
		}
	};

}

Video_Mode::Video_Mode() : Width(0),
						   Height(0),
						   Bits_Per_Pixel(0)
{

}

Video_Mode::Video_Mode(unsigned int The_Width, unsigned int The_Height, unsigned int The_Bits_Per_Pixel)
	: Width(The_Width),
	  Height(The_Height),
	  Bits_Per_Pixel(The_Bits_Per_Pixel)
{

}

Video_Mode Video_Mode::Get_Desktop_Mode()
{
	DEVMODE Device_Mode = {0};
	Device_Mode.dmSize = sizeof(Device_Mode);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &Device_Mode);
	return Video_Mode(Device_Mode.dmPelsWidth, Device_Mode.dmPelsHeight, Device_Mode.dmBitsPerPel);
}

std::size_t Video_Mode::Get_Mode_Count()
{
	Initialize_Modes();
	return Supported_Modes.size();
}

Video_Mode Video_Mode::Get_Mode(std::size_t Index)
{
	Initialize_Modes();
	return Supported_Modes[Index];
}

bool Video_Mode::Is_Valid() const
{
	Initialize_Modes();
	return Supported_Modes.end() != std::find(Supported_Modes.begin(), Supported_Modes.end(), *this);
}

bool Video_Mode::operator==(const Video_Mode &Mode) const
{
	return (Width == Mode.Width
		&&  Height == Mode.Height
		&&  Bits_Per_Pixel == Mode.Bits_Per_Pixel);
}

bool Video_Mode::operator!=(const Video_Mode &Mode) const
{
	return !(*this == Mode);
}

void Video_Mode::Initialize_Modes()
{
	static bool Initialized = false;
	if (!Initialized)
	{
		DEVMODE Device_Mode = {0};
		Device_Mode.dmSize = sizeof(Device_Mode);
		for (std::size_t i = 0; 0 != EnumDisplaySettings(NULL, i, &Device_Mode); ++i)
		{
			Video_Mode Mode(Device_Mode.dmPelsWidth, Device_Mode.dmPelsHeight, Device_Mode.dmBitsPerPel);
			if (Supported_Modes.end() == std::find(Supported_Modes.begin(), Supported_Modes.end(), Mode))
				Supported_Modes.push_back(Mode);
		}
		std::sort(Supported_Modes.begin(), Supported_Modes.end(), Compare_Modes());
	}
}
