#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 private:
  template <bool IsConst>
  class CommonIterator;

 public:
  using allocator_type = Allocator;
  using value_type = T;
  using pointer = T*;
  using difference_type = std::ptrdiff_t;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(&buffer_[info_.back_index], info_.back); }
  iterator end() { return iterator(&buffer_[info_.front_index], info_.front); }
  const_iterator begin() const {
    return const_iterator(const_cast<T**>(&buffer_[info_.back_index]),
                          const_cast<T*>(info_.back));
  }
  const_iterator end() const {
    return const_iterator(const_cast<T**>(&buffer_[info_.front_index]),
                          const_cast<T*>(info_.front));
  }
  const_iterator cbegin() const {
    return const_iterator(const_cast<T**>(&buffer_[info_.back_index]),
                          const_cast<T*>(info_.back));
  }
  const_iterator cend() const {
    return const_iterator(const_cast<T**>(&buffer_[info_.front_index]),
                          const_cast<T*>(info_.front));
  }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(cbegin());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  Deque();
  Deque(const Allocator& alloc);
  Deque(const Deque& other);
  Deque(size_t count, const Allocator& alloc = Allocator());
  Deque(size_t count, const T& value, const Allocator& alloc = Allocator());
  Deque(Deque&& other) noexcept(
      std::is_nothrow_move_constructible<allocator_type>::value);
  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator());
  ~Deque();
  Deque& operator=(const Deque& other);
  Deque& operator=(Deque&& other) noexcept(
      value_alloc_traits::propagate_on_container_move_assignment::value&&
          std::is_nothrow_move_assignable<allocator_type>::value);
  size_t size() const noexcept {
    if (buffer_.empty()) {
      return 0;
    }
    if (info_.back_index == info_.front_index) {
      return info_.front - info_.back;
    }
    size_t full_filled =
        (info_.front_index - info_.back_index - 1) * ChunkSize::kValue;
    size_t back_filled =
        ChunkSize::kValue - (info_.back - buffer_[info_.back_index]);
    size_t front_filled =
        (info_.front == nullptr) ? 0 : info_.front - buffer_[info_.front_index];
    return back_filled + full_filled + front_filled;
  };

  bool empty() const noexcept { return size() == 0; };
  allocator_type get_allocator() { return alloc_obj_; }
  T& operator[](size_t index);
  const T& operator[](size_t index) const;
  T& at(size_t index);
  const T& at(size_t index) const;
  void push_back(const T& value);
  void push_back(T&& value);
  template <typename... Args>
  void emplace_back(Args&&... args);
  void pop_back();
  void push_front(const T& value);
  void push_front(T&& value);
  template <typename... Args>
  void emplace_front(Args&&... args);
  void pop_front();
  template <typename... Args>
  iterator emplace(iterator iter, Args&&... args);
  iterator insert(iterator iter, const T& value);
  iterator erase(iterator iter);

 private:
  using alloc_traits = std::allocator_traits<allocator_type>;
  using value_allocator =
      typename alloc_traits::template rebind_alloc<value_type>;
  using value_alloc_traits = std::allocator_traits<value_allocator>;
  using chunk_allocator =
      typename value_alloc_traits::template rebind_alloc<pointer>;
  using chunk_alloc_traits = std::allocator_traits<chunk_allocator>;
  struct DequeInfo {
    T* back = nullptr;
    T* front = nullptr;
    size_t back_index = 0;
    size_t front_index = 0;
    DequeInfo() {}
    DequeInfo(const T*& back, const T*& front, size_t back_index,
              size_t front_index)
        : back(back),
          front(front),
          back_index(back_index),
          front_index(front_index) {}
    DequeInfo(size_t back_index, size_t front_index)
        : back_index(back_index), front_index(front_index) {}
    DequeInfo(const DequeInfo& info)
        : back(info.back),
          front(info.front),
          back_index(info.back_index),
          front_index(info.front_index) {}
    void assign_deque_info(const std::vector<T*, chunk_allocator>& buffer,
                           const Deque& other) {
      back_index = other.info_.back_index;
      front_index = other.info_.front_index;
      back = buffer[back_index] +
             (other.info_.back - other.buffer_[other.info_.back_index]);
      front = buffer[front_index] +
              (other.info_.front - other.buffer_[other.info_.front_index]);
    }
  };
  template <bool IsConst>
  class CommonIterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    CommonIterator() {}
    CommonIterator(T** buffer, T* begin)
        : buffer_iter_(buffer), iter_ptr_(begin) {}
    std::conditional_t<IsConst, const T&, T&> operator*() const {
      return *iter_ptr_;
    }
    std::conditional_t<IsConst, const T*, T*> operator->() const {
      return iter_ptr_;
    }
    CommonIterator& operator++();
    CommonIterator operator++(int);
    CommonIterator& operator--();
    CommonIterator operator--(int);
    CommonIterator& operator+=(difference_type num);
    CommonIterator& operator-=(difference_type num);
    CommonIterator operator+(difference_type num) const;
    CommonIterator operator-(difference_type num) const;
    friend CommonIterator operator+(difference_type num,
                                    const CommonIterator& iter) {
      return iter + num;
    }
    friend difference_type operator-(const CommonIterator& iter1,
                                     const CommonIterator& iter2) {
      if (iter1 != iter2) {
        return (iter1.buffer_iter_ - iter2.buffer_iter_) * ChunkSize::kValue +
               (iter1.iter_ptr_ - *iter1.buffer_iter_) -
               (iter2.iter_ptr_ - *iter2.buffer_iter_);
      }
      return 0;
    }
    friend bool operator<(const CommonIterator& iter1,
                          const CommonIterator& iter2) {
      return iter1.buffer_iter_ < iter2.buffer_iter_ ||
             (iter1.buffer_iter_ == iter2.buffer_iter_ &&
              iter1.iter_ptr_ < iter2.iter_ptr_);
    }
    friend bool operator>(const CommonIterator& iter1,
                          const CommonIterator& iter2) {
      return iter2 < iter1;
    }
    friend bool operator==(const CommonIterator& iter1,
                           const CommonIterator& iter2) {
      return iter1.iter_ptr_ == iter2.iter_ptr_;
    }
    friend bool operator!=(const CommonIterator& iter1,
                           const CommonIterator& iter2) {
      return !(iter1 == iter2);
    }
    friend bool operator<=(const CommonIterator& iter1,
                           const CommonIterator& iter2) {
      return !(iter2 < iter1);
    }
    friend bool operator>=(const CommonIterator& iter1,
                           const CommonIterator& iter2) {
      return !(iter1 < iter2);
    }

   private:
    T** buffer_iter_;
    T* iter_ptr_ = nullptr;
  };

  struct ChunkSize {
    static const difference_type kValue =
        sizeof(T) < 256 ? 4096 / sizeof(T) : 16;
  };

  static size_t chunks_amount(size_t num) {
    size_t min_size = 2;
    return std::max(min_size,
                    num / ChunkSize::kValue + (num % ChunkSize::kValue != 0));
  }
  void clear() noexcept;
  void reallocate();
  void base_allocation();
  void particular_clear(std::vector<T*, chunk_allocator>& new_buffer, T* begin,
                        size_t begin_index, T* end,
                        value_allocator& temp_alloc);
  [[no_unique_address]] value_allocator alloc_obj_;
  chunk_allocator chunk_alloc_;
  std::vector<T*, chunk_allocator> buffer_;
  DequeInfo info_;
};

