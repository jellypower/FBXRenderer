#include "SSHierarchyObject.h"

SSHierarchyObject::SSHierarchyObject(uint32 childCapacity)
	: _parent(nullptr), _childs(childCapacity)
{
}

SSHierarchyObject::SSHierarchyObject()
	: _parent(nullptr), _childs(0)
{
}
