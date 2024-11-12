#include "Rgba8.hpp"

//-----------------------------------------------------------------------------------------------
Rgba8::Rgba8()
	: r(255), g(255), b(255), a(255)
{
}

//-----------------------------------------------------------------------------------------------
Rgba8::Rgba8
(
	const unsigned char red,
	const unsigned char green,
	const unsigned char blue,
	const unsigned char alpha
)
	: r(red), g(green), b(blue), a(alpha)
{
}