template <typename T, typename Allocator>
void Deque<T, Allocator>::clear() noexcept {
  if (buffer_.empty()) {
    return;
  }
  if (!empty()) {
    size_t back_temp_index = info_.back_index;
    while (info_.back != info_.front) {
      value_alloc_traits::destroy(alloc_obj_, info_.back);
      if (info_.back == buffer_[back_temp_index] + ChunkSize::kValue - 1) {
        ++back_temp_index;
        info_.back = buffer_[back_temp_index];
      } else {
        ++info_.back;
      }
    }
  }
  for (size_t i = 0; i < buffer_.size(); ++i) {
    if (buffer_[i] != nullptr) {
      value_alloc_traits::deallocate(alloc_obj_, buffer_[i], ChunkSize::kValue);
    }
  }
  buffer_.clear();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::base_allocation() {
  for (size_t i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
  }
  info_.front_index = info_.back_index = 1;
  info_.back = info_.front = buffer_[info_.front_index];
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque() : buffer_(2, nullptr, chunk_alloc_) {
  base_allocation();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Allocator& alloc)
    : alloc_obj_(alloc),
      chunk_alloc_(alloc),
      buffer_(2, nullptr, chunk_alloc_) {
  base_allocation();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Deque& other)
    : alloc_obj_(value_alloc_traits::select_on_container_copy_construction(
          other.alloc_obj_)),
      chunk_alloc_(chunk_alloc_traits::select_on_container_copy_construction(
          other.chunk_alloc_)),
      buffer_(other.buffer_.size(), nullptr, other.chunk_alloc_),
      info_(other.info_.back_index, other.info_.front_index) {
  for (size_t i = info_.back_index; i <= info_.front_index; ++i) {
    buffer_[i] = value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
  }
  info_.back = buffer_[info_.back_index] +
               (other.info_.back - other.buffer_[other.info_.back_index]);
  info_.front = buffer_[info_.front_index] +
                (other.info_.front - other.buffer_[other.info_.front_index]);
  T* pointer = info_.back;
  size_t ptr_index = info_.back_index;
  try {
    for (const auto& elem : other) {
      value_alloc_traits::construct(alloc_obj_, pointer, elem);
      if (pointer == buffer_[ptr_index] + ChunkSize::kValue - 1) {
        ++ptr_index;
        pointer = buffer_[ptr_index];
      } else {
        ++pointer;
      }
    }
  } catch (...) {
    T* iter = info_.back;
    size_t iter_index = info_.back_index;
    for (; iter != pointer;) {
      value_alloc_traits::destroy(alloc_obj_, iter);
      if (iter == buffer_[iter_index] + ChunkSize::kValue - 1) {
        ++iter_index;
        iter = buffer_[iter_index];
      } else {
        ++iter;
      }
    }
    info_.back = info_.front = nullptr;
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const Allocator& alloc)
    : alloc_obj_(alloc),
      chunk_alloc_(alloc),
      buffer_(chunks_amount(count), nullptr, alloc) {
  for (size_t i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
  }

  info_.back = buffer_[info_.back_index];
  info_.front = info_.back;
  try {
    for (size_t size = 0; size < count; ++size) {
      value_alloc_traits::construct(alloc_obj_, info_.front);
      if (info_.front == buffer_[info_.front_index] + ChunkSize::kValue - 1) {
        ++info_.front_index;
        info_.front = buffer_[info_.front_index];
      } else {
        ++info_.front;
      }
    }
  } catch (...) {
    for (; info_.back != info_.front;) {
      value_alloc_traits::destroy(alloc_obj_, info_.back);
      if (++info_.back == buffer_[info_.back_index] + ChunkSize::kValue) {
        ++info_.back_index;
        info_.back = buffer_[info_.back_index];
      }
    }
    info_.back = info_.front = nullptr;
    info_.back_index = 0;
    info_.front_index = buffer_.size() - 1;
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const T& value, const Allocator& alloc)
    : alloc_obj_(alloc),
      chunk_alloc_(alloc),
      buffer_(chunks_amount(count), nullptr, alloc) {
  for (size_t i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
  }
  info_.back = buffer_[info_.back_index];
  info_.front = info_.back;
  try {
    for (size_t size = 0; size < count; ++size) {
      value_alloc_traits::construct(alloc_obj_, info_.front, value);
      if (info_.front == buffer_[info_.front_index] + ChunkSize::kValue - 1) {
        ++info_.front_index;
        info_.front = buffer_[info_.front_index];
      } else {
        ++info_.front;
      }
    }
  } catch (...) {
    for (; info_.back != info_.front;) {
      value_alloc_traits::destroy(alloc_obj_, info_.back);
      if (++info_.back == buffer_[info_.back_index] + ChunkSize::kValue) {
        ++info_.back_index;
        info_.back = buffer_[info_.back_index];
      }
    }
    info_.back = info_.front = nullptr;
    info_.back_index = 0;
    info_.front_index = buffer_.size() - 1;
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(Deque&& other) noexcept(
    std::is_nothrow_move_constructible<allocator_type>::value)
    : alloc_obj_(std::move(other.alloc_obj_)),
      chunk_alloc_(std::move(other.chunk_alloc_)),
      buffer_(std::move(other.buffer_)),
      info_(std::move(other.info_)) {
  other.buffer_.resize(2, nullptr);
  other.base_allocation();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(std::initializer_list<T> init,
                           const Allocator& alloc)
    : alloc_obj_(alloc),
      chunk_alloc_(alloc),
      buffer_(chunks_amount(init.size()), nullptr, alloc) {
  for (size_t i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
  }
  info_.back = buffer_[info_.back_index];
  info_.front = info_.back;
  try {
    for (const auto& elem : init) {
      value_alloc_traits::construct(alloc_obj_, info_.front, elem);
      if (info_.front == buffer_[info_.front_index] + ChunkSize::kValue - 1) {
        ++info_.front_index;
        info_.front = buffer_[info_.front_index];
      } else {
        ++info_.front;
      }
    }
  } catch (...) {
    for (; info_.back != info_.front;) {
      value_alloc_traits::destroy(alloc_obj_, info_.back);
      if (info_.back == buffer_[info_.back_index] + ChunkSize::kValue - 1) {
        ++info_.back_index;
        info_.back = buffer_[info_.back_index];
      }
    }
    info_.back = info_.front = nullptr;
    info_.back_index = 0;
    info_.front_index = buffer_.size() - 1;
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::~Deque() {
  clear();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::particular_clear(
    std::vector<T*, chunk_allocator>& new_buffer, T* begin, size_t begin_index,
    T* end, value_allocator& temp_alloc) {
  while (begin != end) {
    value_alloc_traits::destroy(temp_alloc, begin);
    if (begin == new_buffer[begin_index] + ChunkSize::kValue - 1) {
      ++begin_index;
      begin = new_buffer[begin_index];
    } else {
      ++begin;
    }
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(
    const Deque<T, Allocator>& other) {
  if (this == &other) {
    return *this;
  }
  value_allocator temp_alloc =
      (value_alloc_traits::propagate_on_container_copy_assignment::value)
          ? other.alloc_obj_
          : alloc_obj_;
  std::vector<T*, chunk_allocator> new_buffer(
      other.buffer_.size(), nullptr,
      (chunk_alloc_traits::propagate_on_container_copy_assignment::value)
          ? other.chunk_alloc_
          : chunk_alloc_);
  size_t min_range = 0;
  size_t max_range = 1;
  for (size_t i = std::min(min_range, other.info_.back_index);
       i <= std::max(max_range, other.info_.front_index); ++i) {
    new_buffer[i] = value_alloc_traits::allocate(temp_alloc, ChunkSize::kValue);
  }
  T* temp_ptr = new_buffer[other.info_.back_index] +
                (other.info_.back - other.buffer_[other.info_.back_index]);
  size_t temp_index = other.info_.back_index;
  T* another_ptr = other.info_.back;
  size_t another_ind = other.info_.back_index;
  try {
    while (another_ptr != other.info_.front) {
      value_alloc_traits::construct(temp_alloc, temp_ptr, *another_ptr);
      if (++another_ptr == other.buffer_[another_ind] + ChunkSize::kValue) {
        ++another_ind;
        another_ptr = other.buffer_[another_ind];
      }
      if (++temp_ptr == new_buffer[temp_index] + ChunkSize::kValue) {
        ++temp_index;
        temp_ptr = new_buffer[temp_index];
      }
    }
  } catch (...) {
    T* temp_ptr1 = new_buffer[other.info_.back_index] +
                   (other.info_.back - other.buffer_[other.info_.back_index]);
    size_t temp1_index = other.info_.back_index;
    particular_clear(new_buffer, temp_ptr1, temp1_index, temp_ptr, temp_alloc);
    for (size_t i = 0; i < new_buffer.size(); ++i) {
      if (new_buffer[i] != nullptr) {
        value_alloc_traits::deallocate(temp_alloc, new_buffer[i],
                                       ChunkSize::kValue);
      }
    }
    new_buffer.clear();
    throw;
  }
  clear();
  alloc_obj_ = temp_alloc;
  chunk_alloc_ = other.chunk_alloc_;
  buffer_ = std::move(new_buffer);
  info_.assign_deque_info(buffer_, other);
  return *this;
}

template <typename T, typename Allocator>
Deque<T, Allocator>&
Deque<T, Allocator>::operator=(Deque<T, Allocator>&& other) noexcept(
    value_alloc_traits::propagate_on_container_move_assignment::value&&
        std::is_nothrow_move_assignable<allocator_type>::value) {
  if (this == &other) {
    return *this;
  }
  clear();
  if (value_alloc_traits::propagate_on_container_move_assignment::value ||
      alloc_obj_ != other.alloc_obj_) {
    alloc_obj_ = std::move(other.alloc_obj_);
  }
  if (chunk_alloc_traits::propagate_on_container_move_assignment::value ||
      chunk_alloc_ != other.chunk_alloc_) {
    chunk_alloc_ = std::move(other.chunk_alloc_);
  }
  buffer_ = std::move_if_noexcept(other.buffer_);
  info_ = std::move(other.info_);
  other.buffer_.resize(2, nullptr);
  other.base_allocation();
  return *this;
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::operator[](size_t index) {
  size_t shift = (info_.back - buffer_[info_.back_index]) + index;
  return buffer_[info_.back_index + shift / ChunkSize::kValue]
                [shift % ChunkSize::kValue];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::operator[](size_t index) const {
  size_t shift = (info_.back - buffer_[info_.back_index]) + index;
  return buffer_[info_.back_index + shift / ChunkSize::kValue]
                [shift % ChunkSize::kValue];
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::at(size_t index) {
  if (index >= size()) {
    throw std::out_of_range("deque");
  }
  size_t shift = (info_.back - buffer_[info_.back_index]) + index;
  return buffer_[info_.back_index + shift / ChunkSize::kValue]
                [shift % ChunkSize::kValue];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::at(size_t index) const {
  if (index >= size()) {
    throw std::out_of_range("deque");
  }
  size_t shift = (info_.back - buffer_[info_.back_index]) + index;
  return buffer_[info_.back_index + shift / ChunkSize::kValue]
                [shift % ChunkSize::kValue];
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::reallocate() {
  if (buffer_.empty()) {
    buffer_.resize(2, nullptr);
    return;
  }
  std::vector<T*, chunk_allocator> new_buffer(buffer_.size() * 2, nullptr,
                                              chunk_alloc_);
  size_t new_start = buffer_.size() / 2;
  for (size_t i = 0; i < buffer_.size(); ++i) {
    new_buffer[new_start] = buffer_[i];
    buffer_[i] = nullptr;
    ++new_start;
  }
  info_.front_index += buffer_.size() / 2;
  info_.back_index += buffer_.size() / 2;
  buffer_ = std::move(new_buffer);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(const T& value) {
  emplace_back(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(T&& value) {
  emplace_back(std::move(value));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_back() {
  size_t shift = (info_.back - buffer_[info_.back_index]) + size() - 1;
  T* deleted = buffer_[info_.back_index + shift / ChunkSize::kValue] +
               (shift % ChunkSize::kValue);
  value_alloc_traits::destroy(alloc_obj_, deleted);
  if (deleted == info_.back) {
    info_.back = info_.front = buffer_[info_.back_index];
    return;
  }
  if (info_.front == buffer_[info_.front_index]) {
    if (info_.front_index - info_.back_index > 2) {
      value_alloc_traits::deallocate(alloc_obj_, buffer_[info_.front_index],
                                     ChunkSize::kValue);
      buffer_[info_.front_index] = nullptr;
    }
    --info_.front_index;
  }
  info_.front = deleted;
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_back(Args&&... args) {
  try {
    value_alloc_traits::construct(alloc_obj_, info_.front,
                                  std::forward<Args>(args)...);
  } catch (...) {
    throw;
  }
  if (++info_.front - buffer_[info_.front_index] == ChunkSize::kValue) {
    if (info_.front_index == buffer_.size() - 1) {
      reallocate();
    }
    if (buffer_[++info_.front_index] == nullptr) {
      buffer_[info_.front_index] =
          value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
    }
    info_.front = buffer_[info_.front_index];
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(const T& value) {
  emplace_front(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(T&& value) {
  emplace_front(std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_front(Args&&... args) {
  if (info_.back == buffer_[info_.back_index]) {
    if (info_.back_index == 0) {
      reallocate();
    }
    if (buffer_[--info_.back_index] == nullptr) {
      buffer_[info_.back_index] =
          value_alloc_traits::allocate(alloc_obj_, ChunkSize::kValue);
    }
    info_.back = buffer_[info_.back_index] + ChunkSize::kValue - 1;
  } else {
    --info_.back;
  }
  try {
    value_alloc_traits::construct(alloc_obj_, info_.back,
                                  std::forward<Args>(args)...);
  } catch (...) {
    if (info_.back == buffer_[info_.back_index] + ChunkSize::kValue - 1) {
      value_alloc_traits::deallocate(alloc_obj_, buffer_[info_.back_index],
                                     ChunkSize::kValue);
      info_.back = buffer_[++info_.back_index];
    }
    throw;
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_front() {
  value_alloc_traits::destroy(alloc_obj_, info_.back);
  if (info_.back == buffer_[info_.back_index] + ChunkSize::kValue - 1) {
    if (info_.front_index - info_.back_index > 2) {
      value_alloc_traits::deallocate(alloc_obj_, buffer_[info_.back_index],
                                     ChunkSize::kValue);
      buffer_[info_.back_index] = nullptr;
    }
    ++info_.back_index;
    info_.back = buffer_[info_.back_index];
  } else {
    ++info_.back;
  }
  if (info_.back == info_.front) {
    info_.back = info_.front = buffer_[info_.back_index];
  }
}

template <typename T, typename Allocator>
template <typename... Args>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::emplace(
    iterator iter, Args&&... args) {
  size_t to_begin = iter - begin();
  size_t to_end = end() - iter;
  if (to_begin < to_end) {
    emplace_front(std::forward<Args>(args)...);
    auto iter1 = begin();
    auto iter2 = begin() + 1;
    for (size_t i = 0; i < to_begin; ++i) {
      std::swap(*iter1, *(iter2));
      ++iter1;
      ++iter2;
    }
  } else {
    emplace_back(std::forward<Args>(args)...);
    auto iter1 = end() - 1;
    auto iter2 = end() - 2;
    for (size_t i = 0; i < to_end; ++i) {
      std::swap(*iter1, *(iter2));
      --iter1;
      --iter2;
    }
  }
  return begin() + to_begin;
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::insert(
    iterator iter, const T& value) {
  return emplace(iter, value);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::erase(
    iterator iter) {
  if (size() == 1) {
    pop_front();
    return end();
  }
  auto returned = iter + 1;
  size_t to_begin = iter - begin();
  size_t to_end = end() - iter;
  if (to_begin <= to_end) {
    for (size_t i = 0; i < to_begin; ++i) {
      std::swap(*iter, *(--iter));
    }
    pop_front();
  } else {
    for (size_t i = 0; i < to_end; ++i) {
      std::swap(*iter, *(++iter));
    }
    pop_back();
  }
  return returned;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>&
Deque<T, Allocator>::CommonIterator<IsConst>::operator++() {
  if (++iter_ptr_ - *buffer_iter_ == ChunkSize::kValue) {
    ++buffer_iter_;
    iter_ptr_ = *buffer_iter_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>
Deque<T, Allocator>::CommonIterator<IsConst>::operator++(int) {
  CommonIterator copy = *this;
  ++(*this);
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>&
Deque<T, Allocator>::CommonIterator<IsConst>::operator--() {
  if (iter_ptr_ == *buffer_iter_) {
    --buffer_iter_;
    iter_ptr_ = *buffer_iter_ + ChunkSize::kValue;
  }
  --iter_ptr_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>
Deque<T, Allocator>::CommonIterator<IsConst>::operator--(int) {
  CommonIterator copy = *this;
  --(*this);
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>&
Deque<T, Allocator>::CommonIterator<IsConst>::operator+=(difference_type num) {
  if (num != 0) {
    num += (iter_ptr_ - *(buffer_iter_));
    if (num > 0) {
      buffer_iter_ += num / ChunkSize::kValue;
      iter_ptr_ = *(buffer_iter_) + num % ChunkSize::kValue;
    } else {
      difference_type shift = ChunkSize::kValue - num - 1;
      buffer_iter_ -= shift / ChunkSize::kValue;
      iter_ptr_ =
          *buffer_iter_ + (ChunkSize::kValue - shift % ChunkSize::kValue - 1);
    }
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>&
Deque<T, Allocator>::CommonIterator<IsConst>::operator-=(difference_type num) {
  return *this += -num;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>
Deque<T, Allocator>::CommonIterator<IsConst>::operator+(
    difference_type num) const {
  CommonIterator copy = *this;
  copy += num;
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template CommonIterator<IsConst>
Deque<T, Allocator>::CommonIterator<IsConst>::operator-(
    difference_type num) const {
  CommonIterator copy = *this;
  copy -= num;
  return copy;
}
