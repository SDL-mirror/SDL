#ifndef VIDEO_MODE_HPP
#define VIDEO_MODE_HPP

#include <cstddef>

class Video_Mode
{
public:
	Video_Mode();
	Video_Mode(unsigned int The_Width, unsigned int The_Height, unsigned int The_Bits_Per_Pixel);

	static Video_Mode Get_Desktop_Mode();

	static std::size_t Get_Mode_Count();
	static Video_Mode Get_Mode(std::size_t Index);

	bool Is_Valid() const;

	bool operator==(const Video_Mode &Mode) const;
	bool operator!=(const Video_Mode &Mode) const;

	unsigned int Width;
	unsigned int Height;
	unsigned int Bits_Per_Pixel;

private:
	static void Initialize_Modes();
};

#endif
