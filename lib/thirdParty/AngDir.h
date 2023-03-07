/*
こいつの存在意義
本来MachineManagerに搭載していたものですが、使うわけがない場所でもインクルードされるのを防ごうという意図で作られました。
*/

#ifndef __ANG_DIR__H__
#define __ANG_DIR__H__

enum class Direction : signed char
{

	West,
	North,
	East,
	South,
};

enum class Angle : signed char
{
	Left,
	Forward,
	Right,
	Backward,
};

#endif