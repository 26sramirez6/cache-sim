#include <memory>
#include <vector>
#include <iostream>
#include <bitset>
#include <list>
#include <unordered_map>
#define NDEBUG
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

//#define CACHE_DEBUG
uint32_t constexpr ADDRLEN = 32;
uint32_t constexpr MATS = 3;

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
	bool printSolution;
	enum Policy { LRU, FIFO, Random };
	enum Algo { daxpy, mxm, mxm_blocking };
	Policy policy;
	Algo algo;
	uint32_t wordSize;
	uint32_t ramSize;
	uint32_t ramBlockCount;
	uint32_t totalWords;
	uint32_t wordsPerBlock;
	bool runTests;

	CacheConfig(): nWay(2), cacheSize(65536),
		blockSize(64), matDims(480),
		blockFactor(32), cacheBlockCount(0), numSets(0),
		printSolution(false), policy(LRU), algo(mxm_blocking),
		wordSize(sizeof(double)), ramSize(0),
		ramBlockCount(0), totalWords(0), wordsPerBlock(0),
		runTests(false) {};

	void SetPolicy (char * _policy) {
		if (!strcmp(_policy, "LRU")) {
			this->policy = LRU;
		} else if (!strcmp(_policy, "FIFO")) {
			this->policy = FIFO;
		} else if (!strcmp(_policy, "random")) {
			this->policy = Random;
		}
	}

	void SetAlgo (char * _algo) {
		if (!strcmp(_algo, "daxpy")) {
			this->algo = daxpy;
		} else if (!strcmp(_algo, "mxm")) {
			this->algo = mxm;
		} else if (!strcmp(_algo, "mxm_blocking")) {
			this->algo = mxm_blocking;
		}
	}

	void ComputeStats() {
		// can be re-called as needed
		this->ramSize = 0;
		this->ramBlockCount = 0;
		this->cacheBlockCount = this->cacheSize / this->blockSize;
		this->numSets = this->cacheSize / this->blockSize / this->nWay;
		this->wordsPerBlock = this->blockSize / this->wordSize;
		if (this->blockSize < sizeof(double)) {
			std::cerr << "Block size cannot be less " \
					"than double. Aborting.\n";
			exit(1);
		}

		if (this->algo==mxm_blocking || this->algo==mxm) {
			this->ramSize += this->matDims*this->matDims*this->wordSize*MATS;
		} else {
			this->ramSize += this->matDims*this->wordSize*MATS;
		}
		this->ramSize += (this->ramSize%blockSize);
		this->ramBlockCount = this->ramSize / this->blockSize;
		this->totalWords = this->ramBlockCount * this->wordsPerBlock;
		// the block factor can't be greater than the size of the individual matrices
		if (this->algo==mxm_blocking) {
			assert(this->blockFactor<=this->totalWords/MATS);
			assert(this->matDims%this->blockFactor==0);
		}
	}

	void PrintStats() const {
		std::cout << "INPUTS" << std::string(25, '=') << std::endl;
		std::cout << "Ram Size: " << this->ramSize << std::endl;
		std::cout << "Cache Size: " << this->cacheSize << std::endl;
		std::cout << "Block Size: " << this->blockSize << std::endl;
		std::cout << "Total Blocks in Cache: " << this->cacheBlockCount << std::endl;
		std::cout << "Total Blocks in RAM: " << this->ramBlockCount << std::endl;
		std::cout << "Associativity: " << this->nWay << std::endl;
		std::cout << "Number of Sets: " << this->numSets << std::endl;

		std::string p;
		switch (this->policy) {
		case LRU:
			p = "LRU";
			break;
		case FIFO:
			p = "FIFO";
			break;
		case Random:
			p = "Random";
			break;
		default:
			break;
		}
		std::cout << "Replacement Policy: " << p << std::endl;

		std::string a;
		switch (this->algo) {
		case daxpy:
			a = "daxpy";
			break;
		case mxm:
			a = "mxm";
			break;
		case mxm_blocking:
			a = "mxm_blocking";
			break;
		default:
			break;
		}

		std::cout << "Algorithm: " << a << std::endl;
		std::cout << "MXM Blocking Factor: " << this->blockFactor << std::endl;
		std::cout << "Matrix or Vector dimension: " << this->matDims << std::endl;
		std::cout << "Total Words: " << this->totalWords << std::endl;
	}
};

