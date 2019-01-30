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

#define CACHE_DEBUG
uint32_t constexpr ADDRLEN = 32;

static inline uint32_t
GetBitLength(uint32_t val) {
	uint32_t ret = 0;
	while (val &= ~(1<<ret++)) {}
	return ret;
}

struct CacheConfig {
	uint32_t nWay;
	uint32_t cacheSize;
	uint32_t blockSize;
	uint32_t matDims;
	uint32_t blockFactor;
	uint32_t cacheBlockCount;
	uint32_t numSets;
	uint32_t ramSize;
	uint32_t wordsPerBlock;
	bool logging;
	std::string policy;
	std::string algo;
	uint32_t wordSize;
	uint32_t ramBlockCount;

	CacheConfig(): nWay(2), cacheSize(65536),
		blockSize(64), matDims(480),
		blockFactor(32), logging(false),
		policy("LRU"), algo("mxm_block"),
		wordSize(sizeof(double)) {

		this->cacheBlockCount = this->cacheSize / this->blockSize;
		this->numSets = this->cacheSize / this->blockSize / this->nWay;
		this->wordsPerBlock = this->blockSize / this->wordSize;

		// initialized on ComputeRAMStats()
		this->ramSize = 0;
		this->ramBlockCount = 0;
	};

	void ComputeRAMStats() {
		// can be re-called as needed
		this->ramSize = 0;
		this->ramBlockCount = 0;
		if (this->policy=="mxm_block" || this->policy=="mxm") {
			this->ramSize += this->matDims*this->matDims*this->wordSize*3;
		} else {
			this->ramSize += this->matDims*this->wordSize*3;
		}
		this->ramSize += (this->ramSize%blockSize);
		this->ramBlockCount = this->ramSize / this->blockSize;
	}
};

class Address {
private:
	uint32_t address_;

	// field sizes in bits statically stored
	// once cache size is known
	static uint32_t byteFieldSize_;
	static uint32_t wordFieldSize_;
	static uint32_t tagFieldSize_;
	static uint32_t indexFieldSize_;
	static uint32_t setFieldSize_;
	static uint32_t cacheBlockFieldSize_;
	static uint32_t ramBlockFieldSize_;
	//static uint32_t
	// masks for each field statically
	// stored once cache size is known
	static uint32_t byteMask_;
	static uint32_t tagMask_;
	static uint32_t indexMask_;
	static uint32_t wordMask_;
	static uint32_t setMask_;
	static uint32_t cacheBlockMask_;
	static uint32_t ramBlockMask_;
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

	inline uint32_t GetSet() const {
		return (this->address_&Address::setMask_)>>
				(Address::wordFieldSize_);
	}

	inline uint32_t GetCacheBlock() const {
		return (this->address_&Address::cacheBlockMask_)>>
				(Address::wordFieldSize_ + Address::setFieldSize_);
	}

	inline uint32_t GetRamBlock() const {
		return this->address_&Address::ramBlockMask_;
	}

	inline uint32_t GetWord() const {
		return this->address_&Address::wordMask_;
	}

