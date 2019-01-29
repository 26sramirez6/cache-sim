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

#define CACHE_DEBUG;
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
	uint32_t numBlocks;
	uint32_t numSets;
	uint32_t ramSize;
	uint32_t wordsPerBlock;
	uint32_t blocksInRam;
	bool logging;
	std::string policy;
	std::string algo;

	CacheConfig(): nWay(2), cacheSize(65536),
		blockSize(64), matDims(480),
		blockFactor(32), logging(false),
		policy("LRU"), algo("mxm_block") {

		this->numBlocks = cacheSize / blockSize;
		this->numSets = cacheSize / blockSize / nWay;
		this->wordsPerBlock = this->blockSize / sizeof(double);

		// initialized on ComputeRAMStats()
		this->ramSize = 0;
		this->blocksInRam = 0;
	};

	void ComputeRAMStats() {
		// can be re-called as needed
		this->ramSize = 0;
		this->blocksInRam = 0;
		if (this->policy=="mxm_block" || this->policy=="mxm") {
			this->ramSize += this->matDims*this->matDims*sizeof(double)*3;
		} else {
			this->ramSize += this->matDims*sizeof(double)*3;
		}
		this->ramSize += (this->ramSize%blockSize);
		this->blocksInRam = this->ramSize / this->blockSize;
	}
};

class Address {
private:
	uint32_t address_;

	// field sizes in bits statically stored
	// once cache size is known
	static uint32_t wordFieldSize_;
	static uint32_t tagFieldSize_;
	static uint32_t indexFieldSize_;
	static uint32_t setFieldSize_;
	static uint32_t blockWithinSetFieldSize_;

	// masks for each field statically
	// stored once cache size is known
	static uint32_t tagMask_;
	static uint32_t indexMask_;
	static uint32_t wordMask_;
	static uint32_t setMask_;
	static uint32_t blockWithinSetMask_;
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

	inline uint32_t GetBlockWithinSet() const {
		return (this->address_&Address::blockWithinSetMask_)>>
				(Address::wordFieldSize_ + Address::setFieldSize_);
	}

	inline uint32_t GetWordWithinBlock() const {
		return this->address_&Address::wordMask_;
	}

	static void StaticInit(CacheConfig& config) {
		indexFieldSize_ = GetBitLength(config.numBlocks) - 1;
		wordFieldSize_ = GetBitLength(config.wordsPerBlock) - 1;
		setFieldSize_ = GetBitLength(config.numSets) - 1;
		blockWithinSetFieldSize_ = indexFieldSize_ - setFieldSize_;
		tagFieldSize_ = ADDRLEN - indexFieldSize_ - wordFieldSize_;


		wordMask_ = (1 << wordFieldSize_) - 1;
		indexMask_ = ((1 << (indexFieldSize_ + wordFieldSize_)) - 1)&(~wordMask_);
		tagMask_ = ~((1 << (ADDRLEN - tagFieldSize_)) - 1);
		setMask_ = ((1 << (setFieldSize_ + wordFieldSize_)) - 1)&(~wordMask_);
		blockWithinSetMask_ = ~(setMask_|wordMask_|tagMask_);

#ifdef CACHE_DEBUG
		std::cout << "word mask:             " << std::bitset<ADDRLEN>(wordMask_) << std::endl;
		std::cout << "index mask:            " << std::bitset<ADDRLEN>(indexMask_) << std::endl;
		std::cout << "tag mask:              " << std::bitset<ADDRLEN>(tagMask_) << std::endl;
		std::cout << "set mask:              " << std::bitset<ADDRLEN>(setMask_) << std::endl;
		std::cout << "block within set mask: " << std::bitset<ADDRLEN>(blockWithinSetMask_) << std::endl;
#endif

		assert(ADDRLEN > indexFieldSize_ + wordFieldSize_);
		assert(setFieldSize_ + blockWithinSetFieldSize_==indexFieldSize_);
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
uint32_t Address::setMask_ = 0;
uint32_t Address::blockWithinSetMask_ = 0;
uint32_t Address::setFieldSize_ = 0;
uint32_t Address::blockWithinSetFieldSize_ = 0;


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
	RAM(CacheConfig& config) : size_(config.ramSize), blocks_(config.blocksInRam) {}

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
		numBlocks_(config.numBlocks), numSets_(config.numSets), ram_(ram) {

		srand (time(NULL));
		this->blocks_.resize(this->numSets_, std::vector<DataBlock>(this->nWay_));
		this->valid_.resize(this->numSets_, std::vector<bool>(this->nWay_, false));
		this->tags_.resize(this->numSets_, std::vector<uint32_t>(this->nWay_));
	}

	double GetDouble(Address& address) {
		uint32_t setIndex = address.GetSet();
		uint32_t wordIndex = address.GetWordWithinBlock();
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