class Address {
private:


	// field sizes in bits statically stored
	// once cache size is known
	static uint32_t byteFieldSize_;
	static uint32_t wordFieldSize_;
	static uint32_t tagFieldSize_;
	static uint32_t indexFieldSize_;
	static uint32_t setFieldSize_;
	static uint32_t cacheBlockFieldSize_;
	static uint32_t ramBlockFieldSize_;

	// masks for each field statically
	// stored once cache size is known
	static uint32_t byteMask_;
	static uint32_t tagMask_;
	static uint32_t indexMask_;
	static uint32_t wordMask_;
	static uint32_t setMask_;
	static uint32_t cacheBlockMask_;
	static uint32_t ramBlockMask_;

	// right shift factors statically stored
	static uint32_t wordShift_;
	static uint32_t setShift_;
	static uint32_t cacheBlockShift_;
	static uint32_t tagShift_;
	static uint32_t ramBlockShift_;
public:
	const uint32_t address_;

#ifndef NDEBUG
	Address(uint32_t address) : address_(address) {	this->Assert();	}
#else
	Address(uint32_t address) : address_(address) {}
#endif

	Address (const Address& other) : address_(other.address_) {	}

	Address (const Address&& other) : address_(std::move(other.address_)){ }

	Address& operator=(const Address& other) = default;

	Address& operator=(Address&& other) = default;

	void PrintAddress() const {
		std::cout << this->address_ << std::endl;
	}

	uint32_t GetTag() const {
		return (this->address_&Address::tagMask_)>>Address::tagShift_;
	}

	uint32_t GetSet() const {
		return (this->address_&Address::setMask_)>>Address::setShift_;
	}

	uint32_t GetCacheFullIndex() const {
		return (this->address_&Address::indexMask_)>>Address::setShift_;
	}

	uint32_t GetCacheBlock() const {
		return (this->address_&Address::cacheBlockMask_)>>Address::cacheBlockShift_;
	}

	uint32_t GetRamBlock() const {
		return (this->address_&Address::ramBlockMask_)>>Address::ramBlockShift_;
	}

	uint32_t GetWord() const {
		return (this->address_&Address::wordMask_)>>Address::wordShift_;
	}

