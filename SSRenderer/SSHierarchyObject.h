#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"

#include "SSNonCopyable.h"

class SSHierarchyObject
{
protected:
	SSHierarchyObject* _parent;
	SS::PooledList<SSHierarchyObject*> _childs;


protected:
	SSHierarchyObject(uint32 childCapacity);
	SSHierarchyObject();
	virtual ~SSHierarchyObject() = 0 { }
};

