#include "../include/Entity.h"
#include <iostream>

namespace library
{

    void Entity::display() const
    {
        std::cout << "ID: " << id_ << ", Name: " << name_ << std::endl;
    }

}