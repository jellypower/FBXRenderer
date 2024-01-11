#pragma once
#include "SSHierarchyObject.h"
#include "SSEngineDefault/SSEngineDefault.h"


class SSPlaceableObject : public SSHierarchyObject
{
protected:
	Transform _transform;

public:
	virtual ~SSPlaceableObject() override = 0 { };
};

