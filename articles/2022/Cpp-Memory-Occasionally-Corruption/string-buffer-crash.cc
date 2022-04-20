#include <iostream>
#include <string>

class BufferReader {
 public:
  explicit BufferReader(const char* data)
      : payload_(data + sizeof(uint32_t)),
        payload_size_(*reinterpret_cast<const uint32_t*>(data)) {
    std::cout << static_cast<const void*>(data) << " -- ";
  }

  bool ReadInt32(int32_t* result) {
    if (read_index_ + sizeof(int32_t) > payload_size_) {
      return false;
    }
    *result = *reinterpret_cast<const int32_t*>(payload_ + read_index_);
    read_index_ += sizeof(int32_t);
    return true;
  }

  // Also supports ReadBool(), ReadInt16(), ReadInt64(), ReadString()...

 private:
  const char* payload_ = nullptr;
  uint32_t payload_size_ = 0;
  size_t read_index_ = 0;
};

class Response {
 public:
  std::string buffer() const { return buffer_; }

  // Also supports other Response related functions...

  static Response Generate(size_t count) {
    std::string buffer;
    buffer.resize(sizeof(uint32_t) + sizeof(int32_t) * count);
    *reinterpret_cast<uint32_t*>(&buffer[0]) = sizeof(int32_t) * count;
    for (size_t i = 0; i < count; ++i) {
      *reinterpret_cast<int32_t*>(
          &buffer[sizeof(uint32_t) + sizeof(int32_t) * i]) = 41 + i;
    }

    Response result;
    result.buffer_ = buffer;
    return result;
  }

 private:
  std::string buffer_;
};

void Foo(size_t count) {
  Response response = Response::Generate(count);

  BufferReader reader(response.buffer().data());
  for (size_t i = 0; i < count; ++i) {
    int32_t payload = 0;
    if (reader.ReadInt32(&payload)) {
      std::cout << payload;
    } else {
      std::cout << "Invalid " << i;
    }
    std::cout << (i < count - 1 ? ", " : "\n");
  }
}

int main() {
  Foo(1);
  Foo(2);
  Foo(3);
  Foo(4);
  return 0;
}
