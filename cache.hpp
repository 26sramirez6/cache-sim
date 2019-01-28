#include <memory>
#include <vector>
#include <iostream>
#include <bitset>

//#define NDEBUG
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

uint32_t constexpr ADDRLEN = 32;

static inline uint32_t
GetBitLength(uint32_t val) {
	uint32_t ret = 0;
	while (val &= ~(1<<ret++)) {}
	return ret;
}


class Address {
private:
	uint32_t address_;
	static uint32_t wordFieldSize_;
	static uint32_t tagFieldSize_;
	static uint32_t indexFieldSize_;
	static uint32_t tagMask_;
	static uint32_t indexMask_;
	static uint32_t wordMask_;
public:

#ifndef NDEBUG
	Address(uint32_t address) : address_(address) { this->Assert(); }
#else
	Address(uint32_t address) : address_(address) {}
#endif

	inline uint32_t GetTag() const {
		return (this->address_&Address::tagMask_)>>
				(Address::wordFieldSize_ + Address::indexFieldSize_);
	}

	inline uint32_t GetIndex() const {
		return (this->address_&Address::indexMask_)>>
				(Address::wordFieldSize_);
	}

	inline uint32_t GetOffset() const {
		return this->address_&Address::wordMask_;
	}

	static void StaticInit(uint32_t cacheSize, uint32_t blockSize) {
		Address::indexFieldSize_ = GetBitLength(cacheSize / blockSize) - 1;
		Address::wordFieldSize_ = GetBitLength(blockSize / sizeof(double)) - 1;
		assert(ADDRLEN > Address::indexFieldSize_ + Address::wordFieldSize_);
		Address::tagFieldSize_ = ADDRLEN - indexFieldSize_ - wordFieldSize_;
		Address::wordMask_ = (1 << Address::wordFieldSize_) - 1;
		Address::indexMask_ = ((1 << (Address::indexFieldSize_ + Address::wordFieldSize_)) - 1)&(~Address::wordMask_);
		Address::tagMask_ = ~((1 << (ADDRLEN - tagFieldSize_)) - 1);
	}

	static void Assert() {
		assert(Address::indexFieldSize_!=0);
		assert(Address::wordFieldSize_!=0);
		assert(Address::tagFieldSize_!=0);
		assert(Address::tagMask_!=0);
		assert(Address::indexMask_!=0);
		assert(Address::wordMask_!=0);
	}
};
uint32_t Address::wordFieldSize_ = 0;
uint32_t Address::tagFieldSize_ = 0;
uint32_t Address::indexFieldSize_ = 0;
uint32_t Address::wordMask_ = 0;
uint32_t Address::tagMask_ = 0;
uint32_t Address::indexMask_ = 0;


class DataBlock {
private:
	static uint32_t size_;
	static uint32_t numWords_;
	std::vector<double> data_;
public:

#ifndef NDEBUG
	DataBlock() : data_(size_) { assert(this->size_!=0); }
#else
	DataBlock() : data_(size_) {}
#endif

	inline double GetWord(uint32_t offset) { return this->data_[offset]; }

	static void StaticInit(uint32_t blockSize) {
		DataBlock::size_ = blockSize;
		DataBlock::numWords_ = blockSize / sizeof(double);
	}

	static uint32_t GetSize() {
		assert(DataBlock::size_!=0);
		return DataBlock::size_;
	}
};
uint32_t DataBlock::size_ = 0;
uint32_t DataBlock::numWords_ = 0;

class RAM {
private:
	uint32_t size_;
	std::vector<DataBlock> blocks_;
public:
	RAM(uint32_t size) : size_(size), blocks_(size/DataBlock::GetSize()) {}

	DataBlock GetBlockCopy(Address& address) {
		return blocks_[address.GetIndex()];
	}

	void SetBlock(Address& address, std::unique_ptr<DataBlock> value) {

	}
};

class Cache {
private:
	uint32_t numSets_;
	uint32_t numBlocks_;
	uint32_t nWay_;
	uint32_t cacheSize_;
	uint32_t blockSize_;
	RAM & ram_;
	std::vector<std::vector<DataBlock> > blocks_;
	std::vector<std::vector<bool> > valid_;
	std::vector<std::vector<uint32_t> > tags_;

	void RandomWrite(Address& address, DataBlock& block, std::vector<DataBlock>& set) {
		set[rand()%this->nWay_] = block;
	}

public:
	Cache(uint32_t nWay, uint32_t blockSize, uint32_t cacheSize, RAM& ram) :
		nWay_(nWay), cacheSize_(cacheSize), blockSize_(blockSize), ram_(ram) {
		srand (time(NULL));
		this->numBlocks_ = cacheSize / blockSize;
		this->numSets_ = this->numBlocks_ / nWay;
		this->blocks_.resize(this->numSets_, std::vector<DataBlock>(this->nWay_));
		this->valid_.resize(this->numSets_, std::vector<bool>(this->nWay_, false));
		this->tags_.resize(this->numSets_, std::vector<uint32_t>(this->nWay_));
	}

	double GetDouble(Address& address) {
		unsigned index = address.GetIndex()%this->numSets_;
		std::vector<DataBlock>& set = this->blocks_[index];
		std::vector<bool>& valid = this->valid_[index];
		std::vector<uint32_t>& tag = this->tags_[index];

		for (uint32_t i=0; i<this->nWay_; ++i) {
			if (valid[i] && tag[i]==address.GetTag()) { // cache hit
				return set[i].GetWord(address.GetOffset());
			}
		}
		// cache miss: fetch from RAM.
		// Note: I'm returning a new DataBlock object,
		// not a pointer to the one in RAM.
		// this is to simulate the write from RAM to cache that occurs
		// on a cache miss
		DataBlock block = ram_.GetBlockCopy(address);
		this->RandomWrite(address, block, set);
		return 1.;
	}

	double SetDouble(Address& address, double val) {
		return 1.;
	}

	DataBlock * GetBlock(Address& address) {
		return nullptr;
	}

	void SetBlock(Address& address, std::unique_ptr<DataBlock> block) {

	}
};

class CPU {
private:
	std::unique_ptr<Cache> cache_;
	std::unique_ptr<RAM> ram_;

public:
	CPU(uint32_t nWay, uint32_t cacheSize, uint32_t blockSize, uint32_t ramSize) {
		DataBlock::StaticInit(blockSize);
		Address::StaticInit(cacheSize, blockSize);
		this->ram_ = std::unique_ptr<RAM>{ new RAM(ramSize) };
		this->cache_ = std::unique_ptr<Cache>{ new Cache(nWay, blockSize, cacheSize, *this->ram_) };
	}

	double LoadDouble(Address& address) const {
		return 1.;
	}

	void StoreDouble(Address& address, double value) {

	}

	double AddDouble(double val1, double val2) const {
		return 1.;
	}

	double MultDouble(double val1, double val2) const {
		return 1.;
	}
};
