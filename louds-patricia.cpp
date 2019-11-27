#include "louds-patricia.hpp"

#ifdef _MSC_VER
 #include <intrin.h>
 #include <immintrin.h>
#else  // _MSC_VER
 #include <x86intrin.h>
#endif  // _MSC_VER

#include <cassert>
#include <queue>
#include <vector>

namespace louds {
namespace {

uint64_t Popcnt(uint64_t x) {
#ifdef _MSC_VER
  return __popcnt64(x);
#else  // _MSC_VER
  return __builtin_popcountll(x);
#endif  // _MSC_VER
}

uint64_t Ctz(uint64_t x) {
#ifdef _MSC_VER
  return _tzcnt_u64(x);
#else  // _MSC_VER
  return __builtin_ctzll(x);
#endif  // _MSC_VER
}

struct BitVector {
  struct Rank {
    uint32_t abs_hi;
    uint8_t abs_lo;
    uint8_t rels[3];

    uint64_t abs() const {
      return ((uint64_t)abs_hi << 8) | abs_lo;
    }
    void set_abs(uint64_t abs) {
      abs_hi = (uint32_t)(abs >> 8);
      abs_lo = (uint8_t)abs;
    }
  };

  vector<uint64_t> words;
  vector<Rank> ranks;
  vector<uint32_t> selects;
  uint64_t n_bits;

  BitVector() : words(), ranks(), selects(), n_bits(0) {}

  uint64_t get(uint64_t i) const {
    return (words[i / 64] >> (i % 64)) & 1UL;
  }
  void set(uint64_t i, uint64_t bit) {
    if (bit) {
      words[i / 64] |= (1UL << (i % 64));
    } else {
      words[i / 64] &= ~(1UL << (i % 64));
    }
  }

  void add(uint64_t bit) {
    if (n_bits % 256 == 0) {
      words.resize((n_bits + 256) / 64);
    }
    set(n_bits, bit);
    ++n_bits;
  }
  // build builds indexes for rank and select.
  void build() {
    uint64_t n_blocks = words.size() / 4;
    uint64_t n_ones = 0;
    ranks.resize(n_blocks + 1);
    for (uint64_t block_id = 0; block_id < n_blocks; ++block_id) {
      ranks[block_id].set_abs(n_ones);
      for (uint64_t j = 0; j < 4; ++j) {
        if (j != 0) {
          uint64_t rel = n_ones - ranks[block_id].abs();
          ranks[block_id].rels[j - 1] = rel;
        }

        uint64_t word_id = (block_id * 4) + j;
        uint64_t word = words[word_id];
        uint64_t n_pops = Popcnt(word);
        uint64_t new_n_ones = n_ones + n_pops;
        if (((n_ones + 255) / 256) != ((new_n_ones + 255) / 256)) {
          uint64_t count = n_ones;
          while (word != 0) {
            uint64_t pos = Ctz(word);
            if (count % 256 == 0) {
              selects.push_back(((word_id * 64) + pos) / 256);
              break;
            }
            word ^= 1UL << pos;
            ++count;
          }
        }
        n_ones = new_n_ones;
      }
    }
    ranks.back().set_abs(n_ones);
    selects.push_back(words.size() * 64 / 256);
  }

  // rank returns the number of 1-bits in the range [0, i).
  uint64_t rank(uint64_t i) const {
    uint64_t word_id = i / 64;
    uint64_t bit_id = i % 64;
    uint64_t rank_id = word_id / 4;
    uint64_t rel_id = word_id % 4;
    uint64_t n = ranks[rank_id].abs();
    if (rel_id != 0) {
      n += ranks[rank_id].rels[rel_id - 1];
    }
    n += Popcnt(words[word_id] & ((1UL << bit_id) - 1));
    return n;
  }
  // select returns the position of the (i+1)-th 1-bit.
  uint64_t select(uint64_t i) const {
    const uint64_t block_id = i / 256;
    uint64_t begin = selects[block_id];
    uint64_t end = selects[block_id + 1] + 1UL;
    if (begin + 10 >= end) {
      while (i >= ranks[begin + 1].abs()) {
        ++begin;
      }
    } else {
      while (begin + 1 < end) {
        const uint64_t middle = (begin + end) / 2;
        if (i < ranks[middle].abs()) {
          end = middle;
        } else {
          begin = middle;
        }
      }
    }
    const uint64_t rank_id = begin;
    i -= ranks[rank_id].abs();

    uint64_t word_id = rank_id * 4;
    if (i < ranks[rank_id].rels[1]) {
      if (i >= ranks[rank_id].rels[0]) {
        word_id += 1;
        i -= ranks[rank_id].rels[0];
      }
    } else if (i < ranks[rank_id].rels[2]) {
      word_id += 2;
      i -= ranks[rank_id].rels[1];
    } else {
      word_id += 3;
      i -= ranks[rank_id].rels[2];
    }
    return (word_id * 64) + Ctz(_pdep_u64(1UL << i, words[word_id]));
  }

