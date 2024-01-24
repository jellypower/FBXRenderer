#pragma once

class SSNoncopyable
{
public:
	SSNoncopyable() = default;

private:
	SSNoncopyable(const SSNoncopyable& rhs) = delete;
	SSNoncopyable& operator=(const SSNoncopyable& rhs) = delete;
};
