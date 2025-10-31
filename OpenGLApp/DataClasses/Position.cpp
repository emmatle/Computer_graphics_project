#include "Position.h"

Position::Position() {
		this->x = 0;
		this->y = 0;
	}

	Position::Position(float x, float y) {
		this->x = x;
		this->y = y;
	}

	Position::Position(glm::vec3 Position){
		this->x = Position.x;
		this->y = Position.y;
	}

	void Position::setPosition(float newX, float newY) {
		this->x = newX;
		this->y = newY;
	}

	void Position::setX0(float newX) {
		this->x = newX;
	}

	float Position::getX0() const {
		return x;
	}

	void Position::setY0(float newY) {
		this->y = newY;
	}

	float Position::getY0() const {
		return y;
	}

	glm::vec3 Position::posToVec3() const
	{
		return {x, y, 0.0f};
	}

	glm::vec2 Position::posToVec2() const
	{
		return {x,y};
	}

	Position& Position::operator=(const Position &op) {
		this->x = op.x;
		this->y = op.y;
		return *this;
	}

	Position& Position::operator=(const glm::vec3 op)
	{
		this->x = op.x;
		this->y = op.y;
		return *this;
	}

	void Position::printPosition() const
	{
		cout << "X : " << this->x << " - Y :" << this->y << endl;
	}

	bool Position::operator==(const Position &op) const
	{
		if (x == op.x &&
			y == op.y)
			return true;
		return false;
	}
