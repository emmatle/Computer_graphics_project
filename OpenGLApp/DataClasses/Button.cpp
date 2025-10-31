#include "Button.h"

Button::Button()
{
	this->x = 0.0f;
	this->y = 0.0f;
	this->selected = false;
	this->height = 1.0f;
	this->width = 1.0f;
	this->clicked = false;
}

Button::Button(const float x, const float y, const float wight, const float height)
{
	this->x = x;
	this->y = y;
	this->height = height;
	this->width = wight;
	this->selected = false;
	this->clicked = false;
}



Button& Button::operator=(const Button &op)
{
	this->x = op.x;
	this->y = op.y;
	this->height = op.height;
	this->width = op.width;
	this->selected = op.selected;
	return *this;
}