  uint64_t size() const {
    return sizeof(uint64_t) * words.size()
      + sizeof(Rank) * ranks.size()
      + sizeof(uint32_t) * selects.size();
  }
};

struct Level {
  BitVector louds;
  BitVector outs;
  vector<uint8_t> labels;
  uint64_t offset;

  Level() : louds(), outs(), labels(), offset(0) {}

  uint64_t size() const {
    return louds.size() + outs.size() + labels.size();
  }
};

struct Trie {
  vector<Level> levels;
  string last_key;
  uint64_t n_keys;
  uint64_t n_nodes;

  Trie() : levels(2), last_key(), n_keys(0), n_nodes(1) {
    levels[0].louds.add(0);
    levels[0].louds.add(1);
    levels[1].louds.add(1);
    levels[0].outs.add(0);
    levels[0].labels.push_back(' ');
  }

  void add(const std::string &key) {
    assert(n_keys == 0 || key > last_key);
    if (key.empty()) {
      levels[0].outs.set(0, 1);
      ++n_keys;
      return;
    }
    if (key.length() + 1 >= levels.size()) {
      levels.resize(key.length() + 2);
    }
    uint64_t i = 0;
    for ( ; i < key.length(); ++i) {
      uint8_t byte = key[i];
      if ((i == last_key.length()) ||
          (byte != levels[i + 1].labels.back())) {
        levels[i + 1].louds.set(levels[i + 1].louds.n_bits - 1, 0);
        levels[i + 1].louds.add(1);
        levels[i + 1].outs.add(0);
        levels[i + 1].labels.push_back(key[i]);
        ++n_nodes;
        break;
      }
    }
    for (++i; i < key.length(); ++i) {
      levels[i + 1].louds.add(0);
      levels[i + 1].louds.add(1);
      levels[i + 1].outs.add(0);
      levels[i + 1].labels.push_back(key[i]);
      ++n_nodes;
    }
    levels[i + 1].louds.add(1);
    levels[i].outs.set(levels[i].outs.n_bits - 1, 1);
    last_key = key;
    ++n_keys;
  }

  void build() {
    for (uint64_t i = 0; i < levels.size(); ++i) {
      levels[i].louds.build();
    }
  }

  uint64_t size() const {
    uint64_t size = 0;
    for (uint64_t i = 0; i < levels.size(); ++i) {
      const Level &level = levels[i];
      size += level.louds.size();
      size += level.outs.size();
      size += level.labels.size();
    }
    return size;
  }
};

struct Node {
  uint64_t level_id:24;
  uint64_t node_pos:40;
};

}  // namespace

class PatriciaImpl {
 private:
  Trie trie_;
  BitVector louds_;
  BitVector outs_;
  BitVector links_;
  vector<uint8_t> labels_;
  BitVector tail_bits_;
  vector<uint8_t> tail_bytes_;

 public:
  PatriciaImpl()
    : trie_(), louds_(), outs_(), links_(), labels_(),
      tail_bits_(), tail_bytes_() {}

