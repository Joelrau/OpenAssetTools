#pragma once

#include "Utils/ClassUtils.h"
#include <string>
#include <vector>

class NamespaceBuilder
{
    std::vector<std::string> m_elements;

public:
    static std::string Combine(const std::string& _namespace, const std::string& name);

    _NODISCARD bool IsEmpty() const;
    void Push(std::string element);
    void Pop();
    std::string ToString();
};
