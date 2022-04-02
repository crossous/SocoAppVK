#pragma once

template <typename T>
class Singleton
{
public:
	T* GetInstance()
	{
		T* Instance = new T();
		return Instance;
	}
protected:
	Singleton() = default;
};