	static void StaticInit(const CacheConfig& config) {
		indexFieldSize_ = GetBitLength(config.cacheBlockCount) - 1;
		wordFieldSize_ = GetBitLength(config.wordsPerBlock) - 1;
		setFieldSize_ = GetBitLength(config.numSets) - 1;
		byteFieldSize_ = GetBitLength(config.wordSize) - 1;
		ramBlockFieldSize_ = GetBitLength(config.ramBlockCount);
		cacheBlockFieldSize_ = indexFieldSize_ - setFieldSize_;
		tagFieldSize_ = ADDRLEN - setFieldSize_ - wordFieldSize_ - byteFieldSize_;

		byteMask_ = (1 << byteFieldSize_) - 1;
		wordMask_ = ((1 << (wordFieldSize_ + byteFieldSize_)) - 1)&(~byteMask_);
		indexMask_ = ((1 << (indexFieldSize_ + wordFieldSize_ + byteFieldSize_)) - 1)&(~(wordMask_|byteMask_));
		tagMask_ = ~((1 << (ADDRLEN - tagFieldSize_)) - 1);
		setMask_ = ((1 << (setFieldSize_ + wordFieldSize_ + byteFieldSize_)) - 1)&(~(wordMask_|byteMask_));
		cacheBlockMask_ = ~(byteMask_|wordMask_|setMask_|tagMask_);
		ramBlockMask_ = ((1 << (byteFieldSize_ + wordFieldSize_ + ramBlockFieldSize_)) - 1)&
				(~((1 << (byteFieldSize_ + wordFieldSize_)) - 1));

		wordShift_ = byteFieldSize_;
		setShift_ = wordFieldSize_ + byteFieldSize_;
		cacheBlockShift_ = byteFieldSize_ + wordFieldSize_ + setFieldSize_;
		tagShift_ = setFieldSize_ + wordFieldSize_ + byteFieldSize_;
		ramBlockShift_ = wordFieldSize_ + byteFieldSize_;;

#ifdef CACHE_DEBUG
		std::cout << "word shift: " << wordShift_ << std::endl;
		std::cout << "set shift: " << setShift_ << std::endl;
		std::cout << "cacheblock shift: " << cacheBlockShift_ << std::endl;
		std::cout << "tag shift: " << tagShift_ << std::endl;
		std::cout << "ramblock shift: " << ramBlockShift_ << std::endl;
		std::cout << "byte mask:             " << std::bitset<ADDRLEN>(byteMask_) << std::endl;
		std::cout << "word mask:             " << std::bitset<ADDRLEN>(wordMask_) << std::endl;
		std::cout << "set mask:              " << std::bitset<ADDRLEN>(setMask_) << std::endl;
		std::cout << "cache block mask:      " << std::bitset<ADDRLEN>(cacheBlockMask_) << std::endl;
		std::cout << "cache full index mask: " << std::bitset<ADDRLEN>(indexMask_) << std::endl;
		std::cout << "cache tag mask:        " << std::bitset<ADDRLEN>(tagMask_) << std::endl;
		std::cout << "ram block mask:        " << std::bitset<ADDRLEN>(ramBlockMask_) << std::endl;
#endif
		Address::Assert();
	}

