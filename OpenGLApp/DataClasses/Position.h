#pragma once
#include <glm/glm.hpp>
#include <iostream>
using namespace std;

class Position
{
public:
	Position();
	Position(float x, float y);
	explicit Position(glm::vec3 Position);

	void setPosition(float newX, float newY);

	void setX0(float newX);
	float getX0() const;

	void setY0(float newY);
	float getY0() const;

	glm::vec3 posToVec3() const;
	glm::vec2 posToVec2() const;

	Position& operator=(const Position &op);
	Position& operator=(glm::vec3 op);
	
	void printPosition() const;

	bool operator==(const Position &op) const;

protected:
	float x;
	float y;
};
