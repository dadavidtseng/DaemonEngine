#include "Rgba8.hpp"

//-----------------------------------------------------------------------------------------------
Rgba8::Rgba8()
	: r(255.f), g(255.f), b(255.f), a(255.f)
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