	static void Assert() {
		assert(Address::indexFieldSize_!=0);
		assert(Address::tagFieldSize_!=0);
		assert(Address::tagMask_!=0);
		assert(Address::indexMask_!=0);
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
uint32_t Address::wordShift_ = 0;
uint32_t Address::setShift_ = 0;
uint32_t Address::cacheBlockShift_ = 0;
uint32_t Address::tagShift_ = 0;
uint32_t Address::ramBlockShift_ = 0;


class DataBlock {
private:
	static uint32_t size_;
	static uint32_t numWords_;

public:
	std::vector<double> data_;
	DataBlock() : data_(numWords_) { assert(this->size_!=0); }
	~DataBlock() { }
	DataBlock(DataBlock&& other) : data_(std::move(other.data_)) {
//		std::cout<<"Datablock move ctor called"<<std::endl;
	}
	DataBlock(const DataBlock &other) : data_(other.data_) {
//		std::cout<<"Datablock copy ctor called"<<std::endl;
	}
	DataBlock& operator=(const DataBlock& other) = default;
	DataBlock& operator=(DataBlock&& other) = default;

	double GetWord(const uint32_t offset) const { return this->data_[offset]; }
	void SetWord(const uint32_t offset, const double value) {
		this->data_[offset] = value;
	}

	static void StaticInit(const CacheConfig& config) {
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
	const uint32_t size_;

public:
	std::vector<DataBlock> blocks_;
	RAM(const CacheConfig& config) : size_(config.ramSize), blocks_(config.ramBlockCount) {}

	DataBlock GetBlockCopy(const Address& address) const {
		return DataBlock(blocks_[address.GetRamBlock()]);
	}

	void SetWord(const Address& address, const uint32_t offset, const double val) {
		this->blocks_[address.GetRamBlock()].SetWord(offset, val);
	}
};


class Cache {
protected:

	struct CacheLine {
		DataBlock dataBlock_;
		const uint32_t tag_;
		CacheLine(DataBlock& dataBlock, const uint32_t tag) : dataBlock_(dataBlock), tag_(tag) {}
		CacheLine(const CacheLine& other) : dataBlock_(other.dataBlock_), tag_(other.tag_) {}
		CacheLine(CacheLine&& other) : dataBlock_(other.dataBlock_), tag_(other.tag_) {}
		CacheLine& operator=(const CacheLine& other) = default;
		CacheLine& operator=(CacheLine&& other) = default;
		~CacheLine() { }
	};

	const uint32_t nWay_;
	const uint32_t cacheSize_;
	const uint32_t blockSize_;
	const uint32_t numBlocks_;
	const uint32_t numSets_;
	unsigned long long rhits_;
	unsigned long long rmisses_;
	unsigned long long whits_;
	unsigned long long wmisses_;

	RAM & ram_;

	std::vector< std::list<CacheLine> > blocks_;
	std::vector< std::unordered_map<uint32_t, std::list<CacheLine>::iterator> > maps_;

	Cache(const CacheConfig& config, RAM& ram) :
		nWay_(config.nWay), cacheSize_(config.cacheSize),
		blockSize_(config.blockSize), numBlocks_(config.cacheBlockCount),
		numSets_(config.numSets), rhits_(0), rmisses_(0),
		whits_(0), wmisses_(0), ram_(ram), blocks_(config.numSets), maps_(config.numSets) {

		srand (time(NULL));
	}

public:
	virtual ~Cache() {};
	virtual double GetDouble(const Address& address) = 0;
	virtual void SetDouble(const Address& address, const double val) = 0;
	// factory pattern
	static std::unique_ptr<Cache> Create(const CacheConfig& config, RAM& ram);

	void PrintStats() const {
		std::cout << "RESULTS" << std::string(25, '=') << std::endl;
		std::cout << "Instruction Count: " <<
			this->wmisses_ + this->whits_ + this->rmisses_ + this->rhits_
				<< std::endl;
		std::cout << "Read hits: " << this->rhits_ <<std::endl;
		std::cout << "Read misses: " << this->rmisses_ <<std::endl;
		std::cout << "Read miss rate: " <<
			static_cast<double>(this->rmisses_) / (this->rhits_ + this->rmisses_)
				<<std::endl;
		std::cout << "Write hits: " << this->whits_ <<std::endl;
		std::cout << "Write misses: " << this->wmisses_ <<std::endl;
		std::cout << "Write miss rate: " <<
			static_cast<double>(this->wmisses_) / (this->whits_ + this->wmisses_)
				<<std::endl;
	}
};

class LRUCache : public Cache {
public:
	LRUCache(const CacheConfig& config, RAM& ram) : Cache(config, ram) {};

	double GetDouble(const Address& address) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		//O(1) search
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		if (hit!=map.end()) { // cache hit
			++this->rhits_;
			// move the hit to the front of the list
			CacheLine cl(*(hit->second));
			list.erase(hit->second);
			list.push_front(cl);
			// update the stored iterator
			map[tag] = list.begin();
			assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(address.GetWord())==cl.dataBlock_.GetWord(address.GetWord()));
			return cl.dataBlock_.GetWord(address.GetWord());
		}

		// cache miss: fetch from RAM.
		++this->rmisses_;

		if (list.size() == this->nWay_) {
			// need to evict back of list (LRU)
			CacheLine& evicted = list.back();
			list.pop_back();
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		// Note: copy constructor is called.
		// a copied block from the one in RAM.
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		// Note: this cacheline holds a reference
		// to this the copied block
		CacheLine line(newBlock, address.GetTag());
		assert(line.dataBlock_.GetWord(address.GetWord())
				==newBlock.GetWord(address.GetWord()));
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		return line.dataBlock_.GetWord(address.GetWord());
	}

	void SetDouble(const Address& address, const double val) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		uint32_t wordIndex = address.GetWord();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		assert(list.size() <= this->nWay_);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		// write through + write allocate:
		// RAM needs to be updated no matter what
		this->ram_.SetWord(address, wordIndex, val);
		assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(wordIndex)==val);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		if (hit!=map.end()) { // cache hit
			// move the hit to the front of the list
			++this->whits_;
			CacheLine cl(*(hit->second));
			cl.dataBlock_.SetWord(wordIndex, val);
			assert(cl.dataBlock_.GetWord(address.GetWord())==val);
			// move the hit to the front of the list
			list.erase(hit->second);
			list.push_front(cl);
			map[tag] = list.begin();
			return;
		}
		// cache miss, bring in the new block from ram
		++this->wmisses_;
		if (list.size() == this->nWay_) {
			// need to evict back of list (LRU)
			CacheLine& evicted = list.back();
			list.pop_back();
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		CacheLine line(newBlock, address.GetTag());
		assert(newBlock.GetWord(address.GetWord())==val);
		assert(line.dataBlock_.GetWord(address.GetWord())==val);
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		assert(list.size()<=this->nWay_);
		return;
	}
};

class FIFOCache : public Cache {
public:
	FIFOCache(const CacheConfig& config, RAM& ram) : Cache(config, ram) {};
private:
	void SetDouble(const Address& address, const double val) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		uint32_t wordIndex = address.GetWord();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		assert(list.size() <= this->nWay_);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		// write through + write allocate:
		// RAM needs to be updated no matter what
		this->ram_.SetWord(address, wordIndex, val);
		assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(wordIndex)==val);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		// **** CACHE HIT ****
		if (hit!=map.end()) {
			// dont bother reordering the queue since its FIFO
			++this->whits_;
			hit->second->dataBlock_.SetWord(wordIndex, val);
			assert(hit->second->dataBlock_.GetWord(wordIndex)==val);
			return;
		}
		// **** CACHE MISS ****
		// cache miss, bring in the new block from ram
		++this->wmisses_;
		if (list.size() == this->nWay_) {
			// need to evict back of list (FIFO)
			CacheLine& evicted = list.back();
			list.pop_back();
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		CacheLine line(newBlock, address.GetTag());
		assert(newBlock.GetWord(address.GetWord())==val);
		assert(line.dataBlock_.GetWord(address.GetWord())==val);
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		assert(list.size()<=this->nWay_);
		return;
	}

