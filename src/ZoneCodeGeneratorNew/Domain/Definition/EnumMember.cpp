#include "EnumMember.h"

EnumMember::EnumMember()
    : m_value(0)
{
}

EnumMember::EnumMember(std::string name, const long long value)
    : m_name(std::move(name)),
      m_value(value)
{
}
