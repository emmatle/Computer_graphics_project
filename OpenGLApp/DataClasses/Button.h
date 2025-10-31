#pragma once
#include "Position.h"

using namespace std;

class Button
{
public:
	Button();
	Button(float x, float y, float wight, float height);


	Button& operator = (const Button &op);

	float x;
	float y;
	float height;
	float width;
	bool selected;
	bool clicked;
};