  void add(const std::string &key) {
    trie_.add(key);
  }
  void build() {
    trie_.build();

    louds_.add(0);
    louds_.add(1);
    outs_.add(trie_.levels[0].outs.get(0));
    links_.add(0);
    labels_.push_back(' ');

    queue<Node> queue;
    if (!trie_.levels[1].louds.get(0)) {
      queue.push(Node{ 1, 0 });
    }
    while (!queue.empty()) {
      Node node = queue.front();
      if (node.level_id != 0) {
        while (!trie_.levels[node.level_id].louds.get(node.node_pos)) {
          louds_.add(0);
          uint64_t level_id = node.level_id;
          uint64_t node_pos = node.node_pos;
          uint64_t node_id = node_pos - trie_.levels[level_id].louds.rank(node_pos);
          labels_.push_back(trie_.levels[level_id].labels[node_id]);
          for ( ; ; ) {
            node_pos = (node_id == 0) ? 0 :
              trie_.levels[level_id + 1].louds.select(node_id - 1) + 1;
            if (trie_.levels[level_id].outs.get(node_id) ||
              !trie_.levels[level_id + 1].louds.get(node_pos + 1)) {
              break;
            }
            node_id = node_pos - node_id;
            tail_bits_.add(level_id == node.level_id);
            ++level_id;
            tail_bytes_.push_back(trie_.levels[level_id].labels[node_id]);
          }
          if (!trie_.levels[level_id + 1].louds.get(node_pos)) {
            queue.push(Node{ level_id + 1, node_pos });
          } else {
            queue.push(Node{ 0, 0 });
          }
          links_.add(level_id > node.level_id);
          outs_.add(trie_.levels[level_id].outs.get(node_id));
          ++node.node_pos;
          ++node_id;
        }
      }
      louds_.add(1);
      queue.pop();
    }

    louds_.build();
    outs_.build();
    links_.build();
    tail_bits_.add(1);
    tail_bits_.build();

    // Note: trie_ is no longer required.
  }

  int64_t lookup(const string &query) const {
    uint64_t node_id = 0;
    for (uint64_t i = 0; i < query.length(); ++i) {
      uint64_t node_pos = louds_.select(node_id) + 1;

      uint64_t end = node_pos;
      uint64_t word = louds_.words[end / 64] >> (end % 64);
      if (word == 0) {
        end += 64 - (end % 64);
        word = louds_.words[end / 64];
        while (word == 0) {
          end += 64;
          word = louds_.words[end / 64];
        }
      }
      end += Ctz(word);
      uint64_t begin = node_pos - node_id - 1;
      end = begin + end - node_pos;

      uint8_t byte = query[i];
      while (begin < end) {
        node_id = (begin + end) / 2;
        if (byte < labels_[node_id]) {
          end = node_id;
        } else if (byte > labels_[node_id]) {
          begin = node_id + 1;
        } else {
          if (links_.get(node_id)) {
            uint64_t tail_pos = tail_bits_.select(links_.rank(node_id));
            for (++i; i < query.length(); ++i) {
              if (tail_bytes_[tail_pos] != (uint8_t)query[i]) {
                return -1;
              }
              ++tail_pos;
              if (tail_bits_.get(tail_pos)) {
                break;
              }
            }
            if (i == query.length()) {
              return -1;
            }
          }
          break;
        }
      }
      if (begin >= end) {
        return -1;
      }
    }
    if (!outs_.get(node_id)) {
      return -1;
    }
    return outs_.rank(node_id);
  }

  uint64_t n_keys() const {
    return trie_.n_keys;
  }
  uint64_t n_nodes() const {
    return outs_.n_bits;
  }
  uint64_t size() const {
    uint64_t size = 0;
    size += louds_.size();
    size += outs_.size();
    size += links_.size();
    size += labels_.size();
    size += tail_bits_.size();
    size += tail_bytes_.size();
    return size;
  }
};

Patricia::Patricia() : impl_(new PatriciaImpl) {}

Patricia::~Patricia() {
  delete impl_;
}

void Patricia::add(const string &key) {
  return impl_->add(key);
}

void Patricia::build() {
  impl_->build();
}

int64_t Patricia::lookup(const string &query) const {
  return impl_->lookup(query);
}

uint64_t Patricia::n_keys() const {
  return impl_->n_keys();
}

uint64_t Patricia::n_nodes() const {
  return impl_->n_nodes();
}

uint64_t Patricia::size() const {
  return impl_->size();
}

}  // namespace louds
