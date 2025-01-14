#include "ParserSingleInputStream.h"

#include <sstream>

ParserSingleInputStream::ParserSingleInputStream(std::istream& stream, std::string fileName)
    : m_stream(stream),
      m_file_name(std::make_shared<std::string>(std::move(fileName))),
      m_line_number(1)
{
}

ParserLine ParserSingleInputStream::NextLine()
{
    std::ostringstream str;
    auto hasLength = false;

    auto c = m_stream.get();
    while (c != EOF)
    {
        switch (c)
        {
        case '\r':
            c = m_stream.get();
            if (c == '\n')
                return ParserLine(m_file_name, m_line_number++, str.str());
            str << '\r';
            hasLength = true;
            continue;

        case '\n':
            return ParserLine(m_file_name, m_line_number++, str.str());

        default:
            str << static_cast<char>(c);
            hasLength = true;
            break;
        }

        c = m_stream.get();
    }

    if (hasLength)
        return ParserLine(m_file_name, m_line_number, str.str());

    return ParserLine();
}

bool ParserSingleInputStream::IncludeFile(const std::string& filename)
{
    return false;
}

void ParserSingleInputStream::PopCurrentFile()
{
}

bool ParserSingleInputStream::IsOpen() const
{
    return !m_stream.eof();
}

bool ParserSingleInputStream::Eof() const
{
    return m_stream.eof();
}