	double GetDouble(const Address& address) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		//O(1) search
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		// *** CACHE LOAD HIT ***
		if (hit!=map.end()) { // cache hit
			++this->rhits_;
			assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(address.GetWord())==
					hit->second->dataBlock_.GetWord(address.GetWord()));
			return hit->second->dataBlock_.GetWord(address.GetWord());
		}

		// *** CACHE LOAD MISS ***
		// cache miss: fetch from RAM.
		++this->rmisses_;

		if (list.size() == this->nWay_) {
			// need to evict back of list (FIFO)
			CacheLine& evicted = list.back();
			list.pop_back();
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		// Note: copy constructor is called.
		// a copied block from the one in RAM.
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		CacheLine line(newBlock, address.GetTag());
		assert(line.dataBlock_.GetWord(address.GetWord())==newBlock.GetWord(address.GetWord()));
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		return line.dataBlock_.GetWord(address.GetWord());
	}
};

class RandomCache : public Cache {
public:
	RandomCache(const CacheConfig& config, RAM& ram) : Cache(config, ram) {};
private:

	void SetDouble(const Address& address, const double val) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		uint32_t wordIndex = address.GetWord();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		assert(list.size() <= this->nWay_);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		// write through + write allocate:
		// RAM needs to be updated no matter what
		this->ram_.SetWord(address, wordIndex, val);
		assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(wordIndex)==val);
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		// **** CACHE HIT ****
		if (hit!=map.end()) {
			// dont bother reordering the queue since its FIFO
			++this->whits_;
			hit->second->dataBlock_.SetWord(wordIndex, val);
			assert(hit->second->dataBlock_.GetWord(wordIndex)==val);
			return;
		}
		// **** CACHE MISS ****
		// cache miss, bring in the new block from ram
		++this->wmisses_;
		if (list.size() == this->nWay_) {
			// evict block at random
			std::list<CacheLine>::iterator it = list.begin();
			for (uint32_t i=0; i<rand()%this->nWay_; ++i) ++it;
			CacheLine& evicted = *it;
			list.erase(it);
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		CacheLine line(newBlock, address.GetTag());
		assert(newBlock.GetWord(address.GetWord())==val);
		assert(line.dataBlock_.GetWord(address.GetWord())==val);
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		assert(list.size()<=this->nWay_);
		return;
	}

	double GetDouble(const Address& address) {
		uint32_t setIndex = address.GetSet();
		uint32_t tag = address.GetTag();
		std::list<CacheLine>& list = this->blocks_[setIndex];
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>& map = this->maps_[setIndex];

		//O(1) search
		std::unordered_map<uint32_t, std::list<CacheLine>::iterator>::const_iterator hit = map.find(tag);
		// *** CACHE LOAD HIT ***
		if (hit!=map.end()) { // cache hit
			++this->rhits_;
			assert(this->ram_.blocks_[address.GetRamBlock()].GetWord(address.GetWord())==
					hit->second->dataBlock_.GetWord(address.GetWord()));
			return hit->second->dataBlock_.GetWord(address.GetWord());
		}

		// *** CACHE LOAD MISS ***
		// cache miss: fetch from RAM.
		++this->rmisses_;

		if (list.size() == this->nWay_) {
			// evict block at random
			std::list<CacheLine>::iterator it = list.begin();
			for (uint32_t i=0; i<rand()%this->nWay_; ++i) ++it;
			CacheLine& evicted = *it;
			list.erase(it);
#ifdef NDEBUG
			map.erase(evicted.tag_);
#else
			int ret = map.erase(evicted.tag_);
#endif
			assert(ret==1);
			assert(map.count(evicted.tag_)==0);
		}
		// Note: copy constructor is called.
		// a copied block from the one in RAM.
		DataBlock newBlock(this->ram_.GetBlockCopy(address));
		CacheLine line(newBlock, address.GetTag());
		assert(line.dataBlock_.GetWord(address.GetWord())==newBlock.GetWord(address.GetWord()));
		list.push_front(line);
		// update the map with new block
		map[tag] = list.begin();
		return line.dataBlock_.GetWord(address.GetWord());
	}
};

