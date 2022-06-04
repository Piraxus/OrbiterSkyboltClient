/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "ObjectUtil.h"

using namespace skybolt;

std::string getName(OBJHANDLE object)
{
	char name[256];
	int length = 256;
	oapiGetObjectName(object, name, length);
	return std::string(name);
}

sim::Vector3 orbiterToSkyboltVector3GlobalAxes(const VECTOR3& v)
{
	return skybolt::sim::Vector3(v.x, v.z, v.y);
}

sim::Vector3 orbiterToSkyboltVector3BodyAxes(const VECTOR3& v)
{
	return skybolt::sim::Vector3(v.z, v.x, -v.y);
}

osg::Vec3f orbiterToSkyboltVector3BodyAxes(const float* v)
{
	return osg::Vec3f(v[2], v[0], -v[1]);
}

osg::Vec3f skyboltToOrbiterVector3BodyAxes(const osg::Vec3f& v)
{
	return osg::Vec3f(v.y(), -v.z(), v.x());
}