	static void StaticInit(CacheConfig& config) {
		indexFieldSize_ = GetBitLength(config.cacheBlockCount) - 1;
		wordFieldSize_ = GetBitLength(config.wordsPerBlock) - 1;
		setFieldSize_ = GetBitLength(config.numSets) - 1;
		byteFieldSize_ = GetBitLength(config.wordSize) - 1;
		ramBlockFieldSize_ = GetBitLength(config.ramBlockCount) - 1;
		cacheBlockFieldSize_ = indexFieldSize_ - setFieldSize_;
		tagFieldSize_ = ADDRLEN - indexFieldSize_ - wordFieldSize_;

		byteMask_ = (1 << byteFieldSize_) - 1;
		wordMask_ = ((1 << (wordFieldSize_ + byteFieldSize_)) - 1)&(~byteMask_);
		indexMask_ = ((1 << (indexFieldSize_ + wordFieldSize_ + byteFieldSize_)) - 1)&(~(wordMask_|byteMask_));
		tagMask_ = ~((1 << (ADDRLEN - tagFieldSize_)) - 1);
		setMask_ = ((1 << (setFieldSize_ + wordFieldSize_)) - 1)&(~wordMask_);
		cacheBlockMask_ = ~(setMask_|wordMask_|tagMask_);
		ramBlockMask_ = ((1 << (byteFieldSize_ + wordFieldSize_ + ramBlockFieldSize_)) - 1)&
				(~((1 << (byteFieldSize_ + wordFieldSize_)) - 1));


#ifdef CACHE_DEBUG
		std::cout << "byte mask:             " << std::bitset<ADDRLEN>(byteMask_) << std::endl;
		std::cout << "word mask:             " << std::bitset<ADDRLEN>(wordMask_) << std::endl;
		std::cout << "set mask:              " << std::bitset<ADDRLEN>(setMask_) << std::endl;
		std::cout << "cache block mask:      " << std::bitset<ADDRLEN>(cacheBlockMask_) << std::endl;
		std::cout << "cache full index mask: " << std::bitset<ADDRLEN>(indexMask_) << std::endl;
		std::cout << "cache tag mask:        " << std::bitset<ADDRLEN>(tagMask_) << std::endl;
		std::cout << "ram block mask:        " << std::bitset<ADDRLEN>(ramBlockMask_) << std::endl;
#endif

		assert(ADDRLEN > indexFieldSize_ + wordFieldSize_);
		assert(setFieldSize_ + cacheBlockFieldSize_==indexFieldSize_);
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
uint32_t Address::byteFieldSize_ = 0;
uint32_t Address::wordFieldSize_ = 0;
uint32_t Address::tagFieldSize_ = 0;
uint32_t Address::indexFieldSize_ = 0;
uint32_t Address::byteMask_ = 0;
uint32_t Address::wordMask_ = 0;
uint32_t Address::tagMask_ = 0;
uint32_t Address::indexMask_ = 0;
uint32_t Address::setMask_ = 0;
uint32_t Address::ramBlockMask_ = 0;
uint32_t Address::cacheBlockMask_ = 0;
uint32_t Address::setFieldSize_ = 0;
uint32_t Address::cacheBlockFieldSize_ = 0;
uint32_t Address::ramBlockFieldSize_ = 0;

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

	static void StaticInit(CacheConfig& config) {
		DataBlock::size_ = config.blockSize;
		DataBlock::numWords_ = config.wordsPerBlock;
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
	RAM(CacheConfig& config) : size_(config.ramSize), blocks_(config.ramBlockCount) {}

	DataBlock GetBlockCopy(Address& address) {
		return blocks_[address.GetIndex()];
	}

	void SetBlock(Address& address, std::unique_ptr<DataBlock> value) {

	}
};

class Cache {
private:
	uint32_t nWay_;
	uint32_t cacheSize_;
	uint32_t blockSize_;
	uint32_t numBlocks_;
	uint32_t numSets_;
	RAM & ram_;

	std::vector<std::vector<DataBlock> > blocks_;
	std::vector<std::vector<bool> > valid_;
	std::vector<std::vector<uint32_t> > tags_;

	void RandomWrite(DataBlock& block, std::vector<DataBlock>& set) {
		// randomly evict a block by overwrite
		set[rand()%this->nWay_] = block;
	}

public:
	Cache(CacheConfig& config, RAM& ram) :
		nWay_(config.nWay), cacheSize_(config.cacheSize), blockSize_(config.blockSize),
		numBlocks_(config.cacheBlockCount), numSets_(config.numSets), ram_(ram) {

		srand (time(NULL));
		this->blocks_.resize(this->numSets_, std::vector<DataBlock>(this->nWay_));
		this->valid_.resize(this->numSets_, std::vector<bool>(this->nWay_, false));
		this->tags_.resize(this->numSets_, std::vector<uint32_t>(this->nWay_));
	}

	double GetDouble(Address& address) {
		uint32_t setIndex = address.GetSet();
		uint32_t wordIndex = address.GetWord();
		uint32_t tag = address.GetTag();
		std::vector<DataBlock>& set = this->blocks_[setIndex];
		std::vector<bool>& valids = this->valid_[setIndex];
		std::vector<uint32_t>& tags = this->tags_[setIndex];

		for (uint32_t i=0; i<this->nWay_; ++i) {
			if (valids[i] && tags[i]==tag) { // cache hit
				return set[i].GetWord(wordIndex);
			}
		}
		// cache miss: fetch from RAM.
		// Note: I'm returning a new DataBlock object,
		// not a pointer to the one in RAM.
		// this is to simulate the write from RAM to cache that occurs
		// on a cache miss
		DataBlock block = ram_.GetBlockCopy(address);
		this->RandomWrite(block, set);
		return block.GetWord(wordIndex);
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
	CPU(CacheConfig& config) {
		DataBlock::StaticInit(config);
		Address::StaticInit(config);
		this->ram_ = std::unique_ptr<RAM>{ new RAM(config) };
		this->cache_ = std::unique_ptr<Cache>{
			new Cache(config, *this->ram_)
		};
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