std::unique_ptr<Cache> Cache::Create(const CacheConfig& config, RAM& ram) {
	if (config.policy == config.LRU) {
		return std::unique_ptr<Cache> { new LRUCache(config, ram) };
	} else if (config.policy == config.FIFO) {
		return std::unique_ptr<Cache> { new FIFOCache(config, ram) };
	} else if (config.policy == config.Random) {
		return std::unique_ptr<Cache> { new RandomCache(config, ram) };
	}
	throw std::invalid_argument("bad policy");
}


class CPU {
private:
	std::unique_ptr<RAM> ram_;
	std::unique_ptr<Cache> cache_;
	const CacheConfig& config_;

public:
	CPU(const CacheConfig& config) : config_(config) {
		DataBlock::StaticInit(config);
		Address::StaticInit(config);
		this->ram_ = std::unique_ptr<RAM>{ new RAM(config) };
		this->cache_ = Cache::Create(config, *this->ram_);
	}

	double LoadDouble(const Address& address) {
#ifdef CACHE_DEBUG
		std::cout << "reading address: " << address.address_ <<
				" set: " << address.GetSet() <<
				" block index: " << address.GetCacheBlock() <<
				" tag: " << address.GetTag() <<
				" word: " << address.GetWord() <<
				" ram block: " << address.GetRamBlock() << std::endl;
#endif
		return this->cache_->GetDouble(address);
	}

	void StoreDouble(Address& address, double value) {
#ifdef CACHE_DEBUG
		std::cout << "storing " << value <<
			" in address: " << address.address_ <<
			" set: " << address.GetSet() <<
			" block index: " << address.GetCacheBlock() <<
			" tag: " << address.GetTag() <<
			" word: " << address.GetWord() <<
			" ram block: " << address.GetRamBlock() << std::endl;
#endif
		this->cache_->SetDouble(address, value);
	}

	double AddDouble(double val1, double val2) const {
		return val1 + val2;
	}

	double MultDouble(double val1, double val2) const {
		return val1*val2;
	}

	void PrintStats() const {
		this->config_.PrintStats();
		this->cache_->PrintStats();
	}
};
