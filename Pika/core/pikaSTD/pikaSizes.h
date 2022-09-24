#pragma once
//////////////////////////////////////////
//pikaSizes.h
//Luta Vlad(c) 2022
//https://github.com/meemknight/PikaEngine
//////////////////////////////////////////

namespace pika
{
	inline size_t KB(size_t x) { return x * 1024ull; }
	inline size_t MB(size_t x) { return KB(x) * 1024ull; }
	inline size_t GB(size_t x) { return MB(x) * 1024ull; }
};
